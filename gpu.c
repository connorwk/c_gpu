#include "gpu.h"

SDL_Window * wind = NULL;
SDL_Surface * surf = NULL;
SDL_Window * uvwind = NULL;
SDL_Surface * uvsurf = NULL;

struct Camera cam = {
    // X, Y, Z
    10,0,-20,
    // Pitch, Yaw, Roll
    -0.4,0,1.57
};

float FOV = 1;

float z_depth [SCREEN_WIDTH][SCREEN_HEIGHT] = {0};

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
        {{ // Top
            {5, -5,  -5},
            {5, -5,  5},
            {5, 5,  5}
        }},
        {{
            {5, -5,  -5},
            {5, 5,  5},
            {5, 5,  -5}
        }},
        {{ // Left
            {-5, -5,  -5},
            {-5, -5,  5},
            {5, -5,  5}
        }},
        {{
            {-5, -5,  -5},
            {5, -5,  -5},
            {5, -5,  5}
        }},
        {{ // Bottom
            {-5, -5,  -5},
            {-5, 5,  -5},
            {-5, 5,  5}
        }},
        {{
            {-5, -5,  -5},
            {-5, 5,  5},
            {-5, -5,  5}
        }},
        {{ // Right
            {-5, -5,  5},
            {5, 5,  5},
            {5, -5,  5}
        }},
        {{
            {-5, -5,  5},
            {-5, 5,  5},
            {5, 5,  5}
        }}
    };

    struct TextureTri testuv[8] = {
    {{
            {256, 512},
            {256, 256},
            {512, 512}
        }},
        {{
            {256, 256},
            {512, 512},
            {512, 256}
        }},
        {{
            {512, 256},
            {512, 512},
            {256, 512}
        }},
        {{
            {512, 256},
            {256, 256},
            {256, 512}
        }},
	{{
            {256, 256},
            {512, 512},
            {512, 256}
        }},
        {{
            {256, 512},
            {256, 256},
            {512, 512}
        }},
	{{
            {512, 256},
            {256, 256},
            {512, 512}
        }},
        {{
            {256, 256},
            {512, 512},
            {256, 512}
        }}
    };

    /*
    for ( int i = 0; i < 8; i++ )
    {
        rasterizeTriangle(test + i, testuv + i);
    }
    */

    SDL_UpdateWindowSurface( wind );

    int running = 1;
    float x;
    while ( running )
    {
        x += 0.01;

        memset(z_depth, 0, sizeof z_depth);
        SDL_FillRect( surf, NULL, SDL_MapRGB( surf->format, 0, 0, 0 ) );
        //cam.x = sin(x)*20;
        cam.y = sin(x)*20;
        cam.z = -cos(x)*20;
        //cam.pitch = -x;
        cam.yaw = -x;
        //printf("Test: %f %f %f\n", cam.y, cam.z, cam.yaw);
        for ( int i = 0; i < 8; i++ )
        {
            rasterizeTriangle(test + i, testuv + i);
        }
        SDL_UpdateWindowSurface( wind );

        SDL_Event e;
        if ( SDL_PollEvent(&e) != 0 )
        {
            if ( e.type == SDL_QUIT ) running = 0;
            if ( e.type == SDL_WINDOWEVENT && e.window.event == SDL_WINDOWEVENT_CLOSE ) running = 0;
        }
        SDL_Delay(10);
    }

    // Cleanup
    SDL_DestroyWindow( wind );
    SDL_Quit();
    return 1;
}

void inline setPixel(SDL_Surface * s, int x, int y, uint32_t color) { *(uint32_t *)(s->pixels + y * s->pitch + x * sizeof(uint32_t)) = color; }
uint32_t inline getPixel(SDL_Surface * s, int x, int y) { return *(uint32_t *)(s->pixels + y * s->pitch + x * sizeof(uint32_t)); }

struct Vec3 cart2bary(struct Triangle * tri, int x, int y)
{
    float pxc = x - tri->v[2].x;
    float pyc = y - tri->v[2].y;

    float det = (tri->v[1].y - tri->v[2].y) * (tri->v[0].x - tri->v[2].x)
              + (tri->v[2].x - tri->v[1].x) * (tri->v[0].y - tri->v[2].y);

