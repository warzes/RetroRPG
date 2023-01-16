#include "Timing.h"
#include <stdlib.h>
#include <stdio.h>
//-----------------------------------------------------------------------------
#if defined(_WIN32)
//#	define WIN32_LEAN_AND_MEAN 1
#	define WIN_32_EXTRA_LEAN 1
#	define NOMINMAX
#	define WINVER 0x0600
#	include <SDKDDKVer.h>
#	include <winapifamily.h>
#	include <Windows.h>
#endif // _WIN32

#if defined(__linux__)
#include <stdlib.h>
#include <stdio.h>

#include <unistd.h>
#include <inttypes.h>
#include <time.h>
#endif // __linux__

#if defined(__EMSCRIPTEN__)
#endif // __EMSCRIPTEN__
//-----------------------------------------------------------------------------
#if defined(_WIN32)
#define TIMING_SCHEDULER_GRANULARITY	1
#elif defined(__linux__)
#define TIMING_SCHEDULER_GRANULARITY	1000
#endif
//-----------------------------------------------------------------------------
Timing timing;
//-----------------------------------------------------------------------------
void Timing::Setup(int targetFPS)
{
#if defined(_WIN32)
	LARGE_INTEGER pf;
	QueryPerformanceFrequency(&pf);

	m_countsPerSec = pf.QuadPart;
	m_countsPerFrame = m_countsPerSec / targetFPS;
	printf("Timing: counts per seconds: %I64d\n", m_countsPerSec);
	printf("Timing: counts per frame: %I64d\n", m_countsPerFrame);
#endif // _WIN32

#if defined(__linux__)
	struct timespec tp;
	int r = clock_getres(CLOCK_MONOTONIC_RAW, &tp);
	if( r < 0 )
	{
		printf("Timing: cannot retrieve clock resolution\n");
		return;
	}

	int16_t cr = (int64_t)tp.tv_sec * 1000000000 + tp.tv_nsec;
	printf("Timing: clock resolution (ms): %f\n", cr / 1000000.0f);

	countsPerSec = 1000000000;
	countsPerFrame = countsPerSec / targetFPS;
	printf("Timing: counts per seconds: %" PRId64 "\n", countsPerSec);
	printf("Timing: counts per frame: %" PRId64 "\n", countsPerFrame);
#endif // __linux__
}
//-----------------------------------------------------------------------------
void Timing::FirstFrame()
{
#if defined(_WIN32)
	LARGE_INTEGER pc;
	QueryPerformanceCounter(&pc);
	m_lastCounter = pc.QuadPart;
	timeBeginPeriod(TIMING_SCHEDULER_GRANULARITY);
#endif // _WIN32

#if defined(__linux__)
	struct timespec tp;
	clock_gettime(CLOCK_MONOTONIC_RAW, &tp);
	lastCounter = (int64_t)tp.tv_sec * 1000000000 + tp.tv_nsec;
#endif // __linux__
}
//-----------------------------------------------------------------------------
void Timing::LastFrame()
{
#if defined(_WIN32)
	timeEndPeriod(TIMING_SCHEDULER_GRANULARITY);
#endif // _WIN32
}
//-----------------------------------------------------------------------------
bool Timing::IsNextFrame()
{
#if defined(_WIN32)
	LARGE_INTEGER pc;
	QueryPerformanceCounter(&pc);

	const int64_t dt = pc.QuadPart - m_lastCounter;
	if( dt < m_countsPerFrame ) return false;
	m_lastCounter = pc.QuadPart;

	fps = (float)m_countsPerSec / dt;
	if( enableDisplay ) display();

	return true;
#endif // _WIN32

#if defined(__linux__)
	struct timespec tp;
	clock_gettime(CLOCK_MONOTONIC_RAW, &tp);
	int64_t pc = (int64_t)tp.tv_sec * 1000000000 + tp.tv_nsec;

	int64_t dt = pc - lastCounter;
	if( dt < countsPerFrame ) return false;
	lastCounter = pc;

	fps = (float)countsPerSec / dt;
	if( enableDisplay ) display();
	return true;
#endif // __linux__
}
//-----------------------------------------------------------------------------
void Timing::WaitNextFrame()
{
#if defined(_WIN32)
	while( 1 )
	{
		LARGE_INTEGER pc;
		QueryPerformanceCounter(&pc);

		const int64_t dt = pc.QuadPart - m_lastCounter;
		const int64_t dg = m_countsPerFrame - dt;
		if( dg <= 0 ) break;

		const int64_t ms = (1000 * dg) / m_countsPerSec - TIMING_SCHEDULER_GRANULARITY;
		if( ms > 0 ) Sleep((uint16_t)ms);
	}

	LARGE_INTEGER pc;
	QueryPerformanceCounter(&pc);

	const int64_t dt = pc.QuadPart - m_lastCounter;
	m_lastCounter = pc.QuadPart;

	fps = (float)m_countsPerSec / dt;
	if( enableDisplay ) display();
#endif // _WIN32

#if defined(__linux__)
	while( 1 )
	{
		struct timespec tp;
		clock_gettime(CLOCK_MONOTONIC_RAW, &tp);
		int64_t pc = (int64_t)tp.tv_sec * 1000000000 + tp.tv_nsec;

		int64_t dt = pc - lastCounter;
		int64_t dg = countsPerFrame - dt;
		if( dg <= 0 ) break;

		int64_t us = (1000000 * dg) / countsPerSec;
		if( us > TIMING_SCHEDULER_GRANULARITY )
			usleep(us - TIMING_SCHEDULER_GRANULARITY);
	}

	struct timespec tp;
	clock_gettime(CLOCK_MONOTONIC_RAW, &tp);
	int64_t pc = (int64_t)tp.tv_sec * 1000000000 + tp.tv_nsec;

	int64_t dt = pc - lastCounter;
	lastCounter = pc;

	fps = (float)countsPerSec / dt;
	if( enableDisplay ) display();
#endif // __linux__
}
//-----------------------------------------------------------------------------
void Timing::display()
{
	static int ttd = 0;
	if( ttd++ == 30 )
	{
		printf("FPS %f\n", fps);
		ttd = 0;
	}
}
//-----------------------------------------------------------------------------