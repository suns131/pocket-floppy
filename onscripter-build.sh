#!/bin/sh
# usage: build.sh sim|dev     builds libpocketons.a (ONScripter-jh + SDL_ttf + SDL_mixer + the app's glue) for one platform.
# SDL itself and FreeType are the ones the app already links (Ren'Py's), so only their headers are used here:
# SDL's with the version set back to the library's (2.0.18), FreeType's from SDL_ttf's own copy.
set -e
W=$(cd "$(dirname "$0")" && pwd)
case "$1" in
  sim) SDK=iphonesimulator; TARGET=arm64-apple-ios18.0-simulator;;
  dev) SDK=iphoneos; TARGET=arm64-apple-ios18.0;;
  *) echo "usage: build.sh sim|dev"; exit 1;;
esac
SYSROOT=$(xcrun --sdk $SDK --show-sdk-path)
OUT=$W/out/$1; rm -rf $OUT; mkdir -p $OUT/obj
COMMON="-target $TARGET -isysroot $SYSROOT -O2 -g0 -fvisibility=hidden -fno-common -I$W/inc-sdl -I$W/pocket -w"
CC="xcrun --sdk $SDK clang"; CXX="xcrun --sdk $SDK clang++"
n=0
compile() { # compiler flags file
  n=$((n+1)); o=$OUT/obj/$n-$(basename "$3" | tr '.' '_').o
  $1 $COMMON $2 -c "$3" -o "$o" || { echo "FAILED: $3"; exit 1; }
}
# SDL_ttf (no HarfBuzz), against FreeType's headers
compile "$CC" "-I$W/ttf -I$W/ttf/external/freetype/include" $W/ttf/SDL_ttf.c
# SDL_mixer: WAV, Ogg Vorbis (stb_vorbis), MP3 (minimp3), FLAC (dr_flac)
MIX="-I$W/mixer/include -I$W/mixer/src -I$W/mixer/src/codecs -DMUSIC_WAV -DMUSIC_OGG -DOGG_USE_STB -DMUSIC_MP3_MINIMP3 -DMUSIC_FLAC_DRFLAC"
for f in $W/mixer/src/*.c $W/mixer/src/codecs/*.c; do compile "$CC" "$MIX" $f; done
# The image loader that stands in for SDL_image
compile "$CC" "" $W/pocket/pocket_image.c
# ONScripter-jh
ONS="-std=c++17 -include $W/pocket/pocket_prefix.h -I$W/src -I$W/mixer/include -I$W/ttf -DMACOSX -DUTF8_FILESYSTEM -DUSE_BUILTIN_EFFECTS -DUSE_BUILTIN_LAYER_EFFECTS -DENABLE_1BYTE_CHAR -DPOCKET_FLOPPY"
for f in $W/work/*.cpp $W/work/builtin_dll/*.cpp $W/pocket/gles_stub.cpp; do
  case $(basename $f) in onscripter_main.cpp|nsaconv.cpp|sarconv.cpp|nsadec.cpp|sardec.cpp|nscriptdecode.cpp|conv_shared.cpp|simple_aviplay.cpp|AVIWrapper.cpp|LUAHandler.cpp) continue;; esac
  compile "$CXX" "$ONS -I$W/work" $f
done
compile "$CXX" "-std=c++17 -I$W/work -I$W/mixer/include -I$W/ttf -DMACOSX -DUTF8_FILESYSTEM -DUSE_BUILTIN_EFFECTS -DUSE_BUILTIN_LAYER_EFFECTS -DENABLE_1BYTE_CHAR -DPOCKET_FLOPPY" $W/pocket/pocket_ons.cpp
rm -f $OUT/libpocketons.a
xcrun --sdk $SDK libtool -static -o $OUT/libpocketons.a $OUT/obj/*.o 2>/dev/null
ls -la $OUT/libpocketons.a
