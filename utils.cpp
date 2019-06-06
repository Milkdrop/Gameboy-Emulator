#include "utils.h"

using namespace Utils;
using namespace std::chrono;

uint8_t Utils::GetBit (uint8_t Value, uint8_t BitNo) {
	return (Value & (1 << BitNo)) != 0;
}

void Utils::SetBit (uint8_t &Value, uint8_t BitNo, uint8_t Set) {
	if (Set)
		Value |= 1 << BitNo;
	else
		Value &= 0xFF ^ (1 << BitNo);
}

void Utils::MicroSleep (uint32_t us) {
	struct timespec req = {0, us * 1000};
	nanosleep (&req, (struct timespec *) NULL);
}

uint64_t Utils::GetCurrentTime (time_point <high_resolution_clock>* StartTime) {
	auto TimeDifference = high_resolution_clock::now () - *StartTime;
	return duration_cast <microseconds> (TimeDifference).count (); // In Microseconds
}