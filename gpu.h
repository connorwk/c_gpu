#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <SDL2/SDL.h>

#define SCREEN_WIDTH  640
#define SCREEN_HEIGHT 480

struct Vec2 { float x, y; };
struct Vec3 { float x, y, z; };
struct Camera { float x, y, z, pitch, yaw, roll; };
// Points define a triangle
struct Triangle { struct Vec3 v[3]; };
struct TextureTri { struct Vec2 v[3]; };

    void setPixel(SDL_Surface * s, int x, int y, uint32_t color);
uint32_t getPixel(SDL_Surface * s, int x, int y);
struct Vec3 cart2bary(struct Triangle * tri, int x, int y);
struct Vec3 bary2cart(struct TextureTri * tri, struct Vec3 bay );
struct Vec3 projectPoint(struct Vec3 Point);
void rasterizeTriangle(struct Triangle * tri, struct TextureTri * uv);