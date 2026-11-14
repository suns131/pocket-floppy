/* The few calls the app makes into the NScripter engine (ONScripter-jh, GPL 2), and the ones it gets back. */
#ifndef POCKET_ONS_H
#define POCKET_ONS_H
#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    /* A finished picture: 32 bits a pixel, blue first, `pitch` bytes a row. Valid only during the call. */
    void (*frame)(void *context, const void *pixels, int width, int height, int pitch);
    /* The game is over (code 0), or could not go on; `message` is the last thing the engine said. */
    void (*ended)(void *context, int code, const char *message);
    void (*log)(void *context, const char *line);
    void *context;
} pocket_ons_host;

/* Starts the game in `game_dir` on a thread of its own. `font` is used when the game brings none (default.ttf).
   `encoding`: 0 work it out from the script, 1 Shift-JIS, 2 GBK. Returns 0 when the thread was started. */
int pocket_ons_start(const char *game_dir, const char *save_dir, const char *font, int encoding, const pocket_ons_host *host);
/* In the game's own pixels. button: 0 only moves, 1 left, 3 right. */
void pocket_ons_mouse(int x, int y, int button, int pressed);
void pocket_ons_key(int sdl_keycode, int pressed);
void pocket_ons_wheel(int notches);
/* Asks the game to end; `ended` follows. */
void pocket_ons_quit(void);
int pocket_ons_running(void);
/* The picture's size, 0 until the game has opened its window. */
void pocket_ons_size(int *width, int *height);
/* 1 when the engine's font library can open this font file. */
int pocket_ons_font_ok(const char *path);
/* 1 Shift-JIS, 2 GBK: what the script in a folder looks like. */
int pocket_ons_guess_encoding(const char *game_dir);

#ifdef __cplusplus
}
#endif
#endif
