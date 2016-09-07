/*
Microlog - A small logging system for C++.

Example:

	microlog::Logger logger;
	logger.SetFile("c:/temp/myapp.log");
	logger.Info("Startup %s", "normal sprintf formatting");
	logger.Debug("I will be discarded");

*/

#ifdef _Printf_format_string_
#define MICROLOG_FORMAT_STRING _In_z_ _Printf_format_string_
#else
#define MICROLOG_FORMAT_STRING
#endif

#ifndef MICROLOG_INCLUDE_H
#define MICROLOG_INCLUDE_H

#include <string>

namespace microlog
{
	enum class Level
	{
		Debug,
		Info,
		Warn,
		Error
	};

	class ILogSink
	{
	public:
		virtual void WriteLog(Level lev, const char* msg, size_t msgLen) = 0;
		virtual ~ILogSink();
	};

	class FileSink : public ILogSink
	{
	public:
		std::string Filename;

		~FileSink() override;
		void WriteLog(Level lev, const char* msg, size_t msgLen) override;

	protected:
		int Handle = -1;

		bool Open();
	};

	// Adds formatting to a log message, such as time and log level
	class FormatSink : public ILogSink
	{
	public:
		ILogSink* Next = nullptr;

		~FormatSink();
		void WriteLog(Level lev, const char* msg, size_t msgLen) override;
	};

	class Logger
	{
	public:
		ILogSink*	Sink = nullptr;
		Level		Level = Level::Info;

		~Logger();

		void Log(microlog::Level lev, MICROLOG_FORMAT_STRING const char* formatMsg, ...);
		void LogVA(microlog::Level lev, MICROLOG_FORMAT_STRING const char* formatMsg, va_list va);
		void LogS(microlog::Level lev, const char* msg);
		void Debug(MICROLOG_FORMAT_STRING const char* formatMsg, ...);
		void Info(MICROLOG_FORMAT_STRING const char* formatMsg, ...);
		void Warn(MICROLOG_FORMAT_STRING const char* formatMsg, ...);
		void Error(MICROLOG_FORMAT_STRING const char* formatMsg, ...);

		void SetFile(const char* filename);


	};

}

#endif // MICROLOG_INCLUDE_H

#ifdef MICROLOG_IMPLEMENTATION

#include <memory>
#include <chrono>
#include <ctime>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <io.h>

namespace microlog
{
	static const char* LevelToString(Level lev)
	{
		switch (lev)
		{
		case Level::Debug: return "Debug";
		case Level::Info: return "Info";
		case Level::Warn: return "Warn";
		case Level::Error: return "Error";
		default: return "Unknown";
		}
	}

	ILogSink::~ILogSink()
	{
	}

	FileSink::~FileSink()
	{
		if (Handle != -1)
			close(Handle);
	}

	void FileSink::WriteLog(Level lev, const char* msg, size_t msgLen)
	{
		if (!Open())
			return;
		write(Handle, msg, msgLen);
	}

