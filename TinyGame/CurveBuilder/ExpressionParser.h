#ifndef ExpressionParser_H
#define ExpressionParser_H

#include <string>
#include <vector>
#include <map>
#include <cmath>
#include <cassert>

using std::string;

class CodeTemplate;
class SymbolTable;
struct SymbolEntry;


typedef double RealType;
enum class ValueLayout
{
	Int32,
	Double,
	Float,

	Real = Double,
};

inline uint32 GetLayoutSize(ValueLayout layout)
{
	switch( layout )
	{
	case ValueLayout::Int32: return sizeof(int32);
	case ValueLayout::Double: return sizeof(double);
	case ValueLayout::Float: return sizeof(float);
	default:
		break;
	}
	return 0;
}
struct FunInfo
{
	static int const MaxArgNum = 8;
	void* ptrFun;
	int   numParam;
	ValueLayout argLayouts[MaxArgNum];
	ValueLayout returnLayout;


	bool isSameLayout(FunInfo const& rhs) const
	{
		if( returnLayout != rhs.returnLayout )
			return false;
		if( numParam == rhs.numParam )
			return false;

		for( int i = 0; i < numParam; ++i )
		{
			if( argLayouts[i] != rhs.argLayouts[i] )
				return false;
		}
		return true;
	}

	bool operator == ( FunInfo const& rhs ) const
	{
		if( ptrFun != rhs.ptrFun )
			return false;

		return true;
	}

#if 0
	template< class RT , class ...Args >
	static FunInfo Get(RT(*fun)(...Args))
	{
	}
#endif
};

struct ConstValueInfo
{
	ValueLayout layout;
	union 
	{
		double toDouble;
		float  toFloat;
		int32  toInt32;
		RealType toReal;
	};

	ConstValueInfo(){}
	ConstValueInfo(RealType value):layout(ValueLayout::Real), toReal(value){}

	bool operator == (ConstValueInfo const& rhs ) const
	{
		if( layout != rhs.layout )
			return false;

		switch( layout )
		{
		case ValueLayout::Int32:  return toInt32 == rhs.toInt32;
		case ValueLayout::Double: return toDouble == rhs.toDouble;
		case ValueLayout::Float:  return toFloat == rhs.toFloat;
		}

		return true;
	}

	bool isSameValue (void* data) const
	{
		switch( layout )
		{
		case ValueLayout::Int32:  return *(int32*)data == toInt32;
		case ValueLayout::Double: return *(double*)data == toDouble;
		case ValueLayout::Float:  return *(float*)data == toFloat;
		}
		return false;
	}

	void assignTo(void* data) const
	{
		switch( layout )
		{
		case ValueLayout::Int32:  *(int32*)data = toInt32;
		case ValueLayout::Double: *(double*)data = toDouble;
		case ValueLayout::Float:  *(float*)data = toFloat;
		}
	}
};

struct VarValueInfo
{
	ValueLayout layout;
	void* ptr;


	void assignTo(void* data) const
	{
		switch( layout )
		{
		case ValueLayout::Int32:  *(int32*)data = *(int32*)ptr;
		case ValueLayout::Double: *(double*)data = *(double*)ptr;
		case ValueLayout::Float:  *(float*)data = *(float*)ptr;
		}
	}
};


typedef RealType (__cdecl *FunType0)();
typedef RealType (__cdecl *FunType1)(RealType);
typedef RealType (__cdecl *FunType2)(RealType,RealType);
typedef RealType (__cdecl *FunType3)(RealType,RealType,RealType);
typedef RealType (__cdecl *FunType4)(RealType,RealType,RealType,RealType);
typedef RealType (__cdecl *FunType5)(RealType,RealType,RealType,RealType,RealType);

class ExprParse
{
public:
	static unsigned const TOKEN_MASK        = 0xff0000;
	static unsigned const SYMBOL_FLAG_BIT   = 0x800000;
	static unsigned const PRECEDENCE_MASK   = 0x001f00;
	static unsigned const REVERSE_BIT       = 0x002000;
	static unsigned const ASSOC_LR_BIT      = 0x004000;
	static unsigned const EXCHANGE_BIT      = 0x008000;

	enum TokenType
	{
		TOKEN_TYPE_ERROR = -1,
		TOKEN_NONE       = 0x000000 ,
		TOKEN_LBAR       = 0x010000 ,
		TOKEN_RBAR       = 0x020000 ,
		TOKEN_BINARY_OP  = 0x030000 ,

		TOKEN_FUN        = 0x040000 ,
		TOKEN_VALUE      = 0x050000 ,
		TOKEN_UNARY_OP   = 0x060000 ,

		IDT_SEPARETOR    = 0x070001 ,

