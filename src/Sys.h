#pragma once

namespace sx
{

struct Timer
{
	std::chrono::system_clock::time_point	Start;
	Timer()
	{
		Start = std::chrono::high_resolution_clock::now();
	}
	double DurationMS() const
	{
		return (double) std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - Start).count();
	}
};

typedef std::lock_guard<std::mutex> LockGuard;
//struct LockGuard
//{
//	std::mutex* M;
//	TakeMutex(std::mutex& m) : M(&m)
//	{
//		M->lock();
//	}
//	~TakeMutex()
//	{
//		M->unlock();
//	}
//};

template<typename T> T Clamp(T v, T low, T high) { return v < low ? low : (v > high ? high : v); }

}

