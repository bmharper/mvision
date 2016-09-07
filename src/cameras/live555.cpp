#include "pch.h" 
#include "live555.h" 
#include "live555_impl.h" 
#include "../Common.h" 
#include "../image/image.h"

#ifdef SX_LIVE555

namespace sx
{

void Live555Environment::Create()
{
	if (Global.Live555Env)
	{
		Global.Live555Env->RefCount++;
		return;
	}

	auto env = new Live555Environment();
	env->RefCount = 1;
	env->Thread = std::thread(ThreadFunc, env);
	Global.Live555Env = env;
}

void Live555Environment::Destroy()
{
	Global.Live555Env->RefCount--;
}

Live555Environment* Live555Environment::Get()
{
	return Global.Live555Env;
}

void Live555Environment::ThreadFunc(Live555Environment* self)
{
	BasicTaskScheduler* scheduler = BasicTaskScheduler::createNew();
	self->Scheduler = scheduler;
	self->Env = MyUsageEnvironment::createNew(*self->Scheduler);
	Global.Live555Env = self;

	while (self->RefCount != 0)
	{
		// run the live555 event loop
		//self->EnvLock.lock();
		scheduler->SingleStep(1000 * 1000);
		//self->EnvLock.unlock();

		// copy all queue items out and clear queue
		self->QueueLock.lock();
		auto queueItems = self->Queue;
		self->Queue.clear();
		self->QueueLock.unlock();

		// run the queue items
		for (const auto& qi : queueItems)
			qi();
	}

	Global.Live555Env = nullptr;
	self->Env->reclaim();
	delete self->Scheduler;
	delete self;
}

void Live555Environment::Schedule(std::function<void()> func)
{
	LockGuard lock(QueueLock);
	Queue.push_back(func);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool Live555Camera::Open(std::string url)
{
	auto env = Live555Environment::Get();

	// Damn lazy sync primitive. I need to figure out how to do this properly using C++11 primitives
	OpenReady = 0;
	OpenError = "";

	auto initiateOpen = [env, url, this]() -> void
	{
		const int RTSP_CLIENT_VERBOSITY_LEVEL = 1; // by default, print verbose output from each "RTSPClient"
		const char* progName = "SxMonitor";

		// Begin by creating a "RTSPClient" object.  Note that there is a separate "RTSPClient" object for each stream that we wish
		// to receive (even if more than stream uses the same "rtsp://" URL).
		RtspClient = MyRTSPClient::createNew(*env->Env, url.c_str(), RTSP_CLIENT_VERBOSITY_LEVEL, progName);
		if (RtspClient == nullptr)
		{
			OpenError = "Failed to create a RTSP client for URL \"" + url + "\": " + env->Env->getResultMsg() + "\n";
		}
		OpenReady = 1;

		//++rtspClientCount;

		// Next, send a RTSP "DESCRIBE" command, to get a SDP description for the stream.
		// Note that this command - like all RTSP commands - is sent asynchronously; we do not block, waiting for a response.
		// Instead, the following function call returns immediately, and we handle the RTSP response later, from within the event loop:
		RtspClient->sendDescribeCommand([](RTSPClient* cbClient, int resultCode, char* resultString) -> void {
			((MyRTSPClient*) cbClient)->ContinueAfterDESCRIBE(resultCode, resultString);
		});
	};

	env->Schedule(initiateOpen);

	// Wait for initiateOpen() to signal a result.
	// See above comment - this is lazy and dumb.
	while (OpenReady == 0)
		std::this_thread::sleep_for(std::chrono::milliseconds(1));

	return OpenError == "";
}

int Live555Camera::Width()
{
	return 0;
}

int Live555Camera::Height()
{
	return 0;
}

Image* Live555Camera::NextFrame()
{
	if (RtspClient != nullptr && RtspClient->Sink != nullptr)
	{
		return RtspClient->Sink->NextFrame();
	}
	return nullptr;
}

}

#endif