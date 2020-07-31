#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <SDL2/SDL.h>

#define SCREEN_WIDTH  640
#define SCREEN_HEIGHT 480

struct Vector { float x, y, z; };

// Points define a clockwise triangle
struct Triangle { struct Vector v[3]; };

    void setPixel(SDL_Surface * s, int x, int y, uint32_t color);
uint32_t getPixel(SDL_Surface * s, int x, int y);
   float edgeFunction(struct Vector a, struct Vector b, struct Vector c); 
void rasterizeTriangle(struct Triangle * tri, struct Triangle * uv);

