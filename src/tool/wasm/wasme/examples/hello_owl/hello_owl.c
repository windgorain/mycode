#include <stdio.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <unistd.h>
#include <stdlib.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif


int testImage(SDL_Renderer* renderer, const char* fileName)
{
  SDL_Surface *image = IMG_Load(fileName);
  if (!image)
  {
     return 0;
  }
  int result = image->w;

  
  SDL_Rect dest = {.x = 200, .y = 100, .w = 200, .h = 200};

  SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, image);

  SDL_RenderCopy (renderer, tex, NULL, &dest);

  
  SDL_DestroyTexture (tex);

  SDL_FreeSurface (image);

  return result;
}

int main()
{
  SDL_Init(SDL_INIT_VIDEO);

  SDL_Window *window;
  SDL_Renderer *renderer;

  SDL_CreateWindowAndRenderer(600, 400, 0, &window, &renderer);

  int result = 0;

  
  SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
  SDL_RenderClear(renderer);

  
  result = testImage(renderer, "assets/owl.png");
  if (result == 0) {
    result = testImage(renderer, "examples_c/hello_owl/assets/owl.png");
  }
  if (result == 0) {
     printf("IMG_Load: %s\n", IMG_GetError());
     return 0;
  }

  
  SDL_RenderPresent(renderer);

  printf("you should see an image.\n");

  SDL_Delay(2000);

  return 0;
}

