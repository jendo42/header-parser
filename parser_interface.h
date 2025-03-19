#pragma once
#include <stdint.h>
#include <iostream>
#include "type_node.h"

enum class ScopeType
{
	kUnknown,
	kGlobal,
	kNamespace,
	kClass,
	kStructure,
	kUnion
};

enum class AccessControlType
{
	kPublic,
	kPrivate,
	kProtected
};

class ParserInterface
{
public:
	virtual void destroy() = 0;
	virtual void begin(const std::string_view& source) = 0;
	virtual void end(const std::string_view& source, const std::string_view &error) = 0;

	virtual void include(const std::string_view& filename) = 0;
	virtual void comment(const std::string_view& comment) = 0;
	virtual void access(AccessControlType act) = 0;
	virtual void using_(bool hasAssigment) = 0;
	virtual void friend_() = 0;

	virtual void beginEnum(int startLine, const std::string_view& name, const std::string_view& base, bool isEnumClass) = 0;
	virtual void enumValue(const std::string_view& key, const std::string_view& value) = 0;
	virtual void endEnum(const std::string_view& name) = 0;

	virtual void beginClass(int startLine, const std::string_view& name, ScopeType type) = 0;
	virtual void baseType() = 0;
	virtual void endClass(const std::string_view& name, bool forwardDecl) = 0;

	virtual void beginNamespace(const std::string_view& name) = 0;
	virtual void endNamespace(const std::string_view& name) = 0;

	virtual void beginTemplate() = 0;
	virtual void templateArgument(const std::string_view&name, bool hasDefaultType) = 0;
	virtual void endTemplate() = 0;

	virtual void beginType(TypeNode::Type type, Specifiers specifiers) = 0;
	virtual void typeName(const std::string_view& name) = 0;
	virtual void endType() = 0;

	virtual void beginProperty(int startLine, const std::string_view& name, Specifiers specifiers) = 0;
	virtual void arraySubscript(const std::string_view& name) = 0;
	virtual void endProperty(const std::string_view& name) = 0;

	virtual void beginFunction(int startLine, TypeNode::Type type, const std::string_view& name) = 0;
	virtual void functionArgument(const std::string_view& name, const std::string_view& defaultValue) = 0;
	virtual void endFunction(const std::string_view& name, Specifiers specifiers) = 0;

	virtual void beginTypedef(int startLine, const std::string_view& name) = 0;
	virtual void endTypedef(const std::string_view& name) = 0;

	virtual void beginMacro(const std::string_view& name) = 0;
	virtual void macroArgument(const std::string_view& name) = 0;
	virtual void endMacro(const std::string_view& name) = 0;

	/*
	virtual void constant(bool b) = 0;
	virtual void constant(uint32_t b) = 0;
	virtual void constant(int32_t b) = 0;
	virtual void constant(uint64_t b) = 0;
	virtual void constant(int64_t b) = 0;
	virtual void constant(double b) = 0;
	virtual void constant(const std::string &b) = 0;
	*/

	static std::string_view ScopeType2String(ScopeType type)
	{
		switch (type) {
		case ScopeType::kGlobal:
			return "global";
		case ScopeType::kNamespace:
			return "namespace";
		case ScopeType::kClass:
			return "class";
		case ScopeType::kStructure:
			return "struct";
		case ScopeType::kUnion:
			return "union";
		default:
			return "unknown";
		}
	}
};
