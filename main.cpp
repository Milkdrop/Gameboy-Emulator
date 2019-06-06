#include <stdint.h>
#include <stdio.h>
#include <chrono>
#include <SDL2/SDL.h>
#include "PPU.h"
#include "MMU.h"
#include "CPU.h"
#include "utils.h"

using namespace Utils;

void OpenFileError (const char* Filename);
void LoadROM (MMU* mmu);
void AnalyzeROM (MMU* mmu);
void CPULoop (CPU* cpu, MMU* mmu, PPU* ppu);

void SaveGame (MMU* mmu);
void SaveState (uint8_t ID);
void LoadState (uint8_t ID);
void Reset (MMU* &mmu, CPU* &cpu, PPU* &ppu);

uint8_t ROMwBattery [] = {0x03, 0x06, 0x09, 0x0D, 0x0F, 0x10, 0x1B, 0x1E, 0x20, 0xFF};
uint8_t ROMwRAM [] = {0x02, 0x03, 0x06, 0x08, 0x09, 0x0C, 0x0D, 0x10, 0x12, 0x13, 0x1A, 0x1B, 0x1D, 0x1E, 0x20, 0x22, 0xFF};

char* ROMFilename;
char* SaveFilename;

// Initializations
int main (int argc, char** argv) {
	if (argc < 2) {
		printf ("Please specify Game ROM Filename:\n");
		printf ("\t- %s Game.gb\n", argv[0]);
		return 1;
	}
	
	// Init SDL
	printf ("[INFO] Initializing SDL...");
	if (SDL_Init (SDL_INIT_EVERYTHING) < 0) {
		printf ("\n[ERR] SDL failed to initialize: %s", SDL_GetError());
		return 1;
	}
	printf ("OK\n");
	
	// Init Hardware
	MMU* mmu = new MMU;
	CPU* cpu = new CPU (mmu);
	PPU* ppu = new PPU ("Gameboy", 2);
	
	ROMFilename = argv[1]; // Keep it for other functions to use
	LoadROM (mmu);
	
	// Loop
	CPULoop (cpu, mmu, ppu);
	
	// Cleanup
	SDL_Quit ();
	printf ("\n\n[INFO] CPU Stopped.\n");
}

// ROM Management
void OpenFileError (const char* Filename) {
	printf ("[ERR] There was an error opening the file: %s\n", Filename);
	exit(1);
}

void LoadROM (MMU* mmu) {
	printf ("[INFO] Opening ROM %s\n", ROMFilename);
	
	FILE* ROMfd = fopen (ROMFilename, "rb");
	if (ROMfd == 0)
		OpenFileError (ROMFilename);
	
	fseek (ROMfd, 0, SEEK_END);
	uint32_t ROMSize = ftell (ROMfd);
	ROMfd = freopen (ROMFilename, "rb", ROMfd);
	
	if (fread(mmu->ROM, 1, ROMSize, ROMfd) != ROMSize)
		OpenFileError (ROMFilename);
	
	fclose (ROMfd);
	
	AnalyzeROM (mmu);
	
	SaveFilename = (char*) malloc (strlen (ROMFilename) + 4); // Set savefilename for further use
	strcpy (SaveFilename, ROMFilename);
	strcat (SaveFilename, ".sav");
	
	FILE* Savefile = fopen (SaveFilename, "r");
	if (Savefile != NULL) {
		printf ("[INFO] Found savefile for game at %s\n", SaveFilename);
		
		uint32_t SaveSize = 0x2000 * mmu->ExternalRAMSize;
		if (fread (mmu->ExternalRAM, 1, SaveSize, Savefile) != SaveSize) // Load External RAM
			OpenFileError (SaveFilename);
		
		fclose (Savefile);
	}
}

