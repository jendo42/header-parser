#include <cstdarg>
#include <cassert>
#include <fstream>

#include "DebugParserInterface.h"
#include "helpers.h"

DebugParserInterface::DebugParserInterface(ParserInterface& output)
	: ParserInterface()
	, m_interface(output)
	, m_indent(0)
	, m_typeData()
	, m_typeStack()
	, m_doneTypes()
{

}

void DebugParserInterface::destroy()
{
	m_interface.destroy();
	delete this;
}

void DebugParserInterface::begin(const std::string_view& source)
{
	this->printf("begin(%.*s)", source.length(), source.data());
	++m_indent;
	m_interface.begin(source);
}

void DebugParserInterface::end(const std::string_view& source, const std::string_view &error)
{
	this->printf("end(%.*s)", source.length(), source.data());
	--m_indent;
	m_interface.end(source, error);
}

void DebugParserInterface::include(const std::string_view& filename)
{
	this->printf("include(%.*s)", filename.length(), filename.data());
	m_interface.include(filename);
}

void DebugParserInterface::comment(const std::string_view& comment)
{
	this->printf("comment(%.*s)", comment.length(), comment.data());
	m_interface.comment(comment);
}

void DebugParserInterface::access(AccessControlType act)
{
	this->printf("access(%d)", act);
	m_interface.access(act);
}

void DebugParserInterface::using_(bool hasAssigment)
{
	TypeData type1, type2;
	TypeData *key, *value;
	assert(takeType(type1) == true);

	if (hasAssigment) {
		assert(takeType(type2) == true);
		value = &type1;
		key = &type2;
	} else {
		key = &type1;
	}
	if (hasAssigment) {
		this->printf("using_(%s) -> %s", key->ToString().c_str(), value->ToString().c_str());
	} else {
		this->printf("using_(%s)", key->ToString().c_str());
	}

	m_interface.using_(hasAssigment);
}

void DebugParserInterface::friend_()
{
	TypeData type;
	assert(takeType(type) == true);
	this->printf("friend(%s)", type.ToString().c_str());
}

void DebugParserInterface::beginEnum(int startLine, const std::string_view& name, const std::string_view& base, bool isEnumClass)
{
	this->printf("beginEnum(%d, %.*s, %.*s, %s)", startLine, name.length(), name.data(), base.length(), base.data(), isEnumClass ? "true" : "false");
	++m_indent;
	m_interface.beginEnum(startLine, name, base, isEnumClass);
}

void DebugParserInterface::enumValue(const std::string_view& key, const std::string_view& value)
{
	if (value.empty()) {
		this->printf("enumValue(%.*s)", key.length(), key.data());
	} else {
		this->printf("enumValue(%.*s = %.*s)", key.length(), key.data(), value.length(), value.data());
	}
	m_interface.enumValue(key, value);
}

void DebugParserInterface::endEnum(const std::string_view& name)
{
	this->printf("endEnum(%.*s)", name.length(), name.data());
	--m_indent;
	m_interface.endEnum(name);
}

void DebugParserInterface::beginClass(int startLine, const std::string_view& name, ScopeType type)
{
	this->printf("beginClass(%d, %.*s, %d)", startLine, name.length(), name.data(), type);
	++m_indent;
	m_interface.beginClass(startLine, name, type);
}

void DebugParserInterface::baseType()
{
	TypeData type;
	assert(takeType(type) == true);
	this->printf("baseType() -> %s", type.ToString().c_str());
	m_interface.baseType();
}

void DebugParserInterface::endClass(const std::string_view& name, bool forwardDecl)
{
	this->printf("endClass(%.*s, %d)", name.length(), name.data(), forwardDecl);
	--m_indent;
	m_interface.endClass(name, forwardDecl);
}

void DebugParserInterface::beginNamespace(const std::string_view& name)
{
	this->printf("beginNamespace(%.*s)", name.length(), name.data());
	++m_indent;
	m_interface.beginNamespace(name);
}

void DebugParserInterface::endNamespace(const std::string_view& name)
{
	this->printf("endNamespace(%.*s)", name.length(), name.data());
	--m_indent;
	m_interface.endNamespace(name);
}

void DebugParserInterface::beginTemplate()
{
	this->printf("beginTemplate()");
	++m_indent;
	m_interface.beginTemplate();
}

void DebugParserInterface::templateArgument(const std::string_view& name, bool hasDefaultType)
{
	TypeData defaultType;
	TypeData templateParameterType;
	if (hasDefaultType) {
		assert(takeType(defaultType) == true);
	}
	assert(takeType(templateParameterType) == true);
	if (hasDefaultType) {
		this->printf("templateArgument(%.*s = %s) -> %s", name.length(), name.data(), defaultType.ToString().c_str(), templateParameterType.ToString().c_str());
	} else {
		this->printf("templateArgument(%.*s) -> %s", name.length(), name.data(), templateParameterType.ToString().c_str());
	}
	m_interface.templateArgument(name, hasDefaultType);
}

void DebugParserInterface::endTemplate()
{
	this->printf("endTemplate()");
	--m_indent;
	m_interface.endTemplate();
}

void DebugParserInterface::beginType(TypeNode::Type type, Specifiers specifiers)
{
	TypeData* newTop;
	auto top = typeTop();
	if (!top) {
		newTop = &m_typeData;
	} else {
		newTop = &top->children.emplace_back();
	}

	*newTop = TypeData{
			type, m_access, specifiers, "", std::vector<TypeData>()
	};

	m_typeStack.push_back(newTop);
	m_interface.beginType(type, specifiers);
}

