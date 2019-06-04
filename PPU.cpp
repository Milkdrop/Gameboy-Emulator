#include "PPU.h"

const uint32_t Colors [4] = {0xffffffff, 0xffaaaaaa, 0xff555555, 0xff000000};

uint32_t BGPalette [4];

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

void PPU::Update (uint8_t* Memory, uint8_t* IOMap) {
	uint16_t BGTable = 0x9800;
	uint16_t BGTileTable = 0x9000;
	uint8_t BGAddressingMode = 1; // Signed
	
	if (GetBit (IOMap [0x40], 3))
		BGTable = 0x9C00;
	
	if (GetBit (IOMap [0x40], 4)) {
		BGTileTable = 0x8000;
		BGAddressingMode = 0; // Unsigned
	}
	
	/* LCDC TODO:
	bits 1, 2, 5, 6
	*/
	
	// Set Scrolling
	uint8_t ScrollY = IOMap[0x42];
	uint8_t ScrollX = IOMap[0x43];
	
	// Set BG Palette
	BGPalette [0] = GetBit (IOMap [0x47], 0) | (GetBit (IOMap [0x47], 1) << 1);
	BGPalette [1] = GetBit (IOMap [0x47], 2) | (GetBit (IOMap [0x47], 3) << 1);
	BGPalette [2] = GetBit (IOMap [0x47], 4) | (GetBit (IOMap [0x47], 5) << 1);
	BGPalette [3] = GetBit (IOMap [0x47], 6) | (GetBit (IOMap [0x47], 7) << 1);
	
	if (CurrentY < Height) {
		for (int CurrentX = 0; CurrentX < Width; CurrentX++) {
			uint32_t ColorToDraw = Colors [0];
			uint8_t ActualX = CurrentX + ScrollX;
			uint8_t ActualY = CurrentY + ScrollY;
			
			if (GetBit (IOMap [0x40], 0)) { // BG Display
				uint8_t BGTile = Memory [BGTable + ((ActualY >> 3) << 5) + (ActualX >> 3)];
				uint8_t PixelX = ActualX & 0x7;
				uint8_t PixelY = ActualY & 0x7;
				
				uint8_t* BGTileData;
				if (BGAddressingMode)
					BGTileData = Memory + (BGTileTable + ((int8_t) BGTile) * 16);
				else {
					BGTileData = Memory + (BGTileTable + BGTile * 16);
				}
				
				// First Byte: LSB of Color for Pixel (PixelX, PixelY)
				// Second Byte: MSB ~~~
				
				uint8_t Color = (GetBit (BGTileData [PixelY * 2 + 1], 7 - PixelX) << 1) | GetBit (BGTileData [PixelY * 2], 7 - PixelX);
				
				ColorToDraw = Colors [BGPalette [Color]];
			}
			
			if (GetBit (IOMap [0x40], 1)) { // Sprite Display
				printf ("Display Sprites\n");
			}
			
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