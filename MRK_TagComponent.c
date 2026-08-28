[ComponentEditorProps(
	category: "Tagging System",
	description: "Handles client-side tagging input"
)]
class MRK_TagComponentClass : ScriptComponentClass
{
}

class MRK_TagComponent : ScriptComponent
{
	protected ref array<ref MRK_TaggedTarget> m_TaggedTargets =
		new array<ref MRK_TaggedTarget>();
	
	protected Widget m_BinocularReticleRoot;
	protected ImageWidget m_BinocularReticleImage;
	protected IEntity m_CurrentAcquisitionTarget;
	protected float m_AcquisitionTime;

	override void OnPostInit(IEntity owner)
	{
		InputManager inputManager;

		super.OnPostInit(owner);

		Print("MRK: Enemy Tag Component initialized");

		inputManager = GetGame().GetInputManager();

		if (!inputManager)
		{
			Print("MRK ERROR: No InputManager found");
			return;
		}

		inputManager.AddActionListener(
			"MRK_TagTarget",
			EActionTrigger.DOWN,
			TagTarget
		);
		
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

		return trace.TraceEnt;
	}
	
	protected void CreateBinocularReticle()
	{
		ResourceName reticleResource;

		Print("MRK RETICLE: Creating layout");

		m_BinocularReticleRoot = GetGame().GetWorkspace().CreateWidgets(
			MRK_BINOCULAR_RETICLE_UID + "UI/layouts/MRK_BinocularReticle.layout"
		);

		if (!m_BinocularReticleRoot)
		{
			Print("MRK RETICLE ERROR: Layout creation failed");
			return;
		}

		Print("MRK RETICLE: Layout created");
		
		DebugWidgetTree(m_BinocularReticleRoot);

		m_BinocularReticleImage = ImageWidget.Cast(
			m_BinocularReticleRoot.GetChildren()
		);

		if (!m_BinocularReticleImage)
		{
			Print("MRK RETICLE ERROR: Could not cast child to ImageWidget");
			return;
		}

		Print("MRK RETICLE: ReticleImage acquired from child");
		
		// DEBUG START
		Widget child;

		child = m_BinocularReticleRoot.GetChildren();

		if (child)
		{
			PrintFormat(
				"MRK RETICLE DEBUG: Child name=%1",
				child.GetName()
			);
		}
		// DEBUG END

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

			return;
		}

		Print("MRK RETICLE: Texture loaded");

		m_BinocularReticleImage.SetImage(0);

		m_BinocularReticleRoot.SetVisible(false);

		Print("MRK RETICLE: Setup complete");
	}
	
	protected void DebugWidgetTree(Widget widget, int depth = 0)
	{
		Widget child;
		string prefix;

		if (!widget)
		{
			return;
		}

		prefix = "";

		for (int i = 0; i < depth; i++)
		{
			prefix = prefix + "  ";
		}

		PrintFormat(
			"MRK UI DEBUG: %1Widget='%2'",
			prefix,
			widget.GetName()
		);

		child = widget.GetChildren();

		while (child)
		{
			DebugWidgetTree(
				child,
				depth + 1
			);

			child = child.GetSibling();
		}
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

		if (!IsUsingBinoculars())
		{
			m_BinocularReticleRoot.SetVisible(false);

			ResetTargetAcquisition();

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

			ResetTargetAcquisition();

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

			ResetTargetAcquisition();

			return;
		}

		/*
		* Valid new target.
		*/
		m_BinocularReticleImage.SetColor(
			Color.FromRGBA(0, 255, 0, 255)
		);

		/*
		* We just moved onto a new target.
		*
		* Start its acquisition timer from zero.
		*/
		if (target != m_CurrentAcquisitionTarget)
		{
			m_CurrentAcquisitionTarget = target;
			m_AcquisitionTime = 0.0;

			return;
		}

		/*
		* Still looking at the same target.
		*
		* Convert the update interval from milliseconds
		* to seconds.
		*/
		deltaTime =
			MRK_BINOCULAR_RETICLE_UPDATE_MS / 1000.0;

		m_AcquisitionTime =
			m_AcquisitionTime + deltaTime;

		PrintFormat(
			"MRK ACQUIRE: %1 / %2",
			m_AcquisitionTime,
			MRK_TAG_ACQUISITION_TIME
		);

		/*
		* Acquisition complete.
		*/
		if (m_AcquisitionTime >= MRK_TAG_ACQUISITION_TIME)
		{
			TagEntity(target);

			ResetTargetAcquisition();
		}
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