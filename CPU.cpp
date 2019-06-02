#include "CPU.h"

const uint8_t ClocksPerInstruction[] = {
//  0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F
	4,  12, 8,  8,  4,  4,  8,  4,  20, 8,  8,  8,  4,  4,  8,  4,  // 0
	4,  12, 8,  8,  4,  4,  8,  4,  12, 8,  8,  8,  4,  4,  8,  4,  // 1
	8,  12, 8,  8,  4,  4,  8,  4,  8,  8,  8,  8,  4,  4,  8,  4,  // 2
	8,  12, 8,  8,  12, 12, 12, 4,  8,  8,  8,  8,  4,  4,  8,  4,  // 3
	4,  4,  4,  4,  4,  4,  8,  4,  4,  4,  4,  4,  4,  4,  8,  4,  // 4
	4,  4,  4,  4,  4,  4,  8,  4,  4,  4,  4,  4,  4,  4,  8,  4,  // 5
	4,  4,  4,  4,  4,  4,  8,  4,  4,  4,  4,  4,  4,  4,  8,  4,  // 6
	8,  8,  8,  8,  8,  8,  4,  8,  4,  4,  4,  4,  4,  4,  8,  4,  // 7
	4,  4,  4,  4,  4,  4,  8,  4,  4,  4,  4,  4,  4,  4,  8,  4,  // 8
	4,  4,  4,  4,  4,  4,  8,  4,  4,  4,  4,  4,  4,  4,  8,  4,  // 9
	4,  4,  4,  4,  4,  4,  8,  4,  4,  4,  4,  4,  4,  4,  8,  4,  // A
	4,  4,  4,  4,  4,  4,  8,  4,  4,  4,  4,  4,  4,  4,  8,  4,  // B
	8,  12, 12, 16, 12, 16, 8,  16, 8,  16, 12, 4,  12, 24, 8,  16, // C
	8,  12, 12, 0,  12, 16, 8,  16, 8,  16, 12, 0,  12, 0,  8,  16, // D
	12, 12, 8,  0,  0,  16, 8,  16, 16, 4,  16, 0,  0,  0,  8,  16, // E
	12, 12, 8,  4,  0,  16, 8,  16, 12, 8,  16, 4,  0,  0,  8,  16  // F
};

CPU::CPU (MMU* _mmu) {
	mmu = _mmu;
}

uint16_t CPU::StackPop () {
	uint16_t Value = mmu->GetWordAt (SP);
	SP += 2;
	return Value;
}

void CPU::StackPush (uint16_t Value) {
	SP -= 2;
	mmu->SetWordAt (SP, Value);
}

void CPU::SetZ (uint8_t Value) {
	*reg_F &= 0b01111111;
	*reg_F |= Value << 7;
}

void CPU::SetN (uint8_t Value) {
	*reg_F &= 0b10111111;
	*reg_F |= Value << 6;
}

void CPU::SetH (uint8_t Value) {
	*reg_F &= 0b11011111;
	*reg_F |= Value << 5;
}

void CPU::SetC (uint8_t Value) {
	*reg_F &= 0b11101111;
	*reg_F |= Value << 4;
}

uint8_t CPU::GetCarry (uint16_t OpA, uint16_t OpB, uint8_t BitNo) {
	uint32_t Result = OpA + OpB;
	return ((Result ^ OpA ^ OpB) >> BitNo) & 1;
}

void CPU::SetFlagsAdd (uint8_t OpA, uint8_t OpB, uint8_t Carry, uint8_t CarrySetMode) {
	// CarrySetMode: 0 - HC, 1 - C, 2 - H, 3 - None
	uint8_t Result = OpA + OpB;
	
	SetZ (Result == 0);
	SetN (0);
	
	if (CarrySetMode == 0 || CarrySetMode == 2) // H
		SetH (GetCarry (OpA, OpB + Carry, 4));
	
	if (CarrySetMode == 0 || CarrySetMode == 1) // C
		SetC (GetCarry (OpA, OpB + Carry, 8));
}

void CPU::SetFlagsSub (uint8_t OpA, uint8_t OpB, uint8_t Carry, uint8_t CarrySetMode) {
	SetFlagsAdd (OpA, ~OpB, !Carry, CarrySetMode);
	SetN (1);
}

void CPU::Clock () {
	uint8_t Instruction = mmu->GetByteAt (PC++);
	
	Execute (Instruction);
}

