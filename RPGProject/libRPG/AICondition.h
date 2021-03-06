#ifndef AICondition_h__
#define AICondition_h__

enum ConditionType
{
	CDT_FIND_EMPTY  = 0,
	CDT_LOSED_EMPTY ,

	CDT_EMPTY_IN_ATTACK_RANGE ,
	CDT_EMPTY_IN_ATTACK_ANGLE ,

	CDT_TAKE_DAMAGE ,

	CDT_TALK_SECTION_END ,
	CDT_TALK_END  ,
	CDT_TRADE_END ,

	CDT_NO_SCHEDULE  ,
	CDT_SCHEDULE_END ,
	CDT_TASK_SUCCESS ,
	CDT_TASK_FAIL ,

	CDT_EMPTY_DEAD ,
	CDT_EMPTY_TOO_FAR ,
	CDT_EMPTY_OUT_VIEW ,
	CDT_NAV_GOAL ,
	CDT_NAV_MOVING ,

	CDT_OWNER_TOO_FAR ,
	CDT_OWNER_DEAD ,
	CDT_OWNER_ATTACK ,
	CDT_OWNER_TAKE_DAMAGE ,

	CDT_NEAR_PLAYER ,
};


#endif // AICondition_h__
