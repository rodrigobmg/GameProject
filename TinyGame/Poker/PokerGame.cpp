#include "TinyGamePCH.h"
#include "PokerGame.h"

#include "StageBase.h"
#include "GameServer.h"
#include "GameSettingHelper.h"
#include "Widget/GameRoomUI.h"

#include "Big2Stage.h"
#include "HoldemStage.h"
#include "FreeCellStage.h"

#include "GameSettingPanel.h"
#include "Widget/GameRoomUI.h"
#include "GameWidgetID.h"
#include "DataSteamBuffer.h"

namespace Poker
{
	EXPORT_GAME_MODULE(GameModule)

	GameModule::GameModule()
	{
		mRule = RULE_BIG2;
	}

	GameModule::~GameModule()
	{

	}

	StageBase* GameModule::createStage( unsigned id )
	{
		switch( mRule )
		{
		case RULE_FREECELL:
			if ( id == STAGE_SINGLE_GAME )
			{
				return new FreeCellStage;
			}
			break;
		case RULE_BIG2:
			if( id == STAGE_NET_GAME || id == STAGE_SINGLE_GAME )
			{
				return new Big2::LevelStage;
			}
			break;
		case RULE_HOLDEM:
			if( id == STAGE_NET_GAME || id == STAGE_SINGLE_GAME )
			{
				return new Holdem::LevelStage;
			}
			break;
		}
		return NULL;
	}

	bool GameModule::getAttribValue( AttribValue& value )
	{
		switch( value.id )
		{
		case ATTR_NET_SUPPORT:
			value.iVal = true;
			return true;
		case ATTR_SINGLE_SUPPORT:
			value.iVal = true;
			break;
		}
		return false;
	}

	void GameModule::beginPlay( StageModeType type, StageManager& manger )
	{
		IGameModule::beginPlay( type , manger );
	}

	class CNetRoomSettingHelper : public NetRoomSettingHelper
	{
	public:
		CNetRoomSettingHelper( GameModule* game )
			:mGame( game ){}

		enum
		{
			UI_RULE_CHOICE = UI_GAME_ID ,

		};
		enum
		{
			MASK_BASE = BIT(1) ,
			MASK_RULE = BIT(2) ,
		};
		virtual bool checkSettingSV()
		{
			SVPlayerManager* playerMgr = static_cast< SVPlayerManager* >( getPlayerListPanel()->getPlayerManager() );
			switch( mGame->getRule() )
			{
			case RULE_BIG2:
				if ( playerMgr->getPlayerNum() < 4 )
				{
					int num = 4 - playerMgr->getPlayerNum();
					for( int i = 0 ; i < num ; ++i )
						playerMgr->createAIPlayer();
				}
				break;
			}
			return true;
		}
		virtual void clearUserUI()
		{
			getSettingPanel()->removeChildWithMask( MASK_BASE | MASK_RULE );
		}
		virtual void doSetupSetting( bool beServer )
		{
			setupBaseUI();

			switch( mGame->getRule() )
			{
			case RULE_BIG2:
				setupUI_Big2();
				break;
			case RULE_HOLDEM:
				setupUI_Holdem();
				break;
			}
		}

		void setupBaseUI()
		{
			GChoice* choice = mSettingPanel->addChoice( UI_RULE_CHOICE , LOCTEXT("Game Rule") , MASK_BASE );
			choice->addItem( LOCTEXT("Big2") );
			choice->addItem( LOCTEXT("Holdem") );
			switch( mGame->getRule() )
			{
			case RULE_BIG2:   
				choice->setSelection(0); 
				break;
			case RULE_HOLDEM: 
				choice->setSelection(1); 
				break;
			}
		}

		bool onWidgetEvent( int event ,int id , GWidget* widget )
		{
			switch( id )
			{
			case UI_RULE_CHOICE:
				getSettingPanel()->removeChildWithMask( MASK_RULE );
				getSettingPanel()->adjustChildLayout();
				switch( GUI::CastFast< GChoice >( widget )->getSelection() )
				{
				case 0:
					mGame->setRule( RULE_BIG2 );
					setupUI_Big2();
					break;
				case 1:
					mGame->setRule( RULE_HOLDEM );
					setupUI_Holdem();
				}
				modifyCallback( getSettingPanel() );
				return false;
			}

			return true;
		}
		void setupUI_Big2()
		{
			setMaxPlayerNum( 4 );
		}

		void setupUI_Holdem()
		{
			setMaxPlayerNum( Holdem::MaxPlayerNum );
		}

		virtual void setupGame( StageManager& manager , StageBase* subStage )
		{

		}
		virtual void doExportSetting( DataSteamBuffer& buffer )
		{
			int rule = mGame->getRule();
			buffer.fill( rule );

		}
		virtual void doImportSetting( DataSteamBuffer& buffer )
		{
			getSettingPanel()->removeChildWithMask( MASK_BASE | MASK_RULE );
			getSettingPanel()->adjustChildLayout();

			int rule;
			buffer.take( rule );
			mGame->setRule( GameRule( rule) );
			setupBaseUI();
		}
		GameModule* mGame;
	};

	SettingHepler* GameModule::createSettingHelper( SettingHelperType type )
	{
		switch( type )
		{
		case SHT_NET_ROOM_HELPER:
			return new CNetRoomSettingHelper( this );
		}
		return NULL;
	}

}//namespace Poker
