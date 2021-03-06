#include "ProfileSystem.h"

//#include "TMessageShow.h"

#include "Clock.h"
#include "PlatformThread.h"

#include <atomic>

HighResClock  gProfileClock;

inline void Profile_GetTicks(unsigned long int * ticks)
{
	*ticks = gProfileClock.getTimeMicroseconds();
}

inline float Profile_GetTickRate(void)
{
	//	return 1000000.f;
	return 1000.f;
}

class ProfileSystemImpl : public ProfileSystem
{
public:

	void  cleanup();
	void  resetSample();

	void  incrementFrameCount();
	int	  getFrameCountSinceReset() { return mFrameCounter; }
	float  getTimeSinceReset();

	ProfileSampleIterator getSampleIterator(uint32 ThreadId = 0);

#if 0
	void   dumpRecursive(ProfileSampleIterator* profileIterator, int spacing);
	void   dumpAll(uint32 ThreadId);
#endif

	static ThreadProfileData* GetThreadData();
private:
	friend class ProfileSystem;
	ProfileSystemImpl(char const* rootName = "Root");
	~ProfileSystemImpl();

	int				         mFrameCounter;
	unsigned long int        ResetTime;
};

#if CORE_SHARE_CODE

#if 0
Mutex gInstanceLock;
std::atomic< ProfileSystem* > gInstance;
ProfileSystem& IProfileSystem::Get()
{
	ProfileSystem* instance = gInstance.load(std::memory_order_acquire);

	if( instance == nullptr )
	{
		Mutex::Locker locker(gInstanceLock);
		instance = gInstance.load(std::memory_order_relaxed);
		if( instance == nullptr )
		{
			instance = new ProfileSystem("Root");
			gInstance.store( instance, std::memory_order_release);
		}
	}
	return *instance;
}
#else
ProfileSystem& ProfileSystem::Get()
{
	static ProfileSystemImpl sInstance;
	return sInstance;
}
#endif

ProfileSampleScope::ProfileSampleScope(const char * name, unsigned flag)
{
	ProfileSystemImpl::GetThreadData()->tryStartTiming(name, flag);
}

ProfileSampleScope::~ProfileSampleScope(void)
{
	ProfileSystemImpl::GetThreadData()->stopTiming();
}

#endif //CORE_SHARE_CODE


ProfileSystemImpl::ProfileSystemImpl(char const* rootName)
	:mFrameCounter(0)
	,ResetTime(0)
{


}

ProfileSystemImpl::~ProfileSystemImpl()
{
	cleanup();
}

void ProfileSystemImpl::cleanup()
{

}

void ProfileSystemImpl::resetSample()
{
	gProfileClock.reset();
	mFrameCounter = 0;
	Profile_GetTicks(&ResetTime);
	//ThreadData reset
}



void ProfileSystemImpl::incrementFrameCount()
{
	++mFrameCounter;
}


float ProfileSystemImpl::getTimeSinceReset()
{
	unsigned long int time;
	Profile_GetTicks(&time);
	time -= ResetTime;
	return (float)time / Profile_GetTickRate();
}

ProfileSampleIterator ProfileSystemImpl::getSampleIterator(uint32 ThreadId)
{
	return GetThreadData()->getSampleIterator();
}

std::unordered_map< uint32, ThreadProfileData* > ThreadDataMap;
thread_local ThreadProfileData* gThreadDataLocal = nullptr;

ThreadProfileData* ProfileSystemImpl::GetThreadData()
{
	if( gThreadDataLocal == nullptr )
	{
		gThreadDataLocal = new ThreadProfileData("Root");
		gThreadDataLocal->mThreadId = Thread::GetCurrentThreadId();
		//mThreadDataMap.insert({ gThreadDataLocal->mThreadId , gThreadDataLocal });
	}
	return gThreadDataLocal;
}

#if 0
#include <stdio.h>

