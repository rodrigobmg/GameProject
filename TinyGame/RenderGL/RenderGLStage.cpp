#include "TinyGamePCH.h"
#include "RenderGLStage.h"
#include "RenderGLStageAssetID.h"

#include "RHI/GpuProfiler.h"
#include "RHI/MeshUtility.h"
#include "RHI/DrawUtility.h"
#include "RHI/RHICommand.h"
#include "RHI/ShaderCompiler.h"

#include "GameGUISystem.h"
#include "GLGraphics2D.h"
#include "FileSystem.h"
#include "Widget/WidgetUtility.h"
#include "PlatformThread.h"

#include "GL/wglew.h"
#include <inttypes.h>

/*
#TODO:
#Material
-Translucent Impl
-Tangent Y sign Fix

#Shader
-World Position reconstrcut
-ForwardShading With Material
-Shadow PCF
-PostProcess flow Register

-Merge with CFly Library !!
-SceneManager
-Game Client Type : Mesh -> Object Texture
-Use Script setup Scene and reload
-RHI Resource Manage
-Game Resource Manage ( Material Object )
*/




namespace Render
{
	RHITexture2D* CreateHeightMapRawFile(char const* path , Texture::Format format , Vec2i const& size )
	{
		std::vector< char > buffer;
		if( !FileUtility::LoadToBuffer(path, buffer) )
			return nullptr;

		uint32 byteNum = Texture::GetFormatSize(format) * size.x * size.y;
		if( byteNum != buffer.size() )
			return nullptr;

		RHITexture2D* texture = RHICreateTexture2D(format, size.x, size.y, 1, 0 , &buffer[0], 1);
		if( texture == nullptr )
			return nullptr;

		{
			GL_BIND_LOCK_OBJECT(*texture);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		}

		return texture;
	}

	IAssetProvider* SceneAsset::sAssetProvider = nullptr;

	SampleStage::SampleStage()
	{
		mPause = false;
		mbShowBuffer = true;

		SceneAsset::sAssetProvider = this;
	}

	SampleStage::~SampleStage()
	{

	}

	float dbg;

