#include "RubiksStage.h"

namespace Rubiks
{

	void Solver::run()
	{
		if ( mInitState.isEqual( mFinalState ) )
			return;

		cleanup();
		class FindRunable : public RunnableThreadT< FindRunable >
		{

		public:
			unsigned run()
			{
				solver->run_FindThread();
				return 0;
			}
			void exit(){ delete this; }
			Solver* solver;
		};

		mbRunning = true;
		FindRunable* findWork = new FindRunable;
		findWork->solver = this;
		findWork->start();

		{
			StateNode* node = new StateNode;
			node->state = mInitState;
			node->parent = nullptr;
			Mutex::Locker locker( mRequestFindMutex );
			mRequestFindNodes.push_back( node );
			mRequestFindCond.notifyAll();
		}


		for(;;)
		{
			StateNode* node = nullptr;
			{
				Mutex::Locker locker( mUncheckMutex );
				mUncheckCond.waitUntil( locker , fastdelegate::FastDelegate< bool () >( this , &Solver::haveUncheck ) );

				if ( mbRunning == false )
					break;

				node = mUncheckNodes.front();
				mUncheckNodes.pop_front();
			}

			if ( node == nullptr )
				break;

			if ( mCheckedStates.find( &node->state ) == mCheckedStates.end() )
			{
				if ( mFinalState.isEqual( node->state ) )
				{
					solveSuccess();
					return;
				}
				mCheckedStates.insert( &node->state );
				{
					Mutex::Locker locker( mRequestFindMutex );
					mRequestFindNodes.push_back( node );
					mRequestFindCond.notifyAll();
				}
			}
			else
			{
				int i = 1;

			}
		}

		findWork->join();
	}

	void Solver::term()
	{
		Mutex::Locker locker( mRequestFindMutex );
		Mutex::Locker locker2( mUncheckMutex );
		mbRunning = false;

		mRequestFindCond.notifyAll();
		mUncheckCond.notifyAll();
	}

	void Solver::run_FindThread()
	{
		for(;;)
		{
			StateNode* node;
			{
				Mutex::Locker locker( mRequestFindMutex );
				mRequestFindCond.waitUntil( locker , fastdelegate::FastDelegate< bool () >( this , &Solver::haveReauestFind ) );

				if ( mbRunning == false )
				{
					return;
				}
				node = mRequestFindNodes.front();
				mRequestFindNodes.pop_front();
			}
			assert( node );

			int const NewStateNum = 2 * CountFace;
			StateNode* newStates[ NewStateNum ];
			for( int i = 0 ; i < NewStateNum ; ++i )
			{
				newStates[i] = new StateNode;
				mAllocNodes.push_back( newStates[i] );
			}
			generateNextNodes( node , newStates );
			{
				Mutex::Locker locker( mUncheckMutex );
				mUncheckNodes.insert( mUncheckNodes.end() , newStates , newStates + NewStateNum );
				mUncheckCond.notify();
			}
		}
	}
	
	void Solver::cleanup()
	{
		mRequestFindNodes.clear();
		mUncheckNodes.clear();
		for( int i = 0 ; i < mAllocNodes.size() ; ++i )
		{
			delete mAllocNodes[i];
		}
		mAllocNodes.clear();
		mCheckedStates.clear();
	}

	void Solver::generateNextNodes(StateNode* node , StateNode* nextNodes[])
	{
		for( int i = 0 ; i < CountFace ; ++i)
		{
			StateNode* n1 = nextNodes[i];
			n1->rotation = FaceDir(i);
			n1->bInverse = false;
			n1->parent = node;
			CubeOperator::Rotate( node->state , FaceDir(i) , n1->state );

			StateNode* n2 = nextNodes[ i + CountFace ];
			n2->rotation = FaceDir(i);
			n2->bInverse = true;
			n2->parent = node;
			CubeOperator::RotateInv( node->state , FaceDir(i) , n2->state );
		}
	}

}//namespace Rubiks