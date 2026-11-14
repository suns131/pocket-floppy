/* The app's side of the NScripter engine: starts ONScripter on its own thread with SDL's "dummy" video
   driver (no window of its own), hands each finished picture to the host, takes the host's input as SDL
   events, and turns the engine's exit() into the end of that thread instead of the end of the app. */
#include <SDL.h>
#include <SDL_ttf.h>
#include <SDL_mixer.h>
#include <pthread.h>
#include <string>
#include <new>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include "ONScripter.h"
#include "gbk2utf16.h"
#include "sjis2utf16.h"
#include "pocket_ons.h"

#undef exit
#undef SDL_RenderPresent
#undef SDL_CreateWindow

Coding2UTF16 *coding2utf16 = NULL;

static pocket_ons_host host;
static pthread_t game_thread;
static volatile int running = 0;
static volatile int picture_width = 0, picture_height = 0;
static std::string game_dir, save_dir, font_file;
static int wanted_encoding = 0;
static char last_words[512];
static volatile int quit_wanted = 0;
ONScripter *pocket_ons_engine = NULL;
static SDL_Window *game_window = NULL;

static void say(const char *line) { if (host.log) host.log(host.context, line); }

extern "C" void pocket_ons_say(int error, const char *format, va_list arguments) {
    char line[1024];
    vsnprintf(line, sizeof line, format, arguments);
    size_t length = strlen(line);
    while (length > 0 && (line[length - 1] == '\n' || line[length - 1] == '\r')) line[--length] = 0;
    if (!length) return;
    if (error) snprintf(last_words, sizeof last_words, "%s", line);
    say(line);
}

static void finish(int code) {
    Mix_HaltMusic();
    Mix_HaltChannel(-1);
    Mix_CloseAudio();
    /* TTF_Quit is left out on purpose: the engine keeps the fonts it has opened in a list that outlives
       a game, and the next game would be handed fonts whose library is gone. */
    game_window = NULL;
    SDL_Quit();
    unsetenv("SDL_VIDEODRIVER");
    running = 0;
    picture_width = picture_height = 0;
    if (host.ended) host.ended(host.context, code, last_words);
}

/* What the engine calls where it used to call exit(): only this game ends. */
extern "C" void pocket_ons_exit(int code) {
    if (running && pthread_equal(pthread_self(), game_thread)) {
        finish(code);
        pthread_exit(NULL);
    }
    /* From some other thread (a timer): the game thread is asked to stop, this thread just ends. */
    SDL_Event event;
    memset(&event, 0, sizeof event);
    event.type = SDL_QUIT;
    SDL_PushEvent(&event);
    pthread_exit(NULL);
}

/* After every present the picture is in the window's own surface (software renderer): the host gets it. */
extern "C" void pocket_ons_present(SDL_Renderer *renderer) {
    SDL_RenderPresent(renderer);
    /* Asked to end before the engine was listening: asked again now that it is. */
    if (quit_wanted) { SDL_Event event; memset(&event, 0, sizeof event); event.type = SDL_QUIT; SDL_PushEvent(&event); }
    SDL_Surface *surface = game_window ? SDL_GetWindowSurface(game_window) : NULL;
    if (!surface || !surface->pixels) return;
    if (!picture_width) {
        char line[160];
        snprintf(line, sizeof line, "nscripter: first picture %dx%d, %d bytes a pixel, format %s", surface->w, surface->h, surface->format->BytesPerPixel, SDL_GetPixelFormatName(surface->format->format));
        say(line);
    }
    picture_width = surface->w;
    picture_height = surface->h;
    if (host.frame) host.frame(host.context, surface->pixels, surface->w, surface->h, surface->pitch);
}

/* The engine's one window is remembered: its surface is where the pictures end up. */
extern "C" SDL_Window *pocket_ons_window(const char *title, int x, int y, int w, int h, Uint32 flags) {
    game_window = SDL_CreateWindow(title ? title : "", x, y, w, h, flags & ~(Uint32)(SDL_WINDOW_FULLSCREEN | SDL_WINDOW_ALLOW_HIGHDPI));
    return game_window;
}

static bool exists(const std::string &path) { struct stat info; return stat(path.c_str(), &info) == 0; }

/* The script, decoded where it is kept scrambled: enough of it to tell the encoding. */
static std::string script_sample(const std::string &dir) {
    static const char *plain[] = { "0.txt", "00.txt", "0.utf", "00.utf", NULL };
    for (int i = 0; plain[i]; i++) {
        FILE *file = fopen((dir + "/" + plain[i]).c_str(), "rb");
        if (!file) continue;
        std::string text(200000, 0);
        text.resize(fread(&text[0], 1, text.size(), file));
        fclose(file);
        return text;
    }
    FILE *file = fopen((dir + "/nscript.dat").c_str(), "rb");
    if (file) {
        std::string text(200000, 0);
        text.resize(fread(&text[0], 1, text.size(), file));
        fclose(file);
        for (size_t i = 0; i < text.size(); i++) text[i] ^= 0x84;
        return text;
    }
    return std::string();
}

