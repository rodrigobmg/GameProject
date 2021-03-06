#ifndef CARDefine_h__201a14a3_46e7_42a6_b6ac_29fbc7d227ba
#define CARDefine_h__201a14a3_46e7_42a6_b6ac_29fbc7d227ba

#include "Core/IntegerType.h"

#include <cassert>

#ifndef BIT
#	define BIT( n ) ( 1 << (n) )
#endif

#ifndef ARRAY_SIZE
#	define ARRAY_SIZE( array ) ( sizeof(array) / sizeof( array[0]) )
#endif

namespace CAR
{
	class MapTile;
	class LevelActor;

	typedef uint32 TileId;
	typedef int    PlayerId;

	TileId const FAIL_TILE_ID = TileId(-1);
	TileId const TEMP_TILE_ID = TileId(-2);

	PlayerId const CAR_ERROR_PLAYER_ID = PlayerId(-1);
	PlayerId const FAIL_PLAYER_ID = PlayerId(31);

	int const ERROR_GROUP_ID = -1;
	int const ABBEY_GROUP_ID = -2;

	template< class T >
	constexpr int BitMaskToIndex(T mask)
	{
		return (mask >> 1) ? 1 + BitMaskToIndex(mask >> 1) : 0;
	}

	enum Expansion : uint8;


	enum SideType : uint8
	{
		eField  = 0,
		eRoad  ,
		eRiver ,
		eCity  ,
		eAbbey , //EXP_ABBEY_AND_MAYOR
		eInsideLink ,

		eEmptySide ,
	};


	typedef uint16 SideContentType;
	typedef uint32 TileContentType;

	struct TileContent
	{
		enum Enum
		{
			eCloister            = BIT(0) ,
			eCathedral           = BIT(1) , //EXP_INNS_AND_CATHEDRALS
			eVolcano             = BIT(2) , //EXP_THE_PRINCESS_AND_THE_DRAGON 
			eTheDragon           = BIT(3) , //EXP_THE_PRINCESS_AND_THE_DRAGON 
			eMagicPortal         = BIT(4) , //EXP_THE_PRINCESS_AND_THE_DRAGON
			eTowerFoundation     = BIT(7) , //EXP_THE_TOWER
			eBazaar              = BIT(8) , //EXP_BRIDGES_CASTLES_AND_BAZAARS
			eHill                = BIT(9) , //EXP_HILLS_AND_SHEEP
			eVineyard            = BIT(10), //EXP_HILLS_AND_SHEEP
			eHalfling            = BIT(11), //EXP_HALFLINGS_I EXP_HALFLINGS_II
			eLaPorxada           = BIT(12), //EXP_LA_PORXADA
			eMage                = BIT(13), //EXP_MAGE_AND_WITCH
			eGold                = BIT(14), //EXP_GOLDMINES
			eFlyMachine          = BIT(15), //EXP_THE_FLY_MACHINES
			eCropCirclePitchfork = BIT(16), //EXP_CROP_CIRCLE_I EXP_CROP_CIRCLE_II
			eCropCircleClub      = BIT(17), //EXP_CROP_CIRCLE_I EXP_CROP_CIRCLE_II
			eCropCircleShield    = BIT(18), //EXP_CROP_CIRCLE_I EXP_CROP_CIRCLE_II
			eBesieger            = BIT(19),
			eShrine              = BIT(20), //EXP_HERETICS_AND_SHRINES
			eMonastery           = BIT(21),
			eFestival            = BIT(22), //EXP_THE_FESTIVAL
			eWindRose_W          = BIT(23),
			eWindRose_N          = BIT(24),
			eWindRose_S          = BIT(25),
			eWindRose_E          = BIT(26),
			eBlueWindRose        = BIT(27),
			eFair                = BIT(28),
			//runtime
			eTemp                = BIT(31) ,

			LastMaskPlusOne,
			MaxMaskIndex = BitMaskToIndex<uint32>(LastMaskPlusOne - 1),
		};

		static uint32 const FeatureMask = eCloister | eShrine | eMonastery;
		static uint32 const CropCircleMask = eCropCirclePitchfork | eCropCircleClub | eCropCircleShield;
		static uint32 const OrangeWindRoseMask = eWindRose_W | eWindRose_N | eWindRose_S | eWindRose_E;
		static uint32 const WindRoseMask = OrangeWindRoseMask | eBlueWindRose;
	};

	struct  SideContent
	{
		enum Enum
		{
			ePennant             = BIT(0) ,
			eInn                 = BIT(1) , //EXP_INNS_AND_CATHEDRALS
			eWineHouse           = BIT(2) , //EXP_TRADEERS_AND_BUILDERS
			eGrainHouse          = BIT(3) , //EXP_TRADEERS_AND_BUILDERS
			eClothHouse          = BIT(4) , //EXP_TRADEERS_AND_BUILDERS
			ePrincess            = BIT(5) , //EXP_THE_PRINCESS_AND_THE_DRAGON
			eNotSemiCircularCity = BIT(6) , //EXP_BRIDGES_CASTLES_AND_BAZAARS
			eSheep               = BIT(7) , //EXP_HILLS_AND_SHEEP
			eHalfSeparate        = BIT(8) , //EXP_HILLS_AND_SHEEP
			eSchool              = BIT(9) , //EXP_THE_SCHOOL
			eGermanCastle        = BIT(10),
			eCityOfCarcassonne   = BIT(11),

