#pragma once
#ifndef NeuralNetwork_H_9D9E20F4_5B5B_45BE_A95C_5A2CEB755C3F
#define NeuralNetwork_H_9D9E20F4_5B5B_45BE_A95C_5A2CEB755C3F

#include "Math/Base.h"

#include <cassert>
#include <vector>

typedef double NNScale;
typedef NNScale (*NNActivationFun)(NNScale);

template< class T , class Q , class Fun >
void Transform(T begin, T end, T dest, Fun fun = Fun())
{
	T cur = begin;
	while( cur != end )
	{
		dest = fun(*cur);
		++cur;
		++dest;
	}
}

class NNFun
{
public:
	enum Name
	{
		SigmoidFun ,
		TanhFun ,
		ReLuFun ,
	};

	static NNScale Sigmoid(NNScale value)
	{
		if( value < -10 )
			return 0;
		if( value > 10 )
			return 1;
		return 1.0 / (1.0 + exp(-value));
	}

	static NNScale SigmoidDif(NNScale value)
	{
		if( value < -10 )
			return 0;
		if( value > 10 )
			return 0;
		float v = 1.0 / (1.0 + exp(-value));
		return v * (1 - v);
	}
	static NNScale Tanh(NNScale value)
	{
		if( value < -10 )
			return 0;
		if( value > 10 )
			return 1;

		NNScale expV = exp(value);
		NNScale expVInv = 1 / expV;

		return (expV - expVInv) / (expV + expVInv);
	}

	static NNScale TanhDif(NNScale value)
	{
		if( value < -10 )
			return 0;
		if( value > 10 )
			return 0;

		NNScale expV = exp(value);
		NNScale expVInv = 1 / expV;
		return (expV - expVInv) / (expV + expVInv);
	}

	static NNScale ReLU(NNScale value)
	{
		return (value > 0) ? value : 0;
	}

	static NNScale ReLUDif(NNScale value)
	{
		return (value > 0) ? 1 : 0;
	}

};


struct NeuralLayer
{
	int weightOffset;
	NNActivationFun fun;
	NNActivationFun funDif;
	int numNode;
	NeuralLayer()
	{
		numNode = 0;
		weightOffset = 0;
		fun = NNFun::Sigmoid;
		funDif = NNFun::SigmoidDif;
		//fun = NNFun::ReLU;
	}
};

struct NeuralFullConLayer : NeuralLayer
{
	NeuralFullConLayer()
	{

	}
};

struct NeuralConv2DLayer : NeuralLayer
{
	int convSize;
	int dataSize[2];

	NeuralConv2DLayer()
	{
		fun = NNFun::ReLU;
	}
};

struct NerualPool2DLayer : NeuralLayer
{
	int PoolSize;


};

class FNNCalc
{
public:
	static void LayerFrontFeedback(NeuralFullConLayer& layer, NNScale weightData[], int numInput, NNScale inputs[], NNScale outputs[]);
	static void LayerFrontFeedback(NeuralFullConLayer& layer, NNScale weightData[], int numInput, NNScale inputs[], NNScale outputs[], NNScale outNetworkInputs[]);
	static void LayerFrontFeedback(NeuralConv2DLayer& layer, NNScale weightData[], int numSliceInput, int inputSize[], NNScale inputs[], NNScale outputs[]);


	static FORCEINLINE NNScale VectorDot(int dim, NNScale* RESTRICT a, NNScale* RESTRICT b)
	{
		NNScale result = 0;
		for( ; dim; --dim )
		{
			result += (*a++) * (*b++);
		}
		return result;
	}

	static FORCEINLINE NNScale AreaConv(int dim, int stride, NNScale* RESTRICT area, NNScale* RESTRICT v)
	{
		NNScale result = 0;
		for( int i = 0; i < dim; ++i )
		{
			result += VectorDot(dim, area, v);
			area += stride;
			v += dim;
		}
		return result;
	}
};

class FCNNLayout
{
public:
	void init(int const topology[], int dimNum);
	void init(int numInput, int numOutput, int numHiddenLayer, int const hiddenlayerNodeNum[]);

	NeuralFullConLayer& getLayer(int idx) { return mLayers[idx]; }

	void getTopology(std::vector<int>& outTopology);


	int getPrevLayerNodeNum(int idxLayer) const;

	int getNetworkInputOffset(int idxLayer);

	int getHiddenLayerNum() const { return mLayers.size() - 1; }
	int getInputNum()  const { return mNumInput; }
	int getOutputNum() const { return mLayers.back().numNode; }
	void setActivationFunction(int idxLayer, NNActivationFun fun)
	{
		assert(fun);
		mLayers[idxLayer].fun = fun;
	}

	int getHiddenNodeNum() const;

	int getMaxLayerNodeNum() const { return mMaxLayerNodeNum; }
	int getWeightNum() const;

	int mMaxLayerNodeNum;
	int mNumInput;
	std::vector< NeuralFullConLayer > mLayers;
};

class FCNeuralNetwork
{
public:
	void init(FCNNLayout& inLayout)
	{
		mLayout = &inLayout;
	}
	NNScale* getWeights(int idxLayer, int idxNode);

	void setWeights(std::vector< NNScale >& weights)
	{
		assert(mLayout->getWeightNum() <= weights.size());
		mWeights = &weights[0];
	}
	
	void calcForwardFeedback(NNScale inputs[], NNScale outputs[]);
	void calcForwardFeedbackSignal(NNScale inputs[], NNScale outSingnals[]);
	void calcForwardFeedbackSignal(NNScale inputs[], NNScale outSingnals[], NNScale outNetworkInputs[]);
	FCNNLayout& getLayout() { return *mLayout; }
private:
	
	FCNNLayout* mLayout = nullptr;
	NNScale* mWeights;	
};

class Conv2DNeuralNetwork
{

};


class NeuralPiplineNode
{


};

#endif // NeuralNetwork_H_9D9E20F4_5B5B_45BE_A95C_5A2CEB755C3F
