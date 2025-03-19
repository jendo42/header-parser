#include <iostream>
#include <vector>
#include <algorithm>
#include "parser.h"
#include "token.h"
#include <cstdarg>
#include <Windows.h>
#include <unordered_set>

#include "ScopeGuard.h"

static const std::unordered_set<std::string> g_baseTypes{
	"void",
	"bool",
	"int",
	"char",
	"float",
	"double"
};

static std::string_view Signedness2String(SignednessSpecifier sign)
{
	switch (sign) {
	default:
		return "";
	case SignednessSpecifier::kSigned:
		return "signed";
	case SignednessSpecifier::kUnsigned:
		return "unsigned";
	}
}

static std::string_view Size2String(SizeSpecifier size)
{
	switch (size) {
	default:
		return "";
	case SizeSpecifier::kShort:
		return "short";
	case SizeSpecifier::kLong:
		return "long";
	case SizeSpecifier::kLongLong:
		return "long long";
	}
}

static bool CanHaveConstructor(ScopeType scope)
{
	switch (scope) {
	case ScopeType::kClass:
	case ScopeType::kStructure:
	case ScopeType::kUnion:
		return true;
	default:
		return false;
	}
}

static ScopeType Token2ScopeType(const std::string_view& token)
{
	if (token == "class") {
		return ScopeType::kClass;
	} else if (token == "struct") {
		return ScopeType::kStructure;
	} else if (token == "struct") {
		return ScopeType::kStructure;
	} else if (token == "union") {
		return ScopeType::kUnion;
	} else {
		return ScopeType::kUnknown;
	}
}

static bool isStructure(const std::string_view& token)
{
	return
		   token == "class"
		|| token == "struct"
		|| token == "union"
		|| token == "enum"
	;
}

static bool isSpecifier(const std::string_view& token)
{
	return isStructure(token) || token == "typename";
}

//-------------------------------------------------------------------------------------------------
// Class used to write a typenode structure to json
//-------------------------------------------------------------------------------------------------
class TypeNodeWriter : public TypeNodeVisitor
{
public:
	TypeNodeWriter(ParserInterface &writer) :
		writer_(writer) {}

	//-------------------------------------------------------------------------------------------------
	virtual void Visit(FunctionNode& node) override
	{
		writer_.typeName(node.name);

		// return type
		VisitNode(*node.returns);

		for (auto& arg : node.arguments)
		{
			VisitNode(*arg->type, arg->name);
		}
	}

	//-------------------------------------------------------------------------------------------------
	virtual void Visit(LReferenceNode& node) override
	{
		VisitNode(*node.base);
	}

	//-------------------------------------------------------------------------------------------------
	virtual void Visit(LiteralNode& node) override
	{
		writer_.typeName(node.name);
	}

	//-------------------------------------------------------------------------------------------------
	virtual void Visit(PointerNode& node) override
	{
		VisitNode(*node.base);
	}

	//-------------------------------------------------------------------------------------------------
	virtual void Visit(ReferenceNode& node) override
	{
		VisitNode(*node.base);
	}

	//-------------------------------------------------------------------------------------------------
	virtual void Visit(TemplateNode& node) override
	{
		writer_.typeName(node.name);
		for (auto& arg : node.arguments)
			VisitNode(*arg);
	}

	//-------------------------------------------------------------------------------------------------
	virtual void VisitNode(TypeNode &node, const std::string_view &name = std::string_view()) override
	{
		writer_.beginType(node.type, node.specifiers);
		if (!name.empty()) {
			writer_.typeName(name);
		}
		TypeNodeVisitor::VisitNode(node);
		writer_.endType();
	}

private:
	ParserInterface &writer_;
};

//--------------------------------------------------------------------------------------------------
Parser::Parser(ParserInterface& writer)
	: writer_(writer)
	, m_unnamedCnt(0)
{

}

//--------------------------------------------------------------------------------------------------
Parser::~Parser()
{

}

