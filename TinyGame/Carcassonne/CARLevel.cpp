#include "CARLevel.h"

#include "CARMapTile.h"

#include <algorithm>

namespace CAR
{
	static int const gBitIndexMap[ 9 ] = { 0 , 0 , 1 , 1 , 2 , 2 , 2 , 2 , 3 };

	int FBit::ToIndex4( unsigned bit )
	{
		assert( (bit&0xf) == bit );
		assert( ( bit & ( bit - 1 ) ) == 0 );
		return gBitIndexMap[ bit ];
	}
	int FBit::ToIndex8( unsigned bit )
	{
		assert( (bit&0xff) == bit );
		assert( ( bit & ( bit - 1 ) ) == 0 );
		int result = 0;
		if ( bit & 0xf0 ){ result += 4; bit >>= 4; }
		return result + gBitIndexMap[ bit ];
	}
	int FBit::ToIndex32( unsigned bit )
	{
		assert( (bit&0xff) == bit );
		assert( ( bit & ( bit - 1 ) ) == 0 );
		int result = 0;
		if ( bit & 0xffff0000 ){ result += 16; bit >>= 16; }
		if ( bit & 0xff00 ){ result += 8; bit >>= 8; }
		if ( bit & 0xf0 ){ result += 4; bit >>= 4; }
		return result + gBitIndexMap[ bit ];
	}

	unsigned FBit::RotateRight(unsigned bits , unsigned offset , unsigned numBit)
	{
		assert( offset <= numBit );
		unsigned mask = ( 1 << numBit ) - 1;
		return ( ( bits >> offset ) | ( bits << ( numBit - offset ) ) ) & mask;
	}

	unsigned FBit::RotateLeft(unsigned bits , unsigned offset , unsigned numBit)
	{
		assert( offset <= numBit );
		unsigned mask = ( 1 << numBit ) - 1;
		return ( ( bits << offset ) | ( bits >> ( numBit - offset ) ) ) & mask;
	}

	Level::Level()
	{

	}

	void Level::restart()
	{
		mMap.clear();
		mEmptyLinkPosSet.clear();
	}

	MapTile* Level::placeTile(TileId tileId , Vec2i const& pos , int rotation , PutTileParam& param )
	{
		if ( !canPlaceTile( tileId , pos , rotation , param ) )
			return nullptr;
		return placeTileNoCheck( tileId , pos , rotation , param );
	}

	bool Level::canPlaceTile( TileId tileId , Vec2i const& pos , int rotation , PutTileParam& param )
	{
		if ( findMapTile( pos ) != nullptr )
			return false;

		Tile const& tile = getTile( tileId );
		int count = 0;

		bool checkRiverConnect = false;
		int  numRiverConnect = 0;
		MapTile* riverLink = nullptr;
		int  dirRiverLink = -1;
		for( int i = 0 ; i < FDir::TotalNum ; ++i )
		{
			int lDir = FDir::ToLocal( i , rotation );

			if ( param.checkRiverConnect )
			{
				if ( tile.getLinkType(lDir) == SideType::eRiver )
				{
					checkRiverConnect = true;
				}
			}
			Vec2i linkPos = FDir::LinkPos( pos , i );
			MapTile* dataCheck = findMapTile( linkPos );
			if ( dataCheck )
			{
				++count;
				int lDirCheck = FDir::ToLocal( FDir::Inverse( i ) , dataCheck->rotation );
				Tile const& tileCheck = getTile( dataCheck->getId() );
				if ( !Tile::CanLink( tileCheck , lDirCheck , tile , lDir ) )
				{
					if ( param.usageBridge )
					{
						if ( !tileCheck.canLinkRoad( lDirCheck ) )
							return false;

						if ( tile.getLinkType(lDirCheck) != SideType::eField ||
							tile.getLinkType(lDir) == SideType::eField  )
							return false;

						param.dirNeedUseBridge = i;
					}
					else
					{
						return false;
					}
				}

				if ( checkRiverConnect && tileCheck.canLinkRiver( lDirCheck ) )
				{
					++numRiverConnect;
					riverLink = dataCheck;
					dirRiverLink = i;
				}
			}
		}

		if ( count == 0 )
			return false;

		if ( checkRiverConnect )
		{
			if (  numRiverConnect == 0 )
				return false;
			if ( param.checkRiverDirection && numRiverConnect == 1 )
			{
				unsigned linkMask = FBit::RotateLeft( tile.sides[ FDir::ToLocal( dirRiverLink , rotation ) ].linkDirMask , rotation , 4 );
				linkMask &= ~BIT( dirRiverLink );
				if ( linkMask && FBit::Extract( linkMask ) == linkMask )
				{
					if ( !checkRiverLinkDirection( riverLink->pos , FDir::Inverse( dirRiverLink ) , FBit::ToIndex4( linkMask ) ) )
						return false;
				}
			}
		}

		return true;
	}