		BOP_COMMA        = 0x000001 | TOKEN_BINARY_OP ,
		BOP_ASSIGN       = 0x000102 | TOKEN_BINARY_OP | ASSOC_LR_BIT ,
		BOP_BIG          = 0x000203 | TOKEN_BINARY_OP ,
		BOP_BIGEQU       = 0x000204 | TOKEN_BINARY_OP ,
		BOP_SML          = 0x000205 | TOKEN_BINARY_OP ,
		BOP_SMLEQU       = 0x000206 | TOKEN_BINARY_OP ,
		BOP_EQU          = 0x000207 | TOKEN_BINARY_OP ,
		BOP_NEQU         = 0x000208 | TOKEN_BINARY_OP ,
		BOP_ADD          = 0x000309 | TOKEN_BINARY_OP | REVERSE_BIT | EXCHANGE_BIT ,
		BOP_SUB          = 0x00030a | TOKEN_BINARY_OP | REVERSE_BIT | EXCHANGE_BIT ,
		BOP_MUL          = 0x00060b | TOKEN_BINARY_OP | REVERSE_BIT | EXCHANGE_BIT ,
		BOP_DIV          = 0x00060c | TOKEN_BINARY_OP | REVERSE_BIT | EXCHANGE_BIT ,
		BOP_POW          = 0x00080d | TOKEN_BINARY_OP ,

		UOP_MINS         = 0x000901 | TOKEN_UNARY_OP | ASSOC_LR_BIT ,
		UOP_PLUS         = 0x000902 | TOKEN_UNARY_OP | ASSOC_LR_BIT ,
		UOP_INC_PRE      = 0x000903 | TOKEN_UNARY_OP ,
		UOP_DEC_PRE      = 0x000904 | TOKEN_UNARY_OP ,

		FUN_DEF          = 0x000a01 | TOKEN_FUN ,

		VALUE_CONST      = 0x000001 | TOKEN_VALUE ,
		VALUE_VARIABLE   = 0x000002 | TOKEN_VALUE ,
		VALUE_INPUT      = 0x000003 | TOKEN_VALUE ,
	};

	inline static int  PrecedeceOrder( TokenType token ){ return token & PRECEDENCE_MASK; }
	inline static bool IsFunction(TokenType token){ return ( token & TOKEN_MASK ) == TOKEN_FUN; }
	inline static bool IsBinaryOperator(TokenType token){ return ( token & TOKEN_MASK ) == TOKEN_BINARY_OP; }
	inline static bool IsValue(TokenType token){ return ( token & TOKEN_MASK ) == TOKEN_VALUE; }
	inline static bool IsUnaryOperator(TokenType token){ return ( token & TOKEN_MASK ) == TOKEN_UNARY_OP; }
	inline static bool IsOperator(TokenType token){ return IsUnaryOperator(token) || IsBinaryOperator(token); }

	inline static bool CanReverse( TokenType token ){ assert( IsBinaryOperator( token ) ); return !!( token & REVERSE_BIT ); }
	inline static bool CanExchange( TokenType token ){ assert( IsBinaryOperator( token ) ); return !!( token & EXCHANGE_BIT ); }
	inline static bool IsAssocLR( TokenType token ){ assert( IsOperator( token ) ); return !!( token & ASSOC_LR_BIT ); }
	
	struct Unit
	{
		Unit(){}
		Unit(TokenType type)
			:type(type),isReverse(false){}
		Unit(TokenType type, ConstValueInfo val)
			:type(type),constValue(val){}
		Unit(TokenType type,SymbolEntry const* symbol)
			:type(type), symbol(symbol){}


		TokenType    type;
		union
		{
			SymbolEntry const* symbol;
			bool            isReverse;
			ConstValueInfo  constValue;
		};
	};

	
	typedef std::vector<Unit> UnitCodes;
	static void print( Unit const& unit , SymbolTable const& table );
	static void print( UnitCodes const& codes , SymbolTable const& table , bool haveBracket );


	enum
	{
		CN_LEFT  = 0,
		CN_RIGHT = 1,
	};

	static int LEAF_UNIT_INDEX( int idxNode ){ return -( idxNode + 1 ); }

	struct Node
	{
		Unit*    opUnit;
		int      parent;
		intptr_t children[2];

		int   getLeaf( int idxChild )
		{ 
			assert( children[ idxChild ] < 0 );
			return LEAF_UNIT_INDEX( children[ idxChild ] ); 
		}
	};
	typedef std::vector< Node > NodeVec;
};


struct SymbolEntry
{
	enum Type
	{
		eFun,
		eConstValue,
		eVar,
		eInputVar,
	};
	Type type;

	union
	{
		uint8          inputIndex;
		FunInfo        fun;
		ConstValueInfo constValue;
		VarValueInfo   varValue;
	};

	SymbolEntry() {}
	SymbolEntry(FunInfo const& funInfo)
		:type(eFun)
		,fun(funInfo)
	{
	}

