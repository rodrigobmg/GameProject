#include "TinyGamePCH.h"
#include "GoCore.h"

#include "SocketBuffer.h"

#include <cassert>
#include <iostream>
#include <fstream>


namespace Go {

	char const EDGE_MARK    = 0x80;
	char const VISITED_MARK = 0x40; 


	static int const gDirOffset[4][2] = 
	{
		1,0,  -1,0,  0,-1,  0,1,
	};

	Board::Board() 
		:mSize(0)
	{
	}

	Board::~Board()
	{
	}

	void Board::copy(Board const& other)
	{
		if( mSize != other.mSize )
		{
			setup(other.mSize, false);
		}
		std::copy(other.mLinkIndex.get(), other.mLinkIndex.get() + getDataSize(), mLinkIndex.get());
		std::copy(other.mData.get(), other.mData.get() + getDataSize(), mData.get());
	}

	void Board::setup(int size, bool bClear)
	{
		if ( mSize < size )
		{
			mSize = size;
			int dataSize = getDataSize();
			mData.reset( new char[ dataSize ] );
			mLinkIndex.reset( new LinkType[ dataSize ] );	
		}
		else
		{
			mSize = size;
		}
		mIndexOffset[eLeft] = -1;
		mIndexOffset[eRight] = 1;
		mIndexOffset[eTop] = -getDataSizeX();
		mIndexOffset[eBottom] = getDataSizeX();

		if ( bClear )
			clear();
	}

	void Board::clear()
	{
		int dataSize = getDataSize();

		std::fill_n( mLinkIndex.get(), dataSize, 0);
		std::fill_n( mData.get(), dataSize , (char)StoneColor::eEmpty );

		for ( int i = getDataSizeX() - 1 ; i < dataSize ; i += getDataSizeX() )
		{
			mData[i] = EDGE_MARK;
		}
		{

			int offsetTop = dataSize - getDataSizeX();
			char* top = mData.get();
			char* bottom = mData.get() + offsetTop;
			for( int i = 0; i < getDataSizeX(); ++i )
			{
				*(top++) = *(bottom++) = EDGE_MARK;
			}
		}	
	}

	int Board::getCaptureCount( int x , int y ) const
	{
		assert( getData( x , y ) != StoneColor::eEmpty );
		return getCaptureCount( getDataIndex( x, y ) );
	}

	int Board::getCaptureCount( int index ) const
	{
		assert( getData( index ) != StoneColor::eEmpty );
		int idxRoot = getLinkRoot( index );
		return -mLinkIndex[ idxRoot ];
	}

	void Board::putStone( int idx, DataType color )
	{
		assert( color != StoneColor::eEmpty );
		assert( mData[idx] == StoneColor::eEmpty );
		mData[ idx ] = color;

		mCacheIdxConRoot = idx;
		mLinkIndex[ idx ] = 0;

		for( int i = 0 ; i < NumDir ; ++i )
		{
			int idxCon = calcLinkIndex( idx , i );
			char data = getData( idxCon );

			if ( data == EDGE_MARK )
				continue;

			if ( data == StoneColor::eEmpty )
			{
				addRootCaptureCount( mCacheIdxConRoot , 1 );
			}
			else
			{
				int idxRoot = getLinkRoot( idxCon );
				if ( data == color && mCacheIdxConRoot != idxRoot )
				{
					linkRoot( idxRoot );
				}
				else
				{
					addRootCaptureCount( idxRoot , -1 );
				}
			}
		}
	}

	int Board::fillLinkedStone( Pos const& p , DataType color )
	{
		mCacheColorR = color;
		assert( color != StoneColor::eEmpty );
		int idx = p.toIndex();
		return fillLinkedStone_R( idx );
	}

	int Board::fillLinkedStone_R( int idx )
	{
		char data = getData( idx );
		if ( data != StoneColor::eEmpty )
			return 0;

		putStone( idx , mCacheColorR );

		int result = 1;
		for( int i = 0 ; i < Board::NumDir ; ++i )
		{
			int idxCon = calcLinkIndex( idx , i );
			result += fillLinkedStone_R( idxCon );
		}

		return result;
	}

