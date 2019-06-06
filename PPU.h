#include <stdint.h>
#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include "utils.h"
#ifndef PPU_H
#define PPU_H

class PPU {
public:
	PPU (const char* Title, const uint16_t _PixelSize);
	~PPU ();
	void OAMSearch (uint8_t* Memory, uint8_t* IOMap);
	void Update (uint8_t* Memory, uint8_t* IOMap);
	void Render ();
	uint8_t SpriteCount = 0;
private:
	uint16_t PixelSize;
	uint8_t CurrentY = 0;
	uint16_t Width = 160; // 160
	uint16_t Height = 144; // 144
	SDL_Window* MainWindow;
	SDL_Renderer* MainRenderer;
	SDL_Texture* MainTexture;
	uint16_t Pixels [160 * 144];
	uint16_t PixelsReady [160 * 144]; // When rendering, use these
	uint8_t OAMQueue [10 * 4]; // 10 Sprites, 4 Bytes each
	
	// Drawing Functions
	void SetPixel (uint32_t CoordX, uint32_t CoordY, uint32_t Color);
};

#endif