	bool Level::checkRiverLinkDirection( Vec2i const& pos , int dirLink , int dir )
	{
		Vec2i posLink = pos;
		for(;;)
		{
			MapTile* mapTile = findMapTile( posLink );
			if ( mapTile == nullptr )
				break;
			unsigned mask = mapTile->getSideLinkMask(dirLink);
			mask &= ~BIT( dirLink );
			if ( mask == 0 )
				return true;
			if ( FBit::Extract( mask ) != mask )
				return true;

			int dirCheck = FBit::ToIndex4( mask );
			if ( dirCheck == dir )
				return false;

			dirLink = FDir::Inverse( dirCheck );
			posLink = FDir::LinkPos( posLink , dirCheck );
		}
		return true;
	}


	MapTile* Level::placeTileNoCheck( TileId tileId , Vec2i const& pos , int rotation , PutTileParam& param )
	{
		Tile const& tile = getTile( tileId );

		LevelTileMap::iterator iter = mMap.insert( std::make_pair( pos , MapTile( tile ,rotation ) ) ).first;
		assert( iter != mMap.end() );

		{
			PosSet::iterator iter = mEmptyLinkPosSet.find( pos );
			if ( iter != mEmptyLinkPosSet.end() )
				mEmptyLinkPosSet.erase( iter );
		}

		MapTile& mapData = iter->second;
		mapData.checkCount = mCheckCount;
		mapData.pos        = pos;
		mapData.rotation   = rotation;

		if ( param.usageBridge && param.dirNeedUseBridge != - 1 )
		{
			mapData.addBridge( param.dirNeedUseBridge );
		}

		for( int i = 0 ; i < FDir::TotalNum ; ++i )
		{
			Vec2i posLink = FDir::LinkPos( pos , i );
			
			MapTile* dataCheck = findMapTile( posLink );
			int lDir = FDir::ToLocal( i , rotation );

			if ( dataCheck != nullptr )
			{
				//link node
				assert( Tile::CanLink( tile , lDir , *dataCheck->mTile , 
					    FDir::ToLocal( FDir::Inverse( i ) , dataCheck->rotation ) ) );

				mapData.connectSide( i , *dataCheck );

				//link farm
				if ( tile.canLinkFarm( lDir ) )
				{
					int idx = Tile::DirToFramIndexFrist( i );
					mapData.connectFarm( idx , *dataCheck );
					mapData.connectFarm( idx + 1 , *dataCheck );
				}

			}
			else
			{
				mEmptyLinkPosSet.insert( posLink );
			}
		}


		return &mapData;
	}


	Tile const& Level::getTile(TileId id) const
	{
		TileSet const& tileSet = mTileSetManager->getTileSet( id );
		return *tileSet.tile;
	}

	int Level::getPosibleLinkPos( TileId tileId , std::vector< Vec2i >& outPos , PutTileParam& param )
	{
		int result = 0;
		for( PosSet::iterator iter = mEmptyLinkPosSet.begin() , itEnd = mEmptyLinkPosSet.end();
			iter != itEnd ; ++iter )
		{
			Vec2i const& pos = *iter;
			
			for(int i = 0 ; i < FDir::TotalNum ; ++i )
			{
				if ( canPlaceTile( tileId , pos , i , param ) )
				{
					++result;
					outPos.push_back( pos );
					break;
				}
			}
		}
		return result;
	}

	MapTile* Level::findMapTile(Vec2i const& pos)
	{
		LevelTileMap::iterator iter = mMap.find( pos );
		if ( iter == mMap.end() )
			return nullptr;
		return &iter->second;
	}

	void Level::incCheckCount()
	{
		++mCheckCount;
		if ( mCheckCount == 0 )
		{
			for( LevelTileMap::iterator iter = mMap.begin() , itEnd = mMap.end();
				iter != itEnd ; ++iter )
			{
				iter->second.checkCount = 0;
			}
		}
	}

	bool Level::isEmptyLinkPos(Vec2i const& pos)
	{
		return mEmptyLinkPosSet.find( pos ) != mEmptyLinkPosSet.end();
	}

	TileSetManager::TileSetManager()
	{

	}


	TileSetManager::~TileSetManager()
	{
		cleanup();
	}

	void TileSetManager::cleanup()
	{
		for( int i = 0 ; i < mTileMap.size(); ++i )
		{
			delete mTileMap[i].tile;
		}

		for( int i = 0 ; i < TileSet::NumGroup ; ++i )
		{
			mSetMap[i].clear();
		}
		mTileMap.clear();

	}

