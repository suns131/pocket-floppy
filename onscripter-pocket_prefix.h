/* Put in front of every ONScripter source by the build: the engine runs as a guest inside an app,
   so it must not end the process, and what it presents goes to the host. */
#ifndef POCKET_PREFIX_H
#define POCKET_PREFIX_H
#include <stdlib.h>
#include <stdio.h>
#include <SDL.h>
#ifdef __cplusplus
#include <cstdlib>
#include <cmath>
extern "C" {
#endif
void pocket_ons_exit(int code) __attribute__((noreturn));
void pocket_ons_present(SDL_Renderer *renderer);
SDL_Window *pocket_ons_window(const char *title, int x, int y, int w, int h, Uint32 flags);
void pocket_ons_say(int error, const char *format, va_list arguments);
#ifdef __cplusplus
}
#endif
#define exit(code) pocket_ons_exit(code)
#define SDL_RenderPresent(renderer) pocket_ons_present(renderer)
#define SDL_CreateWindow(title, x, y, w, h, flags) pocket_ons_window(title, x, y, w, h, flags)
#endif
