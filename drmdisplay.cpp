// Simple EGL+GBM+DRM sample to draw a string overlay on RK3588.
// Assumptions:
//   - Connector ID: 208
//   - CRTC ID: 71 (already configured by bootloader/OS; immutable)
//   - Plane ID: 57 (used for overlay; we only set plane, not CRTC)
//   - Uses GBM/EGL/GLES2 for rendering, then drmModeSetPlane for display.
// Build example (needs libdrm, gbm, egl, glesv2, freetype):
//   g++ textscreen.cpp -o textscreen -std=c++17 -ldrm -lgbm -lEGL -lGLESv2 -lfreetype

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#include <xf86drm.h>
#include <xf86drmMode.h>
#include <gbm.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include "drm.h"

static const uint32_t kConnectorId = 208;
static const uint32_t kCrtcId = 71;
static const uint32_t kPlaneId = 57;
static const char* kText = "你好，世界";
static const char* kFontPath = "/usr/share/fonts/NotoSansCJK-Regular.ttc";
static const int kFontSize = 24;

static void fatal(const char* msg) {
  perror(msg);
  exit(EXIT_FAILURE);
}

// Legacy drmModeSetPlane: Rockchip DRM on RK3588 does not expose atomic plane
// properties (FB_ID, CRTC_ID, etc.), so use the legacy API.
// drmModeSetPlane(fd, plane_id, crtc_id, fb_id, flags, crtc_x, crtc_y, crtc_w, crtc_h,
//                 src_x, src_y, src_w, src_h) with src in 16.16 fixed point.
static void set_plane_legacy(int fd, uint32_t plane_id, uint32_t crtc_id, uint32_t fb_id,
                            uint32_t w, uint32_t h) {
  int ret = drmModeSetPlane(fd, plane_id, crtc_id, fb_id, 0,
                            0, 0,                    /* crtc_x, crtc_y: top-left */
                            w, h,                    /* crtc_w, crtc_h */
                            0, 0,                    /* src_x, src_y */
                            (uint32_t)w << 16, (uint32_t)h << 16);  /* src_w, src_h in 16.16 */
  if (ret != 0) {
    fprintf(stderr, "drmModeSetPlane failed: %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }
}

static GLuint compile_shader(GLenum type, const char* src) {
  GLuint s = glCreateShader(type);
  glShaderSource(s, 1, &src, nullptr);
  glCompileShader(s);
  GLint ok;
  glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
  if (!ok) {
    char log[512];
    glGetShaderInfoLog(s, sizeof(log), nullptr, log);
    fprintf(stderr, "shader compile error: %s\n", log);
    fatal("compile shader");
  }
  return s;
}

static GLuint build_program() {
  const char* vs = R"(attribute vec2 aPos;
attribute vec2 aTexCoord;
varying vec2 vTexCoord;
uniform vec2 uViewport;
void main() {
  vec2 ndc = (aPos / uViewport) * 2.0 - 1.0;
  gl_Position = vec4(ndc.x, -ndc.y, 0.0, 1.0);
  vTexCoord = aTexCoord;
})";
  const char* fs = R"(precision mediump float;
varying vec2 vTexCoord;
uniform sampler2D uTex;
void main() {
  float a = texture2D(uTex, vTexCoord).a;
  gl_FragColor = vec4(1.0, 1.0, 1.0, a);
})";
  GLuint prog = glCreateProgram();
  GLuint v = compile_shader(GL_VERTEX_SHADER, vs);
  GLuint f = compile_shader(GL_FRAGMENT_SHADER, fs);
  glAttachShader(prog, v);
  glAttachShader(prog, f);
  glLinkProgram(prog);
  GLint ok;
  glGetProgramiv(prog, GL_LINK_STATUS, &ok);
  if (!ok) fatal("link program");
  glDeleteShader(v);
  glDeleteShader(f);
  return prog;
}

// Decode next UTF-8 codepoint; advances *s. Returns 0 at end or on error.
static unsigned int utf8_next(const char** s) {
  const unsigned char* p = (const unsigned char*)*s;
  unsigned char c = *p++;
  if (c == 0) { *s = (const char*)p; return 0; }
  *s = (const char*)p;
  if (c < 0x80) return c;
  if ((c & 0xE0) == 0xC0) {
    unsigned int u = (c & 0x1F);
    u = (u << 6) | (*p++ & 0x3F);
    *s = (const char*)p;
    return u;
  }
  if ((c & 0xF0) == 0xE0) {
    unsigned int u = (c & 0x0F);
    u = (u << 6) | (*p++ & 0x3F);
    u = (u << 6) | (*p++ & 0x3F);
    *s = (const char*)p;
    return u;
  }
  if ((c & 0xF8) == 0xF0) {
    unsigned int u = (c & 0x07);
    u = (u << 6) | (*p++ & 0x3F);
    u = (u << 6) | (*p++ & 0x3F);
    u = (u << 6) | (*p++ & 0x3F);
    *s = (const char*)p;
    return u;
  }
  return 0;
}

