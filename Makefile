gpu: gpu.h gpu.c
	gcc -std=c99 -g -rdynamic -o gpu gpu.c -lSDL2

clean:
	rm gpu
