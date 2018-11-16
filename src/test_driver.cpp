#include <iostream>
#include <spdlog/spdlog.h>
#include <boost/filesystem.hpp>
#include <Windows.h>

int main(_In_ int _Argc, _In_reads_(_Argc) _Pre_z_ char ** _Argv, _In_z_ char ** _Env)
{
	wchar_t app_path[MAX_PATH] = {0};
	::GetModuleFileNameW(nullptr, app_path, MAX_PATH);
	boost::filesystem::path path_(app_path);
	path_.remove_filename();
	path_.append("test_target.exe");

	for (int i = 0; i < 100; ++i){
		STARTUPINFOW si = { 0 };
		si.cb = sizeof(si);
		PROCESS_INFORMATION pi = { 0 };
		std::wstring str(path_.c_str());
		if (::CreateProcessW(path_.c_str(), nullptr, nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi)){
			std::cout << "s " << pi.dwProcessId << std::endl;
		}
		else{
			std::cout << "f " << GetLastError() << std::endl;
		}
	}
	return 0;
}

