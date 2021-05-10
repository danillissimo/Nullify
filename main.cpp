#include <iostream>
#include <iomanip>
#include <unordered_map>
#include <functional>
#include "RecursiveDirectoryTraversal.h"

#pragma region Definitions=========================================================================

#if _MSC_VER
#define noreturn __declspec(noreturn)
#else
#define noreturn __attribute__((noreturn))
#endif

#define SHLWAPI_LINKAGE_ERROR false

#if !SHLWAPI_LINKAGE_ERROR
#include <shlwapi.h>
#pragma comment(lib,"shlwapi.lib")
#else
typedef BOOL WINBOOL;
typedef WINBOOL(WINAPI* PathFileExistsFunction)(LPCSTR);

inline PathFileExistsFunction getPathFileExistsFunction() {
	return (PathFileExistsFunction)(GetProcAddress(LoadLibraryA("shlwapi.dll"), "PathFileExistsA"));
}
#endif

#pragma endregion

#pragma region Variables===========================================================================
enum param {
	undefined = 0,
	defined = 1,
};

param
inPlace = undefined,
recursive = undefined,
excludeRoot = undefined,
listFiles = undefined;

char* targetPath = nullptr;

const FILETIME filetime = { 1, 0 };
#pragma endregion

#pragma region Output==============================================================================

noreturn void showHelpAndExit() {
	std::cout
		<< "Erases all files' time attributes: creation time, last access time, and last write time\n"
		<< "\n"
		<< "nullify /p[ath] file_or_folder_path | /h[ere] | /i[nplace]\n"
		<< "        /r[ecursive] [/e[xcluderoot]]\n"
		<< "        /l[ist]\n"
		<< "        /? /h[elp]\n"
		<< "\n"
		<< "        Parameter case and order doesn't matter\n"
		<< "\n"
		<< "Necessary parameters:\n"
		<< "    /p[ath]             - must be followed by target folder/file name\n"
		<< "    /h[ere], /i[nplace] - uses current working directory as path\n"
		<< "If neither parameter is specified, nothing happens\n\n"
		<< "\n"
		<< "Optional parameters:\n"
		<< "    /r[ecursive]   - if target is a folder, recursively process all subfolders,\n"
		<< "                     ignored otherwise\n"
		<< "    /e[xcluderoot] - if target is a folder and parameter /r is present, \n"
		<< "                     exclude target folder, ignored otherwise\n"
		<< "    /l[ist]        - list all processed files and folders, increases execution\n"
		<< "                     time\n"
		<< "    /?, /help      - if present, all other params are ignored and this tip\n"
		<< "                     is shown\n";
	exit(0);
}

#pragma endregion

#pragma region File processing=====================================================================

void processFileSystemObject(const char* path, DWORD fileFlags) {

#define MSG_DONE "Done: "
#define MSG_ACCESS_DENIED "Access denied: "
#define MSG_UNKNOWN_ERROR "Unknown error: "
	constexpr int width = std::max<int>(std::max<int>(sizeof(MSG_DONE), sizeof(MSG_ACCESS_DENIED)), sizeof(MSG_UNKNOWN_ERROR)) - 1;

	HANDLE file = CreateFileA(
		path,
		FILE_WRITE_ATTRIBUTES,
		FILE_SHARE_WRITE,
		NULL,
		OPEN_EXISTING,
		fileFlags,
		NULL);
	unsigned long err = GetLastError();

	if (!err) {
		SetFileTime(file, &filetime, &filetime, &filetime);
		CloseHandle(file);
		if (listFiles) {
			std::cout << std::left << std::setw(width) << MSG_DONE << path << std::endl;
		}
	}
	else {
		const char* errMsg;
		switch (err) {
		case 5:
			errMsg = MSG_ACCESS_DENIED;
			break;
		default:
			errMsg = MSG_UNKNOWN_ERROR;
			break;
		}
		std::cout << std::left << std::setw(width) << errMsg << path << std::endl;
	}
}

inline void processFile(const char* path) {
	processFileSystemObject(path, FILE_ATTRIBUTE_NORMAL);
}

inline void processFolder(const char* path) {
	processFileSystemObject(path, FILE_FLAG_BACKUP_SEMANTICS);
}

