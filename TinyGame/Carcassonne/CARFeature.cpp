#include "CARFeature.h"

#include "CARPlayer.h"
#include "CARGameSetting.h"
#include "CARParamValue.h"
#include "CARLevelActor.h"
#include "CARLevel.h"
#include "CARDebug.h"

#include <algorithm>

namespace CAR
{
	int const DefaultActorMaskNum = 3;

	int FeatureBase::getActorPutInfoInternal(int playerId , ActorPos const& actorPos , unsigned actorMasks[] , int numMask , std::vector< ActorPosInfo >& outInfo)
	{
		unsigned actorTypeMask = 0;
		unsigned actorTypeMaskOther = 0;
		for( int i = 0 ; i < mActors.size() ; ++i )
		{
			LevelActor* actor = mActors[i];
			if ( actor->owner == nullptr )
				continue;

			if ( mActors[i]->owner->getId() != playerId )
				actorTypeMaskOther |= BIT(mActors[i]->type);
			else
				actorTypeMask |= BIT(mActors[i]->type);
		}

		if ( actorTypeMask == 0 && actorTypeMaskOther != 0 )
			return 0;

		ActorPosInfo info;
		info.actorTypeMask = 0;
		for( int i = 0 ; i < numMask ; ++i )
		{
			if ( actorMasks[i] == 0 )
				continue;
			if ( actorTypeMask & actorMasks[i] )
				continue;

			info.actorTypeMask = actorMasks[i];
			break;
		}

		if ( info.actorTypeMask == 0 )
			return 0;

		info.pos   = actorPos;
		info.group = group;
		outInfo.push_back( info );
		return 1;
	}

	int FeatureBase::getDefaultActorPutInfo(int playerId , ActorPos const& actorPos , unsigned actorMasks[] , std::vector< ActorPosInfo >& outInfo )
	{
		actorMasks[0] |= BIT(ActorType::eMeeple);
		if ( mSetting->haveUse( EXP_INNS_AND_CATHEDRALS ) )
		{
			actorMasks[0] |= BIT( ActorType::eBigMeeple );
		}
		if ( mSetting->haveUse( EXP_TRADERS_AND_BUILDERS ) )
		{
			actorMasks[1] |= BIT( ActorType::eBuilder );
		}
		return getActorPutInfoInternal( playerId , actorPos , actorMasks , DefaultActorMaskNum , outInfo );
	}

	void FeatureBase::mergeData(FeatureBase& other , MapTile const& putData , int meta)
	{
		assert( other.type == type );
		assert( group != -1 && other.group != -1 );
		while( LevelActor* actor = other.popActor() )
		{
			addActor( *actor );
		}
		MergeData( mapTiles , other.mapTiles );
	}

	void FeatureBase::addActor( LevelActor& actor )
	{
		if ( actor.feature != nullptr )
		{
			actor.feature->removeActor( actor );
		}
		actor.feature = this;
		mActors.push_back( &actor );
	}

	void FeatureBase::removeActor( LevelActor& actor )
	{
		assert( actor.feature == this );
		actor.feature = nullptr;
		mActors.erase( std::find( mActors.begin() , mActors.end() , &actor ) );
	}

	LevelActor* FeatureBase::removeActorByIndex(int index)
	{
		assert(  0 <= index && index < mActors.size() );
		LevelActor* actor = mActors[ index ];
		actor->feature = nullptr;
		mActors.erase( mActors.begin() + index );
		return actor;
	}

	int FeatureBase::calcScore(std::vector< FeatureScoreInfo >& scoreInfos)
	{
		initFeatureScoreInfo( scoreInfos );
		addMajority( scoreInfos );
		int numPlayer = evalMajorityControl( scoreInfos );
		for( int i = 0 ; i < numPlayer ; ++i )
		{
			scoreInfos[i].score = calcPlayerScore( scoreInfos[i].id );
		}
		return numPlayer;
	}

	int FeatureBase::evalMajorityControl(std::vector< FeatureScoreInfo >& featureControls)
	{
		struct CmpFun
		{
			bool operator() ( FeatureScoreInfo const& f1 , FeatureScoreInfo const& f2 ) const
			{
				return f1.majority > f2.majority;
			}
		};
		std::sort( featureControls.begin() , featureControls.end() , CmpFun() );

		int numPlayer = 0;
		if ( featureControls[0].majority != 0 )
		{
			int maxMajority  = featureControls[0].majority;
			++numPlayer;
			for( int i = 1; i < featureControls.size() ; ++i )
			{
				if ( featureControls[i].majority != maxMajority )
					break;

				++numPlayer;
			}
		}
		return numPlayer;
	}