void CPU::Execute (uint8_t Instruction) {
	ClockCount += ClocksPerInstruction [Instruction];
	
	printf ("Executing Instruction 0x%02x At 0x%04x\n", Instruction, PC - 1);
	
	flag_Z = (*reg_F >> 7) & 1;
	flag_N = (*reg_F >> 6) & 1;
	flag_H = (*reg_F >> 5) & 1;
	flag_C = (*reg_F >> 4) & 1;
	
	switch (Instruction) {
		// Misc / Control
		case 0x00: break; // NOP
		case 0x10: Stopped = 1; break; // STOP
		case 0x76: Halt = 1; break; // HALT
		case 0xF3: InterruptsEnabled = 0; break; // DI
		case 0xFB: InterruptsEnabled = 1; break; // EI
		case 0xCB: u8 = mmu->GetByteAt (PC++); // CB
			switch (u8) {
				case 0x11: SetN (0); SetH (0); u8 = *reg_C >> 7; *reg_C <<= 1; *reg_C |= flag_C; SetC (u8); break; // RL
				case 0x7C: SetZ (*reg_H >> 7); SetN (0); SetH (1); break; // BIT
				default: printf ("Doing CB: 0x%02x\n", u8);
			}
			break;
		
		// Jumps / Calls
		case 0xC3: PC = mmu->GetWordAt(PC); break; // JP a16
		case 0xE9: PC = reg_HL; break; // JP (HL)
		case 0xC2: u16 = mmu->GetWordAt(PC); PC += 2; if (!flag_Z) {PC = u16; ClockCount += 4;} break; // JP NZ a16
		case 0xD2: u16 = mmu->GetWordAt(PC); PC += 2; if (!flag_C) {PC = u16; ClockCount += 4;} break; // JP NC a16
		case 0xCA: u16 = mmu->GetWordAt(PC); PC += 2; if (flag_Z)  {PC = u16; ClockCount += 4;} break; // JP Z a16
		case 0xDA: u16 = mmu->GetWordAt(PC); PC += 2; if (flag_C)  {PC = u16; ClockCount += 4;} break; // JP C a16
			
		case 0x18: i8 = mmu->GetByteAt (PC++); PC += i8; break; // JR
		case 0x28: i8 = mmu->GetByteAt (PC++); if (flag_Z)  {PC += i8; ClockCount += 4;} break; // JR Z
		case 0x38: i8 = mmu->GetByteAt (PC++); if (flag_C)  {PC += i8; ClockCount += 4;} break; // JR C
		case 0x20: i8 = mmu->GetByteAt (PC++); if (!flag_Z) {PC += i8; ClockCount += 4;} break; // JR NZ
		case 0x30: i8 = mmu->GetByteAt (PC++); if (!flag_C) {PC += i8; ClockCount += 4;} break; // JR NC
			
		case 0xCD: u16 = mmu->GetWordAt(PC); PC += 2; StackPush (PC); PC = u16; break; // CALL
		case 0xC4: u16 = mmu->GetWordAt(PC); PC += 2; if (!flag_Z) {StackPush (PC); PC = u16; ClockCount += 12;} break; // CALL NZ, a16
		case 0xD4: u16 = mmu->GetWordAt(PC); PC += 2; if (!flag_C) {StackPush (PC); PC = u16; ClockCount += 12;} break; // CALL NC, a16
		case 0xCC: u16 = mmu->GetWordAt(PC); PC += 2; if (flag_Z)  {StackPush (PC); PC = u16; ClockCount += 12;} break; // CALL Z, a16
		case 0xDC: u16 = mmu->GetWordAt(PC); PC += 2; if (flag_C)  {StackPush (PC); PC = u16; ClockCount += 12;} break; // CALL C, a16
		
		case 0xC9: PC = StackPop(); break; // RET
		case 0xD9: PC = StackPop(); InterruptsEnabled = 1; break; // RETI
		case 0xC0: if (!flag_Z) {PC = StackPop(); ClockCount += 12;} break; // RET NZ
		case 0xD0: if (!flag_C) {PC = StackPop(); ClockCount += 12;} break; // RET NC
		case 0xC8: if (flag_Z)  {PC = StackPop(); ClockCount += 12;} break; // RET Z
		case 0xD8: if (flag_C)  {PC = StackPop(); ClockCount += 12;} break; // RET C
		
		case 0xC7: StackPush (PC); PC = 0x00; break; // RST
		case 0xCF: StackPush (PC); PC = 0x08; break; // RST
		case 0xD7: StackPush (PC); PC = 0x10; break; // RST
		case 0xDF: StackPush (PC); PC = 0x18; break; // RST
		case 0xE7: StackPush (PC); PC = 0x20; break; // RST
		case 0xEF: StackPush (PC); PC = 0x28; break; // RST
		case 0xF7: StackPush (PC); PC = 0x30; break; // RST
		case 0xFF: StackPush (PC); PC = 0x38; break; // RST
			
		// 8bit Loads / Moves
		case 0x02: mmu->SetByteAt (reg_BC, *reg_A); break; // LD n, A
		case 0x12: mmu->SetByteAt (reg_DE, *reg_A); break; // LD n, A
		case 0x22: mmu->SetByteAt (reg_HL++, *reg_A); break; // LD n, A
		case 0x32: mmu->SetByteAt (reg_HL--, *reg_A); break; // LD n, A
			
		case 0x0A: *reg_A = mmu->GetByteAt (reg_BC); break; // LD A, (BC)
		case 0x1A: *reg_A = mmu->GetByteAt (reg_DE); break; // LD A, (DE)
		case 0x2A: *reg_A = mmu->GetByteAt (reg_HL++); break; // LD, A (HL+)
		case 0x3A: *reg_A = mmu->GetByteAt (reg_HL--); break; // LD, A (HL+)
		
		case 0x06: *reg_B = mmu->GetByteAt (PC++); break; // LD nn, d8
		case 0x0E: *reg_C = mmu->GetByteAt (PC++); break; // LD nn, d8
		case 0x16: *reg_D = mmu->GetByteAt (PC++); break; // LD nn, d8
		case 0x1E: *reg_E = mmu->GetByteAt (PC++); break; // LD nn, d8
		case 0x26: *reg_H = mmu->GetByteAt (PC++); break; // LD nn, d8
		case 0x2E: *reg_L = mmu->GetByteAt (PC++); break; // LD nn, d8
		case 0x36: mmu->SetByteAt (reg_HL, mmu->GetByteAt (PC++)); break; // LD nn, d8
		case 0x3E: *reg_A = mmu->GetByteAt (PC++); break; // LD nn, d8
			
		case 0x40: *reg_B = *reg_B; break; // LD r1, r2
		case 0x41: *reg_B = *reg_C; break; // LD r1, r2
		case 0x42: *reg_B = *reg_D; break; // LD r1, r2
		case 0x43: *reg_B = *reg_E; break; // LD r1, r2
		case 0x44: *reg_B = *reg_H; break; // LD r1, r2
		case 0x45: *reg_B = *reg_L; break; // LD r1, r2
		case 0x46: *reg_B = mmu->GetByteAt (reg_HL); break; // LD r1, r2
		case 0x47: *reg_B = *reg_A; break; // LD r1, r2
			
		case 0x48: *reg_C = *reg_B; break; // LD r1, r2
		case 0x49: *reg_C = *reg_C; break; // LD r1, r2
		case 0x4A: *reg_C = *reg_D; break; // LD r1, r2
		case 0x4B: *reg_C = *reg_E; break; // LD r1, r2
		case 0x4C: *reg_C = *reg_H; break; // LD r1, r2
		case 0x4D: *reg_C = *reg_L; break; // LD r1, r2
		case 0x4E: *reg_C = mmu->GetByteAt (reg_HL); break; // LD r1, r2
		case 0x4F: *reg_C = *reg_A; break; // LD r1, r2

		case 0x50: *reg_D = *reg_B; break; // LD r1, r2
		case 0x51: *reg_D = *reg_C; break; // LD r1, r2
		case 0x52: *reg_D = *reg_D; break; // LD r1, r2
		case 0x53: *reg_D = *reg_E; break; // LD r1, r2
		case 0x54: *reg_D = *reg_H; break; // LD r1, r2
		case 0x55: *reg_D = *reg_L; break; // LD r1, r2
		case 0x56: *reg_D = mmu->GetByteAt (reg_HL); break; // LD r1, r2
		case 0x57: *reg_D = *reg_A; break; // LD r1, r2
		
		case 0x58: *reg_E = *reg_B; break; // LD r1, r2
		case 0x59: *reg_E = *reg_C; break; // LD r1, r2
		case 0x5A: *reg_E = *reg_D; break; // LD r1, r2
		case 0x5B: *reg_E = *reg_E; break; // LD r1, r2
		case 0x5C: *reg_E = *reg_H; break; // LD r1, r2
		case 0x5D: *reg_E = *reg_L; break; // LD r1, r2
		case 0x5E: *reg_E = mmu->GetByteAt (reg_HL); break; // LD r1, r2
		case 0x5F: *reg_E = *reg_A; break; // LD r1, r2
		
		case 0x60: *reg_H = *reg_B; break; // LD r1, r2
		case 0x61: *reg_H = *reg_C; break; // LD r1, r2
		case 0x62: *reg_H = *reg_D; break; // LD r1, r2
		case 0x63: *reg_H = *reg_E; break; // LD r1, r2
		case 0x64: *reg_H = *reg_H; break; // LD r1, r2
		case 0x65: *reg_H = *reg_L; break; // LD r1, r2
		case 0x66: *reg_H = mmu->GetByteAt (reg_HL); break; // LD r1, r2
		case 0x67: *reg_H = *reg_A; break; // LD r1, r2
		
		case 0x68: *reg_L = *reg_B; break; // LD r1, r2
		case 0x69: *reg_L = *reg_C; break; // LD r1, r2
		case 0x6A: *reg_L = *reg_D; break; // LD r1, r2
		case 0x6B: *reg_L = *reg_E; break; // LD r1, r2
		case 0x6C: *reg_L = *reg_H; break; // LD r1, r2
		case 0x6D: *reg_L = *reg_L; break; // LD r1, r2
		case 0x6E: *reg_L = mmu->GetByteAt (reg_HL); break; // LD r1, r2
		case 0x6F: *reg_L = *reg_A; break; // LD r1, r2
			
		case 0x70: mmu->SetByteAt (reg_HL, *reg_B); break; // LD r1, r2
		case 0x71: mmu->SetByteAt (reg_HL, *reg_C); break; // LD r1, r2
		case 0x72: mmu->SetByteAt (reg_HL, *reg_D); break; // LD r1, r2
		case 0x73: mmu->SetByteAt (reg_HL, *reg_E); break; // LD r1, r2
		case 0x74: mmu->SetByteAt (reg_HL, *reg_H); break; // LD r1, r2
		case 0x75: mmu->SetByteAt (reg_HL, *reg_L); break; // LD r1, r2
		case 0x77: mmu->SetByteAt (reg_HL, *reg_A); break; // LD r1, r2
			
		case 0x78: *reg_A = *reg_B; break; // LD r1, r2
		case 0x79: *reg_A = *reg_C; break; // LD r1, r2
		case 0x7A: *reg_A = *reg_D; break; // LD r1, r2
		case 0x7B: *reg_A = *reg_E; break; // LD r1, r2
		case 0x7C: *reg_A = *reg_H; break; // LD r1, r2
		case 0x7D: *reg_A = *reg_L; break; // LD r1, r2
		case 0x7E: *reg_A = mmu->GetByteAt(reg_HL); break; // LD r1, r2
		case 0x7F: *reg_A = *reg_A; break; // LD r1, r2
			
		case 0xE0: mmu->SetByteAt (0xFF00 + mmu->GetByteAt(PC++), *reg_A); break; // LDH (a8), A
		case 0xF0: *reg_A = mmu->GetByteAt (0xFF00 + mmu->GetByteAt(PC++)); break; // LDH A, (a8)
		
		case 0xE2: mmu->SetByteAt (0xFF00 + *reg_C, *reg_A); break; // LD (C), A
		case 0xF2: *reg_A = mmu->GetByteAt (0xFF00 + *reg_C); break; // LD A, (C)
			
		case 0xEA: mmu->SetByteAt (mmu->GetWordAt (PC), *reg_A); PC += 2; break; // LD n, A
		case 0xFA: *reg_A = mmu->GetByteAt (mmu->GetWordAt (PC)); PC += 2; break; // LD A, (nn)
		
		// 16bit Loads / Moves
		case 0x01: reg_BC = mmu->GetWordAt (PC); PC += 2; break; // LD nn, d16
		case 0x11: reg_DE = mmu->GetWordAt (PC); PC += 2; break; // LD nn, d16
		case 0x21: reg_HL = mmu->GetWordAt (PC); PC += 2; break; // LD nn, d16
		case 0x31: SP = mmu->GetWordAt (PC); PC += 2; break; // LD nn, d16
			
		case 0xC1: reg_BC = StackPop (); break; // POP nn
		case 0xD1: reg_DE = StackPop (); break; // POP nn
		case 0xE1: reg_HL = StackPop (); break; // POP nn
		case 0xF1: reg_AF = StackPop (); break; // POP nn
			
		case 0xC5: StackPush (reg_BC); break; // PUSH nn
		case 0xD5: StackPush (reg_DE); break; // PUSH nn
		case 0xE5: StackPush (reg_HL); break; // PUSH nn
		case 0xF5: StackPush (reg_AF); break; // PUSH nn
			
		case 0x08: mmu->SetByteAt (mmu->GetWordAt (PC), SP); PC += 2; break; // LD (a16), SP
			
		case 0xF8: i8 = mmu->GetByteAt (PC++); reg_HL = SP + i8; break; // LD HL, SP + r8
		case 0xF9: SP = reg_HL; break; // LD SP, HL
		
		// 8bit Arithmetic / Logical
		case 0x04: SetFlagsAdd (*reg_B, 1, 0, 2); (*reg_B)++; break; // INC r
		case 0x14: SetFlagsAdd (*reg_D, 1, 0, 2); (*reg_D)++; break; // INC r
		case 0x24: SetFlagsAdd (*reg_H, 1, 0, 2); (*reg_H)++; break; // INC r
		case 0x34: SetFlagsAdd (mmu->GetByteAt (reg_HL), 1, 0, 2); mmu->SetByteAt (reg_HL, mmu->GetByteAt (reg_HL) + 1); break; // INC r
		
		case 0x05: SetFlagsSub (*reg_B, 1, 0, 2); (*reg_B)--; break; // DEC r
		case 0x15: SetFlagsSub (*reg_D, 1, 0, 2); (*reg_D)--; break; // DEC r
		case 0x25: SetFlagsSub (*reg_H, 1, 0, 2); (*reg_H)--; break; // DEC r
		case 0x35: SetFlagsSub (mmu->GetByteAt (reg_HL), 1, 0, 2); mmu->SetByteAt (reg_HL, mmu->GetByteAt (reg_HL) - 1); break; // DEC r
			
		case 0x27: if (flag_N) { if (flag_C) *reg_A -= 0x60; if (flag_H) *reg_A -= 0x06; } else { if (flag_C || *reg_A > 0x99) {*reg_A += 0x60; SetC (1);} if (flag_H || (*reg_A & 0x0F) > 9) *reg_A += 0x06; } SetZ (*reg_A == 0); SetH (0); break; // DAA
		case 0x37: SetN (0); SetH (0); SetC (1); break; // SCF
		
		case 0x0C: SetFlagsAdd (*reg_C, 1, 0, 2); (*reg_C)++; break; // INC r
		case 0x1C: SetFlagsAdd (*reg_E, 1, 0, 2); (*reg_E)++; break; // INC r
		case 0x2C: SetFlagsAdd (*reg_L, 1, 0, 2); (*reg_L)++; break; // INC r
		case 0x3C: SetFlagsAdd (*reg_A, 1, 0, 2); (*reg_A)++; break; // INC r
			
		case 0x0D: SetFlagsSub (*reg_C, 1, 0, 2); (*reg_C)--; break; // DEC r
		case 0x1D: SetFlagsSub (*reg_E, 1, 0, 2); (*reg_E)--; break; // DEC r
		case 0x2D: SetFlagsSub (*reg_L, 1, 0, 2); (*reg_L)--; break; // DEC r
		case 0x3D: SetFlagsSub (*reg_A, 1, 0, 2); (*reg_A)--; break; // DEC r
			
		case 0x2F: *reg_A = ~*reg_A; SetN (1); SetH (1); break; // CPL
		case 0x3F: SetC (!flag_C); SetN (0); SetH (0); break; // CCF
		
		case 0x80: SetFlagsAdd (*reg_A, *reg_B, 0, 0); *reg_A += *reg_B; break; // ADD r1, r2
		case 0x81: SetFlagsAdd (*reg_A, *reg_C, 0, 0); *reg_A += *reg_C; break; // ADD r1, r2
		case 0x82: SetFlagsAdd (*reg_A, *reg_D, 0, 0); *reg_A += *reg_D; break; // ADD r1, r2
		case 0x83: SetFlagsAdd (*reg_A, *reg_E, 0, 0); *reg_A += *reg_E; break; // ADD r1, r2
		case 0x84: SetFlagsAdd (*reg_A, *reg_H, 0, 0); *reg_A += *reg_H; break; // ADD r1, r2
		case 0x85: SetFlagsAdd (*reg_A, *reg_L, 0, 0); *reg_A += *reg_L; break; // ADD r1, r2
		case 0x86: SetFlagsAdd (*reg_A, mmu->GetByteAt (reg_HL), 0, 0); *reg_A += mmu->GetByteAt (reg_HL); break; // ADD r1, r2
		case 0x87: SetFlagsAdd (*reg_A, *reg_A, 0, 0); *reg_A += *reg_A; break; // ADD r1, r2
			
		case 0x88: u8 = flag_C; SetFlagsAdd (*reg_A, *reg_B, flag_C, 0); *reg_A += *reg_B + u8; break; // ADC r1, r2
		case 0x89: u8 = flag_C; SetFlagsAdd (*reg_A, *reg_C, flag_C, 0); *reg_A += *reg_C + u8; break; // ADC r1, r2
		case 0x8A: u8 = flag_C; SetFlagsAdd (*reg_A, *reg_D, flag_C, 0); *reg_A += *reg_D + u8; break; // ADC r1, r2
		case 0x8B: u8 = flag_C; SetFlagsAdd (*reg_A, *reg_E, flag_C, 0); *reg_A += *reg_E + u8; break; // ADC r1, r2
		case 0x8C: u8 = flag_C; SetFlagsAdd (*reg_A, *reg_H, flag_C, 0); *reg_A += *reg_H + u8; break; // ADC r1, r2
		case 0x8D: u8 = flag_C; SetFlagsAdd (*reg_A, *reg_L, flag_C, 0); *reg_A += *reg_L + u8; break; // ADC r1, r2
		case 0x8E: u8 = flag_C; SetFlagsAdd (*reg_A, mmu->GetByteAt (reg_HL), flag_C, 0); *reg_A += mmu->GetByteAt (reg_HL) + u8; break; // ADC r1, r2
		case 0x8F: u8 = flag_C; SetFlagsAdd (*reg_A, *reg_A, flag_C, 0); *reg_A += *reg_A + u8; break; // ADC r1, r2
			
		case 0x90: SetFlagsSub (*reg_A, *reg_B, 0, 0); *reg_A -= *reg_B; break; // SUB r1, r2
		case 0x91: SetFlagsSub (*reg_A, *reg_C, 0, 0); *reg_A -= *reg_C; break; // SUB r1, r2
		case 0x92: SetFlagsSub (*reg_A, *reg_D, 0, 0); *reg_A -= *reg_D; break; // SUB r1, r2
		case 0x93: SetFlagsSub (*reg_A, *reg_E, 0, 0); *reg_A -= *reg_E; break; // SUB r1, r2
		case 0x94: SetFlagsSub (*reg_A, *reg_H, 0, 0); *reg_A -= *reg_H; break; // SUB r1, r2
		case 0x95: SetFlagsSub (*reg_A, *reg_L, 0, 0); *reg_A -= *reg_L; break; // SUB r1, r2
		case 0x96: SetFlagsSub (*reg_A, mmu->GetByteAt (reg_HL), 0, 0); *reg_A -= mmu->GetByteAt (reg_HL); break; // SUB r1, r2
		case 0x97: SetFlagsSub (*reg_A, *reg_A, 0, 0); *reg_A -= *reg_A; break; // SUB r1, r2
			
		case 0x98: u8 = flag_C; SetFlagsSub (*reg_A, *reg_B, flag_C, 0); *reg_A -= *reg_B + u8; break; // SBC r1, r2
		case 0x99: u8 = flag_C; SetFlagsSub (*reg_A, *reg_C, flag_C, 0); *reg_A -= *reg_C + u8; break; // SBC r1, r2
		case 0x9A: u8 = flag_C; SetFlagsSub (*reg_A, *reg_D, flag_C, 0); *reg_A -= *reg_D + u8; break; // SBC r1, r2
		case 0x9B: u8 = flag_C; SetFlagsSub (*reg_A, *reg_E, flag_C, 0); *reg_A -= *reg_E + u8; break; // SBC r1, r2
		case 0x9C: u8 = flag_C; SetFlagsSub (*reg_A, *reg_H, flag_C, 0); *reg_A -= *reg_H + u8; break; // SBC r1, r2
		case 0x9D: u8 = flag_C; SetFlagsSub (*reg_A, *reg_L, flag_C, 0); *reg_A -= *reg_L + u8; break; // SBC r1, r2
		case 0x9E: u8 = flag_C; SetFlagsSub (*reg_A, mmu->GetByteAt (reg_HL), flag_C, 0); *reg_A -= mmu->GetByteAt (reg_HL) + u8; break; // SBC r1, r2
		case 0x9F: u8 = flag_C; SetFlagsSub (*reg_A, *reg_A, flag_C, 0); *reg_A -= *reg_A + u8; break; // SBC r1, r2
		
		case 0xA0: *reg_A &= *reg_B; SetZ (*reg_A == 0); SetN (0); SetH (1); SetC (0); break; // AND r
		case 0xA1: *reg_A &= *reg_C; SetZ (*reg_A == 0); SetN (0); SetH (1); SetC (0); break; // AND r
		case 0xA2: *reg_A &= *reg_D; SetZ (*reg_A == 0); SetN (0); SetH (1); SetC (0); break; // AND r
		case 0xA3: *reg_A &= *reg_E; SetZ (*reg_A == 0); SetN (0); SetH (1); SetC (0); break; // AND r
		case 0xA4: *reg_A &= *reg_H; SetZ (*reg_A == 0); SetN (0); SetH (1); SetC (0); break; // AND r
		case 0xA5: *reg_A &= *reg_L; SetZ (*reg_A == 0); SetN (0); SetH (1); SetC (0); break; // AND r
		case 0xA6: *reg_A &= mmu->GetByteAt (reg_HL); SetZ (*reg_A == 0); SetN (0); SetH (1); SetC (0); break; // AND r
		case 0xA7: *reg_A &= *reg_A; SetZ (*reg_A == 0); SetN (0); SetH (1); SetC (0); break; // AND r
			
		case 0xA8: *reg_A ^= *reg_B; SetZ (*reg_A == 0); SetN (0); SetH (0); SetC (0); break; // XOR r
		case 0xA9: *reg_A ^= *reg_C; SetZ (*reg_A == 0); SetN (0); SetH (0); SetC (0); break; // XOR r
		case 0xAA: *reg_A ^= *reg_D; SetZ (*reg_A == 0); SetN (0); SetH (0); SetC (0); break; // XOR r
		case 0xAB: *reg_A ^= *reg_E; SetZ (*reg_A == 0); SetN (0); SetH (0); SetC (0); break; // XOR r
		case 0xAC: *reg_A ^= *reg_H; SetZ (*reg_A == 0); SetN (0); SetH (0); SetC (0); break; // XOR r
		case 0xAD: *reg_A ^= *reg_L; SetZ (*reg_A == 0); SetN (0); SetH (0); SetC (0); break; // XOR r
		case 0xAE: *reg_A ^= mmu->GetByteAt (reg_HL); SetZ (*reg_A == 0); SetN (0); SetH (0); SetC (0); break; // XOR r
		case 0xAF: *reg_A ^= *reg_A; SetZ (*reg_A == 0); SetN (0); SetH (0); SetC (0); break; // XOR r
			
		case 0xB0: *reg_A |= *reg_B; SetZ (*reg_A == 0); SetN (0); SetH (0); SetC (0); break; // OR r
		case 0xB1: *reg_A |= *reg_C; SetZ (*reg_A == 0); SetN (0); SetH (0); SetC (0); break; // OR r
		case 0xB2: *reg_A |= *reg_D; SetZ (*reg_A == 0); SetN (0); SetH (0); SetC (0); break; // OR r
		case 0xB3: *reg_A |= *reg_E; SetZ (*reg_A == 0); SetN (0); SetH (0); SetC (0); break; // OR r
		case 0xB4: *reg_A |= *reg_H; SetZ (*reg_A == 0); SetN (0); SetH (0); SetC (0); break; // OR r
		case 0xB5: *reg_A |= *reg_L; SetZ (*reg_A == 0); SetN (0); SetH (0); SetC (0); break; // OR r
		case 0xB6: *reg_A |= mmu->GetByteAt (reg_HL); SetZ (*reg_A == 0); SetN (0); SetH (0); SetC (0); break; // OR r
		case 0xB7: *reg_A |= *reg_A; SetZ (*reg_A == 0); SetN (0); SetH (0); SetC (0); break; // OR r
			
		case 0xB8: SetFlagsSub (*reg_A, *reg_B, 0, 0); break; // CP r
		case 0xB9: SetFlagsSub (*reg_A, *reg_C, 0, 0); break; // CP r
		case 0xBA: SetFlagsSub (*reg_A, *reg_D, 0, 0); break; // CP r
		case 0xBB: SetFlagsSub (*reg_A, *reg_E, 0, 0); break; // CP r
		case 0xBC: SetFlagsSub (*reg_A, *reg_H, 0, 0); break; // CP r
		case 0xBD: SetFlagsSub (*reg_A, *reg_L, 0, 0); break; // CP r
		case 0xBE: SetFlagsSub (*reg_A, mmu->GetByteAt (reg_HL), 0, 0); break; // CP r
		case 0xBF: SetFlagsSub (*reg_A, *reg_A, 0, 0); break; // CP r
		
		case 0xC6: u8 = mmu->GetByteAt (PC++); SetFlagsAdd (*reg_A, u8, 0, 0); *reg_A += u8; break; // ADD A, d8
		case 0xD6: u8 = mmu->GetByteAt (PC++); SetFlagsSub (*reg_A, u8, 0, 0); *reg_A -= u8; break; // SUB A, d8
		case 0xE6: u8 = mmu->GetByteAt (PC++); *reg_A &= u8; SetZ (*reg_A == 0); SetN (0); SetH (1); SetC (0); break; // AND A, d8
		case 0xF6: u8 = mmu->GetByteAt (PC++); *reg_A |= u8; SetZ (*reg_A == 0); SetN (0); SetH (0); SetC (0); break; // OR A, d8
			
		case 0xCE: i8 = flag_C; u8 = mmu->GetByteAt (PC++); SetFlagsAdd (*reg_A, u8, flag_C, 0); *reg_A += u8 + i8; break; // ADD A, d8
		case 0xDE: i8 = flag_C; u8 = mmu->GetByteAt (PC++); SetFlagsSub (*reg_A, u8, flag_C, 0); *reg_A -= u8 + i8; break; // SUB A, d8
		case 0xEE: u8 = mmu->GetByteAt (PC++); *reg_A ^= u8; SetZ (*reg_A == 0); SetN (0); SetH (0); SetC (0); break; // XOR A, d8
		case 0xFE: u8 = mmu->GetByteAt (PC++); SetFlagsSub (*reg_A, u8, 0, 0); break; // CP A, d8
			
		// 16bit Arithmetic / Logical
		case 0x03: reg_BC++; break; // INC rr
		case 0x13: reg_DE++; break; // INC rr
		case 0x23: reg_HL++; break; // INC rr
		case 0x33: SP++; break; // INC rr
			
		case 0x09: SetN (0); SetH (GetCarry (reg_HL, reg_BC, 12)); SetC (GetCarry (reg_HL, reg_BC, 16)); reg_HL += reg_BC; break; // ADD rr, rr
		case 0x19: SetN (0); SetH (GetCarry (reg_HL, reg_DE, 12)); SetC (GetCarry (reg_HL, reg_DE, 16)); reg_HL += reg_DE; break; // ADD rr, rr
		case 0x29: SetN (0); SetH (GetCarry (reg_HL, reg_HL, 12)); SetC (GetCarry (reg_HL, reg_HL, 16)); reg_HL += reg_HL; break; // ADD rr, rr
		case 0x39: SetN (0); SetH (GetCarry (reg_HL, SP, 12)); SetC (GetCarry (reg_HL, SP, 16)); reg_HL += SP; break; // ADD rr, rr
			
		case 0x0B: reg_BC--; break; // DEC rr
		case 0x1B: reg_DE--; break; // DEC rr
		case 0x2B: reg_HL--; break; // DEC rr
		case 0x3B: SP--; break; // DEC rr
		
		// 8bit Rotation / Shifts
		case 0x07: SetZ (0); SetN (0); SetH (0); u8 = *reg_A >> 7; *reg_A <<= 1; *reg_A |= u8; SetC (u8); break; // RLCA
		case 0x17: SetZ (0); SetN (0); SetH (0); u8 = *reg_A >> 7; *reg_A <<= 1; *reg_A |= flag_C; SetC (u8); break; // RLA
			
		case 0x0F: SetZ (0); SetN (0); SetH (0); u8 = *reg_A & 1; *reg_A >>= 1; *reg_A |= u8 << 7; SetC (u8); break; // RRCA
		case 0x1F: SetZ (0); SetN (0); SetH (0); u8 = *reg_A & 1; *reg_A >>= 1; *reg_A |= flag_C << 7; SetC (u8); break; // RRA
		
		default:
			printf ("[ERR] Unknown Opcode: 0x%02x\n", Instruction);
	}
}