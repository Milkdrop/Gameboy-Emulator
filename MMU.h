#include <stdint.h>
#include <stdio.h>
#include <cstring>
#ifndef MMU_H
#define MMU_H

class MMU {
	public:
		MMU ();
		uint8_t GetByteAt (uint16_t Address);
		void SetByteAt (uint16_t Address, uint8_t Value);
		void SetBytesAt (uint16_t Address, uint8_t* Buffer, size_t BufferSize);
		
		uint16_t GetWordAt (uint16_t Address);
		void SetWordAt (uint16_t Address, uint16_t Value);
		
		/* Memory Layout:
			Interrupt Register:			0xFFFF
			Internal RAM:				0xFF80
			Empty, unusable for I/O:	0xFF4C
			Memory Mapped I/O:			0xFF00
			Empty, unusable for I/O:	0xFEA0
			Sprite Attrib Memory (OAM): 0xFE00
			8KB Internal RAM Mirrored:	0xE000
			8KB Internal RAM:			0xC000
			8KB switchable RAM bank: 	0xA000
			8KB VRAM: 					0x8000
 			16KB Switchable ROM bank:	0x4000
			16KB ROM bank #0:			0x0000
		*/
	
		// ROM Config
		uint8_t ROM [8 * 1024 * 1024]; // 8MB Max
		uint8_t ExternalRAM [0x2000][16]; // 16 RAM Banks Max
		uint8_t ROMType = 0;
		uint8_t ROMBattery = 0;
		uint8_t ROMRAM = 0;
	
		// ROM Status
		uint8_t ExternalRAMEnabled = 0;
		uint8_t CurrentRAMBank = 0;
		uint8_t CurrentROMBank = 1;
		uint8_t SelectRAMBank = 0;
		uint8_t RTCRegister [0x0D];
	
		// Convenience Pointers
		uint8_t* IOMap = Memory + 0xFF00;
	
		uint8_t Memory[0x10000];
};

#endif