//--------------------------------------------------------------------------------------------------
bool Parser::Parse(const std::string_view &fileName, const std::string_view &input)
{
	// Pass the input to the tokenizer
	Reset(input.data(), input.length());

	// Start the array
	writer_.begin(fileName);

	// Reset scope
	scopes_.clear();
	scopes_.emplace_back(Scope{
		ScopeType::kGlobal,
		"",
		AccessControlType::kPublic
	});

	// Parse all statements in the file
	while(ParseStatement())
	{
	}

	// End the array
	writer_.end(fileName, GetError());
	return !HasError();
}

bool Parser::ParseBaseType(Token& baseType)
{
	if (!GetIdentifier(baseType)) {
		return false;
	}
	const auto& it = g_baseTypes.find(std::string(baseType.token));
	bool notFound = it == g_baseTypes.cend();
	if (notFound) {
		UngetToken(baseType);
	}
	return !notFound;
}

SignednessSpecifier Parser::ParseSignednessSpecifier()
{
	Token token;
	if (GetIdentifier(token)) {
		if (token.token == "signed") {
			return SignednessSpecifier::kSigned;
		}
		else if (token.token == "unsigned") {
			return SignednessSpecifier::kUnsigned;
		}
		else {
			UngetToken(token);
		}
	}
	return SignednessSpecifier::kNone;
}

SizeSpecifier Parser::ParseSizeSpecifier()
{
	Token token;
	if (GetIdentifier(token)) {
		if (token.token == "short") {
			return SizeSpecifier::kShort;
		}
		else if (token.token == "long") {
			Token token2;
			if (GetIdentifier(token2)) {
				if (token2.token == "long") {
					return SizeSpecifier::kLongLong;
				} else {
					UngetToken(token2);
				}
			}
			return SizeSpecifier::kLong;
		}
		else {
			UngetToken(token);
		}
	}
	return SizeSpecifier::kNone;
}

//--------------------------------------------------------------------------------------------------
bool Parser::ParseStatement()
{
	Token token;
	if(!GetToken(token))
		return false;

	if (!ParseDeclaration(token))
		return false;

	return true;
}

//--------------------------------------------------------------------------------------------------
bool Parser::ParseDeclaration(Token &token)
{
	std::vector<std::string>::const_iterator customMacroIt;
	if (token.token == "#")
		return ParseDirective();
	else if (token.tokenType == TokenType::kMacro)
		return ParseMacro(token);
	else if (token.token == ";")
			return true; // Empty statement
	else if (token.token == "typedef")
			return ParseProperty(token, true);
	else if (token.token == "using")
			return ParseUsing(token);
	else if (token.token == "friend")
			return ParseFriend(token);
	else if (token.token == "namespace")
			return ParseNamespace();
	else if (token.token == "template")
			return ParseTemplate();
	else if (token.token == "enum")
			return ParseEnum(token);
	else if (isStructure(token.token))
			return ParseClass(token);
	else if (ParseAccessControl(token, scopes_.back().currentAccessControlType))
		return RequireSymbol(":");
	else if (ParseFunction(token, &scopes_.back()))
			return true;
	else
		return SkipDeclaration(token);

	return true;
}

//--------------------------------------------------------------------------------------------------
bool Parser::ParseDirective()
{
	Token token;
	ScopeGuard guard{ [&]() {
		SetMacroParsing(true);
	} };

	SetMacroParsing(false);

	// Check the compiler directive
	if(!GetIdentifier(token))
		return Error("Missing compiler directive after #");

	bool multiLineEnabled = false;
	if(token.token == "define")
	{
		if (!GetIdentifier(token)) {
			return Error("Missing compiler directive identifier");
		}
		AddMacro(std::string(token.token));
		multiLineEnabled = true;
	}
	else if(token.token == "include")
	{
		Token includeToken;
		GetToken(includeToken, true);
		writer_.include(std::string(includeToken.token));
	}

	// Skip past the end of the token
	char lastChar = '\n';
	do
	{
		// Skip to the end of the line
		char c;
		while (!this->is_eof() && (c = GetChar()) != '\n') {
			lastChar = c;
		}

	} while(multiLineEnabled && lastChar == '\\');

	SetMacroParsing(true);
	return true;
}

