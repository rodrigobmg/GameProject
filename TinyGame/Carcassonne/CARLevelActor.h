#ifndef CARLevelActor_h__5cc5aa90_118b_43c8_88a1_c0a4b5e0dbfc
#define CARLevelActor_h__5cc5aa90_118b_43c8_88a1_c0a4b5e0dbfc

#include "CARCommon.h"

#include <vector>

namespace CAR
{
	class MapTile;
	class FeatureBase;


	typedef std::vector< class LevelActor* > ActorList;

	class ActorContainer
	{
	public:
		LevelActor* popActor();
		bool        haveActor(){ return !mActors.empty(); }
		bool        haveOtherActor( int playerId );

		bool        havePlayerActor(int playerId, ActorType type)
		{
			return haveActor(BIT(playerId), BIT(type));
		}
		unsigned    getPlayerActorTypeMask( unsigned playerMask ) const;

		LevelActor* iteratorActor( unsigned playerMask , unsigned actorTypeMask , int& iter) const;
		LevelActor* iteratorActorFromPlayer( unsigned playerMask , int& iter) const;
		LevelActor* iteratorActorFromType(unsigned actorTypeMask, int& iter) const;
		LevelActor* findActor( unsigned playerMask , unsigned actorTypeMask ) const;
		LevelActor* findActorFromType( unsigned actorTypeMask ) const;
		LevelActor* findActorFromPlayer(unsigned playerMask ) const;
		bool        haveActor(unsigned playerMask, unsigned actorTypeMask) const { return findActor(playerMask, actorTypeMask) != nullptr; }
		bool        haveActorFromType(unsigned actorTypeMask) const { return findActorFromType(actorTypeMask) != nullptr; }
		bool        haveActorFromPlayer(unsigned playerMask) const { return findActorFromPlayer(playerMask) != nullptr; }
		
		ActorList mActors;
	};

	class LevelActor
	{
	public:
		LevelActor()
		{
			ownerId = CAR_ERROR_PLAYER_ID;
			feature = nullptr;
			mapTile = nullptr;
			userData = nullptr;
			binder = nullptr;
		}
		ActorType   type;
		
		PlayerId     ownerId;
		ActorPos     pos;
		MapTile*     mapTile;
		FeatureBase* feature;
		LevelActor*  binder;
		ActorList    followers;
		void*        userData;
		EFollowerClassName className;

		LevelActor* popFollower();
		void removeFollower( LevelActor& actor );
		void addFollower( LevelActor& actor );	
	};

	




}//namespace CAR


#endif // CARLevelActor_h__5cc5aa90_118b_43c8_88a1_c0a4b5e0dbfc