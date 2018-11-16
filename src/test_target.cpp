#include <iostream>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/msvc_sink.h>
#include <mutex>
#include <chrono>
#include <thread>
#include <boost/progress.hpp>
#include <sstream>
#include "single_instance_guard.h"
#include <Windows.h>

int WinMain( _In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd )
{
	auto logger_ = std::make_shared<spdlog::logger>("SINGLE_INSTANCE_TARGET", std::make_shared < spdlog::sinks::msvc_sink_st >());
	logger_->info("Enter single instance target progress {}!", ::GetCurrentProcessId());
	static std::mutex _mutex;
	std::unique_lock<std::mutex> _lock(_mutex);
	SingleInstanceGuard sig("TEST#TARGET");
	if (sig.anotherIsRunning()){
		logger_->info("I`m another {}!", ::GetCurrentProcessId());
		std::stringstream strstrm;
		strstrm << "Process Id " << ::GetCurrentProcessId() << " will quite!";
		auto str_ = strstrm.str();
		sig.postMessage(str_.c_str(), str_.size());
		logger_->info("Quit another instance target progress! {}", ::GetCurrentProcessId());
		return 0;
	}
	else{
		logger_->info("I`m master {}!", ::GetCurrentProcessId());
		sig.setMessageHandler([=](const char * buf_, std::size_t sz_){
			logger_->info("Receive message \"{}\" from another instance!", std::string(buf_, sz_));
		});
	}
	_lock.unlock();
	std::this_thread::sleep_for(std::chrono::minutes(1));
	logger_->info("Quit master instance target progress! {}", ::GetCurrentProcessId());
	return 0;
}