//--------------------------------------------------------------------------------------------------
bool Parser::SkipDeclaration(Token &token)
{
	int32_t scopeDepth = 0;
	while(GetToken(token))
	{
		if(token.token == ";" && scopeDepth == 0)
			break;

		if(token.token == "{")
			scopeDepth++;

		if(token.token == "}")
		{
			--scopeDepth;
			if(scopeDepth == 0)
				break;
		}
	}

	return true;
}

//--------------------------------------------------------------------------------------------------
bool Parser::ParseEnum(Token &startToken)
{
	auto startLine = (unsigned)startToken.startLine;

	UngetToken(startToken);
	WriteCurrentAccessControlType();

	if (!RequireIdentifier("enum"))
		return false;

	// C++1x enum class type?
	bool isEnumClass = MatchIdentifier("class");

	std::string_view name;
	std::string_view base;

	Token enumToken;
	if (GetIdentifier(enumToken)) {
		// type name is optional
		name = enumToken.token;
	} else {
		name = GenerateUnnamedIdentifier("enum");
	}

	// Parse C++1x enum base
	if(isEnumClass && MatchSymbol(":"))
	{
		Token baseToken;
		if (!GetIdentifier(baseToken))
			return Error("Missing enum type specifier after :");

		// Validate base token
		base = baseToken.token;
	}

	// Require opening brace
	RequireSymbol("{");

	writer_.beginEnum(startLine, name, base, isEnumClass);

	// Parse all the values
	Token token;
	while(GetIdentifier(token))
	{
		// Store the identifier
		std::string_view key = token.token;
		std::string_view value;

		// Parse constant
		if(MatchSymbol("="))
		{
			Token startToken;
			GetToken(startToken);
			UngetToken(startToken);

			// Just parse the value, not doing anything with it atm
			while (GetToken(token) && (token.tokenType != TokenType::kSymbol || (token.token != "," && token.token != "}"))) {

			}

			value = std::string_view(startToken.token.data(), token.startPos - startToken.startPos);
			UngetToken(token);
		}

		writer_.enumValue(key, value);

		// Next value?
		if(!MatchSymbol(","))
			break;
	}

	if (!RequireSymbol("}"))
		return false;

	MatchSymbol(";");

	writer_.endEnum(name);

	return true;
}

//--------------------------------------------------------------------------------------------------
void Parser::PushScope(const std::string_view &name, ScopeType scopeType, AccessControlType accessControlType)
{
	scopes_.emplace_back(Scope{
		scopeType, name, accessControlType
	});
}

//--------------------------------------------------------------------------------------------------
void Parser::PopScope()
{
	scopes_.pop_back();
}

//--------------------------------------------------------------------------------------------------
bool Parser::ParseNamespace()
{
	Token token;
	if (!GetIdentifier(token))
		return Error("Missing namespace name");

	std::string_view name = token.token;

	if (!RequireSymbol("{"))
		return false;

	writer_.beginNamespace(name);
	PushScope(std::string(token.token), ScopeType::kNamespace, AccessControlType::kPublic);

	while (!MatchSymbol("}"))
		if (!ParseStatement())
			return false;

	PopScope();
	writer_.endNamespace(name);
	return true;
}

//-------------------------------------------------------------------------------------------------
bool Parser::ParseAccessControl(const Token &token, AccessControlType& type)
{
	if (token.token == "public")
	{
		type = AccessControlType::kPublic;
		return true;
	}
	else if (token.token == "protected")
	{
		type = AccessControlType::kProtected;
		return true;
	}
	else if (token.token == "private")
	{
		type = AccessControlType::kPrivate;
		return true;
	}
	
	return false;
}

AccessControlType Parser::GetCurrentAccessControlType() const
{
	return scopes_.back().currentAccessControlType;
}

//-------------------------------------------------------------------------------------------------
void Parser::WriteCurrentAccessControlType()
{
	// Writing access is not required if the current scope is not owned by a class
	if (scopes_.back().type != ScopeType::kClass)
		return;

	WriteAccessControlType(GetCurrentAccessControlType());
}

//-------------------------------------------------------------------------------------------------
void Parser::WriteAccessControlType(AccessControlType type)
{
	writer_.access(type);
}