	int FeatureBase::getMajorityValue( ActorType actorType )
	{
		switch( actorType )
		{
		case ActorType::eMeeple:    return 1; break;
		case ActorType::eBigMeeple: return 2; break;
		case ActorType::eWagon:     return 1; break;
		case ActorType::eMayor:
			{
				assert( type == FeatureType::eCity );
				CityFeature* city = static_cast< CityFeature* >( this );
				return city->getSideContentNum( SideContent::ePennant );
			}
			break;
		}
		return 0;
	}

	void FeatureBase::addMajority( std::vector< FeatureScoreInfo >& scoreInfos )
	{
		for( int i = 0 ; i < mActors.size() ; ++i )
		{
			LevelActor* actor = mActors[i];
			FeatureScoreInfo& info = scoreInfos[ actor->owner->getId() ];
			info.majority += getMajorityValue( actor->type );
		}
	}

	void FeatureBase::initFeatureScoreInfo(std::vector< FeatureScoreInfo > &scoreInfos)
	{
		scoreInfos.resize( MaxPlayerNum );
		for( int i = 0 ; i < MaxPlayerNum ; ++i )
		{
			scoreInfos[i].id       = i;
			scoreInfos[i].majority = 0;
		}
	}

	bool FeatureBase::testInRange(Vec2i const& min , Vec2i const& max)
	{
		for( MapTileSet::iterator iter = mapTiles.begin() , itEnd = mapTiles.end() ;
			iter != itEnd ; ++iter )
		{
			MapTile* mapTile = *iter;
			if ( isInRange( mapTile->pos , min , max ) )
				return true;
		}
		return false;
	}

	SideFeature::SideFeature()
	{
		openCount = 0;
	}

	void SideFeature::mergeData( FeatureBase& other , MapTile const& putData , int meta)
	{
		BaseClass::mergeData( other , putData , meta );

		SideFeature& otherData = static_cast< SideFeature& >( other );

		openCount += otherData.openCount;
		unsigned mask = putData.getSideLinkMask( meta );
		int dir;
		while( FBit::MaskIterator< FDir::TotalNum >( mask , dir ) )
		{
			SideNode* nodeCon = putData.sideNodes[dir].outConnect;
			if ( nodeCon && nodeCon->group == other.group )
			{
				openCount -= 2;
				assert( openCount >= 0 );
			}
		}
		int idx = nodes.size();
		MergeData( nodes , otherData.nodes );
		for( int i = idx ; i < nodes.size() ; ++i )
			nodes[i]->group = group;
	}

	bool SideFeature::checkNodesConnected() const
	{
		for( int i = 0 ; i < nodes.size() ; ++i )
		{
			if ( nodes[i]->outConnect == nullptr )
				return false;
		}
		return true;
	}

	void SideFeature::addNode(MapTile& mapData , unsigned dirMask , SideNode* linkNode)
	{
		addMapTile( mapData );
		unsigned mask = dirMask;
		int dir = 0;
		while ( FBit::MaskIterator< FDir::TotalNum >( mask , dir ) )
		{
			SideNode& node = mapData.sideNodes[dir];
			assert( node.group == -1 );
			node.group = group;
			nodes.push_back( &node );

			if ( node.outConnect )
			{
				int conGroup = node.outConnect->group;
				if ( conGroup == group )
				{
					--openCount;
					continue;
				}
				else if ( conGroup == ABBEY_GROUP_ID )
				{
					continue;
				}
			}


			++openCount;

		}
		
	}

	int SideFeature::calcOpenCount()
	{
		int result = 0;
		for( int i = 0 ; i < nodes.size() ; ++i )
		{
			for( int j = i + 1 ; j < nodes.size() ; ++j )
			{
				assert( nodes[i] != nodes[j]);
			}
			SideNode* node = nodes[i];
			if ( node->outConnect == nullptr )
				++result;
		}
		return result;
	}

	int SideFeature::getSideContentNum(unsigned contentMask)
	{
		int result = 0;
		for( int i = 0 ; i < nodes.size() ; ++i )
		{
			SideNode* node = nodes[i];

			MapTile const* mapTile = node->getMapTile();
			uint32 content = mapTile->getSideContnet( node->index ) & contentMask;
			while ( content )
			{
				content &= ~FBit::Extract( content );
				++result;
			}
		}
		return result;
	}