	bool SampleStage::onInit()
	{
		testCode();

		//::Global::GetDrawEngine()->changeScreenSize(1600, 800);
		::Global::GetDrawEngine()->changeScreenSize(1280, 720);

		if ( !Global::GetDrawEngine()->startOpenGL() )
			return false;
		wglSwapIntervalEXT(0);

		Vec2i screenSize = ::Global::GetDrawEngine()->getScreenSize();

		if( !InitGlobalRHIResource() )
			return false;

		if( !ShaderHelper::Get().init() )
			return false;


		LazyObjectManager::Get().registerDefalutObject< StaticMesh >(&mEmptyMesh);
		LazyObjectManager::Get().registerDefalutObject< Material >(GDefalutMaterial);

		if( !loadAssetResouse() )
			return false;

		VERIFY_RETURN_FALSE(ShaderManager::Get().loadFileSimple(mProgSphere, "Shader/Game/Sphere"));
		VERIFY_RETURN_FALSE(ShaderManager::Get().loadFileSimple(mEffectSphereSM, "Shader/Game/Sphere", "#define USE_SHADOW_MAP 1"));
		VERIFY_RETURN_FALSE(ShaderManager::Get().loadMultiFile(mProgBump, "Shader/Game/Bump"));
		VERIFY_RETURN_FALSE(ShaderManager::Get().loadFileSimple(mProgParallax, "Shader/Game/Parallax"));
		VERIFY_RETURN_FALSE(ShaderManager::Get().loadFileSimple(mEffectSimple, "Shader/Game/Simple"));
		VERIFY_RETURN_FALSE(ShaderManager::Get().loadFileSimple(mProgPlanet, "Shader/Game/PlanetSurface"));

		VERIFY_RETURN_FALSE(mProgShadowVolume = ShaderManager::Get().getGlobalShaderT< ShadowVolumeProgram >());

		VERIFY_RETURN_FALSE(mSceneRenderTargets.init(screenSize));
		VERIFY_RETURN_FALSE(mTechShadow.init());
		VERIFY_RETURN_FALSE(mTechDeferredShading.init(mSceneRenderTargets));
		VERIFY_RETURN_FALSE(mTechOIT.init(screenSize));
		VERIFY_RETURN_FALSE(mTechVolumetricLighing.init(screenSize));
		VERIFY_RETURN_FALSE(mSSAO.init(screenSize));
		VERIFY_RETURN_FALSE(mDOF.init(screenSize));

		ShaderEntryInfo const entryNames[] = 
		{ 
			Shader::eVertex , SHADER_ENTRY(MainVS) ,
			Shader::ePixel  , SHADER_ENTRY(MainPS) ,
			Shader::eGeometry , SHADER_ENTRY(MainGS) ,
			Shader::eEmpty , nullptr
		};
		VERIFY_RETURN_FALSE(ShaderManager::Get().loadFile(mProgSimpleSprite, "Shader/Game/SimpleSprite", entryNames));

		if( !mLayerFrameBuffer.create() )
			return false;

		mLayerFrameBuffer.addTextureLayer(*mTechShadow.mShadowMap);

		{
			mSpritePosMesh.mInputLayoutDesc.addElement(Vertex::ATTRIBUTE0, Vertex::eFloat3);
			mSpritePosMesh.mInputLayoutDesc.addElement(Vertex::ATTRIBUTE1, Vertex::eFloat3);
			mSpritePosMesh.mInputLayoutDesc.addElement(Vertex::ATTRIBUTE2, Vertex::eFloat3);
			mSpritePosMesh.mType = PrimitiveType::Points;

			int numVertex = 100000;
			std::vector< Vector3 > vtx;

			int numElemant = mSpritePosMesh.mInputLayoutDesc.getVertexSize() / sizeof(Vector3);
			vtx.resize(numVertex * numElemant);

			Vector3* v = &vtx[0];
			float boxSize = 100;
			for( int i = 0; i < numVertex; ++i )
			{
				*v = RandVector(boxSize * Vector3(-1, -1, -1), boxSize *  Vector3(1,1,1));
				*(v + 1) = 10 * RandVector();
				*(v + 2) = 10 * RandVector();
				v += numElemant;
			}
			if( !mSpritePosMesh.createRHIResource(&vtx[0], numVertex) )
				return false;
		}

		VERIFY_RETURN_FALSE(MeshBuild::PlaneZ(mSimpleMeshs[SimpleMeshId::Plane], 10, 1));
		VERIFY_RETURN_FALSE(MeshBuild::Tile(mSimpleMeshs[SimpleMeshId::Tile], 64, 1.0f));
		VERIFY_RETURN_FALSE(MeshBuild::UVSphere(mSimpleMeshs[SimpleMeshId::Sphere], 2.5, 60, 60));
		//VERIFY_RETURN_FALSE(MeshBuild::UVSphere(mSimpleMeshs[ SimpleMeshId::Sphere ], 2.5, 4, 4) );
		VERIFY_RETURN_FALSE(MeshBuild::IcoSphere(mSimpleMeshs[SimpleMeshId::Sphere2], 2.5, 4));
		VERIFY_RETURN_FALSE(MeshBuild::Cube(mSimpleMeshs[SimpleMeshId::Box]));
		VERIFY_RETURN_FALSE(MeshBuild::SkyBox(mSimpleMeshs[SimpleMeshId::SkyBox]));
		VERIFY_RETURN_FALSE(MeshBuild::Doughnut(mSimpleMeshs[SimpleMeshId::Doughnut], 2, 1, 60, 60));
		VERIFY_RETURN_FALSE(MeshBuild::SimpleSkin(mSimpleMeshs[SimpleMeshId::SimpleSkin], 5, 2.5, 20, 20));

		VERIFY_RETURN_FALSE(mSimpleMeshs[SimpleMeshId::Sphere].generateVertexAdjacency());

		int const TerrainTileNum = 1024;
		if( !MeshBuild::Tile(mSimpleMeshs[SimpleMeshId::Terrain], 1024, 1.0, false) )
			return false;

		mTexTerrainHeight = CreateHeightMapRawFile("Terrain/heightmap.r16", Texture::eR16, Vec2i(TerrainTileNum + 1, TerrainTileNum + 1));
		if( !mTexTerrainHeight.isValid() )
			return false;

		ShaderCompileOption option;
		option.version = 430;
		if( !ShaderManager::Get().loadFile(
			mProgTerrain, "Shader/Terrain",
			SHADER_ENTRY(MainVS), SHADER_ENTRY(MainPS),
			option, nullptr) )
			return false;

		Vector3 v[] =
		{
			Vector3(1,1,0),
			Vector3(-1,1,0) ,
			Vector3(-1,-1,0),
			Vector3(1,-1,0),
		};
		int   idx[6] = { 0 , 1 , 2 , 0 , 2 , 3 };
		mSimpleMeshs[SimpleMeshId::SpherePlane].mInputLayoutDesc.addElement(Vertex::ePosition, Vertex::eFloat3);
		if( !mSimpleMeshs[SimpleMeshId::SpherePlane].createRHIResource(&v[0], 4, &idx[0], 6, true) )
			return false;



#define TEX_DIR "Texture/LancellottiChapel/"
#define TEX_SUB_NAME ".jpg"
		char const* paths[] =
		{
			TEX_DIR "posx" TEX_SUB_NAME,
			TEX_DIR "negx" TEX_SUB_NAME,
			TEX_DIR "posy" TEX_SUB_NAME,
			TEX_DIR "negy" TEX_SUB_NAME,
			TEX_DIR "posz" TEX_SUB_NAME,
			TEX_DIR "negz" TEX_SUB_NAME,
		};
#undef  TEX_DIR
#undef  TEX_SUB_NAME

		mTexSky = RHICreateTextureCube();
		if ( !mTexSky->loadFile( paths ) )
			return false;

		OpenGLCast::To(mTexSky)->bind();
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		OpenGLCast::To(mTexSky)->unbind();

		//////////////////////////////////////

		mViewFrustum.mNear = 0.01;
		mViewFrustum.mFar  = 2000.0;
		mViewFrustum.mAspect = float(screenSize.x) / screenSize.y;
		mViewFrustum.mYFov   = Math::Deg2Rad( 60 / mViewFrustum.mAspect );

		if ( !createFrustum( mFrustumMesh , mViewFrustum.getPerspectiveMatrix() ) )
			return false;

		mCameraMove.target = &mCamStorage[0];
		mCameraMove.target->setPos( Vector3( -10 , 0 , 10 ) );
		mCameraMove.target->setViewDir(Vector3(0, 0, -1), Vector3(1, 0, 0));

		mAabb[0].min = Vector3(1, 1, 1);
		mAabb[0].max = Vector3(5, 4, 7);
		mAabb[1].min = Vector3(-5, -5, -5);
		mAabb[1].max = Vector3(3, 5, 2);
		mAabb[2].min = Vector3(2, -5, -5);
		mAabb[2].max = Vector3(5, -3, 15);

		mTime = 0;
		mRealTime = 0;
		mIdxTestChioce = 0;

		setupScene();

		glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

		RHISetRasterizerState(TStaticRasterizerState<>::GetRHI());
		RHISetDepthStencilState(TStaticDepthStencilState<>::GetRHI());

	
		::Global::GUI().cleanupWidget();

		using namespace std::placeholders;
#define ADD_TEST( NAME , FUN )\
	mSampleTests.push_back({NAME,std::bind(&SampleStage::FUN,this,_1)})

		ADD_TEST("Sphere Plane", render_SpherePlane);
		ADD_TEST("Parallax Mapping", render_ParallaxMapping);
		ADD_TEST("PointLight shadow", renderTest2);
		ADD_TEST("Ray Test", render_RaycastTest);
		ADD_TEST("Deferred Lighting", render_DeferredLighting);
		ADD_TEST("Spite Test", render_Sprite);
		ADD_TEST("OIT Test", render_OIT);
		ADD_TEST("Terrain Test", render_Terrain);
		ADD_TEST("Shadow Volume", render_ShadowVolume);
#undef ADD_TEST

		auto devFrame = WidgetUtility::CreateDevFrame(Vec2i(150, 400));
		for ( int i = 0 ; i < (int)mSampleTests.size() ; ++i )
		{
			auto& test = mSampleTests[i];
			devFrame->addButton(UI_SAMPLE_TEST, test.name)->setUserData(i);
		}

		bInitialized = true;
		restart();
		return true;
	}

	void SampleStage::onInitFail()
	{
		if( mLoadingThread )
			mLoadingThread->kill();
		ShaderManager::Get().clearnupRHIResouse();
		Global::GetDrawEngine()->stopOpenGL(true);
	}

	void SampleStage::onEnd()
	{
		unregisterAllAsset();

		if( mLoadingThread )
			mLoadingThread->kill();
		ShaderManager::Get().clearnupRHIResouse();

		Global::GetDrawEngine()->stopOpenGL(true);
	}