	void Board::linkRoot( int idxRoot )
	{
		assert( idxRoot != mCacheIdxConRoot );

		int life1 = mLinkIndex[ mCacheIdxConRoot ];
		int life2 = mLinkIndex[ idxRoot ];

		int totalLive = life1 + life2 + 1;
		if ( life1 >= life2 )
		{
			mLinkIndex[ mCacheIdxConRoot ] = idxRoot;
			mLinkIndex[ idxRoot ] = totalLive;
			mCacheIdxConRoot = idxRoot;
		}
		else
		{
			mLinkIndex[ idxRoot ] = mCacheIdxConRoot;
			mLinkIndex[ mCacheIdxConRoot ] = totalLive;
		}
	}

	void Board::removeStone( Pos const& p )
	{
		int  idx = p.toIndex();
		char data = getData( idx );
		assert( data != StoneColor::eEmpty );

		mData[ idx ] = StoneColor::eEmpty;

		for( int dir = 0 ; dir < Board::NumDir ; ++dir )
		{
			int idxCon = calcLinkIndex( idx , dir );
			char dataCon = mData[ idxCon ];

			//if( dataCon & VISITED_MARK )
				//continue;
			if ( dataCon == StoneColor::eBlack || dataCon == StoneColor::eWhite )
			{
				if( dataCon != data )
				{
					addRootCaptureCount(getLinkRoot(idxCon), 1);
				}
				else
				{
					relink(idxCon);
				}
			}
		}

		for( int dir = 0 ; dir < Board::NumDir ; ++dir )
		{
			int idxCon = calcLinkIndex( idx , dir );
			removeVisitedMark_R( idxCon );
		}
	}

	Board::DataType Board::getData( int x , int y ) const 
	{
		int idx = getDataIndex( x , y );
		assert( getData( idx ) != EDGE_MARK );
		return DataType( getData( idx ) );
	}

	bool Board::checkRange( int x , int y ) const
	{
		return 0 <= x && x < mSize && 0 <= y && y < mSize;
	}

	int Board::getLinkRoot( int idx ) const
	{
#if 0
		int result = idx;
		while( mLinkIndex[result] > 0 )
		{
			result = mLinkIndex[result];
		}
		//mLinkIndex[idx] = result;
		return result;
#else
		if( mLinkIndex[idx] <= 0 )
			return idx;

		int prev = idx;
		int result = mLinkIndex[idx];
		int next;
		while(  ( next = mLinkIndex[result] ) > 0 )
		{
			mLinkIndex[prev] = next;
			prev = result;
			result = next;
		}

		mLinkIndex[idx] = result;
		return result;
#endif


	}

	int Board::getLinkToRootDist(int idx) const
	{
		int result = 0;
		while( mLinkIndex[idx] > 0 )
		{
			++result;
			idx = mLinkIndex[idx];
		}
		return result;
	}

	void Board::addRootCaptureCount(int idxRoot, int val)
	{
		assert( getLinkRoot( idxRoot ) == idxRoot );
		mLinkIndex[ idxRoot ] -= val;
		assert(mLinkIndex[idxRoot] <= 0);
	}

	int Board::getLiberty_R(int idx) const
	{
		//#TODO
		DataType data = getData(idx);
		if( data == EDGE_MARK )
			return 0;

		return 0;
	}


	int Board::captureLinkedStone(Pos const& p)
	{
		mCacheColorR = getData(p);
		assert(mCacheColorR != StoneColor::eEmpty);
		return captureLinkedStone_R(p.toIndex());
	}

	int Board::captureLinkedStone_R(int idx)
	{
		DataType data = getData( idx );

		if ( data != mCacheColorR )
		{
			if( data == EDGE_MARK || data == StoneColor::eEmpty )
			{

			}
			else
			{
				int idxRoot = getLinkRoot(idx);
				addRootCaptureCount(idxRoot, 1);
			}
			return 0;
		}

		mData[ idx ] = StoneColor::eEmpty;

		int result = 1;
		for( int dir = 0 ; dir < Board::NumDir ; ++dir )
			result += captureLinkedStone_R( calcLinkIndex( idx , dir ) );

		return result;
	}