	void SideFeature::generateRoadLinkFeatures( std::set< unsigned >& outFeatures )
	{
		for( int i = 0 ; i < nodes.size() ; ++i )
		{
			SideNode* node = nodes[i];
			MapTile const* mapTile = node->getMapTile();
			unsigned roadMask = mapTile->getRoadLinkMask( node->index );

			CAR_LOG("road Mask = %u" , roadMask );

			if ( roadMask == 0 )
				continue;
			
			if ( roadMask & Tile::CenterMask )
			{
				switch ( mapTile->getTileContent() & TileContent::FeatureMask )
				{
				case TileContent::eCloister:
					outFeatures.insert( mapTile->group );
					break;
				}
			}
			else
			{
				int dir;
				while( FBit::MaskIterator< FDir::TotalNum >(roadMask,dir) )
				{
					SideNode const& linkNode = mapTile->sideNodes[dir];
					if ( linkNode.group == group )
						continue;

					if ( linkNode.group == -1 )
					{
						CAR_LOG("Warnning: No Link Feature In Road Link");
					}
					else
					{
						outFeatures.insert( linkNode.group );
					}
				}
			}
		}
	}

	void SideFeature::addAbbeyNode(MapTile& mapData , int dir )
	{
		--openCount;
	}

	bool SideFeature::getActorPos(MapTile const& mapTile , ActorPos& actorPos)
	{
		for (int i = 0 ; i < FDir::TotalNum ;++i )
		{
			SideNode const& node = mapTile.sideNodes[i];
			if ( node.group == group )
			{
				actorPos.type = ActorPos::eSideNode;
				actorPos.meta = i;
				return true;
			}
		}
		return false;
	}


	RoadFeature::RoadFeature()
	{
		haveInn    = false;
	}

	int RoadFeature::getActorPutInfo(int playerId , int posMeta ,std::vector< ActorPosInfo >& outInfo)
	{
		unsigned actorMasks[DefaultActorMaskNum] = { 0 , 0 , 0 };
		if ( mSetting->haveUse( EXP_ABBEY_AND_MAYOR ) )
		{
			actorMasks[0] |= BIT( ActorType::eWagon );
		}
		return getDefaultActorPutInfo( playerId , ActorPos( ActorPos::eSideNode , posMeta ) , actorMasks , outInfo );
	}

	void RoadFeature::mergeData(FeatureBase& other , MapTile const& putData , int meta)
	{
		BaseClass::mergeData( other , putData , meta );
		RoadFeature& otherData = static_cast< RoadFeature& >( other );

	}

	void RoadFeature::addNode(MapTile& mapData , unsigned dirMask , SideNode* linkNode)
	{
		BaseClass::addNode( mapData , dirMask , linkNode );

		if ( mSetting->haveUse( EXP_INNS_AND_CATHEDRALS) )
		{
			unsigned mask = dirMask;
			int dir;
			while( FBit::MaskIterator< FDir::TotalNum >( mask , dir ) )
			{
				if ( mapData.getSideContnet( dir ) & SideContent::eInn )
				{
					haveInn = true;
					break;
				}
			}
		}
	}

	bool RoadFeature::checkComplete()
	{
		return openCount == 0;
	}

	int RoadFeature::calcPlayerScore(int playerId)
	{
		int numTile = mapTiles.size();

		int factor = Value::NonCompleteFactor;
		if ( mSetting->haveUse( EXP_INNS_AND_CATHEDRALS ) && haveInn )
		{
			if ( checkComplete() )
				factor += Value::InnAddtitionFactor;
			else
				factor = 0;
		}
		else if ( checkComplete() )
		{
			factor = Value::RoadFactor;
		}
		return numTile * factor;
	}

	CityFeature::CityFeature()
	{
		haveCathedral = false;
		isCastle = false;
	}


	int CityFeature::getActorPutInfo(int playerId , int posMeta ,std::vector< ActorPosInfo >& outInfo)
	{
		unsigned actorMasks[DefaultActorMaskNum] = { 0 , 0 , 0 };
		if ( mSetting->haveUse( EXP_ABBEY_AND_MAYOR ) )
		{
			actorMasks[0] |= BIT( ActorType::eWagon ) | BIT( ActorType::eMayor );
		}
		return getDefaultActorPutInfo( playerId , ActorPos( ActorPos::eSideNode , posMeta ) , actorMasks ,outInfo );
	}

