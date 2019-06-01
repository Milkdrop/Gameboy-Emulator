#include <stdint.h>
#include <stdio.h>
#include <chrono>
#include <SDL2/SDL.h>
#include "Display.h"
#include "MMU.h"
#include "CPU.h"

void OpenFileError (const char* Filename);
void LoadROM (MMU* mmu, uint16_t Address, const char* Filename);
void CPULoop (CPU* cpu, Display* display);

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
	Display* display = new Display ("Gameboy", 160, 144, 2);
	LoadROM (mmu, 0x0000, argv[1]);
	
	// Loop
	CPULoop (cpu, display);
	
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

void CPULoop (CPU* cpu, Display* display) {
	// Main Loop Variables
	SDL_Event ev;
	uint64_t CurrentTime = 0;
	uint64_t LastInput = 0;
	uint8_t Quit = 0;
	
	// Main Loop
	while (!Quit) {
		CurrentTime = clock(); // In Microseconds (On *UNIX)
		
		// Input
		if (CurrentTime - LastInput >= 1000000 / 30 || LastInput > CurrentTime) { // 30 Hz
			LastInput = CurrentTime;
			while (SDL_PollEvent(&ev)) {
				if (ev.type == SDL_QUIT) {
					Quit = 1;
				}
			}
		}
		
		cpu->Clock ();
	}
}