#include "gpu.h"

SDL_Window * wind = NULL;
SDL_Surface * surf = NULL;
SDL_Window * uvwind = NULL;
SDL_Surface * uvsurf = NULL;

int main( int argc, char ** argv )
{
    if ( SDL_Init( SDL_INIT_VIDEO ) < 0 ) { printf( "SDL Init fail, error: %s\n", SDL_GetError() ); return 0; }

    SDL_Surface * uvtemp = SDL_LoadBMP( "uvtest.bmp" );
    if ( uvtemp == NULL ) { printf( "SDL Texture load fail, error: %s\n", SDL_GetError() ); return 0; }

    uvwind = SDL_CreateWindow( "UV Viewer",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        uvtemp->w, uvtemp->h,
        SDL_WINDOW_SHOWN );

    wind = SDL_CreateWindow( *argv,
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        SCREEN_WIDTH, SCREEN_HEIGHT,
        SDL_WINDOW_SHOWN );

    argc--; argv++;
    

    if ( wind == NULL || uvwind == NULL ) { printf( "SDL Window fail, error: %s\n", SDL_GetError() ); return 0; }

    // Get surface and paint it black
    surf = SDL_GetWindowSurface( wind );
    SDL_FillRect( surf, NULL, SDL_MapRGB( surf->format, 0, 0, 0 ) );

    uvsurf = SDL_GetWindowSurface( uvwind );
    SDL_BlitSurface( uvtemp, NULL, uvsurf, NULL );
    SDL_FreeSurface( uvtemp );
    SDL_UpdateWindowSurface( uvwind );

    struct Triangle test[8] = {
        {{ // Top left
            {70,  75,  5},
            {250,250,  1},
            {250, 25,  1}
        }},
        {{
            {70,  75,  5},
            {250, 250, 1},
            {70,  200, 5}
        }},
        {{ // Bottom left
            {50,  250, 1},
            {250, 450, 1},
            {250, 250, 1}
        }},
        {{
            {50,  250, 1},
            {250, 450, 1},
            {80,  420, 5}
        }},
        {{ // Top right
            {350, 45,  5},
            {550, 250, 1},
            {500, 45,  5}
        }},
        {{
            {350, 45,  5},
            {550, 250, 1},
            {300, 250, 1}
        }},
        {{ // Bottom right
            {300, 250, 1},
            {550, 450, 1},
            {500, 250, 1}
        }},
        {{
            {300, 250, 1},
            {550, 450, 1},
            {350, 450, 1}
        }}
    };

    struct Triangle testuv[8] = {
        {{
            {256, 256, 0},
            {768, 768, 0},
            {768, 256, 0}
        }},
        {{
            {256, 256, 0},
            {768, 768, 0},
            {256, 768, 0}
        }},
	{{
            {256, 256, 0},
            {768, 768, 0},
            {768, 256, 0}
        }},
        {{
            {256, 256, 0},
            {768, 768, 0},
            {256, 768, 0}
        }},
	{{
            {256, 256, 0},
            {768, 768, 0},
            {768, 256, 0}
        }},
        {{
            {256, 256, 0},
            {768, 768, 0},
            {256, 768, 0}
        }},
	{{
            {256, 256, 0},
            {768, 768, 0},
            {768, 256, 0}
        }},
        {{
            {256, 256, 0},
            {768, 768, 0},
            {256, 768, 0}
        }}
    };

    for ( int i = 0; i < 8; i++ )
    {
        rasterizeTriangle(test + i, testuv + i);
    }

    SDL_UpdateWindowSurface( wind );

    int running = 1;
    while ( running )
    {
        SDL_Event e;
        while ( SDL_PollEvent(&e) != 0 )
        {
            if ( e.type == SDL_QUIT ) running = 0;
            if ( e.type == SDL_WINDOWEVENT && e.window.event == SDL_WINDOWEVENT_CLOSE ) running = 0;
        }

        SDL_Delay(100);
    }

    // Cleanup
    SDL_DestroyWindow( wind );
    SDL_Quit();
    return 1;
}

    void inline setPixel(SDL_Surface * s, int x, int y, uint32_t color) { *(uint32_t *)(s->pixels + y * s->pitch + x * sizeof(uint32_t)) = color; }
uint32_t inline getPixel(SDL_Surface * s, int x, int y) { return *(uint32_t *)(s->pixels + y * s->pitch + x * sizeof(uint32_t)); }

struct Vector cart2bary(struct Triangle * tri, int x, int y)
{
    float pxc = x - tri->v[2].x;
    float pyc = y - tri->v[2].y;

