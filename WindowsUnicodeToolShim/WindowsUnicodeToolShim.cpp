#include <string>

#include "GpUnicode.h"
#include "WindowsUnicodeToolShim.h"

#include <Windows.h>
#include <vector>
#include <direct.h>

// This library provides front-ends and shims to make tools a bit more portable by handling all path strings as UTF-8,
// and providing a "main" entry point that is also UTF-8.

static std::string ConvertWStringToUTF8(const wchar_t *str)
{
	size_t strLength = wcslen(str);

	std::string result;

	for (size_t i = 0; i < strLength; )
	{
		size_t charsDigested = 0;
		uint32_t codePoint = 0;
		uint8_t asUTF8[4];
		if (!GpUnicode::UTF16::Decode(reinterpret_cast<const uint16_t*>(str) + i, strLength - i, charsDigested, codePoint))
			return "";

		i += charsDigested;

		size_t bytesEmitted = 0;
		GpUnicode::UTF8::Encode(asUTF8, bytesEmitted, codePoint);

		result.append(reinterpret_cast<const char*>(asUTF8), bytesEmitted);
	}

	return result;
}

static std::wstring ConvertUTF8ToWString(const char *str)
{
	size_t strLength = strlen(str);

	std::wstring result;

	for (size_t i = 0; i < strLength; )
	{
		size_t charsDigested = 0;
		uint32_t codePoint = 0;
		uint16_t asUTF16[4];
		if (!GpUnicode::UTF8::Decode(reinterpret_cast<const uint8_t*>(str) + i, strLength - i, charsDigested, codePoint))
			return L"";

		i += charsDigested;

		size_t codePointsEmitted = 0;
		GpUnicode::UTF16::Encode(asUTF16, codePointsEmitted, codePoint);

		result.append(reinterpret_cast<const wchar_t*>(asUTF16), codePointsEmitted);
	}

	return result;
}

FILE *fopen_utf8(const char *path, const char *options)
{
	std::wstring pathUTF16 = ConvertUTF8ToWString(path);
	std::wstring optionsUTF16 = ConvertUTF8ToWString(options);

	return _wfopen(pathUTF16.c_str(), optionsUTF16.c_str());
}

int fputs_utf8(const char *str, FILE *f)
{
	return fputs(str, f);
}


int mkdir_utf8(const char *path)
{
	std::wstring pathUTF16 = ConvertUTF8ToWString(path);
	return _wmkdir(pathUTF16.c_str());
}

void TerminateDirectoryPath(std::string &path)
{
	const size_t length = path.length();

	if (length == 0)
		path.append("\\");
	else
	{
		const char lastChar = path[path.length() - 1];
		if (lastChar != '\\' && lastChar != '/')
			path.append("\\");
	}
}

void ScanDirectoryForExtension(std::vector<std::string> &outPaths, const char *path, const char *ending, bool recursive)
{
	std::wstring dirFilter = std::wstring(L"\\\\?\\") + ConvertUTF8ToWString(path) + L"\\*";
	size_t endingLen = strlen(ending);

	WIN32_FIND_DATAW findDataW;
	HANDLE h = FindFirstFileW(dirFilter.c_str(), &findDataW);

	if (h == INVALID_HANDLE_VALUE)
		return;

	while (true)
	{
		std::string utf8Name = ConvertWStringToUTF8(findDataW.cFileName);
		if (utf8Name != "." && utf8Name != "..")
		{
			if (recursive && findDataW.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				ScanDirectoryForExtension(outPaths, (std::string(path) + "\\" + utf8Name).c_str(), ending, true);
			else if (utf8Name.length() >= endingLen && utf8Name.substr(utf8Name.length() - endingLen) == ending)
				outPaths.push_back(std::string(path) + "\\" + utf8Name);
		}

		if (!FindNextFileW(h, &findDataW))
			break;
	}

	FindClose(h);
}

int fseek_int64(FILE *f, int64_t offset, int origin)
{
	return _fseeki64(f, offset, origin);
}

int64_t ftell_int64(FILE *f)
{
	return _ftelli64(f);
}

int toolMain(int argc, const char **argv);

int main(int argc, const char **argv)
{
	SetConsoleOutputCP(CP_UTF8);

	LPWSTR *szArglist;
	int nArgs;

	szArglist = CommandLineToArgvW(GetCommandLineW(), &nArgs);

	std::vector<std::string> utf8ArgStrings;
	std::vector<const char *> utf8Args;

	utf8ArgStrings.resize(nArgs);
	utf8Args.resize(nArgs);

	for (int i = 0; i < nArgs; i++)
	{
		utf8ArgStrings[i] = ConvertWStringToUTF8(szArglist[i]);
		utf8Args[i] = utf8ArgStrings[i].c_str();
	}

	const char **args = nullptr;
	if (nArgs)
		args = &utf8Args[0];

	return toolMain(nArgs, args);
}
