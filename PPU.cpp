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

PPU::~PPU () {
	free (Pixels);
	free (OAMQueue);
}

inline void PPU::SetPixel (uint32_t CoordX, uint32_t CoordY, uint32_t Color) {
	uint32_t PixelNo = CoordY * Width + CoordX;
	Pixels [PixelNo] = Color;
}

void PPU::OAMSearch (uint8_t* Memory, uint8_t* IOMap) {
	uint8_t SpriteSize = 8 + (GetBit (IOMap [0x40], 2) << 3); // 8x8 or 8x16
	uint8_t QueueNumber = 0;
	
	for (int i = 0xFE00; i <= 0xFE9F; i += 4) {
		if (CurrentY + 16 >= Memory[i] && CurrentY + 16 < Memory[i] + SpriteSize) { // Y Position
			//printf ("%d: Load sprite at %d\n", QueueNumber, CurrentY);
			memcpy (OAMQueue + (QueueNumber << 2), Memory + i, 4);
			QueueNumber++;
			if (QueueNumber == 10) // Max 10 sprites per line
				break;
		}
	}
	
	SpriteCount = QueueNumber;
}

void PPU::Update (uint8_t* Memory, uint8_t* IOMap) {
	uint16_t BGTable = 0x9800;
	uint16_t WindowTable = 0x9800;
	if (GetBit (IOMap [0x40], 3))
		BGTable = 0x9C00;
	
	if (GetBit (IOMap [0x40], 6))
		WindowTable = 0x9C00;
		
	uint8_t BGAddressingMode = GetBit (IOMap [0x40], 4); // 0 - Signed (0x8000), 1 - Unsigned (0x9000)
	
	/* LCDC TODO:
		0xFF40: bits 0, 5, 6
		0xFF4A, 0xFF4B
		Window Drawing + X coordinate sprite priority
	*/
	
	if (GetBit (IOMap [0x40], 0)) { // BG Enabled
		// Set BG Palette
		BGPalette [0] = GetBit (IOMap [0x47], 0) | (GetBit (IOMap [0x47], 1) << 1);
		BGPalette [1] = GetBit (IOMap [0x47], 2) | (GetBit (IOMap [0x47], 3) << 1);
		BGPalette [2] = GetBit (IOMap [0x47], 4) | (GetBit (IOMap [0x47], 5) << 1);
		BGPalette [3] = GetBit (IOMap [0x47], 6) | (GetBit (IOMap [0x47], 7) << 1);
	}
	
	if (GetBit (IOMap [0x40], 1)) { // Sprites Enabled
		// Set Sprite Palettes
		SpritePalette0 [1] = GetBit (IOMap [0x48], 2) | (GetBit (IOMap [0x48], 3) << 1);
		SpritePalette0 [2] = GetBit (IOMap [0x48], 4) | (GetBit (IOMap [0x48], 5) << 1);
		SpritePalette0 [3] = GetBit (IOMap [0x48], 6) | (GetBit (IOMap [0x48], 7) << 1);

		SpritePalette1 [1] = GetBit (IOMap [0x49], 2) | (GetBit (IOMap [0x49], 3) << 1);
		SpritePalette1 [2] = GetBit (IOMap [0x49], 4) | (GetBit (IOMap [0x49], 5) << 1);
		SpritePalette1 [3] = GetBit (IOMap [0x49], 6) | (GetBit (IOMap [0x49], 7) << 1);
	}

	if (CurrentY < Height) {
		for (int CurrentX = 0; CurrentX < Width; CurrentX++) {
			uint32_t ColorToDraw = Colors [0];
			
			uint8_t BGColor = 0;
			if (GetBit (IOMap [0x40], 0)) { // BG Display
				uint8_t BGX = CurrentX + IOMap[0x43];
				uint8_t BGY = CurrentY + IOMap[0x42];
				
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
				BGColor = Color;
				ColorToDraw = Colors [BGPalette [Color]];
			}
			
			if (GetBit (IOMap [0x40], 5)) { // Window Display
				uint8_t CoordX = IOMap[0x4B];
				uint8_t CoordY = IOMap[0x4A];
				
				if (CoordX - 7 <= CurrentX && CoordY <= CurrentY) {
					uint8_t WindowX = CurrentX + 7 - CoordX;
					uint8_t WindowY = CurrentY - CoordY;
					
					uint8_t WindowTile = Memory [WindowTable + ((WindowY >> 3) << 5) + (WindowX >> 3)];
					uint8_t PixelX = WindowX & 0x7;
					uint8_t PixelY = WindowY & 0x7;
					
					uint8_t* WindowTileData;
					if (BGAddressingMode)
						WindowTileData = Memory + (0x8000 + (WindowTile << 4));
					else
						WindowTileData = Memory + (0x9000 + (int8_t) WindowTile * 16);

					// First Byte: LSB of Color for Pixel (PixelX, PixelY)
					// Second Byte: MSB ~~~

					uint8_t Color = (GetBit (WindowTileData [PixelY * 2 + 1], 7 - PixelX) << 1) | GetBit (WindowTileData [PixelY * 2], 7 - PixelX);
					ColorToDraw = Colors [BGPalette [Color]]; // Shared with BG
				}
			}
			
			if (GetBit (IOMap [0x40], 1)) { // Sprite Display
				uint16_t MinX = 256; // Just greater than max (SpriteX)
				
				for (int i = 0; i < (SpriteCount << 2); i += 4) {
					uint8_t CoordY = OAMQueue [i];
					uint8_t CoordX = OAMQueue [i + 1];
						
					if (CurrentX + 8 >= CoordX && CurrentX < CoordX) { // Sprite in Current X
						uint8_t PixelX = 7 - ((CurrentX + 8) - CoordX); // True X coordinate is 7 - X, due to the order of Bits
						uint8_t PixelY = (CurrentY + 16) - CoordY;
							
						uint8_t SpriteTile = OAMQueue [i + 2];
						if (GetBit (IOMap [0x40], 2)) // Ignore bit 0 if 8x16
							SetBit (SpriteTile, 0, 0);
							
						uint8_t* SpriteTileData = Memory + (0x8000 + (SpriteTile << 4));
						uint8_t Color = 0;
						
						if (GetBit (OAMQueue [i + 3], 5)) // Flip X
							PixelX = 7 - PixelX;
						
						// Y flipping is done differently for 8x16
						if (GetBit (IOMap [0x40], 2)) { // 0 - 8x8, 1 - 8x16
							if (GetBit (OAMQueue [i + 3], 6)) // Flip Y
								PixelY = 15 - PixelY;
						} else {
							if (GetBit (OAMQueue [i + 3], 6)) // Flip Y
								PixelY = 7 - PixelY;
						}
							
						Color = (GetBit (SpriteTileData [PixelY * 2 + 1], PixelX) << 1) | GetBit (SpriteTileData [PixelY * 2], PixelX);
						
						if (Color != 0) { // Not Transparent
							if (GetBit (OAMQueue [i + 3], 4)) // Choose sprite palette
								Color = SpritePalette1 [Color];
							else
								Color = SpritePalette0 [Color];
								
							if (GetBit (OAMQueue [i + 3], 7)) { // Above only if BG Color is 0
								if (BGColor == 0)
									ColorToDraw = Colors [Color];
							} else
								ColorToDraw = Colors [Color]; // Above BG
						}
					}
				}
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