    float det = (tri->v[1].y - tri->v[2].y) * (tri->v[0].x - tri->v[2].x)
              + (tri->v[2].x - tri->v[1].x) * (tri->v[0].y - tri->v[2].y);

    struct Vector out;
    out.x = ( (tri->v[1].y - tri->v[2].y) * pxc + (tri->v[2].x - tri->v[1].x) * pyc ) / det;
    out.y = ( (tri->v[2].y - tri->v[0].y) * pxc + (tri->v[0].x - tri->v[2].x) * pyc ) / det;
    out.z = 1 - out.x - out.y;

    return out;
}

struct Vector bary2cart(struct Triangle * tri, struct Vector bay )
{
    struct Vector out;

    out.x = tri->v[0].x * bay.x + tri->v[1].x * bay.y + tri->v[2].x * bay.z;
    out.y = tri->v[0].y * bay.x + tri->v[1].y * bay.y + tri->v[2].y * bay.z;
    out.z = 0;

    return out;
}

struct Vector crossProduct(struct Vector vecta, struct Vector vectb)
{
    struct Vector out;

    out.x = vecta.y * vectb.z - vecta.z * vectb.y;
    out.y = vecta.z * vectb.x - vecta.x * vectb.z;
    out.z = vecta.x * vectb.y - vecta.y * vectb.x;

    return out;
}

void rasterizeTriangle( struct Triangle * tri, struct Triangle * uv )
{
    int i;
    float uv_maxx, uv_maxy, uv_minx, uv_miny, maxx, maxy, minx, miny, minz, maxz, s, t;
    struct Triangle uvord = *uv;
    struct Triangle rord = *tri;
    struct Vector vs1, vs2, q, uv_o, uv_c;

    /* get the bounding box of the triangle */
    maxx = fmaxf(rord.v[0].x, fmaxf(rord.v[1].x, rord.v[2].x));
    minx = fminf(rord.v[0].x, fminf(rord.v[1].x, rord.v[2].x));
    maxy = fmaxf(rord.v[0].y, fmaxf(rord.v[1].y, rord.v[2].y));
    miny = fminf(rord.v[0].y, fminf(rord.v[1].y, rord.v[2].y));
    maxz = fmaxf(rord.v[0].z, fmaxf(rord.v[1].z, rord.v[2].z));
    minz = fminf(rord.v[0].z, fminf(rord.v[1].z, rord.v[2].z));
    

    uv_maxx = fmaxf(uvord.v[0].x, fmaxf(uvord.v[1].x, uvord.v[2].x));
    uv_minx = fminf(uvord.v[0].x, fminf(uvord.v[1].x, uvord.v[2].x));
    uv_maxy = fmaxf(uvord.v[0].y, fmaxf(uvord.v[1].y, uvord.v[2].y));
    uv_miny = fminf(uvord.v[0].y, fminf(uvord.v[1].y, uvord.v[2].y));
    
    /* spanning vectors of edge (rord.v[0],rord.v[1]) and (rord.v[0],rord.v[2]) */
    vs1.x = rord.v[1].x - rord.v[0].x;
    vs1.y = rord.v[1].y - rord.v[0].y;
    vs2.x = rord.v[2].x - rord.v[0].x;
    vs2.y = rord.v[2].y - rord.v[0].y;

    for (int x = minx; x <= maxx; x++)
    {
        for (int y = miny; y <= maxy; y++)
        {
            q.x = x - rord.v[0].x;
	    q.y = y - rord.v[0].y;
	    q.z = 0;
            
            s = crossProduct(q, vs2).z / crossProduct(vs1, vs2).z;
            t = crossProduct(vs1, q).z / crossProduct(vs1, vs2).z;
            
            if ( (s >= 0) && (t >= 0) && (s + t <= 1) )
            { /* inside triangle */
		uv_o = bary2cart(uv, cart2bary(tri, x, y));
                
		uv_c.x = ((1 - uv_o.x) * (uv_minx / minz) + uv_o.x * (uv_maxx / maxz)) / ((1 - uv_o.x) * (1 / minz) + uv_o.x * (1 / maxz));
		uv_c.y = ((1 - uv_o.y) * (uv_miny / minz) + uv_o.y * (uv_maxy / maxz)) / ((1 - uv_o.y) * (1 / minz) + uv_o.y * (1 / maxz));

                setPixel(surf, x, y, getPixel(uvsurf, uv_c.x, uv_c.y));
            }
        }
    }
}