	void SampleStage::testCode()
	{

		VectorCurveTrack track;
		track.mode = TrackMode::Oscillation;
		track.addKey(0, Vector3(0, 0, 0));
		track.addKey(20, Vector3(10, 20, 30));
		track.addKey(10, Vector3(5, 10, 15));
		
		Vector3 a = track.getValue(5);
		Vector3 b = track.getValue(-5);
		Vector3 c = track.getValue(25);
		Vector3 d = track.getValue(35);
		Vector3 e = track.getValue(40);
		Vector3 f = track.getValue(-20);

		{
			Vector3 n(1, 1, 0);
			float factor = n.normalize();
			ReflectMatrix m(n, 1.0 * factor);
			Vector3 v = Vector3(1, 0, 0) * m;
			int i = 1;
		}

	}

	void SampleStage::tick()
	{

		float deltaTime = float(gDefaultTickTime) / 10000;

		getScene(0).tick(deltaTime);

		mCameraMove.update(deltaTime);

		mRealTime += deltaTime;
		if ( mPause )
			return;

		mTime += gDefaultTickTime / 1000.0f;

		mPos.x = 10 * cos( mTime );
		mPos.y = 10 * sin( mTime );
		mPos.z = 0;

		mLights[0].dir.x = sin(mTime);
		mLights[0].dir.y = cos(mTime);
		mLights[0].dir.z = 0;
		mLights[0].pos = mTracks[1].getValue(mTime);
		mLights[0].dir = mTracks[0].getValue(mTime);
		for( int i = 1; i < 4; ++i )
		{
			mLights[i].pos = mTracks[i].getValue(mTime);
		}
		
		for( auto& modifier : mValueModifiers )
		{
			modifier->update(mTime);
		}

	}

	void SampleStage::unregisterAllAsset()
	{
		AssetManager& manager = ::Global::GetAssetManager();
		for( auto& asset : mMaterialAssets )
		{
			manager.unregisterViewer(&asset);
		}
		for( auto& asset : mSceneAssets )
		{
			manager.unregisterViewer(asset.get());
		}
		ShaderManager::Get().unregisterShaderAssets(manager);
	}

	void SampleStage::onRender(float dFrame)
	{
		if( !bInitialized )
			return;

		if( !mGpuSync.pervRender() )
			return;

		GameWindow& window = Global::GetDrawEngine()->getWindow();

		ViewInfo view;
		assert(mCameraMove.target);
		view.gameTime = mTime;
		view.realTime = mRealTime;
		view.rectOffset = IntVector2(0, 0);
		view.rectSize = IntVector2(window.getWidth(), window.getHeight());
		
		view.setupTransform(mCameraMove.target->getViewMatrix() , mViewFrustum.getPerspectiveMatrix() );

		{
			GPU_PROFILE("Frame");
			glMatrixMode(GL_PROJECTION);
			glLoadMatrixf(view.viewToClip);
			glMatrixMode(GL_MODELVIEW);
			glLoadMatrixf(view.worldToView);

			glClearColor(0, 0, 0, 0);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			RHISetViewport(view.rectOffset.x, view.rectOffset.y, view.rectSize.x, view.rectSize.y);
			//drawAxis( 20 );
			RHISetRasterizerState(TStaticRasterizerState<>::GetRHI());
			RHISetDepthStencilState(TStaticDepthStencilState<>::GetRHI());
			RHISetBlendState(TStaticBlendState<>::GetRHI());
			mSampleTests[mIdxTestChioce].renderFun(view);


			
			{
				Vec2i screenSize = ::Global::GetDrawEngine()->getScreenSize();
				Vec2i imageSize = IntVector2(200, -200);
				ViewportSaveScope vpScope;
				OrthoMatrix matProj(0, vpScope[2], 0, vpScope[3], -1, 1);
				MatrixSaveScope matScope(matProj);
				RHISetBlendState(TStaticBlendState<CWM_RGBA , Blend::eSrcAlpha , Blend::eOneMinusSrcAlpha >::GetRHI());
				RHISetRasterizerState(TStaticRasterizerState< ECullMode::None >::GetRHI());
				LinearColor color = LinearColor(0.2, 0.2, 0.2, 0.2);
				DrawUtility::DrawTexture(getTexture(TextureId::WaterMark).getRHI(), IntVector2( screenSize.x , 0 ) - imageSize - IntVector2(20,-20), imageSize , color );
				RHISetRasterizerState(TStaticRasterizerState<>::GetRHI());
				RHISetBlendState(TStaticBlendState<>::GetRHI());
			}

		}

		GLGraphics2D& g = ::Global::GetRHIGraphics2D();

		FixString< 512 > str;
		Vector3 v;
		g.beginRender();


		char const* title = mSampleTests[mIdxTestChioce].name;

		int const offset = 15;
		int textX = 1000;
		int y = 10;
		str.format("%s lightCount = %d", title , renderLightCount);
		g.drawText(textX, y , str );
		str.format("CamPos = %.1f %.1f %.1f", view.worldPos.x, view.worldPos.y, view.worldPos.z);
		g.drawText(textX, y += offset, str);

		g.endRender();

		mGpuSync.postRender();
	}

	void SampleStage::render( RenderContext& context)
	{
		context.beginRender();
		renderScene(context);
		context.endRender();
	}

	void SampleStage::renderTranslucent( RenderContext& context)
	{
		context.beginRender();

		Matrix4 matWorld;
		if ( mIdxTestChioce != 4 )
		{
			StaticMesh& mesh = getMesh(MeshId::Sponza);
			matWorld = Matrix4::Identity();
			mesh.render(matWorld, context );
		}
		else
		{
			RHISetRasterizerState(TStaticRasterizerState<ECullMode::None>::GetRHI());
			if(1 )
			{
				matWorld = Matrix4::Rotate(Vector3(0, 0, -1), Math::Deg2Rad(45 + 180)) * Matrix4::Translate(Vector3(-6, 6, 4));

				Mesh& mesh = getMesh(MeshId::Teapot);
				context.setMaterial(getMaterial(MaterialId::MetelA));
				context.setWorld(matWorld);
				mesh.drawShader(LinearColor(0.7, 0.7, 0.7) );
				
			}
			if (1)
			{
				StaticMesh& mesh = getMesh(MeshId::Lightning);
				matWorld = Matrix4::Scale(3) * Matrix4::Rotate(Vector3(0, 0, -1), Math::Deg2Rad(135 + 180)) * Matrix4::Translate(Vector3(-16, 0, 10));
				//matWorld = Matrix4::Identity();
				mesh.render(matWorld, context);
			}
			RHISetRasterizerState(TStaticRasterizerState<>::GetRHI());
		}
		context.endRender();
	}

