#include "CmtPCH.h"
#include "CmtGame.h"

#include "CmtStage.h"

namespace Chromatron
{
	EXPORT_GAME_MODULE(GameModule)

	bool GameModule::getAttribValue( AttribValue& value )
	{
		switch( value.id )
		{
		case ATTR_SINGLE_SUPPORT:
			value.iVal = true;
			return true;
		case ATTR_AI_SUPPORT:
		case ATTR_NET_SUPPORT:
			value.iVal = false;
			return true;
		case ATTR_CONTROLLER_DEFUAULT_SETTING:
			return true;
		}
		return false;
	}

	void GameModule::beginPlay( StageModeType type, StageManager& manger )
	{
		IGameModule::beginPlay( type , manger );
	}

	StageBase* GameModule::createStage( unsigned id )
	{
		switch( id )
		{
		case STAGE_SINGLE_GAME: return new LevelStage;
		}
		return NULL;
	}


}//namespace Chromatron


