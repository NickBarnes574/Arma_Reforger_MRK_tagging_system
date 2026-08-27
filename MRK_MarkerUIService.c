class MRK_MarkerUIService
{
	static bool CreateMarker(
		IEntity target,
		MRK_TagType tagType,
		out MRK_TaggedTarget taggedTarget
	)
	{
		Widget layoutRoot;
		Widget markerRoot;
		ImageWidget markerImage;
		ResourceName iconResource;

		taggedTarget = null;

		if (!target)
		{
			return false;
		}

		layoutRoot = GetGame().GetWorkspace().CreateWidgets(
			MRK_UID + "UI/layouts/MRK_Marker.layout"
		);

		if (!layoutRoot)
		{
			Print("MRK ERROR: Failed to create marker layout");
			return false;
		}

		markerRoot = layoutRoot.FindAnyWidget(
			"MarkerRoot"
		);

		if (!markerRoot)
		{
			Print("MRK ERROR: Could not find MarkerRoot");

			layoutRoot.RemoveFromHierarchy();

			return false;
		}

		FrameSlot.SetAlignment(
			markerRoot,
			0.0,
			0.0
		);

		markerImage = ImageWidget.Cast(
			layoutRoot.FindAnyWidget("MarkerImage")
		);

		if (!markerImage)
		{
			Print("MRK ERROR: Could not find MarkerImage");

			layoutRoot.RemoveFromHierarchy();

			return false;
		}

		iconResource =
			MRK_TargetClassifier.GetIconForTagType(
				tagType
			);

		if (iconResource.IsEmpty())
		{
			Print("MRK ERROR: No icon for tag type");

			layoutRoot.RemoveFromHierarchy();

			return false;
		}

		if (!markerImage.LoadImageTexture(
			0,
			iconResource
		))
		{
			PrintFormat(
				"MRK ERROR: Failed to load marker icon: %1",
				iconResource
			);

			layoutRoot.RemoveFromHierarchy();

			return false;
		}

		markerImage.SetImage(0);

		taggedTarget = new MRK_TaggedTarget();

		taggedTarget.m_TargetEntity = target;
		taggedTarget.m_LayoutRoot = layoutRoot;
		taggedTarget.m_MarkerRoot = markerRoot;
		taggedTarget.m_MarkerImage = markerImage;
		taggedTarget.m_TagType = tagType;
		taggedTarget.m_LastAlertState =
			MRK_AlertState.MRK_ALERT_IDLE;

		return true;
	}

	static bool UpdatePosition(
		MRK_TaggedTarget taggedTarget
	)
	{
		WorkspaceWidget workspace;
		BaseWorld world;
		IEntity target;
		Widget markerRoot;
		vector worldPosition;
		vector screenPosition;
		float markerHeight;

		if (!taggedTarget)
		{
			return false;
		}

		workspace = GetGame().GetWorkspace();
		world = GetGame().GetWorld();

		if ((!workspace) || (!world))
		{
			return false;
		}

		target = taggedTarget.m_TargetEntity;
		markerRoot = taggedTarget.m_MarkerRoot;

		if ((!target) || (!markerRoot))
		{
			return false;
		}

		worldPosition = target.GetOrigin();

		markerHeight =
			MRK_TargetClassifier.GetMarkerHeight(
				taggedTarget.m_TagType
			);

		worldPosition[1] =
			worldPosition[1] + markerHeight;

		screenPosition = workspace.ProjWorldToScreen(
			worldPosition,
			world
		);

		if (screenPosition[2] <= 0)
		{
			markerRoot.SetVisible(false);

			return true;
		}

		markerRoot.SetVisible(true);

		FrameSlot.SetPos(
			markerRoot,
			screenPosition[0] - MRK_ICON_HALF_SIZE,
			screenPosition[1] - MRK_ICON_HALF_SIZE
		);

		return true;
	}

	static void UpdateMarkerAlertColor(
		MRK_TaggedTarget taggedTarget
	)
	{
		MRK_AlertState alertState;

		if (!taggedTarget)
		{
			return;
		}

		if (!taggedTarget.m_MarkerImage)
		{
			return;
		}

		alertState =
			MRK_TargetStateService.GetTargetAlertState(
				taggedTarget.m_TargetEntity
			);

		if (alertState == taggedTarget.m_LastAlertState)
		{
			return;
		}

		switch (alertState)
		{
			case MRK_AlertState.MRK_ALERT_SEARCHING:
			{
				taggedTarget.m_MarkerImage.SetColor(
					Color.FromRGBA(
						255,
						140,
						0,
						255
					)
				);

				break;
			}

			case MRK_AlertState.MRK_ALERT_COMBAT:
			{
				taggedTarget.m_MarkerImage.SetColor(
					Color.FromRGBA(
						255,
						0,
						0,
						255
					)
				);

				break;
			}

			case MRK_AlertState.MRK_ALERT_IDLE:
			default:
			{
				taggedTarget.m_MarkerImage.SetColor(
					Color.FromRGBA(
						255,
						255,
						255,
						255
					)
				);

				break;
			}
		}

		taggedTarget.m_LastAlertState = alertState;
	}

	static void DestroyMarker(
		MRK_TaggedTarget taggedTarget
	)
	{
		if (!taggedTarget)
		{
			return;
		}

		if (taggedTarget.m_LayoutRoot)
		{
			taggedTarget.m_LayoutRoot.RemoveFromHierarchy();
		}
	}
}