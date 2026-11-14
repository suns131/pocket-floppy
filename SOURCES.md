# 开源组件与源码 · Open-source components and source

口袋软盘用到下面这些开源引擎。按各自授权的要求，这里公开口袋软盘对它们做的全部改动。
Pocket Floppy uses the open-source engines below. As their licences require, every change Pocket Floppy makes to them is published here.

| 组件 Component | 授权 Licence | 上游 Upstream | 口袋软盘的改动 Changes |
|---|---|---|---|
| mkxp-z (RPG Maker XP / VX / VX Ace) | GPL 2.0 | https://github.com/white-axe/mkxp-z (branch `libretro`, commit 650cb08) | [mkxp-z-pocket.patch](mkxp-z-pocket.patch) |
| ONScripter-jh (NScripter) | GPL 2.0 | https://github.com/jh10001/ONScripter-jh (branch `master`, 2022-08) | [onscripter-pocket.patch](onscripter-pocket.patch) and the `onscripter-*` files |
| KiriKit (KiriKiri) | MPL 2.0 | https://github.com/OWCramer/KiriKit | [KiriKit-pocket.patch](KiriKit-pocket.patch) |

## ONScripter 的附加文件 · Files added for ONScripter

- `onscripter-pocket.patch` — changes to the ONScripter-jh sources (software rendering, log lines to the host, engine object reached through a pointer, no controller opened by the engine)
- `onscripter-pocket_prefix.h` — placed in front of every source file by the build: `exit()` ends only the game's thread; presenting a frame and opening the window go to the host
- `onscripter-pocket_ons.cpp`, `onscripter-pocket_ons.h` — starting a game, input, ending, telling the script's encoding
- `onscripter-pocket_image.c`, `onscripter-SDL_image.h` — the three SDL_image calls done with stb_image
- `onscripter-gles_stub.cpp` — empty bodies for the optional GLES sharpening renderer (pictures are drawn in software)
- `onscripter-ons_host.h`, `onscripter-ons_host.m` — the app's side: pictures into a layer, screenshots, touches into the game's coordinates
- `onscripter-build.sh` — the build script (SDL2 headers at 2.0.18, SDL2_ttf 2.24.0, SDL2_mixer 2.8.2)

## 其他组件 · Other components

Ren'Py (MIT, with LGPL parts), Python (PSF), SDL2, SDL2_ttf, SDL2_mixer (zlib), FreeType (FTL), HarfBuzz, libpng, libjpeg-turbo, libwebp, zlib, FFmpeg and FriBidi (LGPL), ANGLE / MetalANGLE (BSD), libarchive (BSD), UnRAR (unRAR licence), inkjs (MIT), Ruffle (MIT / Apache 2.0), Ruby (Ruby licence), v86 (BSD 2-Clause), SeaBIOS (LGPL 3), an Alpine Linux image with Wine (GPL 2 / LGPL 2.1; sources at https://pkgs.alpinelinux.org, release 3.21, x86), stb_image / stb_vorbis / minimp3 / dr_flac (public domain / MIT), Source Han Sans (OFL), WenQuanYi fonts (Apache 2.0 / GPL with font exception).

这些组件未经修改，源码见各自的官方发布。
These are used unmodified; their sources are their own official releases.

需要某个组件对应版本的完整源码或构建方法，请在 [Issues](../../issues) 里提出。
For the complete corresponding source or build steps of any component, ask in [Issues](../../issues).