// Render UTF-8 text with FreeType; 8-bit grayscale used as alpha. Requires
// glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
static void render_utf8(FT_Face face, GLuint prog, GLuint vbo, GLint locPos, GLint locTex,
                        GLint locViewport, const char* text, float start_x, float start_y,
                        float vp_w, float vp_h) {
  glUseProgram(prog);
  glUniform2f(locViewport, vp_w, vp_h);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  float pen_x = start_x, pen_y = start_y;
  const float line_height = (float)(face->size->metrics.height >> 6);
  const char* p = text;

  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glEnableVertexAttribArray(locPos);
  glEnableVertexAttribArray(locTex);

  for (;;) {
    unsigned int cp = utf8_next(&p);
    if (cp == 0) break;
    if (cp == '\n') {
      pen_x = start_x;
      pen_y += line_height;
      continue;
    }
    if (FT_Load_Char(face, cp, FT_LOAD_RENDER) != 0) continue;

    FT_GlyphSlot slot = face->glyph;
    FT_Bitmap* b = &slot->bitmap;
    unsigned int w = b->width, h = b->rows;
    if (w == 0 || h == 0) {
      pen_x += (float)(slot->advance.x >> 6);
      continue;
    }

    // Copy 8-bit grayscale to RGBA: R=G=B=255, A=gray (alpha from FreeType)
    size_t size = (size_t)w * (size_t)h * 4;
    unsigned char* buf = (unsigned char*)malloc(size);
    if (!buf) continue;
    for (unsigned int row = 0; row < h; row++) {
      const unsigned char* src = b->buffer + row * b->pitch;
      unsigned char* dst = buf + row * w * 4;
      for (unsigned int i = 0; i < w; i++) {
        dst[i*4+0] = 255;
        dst[i*4+1] = 255;
        dst[i*4+2] = 255;
        dst[i*4+3] = src[i];
      }
    }

    GLuint tex;
    glGenTextures(1, &tex);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (GLsizei)w, (GLsizei)h, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, buf);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    free(buf);

    float x0 = pen_x + (float)slot->bitmap_left;
    float y0 = pen_y - (float)slot->bitmap_top;
    float x1 = x0 + (float)w, y1 = y0 + (float)h;
    // Quad: (x0,y0),(x1,y0),(x1,y1),(x0,y0),(x1,y1),(x0,y1); tex (0,1),(1,1),(1,0),(0,1),(1,0),(0,0)
    float verts[24] = {
      x0, y0, 0.f, 0.f,   x1, y0, 1.f, 0.f,   x1, y1, 1.f, 1.f,
      x0, y0, 0.f, 0.f,   x1, y1, 1.f, 1.f,   x0, y1, 0.f, 1.f
    };
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STREAM_DRAW);
    glVertexAttribPointer(locPos, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glVertexAttribPointer(locTex, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glDrawArrays(GL_TRIANGLES, 0, 6);

    glDeleteTextures(1, &tex);
    pen_x += (float)(slot->advance.x >> 6);
  }

  glDisableVertexAttribArray(locPos);
  glDisableVertexAttribArray(locTex);
}

static drmModeModeInfo pick_mode(const drmModeConnector* conn) {
  // Prefer DRM_MODE_TYPE_PREFERRED if present, else first mode.
  if (!conn || conn->count_modes <= 0) {
    fprintf(stderr, "Connector has no modes\n");
    exit(EXIT_FAILURE);
  }
  for (int i = 0; i < conn->count_modes; ++i) {
    if (conn->modes[i].type & DRM_MODE_TYPE_PREFERRED) return conn->modes[i];
  }
  return conn->modes[0];
}

static EGLDisplay get_egl_display_for_gbm(gbm_device* gbm) {
  // Prefer eglGetPlatformDisplayEXT(EGL_PLATFORM_GBM_KHR, ...) on KMS/GBM stacks.
  PFNEGLGETPLATFORMDISPLAYEXTPROC getPlatformDisplay =
      (PFNEGLGETPLATFORMDISPLAYEXTPROC)eglGetProcAddress("eglGetPlatformDisplayEXT");
  if (getPlatformDisplay) {
    EGLDisplay d = getPlatformDisplay(EGL_PLATFORM_GBM_KHR, (void*)gbm, nullptr);
    if (d != EGL_NO_DISPLAY) return d;
  }
  // Fallback: may work on some stacks.
  return eglGetDisplay((EGLNativeDisplayType)gbm);
}

