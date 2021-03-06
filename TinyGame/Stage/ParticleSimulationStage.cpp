#include "TestStageHeader.h"


#include "RHI/RHICommon.h"
#include "RHI/ShaderCompiler.h"
#include "RHI/RenderContext.h"
#include "RHI/DrawUtility.h"
#include "RHI/MeshUtility.h"


namespace Render
{
	struct alignas(16) ParticleData
	{
		DECLARE_BUFFER_STRUCT(ParticleDataBlock);

		Vector3 pos;
		float   lifeTime;
		Vector3 velocity;
		float   size;
		Vector4 color;
		Vector3  dumy;
		float   lifeTimeScale;	
	};

	struct alignas(16) ParticleParameters
	{
		DECLARE_BUFFER_STRUCT(ParticleParamBlock);
		Vector3  gravity;
		uint32   numParticle;
		Vector4  dynamic;
	};

	struct alignas(16) ParticleInitParameters
	{
		DECLARE_BUFFER_STRUCT(ParticleInitParamBlock);
		Vector3 posMin;
		Vector3 posMax;
	};

	struct alignas(16) CollisionPrimitive
	{
		DECLARE_BUFFER_STRUCT(CollisionPrimitiveBlock);
		Vector3 velocity;
		int32   type;
		Vector4 param;
	};

	template< class T , class ResourceType >
	class TStructuredBufferBase
	{
	public:

		void releaseResources()
		{
			mResource->release();
		}


		uint32 getElementNum() { return mResource->getSize() / sizeof(T); }

		ResourceType* getRHI() { return mResource; }

		T*   lock()
		{
			return (T*)RHILockBuffer(mResource, ELockAccess::WriteOnly);
		}
		void unlock()
		{
			RHIUnlockBuffer(mResource);
		}

		TRefCountPtr<ResourceType> mResource;
	};


	template< class T >
	class TStructuredUniformBuffer : public TStructuredBufferBase< T , RHIUniformBuffer >
	{
	public:
		bool initializeResource(uint32 numElement)
		{
			mResource = RHICreateUniformBuffer(sizeof(T) , numElement );
			if( !mResource.isValid() )
				return false;
			return true;
		}


	};

	template< class T >
	class TStructuredStorageBuffer : public TStructuredBufferBase< T , RHIVertexBuffer >
	{
	public:
		bool initializeResource(uint32 numElement)
		{
			mResource = RHICreateVertexBuffer(sizeof(T) , numElement );
			if( !mResource.isValid() )
				return false;
			return true;
		}
	};

	class ParticleSimBaseProgram : public GlobalShaderProgram
	{
	public:
		static char const* GetShaderFileName()
		{
			return "Shader/ParticleSimulation";
		}

		void setParameters(
			TStructuredStorageBuffer< ParticleData >& particleBuffer,
			TStructuredUniformBuffer< ParticleParameters >& paramBuffer,
			TStructuredUniformBuffer< ParticleInitParameters >& paramInitBuffer)
		{
			setStructuredBufferT< ParticleData >(*particleBuffer.getRHI());
			setStructuredBufferT< ParticleInitParameters >(*paramInitBuffer.getRHI());
			setStructuredBufferT< ParticleParameters >(*paramBuffer.getRHI());
		}
	};

	class ParticleInitProgram : public ParticleSimBaseProgram
	{
		DECLARE_GLOBAL_SHADER(ParticleInitProgram);
		typedef ParticleSimBaseProgram BaseClass;


		void bindParameters(ShaderParameterMap& parameterMap)
		{
		}

		void setParameters(
			TStructuredStorageBuffer< ParticleData >& particleBuffer,
			TStructuredUniformBuffer< ParticleParameters >& paramBuffer,
			TStructuredUniformBuffer< ParticleInitParameters >& paramInitBuffer)
		{
			BaseClass::setParameters(particleBuffer, paramBuffer, paramInitBuffer);
		}

		static void SetupShaderCompileOption(ShaderCompileOption& option)
		{
			BaseClass::SetupShaderCompileOption(option);
		}

		static ShaderEntryInfo const* GetShaderEntries()
		{
			static ShaderEntryInfo entries[] =
			{
				{ Shader::eCompute , SHADER_ENTRY(MainInitCS) },
				{ Shader::eEmpty , nullptr },
			};
			return entries;
		}
	};


	class ParticleUpdateProgram : public ParticleSimBaseProgram
	{

		DECLARE_GLOBAL_SHADER(ParticleUpdateProgram);
		typedef ParticleSimBaseProgram BaseClass;

