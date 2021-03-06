#ifndef RichGame_h__
#define RichGame_h__

#include "GameModule.h"

namespace Rich
{

	class GameModule : public IGameModule
	{
	public:
		virtual bool  initialize(){ return true; }
		virtual void  cleanup(){}
		virtual void  enter(){}
		virtual void  exit(){} 
		virtual void  deleteThis(){ delete this; }
		//
		virtual void beginPlay( StageModeType type, StageManager& manger );
	public:
		virtual char const*           getName(){ return "Rich"; }
		virtual GameController&       getController(){ return IGameModule::getController(); }
		virtual StageBase*            createStage( unsigned id );
		virtual SettingHepler*        createSettingHelper( SettingHelperType type ){ return nullptr; }

		//old replay version
		virtual ReplayTemplate*       createReplayTemplate( unsigned version ){ return nullptr; }
	};

}//namespace Rich

#endif // RichGame_h__
