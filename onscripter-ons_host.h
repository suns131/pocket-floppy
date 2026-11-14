// NScripter games with the ONScripter-jh engine (GPL 2, linked as libpocketons.a). The engine draws
// in software on a thread of its own; this host shows each finished picture in a layer, keeps the
// latest one for screenshots, and passes touches and keys on in the game's own coordinates.
#ifndef ONS_HOST_H
#define ONS_HOST_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*ons_log_fn)(const char *line);

/// Starts the game in `game_dir` (the folder with nscript.dat or 0.txt), showing it in `layer` (a CALayer).
/// `font` is used when the game has no default.ttf of its own. Returns 0 when started.
int ons_host_start(const char *game_dir, const char *save_dir, const char *font, void *layer, ons_log_fn log);
/// 1 when the engine can open this font file (some system fonts are in forms it does not read).
int ons_host_font_ok(const char *path);
/// 1 Shift-JIS, 2 GBK: what the game's script looks like (to pick a font before starting).
int ons_host_encoding(const char *game_dir);
/// A pointer event at a point of the picture given as fractions (0…1). button: 0 only moves, 1 left, 3 right.
void ons_host_mouse(double x, double y, int button, int pressed);
void ons_host_key(int sdl_keycode, int pressed);
void ons_host_wheel(int notches);
/// 1 while the game runs; 0 once it has ended (by itself, or because it could not go on).
int ons_host_running(void);
/// How the game ended (0: its own "end"), and the last thing the engine said when it could not go on.
int ons_host_exit_code(void);
const char *ons_host_last_message(void);
/// The size of the picture, and how many pictures have been shown.
void ons_host_frame_info(int *width, int *height, uint64_t *frames);
/// Writes the latest picture to `path` as PNG. Returns 0 when written.
int ons_host_capture(const char *path);
/// Asks the game to end and waits a moment for it. Returns 0 when it has ended.
int ons_host_stop(void);

#ifdef __cplusplus
}
#endif

#endif
