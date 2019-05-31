#include "Display.h"

const uint32_t Black		= 0xff000000;
const uint32_t DarkGray		= 0xffffffff;
const uint32_t LightGray	= 0xffffffff;
const uint32_t White		= 0xffffffff;

Display::Display (const char* Title, const uint16_t _Width, const uint16_t _Height, const uint16_t _PixelSize) {
	PixelSize = _PixelSize;
	Width = _Width;
	Height = _Height;
	
	MainWindow = SDL_CreateWindow (Title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, Width * PixelSize, Height * PixelSize, SDL_WINDOW_SHOWN);
	MainRenderer = SDL_CreateRenderer(MainWindow, -1, SDL_RENDERER_ACCELERATED);
	
	SDL_SetRenderDrawColor(MainRenderer, 0x00, 0x00, 0x00, 0x00);
	SDL_RenderClear(MainRenderer);
}

void Display::Update (const uint8_t* VRAM) {
	SDL_Surface* Surface = SDL_CreateRGBSurface (0, Width * PixelSize, Height * PixelSize, 32, 0, 0, 0, 0);
	uint32_t ColorToDraw = White;
	
	for (int y = 0; y < Height; y++) {
		for (int x = 0; x < Width; y ++) {
			if (VRAM [y * Width + x] == 1) {
				SDL_Rect pixel = {x * PixelSize, y * PixelSize, PixelSize, PixelSize};
				SDL_FillRect (Surface, &pixel, ColorToDraw);
			}
		}
	}
	
	SDL_Texture* Texture = SDL_CreateTextureFromSurface (MainRenderer, Surface);
	SDL_RenderCopy(MainRenderer, Texture, NULL, NULL);
	SDL_RenderPresent(MainRenderer);
	SDL_FreeSurface (Surface);
	SDL_DestroyTexture (Texture);
}