// =============================================================================
// WHITEBOARD APP - Drawing application for SPANE Engine (No math lib needed)
// =============================================================================
// Compile: gcc -shared -fPIC -O3 -o whiteboard.so whiteboard.c
// =============================================================================

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// SDK types (must match engine)
#define MAIN_WINDOW_WIDTH  1000
#define MAIN_WINDOW_HEIGHT 700
#define GAME_AREA_WIDTH    800
#define GAME_AREA_HEIGHT   600
#define GAME_AREA_X        180
#define GAME_AREA_Y        50

typedef struct Framebuffer {
    unsigned char pixels[MAIN_WINDOW_WIDTH * MAIN_WINDOW_HEIGHT * 4];
} Framebuffer;

typedef struct Game {
    char name[64];
    char path[512];
    void* data;
    void* handle;
    int active;
    void (*init)(struct Game* game);
    void (*handle_key)(struct Game* game, int key_code, int pressed);
    void (*handle_click)(struct Game* game, int x, int y);
    void (*update)(struct Game* game);
    void (*render)(struct Game* game, Framebuffer* fb);
    void (*cleanup)(struct Game* game);
} Game;

// Font data
static const unsigned char font_5x7[][5] = {
    {0x00,0x00,0x00,0x00,0x00},{0x00,0x00,0x5f,0x00,0x00},{0x00,0x07,0x00,0x07,0x00},
    {0x14,0x7f,0x14,0x7f,0x14},{0x24,0x2a,0x7f,0x2a,0x12},{0x23,0x13,0x08,0x64,0x62},
    {0x36,0x49,0x55,0x22,0x50},{0x00,0x05,0x03,0x00,0x00},{0x00,0x1c,0x22,0x41,0x00},
    {0x00,0x41,0x22,0x1c,0x00},{0x08,0x2a,0x1c,0x2a,0x08},{0x08,0x08,0x3e,0x08,0x08},
    {0x00,0x50,0x30,0x00,0x00},{0x08,0x08,0x08,0x08,0x08},{0x00,0x60,0x60,0x00,0x00},
    {0x20,0x10,0x08,0x04,0x02},{0x3e,0x51,0x49,0x45,0x3e},{0x00,0x42,0x7f,0x40,0x00},
    {0x42,0x61,0x51,0x49,0x46},{0x21,0x41,0x45,0x4b,0x31},{0x18,0x14,0x12,0x7f,0x10},
    {0x27,0x45,0x45,0x45,0x39},{0x3c,0x4a,0x49,0x49,0x30},{0x01,0x71,0x09,0x05,0x03},
    {0x36,0x49,0x49,0x49,0x36},{0x06,0x49,0x49,0x29,0x1e},{0x00,0x36,0x36,0x00,0x00},
    {0x00,0x56,0x36,0x00,0x00},{0x00,0x08,0x14,0x22,0x41},{0x14,0x14,0x14,0x14,0x14},
    {0x41,0x22,0x14,0x08,0x00},{0x02,0x01,0x51,0x09,0x06},{0x32,0x49,0x79,0x41,0x3e},
    {0x7e,0x11,0x11,0x11,0x7e},{0x7f,0x49,0x49,0x49,0x36},{0x3e,0x41,0x41,0x41,0x22},
    {0x7f,0x41,0x41,0x22,0x1c},{0x7f,0x49,0x49,0x49,0x41},{0x7f,0x09,0x09,0x01,0x01},
    {0x3e,0x41,0x41,0x51,0x32},{0x7f,0x08,0x08,0x08,0x7f},{0x00,0x41,0x7f,0x41,0x00},
    {0x20,0x40,0x41,0x3f,0x01},{0x7f,0x08,0x14,0x22,0x41},{0x7f,0x40,0x40,0x40,0x40},
    {0x7f,0x02,0x04,0x02,0x7f},{0x7f,0x04,0x08,0x10,0x7f},{0x3e,0x41,0x41,0x41,0x3e},
    {0x7f,0x09,0x09,0x09,0x06},{0x3e,0x41,0x51,0x21,0x5e},{0x7f,0x09,0x19,0x29,0x46},
    {0x46,0x49,0x49,0x49,0x31},{0x01,0x01,0x7f,0x01,0x01},{0x3f,0x40,0x40,0x40,0x3f},
    {0x1f,0x20,0x40,0x20,0x1f},{0x7f,0x20,0x18,0x20,0x7f},{0x63,0x14,0x08,0x14,0x63},
    {0x03,0x04,0x78,0x04,0x03},{0x61,0x51,0x49,0x45,0x43},{0x00,0x00,0x7f,0x41,0x41},
    {0x02,0x04,0x08,0x10,0x20},{0x41,0x41,0x7f,0x00,0x00},{0x04,0x02,0x01,0x02,0x04},
    {0x40,0x40,0x40,0x40,0x40},{0x00,0x01,0x02,0x04,0x00},{0x20,0x54,0x54,0x54,0x78},
    {0x7f,0x48,0x44,0x44,0x38},{0x38,0x44,0x44,0x44,0x20},{0x38,0x44,0x44,0x48,0x7f},
    {0x38,0x54,0x54,0x54,0x18},{0x08,0x7e,0x09,0x01,0x02},{0x08,0x14,0x54,0x54,0x3c},
    {0x7f,0x08,0x04,0x04,0x78},{0x00,0x44,0x7d,0x40,0x00},{0x20,0x40,0x44,0x3d,0x00},
    {0x00,0x7f,0x10,0x28,0x44},{0x00,0x41,0x7f,0x40,0x00},{0x7c,0x04,0x18,0x04,0x78},
    {0x7c,0x08,0x04,0x04,0x78},{0x38,0x44,0x44,0x44,0x38},{0x7c,0x14,0x14,0x14,0x08},
    {0x08,0x14,0x14,0x18,0x7c},{0x7c,0x08,0x04,0x04,0x08},{0x48,0x54,0x54,0x54,0x20},
    {0x04,0x3f,0x44,0x40,0x20},{0x3c,0x40,0x40,0x20,0x7c},{0x1c,0x20,0x40,0x20,0x1c},
    {0x3c,0x40,0x30,0x40,0x3c},{0x44,0x28,0x10,0x28,0x44},{0x0c,0x50,0x50,0x50,0x3c},
    {0x44,0x64,0x54,0x4c,0x44},{0x00,0x08,0x36,0x41,0x00},{0x00,0x00,0x7f,0x00,0x00},
    {0x00,0x41,0x36,0x08,0x00},{0x08,0x08,0x2a,0x1c,0x08}
};

