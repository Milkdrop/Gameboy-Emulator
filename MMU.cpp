#include "MMU.h"

MMU::MMU () {
	memset (Memory, 0, sizeof(Memory));
}

uint8_t MMU::GetByteAt (uint16_t Address) {
	return Memory [Address];
}

void MMU::SetByteAt (uint16_t Address, uint8_t Value) {
	switch (Address) {
		case 0xFF01: printf ("%c", Value); fflush (stdout); break; // SB
		case 0xFF04: Value = 0; break; // DIV Register, Always write 0
		case 0xFF46: memcpy (Memory + 0xFE00, Memory + (Value << 8), 0xA0); break; // DMA
		default: break;
	}
	
	if (Address >= 0xE000 && Address < 0xFE00) // 8KB Internal
		Address -= 0x2000;
	Memory [Address] = Value;
}

uint16_t MMU::GetWordAt (uint16_t Address) {
	if (Address >= 0xE000 && Address < 0xFE00) // 8KB Internal
		Address -= 0x2000;
	
	return (GetByteAt (Address + 1) << 8) + GetByteAt (Address);
}

void MMU::SetWordAt (uint16_t Address, uint16_t Value) {
	SetByteAt (Address + 1, Value >> 8);
	SetByteAt (Address, Value & 0xFF);
}

void MMU::SetBytesAt (uint16_t Address, uint8_t* Buffer, size_t BufferSize) {
	memcpy (Memory + Address, Buffer, BufferSize);
}