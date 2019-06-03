#include "PPU.h"

const uint32_t Black		= 0xff000000;
const uint32_t DarkGray		= 0xff555555;
const uint32_t LightGray	= 0xffaaaaaa;
const uint32_t White		= 0xffffffff;

PPU::PPU (const char* Title, const uint16_t _Width, const uint16_t _Height, const uint16_t _PixelSize) {
	PixelSize = _PixelSize;
	Width = _Width;
	Height = _Height;
	
	MainWindow = SDL_CreateWindow (Title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, Width * PixelSize, Height * PixelSize, SDL_WINDOW_SHOWN);
	MainRenderer = SDL_CreateRenderer(MainWindow, -1, SDL_RENDERER_ACCELERATED);
	MainTexture = SDL_CreateTexture (MainRenderer, , SDL_TEXTUREACCESS_STREAMING, Width * PixelSize, Height * PixelSize);
	SDL_SetRenderDrawColor(MainRenderer, 0x00, 0x00, 0x00, 0x00);
	SDL_RenderClear(MainRenderer);
}

void PPU::Update (const uint8_t* VRAM, uint8_t* IOMap) {
	uint32_t ColorToDraw = White;
	
	for (int CurrentX = 0; CurrentX < Width; CurrentX++) {
		SDL_Rect pixel = {CurrentX * PixelSize, CurrentY * PixelSize, PixelSize, PixelSize};
		SDL_FillRect (Surface, &pixel, ColorToDraw);
	}
	
	CurrentY = (CurrentY + 1) % 154;
	IOMap [0x44] = CurrentY; // Update current line that's being scanned
	
	SDL_RenderCopy (MainRenderer, MainTexture, NULL, NULL);
	SDL_RenderPresent (MainRenderer);
}