	int Board::peekCaptureStone(Pos const& p, unsigned& outDirMask ) const
	{
		mCacheColorR = StoneColor::Opposite(getData(p));
		assert(mCacheColorR != StoneColor::eEmpty);

		int numCapture = 0;
		for( int dir = 0; dir < Board::NumDir; ++dir )
		{
			Pos posCon;
			if( !getLinkPos(p, dir, posCon) )
				continue;

			DataType dataCon = getData(posCon);
			//if ( dataCon & VISITED_MARK )
				//continue;

			if( dataCon != mCacheColorR )
				continue;

			if( getCaptureCount(posCon) != 0 )
				continue;

			numCapture += peekCaptureConStone(posCon);
			outDirMask |= (1 << dir);
		}

		for( int dir = 0; dir < Board::NumDir; ++dir )
		{
			if( BIT(dir) & outDirMask )
			{
				removeVisitedMark_R(calcLinkIndex(p.toIndex(), dir));
			}		
		}
		return numCapture;
	}

	int Board::peekCaptureConStone(Pos const& p) const
	{
		int idx = p.toIndex();
		DataType data = getData(idx);
		assert(data == mCacheColorR);
		assert((data & VISITED_MARK) == 0);
		mData[idx] |= VISITED_MARK;
		int result = 1;
		for( int dir = 0; dir < Board::NumDir; ++dir )
			result += peekCaptureLinkedStone_R(calcLinkIndex(idx, dir));

		return result;
	}

	int Board::peekCaptureLinkedStone_R(int idx) const
	{
		if( mData[idx] != mCacheColorR )
			return 0;

		mData[idx] |= VISITED_MARK;

		int result = 1;
		for( int dir = 0; dir < Board::NumDir; ++dir )
			result += peekCaptureLinkedStone_R(calcLinkIndex(idx, dir));

		return result;
	}

	int Board::relink(int idx)
	{
		DataType data = getData( idx );
		assert( data != StoneColor::eEmpty && data != EDGE_MARK );
		assert( (data & VISITED_MARK) == 0 );
		mData[ idx ] |= VISITED_MARK;
		mCacheIdxConRoot = idx;
		mCacheColorR = DataType( data );

		int life = 0;
		for( int dir = 0 ; dir < Board::NumDir ; ++dir )
		{
			life += relink_R( calcLinkIndex( idx , dir ) );
		}
		mLinkIndex[ idx ] = -life;
		return life;
	}

	int Board::relink_R( int idx )
	{
		DataType data = mData[ idx ];

		if ( data == StoneColor::eEmpty )
			return 1;

		if ( data != mCacheColorR )
			return 0;

		mData[ idx ] |= VISITED_MARK;
		mLinkIndex[ idx ] = mCacheIdxConRoot;

		int life = 0;
		for( int dir = 0 ; dir < Board::NumDir ; ++dir )
			life += relink_R( calcLinkIndex( idx , dir ) );

		return life;
	}

	void Board::removeVisitedMark_R( int idx ) const
	{
		if ( mData[ idx ] & VISITED_MARK )
		{
			mData[ idx ] &= ~VISITED_MARK;
			for( int dir = 0 ; dir < Board::NumDir ; ++dir )
				removeVisitedMark_R( calcLinkIndex( idx , dir ) );
		}
	}

	bool Board::getLinkPos( Pos const& pos , int dir , Pos& result ) const
	{
		int idx = calcLinkIndex( pos.index , dir );
		
		if ( getData( idx ) == EDGE_MARK )
			return false;

		result.index = idx;
		return true;
	}

	Game::Game()
	{

	}

	void Game::setup( int size )
	{
		mBoard.setup( size );
		doRestart( false );
	}

	void Game::restart()
	{
		doRestart( true );
	}

