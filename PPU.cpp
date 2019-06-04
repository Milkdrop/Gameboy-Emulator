#include "PPU.h"

const uint16_t Colors [4] = {0xffff, 0xaaaa, 0x5555, 0x0000};

uint8_t BGPalette [4];
uint8_t SpritePalette0 [4];
uint8_t SpritePalette1 [4];

PPU::PPU (const char* Title, const uint16_t _PixelSize) {
	PixelSize = _PixelSize;
	
	MainWindow = SDL_CreateWindow (Title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, Width * PixelSize, Height * PixelSize, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
	MainRenderer = SDL_CreateRenderer(MainWindow, -1, SDL_RENDERER_ACCELERATED);
	MainTexture = SDL_CreateTexture (MainRenderer, SDL_PIXELFORMAT_ARGB4444, SDL_TEXTUREACCESS_STREAMING, Width, Height);
	SDL_SetRenderDrawColor(MainRenderer, 0x00, 0x00, 0x00, 0x00);
	SDL_RenderClear(MainRenderer);
	
	SDL_RenderSetLogicalSize (MainRenderer, Width, Height);
}

inline void PPU::SetPixel (uint32_t CoordX, uint32_t CoordY, uint32_t Color) {
	uint32_t PixelNo = CoordY * Width + CoordX;
	
	Pixels [PixelNo] = Color;
}

uint8_t PPU::OAMSearch (uint8_t* Memory, uint8_t* IOMap) {
	uint8_t SpriteSize = 8 + (GetBit (IOMap [0x40], 2) << 3); // 8x8 or 8x16
	uint8_t QueueNumber = 0;
	
	for (int i = 0xFE00; i <= 0xFE9F; i += 4) {
		if (CurrentY + 16 >= Memory[i] && CurrentY + 16 <= Memory[i] + SpriteSize) { // Y Position
			memcpy (OAMQueue + QueueNumber, Memory + i, 4);
			QueueNumber++;
			if (QueueNumber == 10) // Max 10 sprites per line
				break;
		}
	}
	
	return QueueNumber;
}

void PPU::Update (uint8_t* Memory, uint8_t* IOMap) {
	uint16_t BGTable = 0x9800;
	if (GetBit (IOMap [0x40], 3))
		BGTable = 0x9C00;
	
	uint8_t BGAddressingMode = GetBit (IOMap [0x40], 4); // 0 - Signed (0x8000), 1 - Unsigned (0x9000)
	
	/* LCDC TODO:
		0xFF40: bits 0, 5, 6
		0xFF4A, 0xFF4B
		
	*/
	
	if (GetBit (IOMap [0x40], 0)) { // BG Enabled
		BGPalette [0] = GetBit (IOMap [0x47], 0) | (GetBit (IOMap [0x47], 1) << 1);
		BGPalette [1] = GetBit (IOMap [0x47], 2) | (GetBit (IOMap [0x47], 3) << 1);
		BGPalette [2] = GetBit (IOMap [0x47], 4) | (GetBit (IOMap [0x47], 5) << 1);
		BGPalette [3] = GetBit (IOMap [0x47], 6) | (GetBit (IOMap [0x47], 7) << 1);
	}
	
	if (GetBit (IOMap [0x40], 1)) { // Sprites Enabled
		SpritePalette0 [1] = GetBit (IOMap [0x48], 2) | (GetBit (IOMap [0x48], 3) << 1);
		SpritePalette0 [2] = GetBit (IOMap [0x48], 4) | (GetBit (IOMap [0x48], 5) << 1);
		SpritePalette0 [3] = GetBit (IOMap [0x48], 6) | (GetBit (IOMap [0x48], 7) << 1);

		SpritePalette1 [1] = GetBit (IOMap [0x49], 2) | (GetBit (IOMap [0x49], 3) << 1);
		SpritePalette1 [2] = GetBit (IOMap [0x49], 4) | (GetBit (IOMap [0x49], 5) << 1);
		SpritePalette1 [3] = GetBit (IOMap [0x49], 6) | (GetBit (IOMap [0x49], 7) << 1);
	}
	
	// Set BG Scrolling
	uint8_t ScrollY = IOMap[0x42];
	uint8_t ScrollX = IOMap[0x43];

	if (CurrentY < Height) {
		for (int CurrentX = 0; CurrentX < Width; CurrentX++) {
			uint32_t ColorToDraw = Colors [0];
			uint8_t BGX = CurrentX + ScrollX;
			uint8_t BGY = CurrentY + ScrollY;
			
			if (GetBit (IOMap [0x40], 0)) { // BG Display
				// Set BG Palette
				uint8_t BGTile = Memory [BGTable + ((BGY >> 3) << 5) + (BGX >> 3)];
				uint8_t PixelX = BGX & 0x7;
				uint8_t PixelY = BGY & 0x7;
				
				uint8_t* BGTileData;
				
				if (BGAddressingMode)
					BGTileData = Memory + (0x8000 + (BGTile << 4));
				else
					BGTileData = Memory + (0x9000 + (int8_t) BGTile * 16);
				
				// First Byte: LSB of Color for Pixel (PixelX, PixelY)
				// Second Byte: MSB ~~~
				
				uint8_t Color = (GetBit (BGTileData [PixelY * 2 + 1], 7 - PixelX) << 1) | GetBit (BGTileData [PixelY * 2], 7 - PixelX);
				ColorToDraw = Colors [BGPalette [Color]];
			}
			
			if (GetBit (IOMap [0x40], 1)) { // Sprite Display
				// Set Sprite Palettes
				//printf ("Display Sprites\n");
			}
			
			SetPixel (CurrentX, CurrentY, ColorToDraw);
		}
	}
	
	CurrentY = (CurrentY + 1) % 154;
	IOMap [0x44] = CurrentY; // Update current line that's being scanned
	
	if (CurrentY == 0) { // End of Frame, Time to actually draw the pixels
		SDL_UpdateTexture (MainTexture, NULL, Pixels, 2 * Width);
		SDL_RenderCopy (MainRenderer, MainTexture, NULL, NULL);
		SDL_RenderPresent (MainRenderer);
	}
}