	bool FileSink::Open()
	{
		if (Handle == -1)
		{
#ifdef _MSC_VER
			_sopen_s(&Handle, Filename.c_str(), O_CREAT | O_WRONLY | O_BINARY, _SH_DENYNO, _S_IREAD | _S_IWRITE);
#else
			Handle = open(Filename.c_str(), O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
#endif
			if (Handle != -1)
				lseek(Handle, 0, SEEK_END);
		}
		return Handle != -1;
	}

	FormatSink::~FormatSink()
	{
		delete Next;
	}

	void FormatSink::WriteLog(Level lev, const char* msg, size_t msgLen)
	{
		const int STATIC_BUFSIZE = 4096;
		char static_buf[STATIC_BUFSIZE];

#ifdef _MSC_VER
		const int newLineSize = 2;	// \r\n
#else
		const int newLineSize = 1; // \n
#endif

		const int extra = 33 + newLineSize;
		char* buf = static_buf;
		if (msgLen + extra + 1 > STATIC_BUFSIZE)
		{
			buf = (char*) malloc(msgLen + extra + 1);
			if (!buf)
				return;
		}
		
		auto now = std::chrono::steady_clock::now();
		std::time_t t = std::chrono::steady_clock::to_time_t(now);
		auto tm = *gmtime(&t);
		std::chrono::duration<double> dur = now.time_since_epoch();
		int second_6dec = 1000000 * fmod(dur.count(), 1.0);

		sprintf(buf, "%04d-%02d-%02d %02d:%02d:%02d.%06d Z ", 1900 + tm.tm_year, tm.tm_mon, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, second_6dec);
		sprintf(buf + extra - 4 - newLineSize, "[%c] ", LevelToString(lev)[0]);

		memcpy(buf + extra - newLineSize, msg, msgLen);
		if (newLineSize == 1)
		{
			buf[extra + msgLen - newLineSize] = '\n';
		}
		else
		{
			buf[extra + msgLen - newLineSize] = '\r';
			buf[extra + msgLen - newLineSize + 1] = '\n';
		}

		buf[extra + msgLen] = 0;

		Next->WriteLog(lev, buf, msgLen + extra);

		if (buf != static_buf)
			free(buf);
	}

	Logger::~Logger()
	{
		delete Sink;
	}

	void Logger::LogS(microlog::Level lev, const char* msg)
	{
		Sink->WriteLog(lev, msg, strlen(msg));
	}

	void Logger::Log(microlog::Level lev, MICROLOG_FORMAT_STRING const char* formatMsg, ...)
	{
		va_list va;
		va_start(va, formatMsg);
		LogVA(Level::Info, formatMsg, va);
		va_end(va);
	}

	void Logger::LogVA(microlog::Level lev, MICROLOG_FORMAT_STRING const char* formatMsg, va_list va)
	{
		if (lev < Level)
			return;

		const char* OOMSG = "Logger out of memory:";
		const char* MSGTOOBIG = "Logger message is too big:";
		const char* MSGTOOBIG_NIX = "Logger message is too big (unexpected):";
		const int STATICBUFSIZE = 4096;
		const int WINHEAPSIZE = 65536;
		char static_buf[STATICBUFSIZE];
		char* buf = static_buf;

		int r = vsnprintf(static_buf, STATICBUFSIZE, formatMsg, va);
#ifdef _MSC_VER
		if (r == -1)
		{
			// Windows doesn't tell us how much space we need, so we have to just
			// guess a large static size, and give up if that isn't big enough.
			buf = (char*) malloc(WINHEAPSIZE);
			if (!buf)
			{
				Sink->WriteLog(lev, OOMSG, strlen(OOMSG));
				Sink->WriteLog(lev, formatMsg, strlen(formatMsg));
				return;
			}
			r = vsnprintf(buf, WINHEAPSIZE, formatMsg, va);
			if (r == -1)
			{
				free(buf);
				Sink->WriteLog(lev, MSGTOOBIG, strlen(MSGTOOBIG));
				Sink->WriteLog(lev, formatMsg, strlen(formatMsg));
				return;
			}
			buf[WINHEAPSIZE - 1] = 0;
		}
#else
		if (r >= STATICBUFSIZE)
		{
			buf = (char*) malloc(r + 1);
			if (!buf)
			{
				Sink->WriteLog(lev, OOMSG, strlen(OOMSG));
				Sink->WriteLog(lev, formatMsg, strlen(formatMsg));
				return;
			}
			r = vsnprintf(buf, r + 1, formatMsg, va);
			if (r == -1)
			{
				free(buf);
				Sink->WriteLog(lev, MSGTOOBIG_NIX, strlen(MSGTOOBIG_NIX));
				Sink->WriteLog(lev, formatMsg, strlen(formatMsg));
				return;
			}
			buf[r] = 0;
		}
#endif

		Sink->WriteLog(lev, buf, strlen(buf));

		if (buf != static_buf)
			free(buf);
	}

	void Logger::Debug(MICROLOG_FORMAT_STRING const char* formatMsg, ...)
	{
		va_list va;
		va_start(va, formatMsg);
		LogVA(Level::Debug, formatMsg, va);
		va_end(va);
	}

	void Logger::Info(MICROLOG_FORMAT_STRING const char* formatMsg, ...)
	{
		va_list va;
		va_start(va, formatMsg);
		LogVA(Level::Info, formatMsg, va);
		va_end(va);
	}

	void Logger::Warn(MICROLOG_FORMAT_STRING const char* formatMsg, ...)
	{
		va_list va;
		va_start(va, formatMsg);
		LogVA(Level::Warn, formatMsg, va);
		va_end(va);
	}

	void Logger::Error(MICROLOG_FORMAT_STRING const char* formatMsg, ...)
	{
		va_list va;
		va_start(va, formatMsg);
		LogVA(Level::Error, formatMsg, va);
		va_end(va);
	}

	void Logger::SetFile(const char* filename)
	{
		delete Sink;
		auto file = new FileSink();
		file->Filename = filename;
		auto formatter = new FormatSink();
		formatter->Next = file;
		Sink = formatter;
	}

}

#endif // MICROLOG_IMPLEMENTATION

#undef MICROLOG_FORMAT_STRING