	void Game::doRestart(bool beClearBoard , bool bClearStepHistory)
	{
		if ( beClearBoard )
			mBoard.clear();

		if( bClearStepHistory )
			mStepHistory.clear();

		mCurrentStep  = 0;
		
		mNumBlackCaptured = 0;
		mNumWhiteCaptured = 0;

		mSimpleKOStates.clear();

		mNextPlayColor = mSetting.bBlackFrist ? StoneColor::eBlack : StoneColor::eWhite;
		if( mSetting.numHandicap )
		{
			int const posMax = mBoard.getSize() - 1;
			int const posMid = posMax / 2;
			int const starOffset = 3;
			int const startMaxOffset = posMax - starOffset;
#define HANDICAP_POS( POSX , POSY )\
	 mBoard.putStone(mBoard.getPos((POSX), (POSY)), StoneColor::eBlack);
	
			switch( mSetting.numHandicap )
			{
			case 5:
				HANDICAP_POS(posMid, posMid);
				goto Mark4;
			case 7:
				HANDICAP_POS(posMid, posMid);
				goto Mark6;
			case 9:
				HANDICAP_POS(posMid, posMid);
			case 8:
				HANDICAP_POS(posMid, starOffset);
				HANDICAP_POS(posMid, startMaxOffset);
			Mark6:
			case 6:
				HANDICAP_POS(startMaxOffset, posMid);
				HANDICAP_POS(starOffset, posMid);
			Mark4:
			case 4:
				HANDICAP_POS(startMaxOffset, starOffset);
			case 3:
				HANDICAP_POS(starOffset, startMaxOffset);
			case 2:
				HANDICAP_POS(starOffset, starOffset);
				HANDICAP_POS(startMaxOffset, startMaxOffset);
			}
#undef HANDICAP_POS
		}
	}

	bool Game::canPlay(int x, int y) const
	{
		if( !mBoard.checkRange(x, y) )
			return false;

		Pos pos = mBoard.getPos(x, y);

		if( mBoard.getData(pos) != StoneColor::eEmpty )
			return false;

		mBoard.putStone(pos, mNextPlayColor);

		unsigned bitDir = 0;
		int numCapture = const_cast<Game&>(*this).captureStone(pos, bitDir);
		bool result = true;
		if (numCapture == 0)
		{
			if (mBoard.getCaptureCount(pos) == 0)
			{
				result = false;
			}
		}
		else
		{
			KOState koState = calcKOState(pos);
			if (isKOStateReached(pos, mNextPlayColor, koState))
			{
				result = false;
			}

			for (int dir = 0; dir < Board::NumDir; ++dir)
			{
				if (bitDir & BIT(dir))
				{
					Pos linkPos;
					bool isOK = mBoard.getLinkPos(pos, dir, linkPos);
					assert(isOK);
					mBoard.fillLinkedStone(linkPos, StoneColor::Opposite(mNextPlayColor));
				}
			}
		}

		mBoard.removeStone(pos);
		return result;
	}

	bool Game::addStone(int x, int y, DataType color)
	{
		if( !mBoard.checkRange(x, y) )
			return false;

		Pos pos = mBoard.getPos(x, y);
		addStoneInternal(pos, color, false);
		return true;
	}

	bool Game::playStone(int x, int y)
	{
		if( !mBoard.checkRange(x, y) )
			return false;

		Pos pos = mBoard.getPos(x, y);
		return playStone(pos);
	}

	void Game::addStoneInternal(Pos const& pos, DataType color, bool bReviewing)
	{
		DataType curColor = mBoard.getData(pos);

		if( curColor != color )
		{
			mBoard.removeStone(pos);
			if ( color != StoneColor::eEmpty )
				mBoard.putStone(pos, color);
		}
		
		if( !bReviewing )
		{
			StepInfo info;
			info.colorAdded = color;
			info.bPlay = 0;
			info.idxPos = pos.toIndex();
			mStepHistory.push_back(info);
		}
		++mCurrentStep;
	}


	bool Game::playStoneInternal( Pos const& pos , bool bReviewing )
	{
		if ( mBoard.getData( pos ) != StoneColor::eEmpty )
			return false;

		mBoard.putStone( pos , mNextPlayColor );

		unsigned bitDir = 0;
		int numCapture = captureStone( pos , bitDir );
		KOState koState = calcKOState(pos);

		bool result = true;
		if ( numCapture == 0 )
		{
			if ( mBoard.getCaptureCount( pos ) == 0 )
			{
				mBoard.removeStone( pos );
				return false;
			}
		}
		else
		{
			if ( isKOStateReached(pos , mNextPlayColor , koState ) )
			{
				for (int dir = 0; dir < Board::NumDir; ++dir)
				{
					if (bitDir & BIT(dir))
					{
						Pos linkPos;
						bool isOK = mBoard.getLinkPos(pos, dir, linkPos);
						assert(isOK);
						mBoard.fillLinkedStone(linkPos, StoneColor::Opposite(mNextPlayColor));
					}
				}
				mBoard.removeStone( pos );
				return false;
			}
		}

		addKOState(mNextPlayColor, &pos ,  numCapture, koState);

		if ( !bReviewing )
		{
			StepInfo info;
			info.captureDirMask = bitDir;
			info.idxPos = pos.toIndex();
			info.bPlay = 1;
			mStepHistory.push_back(info);
		}

		++mCurrentStep;
		if ( mNextPlayColor == StoneColor::eBlack )
		{
			mNumWhiteCaptured += numCapture;
			mNextPlayColor = StoneColor::eWhite;
		}
		else
		{
			mNumBlackCaptured += numCapture;
			mNextPlayColor = StoneColor::eBlack;
		}
		return true;
	}

