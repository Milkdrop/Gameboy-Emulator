#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include <SDL2/SDL.h>
#include "PPU.h"
#include "MMU.h"
#include "CPU.h"
#include "utils.h"

void OpenFileError (const char* Filename);
void LoadROM (MMU* mmu, const char* Filename);
void AnalyzeROM (MMU* mmu);
void CPULoop (CPU* cpu, MMU* mmu, PPU* ppu);

uint8_t ROMwBattery [] = {0x03, 0x06, 0x09, 0x0D, 0x0F, 0x10, 0x1B, 0x1E, 0x20, 0xFF};
uint8_t ROMwRAM [] = {0x02, 0x03, 0x06, 0x08, 0x09, 0x0C, 0x0D, 0x10, 0x12, 0x13, 0x1A, 0x1B, 0x1D, 0x1E, 0x20, 0x22, 0xFF};

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
	
	LoadROM (mmu, argv[1]);
	AnalyzeROM (mmu);
	
	// Loop
	CPULoop (cpu, mmu, ppu);
	
	// Cleanup
	SDL_Quit ();
	printf ("\n\n[INFO] CPU Stopped.\n");
}

void OpenFileError (const char* Filename) {
	printf ("\n[ERR] There was an error opening the file: %s\n", Filename);
	exit(1);
}

void LoadROM (MMU* mmu, const char* Filename) {
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
	
	memcpy (mmu->ROM, Buffer, ROMSize);
	mmu->SetBytesAt (0x0000, Buffer, 0x10000);
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
	
	uint8_t ROMType = 0; // Actual ROM Type
	uint8_t ROMBattery = 0;
	uint8_t ROMRAM = 0;
	uint8_t CartridgeROMType = mmu->GetByteAt (0x147);
	
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

/* IO TODO
FF02
FF04
FF10 -> FF26 // Sound
*/

// Clock Speed: 4.194304 MHz
void CPULoop (CPU* cpu, MMU* mmu, PPU* ppu) {
	// Main Loop Variables
	SDL_Event ev;
	const uint8_t *Keyboard = SDL_GetKeyboardState(NULL);
	uint8_t* IOMap = mmu->IOMap;
	uint64_t CurrentTime = 0;
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
			if (cpu->ClockCount - LastClock >= ClocksPerMS && !Keyboard [SDL_SCANCODE_SPACE]) { // Throttle - Space for max speed
				// TODO: nanosleep ?
				continue;
			}
		} else {
			LastLoop = CurrentTime;
			LastClock = cpu->ClockCount;
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
			
			if (Keyboard [SDL_SCANCODE_LSHIFT]) // SELECT
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
		
		if (!cpu->Debugging) {
			uint8_t OldDMA = IOMap [0x46];
			cpu->Clock ();
			if (IOMap [0x46] != OldDMA) // DMA Write
				cpu->ClockCount += 160;
		}
	} 
}