		void bindParameters(ShaderParameterMap& parameterMap)
		{
			BaseClass::bindParameters(parameterMap);
			parameterMap.bind( mParamDeltaTime , SHADER_PARAM(DeltaTime));
			parameterMap.bind( mParamNumCollisionPrimitive , SHADER_PARAM(NumCollisionPrimitive));
		}

		void setParameters(
			TStructuredStorageBuffer< ParticleData >& particleBuffer,
			TStructuredUniformBuffer< ParticleParameters >& paramBuffer,
			TStructuredUniformBuffer< ParticleInitParameters >& paramInitBuffer,
			float deltaTime , int32 numCol )
		{
			BaseClass::setParameters(particleBuffer, paramBuffer, paramInitBuffer);
			setParam(mParamDeltaTime, deltaTime);
			setParam(mParamNumCollisionPrimitive, numCol);
		}

		static void SetupShaderCompileOption(ShaderCompileOption& option)
		{
			BaseClass::SetupShaderCompileOption(option);

		}

		static ShaderEntryInfo const* GetShaderEntries()
		{
			static ShaderEntryInfo entries[] =
			{
				{ Shader::eCompute , SHADER_ENTRY(MainUpdateCS) },
				{ Shader::eEmpty , nullptr },
			};
			return entries;
		}

		ShaderParameter mParamNumCollisionPrimitive;
		ShaderParameter mParamDeltaTime;
	};


	IMPLEMENT_GLOBAL_SHADER(ParticleInitProgram);
	IMPLEMENT_GLOBAL_SHADER(ParticleUpdateProgram);

	class SimpleSpriteParticleProgram : public GlobalShaderProgram
	{
	public:
		DECLARE_GLOBAL_SHADER(SimpleSpriteParticleProgram);
		typedef GlobalShaderProgram BaseClass;

		void bindParameters(ShaderParameterMap& parameterMap)
		{
			parameterMap.bind(mParamBaseTexture, SHADER_PARAM(BaseTexture));
		}

		void setParameters(
			TStructuredStorageBuffer< ParticleData >& particleBuffer , RHITexture2D& texture )
		{
			setStructuredBufferT< ParticleData >(*particleBuffer.getRHI());
			setTexture(mParamBaseTexture, texture);
		}

		static void SetupShaderCompileOption(ShaderCompileOption& option)
		{
			BaseClass::SetupShaderCompileOption(option);
		}

		static char const* GetShaderFileName()
		{
			return "Shader/Game/SimpleSpriteParticle";
		}

		static ShaderEntryInfo const* GetShaderEntries()
		{
			static ShaderEntryInfo entries[] =
			{
				{ Shader::eVertex , SHADER_ENTRY(MainVS) },
				{ Shader::eGeometry , SHADER_ENTRY(MainGS) },
				{ Shader::ePixel , SHADER_ENTRY(MainPS) },
				{ Shader::eEmpty , nullptr },
			};
			return entries;
		}

		ShaderParameter mParamBaseTexture;
	};

	IMPLEMENT_GLOBAL_SHADER(SimpleSpriteParticleProgram);

	class SplineProgram : public GlobalShaderProgram
	{
	public:
		DECLARE_GLOBAL_SHADER(SplineProgram);
		typedef GlobalShaderProgram BaseClass;

		static bool constexpr UseTesselation = true;

		void bindParameters(ShaderParameterMap& parameterMap)
		{

		}

		void setParameters()
		{

		}

		static void SetupShaderCompileOption(ShaderCompileOption& option)
		{
			BaseClass::SetupShaderCompileOption(option);
			option.addDefine(SHADER_PARAM(USE_TESSELLATION), UseTesselation);
		}

		static char const* GetShaderFileName()
		{
			return "Shader/Game/Spline";
		}

		static ShaderEntryInfo const* GetShaderEntries()
		{
			if ( UseTesselation )
			{
				static ShaderEntryInfo entriesWithTesselation[] =
				{
					{ Shader::eVertex , SHADER_ENTRY(MainVS) },
					{ Shader::eHull   , SHADER_ENTRY(MainHS) },
					{ Shader::eDomain , SHADER_ENTRY(MainDS) },
					{ Shader::ePixel , SHADER_ENTRY(MainPS) },
					{ Shader::eEmpty , nullptr },
				};
				return entriesWithTesselation;
			}

			static ShaderEntryInfo entries[] =
			{
				{ Shader::eVertex , SHADER_ENTRY(MainVS) },
				{ Shader::ePixel , SHADER_ENTRY(MainPS) },
				{ Shader::eEmpty , nullptr },
			};
			return entries;
		}
	};