//--------------------------------------------------------------------------------------------------
bool Parser::ParseClass(Token &token)
{
	auto startLine = (unsigned)token.startLine;

	WriteCurrentAccessControlType();

	if (!ParseComment())
		return false;

	ScopeType scopeType = Token2ScopeType(token.token);
	if (scopeType == ScopeType::kUnknown)
		return Error("Missing identifier class/struct/union");

	bool isPrivate = scopeType == ScopeType::kClass;

	// Get the class name
	std::string name;

	Token classNameToken;
	if (GetIdentifier(classNameToken)) {
		name = classNameToken.token;
	} else {
		name = GenerateUnnamedIdentifier(token.token);
	}

	writer_.beginClass(startLine, name, scopeType);

	// Match base types
	if(MatchSymbol(":"))
	{
		do
		{
			Token accessOrName;
			if (!GetIdentifier(accessOrName)) {
				return Error("Missing class or access control specifier");
			}

			// Parse the access control specifier
			AccessControlType accessControlType = AccessControlType::kPrivate;
			if (!ParseAccessControl(accessOrName, accessControlType))
				UngetToken(accessOrName);
			WriteAccessControlType(accessControlType);
			
			// Get the name of the class
			if (!ParseType())
				return false;

			writer_.baseType();
		}
		while (MatchSymbol(","));
	}

	if (MatchSymbol(";")) {
		// forward declaration
		writer_.endClass(name, true);
		UngetToken(token);
		return SkipDeclaration(token);
	}
	if (!RequireSymbol("{"))
		return false;

	PushScope(name, scopeType, isPrivate ? AccessControlType::kPrivate : AccessControlType::kPublic);

	while (!MatchSymbol("}"))
		if (!ParseStatement())
			return false;

	PopScope();

	writer_.endClass(name, false);

	Token typedProperty;
	if (GetIdentifier(typedProperty)) {
		writer_.beginType(TypeNode::Type::kLiteral, Specifiers{});
		writer_.typeName(name);
		writer_.endType();
		UngetToken(typedProperty);
		if (!ParseProperty(token, false, true)) {
			return false;
		}
	} else {
		if (!RequireSymbol(";"))
			return false;
	}

	return true;
}

//-------------------------------------------------------------------------------------------------
bool Parser::ParseProperty(Token &token, bool isTypedef, bool skipType)
{
	auto startLine = (unsigned)token.startLine;

	WriteCurrentAccessControlType();

	//if (isTypedef) {
	//	GetToken(token);
	//}

	// Process method specifiers in any particular order
	bool isMutable = false, isStatic = false;
	for (bool matched = true; matched;)
	{
		matched = (!isMutable && (isMutable = MatchIdentifier("mutable"))) ||
			(!isStatic && (isStatic = MatchIdentifier("static")));
	}

	Specifiers specifiers = { 0 };
	specifiers.isMutable = isMutable;
	specifiers.isStatic = isStatic;

	// Parse the type
	std::string_view name;
	if (!skipType) {
		if (!ParseType(nullptr, isTypedef, std::string(), &name))
			return false;
	}

	// Parse the name
	if (name.empty()) {
		Token nameToken;
		if (!GetIdentifier(nameToken))
			return false;
		name = nameToken.token;
	}

	if (isTypedef) {
		writer_.beginTypedef(startLine, name);
	} else {
		writer_.beginProperty(startLine, name, specifiers);
	}

	// Parse array
	if (MatchSymbol("["))
	{
		Token arrayToken;
		if(!GetConst(arrayToken))
			if(!GetIdentifier(arrayToken))
				return false; // Expected a property name

		writer_.arraySubscript(std::string(arrayToken.token));

		if(!MatchSymbol("]"))
			return false;
	}

	if (isTypedef) {
		writer_.endTypedef(name);
	} else {
		writer_.endProperty(name);
	}

	// Skip until the end of the definition
	Token t;
	while(GetToken(t))
		if(t.token == ";")
			break;

	return true;
}
//-------------------------------------------------------------------------------------------------
bool Parser::ParseUsing(Token& token)
{
	auto startLine = (unsigned)token.startLine;

	WriteCurrentAccessControlType();

	// Parse the type
	if (!ParseType())
		return false;

	bool hasAssigment = false;
	if (MatchSymbol("=")) {
		hasAssigment = true;
		if (!ParseType())
			return false;
	}

	writer_.using_(hasAssigment);

	// Skip until the end of the definition
	Token t;
	while (GetToken(t))
		if (t.token == ";")
			break;

	return true;
}

