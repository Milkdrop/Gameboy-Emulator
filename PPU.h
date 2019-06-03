#include <stdint.h>
#include <SDL2/SDL.h>
#include <stdio.h>
#ifndef PPU_H
#define PPU_H

class PPU {
public:
	PPU (const char* Title, const uint16_t _Width, const uint16_t _Height, const uint16_t _PixelSize);
	void Update (const uint8_t* VRAM, uint8_t* IOMap);
private:
	uint16_t PixelSize;
	uint16_t Width;
	uint16_t Height;
	SDL_Window* MainWindow;
	SDL_Renderer* MainRenderer;
	SDL_Texture* MainTexture;
	SDL_Rect* TextureRect;
	uint8_t CurrentY = 0;
};

#endif