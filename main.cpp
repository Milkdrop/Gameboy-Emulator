#include <stdint.h>
#include <stdio.h>
#include <chrono>
#include <SDL2/SDL.h>
#include "PPU.h"
#include "MMU.h"
#include "CPU.h"

void OpenFileError (const char* Filename);
void LoadROM (MMU* mmu, uint16_t Address, const char* Filename);
uint8_t GetBit (uint8_t Value, uint8_t BitNo);
void SetBit (uint8_t &Value, uint8_t BitNo, uint8_t Set);
void CPULoop (CPU* cpu, MMU* mmu, PPU* ppu);
	
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
	//SDL_RenderSetScale
	LoadROM (mmu, 0x0000, argv[1]);
	
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

void LoadROM (MMU* mmu, uint16_t Address, const char* Filename) {
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
	
	mmu->SetBytesAt (Address, Buffer, ROMSize);
	free (Buffer);
	fclose (ROMfd);
	printf ("OK\n");
}

uint8_t GetBit (uint8_t Value, uint8_t BitNo) {
	return (Value & (1 << BitNo)) != 0;
}

void SetBit (uint8_t &Value, uint8_t BitNo, uint8_t Set) {
	if (Set) {
		Value |= 1 << BitNo;
	} else {
		Value &= 0xFF ^ (1 << BitNo);
	}
}

/* IO TODO
FF02
FF04
FF10 -> FF26 // Sound
*/

void CPULoop (CPU* cpu, MMU* mmu, PPU* ppu) {
	// Main Loop Variables
	SDL_Event ev;
	const uint8_t *Keyboard = SDL_GetKeyboardState(NULL);
	uint8_t* IOMap = mmu->IOMap;
	uint64_t CurrentTime = 0;
	uint64_t LastInput = 0;
	uint64_t LastDraw = 0;
	uint64_t LastTimer = 0;
	uint64_t LastDebug = 0;
	uint8_t PressDebug = 0;
	uint8_t Quit = 0;
	
	// Main Loop
	while (!Quit) {
		CurrentTime = clock(); // In Microseconds (On *UNIX)
		
		// Debug / Info
		if (CurrentTime - LastDebug >= 5000000 || LastDebug > CurrentTime) { // 30 Hz
			LastDebug = CurrentTime;
			//printf ("Speed: @%f MHz\n", (float) cpu->ClockCount / 5000000);
			cpu->ClockCount = 0;
		}
		
		// Input
		if (CurrentTime - LastInput >= 1000000 / 30 || LastInput > CurrentTime) { // 30 Hz
			LastInput = CurrentTime;
			
			while (SDL_PollEvent(&ev)) {
				if (ev.type == SDL_QUIT) {
					Quit = 1;
				}
			}
			
			uint8_t JoypadPort = 0;
			
			if (Keyboard [SDL_SCANCODE_RIGHT]) {
				SetBit (JoypadPort, 5, 1);
				SetBit (JoypadPort, 0, 1);
			}
			
			if (Keyboard [SDL_SCANCODE_LEFT]) {
				SetBit (JoypadPort, 5, 1);
				SetBit (JoypadPort, 1, 1);
			}
			
			if (Keyboard [SDL_SCANCODE_UP]) {
				SetBit (JoypadPort, 5, 1);
				SetBit (JoypadPort, 2, 1);
			}
			
			if (Keyboard [SDL_SCANCODE_DOWN]) {
				SetBit (JoypadPort, 5, 1);
				SetBit (JoypadPort, 3, 1);
			}
			
			if (Keyboard [SDL_SCANCODE_A]) {
				SetBit (JoypadPort, 4, 1);
				SetBit (JoypadPort, 0, 1);
			}
			
			if (Keyboard [SDL_SCANCODE_B]) {
				SetBit (JoypadPort, 4, 1);
				SetBit (JoypadPort, 1, 1);
			}
			
			if (Keyboard [SDL_SCANCODE_SPACE]) { // SELECT
				SetBit (JoypadPort, 4, 1);
				SetBit (JoypadPort, 2, 1);
			}
			
			if (Keyboard [SDL_SCANCODE_RETURN]) { // START
				SetBit (JoypadPort, 4, 1);
				SetBit (JoypadPort, 3, 1);
			}
			
			IOMap [0x00] = JoypadPort;
			
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
		
		// Draw
		if (CurrentTime - LastDraw >= 1000000 / (60 * 154) || LastDraw > CurrentTime) { // 60 Hz / 154 -> One Line
			LastDraw = CurrentTime;
			uint8_t LCDControl = IOMap [0x40];
			
			if (GetBit (LCDControl, 7)) { // LCD Operation
				// TODO Drawing
				ppu->Update (mmu->VRAM, IOMap);
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
		
		if (!cpu->Debugging)
			cpu->Clock ();
	}
}