	void SampleStage::onRemoveLight(SceneLight* light)
	{
		for ( auto iter = mValueModifiers.begin() ; iter != mValueModifiers.end() ; )
		{
			IValueModifier* modifier = (*iter).get();
			if( modifier->isHook(&light->info.pos) ||
			    modifier->isHook(&light->info.color) ||
			    modifier->isHook(&light->info.intensity) )
			{
				iter = mValueModifiers.erase(iter);
			}
			else
			{
				++iter;
			}
		}
	}

	void SampleStage::onRemoveObject(SceneObject* object)
	{
		
	}

	void SampleStage::render_SpherePlane(ViewInfo& view)
	{
		//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		//glColor3f( 1 , 1 , 0 );

		//glTranslatef( 20 , 20 , 0 );

		{
			GL_BIND_LOCK_OBJECT(mProgSphere);

			view.setupShader(mProgSphere);
			mProgSphere.setParam(SHADER_PARAM(Sphere.radius), 0.3f);
			mProgSphere.setParam(SHADER_PARAM(Sphere.localPos), mLights[1].pos);
			mSimpleMeshs[ SimpleMeshId::SpherePlane ].draw();

			mProgSphere.setParam(SHADER_PARAM(Sphere.radius), 1.5f);
			mProgSphere.setParam(SHADER_PARAM(Sphere.localPos), Vector3(10, 3, 1.0f));
			mSimpleMeshs[SimpleMeshId::SpherePlane].draw();
		}

		float const off = 0.5;
		float xform[6][9] = 
		{
			{ 0,0,-1, 0,1,0, 1,0,0, } ,
			{ 0,0,1, 0,1,0, -1,0,0, } ,
			{ 1,0,0, 0,0,-1, 0,1,0, } ,
			{ 1,0,0, 0,0,1,  0,-1,0 } ,
			{ 1,0,0, 0,1,0,  0,0,1  } ,
			{ 1,0,0, 0,-1,0, 0,0,-1 } ,
		};
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

		GL_BIND_LOCK_OBJECT(mProgPlanet);

		mProgPlanet.setParam(SHADER_PARAM(planet.radius), 2.0f);
		mProgPlanet.setParam(SHADER_PARAM(planet.maxHeight), 0.1f);
		mProgPlanet.setParam(SHADER_PARAM(planet.baseHeight), 0.5f);

		float const TileBaseScale = 2.0f;
		float const TilePosMin = -1.0f;
		float const TilePosMax = 1.0f;
		mProgPlanet.setParam(SHADER_PARAM(tile.pos), TilePosMin, TilePosMin);
		mProgPlanet.setParam(SHADER_PARAM(tile.scale), TileBaseScale);

		for( int i = 0 ; i < 6 ; ++i )
		{
			mProgPlanet.setMatrix33(SHADER_PARAM(xformFace) , xform[i] );
			glPushMatrix();
			mSimpleMeshs[SimpleMeshId::Tile].draw();
			glPopMatrix();
		}
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}

	void SampleStage::render_ParallaxMapping(ViewInfo& view)
	{
		if ( 1 )
		{
			GL_BIND_LOCK_OBJECT(mProgParallax);
			view.setupShader(mProgParallax);

#if 0
			mProgParallax.setTexture(SHADER_PARAM(texBase), getTexture(TextureId::RocksD).getRHI(), 0);
			mProgParallax.setTexture(SHADER_PARAM(texNormal), getTexture(TextureId::RocksNH).getRHI(), 1);
#else
			mProgParallax.setTexture(SHADER_PARAM(texBase), getTexture(TextureId::Base).getRHI() , 0);
			mProgParallax.setTexture(SHADER_PARAM(texNormal), getTexture(TextureId::Normal).getRHI() , 1);
#endif

			mProgParallax.setParam(SHADER_PARAM(GLight.worldPosAndRadius), Vector4(mLights[1].pos, mLights[1].radius));
			mSimpleMeshs[SimpleMeshId::Plane].draw();
		}
	}


	void SampleStage::renderTest2(ViewInfo& view)
	{
		drawSky();
		visitLights([this, &view](int index, LightInfo const& light)
		{
			if( bUseFrustumTest && !light.testVisible( view )  )
				return;

			mTechShadow.renderLighting(view, *this, light, index != 0);
		});
		showLight(view);
		mTechShadow.drawShadowTexture( mLights[mNumLightDraw -1].type , Vec2i(0, 0), 200 );
		//TechBase::drawCubeTexture(mTexSky);
	}

	void SampleStage::render_RaycastTest(ViewInfo& view)
	{

		if ( mCameraMove.target != &mCamStorage[0] )
		{
			glPushMatrix();
			glMultMatrixf( mCamStorage[0].getTransform() );
			drawAxis( 1 );
			glColor3f( 1 , 1 , 1 );
			mFrustumMesh.draw();
			glPopMatrix();
		}

		glBegin(GL_LINES);
		glColor3f( 1 , 1 , 0 );
		glVertex3fv( &rayStart[0] );
		glVertex3fv( &rayEnd[0] );
		glEnd();

		mEffectSimple.bind();

		view.setupShader(mEffectSimple);
#if 0
		glPushMatrix();
		glMultMatrixf( Matrix4::Rotate( Vector3(1,1,1) , Math::Deg2Rad(45) ) * Matrix4::Translate( Vector3(3,-3,7) ) );
		glColor3f( 0.3 , 0.3 , 1 );
		mBoxMesh.draw();
		glPopMatrix();
#endif

		for ( int i = 0 ; i < NumAabb ; ++i )
		{
			AABB& aabb = mAabb[i];
			glPushMatrix();
			Vector3 len = aabb.max - aabb.min;
			glTranslatef( aabb.min.x , aabb.min.y , aabb.min.z );
			glScalef( len.x , len.y , len.z );
			if ( mIsIntersected )
				glColor3f( 1 , 1 , 1 );
			else
				glColor3f( 0.3 , 0.3 , 0.3 );

			DrawUtility::CubeMesh();
			glPopMatrix();
		}

		mEffectSimple.unbind();

		if ( mIsIntersected )
		{
			glPointSize( 5 );
			glBegin( GL_POINTS );
			glColor3f( 1 , 0 , 0 );
			glVertex3fv( &mIntersectPos[0] );
			glEnd();
			glPointSize( 1 );
		}
	}

