deps = main.cpp CPU.cpp MMU.cpp PPU.cpp utils.cpp

main: $(deps)
	g++ --std=c++14 -Wextra -Wall -pedantic -O2 -fomit-frame-pointer -fno-rtti -fno-exceptions $(deps) -o main -lSDL2