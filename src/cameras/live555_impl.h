#pragma once

// live555_impl.cpp and live555_impl.h contain live555 implementation details that internal
// consumers of the live555 interface needn't see.

#ifdef SX_LIVE555

#include "../util/Buffer.h"

namespace sx
{

class H264Decoder;

class MyUsageEnvironment : public BasicUsageEnvironment {
public:
	static MyUsageEnvironment* createNew(TaskScheduler& taskScheduler);

	// redefined virtual functions:
	virtual UsageEnvironment& operator<<(char const* str) override;
	virtual UsageEnvironment& operator<<(int i) override;
	virtual UsageEnvironment& operator<<(unsigned u) override;
	virtual UsageEnvironment& operator<<(double d) override;
	virtual UsageEnvironment& operator<<(void* p) override;

protected:
	std::string Buffer;

	MyUsageEnvironment(TaskScheduler& taskScheduler);
	// called only by "createNew()" (or subclass constructors)
	virtual ~MyUsageEnvironment();
};

// Define a class to hold per-stream state that we maintain throughout each stream's lifetime
class StreamClientState
{
public:
	StreamClientState();
	virtual ~StreamClientState();

public:
	MediaSubsessionIterator* iter = nullptr;
	MediaSession* session = nullptr;
	MediaSubsession* subsession = nullptr;
	TaskToken streamTimerTask = nullptr;
	double duration = 0;
};

class MySink : public MediaSink
{
public:
	static MySink* createNew(UsageEnvironment& env,
		MediaSubsession& subsession, // identifies the kind of data that's being received
		char const* streamId = NULL); // identifies the stream itself (optional)

	Image* NextFrame();

private:
	MySink(UsageEnvironment& env, MediaSubsession& subsession, char const* streamId);
	// called only by "createNew()"
	virtual ~MySink();

	static void afterGettingFrame(void* clientData, unsigned frameSize, unsigned numTruncatedBytes, struct timeval presentationTime, unsigned durationInMicroseconds);
	void afterGettingFrame(unsigned frameSize, unsigned numTruncatedBytes, struct timeval presentationTime, unsigned durationInMicroseconds);

private:
	// redefined virtual functions:
	virtual Boolean continuePlaying();

private:
	MediaSubsession&		fSubsession;
	char*					fStreamId;
	size_t					ReceiveBufferSize = 512 * 1024 - 4; // The -4 is because we add 4 bytes of NALU start code
	//uint8_t*				ReceiveBuffer = nullptr;
	H264Decoder*			Decoder = nullptr;
	ByteBuf					StreamBuf;
	//std::vector<uint8_t>	AllBuf;
	//ring::ByteBuf			StreamBuf;
};

// If you're streaming just a single stream (i.e., just from a single URL, once), then you can define and use just a single
// "StreamClientState" structure, as a global variable in your application.  However, because - in this demo application - we're
// showing how to play multiple streams, concurrently, we can't do that.  Instead, we have to have a separate "StreamClientState"
// structure for each "RTSPClient".  To do this, we subclass "RTSPClient", and add a "StreamClientState" field to the subclass:
class MyRTSPClient : public RTSPClient
{
public:
	StreamClientState scs;
	MySink* Sink = nullptr;

	static MyRTSPClient* createNew(UsageEnvironment& env, char const* rtspURL,
		int verbosityLevel = 0,
		char const* applicationName = NULL,
		portNumBits tunnelOverHTTPPortNum = 0);

	void	ContinueAfterDESCRIBE(int resultCode, char* resultString);

protected:
	MyRTSPClient(UsageEnvironment& env, char const* rtspURL, int verbosityLevel, char const* applicationName, portNumBits tunnelOverHTTPPortNum);
	// called only by createNew();
	virtual ~MyRTSPClient();

	void SetupNextSubsession();
	void ContinueAfterSETUP(int resultCode, char* resultString);

	static void ContinueAfterPLAY(RTSPClient* rtspClient, int resultCode, char* resultString);
	static void SubsessionByeHandler(void* clientData);
	static void SubsessionAfterPlaying(void* clientData);
	static void ShutdownStream(RTSPClient* rtspClient, int exitCode);

};

}

#endif