void DebugParserInterface::typeName(const std::string_view& name)
{
	auto top = typeTop();
	assert(top != nullptr);
	top->name = name;
	m_interface.typeName(name);
}

void DebugParserInterface::endType()
{
	auto top = typeTop();
	assert(top != nullptr);
	m_typeStack.pop_back();
	if (m_typeStack.empty()) {
		m_doneTypes.push_back(m_typeData);
	}
	m_interface.endType();
}

void DebugParserInterface::beginProperty(int startLine, const std::string_view& name, Specifiers specifiers)
{
	TypeData type;
	assert(takeType(type) == true);
	this->printf("beginProperty(%d, %.*s, %s) -> %s", startLine, name.length(), name.data(), specifiers.ToString().c_str(), type.ToString().c_str());
	++m_indent;
	m_interface.beginProperty(startLine, name, specifiers);
}

void DebugParserInterface::arraySubscript(const std::string_view& name)
{
	this->printf("arraySubscript(%.*s)", name.length(), name.data());
	m_interface.arraySubscript(name);
}

void DebugParserInterface::endProperty(const std::string_view& name)
{
	this->printf("endProperty(%.*s)", name.length(), name.data());
	--m_indent;
	m_interface.endProperty(name);
}

void DebugParserInterface::beginFunction(int startLine, TypeNode::Type type, const std::string_view& name)
{
	TypeData returnType;
	assert(takeType(returnType) == true);
	this->printf("beginFunction(%d, %d, %.*s) -> %s", startLine, type, name.length(), name.data(), returnType.ToString().c_str());
	++m_indent;
	m_interface.beginFunction(startLine, type, name);
}

void DebugParserInterface::functionArgument(const std::string_view& name, const std::string_view& defaultValue)
{
	TypeData type;
	assert(takeType(type) == true);
	if (name.empty() || defaultValue.empty()) {
		const std::string_view& str = name.empty() ? defaultValue : name;
		this->printf("functionArgument(%.*s) -> %s", str.length(), str.data(), type.ToString().c_str());
	}
	else {
		this->printf("functionArgument(%.*s = %.*s) -> %s", name.length(), name.data(), defaultValue.length(), defaultValue.data(), type.ToString().c_str());
	}
	m_interface.functionArgument(name, defaultValue);
}

void DebugParserInterface::endFunction(const std::string_view& name, Specifiers specifiers)
{
	this->printf("endFunction(%.*s, %s)", name.length(), name.data(), specifiers.ToString().c_str());
	--m_indent;
	m_interface.endFunction(name, specifiers);
}

void DebugParserInterface::beginTypedef(int startLine, const std::string_view& name)
{
	TypeData type;
	assert(takeType(type) == true);
	this->printf("beginTypedef(%d, %.*s) -> %s", startLine, name.length(), name.data(), type.ToString().c_str());
	++m_indent;
	m_interface.beginTypedef(startLine, name);
}

void DebugParserInterface::endTypedef(const std::string_view& name)
{
	this->printf("endTypedef(%.*s)", name.length(), name.data());
	--m_indent;
	m_interface.endTypedef(name);
}

void DebugParserInterface::beginMacro(const std::string_view& name)
{
	this->printf("beginMacro(%.*s)", name.length(), name.data());
	++m_indent;
	m_interface.beginMacro(name);
}

void DebugParserInterface::macroArgument(const std::string_view& name)
{
	this->printf("macroArgument(%.*s)", name.length(), name.data());
	m_interface.macroArgument(name);
}

void DebugParserInterface::endMacro(const std::string_view& name)
{
	this->printf("endMacro(%.*s)", name.length(), name.data());
	--m_indent;
	m_interface.endMacro(name);
}
/*
void DebugParserInterface::constant(bool b)
{
	this->printf("constant(%s)", b ? "true" : "false");
}

void DebugParserInterface::constant(uint32_t b)
{
	this->printf("constant(0x%.8X)", b);
}

void DebugParserInterface::constant(int32_t b)
{
	this->printf("constant(%d)", b);
}

void DebugParserInterface::constant(uint64_t b)
{
	this->printf("constant(%.8lX)", b);
}

void DebugParserInterface::constant(int64_t b)
{
	this->printf("constant(%ld)", b);
}

void DebugParserInterface::constant(double b)
{
	this->printf("constant(%f)", b);
}

void DebugParserInterface::constant(const std::string &b)
{
	this->printf("constant(%s)", b.c_str());
}
*/
void DebugParserInterface::printf(const char* format, ...)
{
	assert(m_indent >= 0);
	char buffer[4096];
	va_list args;
	va_start(args, format);
	for (int i = 0; i < m_indent; i++) {
		buffer[i] = '\t';
	}
	int size = vsprintf(buffer + m_indent, format, args);
	buffer[size + m_indent] = '\n';
	std::string_view str(buffer, size + m_indent + 1);
	m_output << str;
	DebugPrint(str);
	va_end(args);
}

TypeData* DebugParserInterface::typeTop() const
{
	return m_typeStack.empty() ? nullptr : m_typeStack.back();
}

bool DebugParserInterface::takeType(TypeData &out)
{
	if (m_doneTypes.empty()) {
		return false;
	}

	out = m_doneTypes.back();
	m_doneTypes.pop_back();
	return true;
}
