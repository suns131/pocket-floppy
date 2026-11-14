/* A stand-in for SDL_image.h with only what ONScripter uses; see pocket_image.c. */
#ifndef POCKET_SDL_IMAGE_H
#define POCKET_SDL_IMAGE_H
#include <SDL.h>
#ifdef __cplusplus
extern "C" {
#endif
SDL_Surface *IMG_Load_RW(SDL_RWops *source, int release);
SDL_Surface *IMG_LoadJPG_RW(SDL_RWops *source);
int IMG_isPNG(SDL_RWops *source);
#define IMG_GetError SDL_GetError
#ifdef __cplusplus
}
#endif
#endif
