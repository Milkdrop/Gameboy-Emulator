#include <stdint.h>
#include "utils.h"

uint8_t GetBit (uint8_t Value, uint8_t BitNo) {
	return (Value & (1 << BitNo)) != 0;
}

void SetBit (uint8_t &Value, uint8_t BitNo, uint8_t Set) {
	if (Set) {
		Value |= 1 << BitNo;
	} else {
		Value &= 0xFF ^ (1 << BitNo);
	}
}