    struct Vec3 out;
    out.x = ( (tri->v[1].y - tri->v[2].y) * pxc + (tri->v[2].x - tri->v[1].x) * pyc ) / det;
    out.y = ( (tri->v[2].y - tri->v[0].y) * pxc + (tri->v[0].x - tri->v[2].x) * pyc ) / det;
    out.z = 1 - out.x - out.y;

    return out;
}

struct Vec3 bary2cart(struct TextureTri * tri, struct Vec3 bay )
{
    struct Vec3 out;

    out.x = tri->v[0].x * bay.x + tri->v[1].x * bay.y + tri->v[2].x * bay.z;
    out.y = tri->v[0].y * bay.x + tri->v[1].y * bay.y + tri->v[2].y * bay.z;
    out.z = 0;

    return out;
}

struct Vec3 projectPoint(struct Vec3 Point) {
    struct Vec3 t, c, s, d, out;
    float ez, bx, by;

    t.x = Point.x - cam.x;
    t.y = Point.y - cam.y;
    t.z = Point.z - cam.z;

    c.x = cos(cam.pitch);
    c.y = cos(cam.yaw);
    c.z = cos(cam.roll);

    s.x = sin(cam.pitch);
    s.y = sin(cam.yaw);
    s.z = sin(cam.roll);

    d.x = c.y * (s.z * t.y + c.z * t.x) - s.y * t.z;
    d.y = s.x * (c.y * t.z + s.y * (s.z * t.y + c.z * t.x)) + c.x * (c.z * t.y - s.z * t.x);
    out.z = c.x * (c.y * t.z + s.y * (s.z * t.y + c.z * t.x)) - s.x * (c.z * t.y - s.z * t.x);

    ez = 1.0 / tan(FOV / 2.0);

    bx = ez / out.z * d.x;
    by = ez / out.z * d.y;

    if (out.z < 0) {
        bx = -bx;
        by = -by;
    }

    out.x = (int) (SCREEN_WIDTH + bx * SCREEN_HEIGHT) / 2;
    out.y = (int) (SCREEN_HEIGHT + by * SCREEN_HEIGHT) / 2;

    return out;
}