	SymbolEntry(ConstValueInfo value)
		:type(eConstValue)
		, constValue(value)
	{
	}
	SymbolEntry(RealType* ptr)
		:type(eVar)
	{
		varValue.ptr = ptr;
		varValue.layout = ValueLayout::Real;
	}
	SymbolEntry(uint8 index)
		:type(eInputVar)
		, inputIndex(index)
	{
	}
};

class SymbolTable
{
public:
	// can redefine
	void            defineConst( char const* name ,RealType val )  { mNameToEntryMap[name] = val; }
	void            defineVar( char const* name , RealType* varPtr){  mNameToEntryMap[name] = varPtr; }
	void            defineVarInput(char const* name, uint8 inputIndex) { mNameToEntryMap[name] = inputIndex; }
	void            defineFun( char const* name , FunType0 fun ){  defineFunInternal(name,(void*)fun,0);  }
	void            defineFun( char const* name , FunType1 fun ){  defineFunInternal(name,(void*)fun,1);  }
	void            defineFun( char const* name , FunType2 fun ){  defineFunInternal(name,(void*)fun,2);  }
	void            defineFun( char const* name , FunType3 fun ){  defineFunInternal(name,(void*)fun,3);  }
	void            defineFun( char const* name , FunType4 fun ){  defineFunInternal(name,(void*)fun,4);  }
	void            defineFun( char const* name , FunType5 fun ){  defineFunInternal(name,(void*)fun,5);  }

	ConstValueInfo const* findConst(std::string const& name) const;
	FunInfo const*        findFun(std::string const& name) const;
	VarValueInfo const*   findVar(std::string const& name ) const;
	int             findInput(std::string const& name) const;
	char const*     getFunName( FunInfo const& info ) const;
	char const*     getVarName( void* var ) const;

	bool            isFunDefined(std::string const& name) const{  return isDefinedInternal( name , SymbolEntry::eFun ); }
	bool            isVarDefined(std::string const& name) const{ return isDefinedInternal(name, SymbolEntry::eVar ); }
	bool            isConstDefined(std::string const& name ) const{ return isDefinedInternal(name, SymbolEntry::eConstValue); }

	int             getVarTable( char const* varStr[], double varVal[] ) const;


	SymbolEntry const* findSymbol(std::string const& name) const
	{
		auto iter = mNameToEntryMap.find(name);
		if( iter == mNameToEntryMap.end() )
			return nullptr;
		return &iter->second;
	}
	SymbolEntry const* findSymbol(std::string const& name, SymbolEntry::Type type) const
	{
		auto iter = mNameToEntryMap.find(name);
		if( iter == mNameToEntryMap.end() || iter->second.type != type )
			return nullptr;
		return &iter->second;
	}
protected:


	bool  isDefinedInternal(std::string const& name, SymbolEntry::Type type) const
	{
		return findSymbol(name, type) != nullptr;
	}

	void defineFunInternal( char const* name ,void* funPtr ,int num )
	{
		FunInfo info;
		info.ptrFun = funPtr;
		info.numParam = num;
		mNameToEntryMap[name] = info;
	}

	std::map< std::string, SymbolEntry > mNameToEntryMap;

};


enum ParseErrorCode
{
	eExprError  ,
	eAllocFailed ,
};

class ParseException : public std::exception
{
public:
	ParseException( ParseErrorCode code , char const* what )
		:std::exception( what )
		,errorCode( code ){}
	ParseErrorCode errorCode;
};

class ExprTreeBuilder : public ExprParse
{
	typedef ExprParse::Unit Unit;
public:

	void build( NodeVec& nodes , Unit* exprCode , int numUnit ) /*throw ParseException */;
	void convertPostfixCode( UnitCodes& codes );
	enum
	{
		TREE_NO_ERROR = 0 ,
		TREE_OP_NO_CHILD ,
		TREE_ASSIGN_LEFT_NOT_VAR ,
		TREE_FUN_PARAM_NUM_NO_MATCH ,

	};
	int   checkTreeError();

	void  optimizeNodeOrder();
	bool  optimizeNodeConstValue(int idxNode );
	bool  optimizeNodeBOpOrder( int idxNode );

	void  printTree( SymbolTable const& table );
private:

	Node& getNode( int idx ){ return mTreeNodes[ idx ]; }
	void  printSpace( int num );
	void  printTree_R( int idxNode , int depth );
	int   buildTree_R( int idxParent , int idxStart , int idxEnd , bool funDef );
	void  convertPostfixCode_R( UnitCodes& codes , int idxNode );
	int   checkTreeError_R( int idxNode );
	int   optimizeNodeOrder_R( int idxNode );

