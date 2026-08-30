class MRK_Settings
{
	static bool ShowEnemyTags()
	{
		return true;
	}

	static bool ShowFriendlyTags()
	{
		return true;
	}

	static bool ShowCivilianTags()
	{
		return true;
	}

	static bool ShowUnoccupiedVehicleTags()
	{
		return true;
	}

	static float FriendlyTagLifetime()
	{
		return 8.0;
	}

	static float CivilianTagLifetime()
	{
		return 5.0;
	}

	static float UnoccupiedVehicleDisplayDistance()
	{
		return 750.0;
	}

	/*
	 * Markers remain fully opaque at close range, then fade smoothly
	 * as distance increases. These are kept here so they can later be
	 * exposed directly through an in-game settings menu.
	 */
	static float MarkerFadeStartDistance()
	{
		return 300.0;
	}

	static float MarkerFadeEndDistance()
	{
		return 1200.0;
	}

	static float MarkerMinimumOpacity()
	{
		return 0.35;
	}

	static float MaximumMarkerDisplayDistance()
	{
		return 1500.0;
	}
}
