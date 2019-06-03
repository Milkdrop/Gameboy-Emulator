#include <stdint.h>
#include <stdio.h>
#include <SDL2/SDL.h>
#include "MMU.h"
#ifndef CPU_H
#define CPU_H

class CPU {
	public:
		CPU (MMU* _mmu);
		void Clock ();
		void Debug ();
		void Interrupt (uint8_t ID);
	
		uint64_t ClockCount = 0;
		uint8_t Debugging = 0;
	private:
		MMU* mmu;
		void Execute (uint8_t Instruction);
	
		// Registers
		uint16_t reg_AF = 0;
		uint8_t* reg_A = ((uint8_t*) &reg_AF) + 1;
		uint8_t* reg_F = ((uint8_t*) &reg_AF); // Flags - Z N H C 0 0 0 0
		uint16_t reg_BC = 0;
		uint8_t* reg_B = ((uint8_t*) &reg_BC) + 1;
		uint8_t* reg_C = ((uint8_t*) &reg_BC);
		uint16_t reg_DE = 0;
		uint8_t* reg_D = ((uint8_t*) &reg_DE) + 1;
		uint8_t* reg_E = ((uint8_t*) &reg_DE);
		uint16_t reg_HL = 0;
		uint8_t* reg_H = ((uint8_t*) &reg_HL) + 1;
		uint8_t* reg_L = ((uint8_t*) &reg_HL);
		
		uint16_t SP = 0;
		uint16_t PC = 0;
	
		// Flags - For Convenience
		uint8_t flag_Z = 0;
		uint8_t flag_N = 0;
		uint8_t flag_H = 0;
		uint8_t flag_C = 0;
		
		// Temporary Vars
		int8_t i8 = 0;
		int16_t i16 = 0;
		uint8_t u8 = 0;
		uint16_t u16 = 0;
	
		// Status
		uint8_t Halt = 0;
		uint8_t Stopped = 0;
		uint8_t InterruptsEnabled = 0;
	
		// Functions - Stack
		void StackPush (uint16_t Value);
		uint16_t StackPop ();
	
		// Functions - Flags
		void SetZ (uint8_t Value);
		void SetN (uint8_t Value);
		void SetH (uint8_t Value);
		void SetC (uint8_t Value);
		uint8_t GetCarry (uint16_t OpA, uint16_t OpB, uint8_t Carry, uint8_t BitNo);
		void SetFlagsAdd (uint8_t OpA, uint8_t OpB, uint8_t Carry, uint8_t CarrySetMode);
		void SetFlagsSub (uint8_t OpA, uint8_t OpB, uint8_t Carry, uint8_t CarrySetMode);
};

#endif