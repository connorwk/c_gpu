#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <SDL2/SDL.h>

#define SCREEN_WIDTH  640
#define SCREEN_HEIGHT 480

struct Vec4 { float x, y, z, w; };

// Points define a triangle
struct Triangle { struct Vec4 v[3]; };

    void setPixel(SDL_Surface * s, int x, int y, uint32_t color);
uint32_t getPixel(SDL_Surface * s, int x, int y);
void rasterizeTriangle(struct Triangle * tri, struct Triangle * uv);