			LastMaskPlusOne ,
			MaxMaskIndex = BitMaskToIndex<uint16>( LastMaskPlusOne - 1 ),
		};

		static unsigned const InsideLinkTypeMask = eSchool | eGermanCastle;
	};


	static_assert(SideContent::MaxMaskIndex < 8 * sizeof(SideContentType), "SideContentType can't set all SideContent enum");
	static_assert(TileContent::MaxMaskIndex < 8 * sizeof(TileContentType), "TileContentType can't set all TileContent enum");


	namespace ECityQuarter
	{
		enum Type
		{
			Castle ,
			Market ,
			Blacksmith ,
			Cathedral ,
		};
	}

	struct ActorPos
	{
		enum Enum
		{
			eSideNode ,
			eFarmNode ,
			eTile     ,
			eTower    ,
			eTileCorner ,
			ePlayer ,
			eCityQuarter ,
			eNone ,
		};
		ActorPos( Enum aType , int aMeta )
			:type( aType ),meta( aMeta ){}
		ActorPos(){}

		static ActorPos None() { return ActorPos(eNone, 0); }
		static ActorPos Tile() { return ActorPos(eTile, 0); }
		static ActorPos Player(PlayerId id) { return ActorPos(ePlayer, id); }
		Enum type;
		int  meta;

		bool operator == (ActorPos const& rhs) const
		{
			if( type != rhs.type )
				return false;
			if( type != ActorPos::eTile && meta != rhs.meta )
				return false;
			return true;
		}
	};


	enum class EFollowerClassName
	{
		Undefined,
		Thief,
		Knight,
		Monk,
		Abbot,
		Heretic,
		Farmer,
		Lord,
	};

	enum ActorType
	{
		//follower
		eMeeple ,
		eBigMeeple , //EXP_INNS_AND_CATHEDRALS 
		eMayor , //EXP_ABBEY_AND_MAYOR
		eWagon , //EXP_ABBEY_AND_MAYOR
		eBarn  , //EXP_ABBEY_AND_MAYOR
		eShepherd , //EXP_HILLS_AND_SHEEP
		ePhantom , //EXP_PHANTOM
		eAbbot , //CII


		eBuilder ,  //EXP_TRADEERS_AND_BUILDERS
		ePig ,     //EXP_TRADEERS_AND_BUILDERS

		//neutral
		eDragon , //EXP_THE_PRINCESS_AND_THE_DRAGON
		eFariy  , //EXP_THE_PRINCESS_AND_THE_DRAGON
		eCount ,
		eMage,  //EXP_MAGE_AND_WITCH
		eWitch, //EXP_MAGE_AND_WITCH
		eBigPinkPig ,
		eTecher , //EXP_THE_SCHOOL

		NUM_ACTOR_TYPE  ,
		NUM_PLAYER_ACTOR_TYPE = eDragon ,
		eNone = -1,
	};

	struct FieldType
	{
		enum Enum
		{
			eActorStart = 0 ,
			eActorEnd   = eActorStart + NUM_PLAYER_ACTOR_TYPE - 1 ,

			//EXP_TRADEERS_AND_BUILDERS
			eGain , 
			eWine ,
			eCloth ,
			//EXP_THE_TOWER
			eTowerPices ,
			//EXP_ABBEY_AND_MAYOR
			eAbbeyPices ,
			//EXP_BRIDGES_CASTLES_AND_BAZAARS
			eBridgePices ,
			eCastleTokens ,
			eTileIdAuctioned ,
			//EXP_CASTLES
			eGermanCastleTiles ,
			//EXP_GOLDMINES
			eGoldPieces ,
			//EXP_HALFLINGS_I && EXP_HALFLINGS_II
			eHalflingTiles ,
			//EXP_LA_PORXADA
			eLaPorxadaFinishScoring ,
			//EXP_THE_MESSSAGES
			eWomenScore ,
			//EXP_THE_ROBBERS
			eRobberScorePos,

			//EXP_LITTLE_BUILDINGS
			eTowerBuildingTokens,
			eHouseBuildingTokens,
			eShedBuildingTokens,

			NUM,
		};
	};

	enum TileTag
	{
		TILE_NO_TAG = 0,
		TILE_START_TAG ,
		TILE_START_SEQ_BEGIN_TAG ,
		TILE_START_SEQ_END_TAG ,
		TILE_FRIST_PLAY_TAG ,
		TILE_END_TAG ,
		TILE_ABBEY_TAG , //EXP_ABBEY_AND_MAYOR
		TILE_GERMAN_CASTLE_TAG ,
		TILE_HALFLING_TAG ,
		TILE_SCHOOL_TAG ,
	};

	enum SheepToken
	{
		eWolf = 0,
		eOne ,
		eTwo ,
		eThree ,
		eFour ,
		
		Num ,
	};

	namespace EMessageTile
	{
		enum Type
		{
			ScoreSmallestRoad,
			ScoreSmallestCity,
			ScoreSmallestCloister,
			TwoPointsForEachPennant ,
			TwoPointsForEachKnight ,
			TwoPointsForEachFarmer ,
			OneTile ,
			ScoreAFollowerAndRemove ,
		};
	};

	struct ActorInfo
	{
		int       playerId;
		ActorType type;

		bool operator == ( ActorInfo const& rhs ) const
		{
			return playerId == rhs.playerId &&
				type == rhs.type;
		}
	};


}//namespace CAR

#endif // CARDefine_h__201a14a3_46e7_42a6_b6ac_29fbc7d227ba