	void CityFeature::mergeData( FeatureBase& other , MapTile const& putData , int meta)
	{
		BaseClass::mergeData( other , putData , meta );

		assert( other.type == type );
		CityFeature& otherData = static_cast< CityFeature& >( other );
		MergeData( linkFarms , otherData.linkFarms );
		for( std::set< FarmFeature* >::iterator iter = otherData.linkFarms.begin() , itEnd = otherData.linkFarms.end();
			iter != itEnd ; ++iter )
		{
			FarmFeature* farm = *iter;
			farm->linkCities.erase( &otherData );
		}
	}

	void CityFeature::addNode(MapTile& mapData , unsigned dirMask , SideNode* linkNode)
	{
		BaseClass::addNode( mapData , dirMask , linkNode );

		if ( mSetting->haveUse( EXP_INNS_AND_CATHEDRALS) )
		{
			if ( mapData.getTileContent() & TileContent::eCathedral )
			{
				haveCathedral = true;
			}
		}
	}

	bool CityFeature::checkComplete()
{
		return openCount == 0;
	}

	int CityFeature::calcPlayerScore( int playerId )
	{
		int numTile = mapTiles.size();
		int factor = Value::NonCompleteFactor;
		int pennatFactor = Value::PennatNonCompletFactor;
		if ( mSetting->haveUse( EXP_INNS_AND_CATHEDRALS ) && haveCathedral )
		{
			if ( checkComplete() )
			{
				factor = Value::CityFactor + Value::CathedralAdditionFactor;
				pennatFactor = Value::PennatFactor + Value::CathedralAdditionFactor;
			}
			else
			{
				factor = 0;
				pennatFactor = 0;
			}
		}
		else if ( checkComplete() )
		{
			factor = Value::CityFactor;
			pennatFactor = Value::PennatFactor;
		}

		int numPennats = getSideContentNum( SideContent::ePennant );

		return numTile * factor + numPennats * pennatFactor;
	}

	bool CityFeature::isSamllCircular()
	{
		assert( checkComplete() );
		if ( nodes.size() != 2 )
			return false;
		SideNode* nodeA = nodes[0];
		if ( nodeA->getMapTile()->mTile->isSemiCircularCity( nodeA->getLocalDir() ) == false )
			return false;
		SideNode* nodeB = nodes[1];
		if ( nodeB->getMapTile()->mTile->isSemiCircularCity( nodeB->getLocalDir() ) == false )
			return false;
		return true;
	}

	int CityFeature::calcScore(std::vector< FeatureScoreInfo >& scoreInfos)
	{
		if ( isCastle )
			return -1;
		return BaseClass::calcScore( scoreInfos );
	}

	FarmFeature::FarmFeature()
	{
		haveBarn = false;
	}

	int FarmFeature::getActorPutInfo(int playerId , int posMeta ,std::vector< ActorPosInfo >& outInfo)
	{
		if ( haveActorMask( BIT( ActorType::eBarn ) ) )
			return 0;

		unsigned actorMasks[] = { BIT(ActorType::eMeeple) , 0 };
		if ( mSetting->haveUse( EXP_INNS_AND_CATHEDRALS ) )
		{
			actorMasks[0] |= BIT( ActorType::eBigMeeple );
		}
		if ( mSetting->haveUse( EXP_TRADERS_AND_BUILDERS ) )
		{
			actorMasks[1] |= BIT( ActorType::ePig );
		}
		return getActorPutInfoInternal( playerId , ActorPos( ActorPos::eFarmNode , posMeta ) , actorMasks , ARRAY_SIZE( actorMasks ) , outInfo );
	}

	void FarmFeature::mergeData( FeatureBase& other , MapTile const& putData , int meta)
	{
		BaseClass::mergeData( other , putData , meta );

		FarmFeature& otherData = static_cast< FarmFeature& >( other );
		int idx = nodes.size();
		MergeData( nodes , otherData.nodes );
		MergeData( linkCities , otherData.linkCities );
		for( std::set< CityFeature* >::iterator iter = otherData.linkCities.begin() , itEnd = otherData.linkCities.end();
			iter != itEnd ; ++iter )
		{
			CityFeature* city = *iter;
			city->linkFarms.erase( &otherData );
		}
		for( int i = idx ; i < nodes.size() ; ++i )
			nodes[i]->group = group;
	}

	void FarmFeature::addNode( MapTile& mapData , unsigned idxMask , FarmNode* linkNode)
	{
		addMapTile( mapData );

		unsigned mask = idxMask;
		int idx;
		while ( FBit::MaskIterator< Tile::NumFarm >( mask , idx ) )
		{
			FarmNode& node = mapData.farmNodes[idx];
			assert( node.group == -1 );
			node.group = group;
			nodes.push_back( &node );
		}
	}

