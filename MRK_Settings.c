/*
 * Persistent user-facing settings for the MRK tagging system.
 *
 * ModuleGameSettings is Reforger's native game-user-settings storage.
 * MRK_Settings is the lightweight runtime facade used by the hot update loops.
 */
class MRK_GameSettings : ModuleGameSettings
{
	[Attribute("1")]
	bool m_bShowEnemyTags = true;

	[Attribute("1")]
	bool m_bShowFriendlyTags = true;

	[Attribute("1")]
	bool m_bShowCivilianTags = true;

	[Attribute("1")]
	bool m_bShowUnoccupiedVehicleTags = true;

	[Attribute("1")]
	bool m_bDistanceFading = true;

	[Attribute("1")]
	bool m_bShowMarkersThroughOptics = true;

	[Attribute("1")]
	bool m_bTagConfirmationSound = true;

	[Attribute("8")]
	float m_fFriendlyTagLifetime = 8.0;

	[Attribute("5")]
	float m_fCivilianTagLifetime = 5.0;

	[Attribute("750")]
	float m_fUnoccupiedVehicleDisplayDistance = 750.0;

	[Attribute("1500")]
	float m_fMaximumMarkerDisplayDistance = 1500.0;

	[Attribute("200")]
	float m_fMarkerFadeStartDistance = 200.0;

	[Attribute("800")]
	float m_fMarkerFadeMidDistance = 800.0;

	[Attribute("1300")]
	float m_fMarkerFadeFarDistance = 1300.0;

	[Attribute("0.02")]
	float m_fMarkerMinimumOpacity = 0.02;

	[Attribute("1")]
	float m_fMarkerScale = 1.0;

	[Attribute("0.30")]
	float m_fTagAcquisitionTime = 0.30;

	/*
	 * 0 = low forgiveness
	 * 1 = normal forgiveness
	 * 2 = high forgiveness
	 */
	[Attribute("1")]
	int m_iAcquisitionForgiveness = 1;
};

class MRK_Settings
{
	protected static ref MRK_GameSettings s_Settings;
	protected static bool s_Initialized;

	static void Init()
	{
		if (s_Initialized)
		{
			Refresh();
			return;
		}

		s_Initialized = true;
		s_Settings = new MRK_GameSettings();

		Refresh();

		if (GetGame())
		{
			GetGame().OnUserSettingsChangedInvoker().Insert(
				Refresh
			);
		}
	}

	static void Refresh()
	{
		BaseContainer module;

		if (!s_Settings)
		{
			s_Settings = new MRK_GameSettings();
		}

		if (!GetGame())
		{
			return;
		}

		if (!GetGame().GetGameUserSettings())
		{
			return;
		}

		module =
			GetGame()
				.GetGameUserSettings()
				.GetModule("MRK_GameSettings");

		if (!module)
		{
			return;
		}

		BaseContainerTools.WriteToInstance(
			s_Settings,
			module
		);
	}

	static void ResetToDefaults()
	{
		BaseContainer module;
		MRK_GameSettings defaults;

		if (!GetGame())
		{
			return;
		}

		if (!GetGame().GetGameUserSettings())
		{
			return;
		}

		module =
			GetGame()
				.GetGameUserSettings()
				.GetModule("MRK_GameSettings");

		if (!module)
		{
			return;
		}

		defaults = new MRK_GameSettings();

		BaseContainerTools.ReadFromInstance(
			defaults,
			module
		);

		GetGame().UserSettingsChanged();
		GetGame().SaveUserSettings();

		Refresh();
	}

	protected static MRK_GameSettings Data()
	{
		if (!s_Settings)
		{
			s_Settings = new MRK_GameSettings();
		}

		return s_Settings;
	}

	static bool ShowEnemyTags()
	{
		return Data().m_bShowEnemyTags;
	}

	static bool ShowFriendlyTags()
	{
		return Data().m_bShowFriendlyTags;
	}

	static bool ShowCivilianTags()
	{
		return Data().m_bShowCivilianTags;
	}

	static bool ShowUnoccupiedVehicleTags()
	{
		return Data().m_bShowUnoccupiedVehicleTags;
	}

	static bool DistanceFadingEnabled()
	{
		return Data().m_bDistanceFading;
	}

	static bool ShowMarkersThroughOptics()
	{
		return Data().m_bShowMarkersThroughOptics;
	}

	static bool TagConfirmationSoundEnabled()
	{
		return Data().m_bTagConfirmationSound;
	}

	static float FriendlyTagLifetime()
	{
		return Data().m_fFriendlyTagLifetime;
	}

	static float CivilianTagLifetime()
	{
		return Data().m_fCivilianTagLifetime;
	}

	static float UnoccupiedVehicleDisplayDistance()
	{
		return Data().m_fUnoccupiedVehicleDisplayDistance;
	}

	static float MaximumMarkerDisplayDistance()
	{
		return Data().m_fMaximumMarkerDisplayDistance;
	}

	static float MarkerFadeStartDistance()
	{
		return Data().m_fMarkerFadeStartDistance;
	}

	static float MarkerFadeMidDistance()
	{
		return Data().m_fMarkerFadeMidDistance;
	}

	static float MarkerFadeFarDistance()
	{
		return Data().m_fMarkerFadeFarDistance;
	}

	static float MarkerMinimumOpacity()
	{
		return Data().m_fMarkerMinimumOpacity;
	}

	static float MarkerScale()
	{
		float scale;

		scale = Data().m_fMarkerScale;

		if (scale < 0.5)
		{
			return 0.5;
		}

		if (scale > 2.0)
		{
			return 2.0;
		}

		return scale;
	}

	static float TagAcquisitionTime()
	{
		float acquisitionTime;

		acquisitionTime = Data().m_fTagAcquisitionTime;

		if (acquisitionTime < 0.05)
		{
			return 0.05;
		}

		return acquisitionTime;
	}

	static float AcquisitionScreenRadius()
	{
		switch (Data().m_iAcquisitionForgiveness)
		{
			case 0:
			{
				return 20.0;
			}

			case 2:
			{
				return 45.0;
			}
		}

		return 30.0;
	}

	static float AcquisitionGraceTime()
	{
		switch (Data().m_iAcquisitionForgiveness)
		{
			case 0:
			{
				return 0.15;
			}

			case 2:
			{
				return 0.45;
			}
		}

		return 0.30;
	}
}
