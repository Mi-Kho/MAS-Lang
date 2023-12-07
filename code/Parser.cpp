#ifndef PARSER_H
#define PARSER_H
#include "Parser.h"
#include "Error.h"
#endif


Base* Parser::parseS()
{
	llvm::SmallVector<Statement*> statements;
	while (!Tok.is(Token::eof))
	{
		switch (Tok.getKind())
		{
		case Token::ident:
		{
			AssignStatement* state = parseAssign();

			statements.push_back(state);
			break;
		}
		case Token::KW_int:
		{
			llvm::SmallVector<DecStatement*> states = parseDefine();
			if (states.size() == 0)
			{
				return nullptr;
			}

			while (states.size() > 0)
			{
				statements.push_back(states.back());
				states.pop_back();
			}
			break;
		}
		case Token::KW_if:
		{
			ControlStatement* statement = parseIf();
			statements.push_back(statement);
			break;

		}
		case Token::KW_loopc:
		{
			ControlStatement* statement = parseLoop();
			statements.push_back(statement);
			break;
		}

		default:
		{
			return new Base(statements);
		}


		}
	}
	return new Base(statements);


}

llvm::SmallVector<DecStatement*> Parser::parseDefine()
{
	llvm::SmallVector<DecStatement*> assignments;

	llvm::SmallVector<Expression*> variables;
	llvm::SmallVector<Expression*> values;

	advance();
	bool SeenTokenVariable = true;
	while (SeenTokenVariable)
	{
		Expression* lhand = parseVar();
		variables.push_back(lhand);

		if (!Tok.is(Token::comma))
			SeenTokenVariable = false;
		else
			advance();
	}



	if (Tok.is(Token::eoi))
	{
		bool SeenTokenValue = true;
		for (int i = 0; i < variables.size(); i++)
		{
			Expression* rhand = new Expression(0);
			values.push_back(rhand);
		}
	}
	else if (!Tok.is(Token::equal))
	{
		Error::AssignmentEqualNotFound();
	}
	else
	{
		advance(); // pass equal

		bool SeenTokenValue = true;
		while (SeenTokenValue)
		{
			Expression* rhand = parseExpr();
			values.push_back(rhand);

			if (!Tok.is(Token::comma))
				SeenTokenValue = false;
			else
				advance();
		}
	}




	if (variables.size() < values.size())
	{
		Error::AssignmentSidesNotEqual();
	}
	else
	{
		while (values.size() != 0)
		{
			assignments.push_back(new DecStatement(variables.back(), values.size() > 0 ? values.back() : new Expression(0)));
			variables.pop_back();
			values.pop_back();
		}

		if (Tok.is(Token::eoi))
		{
			advance();
			return assignments;
		}
		else
		{
			Error::SemiColonNotFound();
		}
	}


}


Expression* Parser::parseExpr()
{
	Expression* Left = parseTerm();
	while (Tok.isOneOf(Token::plus, Token::minus))
	{
		BinaryOp::Operator Op =
			Tok.is(Token::plus) ? BinaryOp::Plus : BinaryOp::Minus;
		advance();
		Expression* Right = parseTerm();
		Left = new BinaryOp(Op, Left, Right);
	}
	return Left;
}


Expression* Parser::parseTerm()
{
	Expression* Left = parsePower();
	while (Tok.isOneOf(Token::star, Token::slash, Token::mod))
	{
		BinaryOp::Operator Op =
			Tok.is(Token::star) ? BinaryOp::Mul : Tok.is(Token::slash) ? BinaryOp::Div : BinaryOp::Mod;
		advance();
		Expression* Right = parsePower();
		Left = new BinaryOp(Op, Left, Right);
	}
	return Left;
}


Expression* Parser::parsePower()
{
	Expression* Left = parseFactor();
	while (Tok.is(Token::power))
	{
		BinaryOp::Operator Op =
			BinaryOp::Pow;
		advance();
		Expression* Right = parseFactor();
		Left = new BinaryOp(Op, Left, Right);
	}
	return Left;
}

Expression* Parser::parseFactor()
{
	Expression* Res = nullptr;
	switch (Tok.getKind())
	{
	case Token::number:
	{
		int number;
		Tok.getText().getAsInteger(10, number);
		Res = new Expression(number);
		advance();
		break;
	}
	case Token::ident:
	{
		Res = new Expression(Tok.getText());
		advance();
		break;
	}
	case Token::l_paren:
	{
		advance();
		Res = parseExpr();
		if (!consume(Token::r_paren))
			break;
	}
	default: // error handling
	{
		Error::NumberVariableExpected();
	}

	}
	return Res;
}