	IMPLEMENT_GLOBAL_SHADER(SplineProgram);

	template< bool bEnable >
	class TTessellationProgram : public GlobalShaderProgram
	{
	public:
		DECLARE_GLOBAL_SHADER(TTessellationProgram);
		typedef GlobalShaderProgram BaseClass;

		void bindParameters(ShaderParameterMap& parameterMap)
		{

		}

		void setParameters()
		{

		}

		static void SetupShaderCompileOption(ShaderCompileOption& option)
		{
			BaseClass::SetupShaderCompileOption(option);
			option.addDefine(SHADER_PARAM(USE_TESSELLATION), bEnable);
		}

		static char const* GetShaderFileName()
		{
			return "Shader/Game/Tessellation";
		}

		static ShaderEntryInfo const* GetShaderEntries()
		{
			if( bEnable )
			{
				static ShaderEntryInfo entriesWithTesselation[] =
				{
					{ Shader::eVertex , SHADER_ENTRY(MainVS) },
					{ Shader::eHull   , SHADER_ENTRY(MainHS) },
					{ Shader::eDomain , SHADER_ENTRY(MainDS) },
					{ Shader::ePixel , SHADER_ENTRY(MainPS) },
					{ Shader::eEmpty , nullptr },
				};
				return entriesWithTesselation;
			}

			static ShaderEntryInfo entries[] =
			{
				{ Shader::eVertex , SHADER_ENTRY(MainVS) },
				{ Shader::ePixel , SHADER_ENTRY(MainPS) },
				{ Shader::eEmpty , nullptr },
			};
			return entries;
		}
	};
	IMPLEMENT_GLOBAL_SHADER(TTessellationProgram<true>);
	IMPLEMENT_GLOBAL_SHADER(TTessellationProgram<false>);

	struct alignas(16) WaterVertexData
	{
		//Vector3 normal;
		Vector2 normal;
		float h;
		float v;
		
	};

	class WaterSimulationProgram : public GlobalShaderProgram
	{
		DECLARE_GLOBAL_SHADER(WaterSimulationProgram);
		typedef GlobalShaderProgram BaseClass;

		void bindParameters(ShaderParameterMap& parameterMap)
		{
			BaseClass::bindParameters(parameterMap);
			parameterMap.bind(mParamDataIn, SHADER_PARAM(WaterDataInBlock));
			parameterMap.bind(mParamDataOut, SHADER_PARAM(WaterDataOutBlock));
			parameterMap.bind(mParamWaterParam, SHADER_PARAM(WaterParam));
			parameterMap.bind(mParamTileNum, SHADER_PARAM(TileNum));
		}

		void setParameters( Vector4 const& param , int TileNum , RHIVertexBuffer& DataIn , RHIVertexBuffer& DataOut )
		{
			setStorageBuffer(mParamDataIn, DataIn);
			setStorageBuffer(mParamDataOut, DataOut);
			setParam(mParamWaterParam, param);
			setParam(mParamTileNum, TileNum);
		}


		static void SetupShaderCompileOption(ShaderCompileOption& option)
		{
			BaseClass::SetupShaderCompileOption(option);

		}

		static char const* GetShaderFileName()
		{
			return "Shader/Game/WaterSimulation";
		}

		static ShaderEntryInfo const* GetShaderEntries()
		{
			static ShaderEntryInfo entries[] =
			{
				{ Shader::eCompute , SHADER_ENTRY(MainUpdateCS) },
				{ Shader::eEmpty , nullptr },
			};
			return entries;
		}

		ShaderParameter mParamDataIn;
		ShaderParameter mParamDataOut;
		ShaderParameter mParamWaterParam;
		ShaderParameter mParamTileNum;
	};

	class WaterUpdateNormalProgram : public GlobalShaderProgram
	{
		DECLARE_GLOBAL_SHADER(WaterUpdateNormalProgram);
		typedef GlobalShaderProgram BaseClass;

		void bindParameters(ShaderParameterMap& parameterMap)
		{
			BaseClass::bindParameters(parameterMap);
			parameterMap.bind(mParamData, SHADER_PARAM(WaterDataOutBlock));
			parameterMap.bind(mParamTileNum, SHADER_PARAM(TileNum));
		}

		void setParameters(int TileNum, RHIVertexBuffer& Data)
		{
			setStorageBuffer(mParamData, Data);
			setParam(mParamTileNum, TileNum);
		}

		static void SetupShaderCompileOption(ShaderCompileOption& option)
		{
			BaseClass::SetupShaderCompileOption(option);

		}