void AnalyzeROM (MMU* mmu) {
	printf ("ROM INFO\n");
	
	printf ("Name: ");
	for (int i = 0x134; i <= 0x142; i++) {
		printf ("%c", mmu->GetByteAt (i));
	}
	printf ("\n");
	
	if (mmu->GetByteAt (0x143) == 0x00)
		printf ("GameBoy ROM\n");
	else if (mmu->GetByteAt (0x143) == 0x80)
		printf ("Color GameBoy ROM\n");
	else
		printf ("Unknown ROM\n");
	
	uint8_t ROMType = 0; // Actual ROM Type
	uint8_t ROMBattery = 0;
	uint8_t ROMRAM = 0;
	uint8_t CartridgeROMType = mmu->GetByteAt (0x0147);
	
	if (CartridgeROMType == 0x00)
		ROMType = 0; // ROM Only
	else if (CartridgeROMType < 0x04)
		ROMType = 1; // MBC1
	else if (CartridgeROMType < 0x07)
		ROMType = 2; // MBC2
	else if (CartridgeROMType < 0x0A)
		ROMType = 0; // ROM + RAM
	else if (CartridgeROMType < 0x0E)
		ROMType = 4; // MMM01, there's no MBC4
	else if (CartridgeROMType < 0x18)
		ROMType = 3; // MBC3
	else if (CartridgeROMType < 0x1F)
		ROMType = 5; // MBC5
	else if (CartridgeROMType < 0x21)
		ROMType = 6; // MBC6
	else if (CartridgeROMType < 0x23)
		ROMType = 7; // MBC7
	
	uint8_t ExternalRAMSize = mmu->GetByteAt (0x0149);
	
	switch (ExternalRAMSize) {
		case 0: mmu->ExternalRAMSize = 0; break;
		case 1: mmu->ExternalRAMSize = 1; break;
		case 2: mmu->ExternalRAMSize = 1; break;
		case 3: mmu->ExternalRAMSize = 4; break;
		case 4: mmu->ExternalRAMSize = 16; break;
		case 5: mmu->ExternalRAMSize = 8; break;
		default: break;
	}
	
	uint8_t ROMwBatteryCount = sizeof (ROMwBattery);
	uint8_t ROMwRAMCount = sizeof (ROMwRAM);
	
	for (int i = 0; i < ROMwBatteryCount; i++)
		if (CartridgeROMType == ROMwBattery [i])
			ROMBattery = 1;
	
	for (int i = 0; i < ROMwRAMCount; i++)
		if (CartridgeROMType == ROMwRAM [i])
			ROMRAM = 1;
	
	printf ("ROM Type: ");
	switch (ROMType) {
		case 0: printf ("ROM Only\n"); break;
		case 1: printf ("MBC1\n"); break;
		case 2: printf ("MBC2\n"); break;
		case 3: printf ("MBC3\n"); break;
		case 4: printf ("MMM01\n"); break;
		case 5: printf ("MBC5\n"); break;
		case 6: printf ("MBC6\n"); break;
		case 7: printf ("MBC7\n"); break;
		default: break;
	}
	
	printf ("ROM Battery: ");
	if (ROMBattery)
		printf ("YES\n");
	else
		printf ("NO\n");
	
	printf ("ROM w/ RAM: ");
	if (ROMRAM)
		printf ("YES\n");
	else
		printf ("NO\n");
	
	mmu->ROMType = ROMType;
	mmu->ROMBattery = ROMBattery;
	mmu->ROMRAM = ROMRAM;
}

// State modifications
void SaveGame (MMU* mmu) {
	if (mmu->ExternalRAMSize != 0) {
		printf ("[INFO] Saving External RAM (Savegame) to %s\n", SaveFilename);

		FILE* Savefile = fopen (SaveFilename, "wb");
		fwrite (mmu->ExternalRAM, 1, 0x2000 * mmu->ExternalRAMSize, Savefile);
		fclose (Savefile);
	}
}

void SaveState (uint8_t ID) {
	char* StateName = (char*) malloc (strlen (ROMFilename) + 9);
	strcpy (StateName, ROMFilename);
	strcat (StateName, ".state");
	snprintf (StateName + strlen(StateName), 4, "%d", ID);
	
	printf ("[INFO] Saving State %d to %s\n", ID, StateName);
	// TODO
}