bool Parser::ParseFriend(Token& token)
{
	if (!ParseType()) {
		return Error("Expected 'type' after 'friend'");
	}

	writer_.friend_();

	Token t;
	while (GetToken(t))
		if (t.token == ";")
			break;

	return true;
}

//--------------------------------------------------------------------------------------------------
bool Parser::ParseFunction(Token &token, const Scope *scope)
{
	auto startLine = (unsigned)token.startLine;
	
	//if (token.tokenType == TokenType::kMacro) {
	//	ParseMacro()
	//}

	UngetToken(token);

	if (!ParseComment())
		return false;

	WriteCurrentAccessControlType();

	// Process method specifiers in any particular order
	bool isVirtual = false, isInline = false, isConstExpr = false, isStatic = false, isExplicit = false;
	for(bool matched = true; matched;)
	{
		matched = (!isVirtual && (isVirtual = MatchIdentifier("virtual"))) ||
				(!isInline && (isInline = MatchIdentifier("inline"))) ||
				(!isConstExpr && (isConstExpr = MatchIdentifier("constexpr"))) ||
				(!isExplicit && (isExplicit = MatchIdentifier("explicit"))) ||
				(!isStatic && (isStatic = MatchIdentifier("static")));
	}

	TypeNode::Type type;
	if (CanHaveConstructor(scope->type) && !isStatic && PeekConstructor(scope->name) != TypeNode::Type::kNone) {
		if (!ParseType(&type, true, scope->name)) {
			return false;
		}
	} else {
		if (!ParseType(&type)) {
			return false;
		}
	}

	if (type == TypeNode::Type::kDestructor) {
		if (!RequireSymbol("~")) {
			return false;
		}
	}

	// Parse the name of the method
	std::string_view name;
	Token nameToken;
	if (!GetIdentifier(nameToken)) {
		return Error("Expected identifier");
	}

	name = nameToken.token;
	if (type == TypeNode::Type::kDestructor) {
		name = std::string_view(name.data() - 1, name.length() + 1);
	}

	if (name == "operator") {
		Token operatorToken;
		if (!GetToken(operatorToken)) {
			return false;
		}
		if (operatorToken.tokenType != TokenType::kSymbol) {
			return false;
		}
		name = std::string_view(name.data(), name.length() + operatorToken.token.length());
		if (operatorToken.token == "(") {
			if (!GetToken(operatorToken)) {
				return false;
			}
			if (operatorToken.token != ")") {
				return Error("Expected ')'");
			}
			name = std::string_view(name.data(), name.length() + operatorToken.token.length());
		}
	}

	// Start argument list from here
	if (!MatchSymbol("(")) {
		UngetToken(token);
		return ParseProperty(token);
	}

	Specifiers specifiers = { 0 };
	specifiers.isInline = isInline;
	specifiers.isVirtual = isVirtual;
	specifiers.isConstExpr = isConstExpr;
	specifiers.isStatic = isStatic;

	writer_.beginFunction(startLine, TypeNode::Type::kFunction, name);

	// Is there an argument list in the first place or is it closed right away?
	if (!MatchSymbol(")"))
	{
		// Walk over all arguments
		do
		{
			// Get the type of the argument
			bool hasName = false;
			TypeNode::Type type;
			if (!ParseType(&type))
				return false;

			// Parse the name of the function
			std::string_view identifier;
			if (GetIdentifier(nameToken)) {
				identifier = nameToken.token;
			}

			std::string_view defaultValue;

			// Parse default value
			if (MatchSymbol("=")) {
				Token token;
				Token startToken;
				GetToken(startToken);
				UngetToken(startToken);
				size_t closureCnt = 0;
				while (GetToken(token)) {
					if (closureCnt == 0 && (token.token == "," || token.token == ")")) {
						UngetToken(token);
						break;
					}
					if (token.token == "(") {
						++closureCnt;
					} else if (token.token == ")") {
						--closureCnt;
					}
				}

				if (startToken.tokenType == TokenType::kConst) {
					defaultValue = startToken.token;
				} else {
					defaultValue = std::string_view(startToken.token.data(), token.startPos - startToken.startPos);
				}
				defaultValue = defaultValue;
			}

			writer_.functionArgument(identifier, defaultValue);
		} while (MatchSymbol(",")); // Only in case another is expected

		MatchSymbol(")");
	}
	
	// Optionally parse constness
	bool isConst = false;
	if (MatchIdentifier("const")) {
		isConst = true;
	}

	bool isOverride = false;
	if (MatchIdentifier("override")) {
		isOverride = true;
	}

	bool isNoExcept = false;
	if (MatchIdentifier("noexcept")) {
		isNoExcept = true;
	}

	// Pure?
	bool isAbstract = false;
	bool isDefault = false;
	bool isDeleted = false;
	Token equals;
	if (MatchSymbol("=") && GetToken(equals)) {
		if (equals.token == "0") {
			isAbstract = true;
		} else if (equals.token == "default") {
			isDefault = true;
		} else if (equals.token == "delete") {
			isDeleted = true;
		} else {
			return Error("Unexpected token '%s'", std::string(equals.token).c_str());
		}
	}

	specifiers.isConstThis = isConst;
	specifiers.isOverride = isOverride;
	specifiers.isAbstract = isAbstract;
	specifiers.isDefault = isDefault;
	specifiers.isDeleted = isDeleted;

	writer_.endFunction(name, specifiers);

	// Skip either the ; or the body of the function
	Token skipToken;
	if (!SkipDeclaration(skipToken))
		return false;

	return true;
}

