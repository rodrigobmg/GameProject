#ifndef DrawEngine_h__
#define DrawEngine_h__

#include "GameConfig.h"
#include "BitmapDC.h"

#include "Math/Vector2.h"
#include "glew/GL/glew.h"
#include "WGLContext.h"
#include "WinGDIRenderSystem.h"


#include <memory>

using Math::Vector2;


#define OPENGL_RENDER_DIB 0

class Graphics2D : public WinGdiGraphics2D
{
public:
	Graphics2D( HDC hDC = NULL ):WinGdiGraphics2D(hDC){}

	using WinGdiGraphics2D::drawPolygon;
	void  drawPolygon(Vector2 pos[], int num)
	{
		Vec2i* pts = (Vec2i*)alloca(sizeof(Vec2i) * num);
		for( int i = 0; i < num; ++i )
		{
			pts[i].x = Math::FloorToInt(pos[i].x);
			pts[i].y = Math::FloorToInt(pos[i].y);
		}
		drawPolygon(pts, num);
	}
};

class GLGraphics2D;


namespace Render
{
	class RHIGraphics2D;
}


class IGraphics2D
{
public:
	virtual void  beginRender() = 0;
	virtual void  endRender() = 0;
	virtual void  beginClip(Vec2i const& pos, Vec2i const& size) = 0;
	virtual void  endClip() = 0;
	virtual void  beginBlend( Vec2i const& pos , Vec2i const& size , float alpha ) = 0;
	virtual void  endBlend() = 0;
	virtual void  setPen( Color3ub const& color ) = 0;
	virtual void  setBrush( Color3ub const& color ) = 0;
	virtual void  drawPixel  (Vector2 const& p , Color3ub const& color )= 0;
	virtual void  drawLine   (Vector2 const& p1 , Vector2 const& p2 ) = 0;

	virtual void  drawRect   (Vector2 const& pos , Vector2 const& size ) = 0;
	virtual void  drawCircle (Vector2 const& center , float radius ) = 0;
	virtual void  drawEllipse(Vector2 const& pos , Vector2 const& size ) = 0;
	virtual void  drawRoundRect(Vector2 const& pos , Vector2 const& rectSize , Vector2 const& circleSize ) = 0;
	virtual void  drawPolygon(Vector2 pos[], int num) = 0;
	virtual void  setTextColor(Color3ub const& color) = 0;
	virtual void  drawText(Vector2 const& pos , char const* str ) = 0;
	virtual void  drawText(Vector2 const& pos , Vector2 const& size , char const* str , bool beClip = false ) = 0;

	void  drawRect(int left, int top, int right, int bottom)
	{
		drawRect(Vector2(left, top), Vector2(right - left, bottom - right));
	}
	class Visitor
	{
	public:
		virtual void visit( Graphics2D& g ) = 0;
		virtual void visit( GLGraphics2D& g ) = 0;
		virtual void visit( Render::RHIGraphics2D& g) = 0;
	};
	virtual void  accept( Visitor& visitor ) = 0;
};


class RenderOverlay
{
public:
	RenderOverlay()
	{

	}

	Graphics2D* createGraphics()
	{
		return nullptr;

	}
	void  destroyGraphics(){}


	void  render()
	{

	}

	Vec2i mRectMin;
	Vec2i mRectMax;
	HDC   mhDC;
};

class TINY_API GameWindow : public WinFrameT< GameWindow >
{
public:
	WORD   getIcon();
	WORD   getSmallIcon();
};

enum class RHITargetName
{
	None   ,
	OpenGL ,
	D3D11 ,
	Vulkan ,
};
class DrawEngine
{
public:
	TINY_API DrawEngine();
	TINY_API ~DrawEngine();

	TINY_API void  init( GameWindow& window );
	TINY_API void  release();
	TINY_API void  changeScreenSize( int w , int h );
	TINY_API void  update(long deltaTime);

	Vec2i         getScreenSize(){ return Vec2i( mBufferDC.getWidth() , mBufferDC.getHeight() ); }
	int           getScreenWidth(){ return mBufferDC.getWidth(); }
	int           getScreenHeight(){ return mBufferDC.getHeight(); }
	Graphics2D&   getScreenGraphics(){ return *mPlatformGraphics; }
	GLGraphics2D& getGLGraphics(){ return *mGLGraphics; }

	TINY_API IGraphics2D&  getIGraphics();

	HFONT       createFont( int size , char const* faceName , bool beBold , bool beItalic );
	WindowsGLContext*    getGLContext(){ return mGLContext; }
	
	GameWindow& getWindow(){ return *mGameWindow; }

	TINY_API void drawProfile(Vec2i const& pos);


	bool isRHIEnabled() { return mRHIName != RHITargetName::None; }
	bool isOpenGLEnabled(){ return mRHIName == RHITargetName::OpenGL; }
	bool isInitialized() { return mbInitialized; }

	TINY_API bool  initializeRHI(RHITargetName targetName , int numSamples);
	TINY_API void  shutdownRHI(bool bDeferred);
	TINY_API bool  startOpenGL( int numSamples = 1 );
	TINY_API void  stopOpenGL(bool bDeferred = false);
	TINY_API bool  beginRender();
	TINY_API void  endRender();

	bool        bUsePlatformBuffer = true;
private:
	void        setupBuffer( int w , int h );

	bool        mbInitialized;
	bool        bRHIShutdownDeferred;
	
	GameWindow* mGameWindow;
	BitmapDC    mBufferDC;
	std::unique_ptr< Graphics2D > mPlatformGraphics;

	RHITargetName mRHIName = RHITargetName::None;
	WindowsGLContext*  mGLContext = nullptr;
	std::unique_ptr< GLGraphics2D >   mGLGraphics;
	std::unique_ptr< Render::RHIGraphics2D > mRHIGraphics;

};

class RenderSurface
{
public:
	RenderSurface()
		:mScenePos(0,0)
		,mScale( 1.0 )
	{

	}

	void         setSurfaceScale( float scale ){  mScale = scale; }
	void         setSurfacePos( Vec2i const& pos ) { mScenePos = pos; }
	Vec2i const& getSurfacePos() const { return mScenePos; }
	float        getSurfaceScale(){ return mScale; }
protected:
	Vec2i        mScenePos;
	float        mScale;
};



#endif // DrawEngine_h__