void rasterizeTriangle( struct Triangle * tri, struct TextureTri * uv )
{
    int i;
    struct Triangle inTri = *tri;
    struct TextureTri inUV = *uv;
    struct Triangle rord;
    struct Vec3 pt, end, uv_o, test;
    struct Vec2 pu;

    int px, py;
    int render = 0;

    rord.v[0] = projectPoint(inTri.v[0]);
    px = (int) rord.v[0].x;
    py = (int) rord.v[0].y;
    //printf("P1 x:%i y:%i\n", px, py);
    render |= px >= 0 && px < SCREEN_WIDTH && py >= 0 && py < SCREEN_HEIGHT;
    rord.v[1] = projectPoint(inTri.v[1]);
    px = (int) rord.v[1].x;
    py = (int) rord.v[1].y;
    //printf("P2 x:%i y:%i\n", px, py);
    render |= px >= 0 && px < SCREEN_WIDTH && py >= 0 && py < SCREEN_HEIGHT;
    rord.v[2] = projectPoint(inTri.v[2]);
    px = (int) rord.v[2].x;
    py = (int) rord.v[2].y;
    //printf("P3 x:%i y:%i\n", px, py);
    render |= px >= 0 && px < SCREEN_WIDTH && py >= 0 && py < SCREEN_HEIGHT;

    if (!render) {
        return;
    }

    // Re-order Vectors so they go top to bottom in y order.
    if ( rord.v[0].y > rord.v[1].y ) { pt = rord.v[0]; rord.v[0] = rord.v[1]; rord.v[1] = pt;}
    if ( rord.v[0].y > rord.v[2].y ) { pt = rord.v[0]; rord.v[0] = rord.v[2]; rord.v[2] = pt;}
    if ( rord.v[1].y > rord.v[2].y ) { pt = rord.v[1]; rord.v[1] = rord.v[2]; rord.v[2] = pt;}

    float ls, rs, cx, xzs, yzls, yzrs;

    // Center point
    cx = rord.v[0].x + ((rord.v[1].y - rord.v[0].y)/(rord.v[2].y - rord.v[0].y)) * (rord.v[2].x - rord.v[0].x);

    // Draw top half of triangle
    // Get Left and Right slopes
    if ( rord.v[1].x <= cx )
    {
        ls  = (rord.v[1].x - rord.v[0].x) / (rord.v[1].y - rord.v[0].y);
        rs  = (rord.v[2].x - rord.v[0].x) / (rord.v[2].y - rord.v[0].y);
        yzls = (rord.v[1].z - rord.v[0].z) / (rord.v[1].y - rord.v[0].y);
        yzrs = (rord.v[2].z - rord.v[0].z) / (rord.v[2].y - rord.v[0].y);
    }
    else
    {
        rs  = (rord.v[1].x - rord.v[0].x) / (rord.v[1].y - rord.v[0].y);
        ls  = (rord.v[2].x - rord.v[0].x) / (rord.v[2].y - rord.v[0].y);
        yzrs = (rord.v[1].z - rord.v[0].z) / (rord.v[1].y - rord.v[0].y);
        yzls = (rord.v[2].z - rord.v[0].z) / (rord.v[2].y - rord.v[0].y);
    }
    //printf("Slope top rs:%f ls:%f\n", rs, ls);

    for ( end = pt = rord.v[0]; pt.y <= rord.v[1].y; pt.y++ )
    {
        if ( end.x > pt.x && pt.y >= 0 && pt.y < SCREEN_HEIGHT )
        {
            // Draw line
            for ( i = pt.x; i <= end.x; i++ )
            {
                float zd = pt.z;
                if ( zd > z_depth[i][(int)pt.y] && i >= 0 && i < SCREEN_WIDTH ) {
                    uv_o = bary2cart(uv, cart2bary(&rord, i, pt.y));
                    setPixel( surf, i, pt.y, getPixel( uvsurf, uv_o.x, uv_o.y ) );
                    //setPixel( surf, i, pt.y, 0x0000FF );
                    z_depth[i][(int)pt.y] = zd;
                }
                zd += xzs;
            }
        }

        pt.x += ls;  end.x += rs;
        pt.z += yzls; end.z += yzrs;
        xzs = (pt.z - end.z) / (pt.x - end.x);
    }

    // Draw bottom half of triangle
    // Get Left and Right slopes
    if ( cx < rord.v[1].x )
    {
        ls  = (rord.v[0].x - rord.v[2].x) / (rord.v[0].y - rord.v[2].y);
        rs  = (rord.v[1].x - rord.v[2].x) / (rord.v[1].y - rord.v[2].y);
        yzls = (rord.v[0].z - rord.v[2].z) / (rord.v[0].y - rord.v[2].y);
        yzrs = (rord.v[1].z - rord.v[2].z) / (rord.v[1].y - rord.v[2].y);
    }
    else
    {
        rs  = (rord.v[0].x - rord.v[2].x) / (rord.v[0].y - rord.v[2].y);
        ls  = (rord.v[1].x - rord.v[2].x) / (rord.v[1].y - rord.v[2].y);
        yzrs = (rord.v[0].z - rord.v[2].z) / (rord.v[0].y - rord.v[2].y);
        yzls = (rord.v[1].z - rord.v[2].z) / (rord.v[1].y - rord.v[2].y);
    }
    //printf("Slope bottom rs:%f ls:%f\n", rs, ls);
    for ( end = pt = rord.v[2]; pt.y >= rord.v[1].y; pt.y-- )
    {
        if ( end.x > pt.x && pt.y >= 0 && pt.y < SCREEN_HEIGHT )
        {
            // Draw line
            for ( i = pt.x; i <= end.x; i++ )
            {
                float zd = pt.z;
                if ( zd > z_depth[i][(int)pt.y] && i >= 0 && i < SCREEN_WIDTH ) {
                    uv_o = bary2cart(uv, cart2bary(&rord, i, pt.y));
                    setPixel( surf, i, pt.y, getPixel( uvsurf, uv_o.x, uv_o.y ) );
                    //setPixel( surf, i, pt.y, 0xFF00FF );
                    z_depth[i][(int)pt.y] = zd;
                }
                zd += xzs;
                //printf("UV: u: %f v: %f u: %f v: %f\n", test.x, test.y, uv_o.x, uv_o.y);
            }
        }
        
        pt.x -= ls;  end.x -= rs;
        pt.z -= yzls; end.z -= yzrs;
        xzs = (pt.z - end.z) / (pt.x - end.x);
    }
}
