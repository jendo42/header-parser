#include <string>
#include <deque>
#include <iostream>

#include <pugixml.hpp>

#include "parser_interface.h"
#include "TypeData.h"

class TypeDbParserInterface
	: public ParserInterface
{
public:
	TypeDbParserInterface(std::string *typedbFile);

	void destroy() override;
	void begin(const std::string_view& source) override;
	void end(const std::string_view& source, const std::string_view &error) override;

	void include(const std::string_view& filename) override;
	void comment(const std::string_view& c) override;
	void access(AccessControlType act) override;
	void using_(bool hasAssigment) override;
	void friend_() override;

	void beginEnum(int startLine, const std::string_view& name, const std::string_view& base, bool isEnumClass) override;
	void enumValue(const std::string_view& key, const std::string_view& value) override;
	void endEnum(const std::string_view& name) override;

	void beginClass(int startLine, const std::string_view& name, ScopeType type) override;
	void baseType() override;
	void endClass(const std::string_view& name, bool forwardDecl) override;

	void beginNamespace(const std::string_view& name) override;
	void endNamespace(const std::string_view& name) override;

	void beginTemplate() override;
	void templateArgument(const std::string_view& name, bool hasDefaultType) override;
	void endTemplate() override;

	void beginType(TypeNode::Type type, Specifiers specifiers) override;
	void typeName(const std::string_view& name) override;
	void endType() override;

	void beginProperty(int startLine, const std::string_view& name, Specifiers specifiers) override;
	void arraySubscript(const std::string_view& name) override;
	void endProperty(const std::string_view& name) override;

	void beginFunction(int startLine, TypeNode::Type type, const std::string_view& name) override;
	void functionArgument(const std::string_view& name, const std::string_view& defaultValue) override;
	void endFunction(const std::string_view& name, Specifiers specifiers) override;

	void beginTypedef(int startLine, const std::string_view& name) override;
	void endTypedef(const std::string_view& name) override;

	void beginMacro(const std::string_view& name) override;
	void macroArgument(const std::string_view& name) override;
	void endMacro(const std::string_view& name) override;
	/*
	void constant(bool b) override;
	void constant(uint32_t b) override;
	void constant(int32_t b) override;
	void constant(uint64_t b) override;
	void constant(int64_t b) override;
	void constant(double b) override;
	void constant(const std::string &b) override;
	*/
private:
	TypeData* typeTop() const;
	bool takeType(TypeData &out);
	bool takeTemplate(TemplateData& out);
	bool processTemplate();

	pugi::xml_node nodeTop() const;
	pugi::xml_node rewriteChild(const std::string_view&name, const std::string_view& attrName);
	pugi::xml_attribute rewriteAttribute(const std::string_view& name);
	pugi::xml_node pushElement(const std::string_view& name, const std::string_view& attrName);
	pugi::xml_node pushElement(const std::string_view& name);
	bool popElement();

	std::string* m_typedbFile;
	pugi::xml_document m_document;
	pugi::xml_node m_sourceMap;
	std::deque<pugi::xml_node> m_nodeStack;

	AccessControlType m_access;

	TypeData m_typeData;
	std::deque<TypeData*> m_typeStack;
	std::deque<TypeData> m_doneTypes;

	TemplateData m_templateData;
	std::deque<TemplateData> m_doneTemplates;
};