void Reset (MMU* &mmu, CPU* &cpu, PPU* &ppu) {
	delete mmu;
	delete cpu;
	delete ppu;

	mmu = new MMU;
	cpu = new CPU (mmu);
	ppu = new PPU ("Gameboy", 2);
	LoadROM (mmu);
}

/* IO TODO
FF02
FF04
FF10 -> FF26 // Sound
*/

// Clock Speed: 4.194304 MHz
void CPULoop (CPU* cpu, MMU* mmu, PPU* ppu) {
	// Main Loop Variables
	SDL_Event ev;
	const uint8_t *Keyboard = SDL_GetKeyboardState (NULL);
	uint8_t* IOMap = mmu->IOMap;
	
	// Time Events - Clock independent
	auto StartTime = std::chrono::high_resolution_clock::now ();
	uint64_t ClockCompensation = 0;
	uint64_t LastInputTime = 0;
	uint64_t LastRenderTime = 0;
	uint64_t LastLoopTime = 0;
	uint64_t LastDebugTime = 0; // To show info
	
	// Input status
	uint8_t PressDebug = 0;
	uint8_t PressControlR = 0;
	uint8_t Quit = 0;
	
	// Timing
	uint32_t LastLineDrawClock = 0;
	uint32_t PixelTransferDuration = 0;
	uint32_t ClocksPerSec = 4194304;
	uint32_t ClocksPerMS = ClocksPerSec / 1000;
	uint32_t LastMSClock = 0;
	uint32_t LastTimerClock = 0;
	uint32_t LastDivClock = 0;
	uint32_t LastDebugClock = 0;
	uint32_t LastDebugInstructionCount = 0;

	// Main Loop
	while (!Quit) {
		uint64_t CurrentTime = GetCurrentTime (&StartTime);
		
		// Throttle
		if (CurrentTime - LastLoopTime <= 4000) { // Check if Host CPU is faster, every 4ms
			uint8_t Throttle = 0;
			
			if (cpu->ClockCount - LastMSClock >= (ClocksPerMS + ClockCompensation) << 2 && !Keyboard [SDL_SCANCODE_SPACE]) // Press space to disable throttling
				Throttle = 1;
			else if (cpu->ClockCount - LastMSClock >= ClocksPerMS + ClockCompensation && Keyboard [SDL_SCANCODE_BACKSPACE]) // x4 slow motion
				Throttle = 1;
			
			if (Throttle) {
				uint32_t usToSleep = 4000 - (CurrentTime - LastLoopTime);
				MicroSleep (usToSleep);
				LastLoopTime = GetCurrentTime (&StartTime);
				LastMSClock = cpu->ClockCount;
				
				uint32_t MicroSleepOvershoot = (LastLoopTime - CurrentTime) - usToSleep; // How much time it actually slept - time it had to sleep
				ClockCompensation = ((ClocksPerSec / 1000000) * MicroSleepOvershoot) >> 2; // Dont use float, since time calculations are already inexact. Having a precise calculation here would result in a faster-than-GB CPU,
																						   // also divide by 4, to accomodate extra inaccuracies
				if (MicroSleepOvershoot > 12000)
					printf ("[WARN] MicroSleep Overshoot is over 12ms (%d us), CPU can't catch up!\n", MicroSleepOvershoot);
			}
		} else { // Passed one milisecond without throttling
			LastLoopTime = CurrentTime;
			LastMSClock = cpu->ClockCount;
		}
		
		// Show debug info
		if (CurrentTime - LastDebugTime >= 5000000) { // Every 5 Seconds
			LastDebugTime = CurrentTime;
			uint32_t ClocksPassed = cpu->ClockCount - LastDebugClock;
			uint32_t InstructionsPassed = cpu->InstructionCount - LastDebugInstructionCount;
			
			printf ("[INFO] CPU Running at @%fMHz (%d Instructions/s)\n", (float) ClocksPassed / 5000000, InstructionsPassed / 5);
			
			LastDebugClock = cpu->ClockCount;
			LastDebugInstructionCount = cpu->InstructionCount;
		}
			
		// Input - SDL
		if (CurrentTime - LastInputTime >= 1000000 / 30) { // 30 Hz
			LastInputTime = CurrentTime;
			
			while (SDL_PollEvent(&ev)) {
				if (ev.type == SDL_QUIT) {
					SaveGame (mmu); // Save game on poweroff
					Quit = 1;
				}
			}
			
			if (cpu->Debugging) {
				if (Keyboard [SDL_SCANCODE_F3] || Keyboard [SDL_SCANCODE_F4]) {
					if (PressDebug == 0) {
						PressDebug = 1;
						//cpu->Debugging = 0;
						cpu->Clock ();
						if (Keyboard [SDL_SCANCODE_F3])
							cpu->Debug ();
					}
				} else
					PressDebug = 0;
			}
			
			if ((Keyboard [SDL_SCANCODE_LCTRL] || Keyboard [SDL_SCANCODE_RCTRL])) { // Save external RAM
				if (Keyboard [SDL_SCANCODE_R]) { // Reset
					if (PressControlR == 0) {
						PressControlR = 1;
						
						printf ("[INFO] State Reset\n");
						Reset (mmu, cpu, ppu);
						IOMap = mmu->IOMap;
						
						StartTime = std::chrono::high_resolution_clock::now ();
						LastInputTime = 0;
						LastRenderTime = 0;
						LastLoopTime = 0;
						LastDebugTime = 0;
						
						LastLineDrawClock = 0;
						mmu->CurrentPPUMode = 1;
						
						LastMSClock = 0;
						LastTimerClock = 0;
						LastDivClock = 0;
						
						LastDebugClock = 0;
						LastDebugInstructionCount = 0;
					}
				} else
					PressControlR = 0;
			}
		}
		
		// IOMap 0x40 - LCDC
		// IOMap 0x41 - LCD STAT
		
		// Rendering
		if (GetBit (IOMap [0x40], 7)) { // LCD Operation
			uint32_t CurrentLineClock = cpu->ClockCount - LastLineDrawClock;
				
			if (IOMap [0x44] < 144) { // Current line being drawn
				if (CurrentLineClock <= 80) { // OAM Period
					if (mmu->CurrentPPUMode == 0 || mmu->CurrentPPUMode == 1) { // Came from HBlank or VBlank
						mmu->CurrentPPUMode = 2;
						ppu->OAMSearch (mmu->Memory, IOMap);
						PixelTransferDuration = 168 + (ppu->SpriteCount * (291 - 168)) / 10; // 10 Sprites should cause maximum duration = 291 Clocks

						SetBit (IOMap [0x41], 0, 0); // Set them now so that the CPU can service the INT correctly
						SetBit (IOMap [0x41], 1, 1);
						
						if (GetBit (IOMap [0x41], 5))
							cpu->Interrupt (1);
					}
				} else if (CurrentLineClock <= 80 + PixelTransferDuration) { // Pixel Transfer
					if (mmu->CurrentPPUMode == 2) {
						mmu->CurrentPPUMode = 3;
						
						SetBit (IOMap [0x41], 0, 1);
						SetBit (IOMap [0x41], 1, 1);
					}
				} else { // HBlank
					if (mmu->CurrentPPUMode == 3) {
						mmu->CurrentPPUMode = 0;
						
						SetBit (IOMap [0x41], 0, 0);
						SetBit (IOMap [0x41], 1, 0);
						
						if (GetBit (IOMap [0x41], 3))
							cpu->Interrupt (1);
					}
				}
			} else {
				if (mmu->CurrentPPUMode == 0) { // VBlank
					mmu->CurrentPPUMode = 1;
					
					SetBit (IOMap [0x41], 0, 1);
					SetBit (IOMap [0x41], 1, 0);
					
					cpu->Interrupt (0);
					
					if (GetBit (IOMap [0x41], 4))
						cpu->Interrupt (1);
				}
			}
			
			if (CurrentLineClock >= (114 << 2)) { // Passed On a New Line
				LastLineDrawClock = cpu->ClockCount;
				ppu->Update (mmu->Memory, IOMap);
				
				if (IOMap [0x44] == IOMap [0x45]) { // Coincidence LY, LYC
					SetBit (IOMap [0x41], 2, 1);
					if (GetBit (IOMap [0x41], 6))
						cpu->Interrupt (1);
				} else
					SetBit (IOMap [0x41], 2, 0);
			}
		}
		
		if (CurrentTime - LastRenderTime >= 1000000 / 50) { // 50 Hz
			LastRenderTime = CurrentTime;
			ppu->Render (); // Actual rendering on the screen
		}
		
		// Timer
		// TODO Timer Obscure Behaviour
		if (GetBit (IOMap [0x07], 2)) { // TIMCONT
			uint32_t TimerDelay = 0;
			if (GetBit (IOMap [0x07], 0)) {
				if (GetBit (IOMap [0x07], 1))
					TimerDelay = 256;
				else
					TimerDelay = 16;
			} else if (GetBit (IOMap [0x07], 1))
				TimerDelay = 64;
			else
				TimerDelay = 1024;
			
			if (cpu->ClockCount - LastTimerClock >= TimerDelay) {
				LastTimerClock = cpu->ClockCount;
				IOMap [0x05]++; // TIMECNT
				
				if (IOMap [0x05] == 0) {
					IOMap [0x05] = IOMap [0x06]; // TIMEMOD
					cpu->Interrupt (2);
				}
			}
		}
		
		// Input - GB
		if (GetBit (IOMap [0x00], 4) == 0) { // Direction Pad
			IOMap [0x00] |= 0xF; // 1 - Not Pressed
			
			if (Keyboard [SDL_SCANCODE_RIGHT])
				SetBit (IOMap [0x00], 0, 0);

			if (Keyboard [SDL_SCANCODE_LEFT])
				SetBit (IOMap [0x00], 1, 0);

			if (Keyboard [SDL_SCANCODE_UP])
				SetBit (IOMap [0x00], 2, 0);

			if (Keyboard [SDL_SCANCODE_DOWN])
				SetBit (IOMap [0x00], 3, 0);
			
			if ((IOMap [0x00] & 0xF) != 0xF) // Something was pressed
				cpu->Interrupt (4);
		}
			
		if (GetBit (IOMap [0x00], 5) == 0) { // Buttons
			IOMap [0x00] |= 0b00001111; // 1 - Not Pressed
			if (Keyboard [SDL_SCANCODE_Z]) // A
				SetBit (IOMap [0x00], 0, 0);
			
			if (Keyboard [SDL_SCANCODE_X]) // B
				SetBit (IOMap [0x00], 1, 0);
			
			if (Keyboard [SDL_SCANCODE_LSHIFT]) // SELECT
				SetBit (IOMap [0x00], 2, 0);
			
			if (Keyboard [SDL_SCANCODE_RETURN]) // START
				SetBit (IOMap [0x00], 3, 0);
			
			if ((IOMap [0x00] & 0xF) != 0xF) // Something was pressed
				cpu->Interrupt (4);
		}
		
		// DIV Register
		if (cpu->ClockCount - LastDivClock >= 256) { // DIV increases every 256 clocks
			LastDivClock = cpu->ClockCount;
			IOMap [0x04]++;
		}
		
		if (!cpu->Debugging) {
			uint8_t OldDMA = IOMap [0x46];
			cpu->Clock ();
			if (IOMap [0x46] != OldDMA) // DMA Write
				cpu->ClockCount += (160 << 2) + 4;
		}
	} 
}