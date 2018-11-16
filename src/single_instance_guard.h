#ifndef _SINGLE_INSTANCE_GUARD_H_
#define _SINGLE_INSTANCE_GUARD_H_

#include <memory>
#include <functional>

class SingleInstanceGuard
{
public:
	SingleInstanceGuard();

	explicit SingleInstanceGuard(const char * name_);

	SingleInstanceGuard(SingleInstanceGuard && other);

	SingleInstanceGuard& operator= (SingleInstanceGuard && other);

	~SingleInstanceGuard();

	explicit operator bool(void) const;

	bool anotherIsRunning(void);

	void postMessage(const char * buf_, std::size_t sz_);

	using MessageHandler = std::function < void(const char * buf_, std::size_t sz_) > ;

	void setMessageHandler(MessageHandler proc_);

	SingleInstanceGuard(const SingleInstanceGuard & other) = delete;

	SingleInstanceGuard& operator= (const SingleInstanceGuard & other) = delete;

private:
	class ImplT;
	std::unique_ptr<ImplT> _impl;
};

#endif	// _SINGLE_INSTANCE_GUARD_H_ [11/13/2018 brian.wang]