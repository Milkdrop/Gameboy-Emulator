#include "MMU.h"

MMU::MMU () {
	memset (Memory, 0, sizeof(Memory));
}

uint8_t MMU::GetByteAt (uint16_t Address) {
	return Memory [Address];
}

void MMU::SetByteAt (uint16_t Address, uint8_t Value) {
	if (Address == 0xFF01) {
		printf ("%c", Value);
	}
	
	Memory [Address] = Value;
}

uint16_t MMU::GetWordAt (uint16_t Address) {
	return (GetByteAt (Address + 1) << 8) + GetByteAt (Address);
}

void MMU::SetWordAt (uint16_t Address, uint16_t Value) {
	SetByteAt (Address + 1, Value >> 8);
	SetByteAt (Address, Value & 0xFF);
}

void MMU::SetBytesAt (uint16_t Address, uint8_t* Buffer, size_t BufferSize) {
	memcpy (Memory + Address, Buffer, BufferSize);
}