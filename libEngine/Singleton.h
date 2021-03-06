#ifndef Singleton_h__
#define Singleton_h__

#include "CppVersion.h"
#include "CoreShare.h"

template< class T >
class SingletonT
{
public:
	static T& Get()
	{
		if ( !_instance )
			_instance = new T;
		return *_instance;
	}

	static void ReleaseInstance()
	{
		if ( _instance )
		{
			delete _instance;
			_instance = 0;
		}
	}
protected:
	SingletonT() {}

	FUNCTION_DELETE(SingletonT(SingletonT const& sing));
	FUNCTION_DELETE(SingletonT& operator = (SingletonT const& sing));

private:
	static T* _instance;
};

template< class T >
T* SingletonT< T >::_instance = 0;

#endif // Singleton_h__
