#include <cstdarg>
#include <cassert>
#include <fstream>

#include "ParserInterfaceSynchronizer.h"

ParserInterfaceSynchronizer::ParserInterfaceSynchronizer(const std::string& out, ParserInterface &target, ResultQueue &sharedQueue)
	: ParserInterface()
	, m_parserInterface(target)
	, m_queue()
	, m_resultQueue(sharedQueue)
	, m_outputFile(out)
	, m_unique(0)
{

}

void ParserInterfaceSynchronizer::destroy()
{
	delete this;
}

void ParserInterfaceSynchronizer::begin(const std::string_view& source)
{
	m_queue = OperationQueue();
	m_inputFile = source;
	auto &pi = m_parserInterface;
	auto src = std::string(source);
	enqueue([=, &pi]() {
		pi.begin(src);
	});
}

void ParserInterfaceSynchronizer::end(const std::string_view& source, const std::string_view &error)
{
	assert(m_inputFile == source);
	auto& pi = m_parserInterface;
	auto src = std::string(source);
	auto err = std::string(error);
	enqueue([=, &pi]() {
		pi.end(src, err);
	});
	m_resultQueue.push(Result{
		std::move(m_queue)
	});
}

void ParserInterfaceSynchronizer::include(const std::string_view& filename)
{
	auto& pi = m_parserInterface;
	auto fn = std::string(filename);
	enqueue([=, &pi]() {
		pi.include(fn);
	});
}

void ParserInterfaceSynchronizer::comment(const std::string_view& comment)
{
	auto& pi = m_parserInterface;
	auto com = std::string(comment);
	enqueue([=, &pi]() {
		pi.include(com);
	});
}

void ParserInterfaceSynchronizer::access(AccessControlType act)
{
	auto& pi = m_parserInterface;
	enqueue([=, &pi]() {
		pi.access(act);
	});
}

void ParserInterfaceSynchronizer::using_(bool hasAssigment)
{
	auto& pi = m_parserInterface;
	enqueue([=, &pi]() {
		pi.using_(hasAssigment);
	});
}

void ParserInterfaceSynchronizer::friend_()
{
	auto& pi = m_parserInterface;
	enqueue([=, &pi]() {
		pi.friend_();
	});
}

void ParserInterfaceSynchronizer::beginEnum(int startLine, const std::string_view& name, const std::string_view& base, bool isEnumClass)
{
	auto& pi = m_parserInterface;
	auto nam = std::string(name);
	auto bas = std::string(base);
	enqueue([=, &pi]() {
		pi.beginEnum(startLine, nam, bas, isEnumClass);
	});
}

void ParserInterfaceSynchronizer::enumValue(const std::string_view& key, const std::string_view& value)
{
	auto& pi = m_parserInterface;
	auto k = std::string(key);
	auto v = std::string(value);
	enqueue([=, &pi]() {
		pi.enumValue(k, v);
	});
}

void ParserInterfaceSynchronizer::endEnum(const std::string_view& name)
{
	auto& pi = m_parserInterface;
	auto nam = std::string(name);
	enqueue([=, &pi]() {
		pi.endEnum(nam);
	});
}

void ParserInterfaceSynchronizer::beginClass(int startLine, const std::string_view& name, ScopeType type)
{
	auto& pi = m_parserInterface;
	auto nam = std::string(name);
	enqueue([=, &pi]() {
		pi.beginClass(startLine, nam, type);
	});
}

void ParserInterfaceSynchronizer::baseType()
{
	auto& pi = m_parserInterface;
	enqueue([=, &pi]() {
		pi.baseType();
	});
}

void ParserInterfaceSynchronizer::endClass(const std::string_view& name, bool forwardDecl)
{
	auto& pi = m_parserInterface;
	auto nam = std::string(name);
	enqueue([=, &pi]() {
		pi.endClass(nam, forwardDecl);
	});
}

void ParserInterfaceSynchronizer::beginNamespace(const std::string_view& name)
{
	auto& pi = m_parserInterface;
	auto nam = std::string(name);
	enqueue([=, &pi]() {
		pi.beginNamespace(nam);
	});
}

void ParserInterfaceSynchronizer::endNamespace(const std::string_view& name)
{
	auto& pi = m_parserInterface;
	auto nam = std::string(name);
	enqueue([=, &pi]() {
		pi.endNamespace(nam);
	});
}

