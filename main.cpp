#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include <SDL2/SDL.h>
#include "PPU.h"
#include "MMU.h"
#include "CPU.h"
#include "utils.h"

const char* ROMTypes[] = {
	"ROM Only", "ROM+MBC1", "ROM+MBC1+RAM", "ROM+MBC1+RAM+BATT",
	"Unknown", "ROM+MBC2", "ROM+MBC2+BATT", "Unknown",
	"ROM+RAM", "ROM+RAM+BATTERY", "Unknown", "ROM+MMM01",
	"ROM+MMM01+SRAM", "ROM+MMM01+SRAM+BATT", "Unknown", "Unknown",
	"Unknown", "Unknown", "ROM+MBC3+RAM", "ROM+MBC3+RAM+BATT",
	"Unknown", "Unknown", "Unknown", "Unknown", 
	"Unknown", "ROM+MBC5", "ROM+MBC5+RAM", "ROM+MBC5+RAM+BATT",
	"ROM+MBC5+RUMBLE", "ROM+MBC5+RUMBLE+SRAM", "ROM+MBC5+RUMBLE+SRAM+BATT", "Pocket Camera"
};

uint8_t ROM [8 * 1024 * 1024]; // 8 MB Max ROM Size
uint8_t BootROM [256]; // Boot ROM Size

void OpenFileError (const char* Filename);
void LoadROM (const char* Filename, const uint8_t Type);
void AnalyzeROM (MMU* mmu);
void CPULoop (CPU* cpu, MMU* mmu, PPU* ppu, uint8_t Boot);
	
