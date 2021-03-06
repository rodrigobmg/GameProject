#include "Game.h"
#include "MenuStage.h"

#include <iostream>

int main()
{
	try
	{
		Game* game = new Game();
		if( !game->init("config.txt") )
			return -1;
		game->addStage( new MenuStage(), false );
		game->run();
		game->exit();

	}
	catch( std::exception& e )
	{
		std::cerr << e.what() << std::endl;
	}
	catch( ... )
	{
		

	}
	return 0;
}