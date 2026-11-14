#import <UIKit/UIKit.h>
#import <QuartzCore/QuartzCore.h>
#import <ImageIO/ImageIO.h>
#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>
#import <os/lock.h>
#import <stdatomic.h>
#import "ons_host.h"
#import "pocket_ons.h"

static CALayer *target;
static ons_log_fn logger;
static os_unfair_lock frame_lock = OS_UNFAIR_LOCK_INIT;
static CGImageRef latest;
static _Atomic int pending;
static _Atomic uint64_t frames;
static _Atomic int exit_code;
static _Atomic int ended;
static char last_message[512];

static void host_log(void *context, const char *line) { if (logger) logger(line); }

// On the game's thread: the picture is copied (it is only valid during the call) and shown on the main thread.
static void host_frame(void *context, const void *pixels, int width, int height, int pitch) {
    atomic_fetch_add(&frames, 1);
    CFDataRef data = CFDataCreate(NULL, pixels, (CFIndex)pitch * height);
    if (!data) return;
    CGDataProviderRef provider = CGDataProviderCreateWithCFData(data);
    static CGColorSpaceRef space;
    static dispatch_once_t once;
    dispatch_once(&once, ^{ space = CGColorSpaceCreateDeviceRGB(); });
    // Blue first, the fourth byte unused: little-endian 32 bits with the alpha skipped.
    CGImageRef image = CGImageCreate(width, height, 8, 32, pitch, space, kCGBitmapByteOrder32Little | kCGImageAlphaNoneSkipFirst, provider, NULL, false, kCGRenderingIntentDefault);
    CGDataProviderRelease(provider);
    CFRelease(data);
    if (!image) return;
    os_unfair_lock_lock(&frame_lock);
    if (latest) CGImageRelease(latest);
    latest = image;
    os_unfair_lock_unlock(&frame_lock);
    // One hand-over at a time: a game that draws faster than the screen shows only loses pictures nobody would see.
    if (atomic_exchange(&pending, 1)) return;
    dispatch_async(dispatch_get_main_queue(), ^{
        atomic_store(&pending, 0);
        os_unfair_lock_lock(&frame_lock);
        CGImageRef show = latest ? CGImageRetain(latest) : NULL;
        os_unfair_lock_unlock(&frame_lock);
        if (!show) return;
        [CATransaction begin];
        [CATransaction setDisableActions:YES];
        target.contents = (__bridge id)show;
        [CATransaction commit];
        CGImageRelease(show);
    });
}

static void host_ended(void *context, int code, const char *message) {
    snprintf(last_message, sizeof last_message, "%s", message ? message : "");
    atomic_store(&exit_code, code);
    atomic_store(&ended, 1);
    if (logger) {
        char line[600];
        snprintf(line, sizeof line, "nscripter: the game ended (%d) %s", code, last_message);
        logger(line);
    }
}

int ons_host_start(const char *game_dir, const char *save_dir, const char *font, void *layer, ons_log_fn log) {
    if (pocket_ons_running()) return -2;
    target = (__bridge CALayer *)layer;
    logger = log;
    atomic_store(&frames, 0); atomic_store(&ended, 0); atomic_store(&exit_code, 0); atomic_store(&pending, 0);
    last_message[0] = 0;
    os_unfair_lock_lock(&frame_lock);
    if (latest) { CGImageRelease(latest); latest = NULL; }
    os_unfair_lock_unlock(&frame_lock);
    pocket_ons_host host = { host_frame, host_ended, host_log, NULL };
    return pocket_ons_start(game_dir, save_dir, font, 0, &host);
}

int ons_host_encoding(const char *game_dir) { return pocket_ons_guess_encoding(game_dir); }
int ons_host_font_ok(const char *path) { return pocket_ons_font_ok(path); }

void ons_host_mouse(double x, double y, int button, int pressed) {
    int width = 0, height = 0;
    pocket_ons_size(&width, &height);
    if (width <= 0 || height <= 0) return;
    int px = (int)(fmin(fmax(x, 0), 1) * (width - 1)), py = (int)(fmin(fmax(y, 0), 1) * (height - 1));
    pocket_ons_mouse(px, py, button, pressed);
}

void ons_host_key(int sdl_keycode, int pressed) { pocket_ons_key(sdl_keycode, pressed); }
void ons_host_wheel(int notches) { pocket_ons_wheel(notches); }
int ons_host_running(void) { return pocket_ons_running() && !atomic_load(&ended); }
int ons_host_exit_code(void) { return atomic_load(&exit_code); }
const char *ons_host_last_message(void) { return last_message; }

void ons_host_frame_info(int *width, int *height, uint64_t *count) {
    pocket_ons_size(width, height);
    if (count) *count = atomic_load(&frames);
}

int ons_host_capture(const char *path) {
    os_unfair_lock_lock(&frame_lock);
    CGImageRef image = latest ? CGImageRetain(latest) : NULL;
    os_unfair_lock_unlock(&frame_lock);
    if (!image) return 1;
    NSURL *url = [NSURL fileURLWithPath:[NSString stringWithUTF8String:path]];
    CGImageDestinationRef destination = CGImageDestinationCreateWithURL((__bridge CFURLRef)url, (__bridge CFStringRef)UTTypePNG.identifier, 1, NULL);
    int result = 1;
    if (destination) {
        CGImageDestinationAddImage(destination, image, NULL);
        result = CGImageDestinationFinalize(destination) ? 0 : 1;
        CFRelease(destination);
    }
    CGImageRelease(image);
    return result;
}

int ons_host_stop(void) {
    if (!pocket_ons_running()) return 0;
    // The game ends at its next look at its events; one that is in the middle of an effect takes a little longer.
    for (int i = 0; i < 60 && pocket_ons_running(); i++) {
        pocket_ons_quit();
        usleep(50 * 1000);
    }
    target = nil;
    return pocket_ons_running() ? 1 : 0;
}
