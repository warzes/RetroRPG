#pragma once

#include <stdint.h>

class Timing
{
public:
	void Setup(int targetFPS);

	void FirstFrame();
	void LastFrame();

	// Is it the time to display the next frame?
	bool IsNextFrame();
	// Wait until it is time to display the next frame
	void WaitNextFrame();

	float fps = 0.0f;          // current game FPS
	bool enableDisplay = true; // periodically displays FPS in console

private:
	void display();

	int64_t m_countsPerSec = 0;   // number of ticks per second
	int64_t m_countsPerFrame = 0; // number of ticks per frame
	int64_t m_lastCounter = 0;    // counter last tick number
};

extern Timing timing;