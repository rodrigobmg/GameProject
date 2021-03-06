#include "GameModule.h"

class MiscTestModule : public IModuleInterface
{
public:
	virtual bool initialize() override
	{
		return true;
	}
	virtual void cleanup() override
	{
		
	}
	virtual bool isGameModule() const override
	{
		return false;
	}

	virtual void deleteThis() override
	{
		delete this;
	}
};


EXPORT_MODULE(MiscTestModule);