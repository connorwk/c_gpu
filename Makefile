gpu: gpu.h gpu.c
	gcc -std=c99 -g -rdynamic -o gpu gpu.c -lSDL2 -lm

debug: gpu.h gpu.c
	gcc -std=c99 -g -rdynamic -o gpu gpu.c -lSDL2 -lm -ggdb

clean:
	rm gpu
