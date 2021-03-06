#pragma once
#ifndef TextureAtlas_H_991594F1_1108_4346_B634_187B185F2B8D
#define TextureAtlas_H_991594F1_1108_4346_B634_187B185F2B8D

#include "RHI/OpenGLCommon.h"

#include "Misc/ImageMergeHelper.h"

namespace Render
{

	class TextureAtlas
	{
	public:
		TextureAtlas();

		bool initialize(Texture::Format format, int w, int h, int border = 0);
		void finalize();

		int  addImageFile(char const* path);
		int  addImage(int w, int h, Texture::Format format, void* data, int pixelStride = 0);
		void getRectUV(int id, Vector2& outMin, Vector2& outMax) const;

		RHITexture2D& getTexture() { return *mTexture; }
		int  getTextureNum() const { return mNextImageId; }


		int              mBorder;
		int              mNextImageId;
		RHITexture2DRef  mTexture;
		ImageMergeHelper mHelper;
	};

}//namespace Render

#endif // TextureAtlas_H_991594F1_1108_4346_B634_187B185F2B8D
