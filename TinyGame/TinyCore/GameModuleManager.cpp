#include "TinyGamePCH.h"
#include "GameModuleManager.h"
#include "GameControl.h"
#include "WindowsHeader.h"

#include "FileSystem.h"


bool GameModuleManager::registerModule( IModuleInterface* module , char const* loadedModuleName 
#if SYS_PLATFORM_WIN
									   , HMODULE hModule 
#endif
)
{
	assert(module);

	IGameModule* gameModule = nullptr;
	if( module->isGameModule() )
	{
		gameModule = static_cast<IGameModule*>(module);
	}

	char const* registerName = ( gameModule ) ? gameModule->getName() : loadedModuleName;
	if( findModule(registerName) )
		return false;

	if( !module->initialize() )
		return false;

	bool bInserted = mNameToModuleMap.insert(std::make_pair(registerName, module)).second;
	assert(bInserted);
	mModuleDataList.push_back(
		{   module
#if SYS_PLATFORM_WIN
          , hModule
#endif
		});

	if( gameModule )
	{
		AttribValue attrSetting(ATTR_CONTROLLER_DEFUAULT_SETTING);
		gameModule->getAttribValue(attrSetting);
	}

	return true;
}

void GameModuleManager::cleanup()
{
	if ( mGameRunning )
		mGameRunning->exit();
	mGameRunning = NULL;

	visitInternal( [](ModuleData& info) ->bool
	{
		info.instance->cleanup();
		info.instance->deleteThis();
		::FreeLibrary(info.hModule);
		return true;
	});

	mNameToModuleMap.clear();
	mModuleDataList.clear();
}

void GameModuleManager::classifyGame( int attrID , GameModuleVec& games )
{
	visitInternal( [ attrID , &games ](ModuleData& info)-> bool
	{
		AttribValue    attrValue(attrID);

		if( info.instance->isGameModule() )
		{
			IGameModule* gameModule = static_cast<IGameModule*>(info.instance);
			if( gameModule->getAttribValue(attrValue) )
			{
				if( attrValue.iVal )
				{
					games.push_back(gameModule);
				}
			}
		}
		return true;
	});
}

IModuleInterface* GameModuleManager::findModule( char const* name )
{
	ModuleMap::iterator iter = mNameToModuleMap.find( name );
	if ( iter != mNameToModuleMap.end() )
		return iter->second;
	return NULL;
}

IGameModule* GameModuleManager::changeGame( char const* name )
{
	IModuleInterface* module = findModule( name );

	if ( module && module->isGameModule() )
	{
		IGameModule* gameModule = static_cast<IGameModule*>(module);
		if( changeGame(gameModule) )
			return gameModule;
	}

	return nullptr;
}

bool GameModuleManager::changeGame(IGameModule* gameModule)
{
	if( !gameModule )
		return false;

	if( mGameRunning != gameModule )
	{
		if( mGameRunning )
			mGameRunning->exit();

		mGameRunning = gameModule;

		try
		{
			mGameRunning->enter();
		}
		catch( ... )
		{
			mGameRunning = nullptr;
			return false;
		}

	}
	return true;
}

GameModuleManager::GameModuleManager()
{
	mGameRunning = NULL;
}

GameModuleManager::~GameModuleManager()
{
	cleanup();
}


template< class T , class Policy >
class TScopeRelease
{
public:
	TScopeRelease(T& object)
		:mObject(&object)
	{

	}

	~TScopeRelease()
	{
		if( mObject )
		{
			Policy::Release(*mObject);
		}
	}

	void release()
	{
		mObject = nullptr;
	}
	T* mObject;
};



bool GameModuleManager::loadModule( char const* path )
{
#if SYS_PLATFORM_WIN
	HMODULE hModule = ::LoadLibrary( path );
	if ( hModule == NULL )
		return false;
	struct ModouleReleasePolicy
	{
		static void Release(HMODULE handle) { ::FreeLibrary(handle); }
	};

	TScopeRelease< HMODULE, ModouleReleasePolicy > scopeRelease( hModule );
	char const* funName = CREATE_MODULE_STR;
	CreateModuleFun createFun = (CreateModuleFun)GetProcAddress(hModule, CREATE_MODULE_STR);
#else

	CreateModuleFun createFun = nullptr;
#endif 
	if( !createFun )
		return false;

	IModuleInterface* module = (*createFun)();
	if( !module )
		return false;

	StringView loadName = FileUtility::CutDirAndExtension(path);
	if( !registerModule( module , loadName.toCString() 
#if SYS_PLATFORM_WIN
						, hModule 
#endif
	) )
	{
		module->deleteThis();
		return false;
	}

#if SYS_PLATFORM_WIN
	scopeRelease.release();
#endif

	return true;
}
