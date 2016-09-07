#include "pch.h" 
#include "../Common.h"
#include "live555_impl.h"
#include "Codecs.h"

#ifdef SX_LIVE555

namespace sx
{

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

MyUsageEnvironment::MyUsageEnvironment(TaskScheduler& taskScheduler) : BasicUsageEnvironment(taskScheduler)
{
}

MyUsageEnvironment::~MyUsageEnvironment()
{
}

MyUsageEnvironment* MyUsageEnvironment::createNew(TaskScheduler& taskScheduler)
{
	return new MyUsageEnvironment(taskScheduler);
}

UsageEnvironment& MyUsageEnvironment::operator<<(char const* str)
{
	size_t len = strlen(str);
	if (len > 0 && str[len - 1] == '\n')
	{
		Buffer.append(str, str + len - 1);
		Log()->LogS(microlog::Level::Info, Buffer.c_str());
		Buffer.clear();
	}
	else
	{
		Buffer.append(str);
	}
	return *this;
}

UsageEnvironment& MyUsageEnvironment::operator<<(int i)
{
	char buf[100];
	sprintf(buf, "%d", i);
	Buffer += buf;
	return *this;
}

UsageEnvironment& MyUsageEnvironment::operator<<(unsigned u)
{
	char buf[100];
	sprintf(buf, "%u", u);
	Buffer += buf;
	return *this;
}

UsageEnvironment& MyUsageEnvironment::operator<<(double d)
{
	char buf[200];
	sprintf(buf, "%f", d);
	Buffer += buf;
	return *this;
}

UsageEnvironment& MyUsageEnvironment::operator<<(void* p)
{
	char buf[100];
	sprintf(buf, "%p", p);
	Buffer += buf;
	return *this;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

StreamClientState::StreamClientState()
{
}

StreamClientState::~StreamClientState()
{
	delete iter;
	if (session != NULL)
	{
		// We also need to delete "session", and unschedule "streamTimerTask" (if set)
		UsageEnvironment& env = session->envir(); // alias

		env.taskScheduler().unscheduleDelayedTask(streamTimerTask);
		Medium::close(session);
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

MySink* MySink::createNew(UsageEnvironment& env, MediaSubsession& subsession, char const* streamId)
{
	return new MySink(env, subsession, streamId);
}

MySink::MySink(UsageEnvironment& env, MediaSubsession& subsession, char const* streamId)
	: MediaSink(env),
	fSubsession(subsession)
{
	fStreamId = strDup(streamId);
	//ReceiveBuffer = (uint8_t*) malloc(ReceiveBufferSize);
#ifdef SX_INTEL_MEDIA_SDK
	Decoder = new IntelH264Decoder();
	if (Decoder->Init())
		Log()->Info("Initialized IntelH264Decoder");
	else
		Log()->Error("Failed to initialize IntelH264Decoder");
#endif
}

MySink::~MySink()
{
	//free(ReceiveBuffer);
	//delete[] fReceiveBuffer;
	delete[] fStreamId;
	delete Decoder;
}

Image* MySink::NextFrame()
{
	return Decoder->ReadFrame();
}

void MySink::afterGettingFrame(void* clientData, unsigned frameSize, unsigned numTruncatedBytes, struct timeval presentationTime, unsigned durationInMicroseconds)
{
	MySink* sink = (MySink*) clientData;
	sink->afterGettingFrame(frameSize, numTruncatedBytes, presentationTime, durationInMicroseconds);
}

// If you don't want to see debugging output for each received frame, then comment out the following line:
//#define DEBUG_PRINT_EACH_RECEIVED_FRAME 1

static int NFrames = 0;
ring::Buf<uint64> FrameTimes;
double lastTime = 0;
double avgTime = 0;

void MySink::afterGettingFrame(unsigned frameSize, unsigned numTruncatedBytes, struct timeval presentationTime, unsigned durationInMicroseconds)
{
	// We've just received a frame of data.  (Optionally) print out information about it:
#ifdef DEBUG_PRINT_EACH_RECEIVED_FRAME
	if (fStreamId != NULL) envir() << "Stream \"" << fStreamId << "\"; ";
	envir() << fSubsession.mediumName() << "/" << fSubsession.codecName() << ":\tReceived " << frameSize << " bytes";
	if (numTruncatedBytes > 0) envir() << " (with " << numTruncatedBytes << " bytes truncated)";
	char uSecsStr[6 + 1]; // used to output the 'microseconds' part of the presentation time
	sprintf(uSecsStr, "%06u", (unsigned) presentationTime.tv_usec);
	envir() << ".\tPresentation time: " << (int) presentationTime.tv_sec << "." << uSecsStr;
	if (fSubsession.rtpSource() != NULL && !fSubsession.rtpSource()->hasBeenSynchronizedUsingRTCP())
		envir() << "!"; // mark the debugging output to indicate that this presentation time is not RTCP-synchronized
#ifdef DEBUG_PRINT_NPT
	envir() << "\tNPT: " << fSubsession.getNormalPlayTime(presentationTime);
#endif
	envir() << "\n";
#endif

	/*
	const char* sps_str = fSubsession.fmtp_spropparametersets();
	unsigned int nRecords = 0;
	SPropRecord* sps = parseSPropParameterSets(sps_str, nRecords);

	uint total = 0;
	for (uint i = 0; i < nRecords; i++)
		total += sps[i].sPropLength;

	uint8_t* sps_all = (uint8_t*) malloc(total + frameSize);
	uint8_t* sps_all_out = sps_all;

	for (uint i = 0; i < nRecords; i++)
	{
		memcpy(sps_all_out, sps[i].sPropBytes, sps[i].sPropLength);
		sps_all_out += sps[i].sPropLength;
	}
	*/

	//memcpy(sps_all_out, ReceiveBuffer, frameSize);

	SXASSERT(StreamBuf.Len + frameSize <= StreamBuf.Cap);
	StreamBuf.Len += frameSize;

	int nBefore = Decoder->FramesInQueue();

	//Decoder->Decode(sps_all, frameSize + total);
	size_t consumed = Decoder->AddData(StreamBuf.Data, StreamBuf.Len);
	//size_t consumed = StreamBuf.Len;

	int newFrames = Decoder->FramesInQueue() - nBefore;
	//int newFrames = 1;
	double npt = fSubsession.getNormalPlayTime(presentationTime);
	//for (int i = 0; i < newFrames; i++)
	//{
	//}
	if (newFrames != 0)
	{
		double tNow = presentationTime.tv_sec + presentationTime.tv_usec / 1000000.0;
		if (lastTime != 0)
		{
			double elapsed = (tNow - lastTime) / (double) newFrames;
			if (avgTime == 0)
				avgTime = elapsed;
			else
				avgTime = 0.99 * avgTime + 0.01 * elapsed;
		}
		lastTime = tNow;
	}

	//Log()->Info("lastTime: %f, npt: %f", lastTime, npt);

	if (NFrames++ % 30 == 0)
	{
		Log()->Info("%.2f FPS", 1.0 / avgTime);
	}

	if (consumed == StreamBuf.Len)
	{
		// assume this is the common case (ie that all of the data is consumed)
		StreamBuf.Len = 0;
	}
	else if (consumed > 0)
	{
		// this must be rare for us to be efficient. Otherwise we need a "magic ring buffer"
		Log()->Info("Consumed %d/%d of buffer", (int) consumed, (int) StreamBuf.Len);
		ByteBuf copy;
		size_t remain = StreamBuf.Len - consumed;
		copy.Ensure(remain);
		copy.Len = remain;
		memcpy(copy.Data, StreamBuf.Data + consumed, remain);
		std::swap(StreamBuf, copy);
	}
	//free(sps_all);
	//delete[] sps;

	// Then continue, to request the next frame of data:
	continuePlaying();
}

Boolean MySink::continuePlaying()
{
	if (fSource == NULL) return False; // sanity check (should not happen)

	// Add a NALU start code (see http://yumichan.net/video-processing/video-compression/introduction-to-h264-nal-unit/)
	// More here: https://codesequoia.wordpress.com/2009/10/18/h-264-stream-structure/
	// This seems to have some erroneous details, but can be helpful: http://stackoverflow.com/questions/24884827/possible-locations-for-sequence-picture-parameter-sets-for-h-264-stream
	StreamBuf.Push(0);
	StreamBuf.Push(0);
	StreamBuf.Push(0);
	StreamBuf.Push(1);

	// Ensure there is enough space to store whatever frame we're about to receive
	StreamBuf.Ensure(StreamBuf.Len + ReceiveBufferSize);

	// Request the next frame of data from our input source.  "afterGettingFrame()" will get called later, when it arrives:
	fSource->getNextFrame(StreamBuf.Data + StreamBuf.Len, ReceiveBufferSize, afterGettingFrame, this, onSourceClosure, this);
	return True;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

MyRTSPClient* MyRTSPClient::createNew(UsageEnvironment& env, char const* rtspURL, int verbosityLevel, char const* applicationName, portNumBits tunnelOverHTTPPortNum)
{
	return new MyRTSPClient(env, rtspURL, verbosityLevel, applicationName, tunnelOverHTTPPortNum);
}

MyRTSPClient::MyRTSPClient(UsageEnvironment& env, char const* rtspURL, int verbosityLevel, char const* applicationName, portNumBits tunnelOverHTTPPortNum)
	: RTSPClient(env, rtspURL, verbosityLevel, applicationName, tunnelOverHTTPPortNum, -1)
{
}

MyRTSPClient::~MyRTSPClient()
{
}

void MyRTSPClient::ContinueAfterDESCRIBE(int resultCode, char* resultString)
{
	do
	{
		UsageEnvironment& env = envir(); // alias

		if (resultCode != 0)
		{
			Log()->Error("Failed to get a SDP description: %s", resultString);
			delete[] resultString;
			break;
		}

		char* const sdpDescription = resultString;
		Log()->Info("Got a SDP description: %s", sdpDescription);

		// Create a media session object from this SDP description:
		scs.session = MediaSession::createNew(env, sdpDescription);
		delete[] sdpDescription; // because we don't need it anymore
		if (scs.session == NULL)
		{
			Log()->Error("Failed to create a MediaSession object from the SDP description: %s", env.getResultMsg());
			break;
		}
		else if (!scs.session->hasSubsessions())
		{
			Log()->Error("This session has no media subsessions (i.e., no \"m=\" lines)");
			break;
		}

		// Then, create and set up our data source objects for the session.  We do this by iterating over the session's 'subsessions',
		// calling "MediaSubsession::initiate()", and then sending a RTSP "SETUP" command, on each one.
		// (Each 'subsession' will have its own data source.)
		scs.iter = new MediaSubsessionIterator(*scs.session);
		SetupNextSubsession();
		return;
	} while (0);

	// An unrecoverable error occurred with this stream.
	//shutdownStream(rtspClient);
}

// By default, we request that the server stream its data using RTP/UDP.
// If, instead, you want to request that the server stream via RTP-over-TCP, change the following to True:
static const bool REQUEST_STREAMING_OVER_TCP = false;

void MyRTSPClient::SetupNextSubsession()
{
	UsageEnvironment& env = envir(); // alias

	scs.subsession = scs.iter->next();
	if (scs.subsession != NULL)
	{
		if (!scs.subsession->initiate())
		{
			Log()->Error("Failed to initiate the %s subsession: %s", scs.subsession->sessionId(), env.getResultMsg());
			SetupNextSubsession(); // give up on this subsession; go to the next one
		}
		else
		{
			Log()->Info("Initiated the %s subsession:", scs.subsession->sessionId());
			if (scs.subsession->rtcpIsMuxed())
				Log()->Info("client port %u", (uint32) scs.subsession->clientPortNum());
			else
				Log()->Info("client ports %u-%u", (uint32) scs.subsession->clientPortNum(), (uint32) scs.subsession->clientPortNum() + 1);

			// Continue setting up this subsession, by sending a RTSP "SETUP" command:
			auto next = [](RTSPClient* cbClient, int resultCode, char* resultString) {
				((MyRTSPClient*) cbClient)->ContinueAfterSETUP(resultCode, resultString);
			};
			sendSetupCommand(*scs.subsession, next, False, REQUEST_STREAMING_OVER_TCP);
		}
		return;
	}

	// We've finished setting up all of the subsessions.  Now, send a RTSP "PLAY" command to start the streaming:
	if (scs.session->absStartTime() != NULL)
	{
		// Special case: The stream is indexed by 'absolute' time, so send an appropriate "PLAY" command:
		sendPlayCommand(*scs.session, ContinueAfterPLAY, scs.session->absStartTime(), scs.session->absEndTime());
	}
	else
	{
		scs.duration = scs.session->playEndTime() - scs.session->playStartTime();
		sendPlayCommand(*scs.session, ContinueAfterPLAY);
	}
}

void MyRTSPClient::ContinueAfterSETUP(int resultCode, char* resultString)
{
	do
	{
		UsageEnvironment& env = envir(); // alias

		if (resultCode != 0)
		{
			Log()->Error("Failed to set up the %s subsession: %s", scs.subsession->sessionId(), resultString);
			break;
		}

		Log()->Info("Set up the %s subsession", scs.subsession->sessionId());
		if (scs.subsession->rtcpIsMuxed())
			Log()->Info("client port: %u", (uint32) scs.subsession->clientPortNum());
		else
			Log()->Info("client ports: %u-%u", (uint32) scs.subsession->clientPortNum(), (uint32) scs.subsession->clientPortNum() + 1);

		// Having successfully setup the subsession, create a data sink for it, and call "startPlaying()" on it.
		// (This will prepare the data sink to receive data; the actual flow of data from the client won't start happening until later,
		// after we've sent a RTSP "PLAY" command.)

		Sink = MySink::createNew(env, *scs.subsession, url());
		scs.subsession->sink = Sink;
		// perhaps use your own custom "MediaSink" subclass instead
		//if (scs.subsession->sink == NULL)
		//{
		//	Log()->Error("Failed to create a data sink for the %s subsession: %s", scs.subsession->sessionId(), env.getResultMsg());
		//	break;
		//}

		Log()->Info("Created a data sink for the %s subsession", scs.subsession->sessionId());
		scs.subsession->miscPtr = this; // a hack to let subsession handler functions get the "RTSPClient" from the subsession 
		scs.subsession->sink->startPlaying(*(scs.subsession->readSource()), SubsessionAfterPlaying, scs.subsession);
		// Also set a handler to be called if a RTCP "BYE" arrives for this subsession:
		if (scs.subsession->rtcpInstance() != NULL)
			scs.subsession->rtcpInstance()->setByeHandler(SubsessionByeHandler, scs.subsession);
	} while (0);
	delete[] resultString;

	// Set up the next subsession, if any:
	SetupNextSubsession();
}

void MyRTSPClient::ContinueAfterPLAY(RTSPClient* rtspClient, int resultCode, char* resultString)
{
	Boolean success = False;
	MyRTSPClient* self = (MyRTSPClient*) rtspClient;

	do
	{
		UsageEnvironment& env = rtspClient->envir(); // alias
		StreamClientState& scs = ((MyRTSPClient*) rtspClient)->scs; // alias

		if (resultCode != 0)
		{
			Log()->Error("Failed to start playing session: %s", resultString);
			break;
		}

		// Set a timer to be handled at the end of the stream's expected duration (if the stream does not already signal its end
		// using a RTCP "BYE").  This is optional.  If, instead, you want to keep the stream active - e.g., so you can later
		// 'seek' back within it and do another RTSP "PLAY" - then you can omit this code.
		// (Alternatively, if you don't want to receive the entire stream, you could set this timer for some shorter value.)
		/*
		if (scs.duration > 0)
		{
			unsigned const delaySlop = 2; // number of seconds extra to delay, after the stream's expected duration.  (This is optional.)
			scs.duration += delaySlop;
			unsigned uSecsToDelay = (unsigned) (scs.duration * 1000000);
			scs.streamTimerTask = env.taskScheduler().scheduleDelayedTask(uSecsToDelay, (TaskFunc*) streamTimerHandler, rtspClient);
		}
		*/

		Log()->Info("Started playing session");
		//if (scs.duration > 0)
		//	env << " (for up to " << scs.duration << " seconds)";

		success = True;
	} while (0);
	delete[] resultString;

	if (!success)
	{
		// An unrecoverable error occurred with this stream.
		ShutdownStream(rtspClient, 1);
	}
}

void MyRTSPClient::SubsessionAfterPlaying(void* clientData)
{
	MediaSubsession* subsession = (MediaSubsession*) clientData;
	MyRTSPClient* rtspClient = (MyRTSPClient*) (subsession->miscPtr);

	// Begin by closing this subsession's stream:
	Medium::close(subsession->sink);
	subsession->sink = NULL;

	// Next, check whether *all* subsessions' streams have now been closed:
	MediaSession& session = subsession->parentSession();
	MediaSubsessionIterator iter(session);
	while ((subsession = iter.next()) != NULL)
	{
		if (subsession->sink != NULL)
			return; // this subsession is still active
	}

	// All subsessions' streams have now been closed, so shutdown the client:
	ShutdownStream(rtspClient, 0);
}

void MyRTSPClient::SubsessionByeHandler(void* clientData)
{
	MediaSubsession* subsession = (MediaSubsession*) clientData;
	MyRTSPClient* rtspClient = (MyRTSPClient*) subsession->miscPtr;
	UsageEnvironment& env = rtspClient->envir(); // alias

	//env << *rtspClient << "Received RTCP \"BYE\" on \"" << *subsession << "\" subsession\n";
	env << "Received RTCP \"BYE\" on \"" << "\" subsession\n";

	// Now act as if the subsession had closed:
	SubsessionAfterPlaying(subsession);
}

void MyRTSPClient::ShutdownStream(RTSPClient* rtspClient, int exitCode)
{
	UsageEnvironment& env = rtspClient->envir(); // alias
	StreamClientState& scs = ((MyRTSPClient*) rtspClient)->scs; // alias

	// First, check whether any subsessions have still to be closed:
	if (scs.session != NULL)
	{
		Boolean someSubsessionsWereActive = False;
		MediaSubsessionIterator iter(*scs.session);
		MediaSubsession* subsession;

		while ((subsession = iter.next()) != NULL)
		{
			if (subsession->sink != NULL)
			{
				Medium::close(subsession->sink);
				subsession->sink = NULL;

				if (subsession->rtcpInstance() != NULL)
				{
					// in case the server sends a RTCP "BYE" while handling "TEARDOWN"
					subsession->rtcpInstance()->setByeHandler(NULL, NULL); 
				}

				someSubsessionsWereActive = True;
			}
		}

		if (someSubsessionsWereActive)
		{
			// Send a RTSP "TEARDOWN" command, to tell the server to shutdown the stream.
			// Don't bother handling the response to the "TEARDOWN".
			rtspClient->sendTeardownCommand(*scs.session, NULL);
		}
	}

	//env << *rtspClient << "Closing the stream.\n";
	Medium::close(rtspClient);
	// Note that this will also cause this stream's "StreamClientState" structure to get reclaimed.

	/*
	if (--rtspClientCount == 0)
	{
		// The final stream has ended, so exit the application now.
		// (Of course, if you're embedding this code into your own application, you might want to comment this out,
		// and replace it with "eventLoopWatchVariable = 1;", so that we leave the LIVE555 event loop, and continue running "main()".)
		exit(exitCode);
	}
	*/
}

}

#endif