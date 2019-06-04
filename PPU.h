#include <stdint.h>
#include <SDL2/SDL.h>
#include <stdio.h>
#include <cstring>
#include "utils.h"
#ifndef PPU_H
#define PPU_H

class PPU {
public:
	PPU (const char* Title, const uint16_t _PixelSize);
	void Update (uint8_t* Memory, uint8_t* IOMap);
private:
	uint16_t PixelSize;
	uint16_t Width = 160; // 160
	uint16_t Height = 144; // 144
	SDL_Window* MainWindow;
	SDL_Renderer* MainRenderer;
	SDL_Texture* MainTexture;
	uint32_t Pixels [160 * 144];
	uint8_t CurrentY = 0;
	
	// Drawing Functions
	void SetPixel (uint32_t CoordX, uint32_t CoordY, uint32_t Color);
};

#endif