	bool bUseSSAO = true;
	bool bUseDOF = true;
	void SampleStage::render_DeferredLighting(ViewInfo& view)
	{
		getScene(0).prepareRender(view);

		{
			GPU_PROFILE("BasePass");
			mTechDeferredShading.renderBassPass(view, *this );
		}
		if( bUseSSAO )
		{
			GPU_PROFILE("SSAO");
			mSSAO.render(view, mSceneRenderTargets);
		}

		if( 1 )
		{	
			GPU_PROFILE("Lighting");

			//RHISetRasterizerState(TStaticRasterizerState< ECullMode::None >::GetRHI());
			mTechDeferredShading.prevRenderLights(view);

			renderLightCount = 0;
			visitLights([this, &view](int index, LightInfo const& light)
			{
				if( bUseFrustumTest && !light.testVisible(view) )
					return;

				++renderLightCount;
				
				ShadowProjectParam shadowProjectParam;
				shadowProjectParam.setupLight(light);
				mTechShadow.renderShadowDepth(view, *this, shadowProjectParam);

				mTechDeferredShading.renderLight(view, light, shadowProjectParam );
			});
		}

		{
			std::vector< LightInfo > lights;
			visitLights([&lights](int index, LightInfo const& light)
			{
				lights.push_back(light);
			});
			GPU_PROFILE("VolumetricLighting");
			mTechVolumetricLighing.render(view, lights);
		}

		bUseDOF = false;

		if( bUseDOF )
		{
			GPU_PROFILE("DOF");
			mDOF.render(view, mSceneRenderTargets);
		}



		{
			GPU_PROFILE("ShowLight");
			mSceneRenderTargets.getFrameBuffer().setDepth(mSceneRenderTargets.getDepthTexture());
			{
				GL_BIND_LOCK_OBJECT(mSceneRenderTargets.getFrameBuffer());
				showLight(view);
			}
			mSceneRenderTargets.getFrameBuffer().removeDepthBuffer();
		}

		if( 1 )
		{
			GPU_PROFILE("DrawTransparency");
			mSceneRenderTargets.getFrameBuffer().setDepth(mSceneRenderTargets.getDepthTexture());
			{
				GL_BIND_LOCK_OBJECT(mSceneRenderTargets.getFrameBuffer());
				mTechOIT.render(view, *this, &mSceneRenderTargets);
			}
			mSceneRenderTargets.getFrameBuffer().removeDepthBuffer();
		}

		if(1)
		{
			GPU_PROFILE("ShowBuffer");

			ViewportSaveScope vpScope;
			OrthoMatrix matProj(0, vpScope[2], 0, vpScope[3], -1, 1);
			MatrixSaveScope matScope(matProj);

			Vec2i showSize;

			float gapFactor = 0.03;

			Vec2i size;
			size.x = vpScope.value[2] / 4;
			size.y = vpScope.value[3] / 4;

			Vec2i gapSize;
			gapSize.x = size.x * gapFactor;
			gapSize.y = size.y * gapFactor;

			Vec2i renderSize = size - 2 * gapSize;

			if( mbShowBuffer )
			{
				{
#if 0
					ViewportSaveScope vpScope;
					int w = vpScope[2] / 4;
					int h = vpScope[3] / 4;
					RHISetViewport(0, h, 3 * w, 3 * h);
#endif
					ShaderHelper::Get().copyTextureToBuffer(mSceneRenderTargets.getRenderFrameTexture());
				}

				RHISetDepthStencilState(StaticDepthDisableState::GetRHI());
				mSceneRenderTargets.getGBuffer().drawTextures( size , gapSize );

				mSSAO.drawSSAOTexture(Vec2i(0, 1).mul(size) + gapSize, renderSize);
#if 0
				DrawUtility::DrawTexture(*mTechOIT.mColorStorageTexture, Vec2i(0, 0), Vec2i(200, 200));
#else
				DrawUtility::DrawTexture( getTexture(TextureId::MarioD).getRHI() , Vec2i(0, 0), Vec2i(200, 200));
#endif
			}
			else
			{
				ShaderHelper::Get().copyTextureToBuffer(mSceneRenderTargets.getRenderFrameTexture());

				RHISetDepthStencilState(StaticDepthDisableState::GetRHI());
				mTechShadow.drawShadowTexture(mLights[mNumLightDraw - 1].type , Vec2i(0, 0), 200 );
			}
		}
	}

	void SampleStage::render_OIT(ViewInfo& view)
	{
		glClearColor(0.7, 0.7, 0.7, 1);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		if( 1 )
		{
			StaticMesh& mesh = getMesh(MeshId::Dragon2);
			RHISetDepthStencilState(TStaticDepthStencilState<>::GetRHI());
			RHISetupFixedPipelineState(view.worldToView , view.viewToClip);
			mesh.drawShader(LinearColor(1, 1, 0));
			mTechOIT.render(view, *this , nullptr );
		}
		else
		{
			mTechOIT.renderTest(view, mSceneRenderTargets, getMesh(MeshId::Havoc), getMaterial(MaterialId::Havoc));
		}	
	}


	void SampleStage::render_Terrain(ViewInfo& view)
	{


		{
			GL_BIND_LOCK_OBJECT(mProgTerrain);

			view.setupShader(mProgTerrain);
			mProgTerrain.setParam( SHADER_PARAM(LocalToWorld), Matrix4::Identity() );
			mProgTerrain.setParam( SHADER_PARAM(TerrainScale), Vector3( 50 , 50 , 10 ) );
			mProgTerrain.setTexture( SHADER_PARAM(TextureHeight), *mTexTerrainHeight);
			mProgTerrain.setTexture(SHADER_PARAM(TextureBaseColor), getTexture(TextureId::Terrain).getRHI());
			mSimpleMeshs[SimpleMeshId::Terrain].drawShader();
		}


	}

	void SampleStage::render_Portal(ViewInfo& view)
	{

	}

	class ShadowVolumeProgram : public GlobalShaderProgram
	{
		DECLARE_GLOBAL_SHADER(ShadowVolumeProgram);
	public:
		static void SetupShaderCompileOption(ShaderCompileOption& option)
		{

		}
		static char const* GetShaderFileName()
		{
			return "Shader/ShadowVolume";
		}
		static ShaderEntryInfo const* GetShaderEntries()
		{

			static ShaderEntryInfo const entries[] =
			{
				{ Shader::eVertex , SHADER_ENTRY(MainVS) },
				{ Shader::eGeometry  , SHADER_ENTRY(MainGS) },
				{ Shader::ePixel  , SHADER_ENTRY(MainPS) },
				{ Shader::eEmpty  , nullptr },
			};

			return entries;
		}

	};

