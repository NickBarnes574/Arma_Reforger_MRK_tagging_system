class MRK_TagManager
{
	protected ref array<ref MRK_TaggedTarget> m_TaggedTargets =
		new array<ref MRK_TaggedTarget>();
	
	protected Widget m_BinocularReticleRoot;
	protected ImageWidget m_BinocularReticleImage;
	protected ImageWidget m_BinocularReticleProgress;
	
	protected IEntity m_CurrentAcquisitionTarget;
	protected float m_AcquisitionTime;

	void Init()
	{
		InputManager inputManager;

		Print("MRK: Tagging system initialized");

		inputManager = GetGame().GetInputManager();

		if (!inputManager)
		{
			Print("MRK ERROR: No InputManager found");
			return;
		}
		
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
		CameraManager cameraManager;
		CameraBase currentCamera;
		BaseWorld world;
		IEntity player;
		TraceParam trace;
		vector cameraTransform[4];
		vector traceStart;
		vector traceEnd;
		vector forward;
		float traceResult;

		cameraManager = GetGame().GetCameraManager();

		if (!cameraManager)
		{
			return null;
		}

		currentCamera = cameraManager.CurrentCamera();

		if (!currentCamera)
		{
			return null;
		}

		world = GetGame().GetWorld();

		if (!world)
		{
			return null;
		}

		player = GetGame().GetPlayerController().GetControlledEntity();

		currentCamera.GetWorldCameraTransform(
			cameraTransform
		);

		traceStart = cameraTransform[3];

		forward = cameraTransform[2];

		traceEnd =
			traceStart +
			(forward * MRK_BINOCULAR_TAG_RANGE);

		trace = new TraceParam();

		trace.Start = traceStart;
		trace.End = traceEnd;

		trace.Flags =
			TraceFlags.WORLD |
			TraceFlags.ENTS;

		trace.LayerMask =
			EPhysicsLayerPresets.Projectile;

		trace.Exclude = player;

		traceResult = world.TraceMove(
			trace,
			null
		);

		if (traceResult >= 1.0)
		{
			return null;
		}

		return MRK_TargetClassifier.ResolveTaggableEntity(trace.TraceEnt);
	}
	
	protected void CreateBinocularReticle()
	{
		ResourceName reticleResource;
		ResourceName progressImageResource;
		ResourceName progressMaskResource;

		m_BinocularReticleRoot =
			GetGame().GetWorkspace().CreateWidgets(
				MRK_BINOCULAR_RETICLE_UID +
					"UI/layouts/MRK_BinocularReticle.layout"
			);

		if (!m_BinocularReticleRoot)
		{
			Print("MRK RETICLE ERROR: Layout creation failed");
			return;
		}

		m_BinocularReticleImage = ImageWidget.Cast(
			m_BinocularReticleRoot.FindAnyWidget(
				"ReticleImage"
			)
		);

		if (!m_BinocularReticleImage)
		{
			Print("MRK RETICLE ERROR: ReticleImage not found");

			m_BinocularReticleRoot.RemoveFromHierarchy();
			m_BinocularReticleRoot = null;

			return;
		}

		m_BinocularReticleProgress = ImageWidget.Cast(
			m_BinocularReticleRoot.FindAnyWidget(
				"ReticleProgress"
			)
		);

		if (!m_BinocularReticleProgress)
		{
			Print("MRK RETICLE ERROR: ReticleProgress not found");

			m_BinocularReticleRoot.RemoveFromHierarchy();

			m_BinocularReticleRoot = null;
			m_BinocularReticleImage = null;

			return;
		}

		reticleResource =
			"{7C780B2721C26EC0}UI/Textures/Editor/EditableEntities/Objectives/EditableEntity_Objective_Move.edds";

		if (!m_BinocularReticleImage.LoadImageTexture(
			0,
			reticleResource
		))
		{
			PrintFormat(
				"MRK RETICLE ERROR: Texture failed to load: %1",
				reticleResource
			);

			m_BinocularReticleRoot.RemoveFromHierarchy();

			m_BinocularReticleRoot = null;
			m_BinocularReticleImage = null;
			m_BinocularReticleProgress = null;

			return;
		}

		m_BinocularReticleImage.SetImage(0);

		/*
		* Visible progress artwork.
		*/
		progressImageResource =
			"{8A8ACADB697F8EBB}UI/Textures/RadialMenu/RadialItemForeground.edds";

		/*
		* Mask that controls how much of the progress
		* artwork is revealed.
		*/
		progressMaskResource =
			"{66618B26E3D5DBA7}UI/Textures/ProgressMasks/ProgressMaskCircular.edds";

		if (!m_BinocularReticleProgress.LoadImageTexture(
			0,
			progressImageResource
		))
		{
			PrintFormat(
				"MRK RETICLE ERROR: Progress image failed to load: %1",
				progressImageResource
			);

			m_BinocularReticleRoot.RemoveFromHierarchy();

			m_BinocularReticleRoot = null;
			m_BinocularReticleImage = null;
			m_BinocularReticleProgress = null;

			return;
		}

		m_BinocularReticleProgress.SetImage(0);

		if (!m_BinocularReticleProgress.LoadMaskTexture(
			progressMaskResource
		))
		{
			PrintFormat(
				"MRK RETICLE ERROR: Progress mask failed to load: %1",
				progressMaskResource
			);

			m_BinocularReticleRoot.RemoveFromHierarchy();

			m_BinocularReticleRoot = null;
			m_BinocularReticleImage = null;
			m_BinocularReticleProgress = null;

			return;
		}
		
		m_BinocularReticleRoot.SetZOrder(1000);

		/*
		* Start with an empty circular progress indicator.
		*/
		m_BinocularReticleProgress.SetMaskProgress(0.0);

		/*
		* Hide progress until we're actually acquiring
		* a valid target.
		*/
		m_BinocularReticleProgress.SetVisible(false);

		/*
		* Entire binocular HUD starts hidden until
		* binoculars are raised.
		*/
		m_BinocularReticleRoot.SetVisible(false);
	}

	protected void UpdateBinocularReticle()
	{
		IEntity target;
		float deltaTime;

		if (!m_BinocularReticleRoot)
		{
			return;
		}

		if (!m_BinocularReticleImage)
		{
			return;
		}

		if (!m_BinocularReticleProgress)
		{
			return;
		}

		if (!IsUsingBinoculars())
		{
			m_BinocularReticleRoot.SetVisible(false);

			ResetReticleProgress();

			return;
		}

		m_BinocularReticleRoot.SetVisible(true);

		target = GetBinocularTarget();

		/*
		* Nothing valid under the reticle.
		*/
		if (!IsTaggableTarget(target))
		{
			m_BinocularReticleImage.SetColor(
				Color.FromRGBA(255, 255, 255, 255)
			);

			ResetReticleProgress();

			return;
		}

		/*
		* Target is already tagged.
		*/
		if (IsAlreadyTagged(target))
		{
			m_BinocularReticleImage.SetColor(
				Color.FromRGBA(180, 180, 180, 255)
			);

			ResetReticleProgress();

			return;
		}

		/*
		* Valid new target.
		*/
		m_BinocularReticleImage.SetColor(
			Color.FromRGBA(226, 167, 79, 255)
		);
		
		m_BinocularReticleProgress.SetColor(
			Color.FromRGBA(226, 167, 79, 255)
		);

		/*
		* New acquisition target.
		*/
		if (target != m_CurrentAcquisitionTarget)
		{
			m_CurrentAcquisitionTarget = target;
			m_AcquisitionTime = 0.0;

			m_BinocularReticleProgress.SetVisible(true);

			return;
		}

		/*
		* Still acquiring the same target.
		*/
		m_BinocularReticleProgress.SetVisible(true);

		deltaTime =
			MRK_BINOCULAR_RETICLE_UPDATE_MS / 1000.0;

		m_AcquisitionTime =
			m_AcquisitionTime + deltaTime;
		
		float progress;

		progress =
			m_AcquisitionTime /
			MRK_TAG_ACQUISITION_TIME;

		if (progress > 1.0)
		{
			progress = 1.0;
		}

		m_BinocularReticleProgress.SetMaskProgress(
			progress
		);

		if (m_AcquisitionTime >= MRK_TAG_ACQUISITION_TIME)
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

		if (!MRK_MarkerUIService.CreateMarker(
			target,
			tagType,
			taggedTarget
		))
		{
			return;
		}

		m_TaggedTargets.Insert(taggedTarget);

		PrintFormat(
			"MRK: TARGET TAGGED! Type=%1 Total=%2",
			tagType,
			m_TaggedTargets.Count()
		);
	}
	
	protected void TagTarget()
	{
		IEntity target;

		if (!IsUsingBinoculars())
		{
			return;
		}

		target = GetBinocularTarget();

		if (!target)
		{
			return;
		}

		TagEntity(target);
	}

	protected void UpdateTaggedMarkers()
	{
		MRK_TaggedTarget taggedTarget;

		for (int idx = m_TaggedTargets.Count() - 1; idx >= 0; idx--)
		{
			taggedTarget = m_TaggedTargets[idx];

			if (!taggedTarget)
			{
				m_TaggedTargets.RemoveOrdered(idx);
				continue;
			}

			if (
				MRK_TargetStateService.ShouldRemoveTarget(
					taggedTarget.m_TargetEntity
				)
			)
			{
				MRK_MarkerUIService.DestroyMarker(
					taggedTarget
				);

				m_TaggedTargets.RemoveOrdered(idx);

				Print("MRK: Removed tagged target");

				continue;
			}

			if (!MRK_MarkerUIService.UpdatePosition(taggedTarget))
			{
				MRK_MarkerUIService.DestroyMarker(
					taggedTarget
				);

				m_TaggedTargets.RemoveOrdered(idx);
			}
		}
	}

	protected void UpdateTaggedAlertStates()
	{
		MRK_TaggedTarget taggedTarget;

		foreach (MRK_TaggedTarget currentTarget : m_TaggedTargets)
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

			MRK_MarkerUIService.UpdateMarkerAlertColor(
				taggedTarget
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
}