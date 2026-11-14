/* The engine's optional sharpening pass draws with OpenGL ES. Here pictures are drawn in software and
   handed to the app, so that pass is never switched on; these empty bodies only satisfy the linker. */
#include "renderer/gles_renderer.h"
GlesRenderer::GlesRenderer(SDL_Window *window, SDL_Texture *texture, const float input_size[2], const float output_size[2], float sharpness) : window(window), texture(texture), context(NULL) {}
GlesRenderer::~GlesRenderer() {}
GLuint GlesRenderer::createShader(GLenum shader_type, const GLchar *shader_src) { return 0; }
void GlesRenderer::initVertexData() {}
void GlesRenderer::copy(const int window_x, const int window_y) {}
void GlesRenderer::setConstBuffer(const float input_size[2], const float output_size[2], float sharpness) {}
void GlesRenderer::pause() {}
void GlesRenderer::resume() {}
