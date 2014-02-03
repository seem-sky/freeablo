
#include <stdio.h>
#include <stdint.h>
#include <time.h>

#include <SDL.h>
#include <SDL_ttf.h>
//#include <SDL_image.h>
#include <iostream>
#include <iomanip>
#include <vector>
#include <algorithm>
#include <sstream>
#include <map>

#include <cel/celfile.h>
#include <cel/celframe.h>

#include <levels/minfile.h>
#include <levels/tilfile.h>
#include <levels/dunfile.h>

#include "level/levelgen.h"
#include "level/random.h"


#define WIDTH 1280
#define HEIGHT 960
#define BPP 4
#define DEPTH 32


#include <sstream>

#define SSTR( x ) dynamic_cast< std::ostringstream & >( \
        ( std::ostringstream() << std::dec << x ) ).str()

#include<boost/filesystem.hpp>

namespace bfs = boost::filesystem;

// How many frames time values to keep
// The higher the value the smoother the result is...
// Don't make it 0 or less :)
#define FRAME_VALUES 10

// An array to store frame times:
Uint32 frametimes[FRAME_VALUES];

// Last calculated SDL_GetTicks
Uint32 frametimelast;

// total frames rendered
Uint32 framecount;

// the value you want
float framespersecond;

// This function gets called once on startup.
void fpsinit() {

    // Set all frame times to 0ms.
    memset(frametimes, 0, sizeof(frametimes));
    framecount = 0;
    framespersecond = 0;
    frametimelast = SDL_GetTicks();

}

void fpsthink() {

    Uint32 frametimesindex;
    Uint32 getticks;
    Uint32 count;
    Uint32 i;

    // frametimesindex is the position in the array. It ranges from 0 to FRAME_VALUES.
    // This value rotates back to 0 after it hits FRAME_VALUES.
    frametimesindex = framecount % FRAME_VALUES;

    // store the current time
    getticks = SDL_GetTicks();

    // save the frame time value
    frametimes[frametimesindex] = getticks - frametimelast;

    // save the last frame time for the next fpsthink
    frametimelast = getticks;

    // increment the frame count
    framecount++;

    // Work out the current framerate

    // The code below could be moved into another function if you don't need the value every frame.

    // I've included a test to see if the whole array has been written to or not. This will stop
    // strange values on the first few (FRAME_VALUES) frames.
    if (framecount < FRAME_VALUES) {

        count = framecount;

    } else {

        count = FRAME_VALUES;

    }

    // add up all the values and divide to get the average frame time.
    framespersecond = 0;
    for (i = 0; i < count; i++) {

        framespersecond += frametimes[i];

    }

    framespersecond /= count;

    // now to make it an actual frames per second value...
    framespersecond = 1000.f / framespersecond;

}


void setpixel_real(SDL_Surface *s, int x, int y, Colour c)
{
    y = y*s->pitch/BPP;

    Uint32 *pixmem32;
    Uint32 Colour;  
 
    Colour = SDL_MapRGB( s->format, c.r, c.g, c.b );
  
    pixmem32 = (Uint32*) s->pixels  + y + x;
    *pixmem32 = Colour;
}

void setpixel(SDL_Surface* s, int x, int y, Colour c)
{
    setpixel_real(s, x, y, c);
    return;

    setpixel_real(s, x*2, y*2, c);
    setpixel_real(s, x*2+1, y*2, c);
    setpixel_real(s, x*2, y*2+1, c);
    setpixel_real(s, x*2+1, y*2+1, c);
}

void draw_at(SDL_Surface* s, int start_x, int start_y, const CelFrame& frame)
{
    for(int x = 0; x < frame.width; x++)
    {
        for(int y = 0; y < frame.height; y++)
        {
            if(frame[x][y].visible)
                setpixel(s, start_x+x, start_y+y, frame[x][y]);
        }
    }
}

SDL_Surface* screen;

SDL_Surface** tileset = NULL;

SDL_Surface* create_transparent_surface(size_t width, size_t height)
{
     SDL_Surface* s; 
    
    // SDL y u do dis
    #if SDL_BYTEORDER == SDL_BIG_ENDIAN
        s = SDL_CreateRGBSurface(SDL_HWSURFACE, width, height, DEPTH, 0xFF000000, 0x00FF0000, 0x0000FF00, 0x000000FF);
    #else
        s = SDL_CreateRGBSurface(SDL_HWSURFACE, width, height, DEPTH, 0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000);
    #endif

    /*s = SDL_CreateRGBSurface(SDL_HWSURFACE, width, height, screen->format->BitsPerPixel,
    screen->format->Rmask,
            screen->format->Gmask,
                    screen->format->Bmask,
                            screen->format->Amask
                            );
*/
    SDL_FillRect(s, NULL, SDL_MapRGBA(s->format, 0, 0, 0, 0)); 

    return s;
}