TypeNode::Type Parser::PeekConstructor(const std::string_view& scopeName)
{
	TypeNode::Type type = TypeNode::Type::kConstructor;
	Token token;
	Token startToken;

	if (!GetToken(token)) {
		return TypeNode::Type::kNone;
	}

	startToken = token;
	if (token.tokenType == TokenType::kSymbol && token.token == "~") {
		type = TypeNode::Type::kDestructor;
		if (!GetToken(token)) {
			UngetToken(startToken);
			return TypeNode::Type::kNone;
		}
	}

	if (token.token != scopeName) {
		UngetToken(startToken);
		return TypeNode::Type::kNone;
	}

	if (MatchSymbol("(")) {
		UngetToken(startToken);
		return type;
	}

	UngetToken(startToken);
	return TypeNode::Type::kNone;
}

//-------------------------------------------------------------------------------------------------
bool Parser::ParseComment()
{
	std::string comment = lastComment_.endLine == cursorLine_ ? lastComment_.text : "";
	if (!comment.empty())
	{
		writer_.comment(comment);
	}

	return true;
}

//--------------------------------------------------------------------------------------------------
bool Parser::ParseType(TypeNode::Type *type, bool visit, const std::string_view& constructorName, std::string_view *outName, bool inTemplate)
{
	std::unique_ptr<TypeNode> node = ParseTypeNode(constructorName, inTemplate);
	if (node == nullptr)
		return false;
	if (visit) {
		TypeNodeWriter writer(writer_);
		writer.VisitNode(*node);
	}
	if (type) {
		*type = node->type;
	}
	if (outName) {
		auto fn = dynamic_cast<FunctionNode*>(node.get());
		if (fn) {
			*outName = fn->name;
		}
	}
	return true;
}

