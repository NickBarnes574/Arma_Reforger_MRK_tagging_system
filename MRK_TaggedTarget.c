class MRK_TaggedTarget
{
	IEntity m_TargetEntity;

	Widget m_LayoutRoot;
	Widget m_MarkerRoot;
	ImageWidget m_MarkerImage;

	MRK_TagType m_TagType;
	MRK_AlertState m_LastAlertState;

	/*
	 * Tag behavior metadata.
	 *
	 * These values are captured when the target is tagged so
	 * rendering/removal code does not need to rediscover the
	 * target category every frame.
	 */
	bool m_IsFriendly;
	bool m_IsCivilian;
	bool m_IsOccupied;
	bool m_IsPersistent;

	float m_TimeAlive;
	float m_Lifetime;
}
