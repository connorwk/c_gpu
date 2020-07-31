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
            {70,  75,  2},
            {250, 25,  1},
            {250, 250, 1}
        }},
        {{
            {70,  75,  2},
            {70,  200, 2},
            {250, 250, 1}
        }},
        {{ // Bottom left
            {50,  250, 1.5},
            {250, 250, 1},
            {250, 450, 1.5}
        }},
        {{
            {50,  250, 1.5},
            {80,  420, 2},
            {250, 450, 1.5}
        }},
        {{ // Top right
            {350, 45,  2},
            {500, 45,  2},
            {550, 250, 1}
        }},
        {{
            {350, 45,  2},
            {300, 250, 1},
            {550, 250, 1}
        }},
        {{ // Bottom right
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
    struct Triangle tr = *tri;
    struct Triangle tex = *uv;
    struct Vector pt, end;
    
    //=========================================================================
    // Texturemapper with full perspective correction, subpixels and subtexels,
    // uses floats all the way through

    static float dizdx, duizdx, dvizdx, dizdy, duizdy, dvizdy;
    static float xa, xb, iza, uiza, viza;
    static float dxdya, dxdyb, dizdya, duizdya, dvizdya;
    static char *texture;

        float x1, y1, x2, y2, x3, y3;
        float iz1, uiz1, viz1, iz2, uiz2, viz2, iz3, uiz3, viz3;
        float dxdy1, dxdy2, dxdy3;
        float tempf;
        float denom;
        float dy;
        int y1i, y2i, y3i;
        int side;

        int x4, x5;
        float z, u, v, dx;
        float iz, uiz, viz;



        // Shift XY coordinate system (+0.5, +0.5) to match the subpixeling
        //  technique

        x1 = tr.v[0].x + 0.5;
        y1 = tr.v[0].y + 0.5;
        x2 = tr.v[1].x + 0.5;
        y2 = tr.v[1].y + 0.5;
        x3 = tr.v[2].x + 0.5;
        y3 = tr.v[2].y + 0.5;

        // Calculate alternative 1/Z, U/Z and V/Z values which will be
        //  interpolated

        iz1 = 1 / tr.v[0].z;
        iz2 = 1 / tr.v[1].z;
        iz3 = 1 / tr.v[2].z;
        uiz1 = tex.v[0].x * iz1;
        viz1 = tex.v[0].y * iz1;
        uiz2 = tex.v[1].x * iz2;
        viz2 = tex.v[1].y * iz2;
        uiz3 = tex.v[2].x * iz3;
        viz3 = tex.v[2].y * iz3;

        //texture = poly->texture;

        // Sort the vertices in ascending Y order

    #define swapfloat(x, y) tempf = x; x = y; y = tempf;
        if (y1 > y2)
        {
            swapfloat(x1, x2);
            swapfloat(y1, y2);
            swapfloat(iz1, iz2);
            swapfloat(uiz1, uiz2);
            swapfloat(viz1, viz2);
        }
        if (y1 > y3)
        {
            swapfloat(x1, x3);
            swapfloat(y1, y3);
            swapfloat(iz1, iz3);
            swapfloat(uiz1, uiz3);
            swapfloat(viz1, viz3);
        }
        if (y2 > y3)
        {
            swapfloat(x2, x3);
            swapfloat(y2, y3);
            swapfloat(iz2, iz3);
            swapfloat(uiz2, uiz3);
            swapfloat(viz2, viz3);
        }
    #undef swapfloat

        y1i = y1;
        y2i = y2;
        y3i = y3;

        // Skip poly if it's too thin to cover any pixels at all

        if ((y1i == y2i && y1i == y3i)
            || ((int) x1 == (int) x2 && (int) x1 == (int) x3))
            return;

        // Calculate horizontal and vertical increments for UV axes (these
        //  calcs are certainly not optimal, although they're stable
        //  (handles any dy being 0)

        denom = ((x3 - x1) * (y2 - y1) - (x2 - x1) * (y3 - y1));

        if (!denom)        // Skip poly if it's an infinitely thin line
            return;    

        denom = 1 / denom;    // Reciprocal for speeding up
        dizdx = ((iz3 - iz1) * (y2 - y1) - (iz2 - iz1) * (y3 - y1)) * denom;
        duizdx = ((uiz3 - uiz1) * (y2 - y1) - (uiz2 - uiz1) * (y3 - y1)) * denom;
        dvizdx = ((viz3 - viz1) * (y2 - y1) - (viz2 - viz1) * (y3 - y1)) * denom;
        dizdy = ((iz2 - iz1) * (x3 - x1) - (iz3 - iz1) * (x2 - x1)) * denom;
        duizdy = ((uiz2 - uiz1) * (x3 - x1) - (uiz3 - uiz1) * (x2 - x1)) * denom;
        dvizdy = ((viz2 - viz1) * (x3 - x1) - (viz3 - viz1) * (x2 - x1)) * denom;

        // Calculate X-slopes along the edges

        if (y2 > y1)
            dxdy1 = (x2 - x1) / (y2 - y1);
        if (y3 > y1)
            dxdy2 = (x3 - x1) / (y3 - y1);
        if (y3 > y2)
            dxdy3 = (x3 - x2) / (y3 - y2);

        // Determine which side of the poly the longer edge is on

        side = dxdy2 > dxdy1;

        if (y1 == y2)
            side = x1 > x2;
        if (y2 == y3)
            side = x3 > x2;

        if (!side)    // Longer edge is on the left side
        {
            // Calculate slopes along left edge

            dxdya = dxdy2;
            dizdya = dxdy2 * dizdx + dizdy;
            duizdya = dxdy2 * duizdx + duizdy;
            dvizdya = dxdy2 * dvizdx + dvizdy;

            // Perform subpixel pre-stepping along left edge

            dy = 1 - (y1 - y1i);
            xa = x1 + dy * dxdya;
            iza = iz1 + dy * dizdya;
            uiza = uiz1 + dy * duizdya;
            viza = viz1 + dy * dvizdya;

            if (y1i < y2i)    // Draw upper segment if possibly visible
            {
                // Set right edge X-slope and perform subpixel pre-
                //  stepping

                xb = x1 + dy * dxdy1;
                dxdyb = dxdy1;

                //drawtpolyperspsubtriseg(y1i, y2i);
	int y4 = y1i;
	int y5 = y2i;
        while (y4 < y5)        // Loop through all lines in the segment
        {
            x4 = xa;
            x5 = xb;

            // Perform subtexel pre-stepping on 1/Z, U/Z and V/Z

            dx = 1 - (xa - x4);
            iz = iza + dx * dizdx;
            uiz = uiza + dx * duizdx;
            viz = viza + dx * dvizdx;

            while (x4++ < x5)    // Draw horizontal line
            {
                // Calculate U and V from 1/Z, U/Z and V/Z

                z = 1 / iz;
                u = uiz * z;
                v = viz * z;

                // Copy pixel from texture to screen

                //*(scr++) = texture[((((int) v) & 0xff) << 8) + (((int) u) & 0xff)];
                setPixel( surf, x4, y4, getPixel( uvsurf, u, v ) );
		//printf("Test: x=%d y=%d uvx=%d uvy=%d\n", x4, y4, (int)u, (int)v);

                // Step 1/Z, U/Z and V/Z horizontally

                iz += dizdx;
                uiz += duizdx;
                viz += dvizdx;
            }

            // Step along both edges

            xa += dxdya;
            xb += dxdyb;
            iza += dizdya;
            uiza += duizdya;
            viza += dvizdya;

            y4++;
        }

            }
            if (y2i < y3i)    // Draw lower segment if possibly visible
            {
                // Set right edge X-slope and perform subpixel pre-
                //  stepping

                xb = x2 + (1 - (y2 - y2i)) * dxdy3;
                dxdyb = dxdy3;

                //drawtpolyperspsubtriseg(y2i, y3i);
	int y4 = y2i;
	int y5 = y3i;
        while (y4 < y5)        // Loop through all lines in the segment
        {
            x4 = xa;
            x5 = xb;

            // Perform subtexel pre-stepping on 1/Z, U/Z and V/Z

            dx = 1 - (xa - x4);
            iz = iza + dx * dizdx;
            uiz = uiza + dx * duizdx;
            viz = viza + dx * dvizdx;

            while (x4++ < x5)    // Draw horizontal line
            {
                // Calculate U and V from 1/Z, U/Z and V/Z

                z = 1 / iz;
                u = uiz * z;
                v = viz * z;

                // Copy pixel from texture to screen

                //*(scr++) = texture[((((int) v) & 0xff) << 8) + (((int) u) & 0xff)];
                setPixel( surf, x4, y4, getPixel( uvsurf, u, v ) );
                //printf("Test: x=%d y=%d uvx=%d uvy=%d\n", x4, y4, (int)u, (int)v);

                // Step 1/Z, U/Z and V/Z horizontally

                iz += dizdx;
                uiz += duizdx;
                viz += dvizdx;
            }

            // Step along both edges

            xa += dxdya;
            xb += dxdyb;
            iza += dizdya;
            uiza += duizdya;
            viza += dvizdya;

            y4++;
        }

            }
        }
        else    // Longer edge is on the right side
        {
            // Set right edge X-slope and perform subpixel pre-stepping

            dxdyb = dxdy2;
            dy = 1 - (y1 - y1i);
            xb = x1 + dy * dxdyb;

            if (y1i < y2i)    // Draw upper segment if possibly visible
            {
                // Set slopes along left edge and perform subpixel
                //  pre-stepping

                dxdya = dxdy1;
                dizdya = dxdy1 * dizdx + dizdy;
                duizdya = dxdy1 * duizdx + duizdy;
                dvizdya = dxdy1 * dvizdx + dvizdy;
                xa = x1 + dy * dxdya;
                iza = iz1 + dy * dizdya;
                uiza = uiz1 + dy * duizdya;
                viza = viz1 + dy * dvizdya;

                //drawtpolyperspsubtriseg(y1i, y2i);
	int y4 = y1i;
	int y5 = y2i;
        while (y4 < y5)        // Loop through all lines in the segment
        {
            x4 = xa;
            x5 = xb;

            // Perform subtexel pre-stepping on 1/Z, U/Z and V/Z

            dx = 1 - (xa - x4);
            iz = iza + dx * dizdx;
            uiz = uiza + dx * duizdx;
            viz = viza + dx * dvizdx;

            while (x4++ < x5)    // Draw horizontal line
            {
                // Calculate U and V from 1/Z, U/Z and V/Z

                z = 1 / iz;
                u = uiz * z;
                v = viz * z;

                // Copy pixel from texture to screen

                //*(scr++) = texture[((((int) v) & 0xff) << 8) + (((int) u) & 0xff)];
                setPixel( surf, x4, y4, getPixel( uvsurf, u, v ) );
                //printf("Test: x=%d y=%d uvx=%d uvy=%d\n", x4, y4, (int)u, (int)v);

                // Step 1/Z, U/Z and V/Z horizontally

                iz += dizdx;
                uiz += duizdx;
                viz += dvizdx;
            }

            // Step along both edges

            xa += dxdya;
            xb += dxdyb;
            iza += dizdya;
            uiza += duizdya;
            viza += dvizdya;

            y4++;
        }

            }
            if (y2i < y3i)    // Draw lower segment if possibly visible
            {
                // Set slopes along left edge and perform subpixel
                //  pre-stepping

                dxdya = dxdy3;
                dizdya = dxdy3 * dizdx + dizdy;
                duizdya = dxdy3 * duizdx + duizdy;
                dvizdya = dxdy3 * dvizdx + dvizdy;
                dy = 1 - (y2 - y2i);
                xa = x2 + dy * dxdya;
                iza = iz2 + dy * dizdya;
                uiza = uiz2 + dy * duizdya;
                viza = viz2 + dy * dvizdya;

                //drawtpolyperspsubtriseg(y2i, y3i);
	int y4 = y2i;
	int y5 = y3i;
        while (y4 < y5)        // Loop through all lines in the segment
        {
            x4 = xa;
            x5 = xb;

            // Perform subtexel pre-stepping on 1/Z, U/Z and V/Z

            dx = 1 - (xa - x4);
            iz = iza + dx * dizdx;
            uiz = uiza + dx * duizdx;
            viz = viza + dx * dvizdx;

            while (x4++ < x5)    // Draw horizontal line
            {
                // Calculate U and V from 1/Z, U/Z and V/Z

                z = 1 / iz;
                u = uiz * z;
                v = viz * z;

                // Copy pixel from texture to screen

                //*(scr++) = texture[((((int) v) & 0xff) << 8) + (((int) u) & 0xff)];
                setPixel( surf, x4, y4, getPixel( uvsurf, u, v ) );
                //printf("Test: x=%d y=%d uvx=%d uvy=%d\n", x4, y4, (int)u, (int)v);

                // Step 1/Z, U/Z and V/Z horizontally

                iz += dizdx;
                uiz += duizdx;
                viz += dvizdx;
            }

            // Step along both edges

            xa += dxdya;
            xb += dxdyb;
            iza += dizdya;
            uiza += duizdya;
            viza += dvizdya;

            y4++;
        }

            }
        }
    }