AssignStatement* Parser::parseAssign()
{
	Expression* variable;
	Expression* value;

	variable = parseVar();

	if (Tok.is(Token::minus_equal))
	{
		advance();
		value = parseExpr();
		value = new BinaryOp(BinaryOp::Minus, variable, value);

	}
	else if (Tok.is(Token::plus_equal))
	{
		advance();
		value = parseExpr();
		value = new BinaryOp(BinaryOp::Plus, variable, value);
	}
	else if (Tok.is(Token::star_equal))
	{
		advance();
		value = parseExpr();
		value = new BinaryOp(BinaryOp::Mul, variable, value);
	}
	else if (Tok.is(Token::slash_equal))
	{
		advance();
		value = parseExpr();
		value = new BinaryOp(BinaryOp::Div, variable, value);
	}
	else if (Tok.is(Token::equal))
	{
		advance(); // pass equal
		value = parseExpr();
	}
	else
	{
		Error::AssignmentEqualNotFound();
	}

	if (!Tok.is(Token::eoi))
	{
		Error::SemiColonNotFound();
	}

	advance(); // pass semicolon
	return new AssignStatement(variable, value);

}


Expression* Parser::parseVar()
{
	if (!Tok.is(Token::ident))
	{
		Error::VariableNameNotFound();
	}

	Expression* variable = new Expression(Tok.getText());
	advance();
	return variable;
}


ControlStatement* Parser::parseLoop()
{
	advance();			// pass loop identifier

	Expression* condition = parseCondition();



	if (!Tok.is(Token::colon))
	{
		Error::ColonExpectedAfterCondition();
	}

	advance();

	if (Tok.is(Token::begin))
	{
		advance();

		Base* AllStates = parseS();

		if (!consume(Token::end))
		{

			return new ControlStatement(condition, AllStates->getStatements(), Statement::StateMentType::Loop);
		}
		else
		{
			Error::EndNotSeenForIf();
		}
	}
	else
	{
		Error::BeginExpectedAfterColon();
	}
}

Expression* Parser::parseCondition()
{
	Expression* lcondition;

	lcondition = parseSubCondition();
	if (Tok.is(Token::KW_and))
	{
		advance();
		Expression* rcondition = parseCondition();
		return new BooleanOp(BooleanOp::And, lcondition, rcondition);
	}
	else if (Tok.is(Token::KW_or))
	{
		advance();
		Expression* rcondition = parseCondition();
		return new BooleanOp(BooleanOp::Or, lcondition, rcondition);
	}
	else
	{
		return lcondition;
	}
}

Expression* Parser::parseSubCondition()
{


	if (Tok.is(Token::l_paren))
	{

		advance();
		Expression* expressionInside = parseCondition();
		if (!consume(Token::r_paren))
		{
			return expressionInside;
		}
		else
		{
			Error::RightParanthesisExpected();
		}
	}

	else if (Tok.is(Token::KW_true))
	{
		advance();

		return new Expression(true);
	}
	else if (Tok.is(Token::KW_false))
	{
		advance();
		return new Expression(false);
	}
	else
	{
		Expression* lhand = parseExpr();
		BooleanOp::Operator Op;

		if (Tok.is(Token::less))
		{
			Op = BooleanOp::Less;
		}
		else if (Tok.is(Token::less))
		{
			Op = BooleanOp::LessEqual;
		}
		else if (Tok.is(Token::greater))
		{
			Op = BooleanOp::Greater;
		}
		else if (Tok.is(Token::greater_equal))
		{
			Op = BooleanOp::GreaterEqual;
		}
		else if (Tok.is(Token::equal_equal))
		{
			Op = BooleanOp::Equal;
		}
		else
		{
			Error::BooleanValueExpected();
		}

		advance();
		Expression* rhand = parseExpr();
		return new BooleanOp(Op, lhand, rhand);

	}
}

ControlStatement* Parser::parseIf()
{
	advance();			// pass if identifier

	Expression* condition = parseCondition();

	

	if (!Tok.is(Token::colon))
	{
		Error::ColonExpectedAfterCondition();
	}

	advance();

	if (Tok.is(Token::begin))
	{
		advance();

		Base* AllStates = parseS();

		if (!consume(Token::end))
		{
	
			return new ControlStatement(condition, AllStates->getStatements(), Statement::StateMentType::If);
		}
		else
		{
			Error::EndNotSeenForIf();
		}
	}
	else
	{
		Error::BeginExpectedAfterColon();
	}


}


Base* Parser::parse()
{
	Base* Res = parseS();
	return Res;
}