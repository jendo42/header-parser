#pragma once
#include <vector>
#include <string>

std::string GetArgmentPos(int index, const std::string& def = std::string());
std::string GetArgumentSwitch(const std::string& key, const std::string& def = std::string());
std::string* GetArgumentSwitchPtr(const std::string& key);
void ParseArumentSwitches(char** argv);

std::vector<std::string> Explode(const std::string& s, const std::string& delimiter);
std::string TrimWhitespaceLeft(const std::string& input);
std::string TrimWhitespaceRight(const std::string& input);
std::string TrimWhitespace(const std::string& input);

bool LoadFile(const std::string_view &fileName, std::string_view& data);
void DebugPrint(const std::string_view& str);