		static char const* GetShaderFileName()
		{
			return "Shader/Game/WaterSimulation";
		}

		static ShaderEntryInfo const* GetShaderEntries()
		{
			static ShaderEntryInfo entries[] =
			{
				{ Shader::eCompute , SHADER_ENTRY(MainUpdateNormal) },
				{ Shader::eEmpty , nullptr },
			};
			return entries;
		}

		ShaderParameter mParamData;
		ShaderParameter mParamTileNum;
	};


	class WaterProgram : public GlobalShaderProgram
	{
		DECLARE_GLOBAL_SHADER(WaterProgram);
		typedef GlobalShaderProgram BaseClass;

		void bindParameters(ShaderParameterMap& parameterMap)
		{
			BaseClass::bindParameters(parameterMap);
			parameterMap.bind(mParamDataIn, SHADER_PARAM(WaterDataInBlock));
			parameterMap.bind(mParamTileNum, SHADER_PARAM(TileNum));
		}

		void setParameters(int TileNum, RHIVertexBuffer& DataIn)
		{
			setStorageBuffer(mParamDataIn, DataIn);
			setParam(mParamTileNum, TileNum);
		}


		static void SetupShaderCompileOption(ShaderCompileOption& option)
		{
			BaseClass::SetupShaderCompileOption(option);

		}

		static char const* GetShaderFileName()
		{
			return "Shader/Game/Water";
		}

		static ShaderEntryInfo const* GetShaderEntries()
		{
			static ShaderEntryInfo entries[] =
			{
				{ Shader::eVertex , SHADER_ENTRY(MainVS) },
				{ Shader::ePixel , SHADER_ENTRY(MainPS) },
				{ Shader::eEmpty , nullptr },
			};
			return entries;
		}

		ShaderParameter mParamDataIn;
		ShaderParameter mParamTileNum;
	};

	IMPLEMENT_GLOBAL_SHADER(WaterSimulationProgram);
	IMPLEMENT_GLOBAL_SHADER(WaterUpdateNormalProgram);
	IMPLEMENT_GLOBAL_SHADER(WaterProgram);

	class ViewFrustum
	{
	public:
		Matrix4 getPerspectiveMatrix()
		{
			return PerspectiveMatrix(mYFov, mAspect, mNear, mFar);
		}

		float mNear;
		float mFar;
		float mYFov;
		float mAspect;
	};

	struct GPUParticleData
	{

		bool initialize()
		{
			
			VERIFY_RETURN_FALSE(mProgInit = ShaderManager::Get().getGlobalShaderT< ParticleInitProgram >(true));
			VERIFY_RETURN_FALSE(mProgUpdate = ShaderManager::Get().getGlobalShaderT< ParticleUpdateProgram >(true));

			VERIFY_RETURN_FALSE(mProgParticleRender = ShaderManager::Get().getGlobalShaderT< SimpleSpriteParticleProgram >(true));

			VERIFY_RETURN_FALSE(mParticleBuffer.initializeResource(1000));
			VERIFY_RETURN_FALSE(mInitParamBuffer.initializeResource(1));
			VERIFY_RETURN_FALSE(mParamBuffer.initializeResource(1));

			{
				ParticleParameters& parameters = *mParamBuffer.lock();
				parameters.numParticle = mParticleBuffer.getElementNum();
				parameters.gravity = Vector3(0, 0, -9.8);
				parameters.dynamic = Vector4(1, 1, 1, 0);
				mParamBuffer.unlock();
			}

			{
				ParticleInitParameters& parameters = *mInitParamBuffer.lock();
				parameters.posMax = Vector3(10, 10, 1);
				parameters.posMin = Vector3(-10, -10, -1);
				mInitParamBuffer.unlock();
			}

			{

				{
					CollisionPrimitive primitive;
					primitive.type = 0;
					primitive.param = Vector4(0, 0, 1, 0);
					mPrimitives.push_back(primitive);
				}
				{
					CollisionPrimitive primitive;
					primitive.type = 1;
					primitive.param = Vector4(5, 0, 0, 10);
					mPrimitives.push_back(primitive);
				}

				{
					CollisionPrimitive primitive;
					primitive.type = 1;
					primitive.param = Vector4(-5, 0, 5, 15);
					mPrimitives.push_back(primitive);
				}
				VERIFY_RETURN_FALSE(mCollisionPrimitiveBuffer.initializeResource(mPrimitives.size()));
				CollisionPrimitive* pData = mCollisionPrimitiveBuffer.lock();
				memcpy(pData, &mPrimitives[0], mPrimitives.size() * sizeof(CollisionPrimitive));
				mCollisionPrimitiveBuffer.unlock();

				return true;
			}
		}


