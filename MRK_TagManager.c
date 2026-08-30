class MRK_TagManager
{
	protected ref array<ref MRK_TaggedTarget> m_TaggedTargets =
		new array<ref MRK_TaggedTarget>();

	protected Widget m_BinocularReticleRoot;
	protected ImageWidget m_BinocularReticleImage;
	protected ImageWidget m_BinocularReticleProgress;

	protected IEntity m_CurrentAcquisitionTarget;
	protected float m_AcquisitionTime;
	protected float m_AcquisitionLostTime;
	protected float m_BinocularADSTime;
	protected bool m_WasWeaponADS;
	protected float m_ScopeExitTime;

	void Init()
	{
		Print("MRK: Tagging system initialized");

		MRK_Settings.Init();

		CreateBinocularReticle();

		GetGame().GetCallqueue().CallLater(
			UpdateBinocularReticle,
			MRK_BINOCULAR_RETICLE_UPDATE_MS,
			true
		);

		GetGame().GetCallqueue().CallLater(
			UpdateTaggedMarkers,
			MRK_MARKER_UPDATE_MS,
			true
		);

		GetGame().GetCallqueue().CallLater(
			UpdateTaggedAlertStates,
			MRK_ALERT_UPDATE_MS,
			true
		);

	}

	protected bool IsTaggableTarget(IEntity target)
	{
		MRK_TagType tagType;

		if (!target)
		{
			return false;
		}

		tagType = MRK_TargetClassifier.ClassifyTarget(target);

		if (tagType == MRK_TagType.MRK_TAG_UNKNOWN)
		{
			return false;
		}

		return true;
	}

	protected IEntity GetBinocularTarget()
	{
		IEntity player;
		BaseWorld world;
		CameraBase currentCamera;

		vector cameraTransform[4];
		vector traceStart;
		vector traceEnd;
		vector forward;

		TraceParam trace;
		float traceResult;

		player =
			GetGame()
				.GetPlayerController()
				.GetControlledEntity();

		if (!player)
		{
			return null;
		}

		world = GetGame().GetWorld();

		if (!world)
		{
			return null;
		}

		currentCamera =
			GetGame().GetCameraManager().CurrentCamera();

		if (!currentCamera)
		{
			return null;
		}

		currentCamera.GetWorldCameraTransform(
			cameraTransform
		);

		traceStart = cameraTransform[3];
		forward = cameraTransform[2];

		traceEnd =
			traceStart +
			(
				forward *
				MRK_BINOCULAR_TAG_RANGE
			);

		trace = new TraceParam();

		trace.Start = traceStart;
		trace.End = traceEnd;

		trace.Flags =
			TraceFlags.WORLD |
			TraceFlags.ENTS;

		trace.LayerMask =
			EPhysicsLayerPresets.Projectile;

		trace.Exclude = player;

		traceResult =
			world.TraceMove(
				trace,
				null
			);

		if (traceResult >= 1.0)
		{
			return null;
		}

		return MRK_TargetClassifier.ResolveTaggableEntity(
			trace.TraceEnt
		);
	}

	protected void CreateBinocularReticle()
	{
		WorkspaceWidget workspace;

		workspace = GetGame().GetWorkspace();

		if (!workspace)
		{
			return;
		}

		m_BinocularReticleRoot =
			workspace.CreateWidgets(
				MRK_BINOCULAR_RETICLE_LAYOUT
			);

		if (!m_BinocularReticleRoot)
		{
			Print(
				"MRK: Failed to create binocular reticle"
			);

			return;
		}

		/*
		* Keep our custom binocular reticle above
		* the vanilla binocular overlay.
		*/
		m_BinocularReticleRoot.SetZOrder(1000);

		m_BinocularReticleImage =
			ImageWidget.Cast(
				m_BinocularReticleRoot.FindAnyWidget(
					"ReticleImage"
				)
			);

		m_BinocularReticleProgress =
			ImageWidget.Cast(
				m_BinocularReticleRoot.FindAnyWidget(
					"ReticleProgress"
				)
			);

		/*
		* Load the binocular reticle icon.
		*/
		if (m_BinocularReticleImage)
		{
			m_BinocularReticleImage.LoadImageTexture(
				0,
				MRK_BINOCULAR_RETICLE_IMAGE
			);

			m_BinocularReticleImage.SetImage(0);

			m_BinocularReticleImage.SetColor(
				Color.FromRGBA(
					255,
					255,
					255,
					255
				)
			);
		}

		/*
		* Load the circular acquisition progress ring
		* and its mask.
		*/
		if (m_BinocularReticleProgress)
		{
			m_BinocularReticleProgress.LoadImageTexture(
				0,
				MRK_BINOCULAR_PROGRESS_IMAGE
			);

			m_BinocularReticleProgress.SetImage(0);

			m_BinocularReticleProgress.LoadMaskTexture(
				MRK_BINOCULAR_PROGRESS_MASK
			);

			m_BinocularReticleProgress.SetMaskProgress(
				0.0
			);

			m_BinocularReticleProgress.SetVisible(
				false
			);
		}

		m_BinocularReticleRoot.SetVisible(
			false
		);
	}

	protected void DestroyBinocularReticle()
	{
		if (m_BinocularReticleRoot)
		{
			m_BinocularReticleRoot.RemoveFromHierarchy();
		}

		m_BinocularReticleRoot = null;
		m_BinocularReticleImage = null;
		m_BinocularReticleProgress = null;
	}

	protected void UpdateBinocularReticle()
	{
		IEntity target;
		float deltaTime;
		float progress;
		bool usingBinoculars;
		bool usingVehicleObserver;

		if (!m_BinocularReticleRoot)
		{
			return;
		}

		deltaTime =
			MRK_BINOCULAR_RETICLE_UPDATE_MS /
			1000.0;

		usingBinoculars =
			IsUsingBinoculars();

		usingVehicleObserver =
			IsUsingVehicleObserverView();

		/*
		 * No valid tagging view is active.
		 *
		 * Hide the binocular UI and clear any
		 * acquisition that was in progress.
		 */
		if (
			!usingBinoculars &&
			!usingVehicleObserver
		)
		{
			m_BinocularADSTime = 0.0;

			m_BinocularReticleRoot.SetVisible(
				false
			);

			ResetReticleProgress();

			return;
		}

		/*
		 * The custom reticle belongs only to the
		 * handheld binoculars.
		 *
		 * Vehicle observer/gunner sights keep their
		 * native reticle; we only reuse the
		 * acquisition/tagging logic.
		 */
		if (usingBinoculars)
		{
			m_BinocularADSTime =
				m_BinocularADSTime +
				deltaTime;

			if (
				m_BinocularADSTime <
				MRK_BINOCULAR_RETICLE_DELAY
			)
			{
				m_BinocularReticleRoot.SetVisible(
					false
				);

				return;
			}

			m_BinocularReticleRoot.SetVisible(
				true
			);
		}
		else
		{
			m_BinocularADSTime = 0.0;

			m_BinocularReticleRoot.SetVisible(
				false
			);

			if (m_BinocularReticleProgress)
			{
				m_BinocularReticleProgress.SetVisible(
					false
				);
			}
		}

		/*
		 * This trace uses the active player camera,
		 * so the same path works for binoculars and
		 * supported vehicle observer/gunner sights.
		 */
		target = GetBinocularTarget();

		/*
		 * Exact ray missed.
		 *
		 * Keep the current acquisition if the same
		 * target is still close to the center of the
		 * active view, then fall back to the short
		 * acquisition grace period.
		 */
		if (!IsTaggableTarget(target))
		{
			if (IsAcquisitionTargetNearReticle())
			{
				target =
					m_CurrentAcquisitionTarget;
			}
			else
			{
				if (HandleAcquisitionLoss())
				{
					return;
				}

				if (
					usingBinoculars &&
					m_BinocularReticleImage
				)
				{
					m_BinocularReticleImage.SetColor(
						Color.FromRGBA(
							255,
							255,
							255,
							255
						)
					);
				}

				return;
			}
		}

		m_AcquisitionLostTime = 0.0;

		/*
		 * Already-tagged targets remain non-acquirable.
		 * Only binoculars need the gray visual feedback.
		 */
		if (IsAlreadyTagged(target))
		{
			if (
				usingBinoculars &&
				m_BinocularReticleImage
			)
			{
				m_BinocularReticleImage.SetColor(
					Color.FromRGBA(
						180,
						180,
						180,
						255
					)
				);
			}

			ResetReticleProgress();

			return;
		}

		/*
		 * Binocular-only acquisition visuals.
		 */
		if (
			usingBinoculars &&
			m_BinocularReticleImage
		)
		{
			m_BinocularReticleImage.SetColor(
				Color.FromRGBA(
					226,
					167,
					79,
					255
				)
			);
		}

		if (
			usingBinoculars &&
			m_BinocularReticleProgress
		)
		{
			m_BinocularReticleProgress.SetColor(
				Color.FromRGBA(
					226,
					167,
					79,
					255
				)
			);
		}

		/*
		 * New acquisition target.
		 */
		if (
			target !=
			m_CurrentAcquisitionTarget
		)
		{
			m_CurrentAcquisitionTarget =
				target;

			m_AcquisitionTime = 0.0;
			m_AcquisitionLostTime = 0.0;

			if (
				usingBinoculars &&
				m_BinocularReticleProgress
			)
			{
				m_BinocularReticleProgress.SetMaskProgress(
					0.0
				);

				m_BinocularReticleProgress.SetVisible(
					true
				);
			}

			return;
		}

		/*
		 * Still acquiring the same target.
		 */
		m_AcquisitionTime =
			m_AcquisitionTime +
			deltaTime;

		progress =
			m_AcquisitionTime /
			MRK_Settings.TagAcquisitionTime();

		if (progress > 1.0)
		{
			progress = 1.0;
		}

		if (
			usingBinoculars &&
			m_BinocularReticleProgress
		)
		{
			m_BinocularReticleProgress.SetVisible(
				true
			);

			m_BinocularReticleProgress.SetMaskProgress(
				progress
			);
		}

		if (
			m_AcquisitionTime >=
			MRK_Settings.TagAcquisitionTime()
		)
		{
			TagEntity(target);
			ResetReticleProgress();
		}
	}

	protected void ResetReticleProgress()
	{
		if (m_BinocularReticleProgress)
		{
			m_BinocularReticleProgress.SetMaskProgress(0.0);
			m_BinocularReticleProgress.SetVisible(false);
		}

		ResetTargetAcquisition();
	}

	protected void ResetTargetAcquisition()
	{
		m_CurrentAcquisitionTarget = null;
		m_AcquisitionTime = 0.0;
		m_AcquisitionLostTime =0.0;
	}

	protected void TagEntity(IEntity target)
	{
		MRK_TagType tagType;
		MRK_TaggedTarget taggedTarget;

		if (!target)
		{
			return;
		}

		if (IsAlreadyTagged(target))
		{
			return;
		}

		tagType = MRK_TargetClassifier.ClassifyTarget(
			target
		);

		if (tagType == MRK_TagType.MRK_TAG_UNKNOWN)
		{
			return;
		}

		taggedTarget = MRK_MarkerUIService.CreateMarker(
			target,
			tagType
		);

		if (!taggedTarget)
		{
			return;
		}

		ConfigureTagBehavior(
			taggedTarget
		);

		m_TaggedTargets.Insert(taggedTarget);

		/*
		 * Play confirmation only after the
	 	 * tag was successfully created.
		 */
		PlayTagSound();

		PrintFormat(
			"MRK: TARGET TAGGED! Type=%1 Total=%2",
			tagType,
			m_TaggedTargets.Count()
		);
	}

	protected void ConfigureTagBehavior(
		MRK_TaggedTarget taggedTarget
	)
	{
		bool isVehicle;

		if (!taggedTarget)
		{
			return;
		}

		if (!taggedTarget.m_TargetEntity)
		{
			return;
		}

		isVehicle =
			taggedTarget.m_TagType !=
			MRK_TagType.MRK_TAG_INFANTRY;

		taggedTarget.m_IsFriendly =
			MRK_TargetStateService.IsFriendlyTarget(
				taggedTarget.m_TargetEntity
			);

		taggedTarget.m_IsCivilian =
			MRK_TargetStateService.IsCivilianTarget(
				taggedTarget.m_TargetEntity
			);

		/*
		 * Infantry is considered occupied by definition.
		 * Vehicle targets query their compartment slots.
		 */
		if (isVehicle)
		{
			taggedTarget.m_IsOccupied =
				MRK_TargetStateService.IsVehicleOccupied(
					taggedTarget.m_TargetEntity
				);
		}
		else
		{
			taggedTarget.m_IsOccupied = true;
		}

		taggedTarget.m_TimeAlive = 0.0;
		taggedTarget.m_Lifetime = 0.0;
		taggedTarget.m_IsPersistent = true;

		/*
		 * Empty vehicles remain logically tagged, but their HUD
		 * marker is distance-limited by MRK_MarkerUIService.
		 */
		if (
			isVehicle &&
			!taggedTarget.m_IsOccupied
		)
		{
			return;
		}

		/*
		 * Friendly targets are useful for quick identification,
		 * but they do not need to occupy HUD space indefinitely.
		 */
		if (taggedTarget.m_IsFriendly)
		{
			taggedTarget.m_IsPersistent = false;
			taggedTarget.m_Lifetime =
				MRK_Settings.FriendlyTagLifetime();

			return;
		}

		/*
		 * Neutral/civilian targets are even more temporary.
		 */
		if (taggedTarget.m_IsCivilian)
		{
			taggedTarget.m_IsPersistent = false;
			taggedTarget.m_Lifetime =
				MRK_Settings.CivilianTagLifetime();

			return;
		}

		/*
		 * Enemy infantry and occupied enemy vehicles stay tagged
		 * until the target is dead/destroyed.
		 */
		taggedTarget.m_IsPersistent = true;
	}

	protected void UpdateTaggedMarkers()
	{
		MRK_TaggedTarget taggedTarget;
		SCR_2DPIPSightsComponent pipSights;

		bool weaponADS;
		bool usePIP;

		float deltaTime;

		int pipCameraIndex;

		/*
		* Detect transition from ADS -> non-ADS.
		*/
		weaponADS = IsWeaponADS();

		deltaTime =
			MRK_MARKER_UPDATE_MS /
			1000.0;

		if (
			m_WasWeaponADS &&
			!weaponADS
		)
		{
			m_ScopeExitTime =
				MRK_SCOPE_EXIT_HIDE_TIME;
		}

		m_WasWeaponADS = weaponADS;

		/*
		* While the scope is lowering, neither the
		* PIP camera nor normal camera gives us a
		* visually stable projection.
		*
		* Hide markers until the animation finishes.
		*/
		if (m_ScopeExitTime > 0.0)
		{
			m_ScopeExitTime =
				m_ScopeExitTime -
				deltaTime;

			if (m_ScopeExitTime < 0.0)
			{
				m_ScopeExitTime = 0.0;
			}

			HideAllTaggedMarkers();

			return;
		}

		/*
		* Determine whether we're currently using
		* the PIP scope projection path.
		*/
		pipSights = GetActivePIPSights();

		usePIP = false;
		pipCameraIndex = -1;

		if (pipSights)
		{
			pipCameraIndex =
				pipSights.GetPIPCameraIndex();

			if (pipCameraIndex >= 0)
			{
				usePIP = true;
			}
		}

		for (int idx = m_TaggedTargets.Count() - 1; idx >= 0; idx--)
		{
			taggedTarget =
				m_TaggedTargets[idx];

			if (!taggedTarget)
			{
				m_TaggedTargets.RemoveOrdered(
					idx
				);

				continue;
			}

			/*
			 * Temporary tags age out automatically.
			 */
			if (!taggedTarget.m_IsPersistent)
			{
				taggedTarget.m_TimeAlive =
					taggedTarget.m_TimeAlive +
					deltaTime;

				if (
					taggedTarget.m_TimeAlive >=
					taggedTarget.m_Lifetime
				)
				{
					MRK_MarkerUIService.DestroyMarker(
						taggedTarget
					);

					m_TaggedTargets.RemoveOrdered(
						idx
					);

					continue;
				}
			}

			/*
			* Remove dead/destroyed targets.
			*/
			if (
				MRK_TargetStateService.ShouldRemoveTarget(
					taggedTarget.m_TargetEntity
				)
			)
			{
				MRK_MarkerUIService.DestroyMarker(
					taggedTarget
				);

				m_TaggedTargets.RemoveOrdered(
					idx
				);

				continue;
			}

			/*
			* PIP weapon optic.
			*
			* This should call the working projection
			* method containing the aiming-rotation
			* compensation we already established.
			*/
			if (usePIP)
			{
				if (
					!MRK_MarkerUIService.UpdatePositionWithCamera(
						taggedTarget,
						pipCameraIndex,
						pipSights
					)
				)
				{
					MRK_MarkerUIService.DestroyMarker(
						taggedTarget
					);

					m_TaggedTargets.RemoveOrdered(
						idx
					);
				}

				continue;
			}

			/*
			* Normal camera / binocular projection.
			*/
			if (
				!MRK_MarkerUIService.UpdatePosition(
					taggedTarget
				)
			)
			{
				MRK_MarkerUIService.DestroyMarker(
					taggedTarget
				);

				m_TaggedTargets.RemoveOrdered(
					idx
				);
			}
		}
	}

	protected void UpdateTaggedAlertStates()
	{
		MRK_TaggedTarget taggedTarget;
		MRK_AlertState alertState;

		foreach (
			MRK_TaggedTarget currentTarget :
			m_TaggedTargets
		)
		{
			taggedTarget = currentTarget;

			if (!taggedTarget)
			{
				continue;
			}

			if (!taggedTarget.m_TargetEntity)
			{
				continue;
			}

			/*
			* Friendlies are always blue.
			*
			* We intentionally do not ask for their
			* alert state.
			*/
			if (taggedTarget.m_IsFriendly)
			{
				MRK_MarkerUIService.UpdateMarkerFriendlyColor(
					taggedTarget
				);

				continue;
			}

			/*
			 * Civilians/neutrals stay white and do not use enemy
			 * alert-state colors.
			 */
			if (taggedTarget.m_IsCivilian)
			{
				MRK_MarkerUIService.UpdateMarkerCivilianColor(
					taggedTarget
				);

				continue;
			}

			/*
			* Enemy/other targets use normal AI
			* alert coloring.
			*/
			alertState =
				MRK_TargetStateService.GetTargetAlertState(
					taggedTarget.m_TargetEntity
				);

			MRK_MarkerUIService.UpdateMarkerAlertColor(
				taggedTarget,
				alertState
			);
		}
	}

	protected bool IsAlreadyTagged(IEntity target)
	{
		MRK_TaggedTarget taggedTarget;

		if (!target)
		{
			return false;
		}

		foreach (MRK_TaggedTarget currentTarget : m_TaggedTargets)
		{
			taggedTarget = currentTarget;

			if (!taggedTarget)
			{
				continue;
			}

			if (taggedTarget.m_TargetEntity == target)
			{
				return true;
			}
		}

		return false;
	}

	protected bool IsWeaponADS()
	{
		IEntity player;
		CharacterControllerComponent characterController;

		player = GetGame().GetPlayerController().GetControlledEntity();

		if (!player)
		{
			return false;
		}

		characterController = CharacterControllerComponent.Cast(
			player.FindComponent(
				CharacterControllerComponent
			)
		);

		if (!characterController)
		{
			return false;
		}

		return characterController.IsWeaponADS();
	}

	protected CharacterWeaponManagerComponent GetWeaponManager()
	{
		IEntity player;
		CharacterControllerComponent characterController;
		BaseWeaponManagerComponent baseWeaponManager;
		CharacterWeaponManagerComponent weaponManager;

		player = GetGame().GetPlayerController().GetControlledEntity();

		if (!player)
		{
			return null;
		}

		characterController = CharacterControllerComponent.Cast(
			player.FindComponent(
				CharacterControllerComponent
			)
		);

		if (!characterController)
		{
			return null;
		}

		baseWeaponManager =
			characterController.GetWeaponManagerComponent();

		if (!baseWeaponManager)
		{
			return null;
		}

		weaponManager =
			CharacterWeaponManagerComponent.Cast(
				baseWeaponManager
			);

		return weaponManager;
	}

	protected BaseSightsComponent GetActiveSights()
	{
		CharacterWeaponManagerComponent weaponManager;
		BaseWeaponComponent weapon;
		BaseSightsComponent sights;

		weaponManager = GetWeaponManager();

		if (!weaponManager)
		{
			return null;
		}

		weapon = weaponManager.GetCurrentWeapon();

		if (!weapon)
		{
			return null;
		}

		/*
		* Check attached optic first.
		*/
		sights = weapon.GetAttachedSights();

		if (sights && sights.IsSightADSActive())
		{
			return sights;
		}

		/*
		* Then check the weapon's native sights.
		*/
		sights = weapon.GetSights();

		if (sights && sights.IsSightADSActive())
		{
			return sights;
		}

		return null;
	}

	protected SCR_2DPIPSightsComponent GetActivePIPSights()
	{
		BaseSightsComponent sights;
		SCR_2DPIPSightsComponent pipSights;

		sights = GetActiveSights();

		if (!sights)
		{
			return null;
		}

		pipSights = SCR_2DPIPSightsComponent.Cast(
			sights
		);

		if (!pipSights)
		{
			return null;
		}

		if (!pipSights.IsPIPEnabled())
		{
			return null;
		}

		if (!pipSights.GetPIPCamera())
		{
			return null;
		}

		return pipSights;
	}

	protected bool IsUsingBinoculars()
	{
		IEntity player;
		SCR_GadgetManagerComponent gadgetManager;
		SCR_GadgetComponent gadgetComponent;
		SCR_BinocularsComponent binocularsComponent;

		player = GetGame().GetPlayerController().GetControlledEntity();

		if (!player)
		{
			return false;
		}

		gadgetManager =
			SCR_GadgetManagerComponent.GetGadgetManager(player);

		if (!gadgetManager)
		{
			return false;
		}

		/*
		* Having binoculars in the player's inventory is not enough.
		* They need to be the currently held gadget.
		*/
		gadgetComponent = gadgetManager.GetHeldGadgetComponent();

		if (!gadgetComponent)
		{
			return false;
		}

		binocularsComponent =
			SCR_BinocularsComponent.Cast(gadgetComponent);

		if (!binocularsComponent)
		{
			return false;
		}

		/*
		* Require the player to actually be aiming through
		* the binoculars.
		*/
		if (!gadgetManager.GetIsGadgetADS())
		{
			return false;
		}

		return true;
	}

	protected bool HandleAcquisitionLoss()
	{
		float deltaTime;

		if (!m_CurrentAcquisitionTarget)
		{
			return false;
		}

		deltaTime =
			MRK_BINOCULAR_RETICLE_UPDATE_MS / 1000.0;

		m_AcquisitionLostTime =
			m_AcquisitionLostTime + deltaTime;

		if (m_AcquisitionLostTime >= MRK_Settings.AcquisitionGraceTime())
		{
			ResetReticleProgress();
			return false;
		}

		return true;
	}

	protected bool IsAcquisitionTargetNearReticle()
	{
		WorkspaceWidget workspace;
		BaseWorld world;
		IEntity target;

		vector worldPosition;
		vector screenPosition;

		float screenCenterX;
		float screenCenterY;

		float deltaX;
		float deltaY;

		float distanceSquared;
		float radiusSquared;

		float markerHeight;

		MRK_TagType tagType;

		target = m_CurrentAcquisitionTarget;

		if (!target)
		{
			return false;
		}

		workspace = GetGame().GetWorkspace();
		world = GetGame().GetWorld();

		if ((!workspace) || (!world))
		{
			return false;
		}

		tagType =
			MRK_TargetClassifier.ClassifyTarget(
				target
			);

		if (
			tagType ==
			MRK_TagType.MRK_TAG_UNKNOWN
		)
		{
			return false;
		}

		worldPosition =
			target.GetOrigin();

		markerHeight =
			MRK_TargetClassifier.GetMarkerHeightForEntity(
				target,
				tagType
			);

		worldPosition[1] =
			worldPosition[1] +
			markerHeight;

		screenPosition =
			workspace.ProjWorldToScreen(
				worldPosition,
				world
			);

		if (screenPosition[2] <= 0)
		{
			return false;
		}

		screenCenterX =
			workspace.GetWidth() *
			0.5;

		screenCenterY =
			workspace.GetHeight() *
			0.5;

		deltaX =
			screenPosition[0] -
			screenCenterX;

		deltaY =
			screenPosition[1] -
			screenCenterY;

		distanceSquared =
			(deltaX * deltaX) +
			(deltaY * deltaY);

		radiusSquared =
			MRK_Settings.AcquisitionScreenRadius() *
			MRK_Settings.AcquisitionScreenRadius();

		return distanceSquared <= radiusSquared;
	}

	protected void HideAllTaggedMarkers()
	{
		MRK_TaggedTarget taggedTarget;

		foreach (MRK_TaggedTarget target : m_TaggedTargets)
		{
			taggedTarget = target;

			if (!taggedTarget)
			{
				continue;
			}

			if (!taggedTarget.m_MarkerRoot)
			{
				continue;
			}

			taggedTarget.m_MarkerRoot.SetVisible(false);
		}
	}

	protected void PlayTagSound()
	{
		if (!MRK_Settings.TagConfirmationSoundEnabled())
		{
			return;
		}

		if (MRK_TAG_SOUND == ResourceName.Empty)
		{
			return;
		}

		AudioSystem.PlaySound(
			MRK_TAG_SOUND
		);
	}




	protected bool CanUseTaggingView()
	{
		if (IsUsingBinoculars())
		{
			return true;
		}

		if (IsUsingVehicleObserverView())
		{
			return true;
		}

		return false;
	}

	protected bool IsUsingVehicleObserverView()
	{
		IEntity player;
		ChimeraCharacter character;
		CompartmentAccessComponent compartmentAccess;
		BaseCompartmentSlot compartment;
		TurretCompartmentSlot turretCompartment;
		TurretControllerComponent attachedTurret;
		BaseSightsComponent currentSights;

		player =
			GetGame()
				.GetPlayerController()
				.GetControlledEntity();

		if (!player)
		{
			return false;
		}

		character =
			ChimeraCharacter.Cast(player);

		if (!character)
		{
			return false;
		}

		compartmentAccess =
			character.GetCompartmentAccessComponent();

		if (!compartmentAccess)
		{
			return false;
		}

		if (!compartmentAccess.IsInCompartment())
		{
			return false;
		}

		compartment =
			compartmentAccess.GetCompartment();

		if (!compartment)
		{
			return false;
		}

		/*
		 * Commander/observer-style seats.
		 *
		 * The LAV commander is a CargoCompartmentSlot,
		 * but its compartment exposes an attached turret
		 * with real sights. Requiring those sights to be
		 * ADS means ordinary cargo seats do not qualify.
		 */
		attachedTurret =
			compartment.GetAttachedTurret();

		if (attachedTurret)
		{
			currentSights =
				attachedTurret.GetCurrentSights();

			if (
				currentSights &&
				attachedTurret.GetCurrentSightsADS()
			)
			{
				return true;
			}
		}

		/*
		 * Gunner-style seats.
		 *
		 * LAV, Humvee, and helicopter door-gunner
		 * TurretCompartmentSlots report their active
		 * sight state through IsInCompartmentADS().
		 */
		turretCompartment =
			TurretCompartmentSlot.Cast(
				compartment
			);

		if (
			turretCompartment &&
			compartmentAccess.IsInCompartmentADS()
		)
		{
			return true;
		}

		return false;
	}
}
