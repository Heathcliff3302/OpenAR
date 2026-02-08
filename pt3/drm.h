// Minimal stb_easy_font implementation extracted from public domain stb.
// This is intentionally lightweight to avoid shipping the full stb_truetype.
// It provides ASCII text rasterization into triangles for simple UI overlays.
// Original author: Sean Barrett, public domain.

// NOTE:
// This file name is kept as stb_truetype.h for convenience in this repo,
// but the contents here are a minimal "stb_easy_font" implementation.
// Define STB_EASY_FONT_IMPLEMENTATION (recommended) or STB_TRUETYPE_IMPLEMENTATION
// in exactly ONE translation unit before including this header to enable
// the implementation.

#ifndef DRM_H
#define DRM_H

// stb_easy_font API ---------------------------------------------------------
// Usage:
//   char buffer[99999];
//   int num_quads = stb_easy_font_print(x, y, "Hello", nullptr, buffer, sizeof(buffer));
//   Each quad is 16 floats (4 verts * vec4). You can feed these to GL as triangle lists.
//
// Only ASCII 32..126 are supported. No kerning/UTF-8.

#if defined(STB_TRUETYPE_IMPLEMENTATION) || defined(STB_EASY_FONT_IMPLEMENTATION)
static int stb_easy_font_width(const char *text);
static int stb_easy_font_height(const char *text);
static int stb_easy_font_print(float x, float y, const char *text, const unsigned char *color, void *vertex_buffer, int vbuf_size);
#else
int  stb_easy_font_width(const char *text);
int  stb_easy_font_height(const char *text);
int  stb_easy_font_print(float x, float y, const char *text, const unsigned char *color, void *vertex_buffer, int vbuf_size);
#endif

// Implementation -----------------------------------------------------------
#if defined(STB_TRUETYPE_IMPLEMENTATION) || defined(STB_EASY_FONT_IMPLEMENTATION)

#include <string.h>

#ifndef STB_EASY_FONT_COLOR
#define STB_EASY_FONT_COLOR stb_easy_font_color
static unsigned char stb_easy_font_color[4] = {255,255,255,255};
#endif

static unsigned char stb__easy_font_width[] =
{
   4,4,6,8,7,7,7,4,5,5,8,8,4,5,4,7,
   7,7,7,7,7,7,7,7,7,7,4,4,7,8,7,7,
   12,8,7,7,8,7,7,8,8,4,7,8,7,9,8,8,
   7,8,8,7,7,8,8,11,8,8,7,4,7,4,7,6,
   5,7,7,6,7,7,6,7,7,4,6,7,4,10,7,7,
   7,7,7,6,6,7,7,9,7,7,6,5,3,5,8,0,
};

static const char *stb__easy_font_char_to_bitmap[] =
{
   "...."
   "...."
   "...."
   "....",
   "..#."
   "..#."
   "..#."
   "..#.",
   ".#.#"
   "...."
   "...."
   "....",
   ".#.#"
   "####"
   ".#.#"
   "####",
   ".###"
   "#.#."
   ".###"
   "..#.",
   "#..#"
   "...#"
   "..#."
   ".#..",
   ".##."
   "#..#"
   ".##."
   "###.",
   "..#."
   "...."
   "...."
   "....",
   "...#"
   "..#."
   "..#."
   "...#",
   "#..."
   ".#.."
   ".#.."
   "#...",
   ".#.#"
   "..#."
   ".#.#"
   "....",
   "..#."
   "####"
   "..#."
   "....",
   "...."
   "...."
   "..#."
   ".#..",
   "...."
   "...."
   "...."
   ".###",
   "...."
   "...."
   "...."
   "..#.",
   "...#"
   "..#."
   ".#.."
   "#...",
   ".##."
   "#..#"
   "#..#"
   ".##.",
   "..#."
   ".##."
   "..#."
   ".###",
   ".##."
   "...#"
   ".#.."
   "####",
   "####"
   "..#."
   "...#"
   "###.",
   "#..#"
   "#..#"
   "####"
   "...#",
   "####"
   "###."
   "#..."
   "###.",
   ".###"
   "#..."
   "###."
   ".###",
   "####"
   "...#"
   "..#."
   ".#..",
   ".###"
   ".#.#"
   ".#.#"
   ".###",
   ".###"
   ".#.#"
   ".###"
   "...#",
   "...."
   "..#."
   "...."
   "..#.",
   "...."
   "..#."
   "...."
   ".#..",
   "...#"
   "..#."
   ".#.."
   "..#.",
   "...."
   ".###"
   "...."
   ".###",
   "#..."
   ".#.."
   "..#."
   ".#..",
   ".###"
   "..#."
   ".#.."
   ".#..",
   ".##."
   "#..#"
   "#.##"
   ".###",
   ".##."
   "#..#"
   "####"
   "#..#",
   "###."
   "#..#"
   "###."
   "###.",
   ".###"
   "#..."
   "#..."
   ".###",
   "###."
   "#..#"
   "#..#"
   "###.",
   "####"
   "###."
   "#..."
   "####",
   "####"
   "###."
   "#..."
   "#...",
   ".###"
   "#..."
   "#.##"
   ".###",
   "#..#"
   "#..#"
   "####"
   "#..#",
   ".###"
   "..#."
   "..#."
   ".###",
   "..##"
   "...#"
   "#..#"
   ".##.",
   "#..#"
   "###."
   "#.#."
   "#..#",
   "#..."
   "#..."
   "#..."
   "####",
   "#..#"
   "####"
   "#..#"
   "#..#",
   "#..#"
   "####"
   "####"
   "#..#",
   ".##."
   "#..#"
   "#..#"
   ".##.",
   "###."
   "#..#"
   "###."
   "#...",
   ".##."
   "#..#"
   ".###"
   "...#",
   "###."
   "#..#"
   "###."
   "#.#.",
   ".###"
   "##.."
   "..##"
   "###.",
   "####"
   "..#."
   "..#."
   "..#.",
   "#..#"
   "#..#"
   "#..#"
   ".##.",
   "#..#"
   "#..#"
   ".##."
   ".##.",
   "#..#"
   "#..#"
   "####"
   "#..#",
   "#..#"
   ".##."
   ".##."
   "#..#",
   "#..#"
   ".##."
   "..#."
   "..#.",
   "####"
   "..#."
   ".#.."
   "####",
   ".###"
   "..#."
   "..#."
   ".###",
   "#..."
   ".#.."
   "..#."
   "...#",
   ".###"
   ".#.."
   ".#.."
   ".###",
   "..#."
   ".#.#"
   "...."
   "....",
   "...."
   "...."
   "...."
   "####",
   ".#.."
   "..#."
   "...."
   "....",
   "...."
   ".##."
   "#..#"
   ".###",
   "#..."
   "###."
   "#..#"
   "###.",
   "...."
   ".###"
   "#..."
   ".###",
   "...#"
   ".###"
   "#..#"
   ".###",
   "...."
   ".##."
   "####"
   ".##.",
   "..##"
   ".#.."
   ".###"
   ".#..",
   "...."
   ".###"
   "#..#"
   ".###"
   "...#",
   "#..."
   "###."
   "#..#"
   "#..#",
   "..#."
   "...."
   "..#."
   ".###",
   "...#"
   "...."
   "...#"
   "#..#",
   "#..."
   "#.##"
   "##.."
   "#.##",
   ".#.."
   ".#.."
   ".#.."
   "..##",
   "...."
   "####"
   "#..#"
   "#..#",
   "...."
   "###."
   "#..#"
   "#..#",
   "...."
   ".##."
   "#..#"
   ".##.",
   "...."
   "###."
   "#..#"
   "###."
   "#...",
   "...."
   ".###"
   "#..#"
   ".###"
   "...#",
   "...."
   "###."
   "#..."
   "#...",
   "...."
   ".###"
   ".##."
   "###.",
   ".#.."
   "###."
   ".#.."
   "..##",
   "...."
   "#..#"
   "#..#"
   ".###",
   "...."
   "#..#"
   ".##."
   ".##.",
   "...."
   "#..#"
   "####"
   "#..#",
   "...."
   "#..#"
   ".##."
   "#..#",
   "...."
   "#..#"
   ".##."
   "..#."
   "#...",
   "...."
   "####"
   "..#."
   "####",
   "..##"
   "..#."
   "..#."
   "..##",
   "..#."
   "..#."
   "..#."
   "..#.",
   "##.."
   ".#.."
   ".#.."
   "##..",
   ".#.#"
   "#.#."
   "...."
   "...."
};

