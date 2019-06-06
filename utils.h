#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include <chrono>

#ifndef UTILS_H
#define UTILS_H

using namespace std::chrono;

namespace Utils {
	uint8_t GetBit (uint8_t Value, uint8_t BitNo);
	void SetBit (uint8_t &Value, uint8_t BitNo, uint8_t Set);
	void MicroSleep (uint32_t us);
	uint64_t GetCurrentTime (time_point <high_resolution_clock>* StartTime);
}

#endif