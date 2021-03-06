#pragma once
#ifndef ShaderCompiler_H_7817AD1B_28BB_41DC_B037_0D75389E42A2
#define ShaderCompiler_H_7817AD1B_28BB_41DC_B037_0D75389E42A2

#include "OpenGLCommon.h"
#include "ShaderCore.h"
#include "GlobalShader.h"
#include "Singleton.h"
#include "Asset.h"

#include "FixString.h"

#include <unordered_map>

#define SHADER_FILE_SUBNAME ".sgc"

namespace Render
{
	class MaterialShaderProgramClass;
	class MaterialShaderProgram;
	class VertexFactoryType;


	enum ShaderFreature
	{



	};

	class ShaderCompiler
	{
	public:
		bool compileCode( Shader::Type type , RHIShader& shader , char const* path, char const* def = nullptr );

		bool bRecompile = true;
		bool bUsePreprocess = true;

	};

	struct ShaderEntryInfo
	{
		Shader::Type type;
		char const*  name;
	};

	enum class ShaderClassType
	{
		Common,
		Global,
		Material,
	};

	typedef std::vector< std::pair< MaterialShaderProgramClass*, MaterialShaderProgram* > > MaterialShaderPairVec;

	class ShaderManager : public SingletonT< ShaderManager >
	{
	public:
		ShaderManager();
		~ShaderManager();


		static CORE_API ShaderManager& Get();

		void setBaseDir(char const* dir){  mBaseDir = dir;  }
		void clearnupRHIResouse();


		template< class ShaderType >
		ShaderType* getGlobalShaderT(bool bForceLoad = true)
		{
			return static_cast<ShaderType*>( getGlobalShader(ShaderType::GetShaderClass() , bForceLoad) );
		}

		template< class ShaderType >
		ShaderType* loadGlobalShaderT( ShaderCompileOption& option)
		{
			return static_cast<ShaderType*>(constructShaderInternal(ShaderType::GetShaderClass(), ShaderClassType::Common , option ));
		}

		int loadMaterialShaders(MaterialShaderCompileInfo const& info, VertexFactoryType& vertexFactoryType , MaterialShaderPairVec& outShaders );

		GlobalShaderProgram* getGlobalShader(GlobalShaderProgramClass& shaderClass , bool bForceLoad );

		bool registerGlobalShader(GlobalShaderProgramClass& shaderClass);

		int  loadAllGlobalShaders();

		GlobalShaderProgram* constructGlobalShader(GlobalShaderProgramClass& shaderClass);
		GlobalShaderProgram* constructShaderInternal(GlobalShaderProgramClass& shaderClass, ShaderClassType classType, ShaderCompileOption& option );

		void cleanupGlobalShader();

		bool loadFileSimple(ShaderProgram& shaderProgram, char const* fileName, char const* def = nullptr, char const* additionalCode = nullptr);

		bool loadFile(ShaderProgram& shaderProgram, char const* fileName,ShaderEntryInfo const entries[],
					  char const* def = nullptr, char const* additionalCode = nullptr);

		bool loadFile(ShaderProgram& shaderProgram, char const* fileName, 
					  char const* vertexEntryName, char const* pixelEntryName, 
					  char const* def = nullptr, char const* additionalCode = nullptr );

		bool loadFile(ShaderProgram& shaderProgram, char const* fileName, 
					  uint8 shaderMask, char const* entryNames[], 
					  char const* def = nullptr, char const* additionalCode = nullptr);

		bool loadFile(ShaderProgram& shaderProgram, char const* fileName, 
					  char const* vertexEntryName, char const* pixelEntryName, 
					  ShaderCompileOption const& option, char const* additionalCode = nullptr);


		bool loadFile(ShaderProgram& shaderProgram, char const* fileName, ShaderEntryInfo const entries[],
					  ShaderCompileOption const& option, char const* additionalCode = nullptr);

		bool loadFile(ShaderProgram& shaderProgram, char const* fileName,
					  uint8 shaderMask, char const* entryNames[],
					  ShaderCompileOption const& option, char const* additionalCode = nullptr);


		bool loadMultiFile(ShaderProgram& shaderProgram, char const* fileName, char const* def = nullptr, char const* additionalCode = nullptr);

