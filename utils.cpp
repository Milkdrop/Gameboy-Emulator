#include "utils.h"

using namespace Utils;
using namespace std::chrono;

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
	printf ("Nano sleeping for %d\n", ms);
	struct timespec req = {0, 0};
	req.tv_sec = 0;
	req.tv_nsec = ms * 1000000L;
	nanosleep (&req, (struct timespec *) NULL);
}

uint64_t Utils::GetCurrentTime (time_point <high_resolution_clock> StartTime) {
	auto CurrentTime = high_resolution_clock::now () - StartTime;
	return duration_cast <microseconds> (CurrentTime).count (); // In Microseconds
}