void drmclass::drmdisplay(const char* reply) {
  int drm_fd = open("/dev/dri/card0", O_RDWR | O_CLOEXEC);
  if (drm_fd < 0) fatal("open card0");

  drmModeRes* res = drmModeGetResources(drm_fd);
  if (!res) fatal("drmModeGetResources");

  drmModeConnector* conn = drmModeGetConnector(drm_fd, kConnectorId);
  if (!conn) fatal("drmModeGetConnector");
  if (conn->connection != DRM_MODE_CONNECTED) fatal("connector not connected");

  drmModeModeInfo mode = pick_mode(conn);
  uint32_t width = mode.hdisplay;
  uint32_t height = mode.vdisplay;

  // GBM setup
  gbm_device* gbm = gbm_create_device(drm_fd);
  if (!gbm) fatal("gbm_create_device");
  gbm_surface* surface = gbm_surface_create(gbm, width, height,
                                            GBM_FORMAT_XRGB8888,
                                            GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING);
  if (!surface) fatal("gbm_surface_create");

  // EGL setup
  EGLDisplay dpy = get_egl_display_for_gbm(gbm);
  if (dpy == EGL_NO_DISPLAY) fatal("eglGetDisplay");
  if (!eglInitialize(dpy, nullptr, nullptr)) fatal("eglInitialize");

  const EGLint cfg_attribs[] = {
      EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
      EGL_RED_SIZE, 8,
      EGL_GREEN_SIZE, 8,
      EGL_BLUE_SIZE, 8,
      EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
      EGL_NONE};
  EGLConfig cfg;
  EGLint ncfg;
  if (!eglChooseConfig(dpy, cfg_attribs, &cfg, 1, &ncfg) || ncfg != 1) fatal("eglChooseConfig");

  EGLint ctx_attribs[] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};
  EGLContext ctx = eglCreateContext(dpy, cfg, EGL_NO_CONTEXT, ctx_attribs);
  if (ctx == EGL_NO_CONTEXT) fatal("eglCreateContext");

  EGLSurface egl_surf = eglCreateWindowSurface(dpy, cfg, surface, nullptr);
  if (egl_surf == EGL_NO_SURFACE) fatal("eglCreateWindowSurface");

  if (!eglMakeCurrent(dpy, egl_surf, egl_surf, ctx)) fatal("eglMakeCurrent");

  glViewport(0, 0, width, height);
  glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);

  FT_Library ft;
  if (FT_Init_FreeType(&ft) != 0) {
    fprintf(stderr, "FT_Init_FreeType failed\n");
    exit(EXIT_FAILURE);
  }
  FT_Face face;
  if (FT_New_Face(ft, kFontPath, 0, &face) != 0) {
    fprintf(stderr, "Cannot load font: %s\n", kFontPath);
    FT_Done_FreeType(ft);
    exit(EXIT_FAILURE);
  }
  FT_Set_Pixel_Sizes(face, 0, (FT_UInt)kFontSize);

  GLuint prog = build_program();
  GLint locPos = glGetAttribLocation(prog, "aPos");
  GLint locTex = glGetAttribLocation(prog, "aTexCoord");
  GLint locViewport = glGetUniformLocation(prog, "uViewport");

  GLuint vbo;
  glGenBuffers(1, &vbo);
  render_utf8(face, prog, vbo, locPos, locTex, locViewport,
              reply, 40.0f, 80.0f, (float)width, (float)height);

  FT_Done_Face(face);
  FT_Done_FreeType(ft);
  glDeleteBuffers(1, &vbo);

  eglSwapBuffers(dpy, egl_surf);

  // Export buffer to DRM fb
  gbm_bo* bo = gbm_surface_lock_front_buffer(surface);
  if (!bo) fatal("gbm_surface_lock_front_buffer");

  uint32_t handle = gbm_bo_get_handle(bo).u32;
  uint32_t stride = gbm_bo_get_stride(bo);
  uint32_t fb_id;
  uint32_t handles[4] = {handle};
  uint32_t strides[4] = {stride};
  uint32_t offsets[4] = {0};
  if (drmModeAddFB2(drm_fd, width, height, GBM_FORMAT_XRGB8888,
                    handles, strides, offsets, &fb_id, 0) != 0) {
    fatal("drmModeAddFB2");
  }

  set_plane_legacy(drm_fd, kPlaneId, kCrtcId, fb_id, width, height);

  printf("Text pushed to plane %u on CRTC %u via connector %u\n",
         kPlaneId, kCrtcId, kConnectorId);
  sleep(5); // keep on screen briefly

  drmModeRmFB(drm_fd, fb_id);
  gbm_surface_release_buffer(surface, bo);
  glDeleteProgram(prog);
  eglDestroySurface(dpy, egl_surf);
  eglDestroyContext(dpy, ctx);
  eglTerminate(dpy);
  gbm_surface_destroy(surface);
  gbm_device_destroy(gbm);
  drmModeFreeConnector(conn);
  drmModeFreeResources(res);
  close(drm_fd);

  //system("echo 1 > /sys/class/vtconsole/vtcon1/bind");
}
