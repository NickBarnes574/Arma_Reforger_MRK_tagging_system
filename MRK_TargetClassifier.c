class MRK_TargetClassifier
{
	static MRK_TagType ClassifyTarget(IEntity target)
	{
		ChimeraCharacter character;
		HelicopterControllerComponent helicopterController;
		TrackedControllerComponent trackedController;
		CarControllerComponent carController;
		EntityPrefabData prefabData;
		ResourceName prefabName;

		if (!target)
		{
			return MRK_TagType.MRK_TAG_UNKNOWN;
		}

		character = ChimeraCharacter.Cast(target);

		if (character)
		{
			return MRK_TagType.MRK_TAG_INFANTRY;
		}

		helicopterController =
			HelicopterControllerComponent.Cast(
				target.FindComponent(
					HelicopterControllerComponent
				)
			);

		if (helicopterController)
		{
			return MRK_TagType.MRK_TAG_HELICOPTER;
		}

		trackedController =
			TrackedControllerComponent.Cast(
				target.FindComponent(
					TrackedControllerComponent
				)
			);

		if (trackedController)
		{
			return MRK_TagType.MRK_TAG_TANK;
		}

		carController =
			CarControllerComponent.Cast(
				target.FindComponent(
					CarControllerComponent
				)
			);

		if (!carController)
		{
			return MRK_TagType.MRK_TAG_UNKNOWN;
		}

		prefabData = target.GetPrefabData();

		if (!prefabData)
		{
			return MRK_TagType.MRK_TAG_UNKNOWN;
		}

		prefabName = prefabData.GetPrefabName();

		return ClassifyWheeledVehicle(prefabName);
	}

	protected static MRK_TagType ClassifyWheeledVehicle(
		ResourceName prefabName
	)
	{
		string name;

		name = prefabName;
		name.ToLower();

		if (name.Contains("btr"))
		{
			return MRK_TagType.MRK_TAG_APC;
		}

		if ((name.Contains("ural")) || (name.Contains("m923")))
		{
			return MRK_TagType.MRK_TAG_TRUCK;
		}

		return MRK_TagType.MRK_TAG_CAR;
	}

	static ResourceName GetIconForTagType(
		MRK_TagType tagType
	)
	{
		switch (tagType)
		{
			case MRK_TagType.MRK_TAG_INFANTRY:
				return "{9F4D0043E24255E8}UI/Textures/Editor/EditableEntities/Characters/EditableEntity_Character_Player.edds";

			case MRK_TagType.MRK_TAG_CAR:
				return "{943873C801ED95B8}UI/Textures/Editor/EditableEntities/Vehicles/EditableEntity_Vehicle_Offroad.edds";

			case MRK_TagType.MRK_TAG_APC:
				return "{95F49CBE9FF7A0CB}UI/Textures/Editor/EditableEntities/Vehicles/EditableEntity_Vehicle_Apc.edds";

			case MRK_TagType.MRK_TAG_TRUCK:
				return "{9EBE212C91B36BBE}UI/Textures/Editor/EditableEntities/Vehicles/EditableEntity_Vehicle_Truck.edds";

			case MRK_TagType.MRK_TAG_HELICOPTER:
				return "{BFBA42C85CB25019}UI/Textures/Editor/EditableEntities/Vehicles/EditableEntity_Vehicle_Helicopter.edds";

			case MRK_TagType.MRK_TAG_TANK:
				return "{95F49CBE9FF7A0CB}UI/Textures/Editor/EditableEntities/Vehicles/EditableEntity_Vehicle_Apc.edds";
		}

		return ResourceName.Empty;
	}

	static float GetMarkerHeight(
		MRK_TagType tagType
	)
	{
		float markerHeight;

		markerHeight = 2.0;

		switch (tagType)
		{
			case MRK_TagType.MRK_TAG_INFANTRY:
			{
				markerHeight = 2.0;
				break;
			}

			case MRK_TagType.MRK_TAG_CAR:
			{
				markerHeight = 2.5;
				break;
			}

			case MRK_TagType.MRK_TAG_TRUCK:
			{
				markerHeight = 3.5;
				break;
			}

			case MRK_TagType.MRK_TAG_APC:
			case MRK_TagType.MRK_TAG_HELICOPTER:
			case MRK_TagType.MRK_TAG_TANK:
			{
				markerHeight = 3.0;
				break;
			}
		}

		return markerHeight;
	}
	
	static IEntity ResolveTaggableEntity(IEntity target)
	{
		IEntity currentEntity;
		IEntity parentEntity;
		MRK_TagType tagType;

		if (!target)
		{
			return null;
		}

		currentEntity = target;

		while (currentEntity)
		{
			tagType = ClassifyTarget(currentEntity);

			if (tagType != MRK_TagType.MRK_TAG_UNKNOWN)
			{
				return currentEntity;
			}

			parentEntity = currentEntity.GetParent();

			if (!parentEntity)
			{
				break;
			}

			currentEntity = parentEntity;
		}

		return null;
	}
	
}