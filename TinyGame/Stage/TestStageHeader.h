#pragma once

#include "GameConfig.h"

#include "StageBase.h"
#include "StageRegister.h"

#include "GameGUISystem.h"
#include "DrawEngine.h"
#include "RenderUtility.h"
#include "GameModule.h"
#include "GameGlobal.h"

#include "Widget/WidgetUtility.h"

#include "Core/IntegerType.h"
#include "MiscTestRegister.h"


#if 0
class TemplateTestStage : public StageBase
{
	typedef StageBase BaseClass;
public:
	TemplateTestStage() {}

	virtual bool onInit()
	{
		if( !BaseClass::onInit() )
			return false;
		::Global::GUI().cleanupWidget();
		restart();
		return true;
	}

	virtual void onEnd()
	{
		BaseClass::onEnd();
	}

	void restart() {}
	void tick() {}
	void updateFrame(int frame) {}

	virtual void onUpdate(long time)
	{
		BaseClass::onUpdate(time);

		int frame = time / gDefaultTickTime;
		for( int i = 0; i < frame; ++i )
			tick();

		updateFrame(frame);
	}

	void onRender(float dFrame)
	{
		Graphics2D& g = Global::GetGraphics2D();
	}

	bool onMouse(MouseMsg const& msg)
	{
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
		case Keyboard::eR: restart(); break;
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
#endif

