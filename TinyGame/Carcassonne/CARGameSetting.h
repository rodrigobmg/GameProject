#ifndef CARGameSetting_h__85a4781f_a1e7_4eeb_a352_d8c9ee34798a
#define CARGameSetting_h__85a4781f_a1e7_4eeb_a352_d8c9ee34798a

#include <algorithm>
#include "Flag.h"
#include "CARParamValue.h"

namespace CAR
{
	enum GameRule
	{
		eHardcore ,

		eSmallCity ,
		eDoubleTurnDrawImmediately ,
		eCantMoveFairy ,
		ePrincessTileMustRemoveKnightOrBuilder ,
		eMoveDragonBeforeScoring ,
		eTowerCaptureEverything ,
	};

	enum class Rule
	{
		eInn,
		eCathedral,
		eBigMeeple,
		eBuilder,
		eTraders,
		ePig,
		eKingAndRobber,
		ePrinecess,
		eDragon,
		eFariy,
		eTower,
		
		eWagon ,
		eMayor,
		eBarn,
		ePhantom,
		eUseHill ,
		eShepherdAndSheep ,
		eBazaar ,
		eBridge ,
		eCastleToken ,

		eHaveGermanCastleTile ,
		eHaveAbbeyTile ,
		eHaveRiverTile ,
		eHaveHalflingTile ,

		//////////////
		eHardcore,
		eSmallCity,
		eDoubleTurnDrawImmediately,
		eCantMoveFairy,
		ePrincessTileMustRemoveKnightOrBuilder,
		eMoveDragonBeforeScoring,
		eTowerCaptureEverything,

		TotalNum,
	};

	class GameSetting : public GameParamCollection
	{
	public:
		GameSetting();

		bool haveRule( Rule ruleFunc ) const;
		void addRule( Rule ruleFunc );
		int  getFarmScoreVersion(){ return mFarmScoreVersion; }

		void calcUsageField( int numPlayer );

		unsigned getFollowerMask();
		inline bool isFollower( ActorType type )
		{
			return ( getFollowerMask() & BIT( type ) ) != 0;
		}

		int getTotalFieldValueNum()
		{
			int result = 0;
			for( auto info : mFieldInfos )
			{
				result += info.num;
			}
			return result;
		}
		int getFieldNum()
		{
			return mNumField;
		}
		int getFieldIndex( FieldType::Enum type )
		{
			return mFieldInfos[type].index;
		}

		int getFieldValueNum(FieldType::Enum type)
		{
			return mFieldInfos[type].num;
		}

		struct FieldInfo
		{
			int index;
			int num;
		};
		int        mFarmScoreVersion;
		int        mNumField;
		FieldInfo  mFieldInfos[FieldType::NUM];

		FlagBits< (int)Rule::TotalNum > mRuleFlags;
		unsigned mExpansionMask;
	};



}//namespace CAR

#endif // CARGameSetting_h__85a4781f_a1e7_4eeb_a352_d8c9ee34798a
