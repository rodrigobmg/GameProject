
template< class RetType TEMP_ARG >
class DelegateN
{
public:
	DelegateN(){ mPtr = 0;  }
	template < class OBJ >
	DelegateN( OBJ* obj , RetType (OBJ::*fun)( SIG_ARG ) ){  bind( obj , fun );  }
	template < class OBJ >
	DelegateN( OBJ* obj )                                 {  bind( obj );  }
	DelegateN( RetType (*fun)( SIG_ARG ) )                {  bind( fun );  }

	template < class OBJ >
	void bind( OBJ* obj , RetType (OBJ::*fun)( SIG_ARG ) ){  new ( mStorage ) MemFun< OBJ >( obj , fun );  }
	template < class OBJ >
	void bind( OBJ* obj )                                 {  new ( mStorage ) OpFun< OBJ >( obj );  }
	void bind( RetType (*fun)( SIG_ARG ) )                {  new ( mStorage ) BaseFun( fun );  }

public:
	RetType  operator()( FUN_ARG ){ return  castFunPtr()->exec( PARAM_ARG ); }

private:
	struct Fun
	{
	public:
		virtual RetType exec( FUN_ARG ) = 0;
	};
	Fun*     castFunPtr(){ return reinterpret_cast< Fun* >( mStorage ); }

	template < class OBJ >
	struct MemFun : public Fun
	{
		typedef RetType (OBJ::*FunType)( SIG_ARG );
		MemFun( OBJ* obj , FunType fun ):obj(obj),fun(fun){}
		virtual RetType exec( FUN_ARG ){  return ( obj->*fun )( PARAM_ARG );  }
		OBJ*    obj;
		FunType fun;
	};

	struct BaseFun : public Fun
	{
		typedef RetType (*FunType)( SIG_ARG );
		BaseFun( FunType fun ):fun(fun){}
		virtual RetType exec( FUN_ARG ){  return ( *fun )( PARAM_ARG );  }
		FunType fun;
	};

	template < class OBJ >
	struct OpFun : public Fun
	{
		OpFun( OBJ* obj ):obj(obj){}
		virtual RetType exec( FUN_ARG ){  return ( *obj)( PARAM_ARG );  }
		OBJ* obj;
	};

	bool  empty(){  return mPtr != 0;  }
	void  clear(){  mPtr = NULL;  }
	union
	{
		void*  mPtr;
		char   mStorage[ sizeof( void* ) * 4 ];
	};
};


//template< class RetType TEMP_ARG >
//class Delegate< RetType ( SIG_ARG ) > : public DelegateN< RetType TEMP_ARG2 >
//{
//	typedef DelegateN< RetType , SIG_ARG > BaseClass;
//public:
//	DelegateN():BaseClass(){}
//	template < class OBJ >
//	DelegateN( OBJ* obj , RetType (OBJ::*fun)( SIG_ARG ) ):BaseClass( obj , fun ){}
//	template < class OBJ >
//	DelegateN( OBJ* obj ):BaseClass( obj ){}
//	DelegateN( RetType (*fun)( SIG_ARG ) ):BaseClass( fun ){}
//};

#undef DelegateN
#undef PARAM_ARG
#undef TEMP_ARG
#undef TEMP_ARG2
#undef SIG_ARG
#undef FUN_ARG
