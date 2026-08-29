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

		if (!m_BinocularReticleRoot)
		{
			return;
		}

		deltaTime =
			MRK_BINOCULAR_RETICLE_UPDATE_MS /
			1000.0;

		/*
		* Binoculars not currently raised.
		*/
		if (!IsUsingBinoculars())
		{
			m_BinocularADSTime = 0.0;

			m_BinocularReticleRoot.SetVisible(
				false
			);

			ResetReticleProgress();

			return;
		}

		/*
		* Binocular ADS starts before the raising
		* animation visually finishes.
		*
		* Delay our custom reticle slightly.
		*/
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

		target = GetBinocularTarget();

		/*
		* Exact ray missed.
		*
		* First see whether the target we were already
		* acquiring is still close enough to the center
		* of the screen to remain "sticky".
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
				/*
				* Allow a brief miss before actually
				* throwing away progress.
				*/
				if (HandleAcquisitionLoss())
				{
					return;
				}

				if (m_BinocularReticleImage)
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

		/*
		* We've got a valid target again, so it is no
		* longer considered "lost".
		*/
		m_AcquisitionLostTime = 0.0;

		/*
		* Already tagged.
		*/
		if (IsAlreadyTagged(target))
		{
			if (m_BinocularReticleImage)
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
		* Valid new target: acquisition color.
		*/
		if (m_BinocularReticleImage)
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

		if (m_BinocularReticleProgress)
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

			if (m_BinocularReticleProgress)
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
			MRK_TAG_ACQUISITION_TIME;

		if (progress > 1.0)
		{
			progress = 1.0;
		}

		if (m_BinocularReticleProgress)
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
			MRK_TAG_ACQUISITION_TIME
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
			if (
				MRK_TargetStateService.IsFriendlyTarget(
					taggedTarget.m_TargetEntity
				)
			)
			{
				MRK_MarkerUIService.UpdateMarkerFriendlyColor(
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

		if (m_AcquisitionLostTime >= MRK_ACQUISITION_GRACE_TIME)
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
			MRK_ACQUISITION_SCREEN_RADIUS *
			MRK_ACQUISITION_SCREEN_RADIUS;

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
		if (MRK_TAG_SOUND == ResourceName.Empty)
		{
			return;
		}

		AudioSystem.PlaySound(
			MRK_TAG_SOUND
		);
	}
}
