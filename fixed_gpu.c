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
            {70,  75,  5, 1},
            {250, 25,  1, 1},
            {250, 250, 1, 1}
        }},
        {{
            {70,  75,  5, 1},
            {70,  200, 5, 1},
            {250, 250, 1, 1}
        }},
        {{ // Bottom left
            {50,  250, 1, 1},
            {250, 450, 1, 1},
            {220, 280, 5, 1}
        }},
        {{
            {50,  250, 1, 1},
            {80,  420, 5, 1},
            {250, 450, 1, 1}
        }},
        {{ // Top right
            {350, 45,  5, 1},
            {500, 45,  5, 1},
            {550, 250, 1, 1}
        }},
        {{
            {350, 45,  5, 1},
            {300, 250, 1, 1},
            {550, 250, 1, 1}
        }},
        {{ // Bottom right
            {300, 250, 1, 1},
            {500, 250, 1, 1},
            {550, 450, 1, 1}
        }},
        {{
            {300, 250, 1, 1},
            {350, 450, 1, 1},
            {550, 450, 1, 1}
        }}
    };

    struct Texture testuv[8] = {
        {{
            {256, 256, 1},
            {768, 256, 1},
            {768, 768, 1}
        }},
        {{
            {256, 256, 1},
            {256, 768, 1},
            {768, 768, 1}
        }},
	{{
            {256, 256, 1},
            {768, 256, 1},
            {768, 768, 1}
        }},
        {{
            {256, 256, 1},
            {256, 768, 1},
            {768, 768, 1}
        }},
	{{
            {256, 256, 1},
            {768, 256, 1},
            {768, 768, 1}
        }},
        {{
            {256, 256, 1},
            {256, 768, 1},
            {768, 768, 1}
        }},
	{{
            {256, 256, 1},
            {768, 256, 1},
            {768, 768, 1}
        }},
        {{
            {256, 256, 1},
            {256, 768, 1},
            {768, 768, 1}
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

struct Vec4 cart2bary(struct Triangle * tri, int x, int y)
{
    float pxc = x - tri->v[2].x;
    float pyc = y - tri->v[2].y;

    float det = (tri->v[1].y - tri->v[2].y) * (tri->v[0].x - tri->v[2].x)
              + (tri->v[2].x - tri->v[1].x) * (tri->v[0].y - tri->v[2].y);

    struct Vec4 out;
    out.x = ( (tri->v[1].y - tri->v[2].y) * pxc + (tri->v[2].x - tri->v[1].x) * pyc ) / det;
    out.y = ( (tri->v[2].y - tri->v[0].y) * pxc + (tri->v[0].x - tri->v[2].x) * pyc ) / det;
    out.z = 1 - out.x - out.y;

    return out;
}

struct Vec4 bary2cart(struct Triangle * tri, struct Vec4 bay )
{
    struct Vec4 out;

    out.x = tri->v[0].x * bay.x + tri->v[1].x * bay.y + tri->v[2].x * bay.z;
    out.y = tri->v[0].y * bay.x + tri->v[1].y * bay.y + tri->v[2].y * bay.z;
    out.z = 0;

    return out;
}

void rasterizeTriangle( struct Triangle * tri, struct Texture * uv )
{
    int i;
    struct Triangle rord = *tri;
    struct Vec4 pt, end;

    // Re-order Vectors so they go top to bottom in y order.
    if ( rord.v[0].y > rord.v[1].y ) { pt = rord.v[0]; rord.v[0] = rord.v[1]; rord.v[1] = pt; }
    if ( rord.v[0].y > rord.v[2].y ) { pt = rord.v[0]; rord.v[0] = rord.v[2]; rord.v[2] = pt; }
    if ( rord.v[1].y > rord.v[2].y ) { pt = rord.v[1]; rord.v[1] = rord.v[2]; rord.v[2] = pt; }

    float ls, rs, cx;

    cx = rord.v[0].x + ((rord.v[1].y - rord.v[0].y)/(rord.v[2].y - rord.v[0].y)) * (rord.v[2].x - rord.v[0].x);

    // Draw top half of triangle
    if ( rord.v[1].x <= cx )
    {
        ls  = (rord.v[1].x - rord.v[0].x) / (rord.v[1].y - rord.v[0].y);
        rs  = (rord.v[2].x - rord.v[0].x) / (rord.v[2].y - rord.v[0].y);
    }
    else
    {
        rs  = (rord.v[1].x - rord.v[0].x) / (rord.v[1].y - rord.v[0].y);
        ls  = (rord.v[2].x - rord.v[0].x) / (rord.v[2].y - rord.v[0].y);
    }
    printf("Slope top rs:%f ls:%f\n", rs, ls);

    for ( end = pt = rord.v[0]; pt.y <= rord.v[1].y; pt.y++ )
    {
        if ( end.x > pt.x )
        {
            // Draw line
            for ( i = pt.x; i <= end.x; i++ )
            {
                setPixel( surf, i, pt.y, 255);
                //setPixel( surf, i, pt.y, getPixel( uvsurf, uv_s.x * pcz, uv_s.y * pcz ) );
            }
        }

        pt.x += ls;  end.x += rs;
    }

    // Draw bottom half of triangle
    if ( cx < rord.v[1].x )
    {
        ls  = (rord.v[0].x - rord.v[2].x) / (rord.v[0].y - rord.v[2].y);
        rs  = (rord.v[1].x - rord.v[2].x) / (rord.v[1].y - rord.v[2].y);
    }
    else
    {
        rs  = (rord.v[0].x - rord.v[2].x) / (rord.v[0].y - rord.v[2].y);
        ls  = (rord.v[1].x - rord.v[2].x) / (rord.v[1].y - rord.v[2].y);
    }
    printf("Slope bottom rs:%f ls:%f\n", rs, ls);
    for ( end = pt = rord.v[2]; pt.y >= rord.v[1].y; pt.y-- )
    {
        if ( end.x > pt.x )
        {
            // Draw line
            for ( i = pt.x; i <= end.x; i++ )
            {
		setPixel( surf, i, pt.y, 0xFF0000 );
                //setPixel( surf, i, pt.y, getPixel( uvsurf, uv_s.x, uv_s.y ) );
            }
        }

        pt.x -= ls;  end.x -= rs;
    }
}
