#ifndef CmtDeviceFactory_h__
#define CmtDeviceFactory_h__

#include "CmtBase.h"
#include "CmtDevice.h"

namespace Chromatron
{
	class DeviceFactory
	{
	public:
		static Device*     Create( DeviceId id , Dir dir , Color color );
		static void        Destroy( Device* dc );
		static DeviceInfo const& GetInfo( DeviceId id );
	};

}//namespace Chromatron

#endif // CmtDeviceFactory_h__