inline bool isFolder(const char* path) {
	return GetFileAttributesA((LPCSTR)path) & FILE_ATTRIBUTE_DIRECTORY;
}

inline void processFileOrFolder(const char* path) {
	if (isFolder((char*)path)) {
		processFolder(path);
	}
	else {
		processFile(path);
	}
}

#pragma endregion

//===========================================================================================

void setParam(param& p) {
	if (p != undefined) {
		std::cout << "Executuion aborted due to repeating params";
		exit(0);
	}
	p = param::defined;
};

void checkPath () {
	if (targetPath) {
		std::cout << "Executuion aborted due to repeating params";
		exit(0);
	}
};

int main(int argc, char** argv) {
	//Don't work without params
	if (argc < 2) {
		std::cout << "Execution aborted as not enough params provided";
		exit(0);
	}

#if SHLWAPI_LINKAGE_ERROR
	PathFileExistsFunction PathFileExistsA = getPathFileExistsFunction();
#endif

	int i;

	std::function<void(void)> readPath = [&]() {
		checkPath();
		targetPath = argv[++i];
	};

	std::function<void(void)> useCurrentPath = []() {
		checkPath();
		char* pathBuffer = new char[MAX_PATH];
		GetCurrentDirectoryA(MAX_PATH, static_cast<LPSTR>(pathBuffer));
		targetPath = pathBuffer;
	};

	std::function<void(void)> setExcludeRoot = []() {
		setParam(excludeRoot);
	};

	std::function<void(void)> setRecursive = []() {
		setParam(recursive);
	};

	std::function<void(void)> setListFiles = []() {
		setParam(listFiles);
	};

	std::function<void(void)> doShowHelpAndExit = showHelpAndExit;

	std::unordered_map<
		const char*,
		std::function<void(void)>,
		std::hash<std::basic_string<char>>,
		std::equal_to<std::basic_string<char>>
	> map
	{
		std::make_pair("/path",		readPath),
		std::make_pair("/p",		readPath),

		std::make_pair("/here",		useCurrentPath),
		std::make_pair("/h",		useCurrentPath),
		std::make_pair("/inplace",	useCurrentPath),
		std::make_pair("/i",		useCurrentPath),

		std::make_pair("/excluderoot",	setExcludeRoot),
		std::make_pair("/e",			setExcludeRoot),

		std::make_pair("/recursive",	setRecursive),
		std::make_pair("/r",			setRecursive),

		std::make_pair("/list",	setListFiles),
		std::make_pair("/l",	setListFiles),

		std::make_pair("/help",	doShowHelpAndExit),
		std::make_pair("/?",	doShowHelpAndExit),
	};

	//Skip 1st param is it's path to the executable
	for (i = 1; i < argc; i++) {
		auto search = map.find(argv[i]);
		if (search != map.end()) {
			search->second();
		}
		else {
			std::cout << "Execution aborted due to incorrect parameters' format: '" << argv[i] << '\'';
			exit(0);
		}
	}
	if (!PathFileExistsA(targetPath)) {
		std::cout << "No such file or directory: " << targetPath << "\nDon't forget to wrap path in quotes if it contains spaces.";
		exit(0);
	}
	auto targetIsDirectory = isFolder(targetPath);

#if _DEBUG
	std::cout
		<< "inPlace           = " << (inPlace == param::defined) << "\n"
		<< "targetIsDirectory = " << targetIsDirectory << "\n"
		<< "recursive         = " << (recursive == param::defined) << "\n"
		<< "excludeRoot       = " << (excludeRoot == param::defined) << "\n"
		<< "listFiles         = " << (listFiles == param::defined) << "\n"
		<< "target path: " << targetPath << "\n";
#endif

	//Do the job
	if (!targetIsDirectory) {
		//process single file
		processFile(targetPath);
	}
	else if (!recursive) {
		//process single folder
		processFolder(targetPath);
	}
	else {
		//process target folder, if not excluded, and all files and folders within it
		if (!excludeRoot) {
			processFolder(targetPath);
		}

		RecursiveDirectoryTraversal traversal;
		if (traversal.begin(targetPath)) {
			do {
				processFileOrFolder(traversal.currentObjectPath());
			} while (traversal.stepInto());
		}
	}

	return 0;
}