	void Game::playPassInternal( bool bReviewing )
	{
		addKOState(mNextPlayColor, nullptr , 0 , KOState::Invalid());

		if (!bReviewing)
		{
			StepInfo info;
			info.idxPos = -1;
			info.captureDirMask = 0;
			info.bPlay = 1;
			mStepHistory.push_back(info);
		}

		++mCurrentStep;
		if( mNextPlayColor == StoneColor::eBlack )
		{
			mNextPlayColor = StoneColor::eWhite;
		}
		else
		{
			mNextPlayColor = StoneColor::eBlack;
		}
	}

	int Game::captureStone( Board::Pos const& pos , unsigned& bitDir )
	{
		DataType dataCapture = StoneColor::Opposite( mBoard.getData( pos ) );

		int numCapture = 0;
		for( int dir = 0 ; dir < Board::NumDir ; ++dir )
		{
			Pos posCon;
			if ( !mBoard.getLinkPos( pos , dir , posCon ) )
				continue;

			DataType dataCon = mBoard.getData( posCon );

			if ( dataCon != dataCapture )
				continue;

			if ( mBoard.getCaptureCount( posCon ) != 0 )
				continue;

			numCapture += mBoard.captureLinkedStone( posCon );
			bitDir |= ( 1 << dir );
		}

		return numCapture;
	}

	bool Game::undoInternal( bool bReviewing )
	{
		if( mCurrentStep == 0 )
			return false;

		StepInfo& step = mStepHistory[ mCurrentStep - 1];

		if( step.bPlay )
		{
			DataType color = StoneColor::Opposite(mNextPlayColor);

			if( step.idxPos != -1 )
			{
				Board::Pos posRemove = mBoard.getPos(step.idxPos);

				if( step.captureDirMask )
				{
					int num = 0;
					for( int dir = 0; dir < Board::NumDir; ++dir )
					{
						if( step.captureDirMask & BIT(dir) )
						{
							Board::Pos p;
							if( mBoard.getLinkPos(posRemove, dir, p) )
							{
								num += mBoard.fillLinkedStone(p, mNextPlayColor);
							}
							else
							{


							}
							
						}
					}

					if( color == StoneColor::eBlack )
					{
						mNumWhiteCaptured -= num;
					}
					else
					{
						mNumBlackCaptured -= num;
					}
				}

				mBoard.removeStone(posRemove);
				removeKOState(color, &posRemove );
			}
			else
			{
				removeKOState(color, nullptr );
			}
			mNextPlayColor = color;
		}
		else
		{
			Pos posRemove = mBoard.getPos(step.idxPos);
			mBoard.removeStone(posRemove);
		}

		--mCurrentStep;
		if ( !bReviewing )
			mStepHistory.pop_back();

		return true;
	}

	void Game::copy(Game const& other)
	{
#define ASSIGN_VAR( VAR ) VAR = other.VAR;
		mBoard.copy(other.mBoard);
		ASSIGN_VAR(mSetting);
		ASSIGN_VAR(mCurrentStep);
		ASSIGN_VAR(mNumBlackCaptured);
		ASSIGN_VAR(mNumWhiteCaptured);
		ASSIGN_VAR(mSimpleKOStates);
		ASSIGN_VAR(mNextPlayColor);
		ASSIGN_VAR(mStepHistory);
#undef ASSIGN_VAR
	}