	IMPLEMENT_GLOBAL_SHADER(ShadowVolumeProgram);

	void SampleStage::render_ShadowVolume(ViewInfo& view)
	{
		drawAxis(10);



		{
			GL_BIND_LOCK_OBJECT(mProgTerrain);

			view.setupShader(mProgTerrain);
			mProgTerrain.setParam(SHADER_PARAM(LocalToWorld), Matrix4::Translate(-50,-50,-10));
			mProgTerrain.setParam(SHADER_PARAM(TerrainScale), Vector3(100, 100, 20));
			mProgTerrain.setTexture(SHADER_PARAM(TextureHeight), *mTexTerrainHeight);
			mProgTerrain.setTexture(SHADER_PARAM(TextureBaseColor), getTexture(TextureId::Terrain).getRHI());
			mSimpleMeshs[SimpleMeshId::Terrain].drawShader();
		}


		auto& mesh = *getMesh(MeshId::Mario).get();
		//auto& mesh = mSimpleMeshs[SimpleMeshId::Box];
		if( !mesh.generateVertexAdjacency() )
			return;


		Matrix4 localToWorld = Matrix4::Rotate( Vector3(0,0,1) , Math::Deg2Rad(mRealTime * 45));
		{
			RHISetupFixedPipelineState(localToWorld * view.worldToView, view.viewToClip);
			mesh.draw(LinearColor(0.2, 0.2, 0.2, 1));
		}

		auto DrawMesh = [&]()
		{
			mProgShadowVolume->setParam(SHADER_PARAM(LocalToWorld), localToWorld);
			mesh.drawAdjShader(LinearColor(1, 0, 0, 1));
		};

		bool bVisualizeVolume = false;
		bool bUseDepthPass = false;
		bool bShowVolumeStencil = true;

		{

			GPU_PROFILE("Render Shadow Volume");

			glStencilMask(-1);
			glClearStencil(0);
			glClear(GL_STENCIL_BUFFER_BIT);

			RHISetRasterizerState(TStaticRasterizerState< ECullMode::None >::GetRHI());
			if( bVisualizeVolume )
			{
				RHISetBlendState(TStaticBlendState< CWM_RGBA, Blend::eOne, Blend::eOne >::GetRHI());
			}
			else
			{
				RHISetBlendState(TStaticBlendState<CWM_NONE>::GetRHI());
			}		

			if( bUseDepthPass )
			{
				RHISetDepthStencilState(
					TStaticDepthStencilSeparateState< false, ECompareFun::Less, true,
						ECompareFun::Always, Stencil::eKeep, Stencil::eKeep, Stencil::eDecrWarp,
						ECompareFun::Always, Stencil::eKeep, Stencil::eKeep, Stencil::eIncrWarp
					>::GetRHI());
			}
			else
			{				
				RHISetDepthStencilState(
					TStaticDepthStencilSeparateState< false, ECompareFun::Less, true,
						ECompareFun::Always, Stencil::eKeep, Stencil::eDecrWarp, Stencil::eKeep,
						ECompareFun::Always, Stencil::eKeep, Stencil::eIncrWarp, Stencil::eKeep
					>::GetRHI());
			}


#if 0
			Matrix4 prevProjectMatrix = view.viewToClip;
			float zn = PerspectiveMatrix::GetNearZ(prevProjectMatrix);
			Matrix4 newProjectMatrix = prevProjectMatrix;
			newProjectMatrix[10] = -1;
			newProjectMatrix[14] = -2 * zn;
			view.setupTransform(view.worldToView, newProjectMatrix);
#endif

			GL_BIND_LOCK_OBJECT(*mProgShadowVolume);
			mProgShadowVolume->setParam(SHADER_PARAM(LightPos), mLights[0].pos);
			mProgShadowVolume->setParam(SHADER_PARAM(LightPos), Vector3(20, 10, 20));
			view.setupShader(*mProgShadowVolume);

			//glEnable(GL_POLYGON_OFFSET_FILL);
			//glPolygonOffset(-1.0, -1.0);
			//glLineWidth(8);
			DrawMesh();
			//glLineWidth(1);
			//glDisable(GL_POLYGON_OFFSET_FILL);
			RHISetRasterizerState(TStaticRasterizerState<>::GetRHI());
		}

		
		if ( bShowVolumeStencil  )
		{
			ViewportSaveScope vpScope;
			MatrixSaveScope matrixScopt( Matrix4::Identity() );
			RHISetRasterizerState(TStaticRasterizerState<ECullMode::None>::GetRHI());
			RHISetDepthStencilState(TStaticDepthStencilState< 
				false , ECompareFun::Always , 
				true , ECompareFun::NotEqual , Stencil::eKeep , Stencil::eKeep , Stencil::eKeep , 0xff , 0xff 
			>::GetRHI() , 0x0 );
			RHISetBlendState(TStaticBlendState< CWM_RGBA , Blend::eOne , Blend::eOne >::GetRHI());
			glColor3f(0, 0, 0.8);
			DrawUtility::ScreenRect();
		}
		RHISetDepthStencilState(TStaticDepthStencilState<>::GetRHI());
		RHISetBlendState(TStaticBlendState<>::GetRHI());
	}

	void SampleStage::showLight(ViewInfo& view)
	{
		GL_BIND_LOCK_OBJECT(mProgSphere);

		RHISetBlendState(TStaticBlendState<>::GetRHI());
		RHISetDepthStencilState(TStaticDepthStencilState<>::GetRHI());

		float radius = 0.15f;
		view.setupShader(mProgSphere);
		mProgSphere.setParam(SHADER_PARAM(Primitive.worldToLocal), Matrix4::Identity());
		mProgSphere.setParam(SHADER_PARAM(Sphere.radius), radius);

		visitLights([this,&view, radius](int index, LightInfo const& light)
		{
			if( bUseFrustumTest && !light.testVisible(view) )
				return;

			mProgSphere.setParam(SHADER_PARAM(Sphere.localPos), light.pos);
			mProgSphere.setParam(SHADER_PARAM(Sphere.baseColor), light.color);
			mSimpleMeshs[SimpleMeshId::SpherePlane].draw();
		});
	}


