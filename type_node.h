#pragma once

#include <vector>
#include <memory>
#include <string>

enum class SignednessSpecifier
{
	kNone,
	kSigned,
	kUnsigned
};

enum class SizeSpecifier
{
	kNone,
	kShort,
	kLong,
	kLongLong,
};

struct Specifiers
{
	unsigned isInline : 1;
	unsigned isVirtual : 1;
	unsigned isConstExpr : 1;
	unsigned isStatic : 1;
	unsigned isDefault : 1;
	unsigned isConstThis : 1;
	unsigned isOverride : 1;
	unsigned isAbstract : 1;
	unsigned isConst : 1;
	unsigned isVolatile : 1;
	unsigned isMutable : 1;
	unsigned isDeleted : 1;

	std::string ToString() const
	{
		std::string out;
		if (isInline) {
			out.push_back('i');
		}
		if (isVirtual) {
			out.push_back('v');
		}
		if (isConstExpr) {
			out.push_back('x');
		}
		if (isStatic) {
			out.push_back('s');
		}
		if (isDefault) {
			out.push_back('d');
		}
		if (isConstThis) {
			out.push_back('t');
		}
		if (isOverride) {
			out.push_back('o');
		}
		if (isAbstract) {
			out.push_back('a');
		}
		if (isConst) {
			out.push_back('c');
		}
		if (isVolatile) {
			out.push_back('l');
		}
		if (isMutable) {
			out.push_back('m');
		}
		return out;
	}
};

struct TypeNode
{
	enum class Type
	{
		kNone,
		kPointer,
		kReference,
		kLReference,
		kLiteral,
		kTemplate,
		kFunction,
		kVariadic,
		kConstructor,
		kDestructor,
		kFunctionPointer,
	};

	TypeNode(Type t) :
		type(t) {}

	virtual ~TypeNode() = default;

	Specifiers specifiers{};
	SignednessSpecifier signedness = SignednessSpecifier::kNone;
	SizeSpecifier size = SizeSpecifier::kNone;

	Type type;
	std::unique_ptr<TypeNode> parent = nullptr;
};

struct PointerNode : public TypeNode
{
	PointerNode(std::unique_ptr<TypeNode> && b) :
		TypeNode(TypeNode::Type::kPointer),
		base(std::forward<std::unique_ptr<TypeNode>>(b)){}

	std::unique_ptr<TypeNode> base;
};

struct ReferenceNode : public TypeNode
{
	ReferenceNode(std::unique_ptr<TypeNode> && b) :
		TypeNode(TypeNode::Type::kReference),
		base(std::forward<std::unique_ptr<TypeNode>>(b)){}

	std::unique_ptr<TypeNode> base;
};

struct LReferenceNode : public TypeNode
{
	LReferenceNode(std::unique_ptr<TypeNode> && b) :
		TypeNode(TypeNode::Type::kLReference),
		base(std::forward<std::unique_ptr<TypeNode>>(b)){}

	std::unique_ptr<TypeNode> base;
};

struct TemplateNode : public TypeNode
{
	TemplateNode(const std::string_view& n) :
		TypeNode(TypeNode::Type::kTemplate),
		name(n) {}

	std::string name;
	std::vector<std::unique_ptr<TypeNode>> arguments;
};

struct LiteralNode : public TypeNode
{
	LiteralNode(const std::string_view& ref) :
		TypeNode(TypeNode::Type::kLiteral),
		name(ref)
		{}

	std::string name;
};

struct FunctionNode : public TypeNode
{
	FunctionNode()
		: TypeNode(TypeNode::Type::kFunction)
	{}

	struct Argument
	{
		std::string name;
		std::unique_ptr<TypeNode> type;
	};

	std::string name;
	std::unique_ptr<TypeNode> returns;
	std::vector<std::unique_ptr<Argument>> arguments;
};

struct ITypeNodeVisitor
{
	virtual void VisitNode(TypeNode &node, const std::string_view& name = std::string_view()) = 0;
	virtual void Visit(PointerNode& node) = 0;
	virtual void Visit(ReferenceNode& node) = 0;
	virtual void Visit(LReferenceNode& node) = 0;
	virtual void Visit(LiteralNode& node) = 0;
	virtual void Visit(TemplateNode& node) = 0;
	virtual void Visit(FunctionNode& node) = 0;
};

struct TypeNodeVisitor : public ITypeNodeVisitor
{
	void VisitNode(TypeNode &node, const std::string_view& name = std::string_view()) override
	{
		switch (node.type)
		{
		case TypeNode::Type::kPointer:
			Visit(reinterpret_cast<PointerNode&>(node));
			break;
		case TypeNode::Type::kReference:
			Visit(reinterpret_cast<ReferenceNode&>(node));
			break;
		case TypeNode::Type::kLReference:
			Visit(reinterpret_cast<LReferenceNode&>(node));
			break;
		case TypeNode::Type::kLiteral:
		case TypeNode::Type::kConstructor:
		case TypeNode::Type::kDestructor:
		case TypeNode::Type::kVariadic:
			Visit(reinterpret_cast<LiteralNode&>(node));
			break;
		case TypeNode::Type::kTemplate:
			Visit(reinterpret_cast<TemplateNode&>(node));
			break;
		case TypeNode::Type::kFunction:
		case TypeNode::Type::kFunctionPointer:
			Visit(reinterpret_cast<FunctionNode&>(node));
			break;
		}
	}
};
