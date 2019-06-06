#pragma once

#include <cstdio>
#include <iostream>
#include <mutex>
#include <string>
#include <cstdarg>

#define TMP_BUFFER_SIZE 1024

class LOG {
private:
	enum OUTTYPE{ COUT, FILE, STREAM };

	std::string strPath;
	std::mutex mtxLog;
	std::FILE *pFile;
	std::ostream *pStream;
	char tmpBuffer[TMP_BUFFER_SIZE] = {0};
	OUTTYPE logType;
	
	// private constructor for the singleton class
	LOG()
	{
		logType = COUT;
		pFile = nullptr;
		pStream = nullptr;
	} // singleton

	// logs a string to the current output method
	void LogString(const char *str)
	{
		mtxLog.lock();
		switch (logType) {
		case COUT:
			std::cout << str << std::endl;
			break;
		case FILE: 
			if (pFile != nullptr) {
				std::fputs(str, pFile);
			}
			break;
		case STREAM:
			if (pStream != nullptr)
			{
				*pStream << str << std::endl;
			}
			break;
		}
		mtxLog.unlock();
	}

	// logs a formatted string to the output method
	void LogStringFormat(const char *str,...)
	{
		mtxLog.lock();
		va_list arVa;
		va_start(arVa, str);
		switch(logType){
		case COUT:
			vsnprintf(tmpBuffer, TMP_BUFFER_SIZE, str, arVa);
			std::cout << tmpBuffer << std::endl;
			break;
		case FILE:
			if (pFile != nullptr) {
				std::vfprintf(pFile, str, arVa);
				std::fputs(str, pFile);
			}
			break;
		case STREAM:
			if (pStream != nullptr)
			{
				vsnprintf(tmpBuffer, TMP_BUFFER_SIZE, str, arVa);
				*pStream << tmpBuffer << std::endl;
			}
			break;
		}
		va_end(arVa);
		mtxLog.unlock();
	}

	// sets the log output to the file specified
	void SetLogPath(const char *str)
	{
		mtxLog.lock();
		cleanupOutputs();
		logType = FILE;
		strPath = str;
		pFile = fopen(str, "a");
		mtxLog.unlock();
	}

	// sets the log output the the stream passed in
	// the caller is responsible for cleaning up the stream
	void SetLogStreamer(std::ostream* stream)
	{
		mtxLog.lock();
		cleanupOutputs();
		logType = STREAM;
		pStream = stream;
		mtxLog.unlock();
	}

	// sets the output to std::cout
	void SetStdOut()
	{
		mtxLog.lock();
		cleanupOutputs();
		logType = COUT;
		mtxLog.unlock();
	}

	// basic cleanup
	void cleanupOutputs()
	{
		if (pStream)
		{
			pStream = nullptr;
		}
		if (pFile)
		{
			fclose(pFile);
			pFile = nullptr;
		}
	}

	// get the singleton instance
	static LOG* getInstance()
	{
		static LOG logger;
		return &logger;
	}

public:
	// destructor
	~LOG() 
	{
		if (pFile)
		{
			fclose(pFile);
		}
	}

	// the following are static versions of the internal functions
	// this was done for convenience to the caller
	static void SETSTDOUT()
	{
		LOG::getInstance()->SetStdOut();
	}

	static void SETLOGSTREAM(std::ostream *stream)
	{
		LOG::getInstance()->SetLogStreamer(stream);
	}

	static void SETLOGPATH(const char *str)
	{
		LOG::getInstance()->SetLogPath(str);
	}

	static void LOGSTR(const char *str)
	{
		LOG::getInstance()->LogString(str);
	}

	static void LOGSTRF(const char *str, ...)
	{
		va_list arVa;
		va_start(arVa, str);
		LOG::getInstance()->LogStringFormat(str,arVa);
		va_end(arVa);
	}
};
