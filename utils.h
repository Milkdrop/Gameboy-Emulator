#include <stdint.h>
#include <time.h>
#include <chrono>

#ifndef UTILS_H
#define UTILS_H

namespace Utils {
	extern std::chrono::time_point<std::chrono::high_resolution_clock> StartTime;

	uint8_t GetBit (uint8_t Value, uint8_t BitNo);
	void SetBit (uint8_t &Value, uint8_t BitNo, uint8_t Set);
	void NanoSleep (uint32_t ms);
	uint64_t GetCurrentTime ();
}

#endif