void	ProfileSystemImpl::dumpRecursive(ProfileSampleIterator* profileIterator, int spacing)
{
	profileIterator->first();
	if( profileIterator->isDone() )
		return;

	float accumulated_time = 0, parent_time = profileIterator->isRoot() ? ProfileSystem::getTimeSinceReset() : profileIterator->getCurrentParentTotalTime();
	int i;
	int frames_since_reset = ProfileSystem::getFrameCountSinceReset();
	for( i = 0; i < spacing; i++ )	printf(".");
	printf("----------------------------------\n");
	for( i = 0; i < spacing; i++ )	printf(".");
	printf("Profiling: %s (total running time: %.3f ms) ---\n", profileIterator->getCurrentParentName(), parent_time);
	float totalTime = 0.f;


	int numChildren = 0;

	for( i = 0; !profileIterator->isDone(); i++, profileIterator->next() )
	{
		numChildren++;
		float current_total_time = profileIterator->getCurrentTotalTime();
		accumulated_time += current_total_time;
		float fraction = parent_time > CLOCK_EPSILON ? (current_total_time / parent_time) * 100 : 0.f;
		{
			int i;	for( i = 0; i < spacing; i++ )	printf(".");
		}
		printf("%d -- %s (%.2f %%) :: %.3f ms / frame (%d calls)\n", i, profileIterator->getCurrentName(), fraction, (current_total_time / (double)frames_since_reset), profileIterator->getCurrentTotalCalls());
		totalTime += current_total_time;
		//recurse into children
	}

	if( parent_time < accumulated_time )
	{
		printf("what's wrong\n");
	}
	for( i = 0; i < spacing; i++ )	printf(".");
	printf("%s (%.3f %%) :: %.3f ms\n", "Unaccounted:", parent_time > CLOCK_EPSILON ? ((parent_time - accumulated_time) / parent_time) * 100 : 0.f, parent_time - accumulated_time);

	for( i = 0; i < numChildren; i++ )
	{
		profileIterator->enterChild(i);
		dumpRecursive(profileIterator, spacing + 3);
		profileIterator->enterParent();
	}
}


void ProfileSystemImpl::dumpAll(uint32 ThreadId)
{
	//#FIXME
	ProfileSampleIterator profileIterator = getSampleIterator(ThreadId);
	dumpRecursive(&profileIterator, 0);
}
#endif


ProfileSampleNode::ProfileSampleNode( const char * name, ProfileSampleNode * parent )
	:mName( name )
	,TotalCalls( 0 )
	,TotalTime( 0 )
	,StartTime( 0 )
	,RecursionCounter( 0 )
	,mParent( parent )
	,mChild(nullptr)
	,mSibling(nullptr)
	,mIsShowChild( true )
{
	reset();
}

ProfileSampleNode::~ProfileSampleNode()
{

}

void ProfileSampleNode::unlink()
{
	mChild = nullptr;
	mSibling = nullptr;
}


ProfileSampleNode* ProfileSampleNode::getSubNode( const char * name )
{
	// Try to find this sub node
	ProfileSampleNode* child = mChild;
	while ( child ) 
	{
		if ( child->mName == name ) 
		{
			return child;
		}
		child = child->mSibling;
	}
	return NULL;

}


void ProfileSampleNode::reset( void )
{
	TotalCalls = 0;
	TotalTime = 0.0f;

	if ( mChild ) 
	{
		mChild->reset();
	}
	if ( mSibling ) 
	{
		mSibling->reset();
	}
}


void ProfileSampleNode::onCall( void )
{
	TotalCalls++;
	if (RecursionCounter++ == 0) 
	{
		Profile_GetTicks(&StartTime);
	}
}


bool ProfileSampleNode::onReturn( void )
{
	if ( --RecursionCounter == 0 && TotalCalls != 0 ) 
	{ 
		unsigned long int time;
		Profile_GetTicks(&time);
		time-=StartTime;
		TotalTime += (float)time / Profile_GetTickRate();
	}
	return ( RecursionCounter == 0 );
}

