#pragma once
#include <functional>

class ScopeGuard
{
public:
	ScopeGuard(const std::function<void()>& deleter)
		: m_deleter(deleter)
	{

	}
	~ScopeGuard()
	{
		m_deleter();
	}
protected:
	std::function<void()> m_deleter;
};