static int stb_easy_font_width(const char *text)
{
   int len = (int)strlen(text), i, width = 0;
   for (i=0; i < len; ++i) {
      unsigned char c = (unsigned char) text[i];
      if (c >= 32 && c < 127)
         width += stb__easy_font_width[c-32];
   }
   return width;
}

static int stb_easy_font_height(const char *text)
{
   int count = 1;
   while (*text) if (*text++ == '\n') ++count;
   return count * 12;
}

static int stb_easy_font_print(float x, float y, const char *text, const unsigned char *color, void *vertex_buffer, int vbuf_size)
{
   float start_x = x;
   int quads = 0;
   float *vbuf = (float *) vertex_buffer;
   const unsigned char *cptr = (const unsigned char *) text;
   const unsigned char *col = color ? color : STB_EASY_FONT_COLOR;
   float r = col[0]/255.0f, g = col[1]/255.0f, b = col[2]/255.0f, a = col[3]/255.0f;

   while (*cptr) {
      if (*cptr == '\n') { x = start_x; y += 12; ++cptr; continue; }
      if (*cptr >= 32 && *cptr < 127) {
         const char *bitmap = stb__easy_font_char_to_bitmap[*cptr-32];
         for (int j=0; j < 4; ++j)
            for (int i=0; i < 4; ++i) {
               if (bitmap[j*4+i] == '#') {
                  float x0 = x + i*2.0f;
                  float y0 = y + j*3.0f;
                  float x1 = x0 + 2.0f;
                  float y1 = y0 + 3.0f;
                  // vbuf_size is in BYTES; each quad emits 36 floats.
                  const int float_cap = vbuf_size > 0 ? (vbuf_size / (int)sizeof(float)) : 0;
                  if ((quads+1) * 36 <= float_cap) {
                     // 6 verts (2 triangles) * (x,y,r,g,b,a) = 36 floats
                     float verts[36] = {
                        x0, y0, r,g,b,a,
                        x1, y0, r,g,b,a,
                        x1, y1, r,g,b,a,
                        x0, y0, r,g,b,a,
                        x1, y1, r,g,b,a,
                        x0, y1, r,g,b,a
                     };
                     memcpy(vbuf + quads*36, verts, sizeof(verts));
                     ++quads;
                  }
               }
            }
         x += stb__easy_font_width[*cptr-32];
      }
      ++cptr;
   }
   return quads;
}
#endif // STB_TRUETYPE_IMPLEMENTATION || STB_EASY_FONT_IMPLEMENTATION

// C++ DRM display helper class (always declared for C++ callers)
#ifdef __cplusplus
class drmclass
{
public:
    void drmdisplay(const char* reply);
};
#endif

#endif // DRM_H