#pragma once
#ifndef GpuProfiler_H_5CF3071A_820F_435C_BC97_1975A2C6D546
#define GpuProfiler_H_5CF3071A_820F_435C_BC97_1975A2C6D546

#include "Singleton.h"
#include "Core/IntegerType.h"

#include <unordered_map>
#include <memory>

namespace Render
{

	class RHIProfileCore
	{
	public:
		virtual ~RHIProfileCore(){}

		virtual void beginFrame() = 0;
		virtual void endFrame() = 0;
		virtual uint32 fetchTiming() = 0;

		virtual void startTiming(uint32 timingHandle) = 0;
		virtual void endTiming(uint32 timingHandle) = 0;
		virtual bool getTimingDurtion(uint32 timingHandle, uint64& outDurtion) = 0;
		virtual double getCycleToMillisecond() = 0;
	};

	struct GpuProfileSample
	{
		std::string name;
		int idxGroup;
		int level;

		uint32 timingHandle;
		float time;
	};

	struct GpuProfiler
	{
	public:
		GpuProfiler();

		static CORE_API GpuProfiler& Get();

		CORE_API void beginFrame();
		CORE_API void endFrame();

		CORE_API GpuProfileSample* startSample(char const* name);
		CORE_API void endSample(GpuProfileSample& sample);

		int  getSampleNum() { return mNumSampleUsed; }
		void setCore(RHIProfileCore* core);

		GpuProfileSample* getSample(int idx) { return mSamples[idx].get(); }


		struct SampleGroup
		{
			std::string name;
			float time;
			int idxParent;
		};
		
		std::vector< std::unique_ptr< GpuProfileSample > > mSamples;

		RHIProfileCore* mCore = nullptr;
		GpuProfileSample* mRootSample;
		bool   mbStartSampling = false;
		int    mCurLevel;
		int    mNumSampleUsed;
		double mCycleToSecond;
	};


	struct GpuProfileScope
	{
		struct NoVA {};
		GpuProfileScope(NoVA, char const* name);
		GpuProfileScope(char const* format, ...);
		~GpuProfileScope();

		GpuProfileSample* sample;
	};

}//namespace Render

#define GPU_PROFILE( name ) Render::GpuProfileScope ANONYMOUS_VARIABLE(GPUProfile)( Render::GpuProfileScope::NoVA() , name );
#define GPU_PROFILE_VA( name , ... ) Render::GpuProfileScope ANONYMOUS_VARIABLE(GPUProfile)( name , __VA_ARGS__);

#endif // GpuProfiler_H_5CF3071A_820F_435C_BC97_1975A2C6D546
