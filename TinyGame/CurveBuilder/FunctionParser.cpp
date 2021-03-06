#include "FunctionParser.h"
#include "Expression.h"

#include <cmath>

namespace CB
{

	static RealType sVarDummy;

	void ReplaceAllSubString(std::string& str, std::string const& from, std::string const& to)
	{
		size_t posStart = 0;
		while( (posStart = str.find(from, posStart)) != std::string::npos )
		{
			str.replace(posStart, from.length(), to);
			posStart += to.length();
		}
	}

	bool FunctionParser::checkValid(char const* expr)
	{
		bool result;
		try
		{
			ExpressionParser parser;
			result = parser.parse(expr, mSymbolDefine);
		}
		catch( ParseException& e )
		{
			return false;
		}
		return result;
	}

	bool FunctionParser::parse(Expression& expr , int numInput , ValueLayout inputLayouts[] )
	{
		expr.mIsParsed = compile(expr.getExprString().c_str(), mSymbolDefine, expr.getEvalData(), numInput , inputLayouts);
		return expr.mIsParsed;
	}

	FunctionParser::FunctionParser()
	{
		SymbolTable& table = getSymbolDefine();

		table.defineVarInput("x", 0);
		table.defineVarInput("y", 1);
		table.defineVarInput("u", 0);
		table.defineVarInput("v", 1);

		table.defineFun("sin", sin);
		table.defineFun("cos", cos);
		table.defineFun("tan", tan);
		table.defineFun("exp", exp);
		table.defineFun("ln", log);
		table.defineFun("log", log10);
		table.defineFun("sqrt", sqrt);
		table.defineFun("pow", pow);
	}

}//namespace CB