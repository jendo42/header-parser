#include <functional>
#include <deque>

#include "parser_interface.h"
#include "TypeData.h"
#include "MPMCQueue.h"

class ParserInterfaceSynchronizer
	: public ParserInterface
{
public:
	typedef std::function<void()> OperationFunction;
	typedef std::deque<OperationFunction> OperationQueue;
	struct Result
	{
		Result() = default;
		Result(const OperationQueue& queue)
			: m_queue(queue)
		{
		}

		Result(const Result&other) noexcept
			: m_queue(other.m_queue)
		{

		}

		Result(Result&& other) noexcept
			: m_queue(std::move(other.m_queue))
		{

		}

		Result& operator=(Result&& other) noexcept
		{
			m_queue = std::move(other.m_queue);
			return *this;
		}

		OperationQueue m_queue;
	};
	typedef rigtorp::MPMCQueue<Result> ResultQueue;

	ParserInterfaceSynchronizer(const std::string& out, ParserInterface &target, ResultQueue &sharedQueue);

	void destroy() override;
	void begin(const std::string_view& source) override;
	void end(const std::string_view& source, const std::string_view& error) override;

	void include(const std::string_view& filename) override;
	void comment(const std::string_view& comment) override;
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

private:
	ParserInterface& m_parserInterface;

	OperationQueue m_queue;
	ResultQueue &m_resultQueue;

	std::string m_inputFile;
	std::string m_outputFile;
	std::vector<std::string> m_context;
	unsigned m_unique;

	std::string UniqueName();

	void enqueue(const OperationFunction& func);
};
