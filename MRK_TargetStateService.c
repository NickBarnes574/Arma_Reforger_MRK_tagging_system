class MRK_TargetStateService
{
	static MRK_AlertState GetTargetAlertState(
		IEntity target
	)
	{
		SCR_AIInfoComponent aiInfo;
		EAIThreatState threatState;

		if (!target)
		{
			return MRK_AlertState.MRK_ALERT_IDLE;
		}

		aiInfo = SCR_AISoundHandling.GetInfoComponent(
			target
		);

		if (!aiInfo)
		{
			return MRK_AlertState.MRK_ALERT_IDLE;
		}

		threatState = aiInfo.GetThreatState();

		switch (threatState)
		{
			case EAIThreatState.ALERTED:
			{
				return MRK_AlertState.MRK_ALERT_SEARCHING;
			}

			case EAIThreatState.THREATENED:
			{
				return MRK_AlertState.MRK_ALERT_COMBAT;
			}

			case EAIThreatState.SAFE:
			case EAIThreatState.VIGILANT:
			default:
			{
				return MRK_AlertState.MRK_ALERT_IDLE;
			}
		}

		return MRK_AlertState.MRK_ALERT_IDLE;
	}

	static bool ShouldRemoveTarget(IEntity target)
	{
		ChimeraCharacter character;
		SCR_CharacterControllerComponent characterController;

		if (!target)
		{
			return true;
		}

		character = ChimeraCharacter.Cast(target);

		if (character)
		{
			characterController =
				SCR_CharacterControllerComponent.Cast(
					target.FindComponent(
						SCR_CharacterControllerComponent
					)
				);

			if (
				characterController &&
				characterController.IsDead()
			)
			{
				return true;
			}

			return false;
		}

		return IsVehicleDestroyed(target);
	}

	protected static bool IsVehicleDestroyed(
		IEntity target
	)
	{
		SCR_DamageManagerComponent damageManager;

		if (!target)
		{
			return true;
		}

		damageManager =
			SCR_DamageManagerComponent.Cast(
				target.FindComponent(
					SCR_DamageManagerComponent
				)
			);

		if (!damageManager)
		{
			return false;
		}

		return damageManager.IsDestroyed();
	}

	static bool IsFriendlyTarget(IEntity target)
	{
		IEntity player;
		FactionAffiliationComponent playerFactionComponent;
		FactionAffiliationComponent targetFactionComponent;
		Faction playerFaction;
		Faction targetFaction;

		if (!target)
		{
			return false;
		}

		player =
			GetGame()
				.GetPlayerController()
				.GetControlledEntity();

		if (!player)
		{
			return false;
		}

		playerFactionComponent =
			FactionAffiliationComponent.Cast(
				player.FindComponent(
					FactionAffiliationComponent
				)
			);

		if (!playerFactionComponent)
		{
			return false;
		}

		targetFactionComponent =
			FactionAffiliationComponent.Cast(
				target.FindComponent(
					FactionAffiliationComponent
				)
			);

		if (!targetFactionComponent)
		{
			return false;
		}

		playerFaction =
			playerFactionComponent.GetAffiliatedFaction();

		targetFaction =
			targetFactionComponent.GetAffiliatedFaction();

		if ((!playerFaction) || (!targetFaction))
		{
			return false;
		}

		return playerFaction.IsFactionFriendly(
			targetFaction
		);
	}
}