		void initParticleData()
		{
			GL_BIND_LOCK_OBJECT(mProgInit);
			mProgInit->setParameters(mParticleBuffer, mParamBuffer, mInitParamBuffer);
			glDispatchCompute(mParticleBuffer.getElementNum(), 1, 1);
		}

		void updateParticleData(float dt)
		{
			GL_BIND_LOCK_OBJECT(mProgUpdate);
			mProgUpdate->setParameters(mParticleBuffer, mParamBuffer, mInitParamBuffer, dt, mPrimitives.size());
			mProgUpdate->setStructuredBufferT<CollisionPrimitive>(*mCollisionPrimitiveBuffer.getRHI());
			glDispatchCompute(mParticleBuffer.getElementNum(), 1, 1);
		}

		TStructuredUniformBuffer< ParticleInitParameters > mInitParamBuffer;
		TStructuredUniformBuffer< ParticleParameters >     mParamBuffer;
		TStructuredStorageBuffer< ParticleData > mParticleBuffer;
		TStructuredUniformBuffer< CollisionPrimitive > mCollisionPrimitiveBuffer;

		ParticleInitProgram* mProgInit;
		ParticleUpdateProgram* mProgUpdate;
		SimpleSpriteParticleProgram* mProgParticleRender;

		std::vector< CollisionPrimitive > mPrimitives;
	};