	static void EmitOp(IGameCopier& copier, Board const& board,  Game::StepInfo const& info , int& color )
	{
		if( info.bPlay )
		{
			if( info.idxPos == -1 )
			{
				copier.emitPlayPass(color);
			}
			else
			{
				int coord[2];
				board.getPosCoord(info.idxPos, coord);
				copier.emitPlayStone(coord[0], coord[1], color);
				color = StoneColor::Opposite(color);
			}
		}
		else
		{
			int coord[2];
			board.getPosCoord(info.idxPos, coord);
			copier.emitAddStone(coord[0], coord[1], info.colorAdded);
		}
	}
	void Game::synchronizeState(IGameCopier& copier, bool bReviewOnly) const
	{
		copier.emitSetup(mSetting);

		int color = mSetting.bBlackFrist ? StoneColor::eBlack : StoneColor::eWhite;
		int indexEnd = (bReviewOnly) ? mCurrentStep : mStepHistory.size();
		for( int i = 0; i < indexEnd; ++i )
		{
			auto const& info = mStepHistory[i];
			EmitOp(copier, mBoard, info, color);
		}
	}

	void Game::synchronizeStateKeep(IGameCopier& copier , int startStep, bool bReviewOnly) const
	{
		int color = mNextPlayColor;

		int playCount = 0;
		if( startStep < mCurrentStep )
		{
			for( int i = startStep; i < mCurrentStep; ++i )
			{
				if( mStepHistory[i].bPlay )
					++playCount;
			}
		}
		else if( startStep > mCurrentStep ) //review state
		{
			if( bReviewOnly )
			{
				for( int i = startStep; i > mCurrentStep; --i )
				{
					copier.emitUndo();
				}
				return;
			}

			if( startStep >= mStepHistory.size() )
			{
				LogError("Can't sync game state : Error start Step!!");
				return;
			}
			for( int i = startStep; i > mCurrentStep; --i )
			{
				if( mStepHistory[i].bPlay )
					++playCount;
			}
		}

		if( playCount % 2 )
		{
			color = StoneColor::Opposite(color);
		}

		int indexEnd = (bReviewOnly) ? mCurrentStep : mStepHistory.size();
		for( int i = startStep; i < indexEnd; ++i )
		{
			auto const& info = mStepHistory[i];
			EmitOp(copier, mBoard, info, color);
		}
	}

	void Game::print(int x, int y)
	{
		using namespace std;
		static char const* dstr[] = 
		{
			"┌","┬","┐",
			"├","┼","┤",
			"└","┴","┘",
		};

		int size = mBoard.getSize();

		for( int j = 0 ; j < size ; ++ j )
		{
			for( int i = 0 ; i < size ; ++ i )
			{
				if ( i == x && j == y )
				{
					cout << "⊕";
					continue;
				}

				switch ( mBoard.getData( i , j ) )
				{
				case StoneColor::eBlack: cout << "○" ; break;
				case StoneColor::eWhite: cout << "●" ; break;
				case StoneColor::eEmpty:
					{
						int index = 0;
						if ( i != 0 )
						{ 
							index += ( i != ( size - 1 ) ) ? 1 : 2;
						}
						if ( j != 0 )
						{
							index += 3 * ( ( j != ( size - 1 ) ) ? 1 : 2 );
						}
						cout << dstr[ index ];
					}
					break;
				}
			}
			cout << endl;
		}

		for ( int dir = 0 ; dir < Board::NumDir ; ++dir )
		{
			int nx = x + gDirOffset[dir][0];
			int ny = y + gDirOffset[dir][1];

			if ( !mBoard.checkRange( nx , ny ) )
				continue;
			DataType nType = mBoard.getData( nx , ny );
			if ( nType == StoneColor::eEmpty )
				continue;
			int life = mBoard.getCaptureCount( nx , ny ) ;
			std::cout << "dir =" << dir << " life = "<< life << std::endl;
		}
	}


	bool Game::isKOStateReached(Pos const& pos, DataType playColor, KOState const& koState) const
	{
		if (koState == KOState::Invalid())
			return false;

		if (mSimpleKOStates.size() < 2)
			return false;

		return koState == mSimpleKOStates[mSimpleKOStates.size() - 1];
	}

	Game::KOState Game::calcKOState(Pos const& pos) const
	{
		return{ pos.toIndex() , 0 };
	}

	void Game::addKOState(DataType playColor, Pos const* pos,  int numCapture, KOState const& koState)
	{
		mSimpleKOStates.push_back(koState);
	}

	void Game::removeKOState(DataType playColor , Pos const* pos)
	{
		mSimpleKOStates.pop_back();
	}

