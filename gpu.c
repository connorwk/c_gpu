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
        {{
            {70,  75,  5},
            {250, 25,  1},
            {250, 250, 1}
        }},
        {{
            {70,  75,  5},
            {70,  200, 5},
            {250, 250, 1}
        }},
        {{
            {50,  250, 1},
            {250, 250, 1},
            {250, 450, 1}
        }},
        {{
            {50,  250, 1},
            {70,  430, 5},
            {250, 450, 1}
        }},
        {{
            {350, 45,  5},
            {500, 45,  5},
            {550, 250, 1}
        }},
        {{
            {350, 45,  5},
            {300, 250, 1},
            {550, 250, 1}
        }},
        {{
            {300, 250, 1},
            {500, 250, 1},
            {550, 450, 1}
        }},
        {{
            {300, 250, 1},
            {350, 450, 1},
            {550, 450, 1}
        }}
    };

    struct Triangle testuv[8] = {
        {{
            {256, 256, 0},
            {768, 256, 0},
            {768, 768, 0}
        }},
        {{
            {256, 256, 0},
            {256, 768, 0},
            {768, 768, 0}
        }},
	{{
            {256, 256, 0},
            {768, 256, 0},
            {768, 768, 0}
        }},
        {{
            {256, 256, 0},
            {256, 768, 0},
            {768, 768, 0}
        }},
	{{
            {256, 256, 0},
            {768, 256, 0},
            {768, 768, 0}
        }},
        {{
            {256, 256, 0},
            {256, 768, 0},
            {768, 768, 0}
        }},
	{{
            {256, 256, 0},
            {768, 256, 0},
            {768, 768, 0}
        }},
        {{
            {256, 256, 0},
            {256, 768, 0},
            {768, 768, 0}
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

struct Vector cart2bayer(struct Triangle * tri, int x, int y)
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

struct Vector bayer2cart(struct Triangle * tri, struct Vector bay )
{
    struct Vector out;

    out.x = tri->v[0].x * bay.x + tri->v[1].x * bay.y + tri->v[2].x * bay.z;
    out.y = tri->v[0].y * bay.x + tri->v[1].y * bay.y + tri->v[2].y * bay.z;
    out.z = 0;

    return out;
}

void rasterizeTriangle( struct Triangle * tri, struct Triangle * uv )
{
    int i;
    struct Triangle rord = *tri;
    struct Vector pt, end;

    // Re-order Vectors so they go top to bottom in y order.
    if ( rord.v[0].y > rord.v[1].y ) { pt = rord.v[0]; rord.v[0] = rord.v[1]; rord.v[1] = pt; }
    if ( rord.v[0].y > rord.v[2].y ) { pt = rord.v[0]; rord.v[0] = rord.v[2]; rord.v[2] = pt; }
    if ( rord.v[1].y > rord.v[2].y ) { pt = rord.v[1]; rord.v[1] = rord.v[2]; rord.v[2] = pt; }

    float ls, rs, lsz, rsz;

    // Draw top half of triangle
    if ( rord.v[1].x <= rord.v[2].x )
    {
        ls  = (rord.v[1].x - rord.v[0].x) / (rord.v[1].y - rord.v[0].y);
        rs  = (rord.v[2].x - rord.v[0].x) / (rord.v[2].y - rord.v[0].y);

        lsz = (rord.v[1].z - rord.v[0].z) / (rord.v[1].y - rord.v[0].y);
        rsz = (rord.v[2].z - rord.v[0].z) / (rord.v[2].y - rord.v[0].y);
    }
    else
    {
        rs  = (rord.v[1].x - rord.v[0].x) / (rord.v[1].y - rord.v[0].y);
        ls  = (rord.v[2].x - rord.v[0].x) / (rord.v[2].y - rord.v[0].y);

        rsz = (rord.v[1].z - rord.v[0].z) / (rord.v[1].y - rord.v[0].y);
        lsz = (rord.v[2].z - rord.v[0].z) / (rord.v[2].y - rord.v[0].y);
    }

    for ( end = pt = rord.v[0]; pt.y <= rord.v[1].y; pt.y++ )
    {
        if ( end.x > pt.x )
        {
            struct Vector uv_s = bayer2cart(uv, cart2bayer(tri, pt.x,  pt.y));
            struct Vector uv_e = bayer2cart(uv, cart2bayer(tri, end.x, pt.y));

            uv_e.x /= end.z;
            uv_e.y /= end.z;
            uv_e.z = 1 / end.z;

            uv_s.x /= pt.z;
            uv_s.y /= pt.z;
            uv_s.z = 1 / pt.z;

            struct Vector uv_d = {
                (uv_e.x - uv_s.x) / (end.x - pt.x),
                (uv_e.y - uv_s.y) / (end.x - pt.x),
                (uv_e.z - uv_s.z) / (end.x - pt.x)
            };

            // Draw line
            for ( i = pt.x; i <= end.x; i++ )
            {
                float pcz = 1 / uv_s.z;

                setPixel( surf, i, pt.y, getPixel( uvsurf, uv_s.x * pcz, uv_s.y * pcz ) );

                uv_s.x += uv_d.x;
                uv_s.y += uv_d.y;
                uv_s.z += uv_d.z;
            }
        }

        pt.x += ls;  end.x += rs;
        pt.z += lsz; end.z += rsz;
    }

    // Draw bottom half of triangle
    if ( rord.v[0].x < rord.v[1].x )
    {
        ls  = (rord.v[0].x - rord.v[2].x) / (rord.v[0].y - rord.v[2].y);
        rs  = (rord.v[1].x - rord.v[2].x) / (rord.v[1].y - rord.v[2].y);

        lsz = (rord.v[0].z - rord.v[2].z) / (rord.v[0].y - rord.v[2].y);
        rsz = (rord.v[1].z - rord.v[2].z) / (rord.v[1].y - rord.v[2].y);
    }
    else
    {
        rs  = (rord.v[0].x - rord.v[2].x) / (rord.v[0].y - rord.v[2].y);
        ls  = (rord.v[1].x - rord.v[2].x) / (rord.v[1].y - rord.v[2].y);

        rsz = (rord.v[0].z - rord.v[2].z) / (rord.v[0].y - rord.v[2].y);
        lsz = (rord.v[1].z - rord.v[2].z) / (rord.v[1].y - rord.v[2].y);
    }

    for ( end = pt = rord.v[2]; pt.y >= rord.v[1].y; pt.y-- )
    {
        if ( end.x > pt.x )
        {
            struct Vector uv_s = bayer2cart(uv, cart2bayer(tri, pt.x,  pt.y));
            struct Vector uv_e = bayer2cart(uv, cart2bayer(tri, end.x, pt.y));

            uv_e.x /= end.z;
            uv_e.y /= end.z;
            uv_e.z = 1 / end.z;

            uv_s.x /= pt.z;
            uv_s.y /= pt.z;
            uv_s.z = 1 / pt.z;

            struct Vector uv_d = {
                (uv_e.x - uv_s.x) / (end.x - pt.x),
                (uv_e.y - uv_s.y) / (end.x - pt.x),
                (uv_e.z - uv_s.z) / (end.x - pt.x)
            };

            // Draw line
            for ( i = pt.x; i <= end.x; i++ )
            {
                float pcz = 1 / uv_s.z;

                setPixel( surf, i, pt.y, getPixel( uvsurf, uv_s.x * pcz, uv_s.y * pcz ) );

                uv_s.x += uv_d.x;
                uv_s.y += uv_d.y;
                uv_s.z += uv_d.z;
            }
        }

        pt.x -= ls;  end.x -= rs;
        pt.z -= lsz; end.z -= rsz;
    }
}
