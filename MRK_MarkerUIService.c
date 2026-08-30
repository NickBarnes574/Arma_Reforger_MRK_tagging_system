class MRK_MarkerUIService
{
	static MRK_TaggedTarget CreateMarker(
		IEntity target,
		MRK_TagType tagType
	)
	{
		WorkspaceWidget workspace;
		Widget layoutRoot;
		Widget markerRoot;
		ImageWidget markerImage;
		MRK_TaggedTarget taggedTarget;
		ResourceName iconResource;

		if (!target)
		{
			return null;
		}

		workspace = GetGame().GetWorkspace();

		if (!workspace)
		{
			return null;
		}

		layoutRoot =
			workspace.CreateWidgets(
				MRK_MARKER_LAYOUT
			);

		if (!layoutRoot)
		{
			return null;
		}

		/*
		* Persistent target markers should sit below
		* weapon optic UI.
		*
		* Do this ONCE when the widget is created.
		*/
		layoutRoot.SetZOrder(0);

		markerRoot =
			layoutRoot.FindAnyWidget(
				"MarkerRoot"
			);

		markerImage =
			ImageWidget.Cast(
				layoutRoot.FindAnyWidget(
					"MarkerImage"
				)
			);

		if ((!markerRoot) || (!markerImage))
		{
			layoutRoot.RemoveFromHierarchy();
			return null;
		}

		/*
		* Infantry uses a smaller 32x32 marker.
		* Everything else remains 64x64.
		*/
		float markerSize;

		markerSize = GetMarkerSize(tagType);

		FrameSlot.SetSize(
			markerRoot,
			markerSize,
			markerSize
		);

		iconResource =
			MRK_TargetClassifier.GetIconForTagType(
				tagType
			);

		if (iconResource != ResourceName.Empty)
		{
			markerImage.LoadImageTexture(
				0,
				iconResource
			);

			markerImage.SetImage(0);
		}

		taggedTarget =
			new MRK_TaggedTarget();

		taggedTarget.m_TargetEntity = target;
		taggedTarget.m_LayoutRoot = layoutRoot;
		taggedTarget.m_MarkerRoot = markerRoot;
		taggedTarget.m_MarkerImage = markerImage;
		taggedTarget.m_TagType = tagType;

		taggedTarget.m_LastAlertState =
			MRK_AlertState.MRK_ALERT_IDLE;

		if (
			MRK_TargetStateService.IsFriendlyTarget(
				target
			)
		)
		{
			markerImage.SetColor(
				Color.FromRGBA(
					80,
					160,
					255,
					255
				)
			);
		}
		else
		{
			markerImage.SetColor(
				Color.FromRGBA(
					255,
					255,
					255,
					255
				)
			);
		}

		return taggedTarget;
	}

	static bool UpdatePosition(
		MRK_TaggedTarget taggedTarget
	)
	{
		WorkspaceWidget workspace;
		BaseWorld world;
		IEntity player;
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

		player =
			GetGame()
				.GetPlayerController()
				.GetControlledEntity();

		if (
			player &&
			!ShouldDisplayMarker(
				taggedTarget,
				player
			)
		)
		{
			markerRoot.SetVisible(false);
			return true;
		}

		worldPosition = target.GetOrigin();

		markerHeight =
			MRK_TargetClassifier.GetMarkerHeightForEntity(
				taggedTarget.m_TargetEntity,
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

		ApplyDistanceOpacity(
			taggedTarget,
			player
		);

		float markerHalfSize;

		markerHalfSize =
			GetMarkerHalfSize(
				taggedTarget.m_TagType
			);

		FrameSlot.SetPos(
			markerRoot,
			screenPosition[0] - markerHalfSize,
			screenPosition[1] - markerHalfSize
		);

		return true;
	}

	static bool UpdatePositionWithCamera(
		MRK_TaggedTarget taggedTarget,
		int cameraIndex,
		SCR_2DPIPSightsComponent pipSights
	)
	{
		WorkspaceWidget workspace;
		BaseWorld world;
		IEntity player;
		IEntity target;
		Widget markerRoot;
		SCR_CharacterControllerComponent characterController;
		CharacterAimingComponent aimingComponent;

		vector worldPosition;
		vector projectedPosition;
		vector aimRotationModification;
		vector camTM[4];
		vector markerTM[4];

		float markerHeight;
		float zoomCorrection;

		if (!taggedTarget)
		{
			return false;
		}

		if (!pipSights)
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

		player = GetGame().GetPlayerController().GetControlledEntity();

		if (!player)
		{
			return false;
		}

		if (
			!ShouldDisplayMarker(
				taggedTarget,
				player
			)
		)
		{
			markerRoot.SetVisible(false);
			return true;
		}

		characterController =
			SCR_CharacterControllerComponent.Cast(
				player.FindComponent(
					SCR_CharacterControllerComponent
				)
			);

		if (!characterController)
		{
			return false;
		}

		aimingComponent =
			characterController.GetAimingComponent();

		if (!aimingComponent)
		{
			return false;
		}

		worldPosition = target.GetOrigin();

		markerHeight =
			MRK_TargetClassifier.GetMarkerHeightForEntity(
				taggedTarget.m_TargetEntity,
				taggedTarget.m_TagType
			);

		worldPosition[1] =
			worldPosition[1] + markerHeight;

		/*
		* Get the character's current weapon/sight
		* aiming offset.
		*/
		aimRotationModification =
			aimingComponent.GetAimingRotationModification();

		/*
		* Get the actual PIP camera transform.
		*/
		pipSights.GetPIPCamera().GetWorldTransform(
			camTM
		);

		/*
		* Build a transform whose position is the
		* marker's world position.
		*/
		Math3D.MatrixIdentity4(
			markerTM
		);

		markerTM[3] = worldPosition;

		/*
		* Bohemia applies the same correction to
		* its own HUD nametags in PIP scopes.
		*/
		zoomCorrection =
			pipSights.GetFOV() /
			pipSights.GetMainCameraFOV();

		SCR_Math3D.RotateAround(
			markerTM,
			camTM[3],
			camTM[1],
			aimRotationModification[0] *
				zoomCorrection,
			markerTM
		);

		SCR_Math3D.RotateAround(
			markerTM,
			camTM[3],
			camTM[0],
			-aimRotationModification[1] *
				zoomCorrection,
			markerTM
		);

		worldPosition = markerTM[3];

		/*
		* Now project through the actual PIP camera.
		*/
		projectedPosition =
			workspace.ProjWorldToScreen(
				worldPosition,
				world,
				cameraIndex
			);

		if (projectedPosition[2] <= 0)
		{
			markerRoot.SetVisible(false);
			return true;
		}

		if (!pipSights.IsScreenPositionInSights(
			projectedPosition
		))
		{
			markerRoot.SetVisible(false);
			return true;
		}

		markerRoot.SetVisible(true);

		ApplyDistanceOpacity(
			taggedTarget,
			player
		);

		float markerHalfSize;

		markerHalfSize =
			GetMarkerHalfSize(
				taggedTarget.m_TagType
			);

		FrameSlot.SetPos(
			markerRoot,
			projectedPosition[0] - markerHalfSize,
			projectedPosition[1] - markerHalfSize
		);

		return true;
	}

	static void UpdateMarkerAlertColor(
		MRK_TaggedTarget taggedTarget,
		MRK_AlertState alertState
	)
	{
		Color markerColor;

		if (!taggedTarget)
		{
			return;
		}

		if (!taggedTarget.m_MarkerImage)
		{
			return;
		}

		/*
		* Avoid repeatedly changing the widget when
		* nothing has changed.
		*/
		if (taggedTarget.m_LastAlertState == alertState)
		{
			return;
		}

		switch (alertState)
		{
			case MRK_AlertState.MRK_ALERT_SEARCHING:
			{
				markerColor =
					Color.FromRGBA(
						226,
						167,
						79,
						255
					);

				break;
			}

			case MRK_AlertState.MRK_ALERT_COMBAT:
			{
				markerColor =
					Color.FromRGBA(
						255,
						70,
						70,
						255
					);

				break;
			}

			case MRK_AlertState.MRK_ALERT_IDLE:
			default:
			{
				markerColor =
					Color.FromRGBA(
						255,
						255,
						255,
						255
					);

				break;
			}
		}

		taggedTarget.m_MarkerImage.SetColor(
			markerColor
		);

		taggedTarget.m_LastAlertState =
			alertState;
	}

	static void UpdateMarkerCivilianColor(
		MRK_TaggedTarget taggedTarget
	)
	{
		if (!taggedTarget)
		{
			return;
		}

		if (!taggedTarget.m_MarkerImage)
		{
			return;
		}

		taggedTarget.m_MarkerImage.SetColor(
			Color.FromRGBA(
				255,
				255,
				255,
				255
			)
		);
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

	static void UpdateMarkerFriendlyColor(
		MRK_TaggedTarget taggedTarget
	)
	{
		if (!taggedTarget)
		{
			return;
		}

		if (!taggedTarget.m_MarkerImage)
		{
			return;
		}

		taggedTarget.m_MarkerImage.SetColor(
			Color.FromRGBA(
				80,
				160,
				255,
				255
			)
		);
	}

	protected static bool ShouldDisplayMarker(
		MRK_TaggedTarget taggedTarget,
		IEntity viewer
	)
	{
		float distance;
		bool isVehicle;

		if ((!taggedTarget) || (!viewer))
		{
			return false;
		}

		if (!taggedTarget.m_TargetEntity)
		{
			return false;
		}

		distance = vector.Distance(
			viewer.GetOrigin(),
			taggedTarget.m_TargetEntity.GetOrigin()
		);

		/*
		 * Keep very distant tags in the logical tag list, but stop
		 * drawing them on the HUD. If the target comes back into
		 * range, its marker automatically becomes visible again.
		 */
		if (
			distance >
			MRK_Settings.MaximumMarkerDisplayDistance()
		)
		{
			return false;
		}

		isVehicle =
			taggedTarget.m_TagType !=
			MRK_TagType.MRK_TAG_INFANTRY;

		/*
		 * Empty vehicles stay logically tagged so the player cannot
		 * immediately retag them, but they stop consuming HUD space
		 * once they are outside the configured display distance.
		 */
		if (
			isVehicle &&
			!taggedTarget.m_IsOccupied
		)
		{
			if (!MRK_Settings.ShowUnoccupiedVehicleTags())
			{
				return false;
			}

			if (
				distance >
				MRK_Settings.UnoccupiedVehicleDisplayDistance()
			)
			{
				return false;
			}
		}

		return true;
	}

	protected static void ApplyDistanceOpacity(
		MRK_TaggedTarget taggedTarget,
		IEntity viewer
	)
	{
		float distance;
		float opacity;

		if ((!taggedTarget) || (!viewer))
		{
			return;
		}

		if ((!taggedTarget.m_TargetEntity) || (!taggedTarget.m_MarkerRoot))
		{
			return;
		}

		distance = vector.Distance(
			viewer.GetOrigin(),
			taggedTarget.m_TargetEntity.GetOrigin()
		);

		opacity = GetMarkerOpacityForDistance(
			distance
		);

		/*
		 * Apply opacity to MarkerRoot rather than MarkerImage so alert
		 * colors (white/orange/red/blue) remain completely independent
		 * from distance fading.
		 */
		taggedTarget.m_MarkerRoot.SetOpacity(
			opacity
		);
	}

	protected static float GetMarkerOpacityForDistance(
		float distance
	)
	{
		float fadeStart;
		float fadeEnd;
		float minimumOpacity;
		float fadeRange;
		float fadeProgress;
		float opacity;

		fadeStart =
			MRK_Settings.MarkerFadeStartDistance();

		fadeEnd =
			MRK_Settings.MarkerFadeEndDistance();

		minimumOpacity =
			MRK_Settings.MarkerMinimumOpacity();

		if (distance <= fadeStart)
		{
			return 1.0;
		}

		if (distance >= fadeEnd)
		{
			return minimumOpacity;
		}

		fadeRange = fadeEnd - fadeStart;

		if (fadeRange <= 0.0)
		{
			return minimumOpacity;
		}

		fadeProgress =
			(distance - fadeStart) /
			fadeRange;

		opacity =
			1.0 -
			(fadeProgress * (1.0 - minimumOpacity));

		if (opacity < minimumOpacity)
		{
			opacity = minimumOpacity;
		}

		if (opacity > 1.0)
		{
			opacity = 1.0;
		}

		return opacity;
	}

	static float GetMarkerSize(MRK_TagType tagType)
	{
		if (
			tagType ==
			MRK_TagType.MRK_TAG_INFANTRY
		)
		{
			return MRK_INFANTRY_MARKER_SIZE;
		}

		return MRK_MARKER_SIZE;
	}

	static float GetMarkerHalfSize(MRK_TagType tagType)
	{
		return GetMarkerSize(tagType) * 0.5;
	}
}