void ParserInterfaceSynchronizer::beginTemplate()
{
	auto& pi = m_parserInterface;
	enqueue([=, &pi]() {
		pi.beginTemplate();
	});
}

void ParserInterfaceSynchronizer::templateArgument(const std::string_view& name, bool hasDefaultType)
{
	auto& pi = m_parserInterface;
	auto nam = std::string(name);
	enqueue([=, &pi]() {
		pi.templateArgument(nam, hasDefaultType);
	});
}

void ParserInterfaceSynchronizer::endTemplate()
{
	auto& pi = m_parserInterface;
	enqueue([=, &pi]() {
		pi.endTemplate();
	});
}

void ParserInterfaceSynchronizer::beginType(TypeNode::Type type, Specifiers specifiers)
{
	auto& pi = m_parserInterface;
	enqueue([=, &pi]() {
		pi.beginType(type, specifiers);
	});
}

void ParserInterfaceSynchronizer::typeName(const std::string_view& name)
{
	auto& pi = m_parserInterface;
	auto nam = std::string(name);
	enqueue([=, &pi]() {
		pi.typeName(nam);
	});
}

void ParserInterfaceSynchronizer::endType()
{
	auto& pi = m_parserInterface;
	enqueue([=, &pi]() {
		pi.endType();
	});
}

void ParserInterfaceSynchronizer::beginProperty(int startLine, const std::string_view& name, Specifiers specifiers)
{
	auto& pi = m_parserInterface;
	auto nam = std::string(name);
	enqueue([=, &pi]() {
		pi.beginProperty(startLine, nam, specifiers);
	});
}

void ParserInterfaceSynchronizer::arraySubscript(const std::string_view& name)
{
	auto& pi = m_parserInterface;
	auto nam = std::string(name);
	enqueue([=, &pi]() {
		pi.arraySubscript(nam);
	});
}

void ParserInterfaceSynchronizer::endProperty(const std::string_view& name)
{
	auto& pi = m_parserInterface;
	auto nam = std::string(name);
	enqueue([=, &pi]() {
		pi.endProperty(nam);
	});
}

void ParserInterfaceSynchronizer::beginFunction(int startLine, TypeNode::Type type, const std::string_view& name)
{
	auto& pi = m_parserInterface;
	auto nam = std::string(name);
	enqueue([=, &pi]() {
		pi.beginFunction(startLine, type, nam);
	});
}

void ParserInterfaceSynchronizer::functionArgument(const std::string_view& name, const std::string_view& defaultValue)
{
	auto& pi = m_parserInterface;
	auto nam = std::string(name);
	auto def = std::string(defaultValue);
	enqueue([=, &pi]() {
		pi.functionArgument(nam, def);
	});
}

void ParserInterfaceSynchronizer::endFunction(const std::string_view& name, Specifiers specifiers)
{
	auto& pi = m_parserInterface;
	auto nam = std::string(name);
	enqueue([=, &pi]() {
		pi.endFunction(nam, specifiers);
	});
}

void ParserInterfaceSynchronizer::beginTypedef(int startLine, const std::string_view& name)
{
	auto& pi = m_parserInterface;
	auto nam = std::string(name);
	enqueue([=, &pi]() {
		pi.beginTypedef(startLine, nam);
	});
}

void ParserInterfaceSynchronizer::endTypedef(const std::string_view& name)
{
	auto& pi = m_parserInterface;
	auto nam = std::string(name);
	enqueue([=, &pi]() {
		pi.endTypedef(nam);
	});
}

void ParserInterfaceSynchronizer::beginMacro(const std::string_view& name)
{
	auto& pi = m_parserInterface;
	auto nam = std::string(name);
	enqueue([=, &pi]() {
		pi.beginMacro(nam);
	});
}

void ParserInterfaceSynchronizer::macroArgument(const std::string_view& name)
{
	auto& pi = m_parserInterface;
	auto nam = std::string(name);
	enqueue([=, &pi]() {
		pi.macroArgument(nam);
	});
}

void ParserInterfaceSynchronizer::endMacro(const std::string_view& name)
{
	auto& pi = m_parserInterface;
	auto nam = std::string(name);
	enqueue([=, &pi]() {
		pi.endMacro(nam);
	});
}

std::string ParserInterfaceSynchronizer::UniqueName()
{
	return std::string("uqn").append(std::to_string(m_unique++));
}

void ParserInterfaceSynchronizer::enqueue(const OperationFunction& func)
{
	m_queue.emplace_back(func);
}
