#include "single_instance_guard.h"
#include <cassert>
#include <boost/asio.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>
#include <boost/interprocess/ipc/message_queue.hpp>
#include <boost/filesystem.hpp>
#include <boost/system/error_code.hpp>
using namespace std;
using namespace boost::asio;
using namespace boost::interprocess;

namespace{
	enum {
		kMaxMessage = 128,
		kMaxMessageSize = 512,
	};
}

class SingleInstanceGuard::ImplT
{
public:
	ImplT(const char* name_)
		: _name(name_)
		, _mutexNameForSingleInstance(_name + "#MUTEX")
		, _nameMessageQueue(_name + "#MESSAGE_QUEUE")
		, _io_context()
		, _mutexSingleInstance()
		, _mutexMessageQueue()
		, _create(true)
		, _anotherIsRunning(false)
		, _isMainInstance(false)
		, _thread()
		, _messageQueue()
		, _messageHandler()
		, _timer()
	{
		init(name_);
	}

	~ImplT()
	{
		_io_context.stop();
		if (_thread.joinable()){
			_thread.join();
		}
		if (nullptr != _mutexSingleInstance){
			if (_isMainInstance){
				_mutexSingleInstance->unlock();
				named_mutex::remove(_mutexNameForSingleInstance.c_str());
				message_queue::remove(_nameMessageQueue.c_str());
			}
		}
	}

	void postMessage(std::string bytes)
	{
		assert(nullptr != _messageQueue);
		_messageQueue->send(bytes.c_str(), bytes.size(), 0);
	}

	void setMessageHandler(MessageHandler handler)
	{
		_messageHandler = std::move(handler);
	}
	
	const std::string _name;
	const std::string _mutexNameForSingleInstance;
	const std::string _nameMessageQueue;
	io_context _io_context;
	unique_ptr<named_mutex> _mutexSingleInstance;
	unique_ptr<named_mutex> _mutexMessageQueue;
	bool _create;
	bool _anotherIsRunning;
	bool _isMainInstance;
	std::thread _thread;
	unique_ptr<message_queue> _messageQueue; 
	MessageHandler _messageHandler;
	std::unique_ptr<system_timer> _timer;

private:
	void init(const char * _name)
	{
		do 
		{
			try{
				_mutexSingleInstance = make_unique<named_mutex>(create_only, 
					_mutexNameForSingleInstance.c_str());
			}
			catch (interprocess_exception& exp){
				if (exp.get_error_code() != already_exists_error){
					throw std::move(exp);
				}
				_create = false;
			}
			assert(nullptr != _mutexSingleInstance || !_create);
			if (nullptr == _mutexSingleInstance){
				_mutexSingleInstance = make_unique<named_mutex>(open_only, 
					_mutexNameForSingleInstance.c_str());
			}
			if (_mutexSingleInstance->try_lock()){
				_isMainInstance = true;
			}
			else{
				std::string shmfile;
				ipcdetail::shared_filepath(_mutexNameForSingleInstance.c_str(), shmfile);
				boost::system::error_code ec_;
				_mutexSingleInstance = nullptr;
				if (boost::filesystem::remove(shmfile, ec_)){
					_create = true;
					continue;
				}
				else if (boost::system::system_category() == ec_.category() 
					&& ec_.value() == ERROR_ACCESS_DENIED){
					_anotherIsRunning = true;
				}
				else{
					BOOST_FILESYSTEM_THROW(ec_);
				}
			}
			_thread = std::thread(std::bind(&ImplT::run, this));
			break;
		} while (nullptr == _mutexSingleInstance);
	}

	void initMessageQueue()
	{
		if (_create || _isMainInstance){
			message_queue::remove(_nameMessageQueue.c_str());
			_messageQueue = make_unique<message_queue>(create_only, 
				_nameMessageQueue.c_str(),
				kMaxMessage, kMaxMessageSize);
		}
		else{
			_messageQueue = make_unique<message_queue>(open_only, 
				_nameMessageQueue.c_str());
		}
	}

	void run()
	{
		initMessageQueue();
		_timer = std::make_unique<system_timer>(_io_context);
		timerReceive();
		_io_context.run();

		_messageQueue = nullptr;
		_timer = nullptr;
	}

	void handler(const boost::system::error_code& error)
	{
		if (!error){
			std::string bytes;
			bytes.resize(kMaxMessageSize);
			std::size_t size_;
			unsigned int priority_ = 0;
			if (_messageQueue->try_receive(&bytes[0], bytes.size(),
				size_, priority_)){
				if (nullptr != _messageHandler){
					_messageHandler(bytes.c_str(), size_);
				}
			}
			timerReceive();
		}
		else{
			BOOST_FILESYSTEM_THROW(error);
		}
	}

	void timerReceive()
	{
		if (!_io_context.stopped()){

			_timer->expires_after(std::chrono::milliseconds(500));
			std::function<void(const boost::system::error_code& error)> callback_ = std::bind(&ImplT::handler,
				this, std::placeholders::_1);
			_timer->async_wait(std::move(callback_));
		}
	}
};

SingleInstanceGuard::SingleInstanceGuard()
	: _impl()
{}

SingleInstanceGuard::SingleInstanceGuard(const char * name_)
	: _impl(std::make_unique<ImplT>(name_))
{}

SingleInstanceGuard::SingleInstanceGuard(SingleInstanceGuard && other)
	: _impl(std::move(other._impl))
{
}

SingleInstanceGuard& SingleInstanceGuard::operator = (SingleInstanceGuard && other)
{
	_impl = std::move(other._impl);
	return *this;
}

SingleInstanceGuard::~SingleInstanceGuard()
{}

SingleInstanceGuard::operator bool(void) const
{
	return nullptr != _impl;
}

bool SingleInstanceGuard::anotherIsRunning(void)
{
	if (nullptr != _impl){
		return _impl->_anotherIsRunning;
	}
	else{
		throw std::logic_error("SingleInstanceGuard has no implementation!");
	}
}

void SingleInstanceGuard::postMessage(const char * buf_, std::size_t sz_)
{
	if (nullptr != _impl){
		std::function<void(void)> callback = std::bind(&ImplT::postMessage,
			_impl.get(), std::string(buf_, sz_));
		post(_impl->_io_context, std::move(callback));
	}
	else{
		throw std::logic_error("SingleInstanceGuard has no implementation!");
	}
}

void SingleInstanceGuard::setMessageHandler(MessageHandler proc_)
{
	if (nullptr != _impl){
		std::function<void(void)> callback = std::bind(&ImplT::setMessageHandler,
			_impl.get(), std::move(proc_));
		post(_impl->_io_context, std::move(callback));
	}
	else{
		throw std::logic_error("SingleInstanceGuard has no implementation!");
	}
}