#ifndef CppVersion_h__
#define CppVersion_h__

#include "CompilerConfig.h"
#include "CommonMarco.h"

#ifdef CPP_COMPILER_MSVC
#	if ( _MSC_VER >= 1700 ) 
#		define CPP_RVALUE_REFENCE_SUPPORT 1
#	    define CPP_VARIADIC_TEMPLATE_SUPPORT 1
#       define CPP_C11_STDLIB_SUPPORT 1
#       define CPP_STATIC_ASSERT_SUPPORT 1
#		define CPP_CX11_KEYWORD_SUPPORT 1
#       define CPP_TR1_SUPPORT 1
#       define CPP_TYPE_TRAITS_SUPPORT 1
#	elif ( _MSC_VER >= 1600 ) 
#		define CPP_CX11_KEYWORD_SUPPORT 1
#		define CPP_TR1_SUPPORT 1
#	elif ( _MSC_VER == 1500 ) 
#       define CPP_TR1_SUPPORT 1
#   endif
#endif

#ifndef CPP_RVALUE_REFENCE_SUPPORT
#	define CPP_RVALUE_REFENCE_SUPPORT 0
#endif //CPP_RVALUE_REFENCE_SUPPORT

#ifndef CPP_VARIADIC_TEMPLATE_SUPPORT
#	define CPP_VARIADIC_TEMPLATE_SUPPORT 0
#endif

#ifndef CPP_CX11_KEYWORD_SUPPORT
#	define CPP_CX11_KEYWORD_SUPPORT 0
#endif //CPP_CX11_KEYWORD_SUPPORT

#ifndef CPP_C11_STDLIB_SUPPORT
#	define CPP_C11_STDLIB_SUPPORT 0
#endif //CPP_C11_STDLIB_SUPPORT

#ifndef CPP_STATIC_ASSERT_SUPPORT
#	define CPP_STATIC_ASSERT_SUPPORT 0
#endif //CPP_C11_STDLIB_SUPPORT

#ifndef CPP_TR1_SUPPORT
#	define CPP_TR1_SUPPORT 0
#endif

#ifndef CPP_TYPE_TRAITS_SUPPORT
#   define CPP_TYPE_TRAITS_SUPPORT 0
#endif

#if !CPP_CX11_KEYWORD_SUPPORT
#	define override
#	define final
#	define nullptr 0
#endif


#ifdef CPP_COMPILER_MSVC
# define ANONYMOUS_VARIABLE(str) MARCO_NAME_COMBINE_2(str, __COUNTER__)
#else
# define ANONYMOUS_VARIABLE(str) MARCO_NAME_COMBINE_2(str, __LINE__)
#endif 

namespace CppVerPriv
{
	template < bool expr >
	struct StaticAssertSize { enum { Size = 0 }; };
	template <>
	struct StaticAssertSize< true > { enum { Size = 1 }; };
}
//TODO
#if !CPP_STATIC_ASSERT_SUPPORT
#	define static_assert( EXPR , MSG )\
	typedef int ( ANONYMOUS_VARIABLE( STATIC_ASSERT_ )[ ::CppVerPriv::StaticAssertSize< EXPR >::Size ] );
#endif


#endif // CppVersion_h__
