#pragma once
#include <vector>
#include <string>

#include "parser_interface.h"

struct TypeData
{
	TypeNode::Type type = TypeNode::Type::kNone;
	AccessControlType access = AccessControlType::kPublic;
	Specifiers specifiers{};

	std::string name;
	std::vector<TypeData> children;

	std::string ToString() const;
};

struct TemplateArgument
{
	std::string type;
	std::string name;
	std::string default;
};

typedef std::vector<TemplateArgument> TemplateData;