	bool  canExchangeNode( int idxNode , TokenType type )
	{
		assert( idxNode < 0 );
		assert( CanExchange( type ) );
		Unit& unit = mExprCodes[ LEAF_UNIT_INDEX( idxNode ) ];

		if ( !IsBinaryOperator( unit.type ) )
			return false;
		if ( PrecedeceOrder( type ) != PrecedeceOrder( unit.type ) )
			return false;

		return true;
	}

	unsigned haveConstValueChild( int idxNode )
	{
		assert( idxNode > 0 );
		Node& node = mTreeNodes[ idxNode ];
		int result = 0;
		if ( isConstValueNode( node.children[ CN_LEFT ] ) )
			result |= ( 1 << CN_LEFT );
		if ( isConstValueNode( node.children[ CN_RIGHT ] ) )
			result |= ( 1 << CN_RIGHT );
		return result;
	}

	bool  isConstValueNode( int idxNode )
	{
		if ( idxNode >= 0 )
			return false;
		Unit& unit = mExprCodes[ LEAF_UNIT_INDEX( idxNode ) ];
		if ( unit.type != VALUE_CONST )
			return false;
		return true;
	}

	std::vector< int > mIdxOpNext;
	SymbolTable const* mTable;
	Node*    mTreeNodes;
	int      mNumNodes;
	Unit*    mExprCodes;
};

class ParseResult : public ExprParse
{
public:
	typedef ExprParse::Unit Unit;

	bool   isUsingVar( char const* name ) const;
	bool   isUsingInput(char const* name) const;
	void   optimize();

	template< class TCodeGenerator >
	void   generateCode(TCodeGenerator& geneartor , int numInput, ValueLayout inputLayouts[]);

	UnitCodes const& getInfixCodes() const { return mIFCodes; }
	UnitCodes const& getPostfixCodes() const { return mPFCodes; }

private:

	bool optimizeZeroValue(int index);
	bool optimizeConstValue(int index);
	bool optimizeVarOrder(int index);
	bool optimizeOperatorOrder(int index);
	bool optimizeValueOrder(int index);

	SymbolTable const* mSymbolDefine = nullptr;
	NodeVec      mTreeNodes;
	UnitCodes    mIFCodes;
	UnitCodes    mPFCodes;

	friend class ExpressionParser;
public:

	void MoveElement(int start,int end,int move)
	{
		for ( int i = start; i < end; ++i )
			mPFCodes[i+move] = mPFCodes[i];
	}
	void printInfixCodes(){ ExprParse::print( mIFCodes , *mSymbolDefine ,  false ); }
	void printPostfixCodes(){ ExprParse::print( mPFCodes , *mSymbolDefine ,  true ); }

};

class TCodeGenerator
{
public:
	typedef ParseResult::TokenType TokenType;
	void codeInit();
	void codeConstValue(ConstValueInfo const&val);
	void codeVar( VarValueInfo const& varInfo);
	void codeInput(uint8 inputIndex);
	void codeFunction(FunInfo const& info);
	void codeBinaryOp(TokenType type,bool isReverse);
	void codeUnaryOp(TokenType type);
	void codeEnd();
};


class ExpressionParser : public ExprParse
{
public:

	enum
	{

	};

	// test Compile String has used Var(name)
	bool parse( char const* expr , SymbolTable const& table );
	bool parse( char const* expr , SymbolTable const& table , ParseResult& result );

	std::string const& getErrorMsg(){ return mErrorMsg; }

protected:

	bool analyzeTokenUnit( char const* expr , SymbolTable const& table , UnitCodes& infixCode );
	void convertCode( UnitCodes& infixCode , UnitCodes& postfixCode );
	bool testTokenValid(int bToken,int cToken);

	std::string  mErrorMsg;
};



template< class TCodeGenerator >
void ParseResult::generateCode( TCodeGenerator& generator , int numInput, ValueLayout inputLayouts[] )
{
	generator.codeInit(numInput , inputLayouts);

	for ( Unit const& unit : mPFCodes )
	{
		switch( unit.type )
		{
		case ExprParse::VALUE_CONST:
			generator.codeConstValue(unit.constValue);
			break;
		case ExprParse::VALUE_VARIABLE:
			generator.codeVar(unit.symbol->varValue);
			break;
		case ExprParse::VALUE_INPUT:
			generator.codeInput(unit.symbol->inputIndex);
			break;
		default:
			switch( unit.type & TOKEN_MASK )
			{
			case TOKEN_FUN:
				generator.codeFunction(unit.symbol->fun);
				break;
			case TOKEN_UNARY_OP:
				generator.codeUnaryOp(unit.type);
				break;
			case TOKEN_BINARY_OP:
				generator.codeBinaryOp(unit.type, unit.isReverse);
				break;
			}
		}
	}
	generator.codeEnd();
}


#endif //ExpressionParser_H