//-------------------------------------------------------------------------------------------------
std::unique_ptr<TypeNode> Parser::ParseTypeNode(const std::string_view &constructorName, bool inTemplate)
{
	std::unique_ptr<TypeNode> node;
	Token token;

	bool isConst = false, isVolatile = false, isMutable = false, isUnsigned = false;
	for (bool matched = true; matched;)
	{
		matched = (!isConst && (isConst = MatchIdentifier("const")))
				|| (!isVolatile && (isVolatile = MatchIdentifier("volatile")))
				|| (!isMutable && (isMutable = MatchIdentifier("mutable")))
		;
	}

	// parse signedness specifiers
	SignednessSpecifier signedness = ParseSignednessSpecifier();
	SizeSpecifier size = ParseSizeSpecifier();
	std::string declarator;

	if (signedness != SignednessSpecifier::kNone || size != SizeSpecifier::kNone) {
		if (signedness != SignednessSpecifier::kNone) {
			declarator.append(Signedness2String(signedness));
		}
		if (size != SizeSpecifier::kNone) {
			if (!declarator.empty()) {
				declarator.push_back(' ');
			}
			declarator.append(Size2String(size));
		}
		// parse a C base type
		if (ParseBaseType(token)) {
			if (!declarator.empty()) {
				declarator.push_back(' ');
			}
			declarator.append(token.token);
		}
	} else {
		// Parse a literal value
		if (!ParseTypeNodeDeclarator(declarator, constructorName, !inTemplate)) {
			return nullptr;
		}
	}

	// Postfix const specifier
	isConst = isConst || MatchIdentifier("const");

	// Template?
	if (MatchSymbol("<"))
	{
		std::unique_ptr<TemplateNode> templateNode(new TemplateNode(declarator));
		do
		{
			auto node = ParseTypeNode(constructorName);
			if (node == nullptr)
				return nullptr;

			templateNode->arguments.emplace_back(std::move(node));
		} while (MatchSymbol(","));

		if (!MatchSymbol(">"))
		{
			Error("Expected '>'");
			return nullptr;
		}

		node.reset(templateNode.release());

		if (MatchSymbol("::")) {
			auto selector = ParseTypeNode(std::string_view());
			if (!selector) {
				Error("Expected type declarator");
				return nullptr;
			}
			selector->parent = std::move(node);
			node.reset(selector.release());
		}
	}
	else
	{
		node.reset(new LiteralNode(declarator));
		if (declarator.length() >= 3 && declarator.substr(declarator.length() - 3) == "...") {
			node->type = TypeNode::Type::kVariadic;
			return std::move(node);
		}
		else if (declarator == constructorName) {
			node->type = TypeNode::Type::kConstructor;
			return std::move(node);
		}
		else if (declarator[0] == '~') {
			node->type = TypeNode::Type::kDestructor;
			return std::move(node);
		}
	}

	// Store gathered stuff
	node->specifiers.isConst = isConst;
	node->signedness = signedness;
	node->size = size;

	// Check reference or pointer types
	while (GetToken(token))
	{
		if (token.token == "&")
			node.reset(new ReferenceNode(std::move(node)));
		else if (token.token == "&&")
			node.reset(new LReferenceNode(std::move(node)));
		else if (token.token == "*")
			node.reset(new PointerNode(std::move(node)));
		else
		{
			UngetToken(token);
			break;
		}

		if (MatchIdentifier("const"))
			node->specifiers.isConst = true;
	}

	// Function pointer?
	if (MatchSymbol("("))
	{
		std::unique_ptr<FunctionNode> funcNode(new FunctionNode());
		// Parse void(*)(args, ...)
		//            ^
		//            |
		Token token;
		bool isFunctionPointer = MatchSymbol("*");
		bool hasTypedef = GetIdentifier(token);
		bool hasTypedefEnd = hasTypedef && MatchSymbol(")") && MatchSymbol("(");

		if (hasTypedef) {
			funcNode->name = token.token;
		}
		if (isFunctionPointer) {
			funcNode->type = TypeNode::Type::kFunctionPointer;
		}

		// Parse arguments
		
		funcNode->returns = std::move(node);

		if (!MatchSymbol(")"))
		{
			do
			{
				std::unique_ptr<FunctionNode::Argument> argument(new FunctionNode::Argument);
				argument->type = ParseTypeNode(std::string());
				if (argument->type == nullptr)
					return nullptr;

				// Get , or name identifier
				if (!GetToken(token))
				{
					Error("Unexpected end of file");
					return nullptr;
				}

				// Parse optional name
				if (token.tokenType == TokenType::kIdentifier)
					argument->name = token.token;
				else
					UngetToken(token);

				funcNode->arguments.emplace_back(std::move(argument));

			} while (MatchSymbol(","));
			if (!MatchSymbol(")")) {
				Error("Expected ')'");
				return nullptr;
			}
		}

		node = std::move(funcNode);
	}

	// This stuff refers to the top node
	node->specifiers.isVolatile = isVolatile;
	node->specifiers.isMutable = isMutable;

	return std::move(node);
}

