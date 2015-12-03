#ifndef RES_ID
#	define ENUM_DEF__
#	define RES_ID( id ) id ,
#else
#	undef ResID_h__
#endif

#ifndef ResID_h__
#define ResID_h__

#ifdef ENUM_DEF__
namespace Zuma
{
	enum  ResID
	{
#endif

	RES_ID( IMAGE_LOADING_SCREEN )
	RES_ID( IMAGE_LOADERBAR )

	RES_ID( IMAGE_DIALOG_BUTTON )
	RES_ID( IMAGE_DIALOG_BACK )

	RES_ID( IMAGE_CURSOR_POINTER )
	RES_ID( IMAGE_CURSOR_HAND )
	RES_ID( IMAGE_CURSOR_DRAGGING )
	RES_ID( IMAGE_CURSOR_TEXT )

	RES_ID( IMAGE_SUNGLOW )

	RES_ID( IMAGE_MM_BACK )
	RES_ID( IMAGE_MM_GAUNTLET )
	RES_ID( IMAGE_MM_ARCADE )
	RES_ID( IMAGE_MM_MOREGAMES )
	RES_ID( IMAGE_MM_OPTIONS )
	RES_ID( IMAGE_MM_QUIT )
	RES_ID( IMAGE_MM_EYELEFT )
	RES_ID( IMAGE_MM_EYERIGHT )
	RES_ID( IMAGE_MM_EYEMASK )
	RES_ID( IMAGE_MM_SUN )
	RES_ID( IMAGE_MM_SKY )
	RES_ID( IMAGE_MM_HAT )

	RES_ID( IMAGE_ADVSKY )
	RES_ID( IMAGE_ADVBACK )
	RES_ID( IMAGE_ADVMIDDLE )
	RES_ID( IMAGE_ADVTEMPLE1 )
	RES_ID( IMAGE_ADVTEMPLE2 )
	RES_ID( IMAGE_ADVTEMPLE3 )
	RES_ID( IMAGE_ADVTEMPLE4 )
	RES_ID( IMAGE_ADVTEMPLE2V )
	RES_ID( IMAGE_ADVTEMPLE3V )
	RES_ID( IMAGE_ADVDOOR1A )
	RES_ID( IMAGE_ADVDOOR1B )
	RES_ID( IMAGE_ADVDOOR1C )
	RES_ID( IMAGE_ADVDOOR2A )
	RES_ID( IMAGE_ADVDOOR2B )
	RES_ID( IMAGE_ADVDOOR2C )
	RES_ID( IMAGE_ADVDOOR3A )
	RES_ID( IMAGE_ADVDOOR3B )
	RES_ID( IMAGE_ADVDOOR3C )
	RES_ID( IMAGE_ADVDOOR4A )
	RES_ID( IMAGE_ADVDOOR4B )
	RES_ID( IMAGE_ADVDOOR4C )
	RES_ID( IMAGE_ADVMAINMENUBUTTON )
	RES_ID( IMAGE_ADVPLAYBUTTON )
	RES_ID( IMAGE_ADVSTAGE )
	RES_ID( IMAGE_ADVTITLE )
	RES_ID( IMAGE_ADVHIGHSCORE )

	RES_ID( IMAGE_BLUE_BALL  )
	RES_ID( IMAGE_YELLOW_BALL)
	RES_ID( IMAGE_RED_BALL)
	RES_ID( IMAGE_GREEN_BALL)
	RES_ID( IMAGE_PURPLE_BALL)
	RES_ID( IMAGE_WHITE_BALL)
	RES_ID( IMAGE_NEXT_BALL)

	RES_ID( IMAGE_BALL_ALPHA)
	RES_ID( IMAGE_BALL_SHADOW)

	RES_ID( IMAGE_BLUE_ACCURACY )
	RES_ID( IMAGE_YELLOW_ACCURACY )
	RES_ID( IMAGE_RED_ACCURACY )
	RES_ID( IMAGE_GREEN_ACCURACY )
	RES_ID( IMAGE_PURPLE_ACCURACY )
	RES_ID( IMAGE_WHITE_ACCURACY )

	RES_ID( IMAGE_BLUE_BACKWARDS )
	RES_ID( IMAGE_YELLOW_BACKWARDS )
	RES_ID( IMAGE_RED_BACKWARDS )
	RES_ID( IMAGE_GREEN_BACKWARDS )
	RES_ID( IMAGE_PURPLE_BACKWARDS )
	RES_ID( IMAGE_WHITE_BACKWARDS )

	RES_ID( IMAGE_BLUE_SLOW )
	RES_ID( IMAGE_YELLOW_SLOW )
	RES_ID( IMAGE_RED_SLOW )
	RES_ID( IMAGE_GREEN_SLOW )
	RES_ID( IMAGE_PURPLE_SLOW )
	RES_ID( IMAGE_WHITE_SLOW )

	RES_ID( IMAGE_GRAY_EXPLOSION )
	RES_ID( IMAGE_EXPLOSION )

