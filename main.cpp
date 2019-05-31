#include <stdio.h>
#include <chrono>
#include <SDL2/SDL.h>
#include "Display.h"
#include "MMU.h"

void OpenFileError (const char* Filename) {
	printf ("\n[ERR] There was an error opening the file: %s\n", Filename);
	exit(1);
}

int main (int argc, char** argv) {
	if (argc < 2) {
		printf ("Please specify ROM Filename:\n");
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
	Display* display = new Display ("Gameboy", 160, 144, 2);
	
	// Main Loop Variables
	SDL_Event ev;
	uint64_t CurrentTime = 0;
	uint64_t LastInput = 0;
	uint8_t Quit = 0;
	
	// Main Loop
	while (!Quit) {
		uint64_t CurrentTime = clock(); // In Microseconds (On *UNIX)
		
		// Input + Overflow Protection
		if (CurrentTime - LastInput >= 1000000 / 30 || LastInput > CurrentTime) { // 30 Hz
			LastInput = CurrentTime;
			// printf ("CHK INPUT\n");
			while (SDL_PollEvent(&ev)) {
				if (ev.type == SDL_QUIT) {
					Quit = 1;
				}
			}
		}
		
		printf ("%lu\n", CurrentTime);
	}
	
	// Cleanup
	SDL_Quit ();
	printf ("\n\n[INFO] CPU Stopped.\n");
}