// Drawing primitives
static inline void fb_pixel(Framebuffer* fb, int x, int y, unsigned char r, unsigned char g, unsigned char b) {
    if (x < GAME_AREA_X || x >= GAME_AREA_X+GAME_AREA_WIDTH || 
        y < GAME_AREA_Y || y >= GAME_AREA_Y+GAME_AREA_HEIGHT) return;
    int i = (y * MAIN_WINDOW_WIDTH + x) * 4;
    fb->pixels[i]=r; fb->pixels[i+1]=g; fb->pixels[i+2]=b; fb->pixels[i+3]=255;
}

static void fb_fill(Framebuffer* fb, int x, int y, int w, int h, unsigned char r, unsigned char g, unsigned char b) {
    for (int dy=0; dy<h; dy++)
        for (int dx=0; dx<w; dx++)
            fb_pixel(fb, x+dx, y+dy, r, g, b);
}

static void fb_char(Framebuffer* fb, int x, int y, char c, unsigned char r, unsigned char g, unsigned char b) {
    if (c<32||c>126) return;
    const unsigned char* glyph = font_5x7[c-32];
    for (int row=0; row<7; row++)
        for (int col=0; col<5; col++)
            if (glyph[col] & (1<<row)) fb_pixel(fb, x+col, y+row, r, g, b);
}

static void fb_text(Framebuffer* fb, int x, int y, const char* s, unsigned char r, unsigned char g, unsigned char b) {
    for (int i=0; s[i]; i++) fb_char(fb, x+i*6, y, s[i], r, g, b);
}

