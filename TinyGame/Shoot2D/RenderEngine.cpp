#include "RenderEngine.h"

#include "Common.h"
#include "Object.h"
#include "Weapon.h"
#include "ObjModel.h"
#include "BitmapDC.h"
#include "ResourcesLoader.h"

#include "GameGlobal.h"
#include "DrawEngine.h"
#include "RenderUtility.h"

namespace Shoot2D
{
	RenderEngine::RenderEngine( HDC hDC ) 
		:mhDC( hDC )
	{

	}

	void RenderEngine::draw( unsigned id )
	{
		ObjModelManger& manger = ObjModelManger::Get();

		Graphics2D& g = ::Global::GetGraphics2D(); 

		ObjModel const& model = manger.getModel(id);


		if ( id == MD_SMALL_BOW ||
			 id == MD_BIG_BOW )
		{
			RenderUtility::SetBrush( g , EColor::Red );
		}
		else
		{
			RenderUtility::SetBrush( g , EColor::White );
		}

		if ( model.geomType == GEOM_RECT )
		{
			int x = model.x / 2 ;
			int y = model.y / 2 ;
			
			g.drawRect( Orgx - x ,Orgy - y ,Orgx + x ,Orgy + y );
		}
		else if( model.geomType == GEOM_CIRCLE )
		{
			int r = model.r;
			RenderUtility::SetBrush( g , EColor::Red );
			g.drawCircle( Vec2i( Orgx , Orgy ) , r );
		}
	}

	void RenderEngine::draw( Object* obj )
	{
		Vec2D pos = getCenterPos( *obj );
		SetDrawOrg( pos.x , pos.y );

		ModelId id = obj->getModelId();

		BmpData* bmp = ResourcesLoader::Get().getBmp( id );

		if ( bmp )
		{
			int x = 0;
			int y = 0;
			bool inv = false;

			if ( id == MD_BASE || 
				 id == MD_PLAYER || 
				 id == MD_BULLET_1)
			{
				Vehicle* veh = (Vehicle*) obj;
				//Weapon* w = veh->getWeapon();
				//if ( w->getStats() ==Weapon::WS_FIRING )
				//	y = 1;
				x = obj->getDir();
				if ( x > 16 )
				{
					x = ( 32 - x ) % 32;
					inv = true;
				}
			}
			else if ( id == MD_SMALL_BOW )
			{
				x = ( obj->getFrameCount()/2 ) % 8;
			}
			else if ( id == MD_BIG_BOW )
			{
				int pos = ( obj->getFrameCount()/2 ) % 8;
				x = pos % 4;
				y = pos / 4;
			}

			draw( *bmp , x , y , inv );
			return;

		}


		draw( obj->getModelId() );
	}

	void RenderEngine::draw( BmpData& data , int x ,int y , bool invDir )
	{
		Graphics2D& g = ::Global::GetGraphics2D(); 

		int w = data.w;
		int h = data.h;

		if ( invDir )
		{
			BitmapDC tempDC( mhDC , w , h );
			StretchBlt( tempDC.getDC() , w , 0 , - w - 1 , h ,
				data.bmpDC.getDC() ,  w * x  , h * y , w , h , SRCCOPY );
			TransparentBlt( mhDC , Orgx - w/2 , Orgy - h/2 , w , h ,
				tempDC.getDC() , 0 , 0 , w , h , RGB(0,255,0) );

		}
		else
		{
			TransparentBlt( mhDC , Orgx - w/2 , Orgy - h/2 , w , h ,
				data.bmpDC.getDC() , w * x , h * y , w , h , RGB(0,255,0) );
		}
	}

	void RenderEngine::beginRender()
	{
		Graphics2D& g = ::Global::GetGraphics2D();
		RenderUtility::SetBrush( g , EColor::White );
		g.drawRect( Vec2i(0,0) , ::Global::GetDrawEngine()->getScreenSize() );

	}

	void RenderEngine::endRender()
	{

	}

}//namespace Shoot2D
