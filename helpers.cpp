#include <unordered_map>
#include <sstream>
#include <string>
#include <iostream>
#include <fstream>

#include "helpers.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

static std::vector<std::string> args;

static std::unordered_map<std::string, std::string> switches;

std::vector<std::string> Explode(const std::string& in, const std::string& delimiter)
{
	std::string s = in;
	std::vector<std::string> tokens;
	size_t pos = 0;
	std::string token;
	while ((pos = s.find(delimiter)) != std::string::npos) {
		token = s.substr(0, pos);
		tokens.push_back(token);
		s.erase((size_t)0, (size_t)(pos + delimiter.length()));
	}

	tokens.push_back(s);
	return tokens;
}

std::string GetArgmentPos(int index, const std::string& def)
{
	if (index >= args.size()) {
		return def;
	}
	return args[index];
}

std::string GetArgumentSwitch(const std::string& key, const std::string& def)
{
	const auto it = switches.find(key);
	return it == switches.cend() ? def : it->second;
}

std::string* GetArgumentSwitchPtr(const std::string& key)
{
	const auto it = switches.find(key);
	return it == switches.cend() ? nullptr : &it->second;
}

void ParseArumentSwitches(char** argv)
{
	for (const char* arg = *argv; (arg = *argv); argv++) {
		std::string str = arg;
		if (str[0] == '-' && str[1] == '-') {
			size_t delim_pos = str.find_first_of('=');
			if (delim_pos == std::string::npos) {
				std::string key = str.substr(2, std::string::npos);
				switches[key] = "";
			}
			else {
				std::string key = str.substr(2, delim_pos - 2);
				std::string value = str.substr(delim_pos + 1, std::string::npos);
				switches[key] = value;
			}
		}
		else {
			args.push_back(str);
		}
	}
}

std::string TrimWhitespaceLeft(const std::string& input)
{
	auto s = input;
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
		return !::isspace(ch);
	}));

	return s;
}

std::string TrimWhitespaceRight(const std::string& input)
{
	auto s = input;
	s.erase(std::find_if(s.rbegin(), s.rend(), [](char ch) {
		return !::isspace(ch);
	}).base(), s.end());

	return s;
}

std::string TrimWhitespace(const std::string& input)
{
	return TrimWhitespaceRight(TrimWhitespaceLeft(input));
}

bool LoadFile(const std::string_view &fileName, std::string_view& data)
{
	HANDLE hFile = CreateFileA(std::string(fileName).c_str(), GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
	if (hFile == INVALID_HANDLE_VALUE) {
		return false;
	}

	HANDLE hMapping = CreateFileMappingA(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
	if (hMapping == NULL) {
		CloseHandle(hFile);
		return false;
	}

	auto dataPtr = (const char*)MapViewOfFile(hMapping, FILE_MAP_READ, 0, 0, 0);
	if (!dataPtr) {
		CloseHandle(hMapping);
		CloseHandle(hFile);
		return false;
	}

	size_t size = GetFileSize(hFile, NULL);
	data = std::string_view(dataPtr, size);
	return true;
}

void DebugPrint(const std::string_view& str)
{
	OutputDebugStringA(std::string(str).c_str());
}

bool LoadFile2(bool cin, const std::string& fileName, std::stringstream& data)
{
	std::ifstream ifs(fileName);
	std::istream* inputStream = &ifs;
	if (!inputStream->good() && cin) {
		inputStream = &std::cin;
		if (!inputStream->good()) {
			return false;
		}
	}

	data << inputStream->rdbuf();
	return true;
}
