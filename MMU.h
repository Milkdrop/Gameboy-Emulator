#include <stdint.h>
#ifndef MMU_H
#define MMU_H

class MMU {
	public:
		MMU ();
		uint8_t GetByteAt (uint16_t Address);
		void SetByteAt (uint16_t Address, uint8_t Value);
		void SetBytesAt (uint16_t Address, uint8_t* Buffer, size_t BufferSize);
	
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
	
		// Convenience Pointers
		const uint8_t* VRAM = Memory + 0x8000;
	private:
		uint8_t Memory[0x10000];
};

#endif