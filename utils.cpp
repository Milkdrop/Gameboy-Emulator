#include "utils.h"

using namespace Utils;

namespace Utils {
	StartTime = std::chrono::high_resolution_clock::now ();
}

uint8_t Utils::GetBit (uint8_t Value, uint8_t BitNo) {
	return (Value & (1 << BitNo)) != 0;
}

void Utils::SetBit (uint8_t &Value, uint8_t BitNo, uint8_t Set) {
	if (Set) {
		Value |= 1 << BitNo;
	} else {
		Value &= 0xFF ^ (1 << BitNo);
	}
}

void Utils::NanoSleep (uint32_t ms) {
	struct timespec req = {0, 0};
	req.tv_sec = 0;
	req.tv_nsec = ms * 1000000L;
	nanosleep (&req, (struct timespec *)NULL);
}

uint64_t Utils::GetCurrentTime () {
	auto ElapsedTime = std::chrono::high_resolution_clock::now () - Utils::StartTime;
	return std::chrono::duration_cast<std::chrono::microseconds> (ElapsedTime).count (); // In Microseconds
}