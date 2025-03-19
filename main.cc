#include <iostream>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <functional>
#include <thread>
#include <atomic>

#include "helpers.h"
#include "parser.h"
#include "handler.h"
#include "ScopeGuard.h"

#include "DebugParserInterface.h"
#include "TypeDbParserInterface.h"
#include "ParserInterfaceSynchronizer.h"

#define LOG_INFO(x) std::cout << x << std::endl
#define LOG_ERROR(x) std::cerr << x << std::endl

#define LOG_INFO_SYNC(q, x) q.emplace(ParserInterfaceSynchronizer::Result{ ParserInterfaceSynchronizer::OperationQueue{[=]() { LOG_INFO(x); }} })
#define LOG_ERROR_SYNC(q, x) q.emplace(ParserInterfaceSynchronizer::Result{ ParserInterfaceSynchronizer::OperationQueue{[=]() { LOG_ERROR(x); }} })

static const std::unordered_map<std::string, std::function<ParserInterface* (const std::string& out)>> generators {
	{"typedb", [](const std::string &out) {
		return new TypeDbParserInterface(GetArgumentSwitchPtr("typedb-output"));
	}},
};

static double GetTime()
{
	static const std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();
	std::chrono::high_resolution_clock::time_point now = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> time_span = std::chrono::duration_cast<std::chrono::duration<double>>(now - start);
	return time_span.count();
}

//----------------------------------------------------------------------------------------------------
int main(int argc, char** argv)
{
	// parse arguments
	ParseArumentSwitches(argv);
	std::string inputFile = GetArgmentPos(1);
	std::string outputFile = GetArgmentPos(2);
	//if (inputFile.empty()) {
	//	std::cerr << "usage: " << argv[0] << " <input_file> <output_file>" << std::endl;
	//	return -1;
	//}

	// macros
	bool profile = GetArgumentSwitchPtr("profile") != nullptr;

	std::string macros = GetArgumentSwitch("macros");
	const auto macroList = Explode(macros, ",");

	std::vector<std::string_view> fileList;
	auto fileListSwitch = GetArgumentSwitch("list");
	if (!fileListSwitch.empty()) {
		// Open input file
		std::string_view fileListData;
		LoadFile(fileListSwitch.c_str(), fileListData);
		std::string_view ptr = fileListData;
		for (size_t pos = 0; (pos = ptr.find_first_of('\n')) != std::string::npos; ptr = ptr.substr(pos + 1)) {
			size_t p = ptr[pos - 1] == '\r' ? pos - 1 : pos;
			auto f = ptr.substr(0, p);
			if (f[0] != '#') {
				fileList.emplace_back(f);
			}
		}
	} else {
		fileList.push_back(inputFile);
	}

	rigtorp::MPMCQueue<std::string_view> fileQueue(fileList.size());
	for (const auto& file : fileList) {
		fileQueue.emplace(file);
	}

	// select generator
	std::string generator = GetArgumentSwitch("generator", "typedb");
	const auto generatorIt = generators.find(generator);
	if (generatorIt == generators.cend()) {
		LOG_ERROR("Unknown generator " << generator.c_str());
		return -1;
	}
	
	ParserInterface* parserInterface = generatorIt->second(outputFile);
	if (GetArgumentSwitchPtr("debug")) {
		parserInterface = new DebugParserInterface(*parserInterface);
	}

	double t1 = GetTime();
	ParserInterfaceSynchronizer::ResultQueue sharedQueue(4096);
	size_t threadCount = std::thread::hardware_concurrency() - 1;
	if (fileList.size() < threadCount) {
		threadCount = fileList.size();
	}

	std::atomic<size_t> threadCounter = threadCount;
	std::atomic<size_t> filesParsed = 0;
	std::vector<std::thread> threadList;
	for (size_t cnt = threadCount; cnt; cnt--) {
		threadList.emplace_back(std::thread{ [=, &macroList, &threadCounter, &outputFile, &sharedQueue, &fileQueue, &filesParsed]() {
			double startTime = GetTime();
			ScopeGuard guard([&]() {
				// the thread finished
				if (--threadCounter == 0) {

				}
			});

			while (!fileQueue.empty()) {
				std::string_view file;
				if (!fileQueue.try_pop(file)) {
					std::this_thread::yield();
					continue;
				}

				double startTime = GetTime();

				// load input
				std::string_view data;
				if (!LoadFile(file, data)) {
					LOG_ERROR_SYNC(sharedQueue, "Failed to load file '" << file << "'");
					continue;
				}

				double loadFileTime = GetTime();

				// create parser
				ParserInterfaceSynchronizer synchronizer(outputFile, *parserInterface, sharedQueue);
				Parser parser(synchronizer);

				// add known macros
				for (auto& macro : macroList) {
					parser.AddMacro(macro);
				}

				// parse input data
				if (!parser.Parse(file, data)) {
					auto err = std::string(parser.GetError());
					LOG_INFO_SYNC(sharedQueue, "'" << file << "': " << err.c_str());
				} else {
					++filesParsed;
				}

				double endTime = GetTime();
				if (profile) {
					LOG_INFO_SYNC(sharedQueue, "'" << file << "': load time " << (loadFileTime - startTime) * 1000 << " ms, parse time " << (endTime - loadFileTime) * 1000 << " ms");
				}
			}
		}});
	}

	double t2 = GetTime();

	while (threadCounter || !sharedQueue.empty()) {
		ParserInterfaceSynchronizer::Result result;
		if (sharedQueue.try_pop(result)) {
			while (!result.m_queue.empty()) {
				result.m_queue.front()();
				result.m_queue.pop_front();
			}
		} else {
			std::this_thread::yield();
		}
	}

	for (auto& thread : threadList) {
		thread.join();
	}

	parserInterface->destroy();

	double t3 = GetTime();

	LOG_INFO("Starting " << threadCount << " thread(s) took: " << (t2 - t1) * 1000 << "ms");
	LOG_INFO("Total time: " << (t3 - t1) * 1000 << "ms");
	LOG_INFO("Total file(s) parsed: " << filesParsed);
	return 0;
}
