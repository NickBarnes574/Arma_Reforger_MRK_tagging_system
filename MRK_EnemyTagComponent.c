[ComponentEditorProps(
	category: "EnemyTagger",
	description: "Handles client-side enemy tagging input"
)]
class MRK_EnemyTagComponentClass : ScriptComponentClass
{
}

class MRK_EnemyTagComponent : ScriptComponent
{
	protected ref array<ref MRK_TaggedTarget> m_TaggedTargets =
		new array<ref MRK_TaggedTarget>();

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

	protected void TagTarget()
	{
		CameraManager cameraManager;
		CameraBase currentCamera;
		IEntity target;
		MRK_TagType tagType;
		MRK_TaggedTarget taggedTarget;

		cameraManager = GetGame().GetCameraManager();

		if (!cameraManager)
		{
			return;
		}

		currentCamera = cameraManager.CurrentCamera();

		if (!currentCamera)
		{
			return;
		}

		target = currentCamera.GetCursorTarget();

		if (!target)
		{
			return;
		}

		if (IsAlreadyTagged(target))
		{
			Print("MRK: Target already tagged");
			return;
		}

		tagType = MRK_TargetClassifier.ClassifyTarget(target);

		if (MRK_TagType.MRK_TAG_UNKNOWN == tagType)
		{
			Print("MRK: Target cannot be tagged");
			return;
		}

		if (!MRK_MarkerUIService.CreateMarker(
			target,
			tagType,
			taggedTarget
		))
		{
			Print("MRK ERROR: Failed to create marker");
			return;
		}

		m_TaggedTargets.Insert(taggedTarget);

		PrintFormat(
			"MRK: TARGET TAGGED! Type=%1 Total=%2",
			tagType,
			m_TaggedTargets.Count()
		);
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
}