	void Game::takeData(SocketBuffer& buffer)
	{
		unsigned size = mStepHistory.size();
		buffer.fill( size );

		for( int i = 0 ; i < mStepHistory.size() ; ++i )
		{
			StepInfo& info = mStepHistory[i];
			buffer.fill( info.idxPos );
		}
	}

	bool Game::restoreData( SocketBuffer& buffer )
	{
		mStepHistory.clear();

		restart();

		unsigned size;
		buffer.take( size );

		for( int i = 0 ; i < size ; ++i )
		{
			int idxPos;
			buffer.take( idxPos );
			if ( idxPos == -1 )
			{
				playPass();
			}
			else if ( playStone( mBoard.getPos( idxPos ) ) )
			{
				return false;
			}
		}
		return true;
	}

	bool Game::saveSGF( char const* path , GameDescription const* description ) const
	{
		std::ofstream fs( path );
		if ( !fs.is_open() )
			return false;

		fs << "(;";
		fs << "FF[4]";
		fs << "CA[UTF-8]";

		if( description )
		{
			fs << "PB[" << description->blackName << ']';
			fs << "PW[" << description->whiteName << ']';
			fs << "DT[" << description->date << ']';
		}

		fs << "SZ[" << mBoard.getSize() << ']';
		fs << "KM[" << mSetting.komi << ']';


		bool bBlack = mSetting.bBlackFrist;

		for( StepInfo const& step : mStepHistory )
		{
			if ( step.bPlay )
			{
				fs << ';' << ((bBlack) ? 'B' : 'W');
				if( step.idxPos != -1 )
				{
					int coord[2];
					mBoard.getPosCoord(step.idxPos, coord);
					fs << '[' << char('a' + coord[0]) << char('a' + coord[1]) << ']';
				}
				bBlack = !bBlack;
			}
			else
			{
				fs << ';' << ((step.colorAdded == StoneColor::eBlack) ? 'AB' : 'AW');

				int coord[2];
				mBoard.getPosCoord(step.idxPos, coord);
				fs << '[' << char('a' + coord[0]) << char('a' + coord[1]) << ']';
			}
		}

		fs << ')';
		return true;
	}

	bool Game::getStepPos(int step, int outPos[2]) const
	{
		if( 0 > step || step >= mStepHistory.size()  )
			return false;

		StepInfo const& info = mStepHistory[step];

		if( info.idxPos == -1 )
			return false;
		mBoard.getPosCoord(info.idxPos, outPos);
		return true;
	}

	int Game::getLastPassCount() const
	{
		int count = 0;
		for( int i = mStepHistory.size() - 1; i >= 0; --i )
		{
			if( mStepHistory[i].idxPos >= 0 )
				break;

			++count;
		}
		return count;
	}

	bool Game::isReviewing() const
	{
		return mCurrentStep < mStepHistory.size();
	}

	int Game::reviewBeginStep()
	{
		int result = mCurrentStep;
		doRestart(true, false);
		return result;
	}

	int Game::reviewPrevSetp(int numStep /*= 1*/)
	{
		int result = 0;
		for( int i = 0; i < numStep; ++i )
		{
			if( !undoInternal(true) )
				break;

			++result;
		}
		return result;
	}

	void Game::advanceStepFromHistory()
	{
		assert(mCurrentStep < mStepHistory.size());
		StepInfo const& info = mStepHistory[mCurrentStep];
		if( info.bPlay )
		{
			if( info.idxPos == -1 )
			{
				playPassInternal(true);
			}
			else
			{
				playStoneInternal(mBoard.getPos(info.idxPos), true);
			}
		}
		else
		{
			addStoneInternal(mBoard.getPos(info.idxPos), info.colorAdded, true);
		}
	}

	int Game::reviewNextStep(int numStep /*= 1*/)
	{
		int result = 0;
		for( int i = 0; i < numStep; ++i )
		{
			if( mCurrentStep >= mStepHistory.size() )
				break;
			advanceStepFromHistory();
			++result;
		}
		return result;
	}

	int Game::reviewLastStep()
	{
		int result = 0;
		while( mCurrentStep < mStepHistory.size() )
		{
			advanceStepFromHistory();
			++result;
		}
		return result;
	}

}//namespace Go