	void SampleStage::drawSky()
	{
		Matrix4 mat;
		glGetFloatv( GL_MODELVIEW_MATRIX , mat );

		RHISetDepthStencilState(StaticDepthDisableState::GetRHI());

		glEnable( GL_TEXTURE_CUBE_MAP );
		OpenGLCast::To(mTexSky)->bind();

		glPushMatrix();

		mat.modifyTranslation( Vector3(0,0,0) );
		glLoadMatrixf( Matrix4::Rotate( Vector3(1,0,0), Math::Deg2Rad(90) ) * mat );
		mSimpleMeshs[SimpleMeshId::SkyBox].draw( LinearColor(1, 1, 1));
		glPopMatrix();

		OpenGLCast::To(mTexSky)->unbind();
		glDisable( GL_TEXTURE_CUBE_MAP );
	}

	void SampleStage::drawAxis(float len)
	{

		struct Vertex
		{
			Vector3 pos;
			Vector3 color;
		};

		Vertex v[] =
		{
			{ Vector3(0,0,0), Vector3(1,0,0)  } ,{ Vector3(len,0,0),Vector3(1,0,0) },
			{ Vector3(0,0,0), Vector3(0,1,0)  } ,{ Vector3(0,len,0),Vector3(0,1,0) },
			{ Vector3(0,0,0), Vector3(0,0,1), } ,{ Vector3(0,0,len),Vector3(0,0,1) },
		};

		glLineWidth( 1.5 );
		TRenderRT< RTVF_XYZ_C >::Draw(PrimitiveType::LineList, &v[0], 6);
		glLineWidth( 1 );
	}

	void SampleStage::render_Sprite(ViewInfo& view)
	{
		Vector3 CubeFaceDir[] =
		{
			Vector3(1,0,0),Vector3(-1,0,0),
			Vector3(0,1,0),Vector3(0,-1,0),
			Vector3(0,0,1),Vector3(0,0,-1),
		};
		Vector3 CubeUpDir[] =
		{
			Vector3(0,-1,0),Vector3(0,-1,0),
			Vector3(0,0,1),Vector3(0,0,-1),
			Vector3(0,-1,0),Vector3(0,-1,0),
		};
		Matrix4 viewMatrix[6];

		Matrix4 viewproject = PerspectiveMatrix(Math::Deg2Rad(90), 1.0, 0.01, 500);
		for( int i = 0; i < 6; ++i )
		{
			viewMatrix[i] = LookAtMatrix(view.worldPos, CubeFaceDir[i], CubeUpDir[i]);
		};

		{
			GPU_PROFILE("DrawSprite");
			GL_BIND_LOCK_OBJECT(mLayerFrameBuffer);
			glClear(GL_COLOR_BUFFER_BIT);
			RHISetViewport(0, 0, 2048, 2048);
			RHISetDepthStencilState(StaticDepthDisableState::GetRHI());
			RHISetBlendState(TStaticBlendState< CWM_RGBA, Blend::eOne, Blend::eOne >::GetRHI());
			
			GL_BIND_LOCK_OBJECT(mProgSimpleSprite);
			view.setupShader(mProgSimpleSprite);
			mProgSimpleSprite.setTexture(SHADER_PARAM(BaseTexture), getTexture(TextureId::Star).getRHI());
			mProgSimpleSprite.setParam(SHADER_PARAM(ViewMatrix), viewMatrix, 6);
			mProgSimpleSprite.setParam(SHADER_PARAM(ViewProjectMatrix), viewproject );
			mSpritePosMesh.drawShader();

		}

		if( 1 )
		{
			ViewportSaveScope vpScope;
			OrthoMatrix matProj(0, vpScope.value[2], 0, vpScope.value[3], -1, 1);
			MatrixSaveScope matScope(matProj);

			Vec2i showSize;

			float gapFactor = 0.03;

			Vec2i size;
			size.x = vpScope.value[2] / 4;
			size.y = vpScope.value[3] / 4;

			Vec2i gapSize;
			gapSize.x = size.x * gapFactor;
			gapSize.y = size.y * gapFactor;

			Vec2i renderSize = size - 2 * gapSize;

			if( mbShowBuffer )
			{
				RHISetDepthStencilState(StaticDepthDisableState::GetRHI());
				RHISetBlendState(TStaticBlendState<>::GetRHI());
				mTechShadow.drawShadowTexture(LightType::Point , Vec2i(0,0) , 400 );
				//mSceneRenderTargets.getGBuffer().drawTextures(size, gapSize);
			}
		}
	}

	bool SampleStage::createFrustum(Mesh& mesh, Matrix4 const& matProj)
	{
		float depth = 0.9999;
		Vector3 v[8] =
		{
			Vector3(1,1,depth),Vector3(1,-1,depth),Vector3(-1,-1,depth),Vector3(-1,1,depth),
			Vector3(1,1,-depth),Vector3(1,-1,-depth),Vector3(-1,-1,-depth),Vector3(-1,1,-depth),
		};

		Matrix4 matInv;
		float det;
		matProj.inverse(matInv, det);
		for( int i = 0; i < 8; ++i )
		{
			v[i] = TransformPosition(v[i], matInv);
		}

		int idx[24] =
		{
			0,1, 1,2, 2,3, 3,0,
			4,5, 5,6, 6,7, 7,4,
			0,4, 1,5, 2,6, 3,7,
		};
		mesh.mInputLayoutDesc.addElement(Vertex::ePosition, Vertex::eFloat3);
		if( !mesh.createRHIResource(v, 8, idx, 24, true) )
			return false;
		mesh.mType = PrimitiveType::LineList;
		return true;
	}

	void SampleStage::calcViewRay(float x, float y)
	{
		Matrix4 matViewProj = mCameraMove.target->getViewMatrix() * mViewFrustum.getPerspectiveMatrix();
		Matrix4 matVPInv;
		float det;
		matViewProj.inverse(matVPInv, det);

		rayStart = TransformPosition(Vector3(x, y, -1), matVPInv);
		rayEnd = TransformPosition(Vector3(x, y, 1), matVPInv);
	}