SDL_Surface* get_sprite(CelFile& f, size_t index)
{
    if(tileset == NULL)
    {
        size_t size = f.numFrames();

        tileset = new SDL_Surface*[size];
        
        for(size_t i = 0; i < size; i++)
            tileset[i] = NULL;
    }
    
    if(tileset[index] != NULL)
        return tileset[index];
    
    CelFrame frame = f[index];

    SDL_Surface* s = create_transparent_surface(frame.width, frame.height);

    draw_at(s, 0, 0, frame);

    
    tileset[index] = s;

    return s;
}

void draw_surface(SDL_Surface* s, int x, int y)
{
    if(x+s->w > 0 && x < WIDTH && y+s->h > 0 && y < HEIGHT)
    {
        SDL_Rect rcDest = { x, y, 0, 0 };
        SDL_BlitSurface (s , NULL, screen, &rcDest );
    }
}

void blit(SDL_Surface* from, SDL_Surface* to, int x, int y)
{
    SDL_Rect rcDest = { x, y, 0, 0 };
    SDL_BlitSurface (from , NULL, to, &rcDest );
}

int x_base = WIDTH/2, y_base = 0;

void draw_min_tile(SDL_Surface* s, CelFile& f, int x, int y, int16_t l, int16_t r)
{
    if(l != -1)
    {
        draw_at(s, x, y, f[l]);
        
        #ifdef CEL_DEBUG
            TTF_Font* font = TTF_OpenFont("FreeMonoBold.ttf", 8);
            SDL_Color foregroundColor = { 0, 0, 0 }; 
            SDL_Color backgroundColor = { 255, 255, 255 };
            SDL_Surface* textSurface = TTF_RenderText_Shaded(font, SSTR(l).c_str(), foregroundColor, backgroundColor);
           
            blit(textSurface, s, x, y);

            SDL_FreeSurface(textSurface);
            TTF_CloseFont(font);
        #endif
    }
    if(r != -1)
    {
        draw_at(s, x+32, y, f[r]);

        #ifdef CEL_DEBUG
            TTF_Font* font = TTF_OpenFont("FreeMonoBold.ttf", 8);
            SDL_Color foregroundColor = { 255, 0, 0 }; 
            SDL_Color backgroundColor = { 255, 255, 255 };
            SDL_Surface* textSurface = TTF_RenderText_Shaded(font, SSTR(r).c_str(), foregroundColor, backgroundColor);
           
            blit(textSurface, s, x+32, y);

            SDL_FreeSurface(textSurface);
            TTF_CloseFont(font);
       #endif
    }
}

void draw_min_pillar(SDL_Surface* s, int x, int y, const MinPillar& pillar, CelFile& tileset)
{
    // Each iteration draw one row of the min
    for(int i = 0; i < pillar.size(); i+=2)
    {
        int16_t l = (pillar[i]&0x0FFF)-1;
        int16_t r = (pillar[i+1]&0x0FFF)-1;
        
        draw_min_tile(s, tileset, x, y, l, r);
    
        y += 32; // down 32 each row

        //std::cout << i << " " << l << " " << r << std::endl;
    }
}

std::map<size_t, SDL_Surface*> tilCache;

void draw_til_block(SDL_Surface* to, int x, int y, const TilFile& til, size_t index, const MinFile& min, CelFile& tileset)
{
    //x += x_base;
    //y += y_base;
    
    //if(x < screen->w && x+128 > 0 && y < screen->h && y+288 > 0)
    {
        SDL_Surface* s;

        if(tilCache.count(index))
            s = tilCache[index];

        else
        {
            s = create_transparent_surface(128, 288);
            
            if(index < til.size())
            {
                draw_min_pillar(s, 32,  0, min
                    [til
                    [index]
                    [0]],
                     tileset);
                draw_min_pillar(s,  0, 16, min[til[index][2]], tileset);
                draw_min_pillar(s, 64, 16, min[til[index][1]], tileset);
                draw_min_pillar(s, 32, 32, min[til[index][3]], tileset);
            }
            else
            {
                std::cout << "ERR" <<std::endl;
            }

            tilCache[index] = s;
        }
        
        blit(s, to, x, y);
    }
}

int tmpindex = 0;

