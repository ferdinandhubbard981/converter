default: converter

converter: converter.c
		clang -std=c11 -Wall -pedantic -g converter.c displayfull.c -I/usr/include/SDL2 -lSDL2 -lm -o converter -fsanitize=undefined -fsanitize=address