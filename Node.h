#pragma once
#include <functional>
#include <string>
#include <vector>
#include <list>

template <typename T>
class Node
{
public:
	using VisitCallback = std::function<bool(Node<T> *)>;
	using VisitCallbackConst = std::function<bool(const Node<T> *)>;

	Node()
		: m_parent(nullptr)
		, m_data()
		, m_index(0)
		, m_children()
		, m_childrenStorage(nullptr)
	{

	}

	Node(const T& data, Node<T>* parent, size_t index)
		: m_parent(parent)
		, m_data(data)
		, m_index(index)
		, m_children()
		, m_childrenStorage(parent->m_childrenStorage)
	{

	}

	virtual ~Node()
	{
		if (isRoot() && m_childrenStorage) {
			delete m_childrenStorage;
		}
	}

	void clear()
	{
		m_children.clear();
	}

	bool visit(VisitCallback enter, VisitCallback leave)
	{
		std::unordered_set<Node*> visited;
		std::stack<Node*> stack;
		stack.push(this);
		while (!stack.empty()) {
			auto node = stack.top();
			if (visited.find(node) == visited.end()) {
				visited.insert(node);
			}
			else {
				if (leave && !leave(node)) {
					return false;
				}
				stack.pop();
				continue;
			}

			if (enter && !enter(node)) {
				continue;
			}

			for (auto i = node->m_children.rbegin(); i != node->m_children.rend(); i++) {
				stack.push(*i);
			}
		}

		return true;
	}

	bool visit(VisitCallbackConst enter, VisitCallbackConst leave) const
	{
		std::unordered_set<const Node*> visited;
		std::stack<const Node*> stack;
		stack.push(this);
		while (!stack.empty()) {
			auto node = stack.top();
			if (visited.find(node) == visited.end()) {
				visited.insert(node);
			}
			else {
				if (leave && !leave(node)) {
					return false;
				}
				stack.pop();
				continue;
			}

			if (enter && !enter(node)) {
				continue;
			}

			for (auto i = node->m_children.rbegin(); i != node->m_children.rend(); i++) {
				stack.push(*i);
			}
		}

		return true;
	}

	bool isRoot() const
	{
		return m_parent == nullptr || m_parent == this;
	}

	size_t index() const
	{
		return m_index;
	}

	Node<T>* child(size_t i) const
	{
		return i < m_children.size() ? m_children[i] : nullptr;
	}

	Node<T>* lastChild() const
	{
		return m_children.empty() ? nullptr : m_children.back();
	}

	Node<T>* parent() const
	{
		return m_parent;
	}

	size_t children() const
	{
		return m_children.size();
	}

	void unlink()
	{
		assert(m_parent != nullptr);
		m_parent->unlinkChild(this);
	}

	void linkChild(Node<T>* node)
	{
		node->m_parent = this;
		node->m_index = m_children.size();
		m_children.push_back(node);
	}

	void reparent(Node<T>* node)
	{
		if (m_parent) {
			m_parent->unlinkChild(this);
		}

		node->linkChild(this);
	}

	void swapWith(size_t i)
	{
		if (m_index == i) {
			return;
		}

		auto node = m_parent->child(i);
		if (node) {
			m_parent->m_children[i] = this;
			m_parent->m_children[m_index] = node;
			node->m_index = m_index;
			m_index = i;
		}
	}

	Node<T>* emplaceChild(const T& data)
	{
		auto index = m_children.size();

		if (!m_childrenStorage) {
			m_childrenStorage = new std::vector<Node<T>>();
		}

		m_childrenStorage->emplace_back(data, this, index);
		m_children.emplace_back(&m_childrenStorage->back());
		if (type != Token::Type::Unknown) {
			m_children.back()->token().m_type = type;
		}
		return m_children.back();
	}

	T& data() {
		return m_data;
	}

	const T& data() const {
		return m_data;
	}

	//std::string makeDot(Token errorToken = Token(), Token prevToken = Token(), ParserError err = ParserError::Ok);

private:
	void unlinkChild(Node<T>* node)
	{
		size_t i = 0;
		m_children.erase(m_children.cbegin() + node->m_index);
		for (const auto& child : m_children) {
			child->m_index = i++;
		}

		node->m_parent = nullptr;
		node->m_index = 0;
	}


	Node<T>* m_parent;
	T m_data;

	size_t m_index;
	std::vector<Node<T>*> m_children;
	std::vector<Node<T>> *m_childrenStorage;
};
