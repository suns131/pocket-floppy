/* The three SDL_image calls ONScripter makes, done with stb_image (public domain / MIT): PNG, JPEG, BMP, GIF. */
#include <SDL.h>
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_STATIC
#define STBI_NO_HDR
#define STBI_NO_LINEAR
#define STBI_NO_PIC
#define STBI_NO_PNM
#include "stb_image.h"
#include "SDL_image.h"

static int read_all(SDL_RWops *source, unsigned char **data, size_t *size) {
    Sint64 start = SDL_RWtell(source), length = SDL_RWsize(source);
    if (length <= 0 || start < 0) return 0;
    length -= start;
    unsigned char *buffer = (unsigned char *)SDL_malloc((size_t)length);
    if (!buffer) return 0;
    size_t done = 0;
    while (done < (size_t)length) {
        size_t got = SDL_RWread(source, buffer + done, 1, (size_t)length - done);
        if (!got) break;
        done += got;
    }
    *data = buffer; *size = done;
    return done > 0;
}

SDL_Surface *IMG_Load_RW(SDL_RWops *source, int release) {
    unsigned char *data = NULL; size_t size = 0;
    SDL_Surface *surface = NULL;
    if (source && read_all(source, &data, &size)) {
        int width = 0, height = 0, channels = 0;
        unsigned char *pixels = stbi_load_from_memory(data, (int)size, &width, &height, &channels, 4);
        if (pixels) {
            /* A picture without transparency stays 24 bits, as SDL_image gives it: the engine tells by the format. */
            int alpha = channels == 4 || channels == 2;
            surface = SDL_CreateRGBSurfaceWithFormat(0, width, height, alpha ? 32 : 24, alpha ? SDL_PIXELFORMAT_RGBA32 : SDL_PIXELFORMAT_RGB24);
            if (surface) {
                for (int y = 0; y < height; y++) {
                    unsigned char *row = (unsigned char *)surface->pixels + y * surface->pitch;
                    const unsigned char *from = pixels + (size_t)y * width * 4;
                    if (alpha) SDL_memcpy(row, from, (size_t)width * 4);
                    else for (int x = 0; x < width; x++) { row[x * 3] = from[x * 4]; row[x * 3 + 1] = from[x * 4 + 1]; row[x * 3 + 2] = from[x * 4 + 2]; }
                }
            }
            stbi_image_free(pixels);
        } else {
            SDL_SetError("%s", stbi_failure_reason());
        }
        SDL_free(data);
    }
    if (release && source) SDL_RWclose(source);
    return surface;
}

SDL_Surface *IMG_LoadJPG_RW(SDL_RWops *source) { return IMG_Load_RW(source, 0); }

int IMG_isPNG(SDL_RWops *source) {
    static const unsigned char magic[8] = { 0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A };
    unsigned char head[8];
    Sint64 start = SDL_RWtell(source);
    int is = SDL_RWread(source, head, 1, 8) == 8 && SDL_memcmp(head, magic, 8) == 0;
    SDL_RWseek(source, start, RW_SEEK_SET);
    return is;
}