void ProfileSampleNode::showAllChild( bool beShow )
{
	ProfileSampleNode* node = getChild();

	showChild( beShow  );
	while ( node )
	{
		node->showAllChild( beShow );
		node = node->getSibling();
	}
}

ProfileSampleIterator::ProfileSampleIterator( ProfileSampleNode* start )
{
	parent = start;
	curNode = parent->getChild();
}


void ProfileSampleIterator::first()
{
	curNode = parent->getChild();
}

void ProfileSampleIterator::next()
{
	curNode = curNode->getSibling();
}

bool ProfileSampleIterator::isDone()
{
	return curNode == NULL;
}

void ProfileSampleIterator::enterChild( int index )
{
	curNode = parent->getChild();
	while ( (curNode != nullptr) && (index != 0) ) 
	{
		index--;
		curNode = curNode->getSibling();
	}

	if ( curNode != nullptr ) 
	{
		parent = curNode;
		curNode = parent->getChild();
	}
}

void ProfileSampleIterator::enterChild()
{
	ProfileSampleNode* nextChild = curNode->getChild();

	parent = curNode;
	curNode = nextChild;
}

void ProfileSampleIterator::enterParent()
{
	if ( parent->getParent() != NULL ) 
	{
		parent = parent->getParent();
	}
	curNode = parent->getChild();
}

void ProfileSampleIterator::before()
{
	ProfileSampleNode* node = parent->getChild();

	if ( node == curNode )
		return;

	while( node )
	{
		if ( node->getSibling() == curNode )
			break;
		node = node->getSibling();
	}
	curNode = node;
}

ThreadProfileData::ThreadProfileData(char const* rootName)
{
	mCurFlag = 0;

	mRootSample.reset(CreateNode(rootName, NULL));
	mRootSample->mPrevFlag = 0;
	mRootSample->mStackNum = 0;

	mCurSample = mRootSample.get();
}


void ThreadProfileData::resetSample()
{
	mRootSample->reset();
	mRootSample->onCall();
}

void ThreadProfileData::tryStartTiming(const char * name, unsigned flag)
{
	if( flag & PROF_FORCCE_ENABLE )
	{
		flag |= (mCurFlag & ~(PROF_DISABLE_THIS | PROF_DISABLE_CHILDREN));
		flag &= ~PROF_FORCCE_ENABLE;
	}
	else
	{
		flag |= mCurFlag;
	}

	if( flag & PROF_DISABLE_THIS )
	{
		mCurFlag = flag;
		return;
	}

	bool needCall = true;
	if( name != mCurSample->getName() )
	{
		if( flag & PROF_DISABLE_CHILDREN )
			flag |= PROF_DISABLE_THIS;

		ProfileSampleNode* node;
		if( flag & PROF_RECURSIVE_ENTRY )
		{
			node = mCurSample->getParent();
			while( node )
			{
				if( name = node->getName() )
					break;
				node = node->getParent();
			}

			if( node )
			{
				while( node != mCurSample )
				{
					stopTiming();
				}
			}
		}
		else
		{
			node = mCurSample->getSubNode(name);
		}


		if( node == nullptr )
		{
			node = CreateNode(name, mCurSample);
			mCurSample->addSubNode(node);
		}
		mCurSample = node;
	}

	mCurSample->onCall();
	mCurSample->mPrevFlag = mCurFlag;
	mCurFlag = flag;
}

void ThreadProfileData::stopTiming()
{
	if( mCurFlag & PROF_DISABLE_THIS )
		return;

	if( mCurSample->onReturn() )
	{
		mCurFlag = mCurSample->mPrevFlag;
		mCurSample = mCurSample->getParent();
	}
}

void ThreadProfileData::cleanup()
{
	if( mRootSample->mChild )
		cleanupNode(mRootSample->mChild);
	if( mRootSample->mSibling )
		cleanupNode(mRootSample->mSibling);

	mRootSample->mChild = nullptr;
	mRootSample->mSibling = nullptr;
	resetSample();

}
