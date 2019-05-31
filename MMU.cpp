#include <cstring>
#include <stdio.h>
#include "MMU.h"

MMU::MMU () {
	memset (Memory, 0, sizeof(Memory));
}

uint8_t MMU::GetByteAt (uint16_t Address) {
	return Memory [Address];
}

void MMU::SetByteAt (uint16_t Address, uint8_t Value) {
	Memory [Address] = Value;
}

void MMU::SetBytesAt (uint16_t Address, uint8_t* Buffer, size_t BufferSize) {
	memcpy (Memory + Address, Buffer, BufferSize);
}