	bool SampleStage::onKey(unsigned key , bool isDown)
	{
		if ( !isDown )
			return false;

		switch( key )
		{
		case Keyboard::eR: restart(); break;
		case 'W': mCameraMove.moveFront(); break;
		case 'S': mCameraMove.moveBack(); break;
		case 'D': mCameraMove.moveRight(); break;
		case 'A': mCameraMove.moveLeft(); break;
		case 'Z': mCameraMove.target->moveUp(0.5); break;
		case 'X': mCameraMove.target->moveUp(-0.5); break;
		case Keyboard::eADD: mNumLightDraw = std::min(mNumLightDraw + 1, 4); break;
		case Keyboard::eSUBTRACT: mNumLightDraw = std::max(mNumLightDraw - 1, 0); break;
		case Keyboard::eB: mbShowBuffer = !mbShowBuffer; break;
		case Keyboard::eF3: 
			mLineMode = !mLineMode;
			glPolygonMode( GL_FRONT_AND_BACK , mLineMode ? GL_LINE : GL_FILL );
			break;
		case Keyboard::eM:
			ShaderHelper::Get().reload();
			reloadMaterials();
			break;
		case Keyboard::eL:
			mLights[0].bCastShadow = !mLights[0].bCastShadow;
			break;
		case Keyboard::eK:
			mTechOIT.bUseBMA = !mTechOIT.bUseBMA;
			break;
		case Keyboard::eP: mPause = !mPause; break;
		case Keyboard::eF2:
			if (0)
			{
				switch( mIdxTestChioce )
				{
				case 0:
					ShaderManager::Get().reloadShader(mProgSphere);
					ShaderManager::Get().reloadShader(mProgPlanet);
					break;
				case 1:
					ShaderManager::Get().reloadShader(mProgParallax);
					break;
				case 2: mTechShadow.reload(); break;
				case 4: mTechDeferredShading.reload(); mSSAO.reload(); break;
				case 5: ShaderManager::Get().reloadShader(mProgSimpleSprite); break;
				case 6: mTechOIT.reload(); break;
				}
			}
			else
			{
				ShaderManager::Get().reloadAll();
			}
			break;
		case Keyboard::eF5:
			if ( mCameraMove.target == &mCamStorage[0])
				mCameraMove.target = &mCamStorage[1];
			else
				mCameraMove.target = &mCamStorage[0];
			break;
		case Keyboard::eF6:
			mTechDeferredShading.boundMethod = (LightBoundMethod)( ( mTechDeferredShading.boundMethod + 1 ) % NumLightBoundMethod );
			break;
		case Keyboard::eF7:
			mTechDeferredShading.debugMode = (DeferredShadingTech::DebugMode)( ( mTechDeferredShading.debugMode + 1 ) % DeferredShadingTech::NumDebugMode );
			break;
		case Keyboard::eF8:
			bUseFrustumTest = !bUseFrustumTest;
			break;
		case Keyboard::eF4:
			break;
		case Keyboard::eNUM1: mIdxTestChioce = 0; break;
		case Keyboard::eNUM2: mIdxTestChioce = 1; break;
		case Keyboard::eNUM3: mIdxTestChioce = 2; break;
		case Keyboard::eNUM4: mIdxTestChioce = 3; break;
		case Keyboard::eNUM5: mIdxTestChioce = 4; break;
		case Keyboard::eNUM6: mIdxTestChioce = 5; break;
		case Keyboard::eNUM7: mIdxTestChioce = 6; break;
		}
		return false;
	}

	bool SampleStage::onWidgetEvent(int event, int id, GWidget* ui)
	{
		switch( id )
		{
		case UI_SAMPLE_TEST:
			mIdxTestChioce = ui->getUserData();
			return false;
		}

		return BaseClass::onWidgetEvent(event, id, ui);
	}

	void SampleStage::reloadMaterials()
	{
		for( int i = 0; i < mMaterialAssets.size(); ++i )
		{
			mMaterialAssets[i].material->reload();
		}
	}

	bool SampleStage::onMouse(MouseMsg const& msg)
	{
		static Vec2i oldPos = msg.getPos();
		
		if ( msg.onLeftDown() )
		{
			oldPos = msg.getPos();
		}
		if ( msg.onMoving() && msg.isLeftDown() )
		{
			float rotateSpeed = 0.01;
			Vector2 off = rotateSpeed * Vector2( msg.getPos() - oldPos );
			mCameraMove.target->rotateByMouse( off.x , off.y );
			oldPos = msg.getPos();
		}

		if ( msg.onRightDown() )
		{
			Vec2i size = ::Global::GetDrawEngine()->getScreenSize();
			Vec2i pos = msg.getPos();
			float x = float( 2 * pos.x ) / size.x - 1;
			float y = 1 - float( 2 * pos.y ) / size.y;
			calcViewRay( x , y );
			
			float tMin = 1;
			mIsIntersected = false;
			for( int i = 0 ; i < NumAabb ; ++i )
			{
				AABB& aabb = mAabb[i];
				float t = -1;
				if ( TestRayAABB( rayStart , rayEnd - rayStart , aabb.min , aabb.max , t ) )
				{
					mIsIntersected = true;
					if ( t < tMin )
					{
						tMin = t;
						mIntersectPos = rayStart + ( rayEnd - rayStart ) * t;
					}
				}
			}
		}

		if ( !BaseClass::onMouse( msg ) )
			return false;

		return true;
	}



	bool WaterTech::init()
	{
		if ( !mBuffer.create() )
			return false;
		if ( !MeshBuild::PlaneZ( mWaterMesh , 20 , 20 ) )
			return false;

		mReflectMap = RHICreateTexture2D(Texture::eRGB32F, MapSize, MapSize);
		if ( !mReflectMap.isValid() )
			return false;

		mBuffer.addTexture( *mReflectMap );
#if USE_DepthRenderBuffer
		RHIDepthRenderBufferRef depthBuf = new RHIDepthRenderBuffer;
		if ( ! depthBuf->create( MapSize , MapSize , Texture::eDepth32 ) )
			return false;
		mBuffer.setDepth( *depthBuf );
#endif
		return true;
	}


	bool GLGpuSync::pervRender()
	{
		if( !bUseFence )
			return true;

		GLbitfield flags = 0;

		if( loadingSync != NULL )
		{
			GLbitfield flags = 0;
			glWaitSync(loadingSync, flags, GL_TIMEOUT_IGNORED);
		}
		return true;
	}

	void GLGpuSync::postRender()
	{
		if( bUseFence )
		{
			GLbitfield flags = 0;
			GLenum condition = GL_SYNC_GPU_COMMANDS_COMPLETE;
			renderSync = glFenceSync(condition, flags);
		}
	}

	bool GLGpuSync::pervLoading()
	{
		if( !bUseFence )
			return true;
		if( renderSync != NULL )
		{
			GLbitfield flags = 0;
			glWaitSync(renderSync, flags, GL_TIMEOUT_IGNORED);
		}
		else
		{
			return false;
		}

		return true;
	}

	void GLGpuSync::postLoading()
	{
		if( bUseFence )
		{
			GLbitfield flags = 0;
			GLenum condition = GL_SYNC_GPU_COMMANDS_COMPLETE;
			loadingSync = glFenceSync(condition, flags);
		}
	}



}//namespace Render