static void fb_line(Framebuffer* fb, int x0, int y0, int x1, int y1, unsigned char r, unsigned char g, unsigned char b) {
    int dx = abs(x1-x0), sx = x0<x1 ? 1 : -1;
    int dy = -abs(y1-y0), sy = y0<y1 ? 1 : -1;
    int err = dx+dy, e2;
    while(1) {
        fb_pixel(fb, x0, y0, r, g, b);
        if (x0==x1 && y0==y1) break;
        e2 = 2*err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

// Simple integer square root using Newton's method (avoids math library)
static int int_sqrt(int n) {
    if (n < 0) return 0;
    int x = n;
    int y = (x + 1) / 2;
    while (y < x) {
        x = y;
        y = (x + n / x) / 2;
    }
    return x;
}

static void fb_circle_outline(Framebuffer* fb, int cx, int cy, int radius, unsigned char r, unsigned char g, unsigned char b) {
    int x = radius, y = 0, err = 0;
    while (x >= y) {
        fb_pixel(fb, cx+x, cy+y, r, g, b); fb_pixel(fb, cx+y, cy+x, r, g, b);
        fb_pixel(fb, cx-y, cy+x, r, g, b); fb_pixel(fb, cx-x, cy+y, r, g, b);
        fb_pixel(fb, cx-x, cy-y, r, g, b); fb_pixel(fb, cx-y, cy-x, r, g, b);
        fb_pixel(fb, cx+y, cy-x, r, g, b); fb_pixel(fb, cx+x, cy-y, r, g, b);
        if (err <= 0) { y += 1; err += 2*y + 1; }
        if (err > 0) { x -= 1; err -= 2*x + 1; }
    }
}

// =============================================================================
// WHITEBOARD APP DATA
// =============================================================================

#define MAX_LAYERS 3
#define TOOL_PEN 0
#define TOOL_LINE 1
#define TOOL_RECT 2
#define TOOL_CIRCLE 3
#define TOOL_ERASER 4

typedef struct {
    int tool;
    int drawing;
    int mx, my;
    int last_mx, last_my;
    int start_x, start_y;
    
    // Color
    unsigned char cr, cg, cb;
    
    // Canvas layers
    unsigned char layers[MAX_LAYERS][GAME_AREA_WIDTH * GAME_AREA_HEIGHT * 4];
    int current_layer;
    int layer_visible[MAX_LAYERS];
    
    // UI state
    int show_help;
    int pen_size;
} WhiteboardData;

// =============================================================================
// WHITEBOARD FUNCTIONS
// =============================================================================

static void wb_clear_layer(WhiteboardData* wb, int layer) {
    memset(wb->layers[layer], 0xFF, GAME_AREA_WIDTH * GAME_AREA_HEIGHT * 4);
}

static void wb_draw_pixel_to_layer(WhiteboardData* wb, int x, int y, unsigned char r, unsigned char g, unsigned char b) {
    if (x < 0 || x >= GAME_AREA_WIDTH || y < 0 || y >= GAME_AREA_HEIGHT) return;
    int i = (y * GAME_AREA_WIDTH + x) * 4;
    wb->layers[wb->current_layer][i] = r;
    wb->layers[wb->current_layer][i+1] = g;
    wb->layers[wb->current_layer][i+2] = b;
    wb->layers[wb->current_layer][i+3] = 255;
}

static void wb_draw_thick_pixel(WhiteboardData* wb, int x, int y, unsigned char r, unsigned char g, unsigned char b) {
    int s = wb->pen_size;
    for (int dy = -s; dy <= s; dy++)
        for (int dx = -s; dx <= s; dx++)
            if (dx*dx + dy*dy <= s*s)
                wb_draw_pixel_to_layer(wb, x+dx, y+dy, r, g, b);
}

static void wb_draw_line_to_layer(WhiteboardData* wb, int x0, int y0, int x1, int y1, unsigned char r, unsigned char g, unsigned char b) {
    int dx = abs(x1-x0), sx = x0<x1 ? 1 : -1;
    int dy = -abs(y1-y0), sy = y0<y1 ? 1 : -1;
    int err = dx+dy, e2;
    while(1) {
        wb_draw_thick_pixel(wb, x0, y0, r, g, b);
        if (x0==x1 && y0==y1) break;
        e2 = 2*err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

static void wb_draw_rect_to_layer(WhiteboardData* wb, int x, int y, int w, int h, unsigned char r, unsigned char g, unsigned char b) {
    for (int dx = 0; dx < w; dx++) {
        wb_draw_thick_pixel(wb, x+dx, y, r, g, b);
        wb_draw_thick_pixel(wb, x+dx, y+h-1, r, g, b);
    }
    for (int dy = 0; dy < h; dy++) {
        wb_draw_thick_pixel(wb, x, y+dy, r, g, b);
        wb_draw_thick_pixel(wb, x+w-1, y+dy, r, g, b);
    }
}

static void wb_draw_circle_to_layer(WhiteboardData* wb, int cx, int cy, int radius, unsigned char r, unsigned char g, unsigned char b) {
    int x = radius, y = 0, err = 0;
    while (x >= y) {
        wb_draw_thick_pixel(wb, cx+x, cy+y, r, g, b);
        wb_draw_thick_pixel(wb, cx+y, cy+x, r, g, b);
        wb_draw_thick_pixel(wb, cx-y, cy+x, r, g, b);
        wb_draw_thick_pixel(wb, cx-x, cy+y, r, g, b);
        wb_draw_thick_pixel(wb, cx-x, cy-y, r, g, b);
        wb_draw_thick_pixel(wb, cx-y, cy-x, r, g, b);
        wb_draw_thick_pixel(wb, cx+y, cy-x, r, g, b);
        wb_draw_thick_pixel(wb, cx+x, cy-y, r, g, b);
        if (err <= 0) { y += 1; err += 2*y + 1; }
        if (err > 0) { x -= 1; err -= 2*x + 1; }
    }
}

// =============================================================================
// APP INITIALIZATION
// =============================================================================

static void wb_init(Game* game) {
    WhiteboardData* wb = calloc(1, sizeof(WhiteboardData));
    
    wb->tool = TOOL_PEN;
    wb->cr = 0x00; wb->cg = 0x00; wb->cb = 0x00;
    wb->current_layer = 0;
    wb->pen_size = 2;
    
    // Initialize layers as white
    for (int i = 0; i < MAX_LAYERS; i++) {
        wb_clear_layer(wb, i);
        wb->layer_visible[i] = 1;
    }
    
    game->data = wb;
    printf("Whiteboard App initialized\n");
    printf("Tools: 1=Pen 2=Line 3=Rectangle 4=Circle 5=Eraser\n");
    printf("Colors: R=Red G=Green B=Blue K=Black W=White Y=Yellow C=Cyan M=Magenta\n");
    printf("Layers: [=Prev ]=Next 0/1/2=Select Layer\n");
    printf("Actions: X=Clear Layer H=Help V=Toggle Visibility +/-=Brush Size\n");
}

// =============================================================================
// INPUT HANDLING
// =============================================================================

static void wb_handle_key(Game* game, int key_code, int pressed) {
    if (!pressed) return;
    WhiteboardData* wb = game->data;
    
    switch(key_code) {
        // Tool selection
        case 49: wb->tool = TOOL_PEN; break;          // 1
        case 50: wb->tool = TOOL_LINE; break;         // 2
        case 51: wb->tool = TOOL_RECT; break;         // 3
        case 52: wb->tool = TOOL_CIRCLE; break;       // 4
        case 53: wb->tool = TOOL_ERASER; break;       // 5
        
        // Color selection
        case 82: wb->cr=0xFF; wb->cg=0x00; wb->cb=0x00; break; // R - Red
        case 71: wb->cr=0x00; wb->cg=0xFF; wb->cb=0x00; break; // G - Green
        case 66: wb->cr=0x00; wb->cg=0x00; wb->cb=0xFF; break; // B - Blue
        case 75: wb->cr=0x00; wb->cg=0x00; wb->cb=0x00; break; // K - Black
        case 87: wb->cr=0xFF; wb->cg=0xFF; wb->cb=0xFF; break; // W - White
        case 89: wb->cr=0xFF; wb->cg=0xFF; wb->cb=0x00; break; // Y - Yellow
        case 77: wb->cr=0xFF; wb->cg=0x00; wb->cb=0xFF; break; // M - Magenta
        case 67: wb->cr=0x00; wb->cg=0xFF; wb->cb=0xFF; break; // C - Cyan
        
        // Layer management
        case 91: // [
            wb->current_layer = (wb->current_layer - 1 + MAX_LAYERS) % MAX_LAYERS;
            break;
        case 93: // ]
            wb->current_layer = (wb->current_layer + 1) % MAX_LAYERS;
            break;
        case 48: wb->current_layer = 0; break; // 0
        case 57: wb->current_layer = 2; break; // 9 (as layer 2)
        
        // Actions
        case 72: wb->show_help = !wb->show_help; break; // H
        case 88: // X - clear current layer
            wb_clear_layer(wb, wb->current_layer);
            break;
        case 61: // +
            if (wb->pen_size < 20) wb->pen_size++;
            break;
        case 45: // -
            if (wb->pen_size > 1) wb->pen_size--;
            break;
        case 86: // V - toggle layer visibility
            wb->layer_visible[wb->current_layer] = !wb->layer_visible[wb->current_layer];
            break;
    }
}

static void wb_handle_click(Game* game, int x, int y) {
    WhiteboardData* wb = game->data;
    
    // Convert to canvas coordinates
    int cx = x - GAME_AREA_X;
    int cy = y - GAME_AREA_Y;
    
    // Check if click is within canvas
    if (cx >= 0 && cx < GAME_AREA_WIDTH && cy >= 0 && cy < GAME_AREA_HEIGHT) {
        if (!wb->drawing) {
            // Start drawing
            wb->drawing = 1;
            wb->start_x = cx;
            wb->start_y = cy;
            wb->last_mx = cx;
            wb->last_my = cy;
            wb->mx = cx;
            wb->my = cy;
            
            // For pen and eraser, draw a point immediately
            if (wb->tool == TOOL_PEN || wb->tool == TOOL_ERASER) {
                unsigned char r, g, b;
                if (wb->tool == TOOL_ERASER) {
                    r = 0xFF; g = 0xFF; b = 0xFF;
                } else {
                    r = wb->cr; g = wb->cg; b = wb->cb;
                }
                wb_draw_thick_pixel(wb, cx, cy, r, g, b);
            }
        }
    }
}

static void wb_update(Game* game) {
    WhiteboardData* wb = game->data;
    
    if (wb->drawing) {
        int cx = wb->mx;
        int cy = wb->my;
        
        if (cx >= 0 && cx < GAME_AREA_WIDTH && cy >= 0 && cy < GAME_AREA_HEIGHT) {
            unsigned char r, g, b;
            if (wb->tool == TOOL_ERASER) {
                r = 0xFF; g = 0xFF; b = 0xFF;
            } else {
                r = wb->cr; g = wb->cg; b = wb->cb;
            }
            
            switch(wb->tool) {
                case TOOL_PEN:
                case TOOL_ERASER:
                    if (cx != wb->last_mx || cy != wb->last_my) {
                        wb_draw_line_to_layer(wb, wb->last_mx, wb->last_my, cx, cy, r, g, b);
                        wb->last_mx = cx;
                        wb->last_my = cy;
                    }
                    break;
                    
                case TOOL_LINE:
                case TOOL_RECT:
                case TOOL_CIRCLE:
                    // Preview only, actual drawing on mouse release
                    break;
            }
        }
    }
}

// =============================================================================
// RENDERING
// =============================================================================

static void wb_render(Game* game, Framebuffer* fb) {
    WhiteboardData* wb = game->data;
    
    // Draw canvas background
    fb_fill(fb, GAME_AREA_X, GAME_AREA_Y, GAME_AREA_WIDTH, GAME_AREA_HEIGHT, 0xC0, 0xC0, 0xC0);
    
    // Draw layers
    for (int layer = 0; layer < MAX_LAYERS; layer++) {
        if (!wb->layer_visible[layer]) continue;
        
        unsigned char* layer_data = wb->layers[layer];
        for (int y = 0; y < GAME_AREA_HEIGHT; y++) {
            for (int x = 0; x < GAME_AREA_WIDTH; x++) {
                int idx = (y * GAME_AREA_WIDTH + x) * 4;
                unsigned char a = layer_data[idx + 3];
                if (a > 0) {
                    fb_pixel(fb, GAME_AREA_X + x, GAME_AREA_Y + y,
                            layer_data[idx], layer_data[idx+1], layer_data[idx+2]);
                }
            }
        }
    }
    
    // Draw grid on canvas
    for (int x = 0; x < GAME_AREA_WIDTH; x += 32) {
        for (int y = 0; y < GAME_AREA_HEIGHT; y++) {
            fb_pixel(fb, GAME_AREA_X + x, GAME_AREA_Y + y, 0xD0, 0xD0, 0xD0);
        }
    }
    for (int y = 0; y < GAME_AREA_HEIGHT; y += 32) {
        for (int x = 0; x < GAME_AREA_WIDTH; x++) {
            fb_pixel(fb, GAME_AREA_X + x, GAME_AREA_Y + y, 0xD0, 0xD0, 0xD0);
        }
    }
    
    // Draw preview of current shape
    if (wb->drawing && wb->mx >= 0 && wb->my >= 0) {
        unsigned char r = wb->cr, g = wb->cg, b = wb->cb;
        if (wb->tool == TOOL_ERASER) {
            r = 0xFF; g = 0xFF; b = 0xFF;
        }
        
        int sx = GAME_AREA_X + wb->start_x;
        int sy = GAME_AREA_Y + wb->start_y;
        int ex = GAME_AREA_X + wb->last_mx;
        int ey = GAME_AREA_Y + wb->last_my;
        
        switch(wb->tool) {
            case TOOL_LINE:
                fb_line(fb, sx, sy, ex, ey, r, g, b);
                break;
            case TOOL_RECT: {
                int rx = (sx < ex) ? sx : ex;
                int ry = (sy < ey) ? sy : ey;
                int rw = abs(ex - sx);
                int rh = abs(ey - sy);
                for (int i = 0; i < rw; i++) {
                    fb_pixel(fb, rx+i, ry, r, g, b);
                    fb_pixel(fb, rx+i, ry+rh, r, g, b);
                }
                for (int i = 0; i < rh; i++) {
                    fb_pixel(fb, rx, ry+i, r, g, b);
                    fb_pixel(fb, rx+rw, ry+i, r, g, b);
                }
                break;
            }
            case TOOL_CIRCLE: {
                int dx = ex - sx;
                int dy = ey - sy;
                int radius = int_sqrt(dx*dx + dy*dy);
                fb_circle_outline(fb, sx, sy, radius, r, g, b);
                break;
            }
        }
    }
    
    // Draw UI panel
    fb_fill(fb, GAME_AREA_X, GAME_AREA_Y + GAME_AREA_HEIGHT - 30, GAME_AREA_WIDTH, 30, 0x2A, 0x2A, 0x2A);
    
    // Status text
    char status[256];
    const char* tools[] = {"Pen", "Line", "Rect", "Circle", "Eraser"};
    const char* color_name = "Custom";
    if (wb->cr==0xFF && wb->cg==0x00 && wb->cb==0x00) color_name = "Red";
    else if (wb->cr==0x00 && wb->cg==0xFF && wb->cb==0x00) color_name = "Green";
    else if (wb->cr==0x00 && wb->cg==0x00 && wb->cb==0xFF) color_name = "Blue";
    else if (wb->cr==0x00 && wb->cg==0x00 && wb->cb==0x00) color_name = "Black";
    else if (wb->cr==0xFF && wb->cg==0xFF && wb->cb==0xFF) color_name = "White";
    else if (wb->cr==0x00 && wb->cg==0xFF && wb->cb==0xFF) color_name = "Cyan";
    else if (wb->cr==0xFF && wb->cg==0x00 && wb->cb==0xFF) color_name = "Magenta";
    else if (wb->cr==0xFF && wb->cg==0xFF && wb->cb==0x00) color_name = "Yellow";
    
    snprintf(status, sizeof(status), "Tool:%s | Color:%s | Layer:%d/%d | Size:%d | H:Help",
             tools[wb->tool], color_name, wb->current_layer+1, MAX_LAYERS, wb->pen_size);
    fb_text(fb, GAME_AREA_X+10, GAME_AREA_Y+GAME_AREA_HEIGHT-20, status, 0xFF, 0xFF, 0xFF);
    
    // Draw color picker
    int colors[][3] = {
        {0,0,0}, {255,255,255}, {255,0,0}, {0,255,0}, {0,0,255},
        {255,255,0}, {255,0,255}, {0,255,255}, {128,128,128}
    };
    for (int i = 0; i < 9; i++) {
        int cx = GAME_AREA_X + GAME_AREA_WIDTH - 200 + i * 22;
        int cy = GAME_AREA_Y + GAME_AREA_HEIGHT - 25;
        fb_fill(fb, cx, cy, 18, 18, colors[i][0], colors[i][1], colors[i][2]);
        if (wb->cr == colors[i][0] && wb->cg == colors[i][1] && wb->cb == colors[i][2]) {
            fb_fill(fb, cx-2, cy-2, 22, 22, 0xFF, 0xFF, 0x00);
        }
    }
    
    // Layer indicators
    for (int i = 0; i < MAX_LAYERS; i++) {
        int lx = GAME_AREA_X + 10 + i * 80;
        int ly = GAME_AREA_Y + GAME_AREA_HEIGHT - 55;
        unsigned char lr = 0x44, lg = 0x44, lb = 0x44;
        if (i == wb->current_layer) {
            lr = 0x00; lg = 0x88; lb = 0xFF;
        }
        if (!wb->layer_visible[i]) {
            lr = 0x44; lg = 0x22; lb = 0x22;
        }
        
        char layer_text[32];
        snprintf(layer_text, sizeof(layer_text), "Layer %d", i+1);
        fb_text(fb, lx, ly, layer_text, lr, lg, lb);
    }
    
    // Help overlay
    if (wb->show_help) {
        fb_fill(fb, GAME_AREA_X+200, GAME_AREA_Y+100, 400, 300, 0x33, 0x33, 0x44);
        fb_text(fb, GAME_AREA_X+220, GAME_AREA_Y+110, "WHITEBOARD HELP", 0xFF, 0xFF, 0x00);
        fb_text(fb, GAME_AREA_X+220, GAME_AREA_Y+130, "=================", 0xFF, 0xFF, 0x00);
        fb_text(fb, GAME_AREA_X+220, GAME_AREA_Y+150, "1-5: Select Tool", 0xFF, 0xFF, 0xFF);
        fb_text(fb, GAME_AREA_X+220, GAME_AREA_Y+170, "R,G,B,K,W,Y,C,M: Colors", 0xFF, 0xFF, 0xFF);
        fb_text(fb, GAME_AREA_X+220, GAME_AREA_Y+190, "[/]: Previous/Next Layer", 0xFF, 0xFF, 0xFF);
        fb_text(fb, GAME_AREA_X+220, GAME_AREA_Y+210, "0-2: Select Layer", 0xFF, 0xFF, 0xFF);
        fb_text(fb, GAME_AREA_X+220, GAME_AREA_Y+230, "+/-: Brush Size", 0xFF, 0xFF, 0xFF);
        fb_text(fb, GAME_AREA_X+220, GAME_AREA_Y+250, "X: Clear Layer", 0xFF, 0xFF, 0xFF);
        fb_text(fb, GAME_AREA_X+220, GAME_AREA_Y+270, "V: Toggle Layer Visibility", 0xFF, 0xFF, 0xFF);
        fb_text(fb, GAME_AREA_X+220, GAME_AREA_Y+290, "H: Hide Help", 0xFF, 0xFF, 0xFF);
        fb_text(fb, GAME_AREA_X+220, GAME_AREA_Y+310, "Click & Drag to Draw", 0xFF, 0xFF, 0xFF);
        fb_text(fb, GAME_AREA_X+220, GAME_AREA_Y+330, "Press H to close", 0x88, 0x88, 0x88);
    }
}

static void wb_cleanup(Game* game) {
    free(game->data);
    printf("Whiteboard App cleaned up\n");
}

// =============================================================================
// EXPORTED GAME CREATOR
// =============================================================================

Game* create_game() {
    Game* game = calloc(1, sizeof(Game));
    if (!game) return NULL;
    
    strcpy(game->name, "Whiteboard App");
    game->active = 1;
    game->init = wb_init;
    game->handle_key = wb_handle_key;
    game->handle_click = wb_handle_click;
    game->update = wb_update;
    game->render = wb_render;
    game->cleanup = wb_cleanup;
    
    return game;
}