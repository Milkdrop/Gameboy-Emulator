#include <stdint.h>
#include <SDL2/SDL.h>
#include <stdio.h>
#ifndef DISPLAY_H
#define DISPLAY_H

class Display {
public:
	Display (const char* Title, const uint16_t _Width, const uint16_t _Height, const uint16_t _PixelSize);
	void Update (const uint8_t* VRAM);
private:
	uint16_t PixelSize;
	uint16_t Width;
	uint16_t Height;
	SDL_Window* MainWindow;
	SDL_Renderer* MainRenderer;
};

#endif