		bool loadSimple(ShaderProgram& shaderProgram, char const* fileNameVS, char const* fileNamePS, char const* def = nullptr, char const* additionalCode = nullptr);

		bool reloadShader(ShaderProgram& shaderProgram);

		void reloadAll();

		void registerShaderAssets(AssetManager& assetManager)
		{
			for( auto pair : mShaderCompileMap )
			{
				assetManager.registerViewer(pair.second);
			}
		}

		void unregisterShaderAssets(AssetManager& assetManager)
		{
			for( auto pair : mShaderCompileMap )
			{
				assetManager.unregisterViewer(pair.second);
			}
		}

		bool loadInternal(ShaderProgram& shaderProgram, char const* fileName, uint8 shaderMask, char const* entryNames[], char const* def, char const* additionalCode, bool bSingleFile);
		bool loadInternal(ShaderProgram& shaderProgram, char const* fileName, ShaderEntryInfo const entries[], ShaderCompileOption const& option, char const* additionalCode, bool bSingleFile);
		bool loadInternal(ShaderProgram& shaderProgram, char const* fileName, uint8 shaderMask, char const* entryNames[], ShaderCompileOption const& option, char const* additionalCode, bool bSingleFile);
		bool loadInternal(ShaderProgram& shaderProgram, char const* filePaths[], ShaderEntryInfo const entries[], char const* def, char const* additionalCode);

		static void MakeEntryInfos(ShaderEntryInfo entries[], uint8 shaderMask, char const* entryNames[])
		{
			int indexUsed = 0;
			for( int i = 0; i < Shader::NUM_SHADER_TYPE; ++i )
			{
				if( (shaderMask & BIT(i)) == 0 )
					continue;
				entries[indexUsed].type = Shader::Type(i);
				entries[indexUsed].name = (entryNames) ? entryNames[indexUsed] : nullptr;
				++indexUsed;
			}
			entries[indexUsed].type = Shader::eEmpty;
			entries[indexUsed].name = nullptr;
		}

		struct ShaderCache
		{
			RHIShaderRef shader;
			std::string filePath;
			std::string define;
		};

		struct ShaderProgramCompileInfo;
		bool updateShaderInternal(ShaderProgram& shaderProgram, ShaderProgramCompileInfo& info);

		struct ShaderCompileInfo
		{
			std::string  filePath;
			Shader::Type type;
			std::string  headCode;

			template< class S1 , class S2 >
			ShaderCompileInfo(Shader::Type inType, S1&& inFilePath ,  S2&& inCode)
				:filePath( std::forward<S2>(inFilePath) ) , type(inType) , headCode( std::forward<S2>(inCode) )
			{}

			ShaderCompileInfo(){}
		};

		struct ShaderProgramCompileInfo : public IAssetViewer
		{
			ShaderProgram* shaderProgram;
			std::vector< ShaderCompileInfo > shaders;
			bool           bShowComplieInfo = false;
			
			ShaderClassType classType = ShaderClassType::Common;
		protected:
			virtual void getDependentFilePaths(std::vector<std::wstring>& paths) override;
			virtual void postFileModify(FileAction action) override;
		};

		void removeFromShaderCompileMap( ShaderProgram& shader )
		{
			auto iter = mShaderCompileMap.find(&shader);

			if( iter != mShaderCompileMap.end() )
			{
				delete iter->second;
				mShaderCompileMap.erase(iter);
			}
		}

		void  generateCompileSetup( 
			ShaderProgramCompileInfo& compileInfo , ShaderEntryInfo const entries[], 
			ShaderCompileOption const& option, char const* additionalCode ,
			char const* fileName , bool bSingleFile );

		uint32         mDefaultVersion;
		ShaderCompiler mCompiler;
		std::string    mBaseDir;

		std::unordered_map< ShaderProgram*, ShaderProgramCompileInfo* > mShaderCompileMap;
		std::unordered_map< GlobalShaderProgramClass*, GlobalShaderProgram* > mGlobalShaderMap;

	};

}


#endif // ShaderCompiler_H_7817AD1B_28BB_41DC_B037_0D75389E42A2