//-------------------------------------------------------------------------------------------------
bool Parser::ParseTypeNodeDeclarator(std::string &declarator, const std::string_view& constructorName, bool checkSpecifier)
{
	// optional forward declaration specifier
	Token specifier;
	if (!GetToken(specifier)) {
		return false;
	}

	bool hasSpecifier = false;
	if (checkSpecifier) {
		hasSpecifier = isSpecifier(specifier.token);
	}
	if (!hasSpecifier) {
		UngetToken(specifier);
	} else {
		declarator = specifier.token;
	}

	// Parse a type name
	Token token;
	bool first = true;
	do
	{
		// TODO: match base type
		// Parse the declarator
		if (MatchSymbol("...")) {
			declarator.append("...");
			return true;
		}
		else if (MatchSymbol("~")) {
			Token token;
			if (!GetIdentifier(token)) {
				return Error("Identifier expected");
			}
			if (token.token != constructorName) {
				return Error("Invalid destructor name");
			}
			declarator = std::string("~").append(token.token);
			if (!RequireSymbol("(")) {
				return false;
			}
			UngetToken(token);
			--cursorPos_;
			return true;
		}
		else if (MatchSymbol("::")) {
			declarator += "::";
		}
		else if (!first) {
			break;
		}
		
		// Mark that this is not the first time in this loop
		first = false;

		// Match an identifier or constant
		if (GetIdentifier(token)) {
			if (token.token == constructorName) {
				if (MatchSymbol("(")) {
					UngetToken(token);
					declarator = constructorName;
					return true;
				}
			}
			UngetToken(token);
		}
		if (!GetIdentifier(token) && !GetConst(token))
			return false;

		declarator += token.token;

	} while (true);

	return true;
}

//----------------------------------------------------------------------------------------------------------------------
/*
void Parser::WriteToken(const Token& token)
{
	if(token.tokenType == TokenType::kConst)
	{
		switch(token.constType)
		{
		case ConstType::kBoolean:
			writer_.constant(token.boolConst);
			break;
		case ConstType::kUInt32:
			writer_.constant(token.uint32Const);
			break;
		case ConstType::kInt32:
			writer_.constant(token.int32Const);
			break;
		case ConstType::kUInt64:
			writer_.constant(token.uint64Const);
			break;
		case ConstType::kInt64:
			writer_.constant(token.int64Const);
			break;
		case ConstType::kReal:
			writer_.constant(token.realConst);
			break;
		case ConstType::kString:
			//writer_.String((std::string("\"") + token.stringConst + "\"").c_str());
			writer_.constant(token.stringConst);
			break;
		}
	}
	else
		writer_.constant(token.token);
}
*/
//-------------------------------------------------------------------------------------------------
bool Parser::ParseTemplate()
{
	if(!RequireSymbol("<"))
		return false;

	writer_.beginTemplate();
	if (!MatchSymbol(">")) {
		do
		{
			if (!ParseTemplateArgument())
				return false;
		} while (MatchSymbol(","));

		if (!RequireSymbol(">"))
			return false;
	}

	writer_.endTemplate();
	return true;
}

//-------------------------------------------------------------------------------------------------
bool Parser::ParseTemplateArgument()
{
	Token token;
	if (!ParseType(nullptr, true, std::string_view(), nullptr, true)) {
		return Error("Expected type or specifier");
	}
		
	// Parse the template argument name
	std::string_view name;
	if (GetIdentifier(token)) {
		name = token.token;
	}

	// Optionally check if there is a default initializer
	bool hasDefaultType = false;
	if (MatchSymbol("=")) {
		if (!ParseType()) {
			return false;
		}
		hasDefaultType = true;
	}

	writer_.templateArgument(name, hasDefaultType);

	return true;
}

std::string Parser::GenerateUnnamedIdentifier(const std::string_view &type)
{
	return std::string("unnamed-").append(type).append(std::to_string(m_unnamedCnt++));
}

void Parser::SetInterface(ParserInterface& i)
{
	writer_ = i;
}
