#ifndef CARExpansion_h__
#define CARExpansion_h__

#include "CARDefine.h"
#include "CARGameSetting.h"

namespace CAR
{

	enum Expansion : uint8
	{
		// ?process -Done +Tested
		EXP_INNS_AND_CATHEDRALS,         //
		EXP_TRADERS_AND_BUILDERS,        //
		EXP_THE_PRINCESS_AND_THE_DRAGON, //+T +Dragon
		EXP_THE_TOWER,                   //-Tower
		EXP_ABBEY_AND_MAYOR,             //
		EXP_KING_AND_ROBBER,             //
		EXP_BRIDGES_CASTLES_AND_BAZAARS, //
		EXP_HILLS_AND_SHEEP,             //-T -Vineyards -Hill -shepherd -ST_HalfSeparate

		EXP_THE_RIVER,                   //+T +RiverRule
		EXP_THE_RIVER_II,                //+T 

		EXP_CASTLES,                     //?T ?CastleTile ?RoadCityScoring ?CastleFeature
		EXP_PHANTOM,

		EXP_BASIC,
		EXP_TEST,
		NUM_EXPANSIONS,
		EXP_NULL,
	};

	struct ExpansionContent
	{
		Expansion   exp;
		TileDefine* defines;
		int         numDefines;
	};

	extern ExpansionContent gAllExpansionTileContents[];

	class GameSetting;
	void AddExpansionRule(GameSetting& gameSetting, Expansion exp);

}//namespace CAR

#endif // CARExpansion_h__