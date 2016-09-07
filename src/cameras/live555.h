#pragma once

/*
Use live555 to implement an RTSP client.
This code was based on the testRTSPClient.cpp sample program.
*/

#ifdef SX_LIVE555

#include "Cameras.h"

class RTSPClient;

namespace sx
{

class MyRTSPClient;

// This holds the state of the live555 event looper
// There is a single one of these inside Global
class Live555Environment
{
public:
	std::thread			Thread;					// The thread on which live555 code runs
	TaskScheduler*		Scheduler = nullptr;
	UsageEnvironment*	Env = nullptr;
	//std::mutex			EnvLock;				// Controls access to Scheduler and Env

	static void					Create();		// Instantiates the one and only global instance of ourselves
	static void					Destroy();		// Destroy our one-and-only instance (if recount reaches zero)
	
	// Retrieve the one-and-only global Live555Environment. Does not guarantee existence.
	// Avoid using this if you can. Instead, retrieve the environment from callbacks, etc.
	// This should only be used when creating new live555 objects.
	static Live555Environment*	Get();

	static void					ThreadFunc(Live555Environment* self);

	void Schedule(std::function<void()> func);

private:
	std::atomic_int32_t					RefCount;
	std::vector<std::function<void()>>	Queue;
	std::mutex							QueueLock;
};

class Live555Camera : public ICamera
{
public:
	bool	Open(std::string url);

	int		Width() override;
	int		Height() override;
	Image*	NextFrame() override;

private:
	MyRTSPClient*		RtspClient;
	std::string			OpenError;			// Result is available once OpenResultEvent is unlocked
	std::atomic_int32_t	OpenReady;

};

}

#endif