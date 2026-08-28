modded class SCR_PlayerController
{
	protected ref MRK_TagManager m_MRK_TagManager;

	override void OnUpdate(float timeSlice)
	{
		super.OnUpdate(timeSlice);

		if (m_MRK_TagManager)
		{
			return;
		}

		if (this != GetGame().GetPlayerController())
		{
			return;
		}

		Print("MRK: Local PlayerController found");

		m_MRK_TagManager = new MRK_TagManager();
		m_MRK_TagManager.Init();
	}
}