	class GPUParticleTestStage : public StageBase
		                       , public GPUParticleData
	{
		typedef StageBase BaseClass;
	public:
		GPUParticleTestStage() {}



		int mIndexWaterBufferUsing = 0;
		TStructuredStorageBuffer< WaterVertexData > mWaterDataBuffers[2];
		WaterSimulationProgram* mProgWaterSimulation;
		WaterUpdateNormalProgram* mProgWaterUpdateNormal;
		WaterProgram* mProgWater;
		

		RHITexture2DRef mTexture;
		RHITexture2DRef mBaseTexture;
		RHITexture2DRef mNormalTexture;
		ViewFrustum mViewFrustum;
		SimpleCamera  mCamera;

		Mesh mTileMesh;
		int  mTileNum = 500;
		Mesh mCube;

		
		float mWaterSpeed = 10;

		Mesh mTilePlane;

		int TessFactor1 = 5;
		int TessFactor2 = 1;
		int TessFactor3 = 1;
		float mDispFactor = 1.0;

		SplineProgram* mProgSpline;
		bool bUseTessellation = true;
		bool bWireframe = false;
		virtual bool onInit()
		{
			if( !BaseClass::onInit() )
				return false;

			if( !::Global::GetDrawEngine()->startOpenGL() )
				return false;

			VERIFY_RETURN_FALSE(GPUParticleData::initialize());
			
			VERIFY_RETURN_FALSE(mProgSpline = ShaderManager::Get().getGlobalShaderT< SplineProgram >(true));

			VERIFY_RETURN_FALSE(mTexture = RHIUtility::LoadTexture2DFromFile("Texture/star.png"));
			VERIFY_RETURN_FALSE(mBaseTexture = RHIUtility::LoadTexture2DFromFile("Texture/stones.bmp"));
			VERIFY_RETURN_FALSE(mNormalTexture = RHIUtility::LoadTexture2DFromFile("Texture/stones_NM_height.tga"));

			VERIFY_RETURN_FALSE(mTexture.isValid());
			VERIFY_RETURN_FALSE(ShaderManager::Get().loadFileSimple(mProgSphere, "Shader/Game/Sphere"));
			Vector3 v[] =
			{
				Vector3(1,1,0),
				Vector3(-1,1,0) ,
				Vector3(-1,-1,0),
				Vector3(1,-1,0),
			};
			int   idx[6] = { 0 , 1 , 2 , 0 , 2 , 3 };
			mSpherePlane.mInputLayoutDesc.addElement(Vertex::ePosition, Vertex::eFloat3);
			VERIFY_RETURN_FALSE(mSpherePlane.createRHIResource(&v[0], 4, &idx[0], 6, true));

			VERIFY_RETURN_FALSE(MeshBuild::Tile(mTileMesh, mTileNum - 1, 100, false));
			VERIFY_RETURN_FALSE(MeshBuild::Tile(mTilePlane, 4, 1, false));
			VERIFY_RETURN_FALSE(MeshBuild::CubeShare(mCube,1));

			VERIFY_RETURN_FALSE(mWaterDataBuffers[0].initializeResource(mTileNum * mTileNum));
			VERIFY_RETURN_FALSE(mWaterDataBuffers[1].initializeResource(mTileNum * mTileNum));
			VERIFY_RETURN_FALSE(mProgWaterSimulation = ShaderManager::Get().getGlobalShaderT< WaterSimulationProgram >(true));
			VERIFY_RETURN_FALSE(mProgWaterUpdateNormal = ShaderManager::Get().getGlobalShaderT< WaterUpdateNormalProgram >(true));
			VERIFY_RETURN_FALSE(mProgWater = ShaderManager::Get().getGlobalShaderT< WaterProgram >(true));
			{
				auto pWaterData = mWaterDataBuffers[0].lock();
				for( int i = 0; i < mTileNum * mTileNum; ++i )
				{
					int x = i % mTileNum;
					int y = i / mTileNum;
					pWaterData[i].h = 0;
					pWaterData[i].v = 0;

					float dist = Distance(Vector2(x, y), 0.5 * Vector2(mTileNum, mTileNum));
#if 0
					if( dist < 30 )
					{
						pWaterData[i].h = 10 * ( 30 - dist ) / 30 ;
					}
#else
					pWaterData[i].h = 10 * Math::Exp(-Math::Squre(dist / 10));
#endif
					//pWaterData[i].normal = Vector3(0, 0, 1);
				}
				mWaterDataBuffers[0].unlock();
			}


			Vec2i screenSize = ::Global::GetDrawEngine()->getScreenSize();
			mViewFrustum.mNear = 0.01;
			mViewFrustum.mFar = 800.0;
			mViewFrustum.mAspect = float(screenSize.x) / screenSize.y;
			mViewFrustum.mYFov = Math::Deg2Rad(60 / mViewFrustum.mAspect);

			mCamera.setPos(Vector3(20, 0, 20));
			mCamera.setViewDir(Vector3(-1, 0, 0), Vector3(0, 0, 1));
			::Global::GUI().cleanupWidget();

			auto devFrame = WidgetUtility::CreateDevFrame();
			devFrame->addText("Water Speed");
			auto slider = devFrame->addSlider(UI_ANY);
			WidgetPropery::Bind(slider, mWaterSpeed, 0, 10);
			devFrame->addText("TessFactor");
			WidgetPropery::Bind(devFrame->addSlider(UI_ANY), TessFactor1, 0, 70);
			WidgetPropery::Bind(devFrame->addSlider(UI_ANY), TessFactor2, 0, 70);
			WidgetPropery::Bind(devFrame->addSlider(UI_ANY), TessFactor3, 0, 70);
			WidgetPropery::Bind(devFrame->addCheckBox(UI_ANY, "UseTess"), bUseTessellation);
			WidgetPropery::Bind(devFrame->addCheckBox(UI_ANY, "WireFrame"), bWireframe);
			devFrame->addText("DispFactor");
			WidgetPropery::Bind(devFrame->addSlider(UI_ANY), mDispFactor, 0, 10);
			restart();

			return true;
		}

		virtual void onEnd()
		{
			BaseClass::onEnd();
		}

		void drawSphere(Vector3 const& pos, float radius)
		{
			RHISetBlendState(TStaticBlendState<>::GetRHI());

			GL_BIND_LOCK_OBJECT(mProgSphere);
			mView.setupShader(mProgSphere);
			mProgSphere.setParam(SHADER_PARAM(Sphere.radius), radius);
			mProgSphere.setParam(SHADER_PARAM(Sphere.localPos), pos);
			
			mSpherePlane.drawShader();
		}

		void upateWaterData(float dt)
		{
			mIndexWaterBufferUsing = 1 - mIndexWaterBufferUsing;
			{
				GL_BIND_LOCK_OBJECT(mProgWaterSimulation);
				Vector4 waterParam;
				waterParam.x = dt;
				waterParam.y = mWaterSpeed * dt * mTileNum / 10;
				waterParam.z = 1;
				mProgWaterSimulation->setParameters(waterParam, mTileNum, *mWaterDataBuffers[1 - mIndexWaterBufferUsing].getRHI(), *mWaterDataBuffers[mIndexWaterBufferUsing].getRHI());
				glDispatchCompute(mTileNum, mTileNum, 1);
			}
			{
				GL_BIND_LOCK_OBJECT(mProgWaterUpdateNormal);
				mProgWaterUpdateNormal->setParameters(mTileNum, *mWaterDataBuffers[mIndexWaterBufferUsing].getRHI());
				glDispatchCompute(mTileNum, mTileNum, 1);

			}
		}
		void restart() 
		{
			initParticleData();
		}
		void tick() {}
		void updateFrame(int frame) {}

		virtual void onUpdate(long time)
		{
			BaseClass::onUpdate(time);

			int frame = time / gDefaultTickTime;
			for( int i = 0; i < frame; ++i )
				tick();

			float dt = float(time) / 1000;
			updateParticleData(dt);
			upateWaterData(dt);

			updateFrame(frame);
		}

		ShaderProgram mProgSphere;
		Mesh mSpherePlane;

		ShaderProgram mProgSimpleParticle;
		ViewInfo mView;



		void onRender(float dFrame)
		{
			Graphics2D& g = Global::GetGraphics2D();


			GameWindow& window = Global::GetDrawEngine()->getWindow();

			mView.gameTime = 0;
			mView.realTime = 0;
			mView.rectOffset = IntVector2(0, 0);
			mView.rectSize = IntVector2(window.getWidth(), window.getHeight());

			Matrix4 matView = mCamera.getViewMatrix();
			mView.setupTransform(matView, mViewFrustum.getPerspectiveMatrix());


			glMatrixMode(GL_PROJECTION);
			glLoadMatrixf(mView.viewToClip);
			glMatrixMode(GL_MODELVIEW);
			glLoadMatrixf(mView.worldToView);

			glClearColor(0.2, 0.2, 0.2, 1);
			glClearDepth(1.0);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

			RHISetViewport(mView.rectOffset.x, mView.rectOffset.y, mView.rectSize.x, mView.rectSize.y);
			RHISetDepthStencilState(TStaticDepthStencilState<>::GetRHI());
			RHISetBlendState(TStaticBlendState<>::GetRHI());

			{
				GL_BIND_LOCK_OBJECT(mProgWater);
				mProgWater->setParameters(mTileNum, *mWaterDataBuffers[mIndexWaterBufferUsing].getRHI());
				mView.setupShader(*mProgWater);
				//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
				mTileMesh.drawShader(LinearColor(1, 0, 0));
				glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			}

			for( auto const& primitive : mPrimitives )
			{
				switch( primitive.type )
				{
				case 0:
					break;
				case 1:
					drawSphere(primitive.param.xyz(), primitive.param.w);
					break;
				}
			}


			{
				glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

				RHISetBlendState(TStaticBlendState< CWM_RGBA, Blend::eSrcAlpha, Blend::eOne >::GetRHI());
				GL_BIND_LOCK_OBJECT(mProgParticleRender);
				mView.setupShader(*mProgParticleRender);
				mProgParticleRender->setParameters(mParticleBuffer, *mTexture);
				glDrawArrays(GL_POINTS , 0, mParticleBuffer.getElementNum() - 2);
			}



			{
				int width = ::Global::GetDrawEngine()->getScreenWidth();
				int height = ::Global::GetDrawEngine()->getScreenHeight();

				RHISetDepthStencilState(StaticDepthDisableState::GetRHI());
				RHISetBlendState(TStaticBlendState<>::GetRHI());

				struct MyVertex
				{
					Vector2 pos;
					Vector3 color;
				};

				MyVertex vertices[] =
				{
					Vector2(100, 100), Vector3(1, 0, 0),
					Vector2(400, 100), Vector3(0, 1, 0),
					Vector2(100, 400),	Vector3(0, 0, 1),
					Vector2(400, 400), Vector3(1, 1, 1),
				};
				GL_BIND_LOCK_OBJECT(mProgSpline);
				mProgSpline->setParam(SHADER_PARAM(XForm), OrthoMatrix(0, width, 0, height, -100, 100));
				mProgSpline->setParam(SHADER_PARAM(TessOuter0), TessFactor2);
				mProgSpline->setParam(SHADER_PARAM(TessOuter1), TessFactor1);
				glPatchParameteri(GL_PATCH_VERTICES, 4);
				TRenderRT< RTVF_XY | RTVF_C > ::DrawShader( SplineProgram::UseTesselation ? PrimitiveType::Patchs : PrimitiveType::LineStrip , vertices, 4);
	
			}

			{
				int width = ::Global::GetDrawEngine()->getScreenWidth();
				int height = ::Global::GetDrawEngine()->getScreenHeight();

				RHISetDepthStencilState(TStaticDepthStencilState<>::GetRHI());
				RHISetBlendState(TStaticBlendState<>::GetRHI());
				if( bWireframe )
				{
					RHISetRasterizerState(TStaticRasterizerState<ECullMode::Back, EFillMode::Wireframe>::GetRHI());
				}

				ShaderProgram* program;
				if( bUseTessellation )
				{
					program = ShaderManager::Get().getGlobalShaderT< TTessellationProgram<true> >();
				}
				else
				{
					program = ShaderManager::Get().getGlobalShaderT< TTessellationProgram<false> >();
				}
				GL_BIND_LOCK_OBJECT(program);

				Matrix4 localToWorld = Matrix4::Scale(10) * Matrix4::Translate( -20 , - 20 , 10 );
				Matrix4 worldToLocal;
				float det;
				localToWorld.inverseAffine(worldToLocal, det);
				program->setParam(SHADER_PARAM(XForm), OrthoMatrix(0, width, 0, height, -100, 100));
				program->setTexture(SHADER_PARAM(BaseTexture), *mBaseTexture);
				program->setParam(SHADER_PARAM(Primitive.localToWorld), localToWorld );
				program->setParam(SHADER_PARAM(Primitive.worldToLocal), worldToLocal );

				mView.setupShader(*program);
				if ( bUseTessellation )
				{
					program->setParam(SHADER_PARAM(TessOuter), TessFactor1);
					program->setParam(SHADER_PARAM(TessInner), TessFactor2);
					program->setTexture(SHADER_PARAM(DispTexture), *mNormalTexture);
					program->setParam(SHADER_PARAM(DispFactor), mDispFactor);
					glPatchParameteri(GL_PATCH_VERTICES, 3);
				}

#if 1
				struct MyVertex
				{
					Vector3 pos;
					Vector3 color;
					Vector3 normal;
					Vector2 uv;
				};


				MyVertex vertices[] =
				{
					Vector3(-1, -1,0), Vector3(1, 0, 0), Vector3(-1, -1,1 ).getNormal() , Vector2(0,0),
					Vector3(1, -1,0), Vector3(0, 1, 0),   Vector3(1, -1,1).getNormal() ,Vector2(1,0),
					Vector3(-1, 1,0),	Vector3(0, 0, 1),   Vector3(-1, 1,1).getNormal() ,Vector2(0,1),
					Vector3(1, 1,0),	Vector3(1, 1, 1),   Vector3(1, 1,1).getNormal() ,Vector2(1,1),
				};

				int indices[] = { 0 , 1 , 2 , 1 , 3 , 2 };

				TRenderRT< RTVF_XYZ_C_N_T2 > ::DrawIndexedShader( 
					bUseTessellation ? PrimitiveType::Patchs : PrimitiveType::TriangleList, 
					vertices, ARRAY_SIZE(vertices) , indices , ARRAY_SIZE(indices) );
#else
				//mTilePlane.drawTessellation();
				mCube.drawTessellation();
#endif
				RHISetRasterizerState(TStaticRasterizerState<>::GetRHI());

			}

			g.beginRender();

			g.endRender();
		}




		bool onMouse(MouseMsg const& msg)
		{
			static Vec2i oldPos = msg.getPos();

			if( msg.onLeftDown() )
			{
				oldPos = msg.getPos();
			}
			if( msg.onMoving() && msg.isLeftDown() )
			{
				float rotateSpeed = 0.01;
				Vector2 off = rotateSpeed * Vector2(msg.getPos() - oldPos);
				mCamera.rotateByMouse(off.x, off.y);
				oldPos = msg.getPos();
			}

			if( !BaseClass::onMouse(msg) )
				return false;
			return true;
		}

		bool onKey(unsigned key, bool isDown)
		{
			if( !isDown )
				return false;
			switch( key )
			{
			case 'W': mCamera.moveFront(1); break;
			case 'S': mCamera.moveFront(-1); break;
			case 'D': mCamera.moveRight(1); break;
			case 'A': mCamera.moveRight(-1); break;
			case 'Z': mCamera.moveUp(0.5); break;
			case 'X': mCamera.moveUp(-0.5); break;
			case Keyboard::eLEFT: --TessFactor1; break;
			case Keyboard::eRIGHT: ++TessFactor1; break;
			case Keyboard::eDOWN: --TessFactor2; break;
			case Keyboard::eUP: ++TessFactor2; break;
			case Keyboard::eR: restart(); break;
			case Keyboard::eF2:
				{
					ShaderManager::Get().reloadAll();
					//initParticleData();
				}
				break;
			}
			return false;
		}

		virtual bool onWidgetEvent(int event, int id, GWidget* ui) override
		{
			switch( id )
			{
			default:
				break;
			}

			return BaseClass::onWidgetEvent(event, id, ui);
		}
	protected:
	};

	REGISTER_STAGE2("GPU Particle Test", GPUParticleTestStage, EStageGroup::FeatureDev, 1);
}
