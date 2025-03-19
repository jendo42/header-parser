#include <cstdarg>
#include <cassert>

#include <windows.h>

#include "TypeDbParserInterface.h"

TypeDbParserInterface::TypeDbParserInterface(std::string* typedbFile)
	: ParserInterface()
	, m_typedbFile(typedbFile)
	, m_document()
	, m_typeData()
	, m_typeStack()
	, m_doneTypes()
{

}

void TypeDbParserInterface::destroy()
{
	if (m_typedbFile) {
		if (!m_document.save_file(m_typedbFile->c_str())) {
			m_document.save(std::cout);
		}
	}

	delete this;
}

void TypeDbParserInterface::begin(const std::string_view& source)
{
	m_access = AccessControlType::kPublic;

	auto node = pushElement("typedb", std::string());
	rewriteAttribute("version").set_value(1);
	rewriteAttribute("generator").set_value("reflector2");

	auto iteration = rewriteAttribute("iteration");
	iteration.set_value(iteration.as_ullong() + 1);
}

void TypeDbParserInterface::end(const std::string_view& source, const std::string_view &error)
{
	if (std::string("typedb") != nodeTop().name()) {
		// recover stack
		m_nodeStack.clear();
		pushElement("typedb", std::string());
	}
	
	// store source file info
	pushElement("source-map", std::string());
	pushElement("file").text().set(source);
	rewriteAttribute("id").set_value(std::hash<std::string_view>()(source));
	rewriteAttribute("error").set_value(error);
	assert(popElement() == true);
	assert(popElement() == true);

	// save output data
	if (!m_typedbFile) {
		std::string output;
		auto pos = source.find_last_of("/\\");
		if (pos == std::string::npos) {
			output = source;
		}
		else {
			output = source.substr(pos + 1);
		}
		output.append(".typedb");

		if (!m_document.save_file(output.c_str())) {
			m_document.save(std::cout);
		}
	}

	assert(popElement() == true);
}

void TypeDbParserInterface::include(const std::string_view& filename)
{

}

void TypeDbParserInterface::comment(const std::string_view& c)
{

}

void TypeDbParserInterface::access(AccessControlType act)
{
	m_access = act;
}

void TypeDbParserInterface::using_(bool hasAssigment)
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
}

void TypeDbParserInterface::friend_()
{
	TypeData type;
	assert(takeType(type) == true);
}

void TypeDbParserInterface::beginEnum(int startLine, const std::string_view& name, const std::string_view& base, bool isEnumClass)
{

}

void TypeDbParserInterface::enumValue(const std::string_view& key, const std::string_view& value)
{

}

void TypeDbParserInterface::endEnum(const std::string_view& name)
{

}

static std::string_view Access2String(AccessControlType type)
{
	switch (type) {
	case AccessControlType::kPublic:
		return "public";
	case AccessControlType::kProtected:
		return "protected";
	case AccessControlType::kPrivate:
		return "private";
	default:
		return "unknown";
	}
}

void TypeDbParserInterface::beginClass(int startLine, const std::string_view& name, ScopeType type)
{
	auto node = pushElement(ScopeType2String(type), name);
	processTemplate();
	//assert(node.attribute("forwarded").as_bool(true) == true);
}

void TypeDbParserInterface::baseType()
{
	TypeData type;
	assert(takeType(type) == true);
	auto node = nodeTop().append_child("base");
	node.append_attribute("access").set_value(Access2String(m_access));
	node.text().set(type.ToString());
}

void TypeDbParserInterface::endClass(const std::string_view& name, bool forwardDecl)
{
	rewriteAttribute("forwarded").set_value(forwardDecl);
	assert(popElement() == true);
}

void TypeDbParserInterface::beginNamespace(const std::string_view& name)
{
	auto node = pushElement("namespace", name);
}

void TypeDbParserInterface::endNamespace(const std::string_view& name)
{
	assert(popElement() == true);
}

void TypeDbParserInterface::beginTemplate()
{
	m_templateData.clear();
}

void TypeDbParserInterface::templateArgument(const std::string_view& name, bool hasDefaultType)
{
	TypeData defaultTyp;
	TypeData paramTyp;
	if (hasDefaultType) {
		assert(takeType(defaultTyp) == true);
	}
	assert(takeType(paramTyp) == true);
	m_templateData.emplace_back(TemplateArgument{
		paramTyp.ToString(), std::string(name), defaultTyp.ToString()
	});
}

void TypeDbParserInterface::endTemplate()
{
	m_doneTemplates.push_back(m_templateData);
}

void TypeDbParserInterface::beginType(TypeNode::Type type, Specifiers specifiers)
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
}

void TypeDbParserInterface::typeName(const std::string_view& name)
{
	auto top = typeTop();
	assert(top != nullptr);
	top->name = name;
}

void TypeDbParserInterface::endType()
{
	auto top = typeTop();
	assert(top != nullptr);
	m_typeStack.pop_back();
	if (m_typeStack.empty()) {
		m_doneTypes.push_back(m_typeData);
	}
}

void TypeDbParserInterface::beginProperty(int startLine, const std::string_view& name, Specifiers specifiers)
{
	TypeData type;
	assert(takeType(type) == true);
	pushElement("property", name);
	rewriteAttribute("spec").set_value(specifiers.ToString());
	rewriteAttribute("type").set_value(type.ToString());
}

