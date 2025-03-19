#pragma once

#include "tokenizer.h"
#include "type_node.h"
#include <string>
#include <unordered_set>
#include <deque>

#include "parser_interface.h"

class Parser : private Tokenizer
{
public:
	Parser(ParserInterface &interface);
	virtual ~Parser();

	// No copying of parser
	Parser(const Parser& other) = delete;
	Parser(Parser&& other) = delete;

	// Parses the given input
	bool Parse(const std::string_view& fileName, const std::string_view &input);

	void SetInterface(ParserInterface& interface);

	using Tokenizer::GetError;
	using Tokenizer::AddMacro;

protected:
	struct Scope
	{
		ScopeType type;
		std::string_view name;
		AccessControlType currentAccessControlType;
	};

	/// Called to parse the next statement. Returns false if there are no more statements.
	bool ParseBaseType(Token &baseType);
	SignednessSpecifier ParseSignednessSpecifier();
	SizeSpecifier ParseSizeSpecifier();
	bool ParseStatement();
	bool ParseDeclaration(Token &token);
	bool ParseDirective();
	bool SkipDeclaration(Token &token);
	bool ParseProperty(Token &token, bool isTypedef = false, bool skipType = false);
	bool ParseEnum(Token &token);
	bool ParseUsing(Token &token);
	bool ParseFriend(Token& token);

	void PushScope(const std::string_view& name, ScopeType scopeType, AccessControlType accessControlType);
	void PopScope();

	bool ParseNamespace();
	bool ParseAccessControl(const Token& token, AccessControlType& type);

	AccessControlType GetCurrentAccessControlType() const;
	void WriteCurrentAccessControlType();

	void WriteAccessControlType(AccessControlType type);
	bool ParseClass(Token &token);
	bool ParseTemplate();
	bool ParseFunction(Token &token, const Scope *scope);
	TypeNode::Type PeekConstructor(const std::string_view& scopeName);

	bool ParseComment();

	bool ParseType(TypeNode::Type *type = nullptr, bool visit = true, const std::string_view& constructorName = std::string_view(), std::string_view *outName = nullptr, bool inTemplate = false);

	std::unique_ptr<TypeNode> ParseTypeNode(const std::string_view& constructorName, bool inTemplate = false);
	bool ParseTypeNodeDeclarator(std::string &declarator, const std::string_view& constructorName, bool checkSpecifier = true);

	//void WriteToken(const Token &token);

private:
	ParserInterface& writer_;

	std::deque<Scope> scopes_;
	unsigned m_unnamedCnt;

	bool ParseTemplateArgument();
	std::string GenerateUnnamedIdentifier(const std::string_view &name);
};
