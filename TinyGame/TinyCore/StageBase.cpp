#include "TinyGamePCH.h"
#include "StageBase.h"


StageBase::StageBase() : mManager( NULL )
{
	
}

StageBase::~StageBase()
{

}

void StageBase::update( long time )
{
	TaskHandler::runTask( time );
	onUpdate( time );
}

void StageBase::render( float dFrame )
{
	onRender( dFrame );
}

bool StageBase::onChar( unsigned code )
{
	return true;
}

bool StageBase::onKey( unsigned key , bool isDown )
{
	return true;
}

bool StageBase::onMouse( MouseMsg const& msg )
{
	return true;
}

void StageBase::onTaskMessage( TaskBase* task , TaskMsg const& msg )
{


}

void StageManager::setNextStage( StageBase* chStage )
{
	if ( chStage == NULL )
		chStage = resolveChangeStageFail( FailReason::NoStage );

	if ( chStage )
	{
		if ( mNextStage )
			delete mNextStage;
		mNextStage = chStage;
		mNextStage->mManager = this;
	}
}


class EmptyStage : public StageBase 
{ 
public:
};

StageManager::StageManager() 
	:mCurStage(  new EmptyStage )
	,mNextStage( NULL )
{

}

StageManager::~StageManager()
{
	cleanup();
}

void StageManager::setupStage()
{
	prevStageChange();

	mCurStage->mManager = this;
	for(;;)
	{
		if( initializeStage(mCurStage) )
			break;

		mCurStage->onInitFail();

		StageBase* stage = resolveChangeStageFail( FailReason::InitFail );
		if ( stage == NULL )
		{
			stage = new EmptyStage;
		}

		if ( stage != mCurStage )
		{
			delete mCurStage;
			stage->mManager = this;
		}
		mCurStage = stage;
	}

	postStageChange( mCurStage );
}

StageBase* StageManager::changeStage( StageID stageId )
{
	StageBase* chStage = createStage( stageId );
	setNextStage( chStage );
	return chStage;
}


void StageManager::checkNewStage()
{
	if ( mNextStage )
	{
		mCurStage->onEnd();
		delete mCurStage;
		postStageEnd();

		mCurStage = mNextStage;
		mNextStage = NULL;
		setupStage();
	}
}

void StageManager::cleanup()
{
	TaskHandler::clearTask();


	if ( mCurStage )
	{
		mCurStage->onEnd();
		delete mCurStage;
		mCurStage = NULL;

		postStageEnd();
	}

	delete mNextStage;
	mNextStage = NULL;

}