void TypeDbParserInterface::arraySubscript(const std::string_view& name)
{
	rewriteAttribute("subscript").set_value(name);
}

void TypeDbParserInterface::endProperty(const std::string_view& name)
{
	assert(popElement() == true);
}

void TypeDbParserInterface::beginFunction(int startLine, TypeNode::Type type, const std::string_view& name)
{
	TypeData returnType;
	assert(takeType(returnType) == true);

	auto node = pushElement("function", name);
	rewriteAttribute("access").set_value(Access2String(m_access));
	rewriteAttribute("returns").set_value(returnType.ToString());

	processTemplate();
}

void TypeDbParserInterface::functionArgument(const std::string_view& name, const std::string_view& defaultValue)
{
	TypeData type;
	assert(takeType(type) == true);
	auto node = nodeTop().append_child("argument");
	node.append_attribute("name").set_value(name);
	node.append_attribute("type").set_value(type.ToString());
}

void TypeDbParserInterface::endFunction(const std::string_view& name, Specifiers specifiers)
{
	rewriteAttribute("spec").set_value(specifiers.ToString());
	assert(popElement() == true);
}

void TypeDbParserInterface::beginTypedef(int startLine, const std::string_view& name)
{
	TypeData type;
	assert(takeType(type) == true);
	pushElement("typedef", name);
	rewriteAttribute("type").set_value(type.ToString());
}

void TypeDbParserInterface::endTypedef(const std::string_view& name)
{
	assert(popElement() == true);
}

void TypeDbParserInterface::beginMacro(const std::string_view& name)
{

}

void TypeDbParserInterface::macroArgument(const std::string_view& name)
{

}

void TypeDbParserInterface::endMacro(const std::string_view& name)
{

}
/*
void TypeDbParserInterface::constant(bool b)
{
	this->printf("constant(%s)", b ? "true" : "false");
}

void TypeDbParserInterface::constant(uint32_t b)
{
	this->printf("constant(0x%.8X)", b);
}

void TypeDbParserInterface::constant(int32_t b)
{
	this->printf("constant(%d)", b);
}

void TypeDbParserInterface::constant(uint64_t b)
{
	this->printf("constant(%.8lX)", b);
}

void TypeDbParserInterface::constant(int64_t b)
{
	this->printf("constant(%ld)", b);
}

void TypeDbParserInterface::constant(double b)
{
	this->printf("constant(%f)", b);
}

void TypeDbParserInterface::constant(const std::string &b)
{
	this->printf("constant(%s)", b.c_str());
}
*/

TypeData* TypeDbParserInterface::typeTop() const
{
	return m_typeStack.empty() ? nullptr : m_typeStack.back();
}

bool TypeDbParserInterface::takeType(TypeData &out)
{
	if (m_doneTypes.empty()) {
		return false;
	}

	out = m_doneTypes.back();
	m_doneTypes.pop_back();
	return true;
}

bool TypeDbParserInterface::takeTemplate(TemplateData& out)
{
	if (m_doneTemplates.empty()) {
		return false;
	}

	out = m_doneTemplates.back();
	m_doneTemplates.pop_back();
	return true;
}

bool TypeDbParserInterface::processTemplate()
{
	TemplateData templateData;
	if (takeTemplate(templateData)) {
		rewriteAttribute("template").set_value(true);
		for (const auto& param : templateData) {
			pushElement("template-argument", param.name);
			rewriteAttribute("specifier").set_value(param.type);
			rewriteAttribute("default").set_value(param.default);
			assert(popElement() == true);
		}
		return true;
	}
	return false;
}

pugi::xml_node TypeDbParserInterface::nodeTop() const
{
	return m_nodeStack.empty() ? m_document.root() : m_nodeStack.back();
}

pugi::xml_node TypeDbParserInterface::rewriteChild(const std::string_view& name, const std::string_view& attrName)
{
	auto node = !attrName.empty() ? nodeTop().find_child_by_attribute(std::string(name).c_str(), "name", std::string(attrName).c_str()) : nodeTop().child(name);
	if (!node) {
		node = nodeTop().append_child(name);
		if (!attrName.empty()) {
			node.append_attribute("name").set_value(attrName);
		}
	}

	return node;
}

pugi::xml_attribute TypeDbParserInterface::rewriteAttribute(const std::string_view& name)
{
	auto attr = nodeTop().attribute(name);
	if (!attr) {
		attr = nodeTop().append_attribute(name);
	}
	return attr;
}

pugi::xml_node TypeDbParserInterface::pushElement(const std::string_view& name, const std::string_view& attrName)
{
	auto node = rewriteChild(name, attrName);
	m_nodeStack.emplace_back(node);
	return nodeTop();
}

pugi::xml_node TypeDbParserInterface::pushElement(const std::string_view& name)
{
	auto node = nodeTop().append_child(name);
	m_nodeStack.emplace_back(node);
	return node;
}

bool TypeDbParserInterface::popElement()
{
	if (!m_nodeStack.empty()) {
		m_nodeStack.pop_back();
		return true;
	}
	return false;
}

