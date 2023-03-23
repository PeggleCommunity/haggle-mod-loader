#include "utils/logger/logger.hpp"
#include "utils/memory.hpp"
#include "utils/loader/loader.hpp"
#include <sstream>

std::initializer_list<std::string> ext_whitelist
{
	".dll",
	".asi",
};

bool ends_with(std::string const& value, std::string const& ending)
{
	if (ending.size() > value.size()) return false;
	return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

void init()
{
	logger::init("Haggle Mod Loader");
	std::printf("----- Haggle Mod Loader -----\n");

	PRINT_INFO("In directory \"%s\"", std::filesystem::absolute(std::filesystem::path("./")).string().c_str());

	if (!std::filesystem::exists("./mods/"))
	{
		PRINT_ERROR("No mods folder found!");
		PRINT_INFO("Make a mods folder in your Peggle directory");
	}
	else
	{
		std::vector<std::string> files;
		for (const auto& entry : std::filesystem::directory_iterator("./mods/"))
		{
			if (!entry.is_directory())
			{
				files.emplace_back(entry.path().string());
			}
		}

		PRINT_INFO("Loading Mods...");
		static int count = 0;
		for (const auto& bin : files)
		{
			for (const auto& ext : ext_whitelist)
			{
				if (ends_with(bin, ext))
				{
					auto mod = LoadLibraryA(bin.c_str());

					if (GetLastError() != 0)
					{
						PRINT_ERROR("%s errored! (%i)", bin.c_str(), GetLastError());
					}
					else
					{
						PRINT_INFO("%s loaded!", bin.c_str());
						++count;
					}
				}
			}
		}

		if (count == 1)
		{
			PRINT_INFO("1 mod loaded");
		}
		else if (count > 1)
		{
			PRINT_INFO("%i mods loaded", count);
		}
		else if (count <= 0)
		{
			PRINT_WARNING("No mods loaded");
		}

		PRINT_INFO("Ready!");
	}
}

DWORD find_proc_id(const std::string& process_name)
{
	PROCESSENTRY32 processInfo;
	processInfo.dwSize = sizeof(processInfo);

	HANDLE processesSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
	if (processesSnapshot == INVALID_HANDLE_VALUE) {
		return 0;
	}

	Process32First(processesSnapshot, &processInfo);
	std::wstring exe_file(processInfo.szExeFile);
	if (!process_name.compare(std::string(exe_file.begin(), exe_file.end())))
	{
		CloseHandle(processesSnapshot);
		return processInfo.th32ProcessID;
	}

	while (Process32Next(processesSnapshot, &processInfo))
	{
		std::wstring exe_file(processInfo.szExeFile);
		if (!process_name.compare(std::string(exe_file.begin(), exe_file.end())))
		{
			CloseHandle(processesSnapshot);
			return processInfo.th32ProcessID;
		}
	}

	CloseHandle(processesSnapshot);
	return 0;
}

bool terminate_process(const std::string& process_name)
{
	bool success = false;

	if (auto pid = find_proc_id(process_name))
	{
		auto handle = OpenProcess(PROCESS_TERMINATE, false, pid);
		success = TerminateProcess(handle, 1);
		CloseHandle(handle);
	}

	return success;
}

int main(int argc, char* argv[])
{
	if (!std::filesystem::exists("./mods/"))
	{
		std::filesystem::create_directory("./mods/");
	}

	if (!std::filesystem::exists("./mods/cache.bin"))
	{
		int appid = 3480;
		std::string game = "Peggle";
		std::string exe = std::format("{}.exe", game);

		if (!std::filesystem::exists(exe))
		{
			appid = 3540;
			game = "PeggleNights";
			exe = std::format("{}.exe", game);

			if (!std::filesystem::exists(exe))
			{
				MessageBoxA(nullptr, "Haggle was unable to determine which game this directory belongs to.\nPlease try again in the proper directory.", "Haggle Mod Loader", 0);
				return 0;
			}
		}

		if (terminate_process(exe))
		{
			if (terminate_process("popcapgame1.exe"))
			{
				std::filesystem::path cache = std::filesystem::absolute(std::filesystem::current_path() / "mods/cache.bin");
				std::filesystem::rename(std::format("C:/ProgramData/PopCap Games/{}/popcapgame1.exe", game).c_str(), cache.c_str());
				ShellExecuteA(nullptr, "open", "Haggle.exe", 0, 0, SW_SHOWNORMAL);
				return 0;
			}
			else
			{
				MessageBoxA(nullptr, "Haggle was unable to extract the data necessary to run, please try again.", "Haggle Mod Loader", 0);
				return 0;
			}
		}
		else
		{
			auto resp = MessageBoxA(nullptr, "Since this is your first time launching Haggle, please launch Peggle through Steam first then launch Haggle.exe.\n\nPress retry to atempt automagic installation.\nPress cancel to stop the process and try again manually.", "Haggle Mod Loader", MB_RETRYCANCEL);

			if (resp == IDRETRY)
			{
				ShellExecuteA(0, "open", std::format("steam://run/{}/", appid).c_str(), 0, 0, 0);
				while (!find_proc_id(exe))
				{
					Sleep(1000);
				}
				ShellExecuteA(nullptr, "open", "Haggle.exe", 0, 0, SW_SHOWNORMAL);
				return 0;
			}
			else if (resp == IDCANCEL)
			{
				return 0;
			}
		}
	}

	loader::load("./mods/cache.bin");
	init();
	return loader::run(argc, argv);
}
