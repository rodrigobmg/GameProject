#pragma once
#ifndef QAGame_H_0A8F83C6_A73E_4468_8F95_7E4DEC6765A7
#define QAGame_H_0A8F83C6_A73E_4468_8F95_7E4DEC6765A7

#include "GameModule.h"


class QuadAssaultModule : public IGameModule
{

public:
	virtual void deleteThis() override;


	virtual char const* getName() override;

	virtual GameController& getController() override;

	virtual bool getAttribValue(AttribValue& value) override;
	virtual void beginPlay(StageModeType type, StageManager& manger) override;
	virtual void endPlay() override;

};

#endif // QAGame_H_0A8F83C6_A73E_4468_8F95_7E4DEC6765A7
