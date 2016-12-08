#include "TinyGamePCH.h"
#include "CFrameActionNetEngine.h"

#include "GameInstance.h"
#include "GamePlayer.h"

CFrameActionEngine::CFrameActionEngine( INetFrameManager* netFrameMgr )
{
	assert( netFrameMgr );
	mNetFrameMgr = netFrameMgr;
}

CFrameActionEngine::~CFrameActionEngine()
{
	mNetFrameMgr->release();
}

bool CFrameActionEngine::build( BuildParam& buildParam )
{
	mWorker   = buildParam.worker;
	mTickTime = buildParam.tickTime;

	buildParam.processor->addInput( *mNetFrameMgr );
	mNetFrameMgr->getActionProcessor().addInput( buildParam.game->getController() );

	return true;
}

void CFrameActionEngine::update( IFrameUpdater& updater , long time )
{
	if ( mWorker->getActionState() == NAS_LEVEL_RUN )
	{
		int updateFrames = time / mTickTime;
		int maxDelayFrames = mWorker->getNetLatency() / mTickTime ;
		int curFrame = mNetFrameMgr->evalFrame( updater , updateFrames , maxDelayFrames );
	}
	else
	{




	}
}

void CFrameActionEngine::setupInputAI( IPlayerManager& manager )
{
	for( IPlayerManager::Iterator iter = manager.getIterator();
		iter.haveMore() ; iter.goNext() )
	{
		GamePlayer* player = iter.getElement();
		if ( player->getAI() )
		{
			IActionInput* input = player->getAI()->getActionInput();
			if ( input )
				mNetFrameMgr->getActionProcessor().addInput( *input );
		}
	}
}