int main (int argc, char** argv) {
	if (argc < 3) {
		printf ("Please specify Boot ROM + Game ROM Filename:\n");
		printf ("\t- %s Boot.bin Game.gb\n", argv[0]);
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
	
	LoadROM (argv[1], 1);
	LoadROM (argv[2], 0);
	mmu->SetBytesAt (0x0000, ROM, 0x10000);
	mmu->SetBytesAt (0x0000, BootROM, 0x100);
	AnalyzeROM (mmu);
	
	// Loop
	CPULoop (cpu, mmu, ppu, 1); // Boot
	mmu->SetBytesAt (0x0000, ROM, 0x100); // Load 0x100 ROM Area, for INTs etc.
	
	CPULoop (cpu, mmu, ppu, 0); // Normal Loop
	
	// Cleanup
	SDL_Quit ();
	printf ("\n\n[INFO] CPU Stopped.\n");
}

void OpenFileError (const char* Filename) {
	printf ("\n[ERR] There was an error opening the file: %s\n", Filename);
	exit(1);
}

void LoadROM (const char* Filename, const uint8_t Type) {
	printf ("Opening ROM %s...", Filename);
	
	FILE* ROMfd = fopen (Filename, "rb");
	if (ROMfd == 0)
		OpenFileError (Filename);
	
	fseek (ROMfd, 0, SEEK_END);
	uint32_t ROMSize = ftell (ROMfd);
	ROMfd = freopen (Filename, "rb", ROMfd);
	
	uint8_t* Buffer = (uint8_t*) malloc (ROMSize);
	
	if (fread(Buffer, 1, ROMSize, ROMfd) != ROMSize)
		OpenFileError (Filename);
	
	switch (Type) {
		case 0: memcpy (ROM, Buffer, ROMSize); break; // Boot ROM
		case 1: memcpy (BootROM, Buffer, ROMSize); break; // Normal ROM
		default: break;
	}
	
	free (Buffer);
	fclose (ROMfd);
	printf ("OK\n");
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
	
	printf ("Rom Type: ");
	uint8_t ROMType = mmu->GetByteAt (0x147);
	if (ROMType == 0xFE)
		printf ("Hudson HuC-3\n");
	else if (ROMType == 0xFD)
		printf ("Bandai TAMA5\n");
	else if (ROMType < 0x20) {
		printf ("%s\n", ROMTypes [ROMType]);
	} else
		printf ("Unknown\n");
}

/* IO TODO
FF02
FF04
FF10 -> FF26 // Sound
*/

// Clock Speed: 4.194304 MHz
void CPULoop (CPU* cpu, MMU* mmu, PPU* ppu, uint8_t Boot) {
	// Main Loop Variables
	SDL_Event ev;
	const uint8_t *Keyboard = SDL_GetKeyboardState(NULL);
	uint8_t* IOMap = mmu->IOMap;
	uint64_t CurrentTime = 0;
	uint64_t LastDebug = 0;
	uint64_t LastInput = 0;
	uint64_t LastTimer = 0;
	uint64_t LastDiv = 0;
	uint8_t PressDebug = 0;
	uint8_t Quit = 0;
	
	// Timing
	uint32_t LastLineDraw = 0;
	uint32_t CurrentPPUMode = 1;
	uint32_t PixelTransferDuration = 0;
	uint64_t ClocksPerMS = 4194;
	uint64_t LastClock = 0;
	uint64_t LastLoop = 0;
	
	// Main Loop
	while (!Quit) {
		CurrentTime = clock(); // In Microseconds (On *UNIX)
		
		if (CurrentTime - LastLoop <= 1000) {
			if (cpu->ClockCount - LastClock >= ClocksPerMS) { // Throttle
				// TODO: nanosleep ?
				continue;
			}
		} else {
			LastLoop = CurrentTime;
			LastClock = cpu->ClockCount;
		}
		
		// Debug / Info
		if (CurrentTime - LastDebug >= 5000000 || LastDebug > CurrentTime) { // 30 Hz
			LastDebug = CurrentTime;
			//printf ("Speed: @%f MHz\n", (float) cpu->ClockCount / 5000000);
			cpu->ClockCount = 0;
		}
		
		// Input - SDL
		if (CurrentTime - LastInput >= 1000000 / 30 || LastInput > CurrentTime) { // 30 Hz
			LastInput = CurrentTime;
			
			while (SDL_PollEvent(&ev)) {
				if (ev.type == SDL_QUIT) {
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
			if (Keyboard [SDL_SCANCODE_A])
				SetBit (IOMap [0x00], 0, 0);
			
			if (Keyboard [SDL_SCANCODE_B])
				SetBit (IOMap [0x00], 1, 0);
			
			if (Keyboard [SDL_SCANCODE_SPACE]) // SELECT
				SetBit (IOMap [0x00], 2, 0);
			
			if (Keyboard [SDL_SCANCODE_RETURN]) // START
				SetBit (IOMap [0x00], 3, 0);
			
			if ((IOMap [0x00] & 0xF) != 0xF) // Something was pressed
				cpu->Interrupt (4);
		}
		
		// IOMap 0x40 - LCDC
		// IOMap 0x41 - LCD STAT
		
		// Rendering
		if (GetBit (IOMap [0x40], 7)) { // LCD Operation
			uint32_t CurrentLineClock = cpu->ClockCount - LastLineDraw;
				
			if (ppu->CurrentY < 144) {
				if (CurrentLineClock <= 80) { // OAM Period
					if (CurrentPPUMode == 0 || CurrentPPUMode == 1) { // Came from VBlank or HBlank
						CurrentPPUMode = 2;
						ppu->OAMSearch (mmu->Memory, IOMap);
						PixelTransferDuration = 168 + (ppu->SpriteCount * (291 - 168)) / 10; // 10 Sprites should cause maximum duration = 291 Clocks

						SetBit (IOMap [0x41], 0, 0); // Set them now so that the CPU can service the INT correctly
						SetBit (IOMap [0x41], 1, 1);
						
						if (GetBit (IOMap [0x41], 5))
							cpu->Interrupt (1);
					}
				} else if (CurrentLineClock <= 80 + PixelTransferDuration) { // Pixel Transfer
					if (CurrentPPUMode == 2) {
						CurrentPPUMode = 3;
						
						SetBit (IOMap [0x41], 0, 1);
						SetBit (IOMap [0x41], 1, 1);
					}
				} else { // HBlank
					if (CurrentPPUMode == 3) {
						CurrentPPUMode = 0;
						
						SetBit (IOMap [0x41], 0, 0);
						SetBit (IOMap [0x41], 1, 0);
						
						if (GetBit (IOMap [0x41], 3))
							cpu->Interrupt (1);
					}
				}
			} else {
				if (CurrentPPUMode == 0) { // VBlank
					CurrentPPUMode = 1;
					
					SetBit (IOMap [0x41], 0, 1);
					SetBit (IOMap [0x41], 1, 0);
					
					cpu->Interrupt (0);
					
					if (GetBit (IOMap [0x41], 4))
						cpu->Interrupt (1);
				}
			}
			
			if (CurrentLineClock >= (144 << 2)) { // Passed On a New Line
				LastLineDraw = cpu->ClockCount;
				ppu->Update (mmu->Memory, IOMap);
				if (ppu->CurrentY == IOMap [0x45]) { // Coincidence LY, LYC
					SetBit (IOMap [0x41], 2, 1);
					if (GetBit (IOMap [0x41], 6))
						cpu->Interrupt (1);
				} else
					SetBit (IOMap [0x41], 2, 0);
			}
		}
		
		// Timer
		if (GetBit (IOMap [0x07], 2)) { // TIMCONT
			uint32_t TimerDelay = 0;
			if (GetBit (IOMap [0x07], 0)) {
				if (GetBit (IOMap [0x07], 1)) {
					TimerDelay = 16384;
				} else
					TimerDelay = 262144;
			} else if (GetBit (IOMap [0x07], 1))
				TimerDelay = 65536;
			else
				TimerDelay = 4096;
			
			if (CurrentTime - LastTimer >= 1000000 / TimerDelay || LastTimer > CurrentTime) {
				LastTimer = CurrentTime;
				IOMap [0x05]++; // TIMECNT
				if (IOMap [0x05] == 0x00) {
					IOMap [0x05] = IOMap [0x06]; // TIMEMOD
					cpu->Interrupt (2);
				}
			}
		}
		
		// DIV Register
		if (CurrentTime - LastDiv >= 1000000 / 16384 || LastDiv > CurrentTime) {
			LastDiv = CurrentTime;
			IOMap [0x04]++;
		}
		
		if (Boot && cpu->PC == 0x100) // Done Booting
			return;
		
		if (!cpu->Debugging) {
			uint8_t OldDMA = IOMap [0x46];
			cpu->Clock ();
			if (IOMap [0x46] != OldDMA) // DMA Write
				cpu->ClockCount += 160;
		}
	} 
}