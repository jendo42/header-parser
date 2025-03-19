#include "TypeData.h"

static std::string StorageSpecifierString(Specifiers specifiers)
{
	std::string str;

	if (specifiers.isStatic) {
		str.append("static ");
	}
	if (specifiers.isConstExpr) {
		str.append("constexpr ");
	}
	if (specifiers.isInline) {
		str.append("inline ");
	}
	else if (specifiers.isConst) {
		str.append("const ");
	}
	else if (specifiers.isMutable) {
		return "mutable ";
	}
	if (specifiers.isVolatile) {
		str.append("volatile ");
	}
	return str;
}

std::string TypeData::ToString() const
{
	if (this == nullptr) {
		return std::string();
	}
	auto stor = StorageSpecifierString(specifiers);
	switch (type) {
	case TypeNode::Type::kPointer:
		return stor.append(children[0].ToString()).append("*").append(name);
	case TypeNode::Type::kReference:
		return stor.append(children[0].ToString()).append("&").append(name);
	case TypeNode::Type::kLReference:
		return stor.append(children[0].ToString()).append("&&").append(name);
	case TypeNode::Type::kLiteral:
		return !children.empty() ? stor.append(children[0].ToString()).append("::").append(name) : stor.append(name);
	case TypeNode::Type::kVariadic:
		return stor.append(name);
	case TypeNode::Type::kTemplate: {
		// TODO: storage specifier 
		std::string str = std::string(name).append("<");
		for (const auto& child : children) {
			str.append(child.ToString());
			str.append(",");
		}
		if (str.back() == ',') {
			str.pop_back();
		}
		str.push_back('>');
		return str;
	}
	case TypeNode::Type::kFunctionPointer:
	case TypeNode::Type::kFunction: {
		std::string str = children[0].ToString().append("(");
		if (type == TypeNode::Type::kFunctionPointer) {
			str.append("*").append(")(");
		}
		for (int i = 1; i < children.size(); i++) {
			str.append(children[i].ToString());
			str.append(",");
		}
		if (str.back() == ',') {
			str.pop_back();
		}
		str.append(")");
		return str;
	}
	case TypeNode::Type::kConstructor:
		return name;
	case TypeNode::Type::kDestructor:
		return "void";
	case TypeNode::Type::kNone:
		return "";
	default:
		return "<unknown>";
	}
}