	RES_ID( IMAGE_BLUE_BOMB )
	RES_ID( IMAGE_YELLOW_BOMB )
	RES_ID( IMAGE_RED_BOMB )
	RES_ID( IMAGE_GREEN_BOMB )
	RES_ID( IMAGE_PURPLE_BOMB )
	RES_ID( IMAGE_WHITE_BOMB )

	RES_ID( IMAGE_BLUE_LIGHT )
	RES_ID( IMAGE_YELLOW_LIGHT )
	RES_ID( IMAGE_RED_LIGHT )
	RES_ID( IMAGE_GREEN_LIGHT )
	RES_ID( IMAGE_PURPLE_LIGHT )
	RES_ID( IMAGE_WHITE_LIGHT )

	RES_ID( IMAGE_BACKWARDS_LIGHT )
	RES_ID( IMAGE_ACCURACY_LIGHT )
	RES_ID( IMAGE_SLOW_LIGHT )

	RES_ID( IMAGE_SPARKLE )

	RES_ID( IMAGE_SHOOTER_BOTTOM )
	RES_ID( IMAGE_SHOOTER_TOP )
	RES_ID( IMAGE_SHOOTER_TONGUE )
	RES_ID( IMAGE_EYE_BLINK )

	RES_ID( IMAGE_DIALOG_CHECKBOX )
	RES_ID( IMAGE_DIALOG_CHECKBOXCAP )
	RES_ID( IMAGE_DIALOG_CHECKBOXLINE )
	RES_ID( IMAGE_SLIDER_TRACK )
	RES_ID( IMAGE_SLIDER_THUMB )
	RES_ID( IMAGE_UPDATE_BAR )
	RES_ID( IMAGE_GOD_HEAD )

	RES_ID( IMAGE_HOLE )
	RES_ID( IMAGE_HOLE_COVER )
	RES_ID( IMAGE_COIN )
	RES_ID( IMAGE_RIGHT_MOUSE )

	RES_ID( IMAGE_MENU_BAR )
	RES_ID( IMAGE_MENU_BUTTON )
	RES_ID( IMAGE_ZUMA_BAR )
	RES_ID( IMAGE_ZUMA_BAR_DONE )
	RES_ID( IMAGE_FROG_LIVES )


	RES_ID( SOUND_BALLCLICK1 )
	RES_ID( SOUND_BALLCLICK2 )
	RES_ID( SOUND_BALLFIRE )
	RES_ID( SOUND_WARNING )
	RES_ID( SOUND_EXPLODE )
	RES_ID( SOUND_EARTHQUAKE )
	RES_ID( SOUND_FROGLAND)
	RES_ID( SOUND_JEWEL_APPEAR )
	RES_ID( SOUND_JEWEL_DISAPPEAR )
	RES_ID( SOUND_JEWEL_HIT )
	RES_ID( SOUND_CHAIN_BONUS )
	RES_ID( SOUND_GAP_BONUS )
	RES_ID( SOUND_ROLLING )
	RES_ID( SOUND_EXTRA_LIFE )

	RES_ID( SOUND_SLOWDOWN_BALL )
	RES_ID( SOUND_BACKWARDS_BALL )
	RES_ID( SOUND_ACCURACY_BALL )
	RES_ID( SOUND_TRAIL_LIGHT )
	RES_ID( SOUND_TRAIL_LIGHT_END )
	RES_ID( SOUND_GAME_START )
	RES_ID( SOUND_LEVEL_UP )
	RES_ID( SOUND_BAR_FULL )
	RES_ID( SOUND_LOST_LIFE )
	RES_ID( SOUND_GAME_OVER )
	RES_ID( SOUND_STAGE_COMPLETE )
	RES_ID( SOUND_TEMPLE_COMPLETE )
	RES_ID( SOUND_BONUS_EXPLOSION )

	RES_ID( SOUND_BALLDESTROYED1 )
	RES_ID( SOUND_BALLDESTROYED2 )
	RES_ID( SOUND_BALLDESTROYED3 )
	RES_ID( SOUND_BALLDESTROYED4 )
	RES_ID( SOUND_BALLDESTROYED5 )
	RES_ID( SOUND_COMBO )

	RES_ID( SOUND_BUTTON1 )
	RES_ID( SOUND_BUTTON2 )
	RES_ID( SOUND_CHORAL1 )
	RES_ID( SOUND_POP )
	RES_ID( SOUND_UFO )

	RES_ID( FONT_HUGE )
	RES_ID( FONT_BROWNTITLE )
	RES_ID( FONT_FLOAT )
	RES_ID( FONT_PLAIN )
	RES_ID( FONT_MAIN10 )
	RES_ID( FONT_TITLE )

#ifdef ENUM_DEF__
		RES_NUM_RESOURCES ,
	};

}//namespace Zuma
#endif

#endif // ResID_h__

#undef ENUM_DEF__
#undef RES_ID