extern "C" int pocket_ons_guess_encoding(const char *dir) {
    std::string text = script_sample(dir);
    /* Hiragana and katakana are 0x82 and 0x83 lead bytes in Shift-JIS; GBK text hardly ever starts a character with them. */
    long kana = 0, pairs = 0;
    for (size_t i = 0; i + 1 < text.size(); i++) {
        unsigned char lead = (unsigned char)text[i];
        if (lead < 0x81) continue;
        pairs++;
        if (lead == 0x82 || lead == 0x83) kana++;
        i++;
    }
    return pairs > 0 && kana * 20 > pairs ? 1 : 2;
}

/* Some of the system's fonts are in forms FreeType does not read: a font is tried before it is offered. */
extern "C" int pocket_ons_font_ok(const char *path) {
    if (!path || !*path) return 0;
    if (!TTF_WasInit() && TTF_Init() < 0) return 0;
    TTF_Font *font = TTF_OpenFont(path, 20);
    if (!font) return 0;
    int usable = TTF_FontHeight(font) > 0;
    TTF_CloseFont(font);
    return usable;
}

static void *game_main(void *) {
    setenv("SDL_VIDEODRIVER", "dummy", 1);
    last_words[0] = 0;
    int encoding = wanted_encoding ? wanted_encoding : pocket_ons_guess_encoding(game_dir.c_str());
    if (coding2utf16) { delete coding2utf16; coding2utf16 = NULL; }
    if (encoding == 1) coding2utf16 = new SJIS2UTF16(); else coding2utf16 = new GBK2UTF16();
    char line[256];
    snprintf(line, sizeof line, "nscripter: script read as %s", encoding == 1 ? "Shift-JIS" : "GBK");
    say(line);

    /* The engine was written as one global object, and counts on its memory starting out as zeros
       (members such as the forced window size are never set otherwise): give it that. */
    void *memory = calloc(1, sizeof(ONScripter));
    pocket_ons_engine = new (memory) ONScripter();
    pocket_ons_engine->setArchivePath(game_dir.c_str());
    pocket_ons_engine->setSaveDir(save_dir.c_str());
    /* The game's own default.ttf comes first; without one, the font the host offers. */
    std::string own = game_dir + "/default.ttf";
    if ((!exists(own) || !pocket_ons_font_ok(own.c_str())) && !font_file.empty()) pocket_ons_engine->setFontFile(font_file.c_str());
    pocket_ons_engine->enableButtonShortCut();
    if (pocket_ons_engine->openScript()) pocket_ons_exit(-1);
    if (pocket_ons_engine->init()) pocket_ons_exit(-1);
    pocket_ons_engine->executeLabel();
    pocket_ons_exit(0);
    return NULL;
}

extern "C" int pocket_ons_start(const char *dir, const char *saves, const char *font, int encoding, const pocket_ons_host *callbacks) {
    if (running) return -1;
    host = *callbacks;
    game_dir = dir; save_dir = saves ? saves : dir; font_file = font ? font : "";
    wanted_encoding = encoding;
    quit_wanted = 0;
    running = 1;
    pthread_attr_t attributes;
    pthread_attr_init(&attributes);
    pthread_attr_setstacksize(&attributes, 8 * 1024 * 1024);
    int failed = pthread_create(&game_thread, &attributes, game_main, NULL);
    pthread_attr_destroy(&attributes);
    if (failed) running = 0;
    return failed;
}

extern "C" int pocket_ons_running(void) { return running; }
extern "C" void pocket_ons_size(int *width, int *height) { if (width) *width = picture_width; if (height) *height = picture_height; }

extern "C" void pocket_ons_mouse(int x, int y, int button, int pressed) {
    if (!running || !SDL_WasInit(SDL_INIT_VIDEO)) return;
    SDL_Event event;
    memset(&event, 0, sizeof event);
    event.type = SDL_MOUSEMOTION;
    event.motion.x = x; event.motion.y = y;
    SDL_PushEvent(&event);
    if (!button) return;
    memset(&event, 0, sizeof event);
    event.type = pressed ? SDL_MOUSEBUTTONDOWN : SDL_MOUSEBUTTONUP;
    event.button.button = button == 3 ? SDL_BUTTON_RIGHT : SDL_BUTTON_LEFT;
    event.button.state = pressed ? SDL_PRESSED : SDL_RELEASED;
    event.button.x = x; event.button.y = y;
    SDL_PushEvent(&event);
}

extern "C" void pocket_ons_key(int keycode, int pressed) {
    if (!running || !SDL_WasInit(SDL_INIT_VIDEO)) return;
    SDL_Event event;
    memset(&event, 0, sizeof event);
    event.type = pressed ? SDL_KEYDOWN : SDL_KEYUP;
    event.key.state = pressed ? SDL_PRESSED : SDL_RELEASED;
    event.key.keysym.sym = keycode;
    event.key.keysym.scancode = SDL_GetScancodeFromKey(keycode);
    SDL_PushEvent(&event);
}

extern "C" void pocket_ons_wheel(int notches) {
    if (!running || !SDL_WasInit(SDL_INIT_VIDEO)) return;
    SDL_Event event;
    memset(&event, 0, sizeof event);
    event.type = SDL_MOUSEWHEEL;
    event.wheel.y = notches;
    SDL_PushEvent(&event);
}

extern "C" void pocket_ons_quit(void) {
    if (!running) return;
    quit_wanted = 1;
    SDL_Event event;
    memset(&event, 0, sizeof event);
    event.type = SDL_QUIT;
    SDL_PushEvent(&event);
}