	int FarmFeature::calcPlayerScore( int playerId )
	{
		int factor = Value::FarmFactorV3;
		switch ( mSetting->getFarmScoreVersion() )
		{
		case 1: factor = Value::FarmFactorV1; break;
		case 2: factor = Value::FarmFactorV2; break;
		case 3: factor = Value::FarmFactorV3; break;
		}

		if ( haveBarn )
			factor += Value::BarnAddtionFactor;

		return calcPlayerScoreInternal(playerId, factor);

	}

	int FarmFeature::calcScore(std::vector< FeatureScoreInfo >& scoreInfos)
	{
		switch( mSetting->getFarmScoreVersion() )
		{
		case 1:
		case 2:
			return -1;
		case 3:
		default:
			return BaseClass::calcScore( scoreInfos );
			break;
		}

		return 0;
	}

	int FarmFeature::calcPlayerScoreInternal(int playerId, int farmFactor)
	{
		int factor = farmFactor;
		int numCityFinish = 0;
		int numCastle = 0;

		for( std::set< CityFeature* >::iterator iter = linkCities.begin() , itEnd = linkCities.end();
			iter != itEnd ; ++iter )
		{
			CityFeature* city = *iter;
			if ( city->checkComplete() )
			{
				++numCityFinish;
				if ( city->isCastle )
					++numCastle;
			}
		}

		if ( mSetting->haveUse( EXP_TRADERS_AND_BUILDERS ) )
		{
			if ( havePlayerActor( playerId , ActorType::ePig ) )
				factor += Value::PigAdditionFactor;
		}

		return numCityFinish * factor + numCastle;
	}

	int FarmFeature::calcPlayerScoreByBarnRemoveFarmer(int playerId)
	{
		return calcPlayerScoreInternal( playerId , (haveBarn)?( Value::BarnRemoveFarmerFactor ):( Value::FarmFactorV3 ) );
	}

	CloisterFeature::CloisterFeature()
	{
		
	}

	int CloisterFeature::getActorPutInfo(int playerId , int posMeta , std::vector< ActorPosInfo >& outInfo)
	{
		unsigned actorMasks[] = { BIT(ActorType::eMeeple) , 0 };
		if ( mSetting->haveUse( EXP_INNS_AND_CATHEDRALS ) )
		{
			actorMasks[0] |= BIT( ActorType::eBigMeeple );
		}
		return getActorPutInfoInternal( playerId , ActorPos( ActorPos::eTile , posMeta ) , actorMasks , ARRAY_SIZE( actorMasks ) , outInfo );
	}

	void CloisterFeature::mergeData(FeatureBase& other , MapTile const& putData , int meta)
	{
		return;
	}

	int CloisterFeature::calcPlayerScore( int playerId )
	{
		return ( neighborTiles.size() + 1 ) * Value::CloisterFactor;
	}

	int CloisterFeature::calcScore(std::vector< FeatureScoreInfo >& scoreInfos)
	{
		for( int i = 0 ; i < mActors.size() ; ++i )
		{
			LevelActor* actor = mActors[i];

			int majority = getMajorityValue( actor->type );

			if ( majority > 0 )
			{
				scoreInfos.resize(1);
				scoreInfos[0].id = actor->owner->getId();
				scoreInfos[0].majority = majority;
				scoreInfos[0].score = calcPlayerScore( actor->owner->getId() );
				return 1;
			}
		}
		return 0;
	}

	void CloisterFeature::generateRoadLinkFeatures( std::set< unsigned >& outFeatures )
	{
		assert( mapTiles.empty() == false );
		MapTile* mapTile = *mapTiles.begin();

		unsigned roadMask = mapTile->calcRoadMaskLinkCenter() & ~Tile::CenterMask;

		int dir;
		while( FBit::MaskIterator< FDir::TotalNum >( roadMask , dir ) )
		{
			SideNode* linkNode = mapTile->sideNodes[dir].outConnect;
			if ( linkNode == nullptr )
				continue;

			if ( linkNode->group == -1 )
			{
				CAR_LOG("Warnning: No Link Feature In Road Link");
			}
			else
			{
				outFeatures.insert( linkNode->group );
			}
		}
	}

	bool CloisterFeature::updateForNeighborTile(MapTile& tile)
	{
		neighborTiles.push_back( &tile );
		return true;
	}

	bool CloisterFeature::getActorPos(MapTile const& mapTile , ActorPos& actorPos)
	{
		if ( mapTile.group == group )
		{
			actorPos.type = ActorPos::eTile;
			actorPos.meta = 0;
			return true;
		}
		return false;
	}

}//namespace CAR