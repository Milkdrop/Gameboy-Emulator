#include "PPU.h"

const uint32_t Black		= 0xff000000;
const uint32_t DarkGray		= 0xff555555;
const uint32_t LightGray	= 0xffaaaaaa;
const uint32_t White		= 0xffffffff;

PPU::PPU (const char* Title, const uint16_t _PixelSize) {
	PixelSize = _PixelSize;
	
	MainWindow = SDL_CreateWindow (Title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, Width * PixelSize, Height * PixelSize, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
	MainRenderer = SDL_CreateRenderer(MainWindow, -1, SDL_RENDERER_ACCELERATED);
	MainTexture = SDL_CreateTexture (MainRenderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, Width, Height);
	SDL_SetRenderDrawColor(MainRenderer, 0x00, 0x00, 0x00, 0x00);
	SDL_RenderClear(MainRenderer);
	
	SDL_RenderSetLogicalSize (MainRenderer, Width, Height);
}

inline void PPU::SetPixel (uint32_t CoordX, uint32_t CoordY, uint32_t Color) {
	uint32_t PixelNo = CoordY * Width + CoordX;
	
	Pixels [PixelNo] = Color;
}

void PPU::Update (const uint8_t* VRAM, uint8_t* IOMap) {
	uint32_t ColorToDraw = White;
	
	if (CurrentY % 4 == 1)
		ColorToDraw = LightGray;
	if (CurrentY % 4 == 2)
		ColorToDraw = DarkGray;
	if (CurrentY % 4 == 3)
		ColorToDraw = Black;
	
	if (CurrentY < Height) {
		for (int CurrentX = 0; CurrentX < Width; CurrentX++) {
			SetPixel (CurrentX, CurrentY, ColorToDraw);
		}
	}
	
	CurrentY = (CurrentY + 1) % 154;
	IOMap [0x44] = CurrentY; // Update current line that's being scanned
	
	if (CurrentY == 0) { // End of Frame
		SDL_UpdateTexture (MainTexture, NULL, Pixels, 4 * Width);
		SDL_RenderCopy (MainRenderer, MainTexture, NULL, NULL);
		SDL_RenderPresent (MainRenderer);
	}
}