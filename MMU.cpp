#include "MMU.h"

MMU::MMU () {
	memset (Memory, 0, sizeof(Memory));
}

uint8_t MMU::GetByteAt (uint16_t Address) {
	if (Address >= 0xE000 && Address < 0xFE00) // 8KB Internal RAM Echo
		Address -= 0x2000;
	
	if (ROMType == 1) {
		if (ExternalRAMEnabled) {
			if (Address >= 0xA000 && Address < 0xC000) { // Read from External RAM
				return ExternalRAM [Address - 0xA000][CurrentRAMBank];
			}
		}
		
		if (Address >= 0x4000 && Address < 0x8000) // Switchable Bank
			return ROM [0x4000 * CurrentROMBank + (Address - 0x4000)];
	} else if (ROMType == 3) {
		if (Address >= 0xA000 && Address < 0xC000) { // Read from External RAM / RTC
			if (CurrentRAMBank <= 0x07)
				return ExternalRAM [Address - 0xA000][CurrentRAMBank];
			else
				return RTCRegister [CurrentRAMBank];
		}
		
		if (Address >= 0x4000 && Address < 0x8000) { // Switchable Bank
			return ROM [0x4000 * CurrentROMBank + (Address - 0x4000)];
		}
	}
	
	return Memory [Address];
}

void MMU::SetByteAt (uint16_t Address, uint8_t Value) {
	switch (Address) {
		case 0xFF01: printf ("%c", Value); fflush (stdout); break; // SB
		case 0xFF04: Value = 0; break; // DIV Register, Always write 0
		case 0xFF46: memcpy (Memory + 0xFE00, Memory + (Value << 8), 0xA0); break; // DMA
		default: break;
	}
	
	if (Address >= 0xE000 && Address < 0xFE00) // 8KB Internal RAM Echo
		Address -= 0x2000;
	
	if (ROMType == 1) {
		if (Address >= 0x0000 && Address <= 0x1FFF) { // Toggle External RAM
			if (Value == 0x0A)
				ExternalRAMEnabled = 1;
			else
				ExternalRAMEnabled = 0;
			return;
		} else if (Address >= 0x2000 && Address < 0x4000) { // Write lower 5 bits of ROM Bank
			CurrentROMBank &= 0b11100000;
			if (Value == 0) // Writing 0 translates to 1 in DMG
				Value = 1;
			
			CurrentROMBank |= Value & 0b00011111;
			return;
		} else if (Address >= 0x4000 && Address < 0x6000) { // Write upper 2 bits of ROM Bank (bit 7 is left as 0), or just select RAM Bank
			if (SelectRAMBank) { // Choose RAM Bank
				CurrentRAMBank = Value & 0b11;
			} else {
				CurrentROMBank &= 0b00011111;
				CurrentROMBank |= ((Value & 0b11) << 5);
			}
			return;
		} else if (Address >= 0x6000 && Address < 0x8000) { // Switch mode above
			if (Value == 0) {
				SelectRAMBank = 0;
				CurrentROMBank &= 0b00011111; // Clear the 2 bits to accommodate the change
			} else {
				SelectRAMBank = 1;
			}
			return;
		} else if (Address >= 0xA000 && Address < 0xC000) { // Write to External RAM
			if (ExternalRAMEnabled)
				ExternalRAM [Address - 0xA000][CurrentRAMBank] = Value;
			return;
		}
		
	} else if (ROMType == 3) {
		if (Address >= 0x0000 && Address < 0x2000) { // Toggle External RAM / RTC
			if (Value == 0x0A)
				ExternalRAMEnabled = 1;
			else
				ExternalRAMEnabled = 0;
			return;
		} else if (Address >= 0x2000 && Address < 0x4000) { // Choose ROM Bank
			if (Value == 0)
				Value = 1;
			
			CurrentROMBank = Value;
			return;
		} else if (Address >= 0x4000 && Address < 0x6000) { // Choose RAM / RTC Bank
			CurrentRAMBank = Value;
			return;
		} else if (Address >= 0xA000 && Address < 0xC000) { // Write to External RAM / RTC
			if (CurrentRAMBank <= 0x07)
				ExternalRAM [Address - 0xA000][CurrentRAMBank] = Value;
			else
				RTCRegister [CurrentRAMBank] = Value;
			return;
		} else if (Address >= 0x6000 && Address < 0x8000) {
			printf ("Latch RTC\n");
			return;
		}
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