	void TileSetManager::import( ExpansionTileContent const& content )
	{
		TileId id = 0;
		for ( int i = 0 ; i < content.numDefines ; ++i )
		{
			TileDefine& tileDef = content.defines[i];
			TileSet& tileSet = createTileSet( tileDef );
			tileSet.expansions = content.exp;
			tileSet.idxDefine = i;
		}
	}

	TileSet& TileSetManager::createTileSet(TileDefine const& tileDef)
	{
		Tile* tile = new Tile;
		
		TileId id = (TileId)mTileMap.size();
		tile->id = id;
		setupTile( *tile , tileDef );

		TileSet tileSet;
		tileSet.numPiece = tileDef.numPiece;
		tileSet.tile = tile;
		tileSet.tag = tileDef.tag;

		mTileMap.push_back( tileSet );

		TileSet::EGroup group = TileSet::eCommon;
		if ( tile->haveRiver() )
		{
			group = TileSet::eRiver;
		}

		tileSet.group = group;
		mSetMap[ group ].push_back( id );

		return mTileMap[ id ];
	}

	void TileSetManager::setupTile(Tile& tile , TileDefine const& tileDef)
	{
		for( int i = 0 ; i < Tile::NumSide ; ++i )
		{
			tile.sides[i].linkType = (SideType)tileDef.linkType[i];
			tile.sides[i].contentFlag = tileDef.sideContent[i];
			tile.sides[i].linkDirMask = BIT(i);
			tile.sides[i].roadLinkDirMask = 0;
		}

		tile.contentFlag = tileDef.content;

		for( int i = 0 ; i < ARRAY_SIZE( tileDef.sideLink ) ; ++i )
		{
			if ( tileDef.sideLink[i] == 0 )
				break;
			assert( ( tileDef.sideLink[i] & ~0xf ) == 0 );

			unsigned linkMask = tileDef.sideLink[i];
			
			int idx;
#if _DEBUG
			int idxStart = -1;
#endif
			while ( FBit::MaskIterator< 8 >( linkMask , idx ) )
			{
#if _DEBUG
				if ( idxStart == -1 ){  idxStart = idx;  }
				assert( Tile::CanLink( tile.sides[idx].linkType , tile.sides[idxStart].linkType ) );
#endif
				tile.sides[idx].linkDirMask = tileDef.sideLink[i];
			}
		}

		for( int i = 0 ; i < ARRAY_SIZE( tileDef.roadLink ) ; ++i )
		{
			if ( tileDef.roadLink[i] == 0 )
				break;

			unsigned linkMask = tileDef.roadLink[i] & ~Tile::CenterMask;
			int idx;
			while ( FBit::MaskIterator< 8 >( linkMask , idx ) )
			{
				tile.sides[idx].roadLinkDirMask |= tileDef.roadLink[i];
			}
		}

		unsigned noFramIndexMask = 0;
		for( int i = 0 ; i < FDir::TotalNum ; ++i )
		{
			if ( tile.canRemoveFarm(i) )
			{
				int idx = Tile::DirToFramIndexFrist( i );
				noFramIndexMask |= BIT(idx)|BIT(idx+1);
			}
		}
		noFramIndexMask &= ~tileDef.centerFarmMask;

		for( int i = 0 ; i < Tile::NumFarm ; ++i )
		{
			tile.farms[i].sideLinkMask = 0;
			if ( noFramIndexMask & BIT(i) )
				tile.farms[i].farmLinkMask = 0;
			else
				tile.farms[i].farmLinkMask = BIT(i);
		}

		unsigned farmLinkRe = Tile::AllFarmMask;
		for( int i = 0 ; i < ARRAY_SIZE( tileDef.farmLink ) ; ++i )
		{
			if ( tileDef.farmLink[i] == 0 )
				break;

			unsigned usageFarmLink = tileDef.farmLink[i];
			if ( usageFarmLink == 0xff )
			{
				usageFarmLink = farmLinkRe;
			}
			farmLinkRe &= ~tileDef.farmLink[i];

			unsigned linkMask = usageFarmLink & ~noFramIndexMask;
			unsigned farmSideLink = calcFarmSideLinkMask( usageFarmLink );
			unsigned mask = linkMask;
			int idx;
			while ( FBit::MaskIterator< Tile::NumFarm >( mask , idx ) )
			{
				tile.farms[idx].farmLinkMask = linkMask;
				tile.farms[idx].sideLinkMask = farmSideLink;
			}
		}
	}

	unsigned TileSetManager::calcFarmSideLinkMask(unsigned linkMask)
	{
		unsigned result = 0;
		while( linkMask )
		{
			unsigned bit = FBit::Extract( linkMask );
			int idx = FBit::ToIndex8( bit );
			result |= BIT( Tile::FarmSideDir( idx ) );
			linkMask &= ~bit;
		}
		return result;
	}


}//namespace CAR