SDL_Surface* level = NULL;
void draw_level(DunFile dun, CelFile& town, MinFile min, TilFile til)
{

    if(level == NULL)
    {
        level = SDL_CreateRGBSurface(SDL_HWSURFACE, ((dun.mWidth+dun.mHeight))*64, ((dun.mWidth+dun.mHeight))*32 + 224, screen->format->BitsPerPixel,
                                              screen->format->Rmask,
                                              screen->format->Gmask,
                                              screen->format->Bmask,
                                              screen->format->Amask);

        int x_shift = dun.mHeight*64 - 64;

        for(int x = 0; x < dun.mWidth; x++)
        {
            for(int y = 0; y < dun.mHeight; y++)
            {
                if(dun[x][y] != 0)
                    draw_til_block(level, (y*(-64)) + 64*x + x_shift, (y*32) + 32*x, til, dun[x][y]-1, min, town);
            }
        }

        for(std::map<size_t, SDL_Surface*>::iterator it = tilCache.begin(); it != tilCache.end(); ++it)
        {
            SDL_FreeSurface((*it).second);
        }

        tilCache.clear();

        SDL_SaveBMP(level, "test.bmp");
    }
   
    
    //draw_til_block(screen, 0, 0, til, tmpindex, min, town);
    //draw_min_pillar(screen,  0, 0, min[tmpindex], town);
    blit(level, screen, x_base, y_base);
}


int main(int argc, char** argv)
{
    SDL_Event event;
  
    int h=0; 
  
    TTF_Init();
    if (SDL_Init(SDL_INIT_VIDEO) < 0 ) return 1;
   
    if (!(screen = SDL_SetVideoMode(WIDTH, HEIGHT, DEPTH, SDL_HWSURFACE | SDL_DOUBLEBUF)))
    {
        SDL_Quit();
        return 1;
    }
    
    /*CelFile town("levels/towndata/town.cel");
    MinFile min("levels/towndata/town.min");
    TilFile til("levels/towndata/town.til");

    DunFile sector1("levels/towndata/sector1s.dun");
    DunFile sector2("levels/towndata/sector2s.dun");
    DunFile sector3("levels/towndata/sector3s.dun");
    DunFile sector4("levels/towndata/sector4s.dun");

    DunFile dun = getTown(sector1, sector2, sector3, sector4);
    */
    CelFile town("levels/l1data/l1.cel");
    MinFile min("levels/l1data/l1.min");
    TilFile til("levels/l1data/l1.til");
   // DunFile dun(argv[1]);
    DunFile dun;

    Freeablo::FAsrand(time(NULL));
    Freeablo::generate(100, 100, dun);
    
    int lr = 0;
    int ud = 0;

    fpsinit();    
 
    bool quit = false;
    while(!quit)
    {
        while(SDL_PollEvent(&event)){
            switch (event.type) 
            {
                case SDL_QUIT:
	                quit = true;
	                break;

                case SDL_KEYDOWN:
                    switch(event.key.keysym.sym){
                        case SDLK_w:
                            ud--;
                            break;
                        case SDLK_s:
                            ud++;
                            break;
                        case SDLK_a:
                            lr--;
                            break;
                        case SDLK_d:
                            lr++;
                            break;
                        case SDLK_p:
                            tmpindex++;
                            break;
                        case SDLK_o:
                            tmpindex--;
                            break;


                    }
                    break;

                case SDL_KEYUP:
                    switch(event.key.keysym.sym){
                         case SDLK_w:
                            ud++;
                            break;
                        case SDLK_s:
                            ud--;
                            break;
                        case SDLK_a:
                            lr++;
                            break;
                        case SDLK_d:
                            lr--;
                            break;
                    }
                    break;


            }
        }

        
        if(SDL_MUSTLOCK(screen)) 
        {
            if(SDL_LockSurface(screen) < 0) continue;
        }

        x_base += lr;
        y_base += ud; 
     
        SDL_FillRect(screen,NULL, SDL_MapRGB( screen->format, 0, 0, 255)); 
        draw_level(dun, town, min, til);
        draw_til_block(screen, 0, 0, til, tmpindex, min, town);
        SDL_Delay(1);
        
        
        if(SDL_MUSTLOCK(screen)) SDL_UnlockSurface(screen);
      
        SDL_Flip(screen); 
        
        fpsthink();

        std::stringstream s;
        s << argv[1] << " FPS: " << std::setw(3) << std::setfill('0') << (int)framespersecond << " " << tmpindex+1;

        SDL_WM_SetCaption(s.str().c_str(), 0);
    }
    TTF_Quit();
}
