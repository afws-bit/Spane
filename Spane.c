// =============================================================================
// SPANE GAME ENGINE - Multi-Instance Sync + Web Mode + Process Isolation
// =============================================================================
// Compile: gcc -O3 -o spane Spane.c -lX11 -lm -ldl -lpthread -lrt
// Run: ./spane [--web] [--clear] [--dev]
// =============================================================================

#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/cursorfont.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <dlfcn.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <sys/prctl.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>

// =============================================================================
// CONFIGURATION
// =============================================================================

// Base design resolution (games are designed for this)
#define BASE_WINDOW_WIDTH  1000
#define BASE_WINDOW_HEIGHT 700
#define BASE_GAME_AREA_WIDTH    800
#define BASE_GAME_AREA_HEIGHT   600
#define BASE_GAME_AREA_X        180
#define BASE_GAME_AREA_Y        50
#define BASE_SIDEBAR_WIDTH      160

#define MAX_GAMES          20
#define MAX_PATH           512

#define WEB_PORT 3000
#define STATE_DIR "~/.spane"
#define SHM_NAME "/spane_shared_state"
#define GAME_SHM_PREFIX "/spane_game_"

// =============================================================================
// 5x7 BITMAP FONT (ASCII 32-126)
// =============================================================================

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

// =============================================================================
// SCREEN DIMENSIONS (Runtime determined)
// =============================================================================

typedef struct {
    int screen_width;
    int screen_height;
    int window_width;
    int window_height;
    int game_area_width;
    int game_area_height;
    int game_area_x;
    int game_area_y;
    int sidebar_width;
    float scale_x;
    float scale_y;
    float game_scale;
} ScreenDimensions;

// =============================================================================
// SDK TYPES
// =============================================================================

typedef struct Framebuffer Framebuffer;
typedef struct Game Game;
typedef struct GameManager GameManager;

// Frame buffer uses base resolution for compatibility
struct Framebuffer {
    unsigned char pixels[BASE_WINDOW_WIDTH * BASE_WINDOW_HEIGHT * 4];
};

struct Game {
    char name[64];
    char path[MAX_PATH];
    void* data;
    void* handle;
    int active;
    
    void (*init)(Game* game);
    void (*handle_key)(Game* game, int key_code, int pressed);
    void (*handle_click)(Game* game, int x, int y);
    void (*update)(Game* game);
    void (*render)(Game* game, Framebuffer* fb);
    void (*cleanup)(Game* game);
};

typedef struct {
    unsigned char framebuffer[BASE_WINDOW_WIDTH * BASE_WINDOW_HEIGHT * 4];
    int ready;
    int running;
    int has_error;
    char error_msg[1024];
    int input_pending;
    int input_type;
    int key_code;
    int key_pressed;
    int click_x;
    int click_y;
    int lock;
    int frame_ready;
} GameSharedMemory;

typedef struct {
    char name[64];
    char path[MAX_PATH];
    int active;
    int index;
    
    pid_t pid;
    int pipe_in[2];
    int pipe_out[2];
    GameSharedMemory* shm;
    char shm_name[64];
    int shm_fd;
    
    int has_error;
    char error_msg[1024];
    time_t error_time;
    int restart_count;
    int error_dialog;
    int error_dialog_button;
    int clipboard_copied;
} GameProcess;

typedef struct {
    int game_count;
    int current_game;
    int instance_count;
    int version;
    char game_paths[MAX_GAMES][MAX_PATH];
    int lock;
} SharedState;

struct GameManager {
    Framebuffer framebuffer;
    GameProcess* games[MAX_GAMES];
    int game_count;
    int current_game;
    int hover_button;
    int mouse_x, mouse_y;
    int frame_count;
    time_t last_fps_update;
    double fps;
    pthread_mutex_t fb_mutex;
    
    Display* display;
    Window window;
    GC gc;
    XImage* ximage;
    Cursor cursor;
    int x11_running;
    int web_mode;
    int dev_mode;
    
    SharedState* shared;
    int shm_fd;
    int instance_id;
    int local_version;
    
    char state_dir[512];
    
    Atom wm_delete_window;
    Atom wm_state;
    Atom wm_fullscreen;
    
    ScreenDimensions screen;
};

// =============================================================================
// FORWARD DECLARATIONS
// =============================================================================

static void sync_from_shared(GameManager* gm);
static void sync_to_shared(GameManager* gm);
static void scan_games_directory(GameManager* gm, const char* dir_path);
static void save_disk_backup(GameManager* gm);
static int load_disk_backup(GameManager* gm);
static void gm_render(GameManager* gm);
static void gm_switch(GameManager* gm, int i);
static void gm_click(GameManager* gm, int x, int y);
static int load_game_from_so(GameManager* gm, const char* path);
static int start_game_process(GameManager* gm, GameProcess* game);
static void stop_game_process(GameManager* gm, GameProcess* game);
static void check_game_processes(GameManager* gm);
static void send_input_to_game(GameManager* gm, GameProcess* game, int type, int key_code, int pressed, int x, int y);
static void game_process_main(GameManager* gm, GameProcess* game);
static void render_error_dialog(Framebuffer* fb, GameProcess* game, GameManager* gm);
static void copy_to_clipboard(const char* text);
static void calculate_screen_dimensions(GameManager* gm);

// =============================================================================
// SPINLOCK
// =============================================================================

static inline void game_shm_lock(GameSharedMemory* s) {
    while (__sync_lock_test_and_set(&s->lock, 1)) {
        usleep(10);
    }
}

static inline void game_shm_unlock(GameSharedMemory* s) {
    __sync_lock_release(&s->lock);
}

static inline void shm_lock(SharedState* s) {
    while (__sync_lock_test_and_set(&s->lock, 1)) {
        usleep(10);
    }
}

static inline void shm_unlock(SharedState* s) {
    __sync_lock_release(&s->lock);
}

// =============================================================================
// CLIPBOARD
// =============================================================================

static void copy_to_clipboard(const char* text) {
    if (!text || !text[0]) return;
    
    int success = 0;
    
    FILE* fp = popen("xclip -selection clipboard 2>/dev/null", "w");
    if (fp) {
        fprintf(fp, "%s", text);
        if (pclose(fp) == 0) success = 1;
    }
    
    if (!success) {
        fp = popen("xsel --clipboard --input 2>/dev/null", "w");
        if (fp) {
            fprintf(fp, "%s", text);
            if (pclose(fp) == 0) success = 1;
        }
    }
    
    if (!success) {
        fp = popen("wl-copy 2>/dev/null", "w");
        if (fp) {
            fprintf(fp, "%s", text);
            pclose(fp);
        }
    }
}

// =============================================================================
// DRAWING
// =============================================================================

static inline void fb_pixel(Framebuffer* fb, int x, int y, unsigned char r, unsigned char g, unsigned char b) {
    if (x < 0 || x >= BASE_WINDOW_WIDTH || y < 0 || y >= BASE_WINDOW_HEIGHT) return;
    int i = (y * BASE_WINDOW_WIDTH + x) * 4;
    fb->pixels[i]=r; fb->pixels[i+1]=g; fb->pixels[i+2]=b; fb->pixels[i+3]=255;
}

static void fb_fill(Framebuffer* fb, int x, int y, int w, int h, unsigned char r, unsigned char g, unsigned char b) {
    int end_x = x + w;
    int end_y = y + h;
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (end_x > BASE_WINDOW_WIDTH) end_x = BASE_WINDOW_WIDTH;
    if (end_y > BASE_WINDOW_HEIGHT) end_y = BASE_WINDOW_HEIGHT;
    for (int dy=y; dy<end_y; dy++)
        for (int dx=x; dx<end_x; dx++)
            fb_pixel(fb, dx, dy, r, g, b);
}

static void fb_rect(Framebuffer* fb, int x, int y, int w, int h, unsigned char r, unsigned char g, unsigned char b) {
    for (int dx=0; dx<w; dx++) { fb_pixel(fb,x+dx,y,r,g,b); fb_pixel(fb,x+dx,y+h-1,r,g,b); }
    for (int dy=1; dy<h-1; dy++) { fb_pixel(fb,x,y+dy,r,g,b); fb_pixel(fb,x+w-1,y+dy,r,g,b); }
}

static void fb_char(Framebuffer* fb, int x, int y, char c, unsigned char r, unsigned char g, unsigned char b) {
    if (c<32||c>126) return;
    const unsigned char* glyph = font_5x7[c-32];
    for (int row=0; row<7; row++)
        for (int col=0; col<5; col++)
            if (glyph[col] & (1<<row)) fb_pixel(fb, x+col, y+row, r, g, b);
}

static void fb_text(Framebuffer* fb, int x, int y, const char* s, unsigned char r, unsigned char g, unsigned char b) {
    if (!s) return;
    for (int i=0; s[i]; i++) fb_char(fb, x+i*6, y, s[i], r, g, b);
}

static void fb_text_center(Framebuffer* fb, int x, int y, int w, const char* s, unsigned char r, unsigned char g, unsigned char b) {
    if (!s) return;
    fb_text(fb, x+(w-(int)strlen(s)*6)/2, y, s, r, g, b);
}

// =============================================================================
// UTILITY
// =============================================================================

static void expand_path(const char* path, char* expanded, int max_len) {
    if (path[0] == '~') {
        const char* home = getenv("HOME");
        if (!home) home = "/tmp";
        snprintf(expanded, max_len, "%s%s", home, path + 1);
    } else {
        strncpy(expanded, path, max_len - 1);
        expanded[max_len-1] = 0;
    }
}

// =============================================================================
// SCREEN DIMENSIONS CALCULATION - FIXED for windowed mode
// =============================================================================

static void calculate_screen_dimensions(GameManager* gm) {
    if (!gm) return;
    
    // Detect screen size
    Screen* screen = DefaultScreenOfDisplay(gm->display);
    gm->screen.screen_width = WidthOfScreen(screen);
    gm->screen.screen_height = HeightOfScreen(screen);
    
    // Use fixed window size based on base resolution (not fullscreen)
    // Use 80% of screen size or base resolution, whichever is smaller
    gm->screen.window_width = BASE_WINDOW_WIDTH;
    gm->screen.window_height = BASE_WINDOW_HEIGHT;
    
    // If screen is smaller, scale down; if larger, keep base size
    if (gm->screen.screen_width < BASE_WINDOW_WIDTH) {
        gm->screen.window_width = gm->screen.screen_width - 100;
        gm->screen.window_height = (gm->screen.window_width * BASE_WINDOW_HEIGHT) / BASE_WINDOW_WIDTH;
    }
    if (gm->screen.screen_height < gm->screen.window_height + 50) {
        gm->screen.window_height = gm->screen.screen_height - 100;
        gm->screen.window_width = (gm->screen.window_height * BASE_WINDOW_WIDTH) / BASE_WINDOW_HEIGHT;
    }
    
    // Ensure minimum window size
    if (gm->screen.window_width < 640) gm->screen.window_width = 640;
    if (gm->screen.window_height < 480) gm->screen.window_height = 480;
    
    // Calculate scale factors
    gm->screen.scale_x = (float)gm->screen.window_width / BASE_WINDOW_WIDTH;
    gm->screen.scale_y = (float)gm->screen.window_height / BASE_WINDOW_HEIGHT;
    
    // Calculate sidebar width (proportional)
    gm->screen.sidebar_width = (int)(BASE_SIDEBAR_WIDTH * gm->screen.scale_x);
    if (gm->screen.sidebar_width < 120) gm->screen.sidebar_width = 120;
    if (gm->screen.sidebar_width > 250) gm->screen.sidebar_width = 250;
    
    // Calculate game area dimensions
    gm->screen.game_area_width = gm->screen.window_width - gm->screen.sidebar_width;
    gm->screen.game_area_height = gm->screen.window_height;
    
    // Game area starts after sidebar
    gm->screen.game_area_x = gm->screen.sidebar_width;
    gm->screen.game_area_y = 0;
    
    // Scale for game rendering (maintain aspect ratio within game area)
    float game_scale_x = (float)gm->screen.game_area_width / BASE_GAME_AREA_WIDTH;
    float game_scale_y = (float)gm->screen.game_area_height / BASE_GAME_AREA_HEIGHT;
    gm->screen.game_scale = (game_scale_x < game_scale_y) ? game_scale_x : game_scale_y;
    
    printf("Screen: %dx%d, Window: %dx%d, Sidebar: %d, Game Area: %dx%d at (%d,%d), Game Scale: %.2f\n",
           gm->screen.screen_width, gm->screen.screen_height,
           gm->screen.window_width, gm->screen.window_height,
           gm->screen.sidebar_width,
           gm->screen.game_area_width, gm->screen.game_area_height,
           gm->screen.game_area_x, gm->screen.game_area_y,
           gm->screen.game_scale);
}

// =============================================================================
// COORDINATE MAPPING FUNCTIONS
// =============================================================================

// Map screen coordinates to base coordinates for game input
static void screen_to_base_coords(GameManager* gm, int screen_x, int screen_y, int* base_x, int* base_y) {
    if (!gm || !base_x || !base_y) return;
    
    *base_x = (int)(screen_x / gm->screen.scale_x);
    *base_y = (int)(screen_y / gm->screen.scale_y);
}

// Map base coordinates to screen coordinates for rendering
static void base_to_screen_coords(GameManager* gm, int base_x, int base_y, int* screen_x, int* screen_y) {
    if (!gm || !screen_x || !screen_y) return;
    
    *screen_x = (int)(base_x * gm->screen.scale_x);
    *screen_y = (int)(base_y * gm->screen.scale_y);
}

// =============================================================================
// SHARED MEMORY
// =============================================================================

static int ensure_state_directory(GameManager* gm) {
    char expanded[512];
    expand_path(STATE_DIR, expanded, sizeof(expanded));
    strncpy(gm->state_dir, expanded, sizeof(gm->state_dir) - 1);
    gm->state_dir[sizeof(gm->state_dir)-1] = 0;
    mkdir(expanded, 0755);
    return 1;
}

static int init_shared_state(GameManager* gm) {
    shm_unlink(SHM_NAME);
    
    gm->shm_fd = shm_open(SHM_NAME, O_CREAT | O_EXCL | O_RDWR, 0600);
    if (gm->shm_fd < 0) {
        perror("shm_open");
        return 0;
    }
    
    if (ftruncate(gm->shm_fd, sizeof(SharedState)) < 0) {
        perror("ftruncate");
        close(gm->shm_fd);
        shm_unlink(SHM_NAME);
        return 0;
    }
    
    gm->shared = mmap(NULL, sizeof(SharedState), PROT_READ | PROT_WRITE, 
                     MAP_SHARED, gm->shm_fd, 0);
    if (gm->shared == MAP_FAILED) {
        perror("mmap");
        close(gm->shm_fd);
        shm_unlink(SHM_NAME);
        return 0;
    }
    
    memset(gm->shared, 0, sizeof(SharedState));
    gm->shared->current_game = -1;
    gm->shared->instance_count = 1;
    gm->shared->version = 1;
    gm->instance_id = 0;
    gm->local_version = 1;
    
    printf("Shared memory ready (instance %d)\n", gm->instance_id + 1);
    return 1;
}

static void sync_from_shared(GameManager* gm) {
    if (!gm->shared) return;
    
    shm_lock(gm->shared);
    
    if (gm->shared->version == gm->local_version) {
        shm_unlock(gm->shared);
        return;
    }
    
    gm->local_version = gm->shared->version;
    int new_count = gm->shared->game_count;
    int new_current = gm->shared->current_game;
    char paths_to_load[MAX_GAMES][MAX_PATH];
    memset(paths_to_load, 0, sizeof(paths_to_load));
    
    for (int i = 0; i < new_count && i < MAX_GAMES; i++) {
        strncpy(paths_to_load[i], gm->shared->game_paths[i], MAX_PATH-1);
        paths_to_load[i][MAX_PATH-1] = 0;
    }
    
    shm_unlock(gm->shared);
    
    if (new_count != gm->game_count || new_current != gm->current_game) {
        for (int i = 0; i < gm->game_count; i++) {
            if (gm->games[i]) {
                stop_game_process(gm, gm->games[i]);
                free(gm->games[i]);
                gm->games[i] = NULL;
            }
        }
        gm->game_count = 0;
        
        for (int i = 0; i < new_count && i < MAX_GAMES; i++) {
            if (paths_to_load[i][0]) {
                load_game_from_so(gm, paths_to_load[i]);
            }
        }
        
        gm->current_game = new_current;
        if (gm->current_game >= gm->game_count) gm->current_game = -1;
    }
}

static void sync_to_shared(GameManager* gm) {
    if (!gm->shared) return;
    
    shm_lock(gm->shared);
    
    int changed = 0;
    
    if (gm->shared->game_count != gm->game_count) {
        gm->shared->game_count = gm->game_count;
        for (int i = 0; i < gm->game_count && i < MAX_GAMES; i++) {
            if (gm->games[i]) {
                strncpy(gm->shared->game_paths[i], gm->games[i]->path, MAX_PATH-1);
                gm->shared->game_paths[i][MAX_PATH-1] = 0;
            }
        }
        changed = 1;
    }
    
    if (gm->shared->current_game != gm->current_game) {
        gm->shared->current_game = gm->current_game;
        changed = 1;
    }
    
    if (changed) {
        gm->shared->version++;
        gm->local_version = gm->shared->version;
    }
    
    shm_unlock(gm->shared);
}

static void cleanup_shared_state(GameManager* gm) {
    if (!gm->shared) return;
    
    shm_lock(gm->shared);
    gm->shared->instance_count--;
    int is_last = (gm->shared->instance_count <= 0);
    shm_unlock(gm->shared);
    
    munmap(gm->shared, sizeof(SharedState));
    gm->shared = NULL;
    
    if (gm->shm_fd >= 0) {
        close(gm->shm_fd);
        gm->shm_fd = -1;
    }
    
    if (is_last) {
        shm_unlink(SHM_NAME);
    }
}

// =============================================================================
// DISK BACKUP
// =============================================================================

static void save_disk_backup(GameManager* gm) {
    if (!gm->shared || gm->game_count == 0) return;
    
    char backup_path[1024];
    snprintf(backup_path, sizeof(backup_path), "%s/backup.dat", gm->state_dir);
    
    shm_lock(gm->shared);
    FILE* f = fopen(backup_path, "wb");
    if (f) {
        fwrite(&gm->shared->game_count, sizeof(int), 1, f);
        fwrite(&gm->shared->current_game, sizeof(int), 1, f);
        for (int i = 0; i < gm->shared->game_count && i < MAX_GAMES; i++) {
            fwrite(gm->shared->game_paths[i], MAX_PATH, 1, f);
        }
        fclose(f);
    }
    shm_unlock(gm->shared);
}

static int load_disk_backup(GameManager* gm) {
    char backup_path[1024];
    snprintf(backup_path, sizeof(backup_path), "%s/backup.dat", gm->state_dir);
    
    FILE* f = fopen(backup_path, "rb");
    if (!f) return 0;
    
    int count, current;
    if (fread(&count, sizeof(int), 1, f) != 1) { fclose(f); return 0; }
    if (fread(&current, sizeof(int), 1, f) != 1) { fclose(f); return 0; }
    
    if (gm->shared) {
        shm_lock(gm->shared);
        gm->shared->game_count = count;
        gm->shared->current_game = current;
        for (int i = 0; i < count && i < MAX_GAMES; i++) {
            if (fread(gm->shared->game_paths[i], MAX_PATH, 1, f) != 1) break;
        }
        gm->shared->version++;
        shm_unlock(gm->shared);
    }
    
    fclose(f);
    return 1;
}

static void clear_all_state(GameManager* gm) {
    if (gm->shared) {
        shm_lock(gm->shared);
        gm->shared->game_count = 0;
        gm->shared->current_game = -1;
        gm->shared->version++;
        shm_unlock(gm->shared);
    }
    
    char backup_path[1024];
    snprintf(backup_path, sizeof(backup_path), "%s/backup.dat", gm->state_dir);
    unlink(backup_path);
}

// =============================================================================
// ERROR DIALOG
// =============================================================================

static void render_error_dialog(Framebuffer* fb, GameProcess* game, GameManager* gm) {
    if (!game || !fb) return;
    
    int dh = gm->dev_mode ? 320 : 280;
    int dx = BASE_GAME_AREA_X + 100;
    int dy = BASE_GAME_AREA_Y + 150;
    int dw = BASE_GAME_AREA_WIDTH - 200;
    
    // Dim the game area behind dialog
    for (int yy = BASE_GAME_AREA_Y; yy < BASE_GAME_AREA_Y + BASE_GAME_AREA_HEIGHT; yy++) {
        for (int xx = BASE_GAME_AREA_X; xx < BASE_GAME_AREA_X + BASE_GAME_AREA_WIDTH; xx++) {
            int i = (yy * BASE_WINDOW_WIDTH + xx) * 4;
            fb->pixels[i] = fb->pixels[i] / 3;
            fb->pixels[i+1] = fb->pixels[i+1] / 3;
            fb->pixels[i+2] = fb->pixels[i+2] / 3;
        }
    }
    
    // Dialog background
    fb_fill(fb, dx + 4, dy + 4, dw, dh, 0x00, 0x00, 0x00);
    fb_fill(fb, dx, dy, dw, dh, 0x2A, 0x2A, 0x35);
    fb_rect(fb, dx, dy, dw, dh, 0xCC, 0x44, 0x44);
    fb_rect(fb, dx+1, dy+1, dw-2, dh-2, 0x88, 0x44, 0x44);
    
    // Title bar
    fb_fill(fb, dx+2, dy+2, dw-4, 30, 0x88, 0x22, 0x22);
    fb_text_center(fb, dx+2, dy+7, dw-4, "GAME CRASHED", 0xFF, 0xFF, 0xFF);
    
    // Error message
    fb_text_center(fb, dx+2, dy+45, dw-4, "The game process has stopped unexpectedly.", 0xFF, 0x88, 0x88);
    
    // Error details
    char error_line[128];
    snprintf(error_line, sizeof(error_line), "Error: %s", 
             game->error_msg[0] ? game->error_msg : "Unknown error");
    
    int text_y = dy + 70;
    const char* msg = error_line;
    char line_buffer[64];
    int line_count = 0;
    while (*msg && text_y < dy + dh - 100 && line_count < 4) {
        int len = 0;
        while (msg[len] && len < 50) len++;
        if (len > 50) {
            int space = len;
            while (space > 0 && msg[space] != ' ') space--;
            if (space > 0) len = space;
            else len = 50;
        }
        memcpy(line_buffer, msg, len);
        line_buffer[len] = 0;
        fb_text_center(fb, dx+2, text_y, dw-4, line_buffer, 0xCC, 0x88, 0x88);
        text_y += 12;
        msg += len;
        if (*msg == ' ') msg++;
        line_count++;
    }
    
    // Restart attempts
    char attempts[64];
    snprintf(attempts, sizeof(attempts), "Restart attempts: %d/3", game->restart_count);
    fb_text_center(fb, dx+2, text_y + 10, dw-4, attempts, 0xAA, 0xAA, 0xAA);
    
    if (game->restart_count >= 3) {
        fb_text_center(fb, dx+2, text_y + 25, dw-4, "Maximum restart attempts reached.", 0xFF, 0x66, 0x00);
    }
    
    // Clipboard copy status (dev mode only)
    if (gm->dev_mode && game->clipboard_copied) {
        fb_text_center(fb, dx+2, text_y + 40, dw-4, "[ Error copied to clipboard! ]", 0x00, 0xFF, 0x00);
    }
    
    // Buttons
    int btn_w = 130;
    int btn_h = 40;
    int btn_y = dy + dh - 80;
    
    // Restart button
    int restart_x = dx + (dw/2) - btn_w - 15;
    unsigned char rr = 0x00, rg = 0x88, rb = 0x00;
    if (game->error_dialog_button == 0) { rr = 0x00; rg = 0xCC; rb = 0x00; }
    fb_fill(fb, restart_x, btn_y, btn_w, btn_h, rr, rg, rb);
    fb_rect(fb, restart_x, btn_y, btn_w, btn_h, 0x00, 0xFF, 0x00);
    fb_text_center(fb, restart_x, btn_y + 14, btn_w, "[ RESTART ]", 0xFF, 0xFF, 0xFF);
    
    // Close button
    int close_x = dx + (dw/2) + 15;
    unsigned char cr = 0x88, cg = 0x22, cb = 0x22;
    if (game->error_dialog_button == 1) { cr = 0xCC; cg = 0x00; cb = 0x00; }
    fb_fill(fb, close_x, btn_y, btn_w, btn_h, cr, cg, cb);
    fb_rect(fb, close_x, btn_y, btn_w, btn_h, 0xFF, 0x44, 0x44);
    fb_text_center(fb, close_x, btn_y + 14, btn_w, "[ CLOSE ]", 0xFF, 0xFF, 0xFF);
    
    // Copy Error button (dev mode only)
    if (gm->dev_mode) {
        int copy_x = dx + (dw/2) - btn_w/2;
        int copy_y = btn_y + btn_h + 10;
        unsigned char cor = 0x44, cog = 0x44, cob = 0x88;
        if (game->error_dialog_button == 2) { cor = 0x66; cog = 0x66; cob = 0xCC; }
        fb_fill(fb, copy_x, copy_y, btn_w, btn_h - 5, cor, cog, cob);
        fb_rect(fb, copy_x, copy_y, btn_w, btn_h - 5, 0x88, 0x88, 0xFF);
        fb_text_center(fb, copy_x, copy_y + 10, btn_w, "[ COPY ERROR ]", 0xFF, 0xFF, 0xFF);
    }
    
    // DEV MODE indicator
    if (gm->dev_mode) {
        fb_text(fb, dx + dw - 80, dy + dh - 15, "DEV MODE", 0x00, 0xFF, 0x00);
    }
}

// =============================================================================
// GAME PROCESS ISOLATION
// =============================================================================

static void game_process_main(GameManager* gm, GameProcess* game) {
    close(game->pipe_in[1]);
    close(game->pipe_out[0]);
    
    signal(SIGTERM, SIG_DFL);
    signal(SIGINT, SIG_DFL);
    
    void* handle = dlopen(game->path, RTLD_NOW);
    if (!handle) {
        game_shm_lock(game->shm);
        snprintf(game->shm->error_msg, sizeof(game->shm->error_msg),
                "Failed to load game: %s", dlerror());
        game->shm->has_error = 1;
        game->shm->ready = 1;
        game_shm_unlock(game->shm);
        exit(1);
    }
    
    typedef Game* (*create_game_fn)();
    create_game_fn create = (create_game_fn)dlsym(handle, "create_game");
    if (!create) {
        game_shm_lock(game->shm);
        snprintf(game->shm->error_msg, sizeof(game->shm->error_msg),
                "Game missing 'create_game' symbol");
        game->shm->has_error = 1;
        game->shm->ready = 1;
        game_shm_unlock(game->shm);
        dlclose(handle);
        exit(1);
    }
    
    Game* game_obj = create();
    if (!game_obj) {
        game_shm_lock(game->shm);
        snprintf(game->shm->error_msg, sizeof(game->shm->error_msg),
                "Game 'create_game' returned NULL");
        game->shm->has_error = 1;
        game->shm->ready = 1;
        game_shm_unlock(game->shm);
        dlclose(handle);
        exit(1);
    }
    
    if (!game_obj->init || !game_obj->render || !game_obj->update || !game_obj->cleanup) {
        game_shm_lock(game->shm);
        snprintf(game->shm->error_msg, sizeof(game->shm->error_msg),
                "Game missing required functions");
        game->shm->has_error = 1;
        game->shm->ready = 1;
        game_shm_unlock(game->shm);
        dlclose(handle);
        exit(1);
    }
    
    game_obj->init(game_obj);
    
    Framebuffer fb;
    memset(&fb, 0, sizeof(fb));
    game_obj->update(game_obj);
    game_obj->render(game_obj, &fb);
    
    game_shm_lock(game->shm);
    memcpy(game->shm->framebuffer, fb.pixels, sizeof(fb.pixels));
    game->shm->frame_ready = 1;
    game->shm->ready = 1;
    game->shm->running = 1;
    game_shm_unlock(game->shm);
    
    while (1) {
        memset(&fb, 0, sizeof(fb));
        
        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(game->pipe_in[0], &rfds);
        struct timeval tv = {0, 0};
        
        if (select(game->pipe_in[0] + 1, &rfds, NULL, NULL, &tv) > 0) {
            char cmd;
            int n = read(game->pipe_in[0], &cmd, 1);
            if (n <= 0 || cmd == 'Q') break;
            
            if (cmd == 'I') {
                int type, key_code, pressed, x, y;
                if (read(game->pipe_in[0], &type, sizeof(int)) != sizeof(int)) break;
                if (read(game->pipe_in[0], &key_code, sizeof(int)) != sizeof(int)) break;
                if (read(game->pipe_in[0], &pressed, sizeof(int)) != sizeof(int)) break;
                if (read(game->pipe_in[0], &x, sizeof(int)) != sizeof(int)) break;
                if (read(game->pipe_in[0], &y, sizeof(int)) != sizeof(int)) break;
                
                if (type == 1 && game_obj->handle_key) {
                    game_obj->handle_key(game_obj, key_code, pressed);
                } else if (type == 2 && game_obj->handle_click) {
                    game_obj->handle_click(game_obj, x, y);
                }
            }
        }
        
        game_obj->update(game_obj);
        game_obj->render(game_obj, &fb);
        
        game_shm_lock(game->shm);
        memcpy(game->shm->framebuffer, fb.pixels, sizeof(fb.pixels));
        game->shm->frame_ready = 1;
        game_shm_unlock(game->shm);
        
        usleep(16667);
    }
    
    game_obj->cleanup(game_obj);
    dlclose(handle);
    
    game_shm_lock(game->shm);
    game->shm->running = 0;
    game_shm_unlock(game->shm);
    
    exit(0);
}

static void cleanup_game_resources(GameProcess* game) {
    if (!game) return;
    
    if (game->pipe_in[0] >= 0) { close(game->pipe_in[0]); game->pipe_in[0] = -1; }
    if (game->pipe_in[1] >= 0) { close(game->pipe_in[1]); game->pipe_in[1] = -1; }
    if (game->pipe_out[0] >= 0) { close(game->pipe_out[0]); game->pipe_out[0] = -1; }
    if (game->pipe_out[1] >= 0) { close(game->pipe_out[1]); game->pipe_out[1] = -1; }
    
    if (game->shm && game->shm != MAP_FAILED) {
        munmap(game->shm, sizeof(GameSharedMemory));
        game->shm = NULL;
    }
    
    if (game->shm_fd >= 0) {
        close(game->shm_fd);
        game->shm_fd = -1;
    }
    
    if (game->shm_name[0] != 0) {
        shm_unlink(game->shm_name);
        game->shm_name[0] = 0;
    }
    
    game->active = 0;
    game->pid = 0;
    game->error_dialog = 0;
    game->clipboard_copied = 0;
}

static int start_game_process(GameManager* gm, GameProcess* game) {
    if (!game || !gm) return 0;
    
    cleanup_game_resources(game);
    
    game->has_error = 0;
    game->error_msg[0] = 0;
    game->error_dialog = 0;
    game->error_dialog_button = -1;
    game->clipboard_copied = 0;
    
    snprintf(game->shm_name, sizeof(game->shm_name), "%s%d_%d_%d", 
             GAME_SHM_PREFIX, getpid(), game->index, (int)time(NULL));
    
    shm_unlink(game->shm_name);
    
    game->shm_fd = shm_open(game->shm_name, O_CREAT | O_EXCL | O_RDWR, 0600);
    if (game->shm_fd < 0) {
        snprintf(game->error_msg, sizeof(game->error_msg), 
                "Failed to create shared memory: %s", strerror(errno));
        game->has_error = 1;
        game->error_time = time(NULL);
        game->error_dialog = 1;
        return 0;
    }
    
    if (ftruncate(game->shm_fd, sizeof(GameSharedMemory)) < 0) {
        snprintf(game->error_msg, sizeof(game->error_msg), 
                "Failed to resize shared memory: %s", strerror(errno));
        game->has_error = 1;
        game->error_time = time(NULL);
        game->error_dialog = 1;
        close(game->shm_fd);
        shm_unlink(game->shm_name);
        game->shm_fd = -1;
        game->shm_name[0] = 0;
        return 0;
    }
    
    game->shm = mmap(NULL, sizeof(GameSharedMemory), PROT_READ | PROT_WRITE, 
                     MAP_SHARED, game->shm_fd, 0);
    if (game->shm == MAP_FAILED) {
        snprintf(game->error_msg, sizeof(game->error_msg), 
                "Failed to map shared memory: %s", strerror(errno));
        game->has_error = 1;
        game->error_time = time(NULL);
        game->error_dialog = 1;
        close(game->shm_fd);
        shm_unlink(game->shm_name);
        game->shm_fd = -1;
        game->shm_name[0] = 0;
        game->shm = NULL;
        return 0;
    }
    
    memset(game->shm, 0, sizeof(GameSharedMemory));
    
    if (pipe(game->pipe_in) < 0 || pipe(game->pipe_out) < 0) {
        snprintf(game->error_msg, sizeof(game->error_msg), 
                "Failed to create pipes: %s", strerror(errno));
        game->has_error = 1;
        game->error_time = time(NULL);
        game->error_dialog = 1;
        cleanup_game_resources(game);
        return 0;
    }
    
    game->pid = fork();
    if (game->pid < 0) {
        snprintf(game->error_msg, sizeof(game->error_msg), 
                "Failed to fork: %s", strerror(errno));
        game->has_error = 1;
        game->error_time = time(NULL);
        game->error_dialog = 1;
        cleanup_game_resources(game);
        return 0;
    }
    
    if (game->pid == 0) {
        game_process_main(gm, game);
        _exit(1);
    }
    
    close(game->pipe_in[0]);
    close(game->pipe_out[1]);
    game->pipe_in[0] = -1;
    game->pipe_out[1] = -1;
    
    time_t start = time(NULL);
    while (1) {
        int status;
        pid_t result = waitpid(game->pid, &status, WNOHANG);
        if (result > 0) {
            snprintf(game->error_msg, sizeof(game->error_msg), 
                    "Game process crashed during startup");
            game->has_error = 1;
            game->error_time = time(NULL);
            game->pid = 0;
            game->error_dialog = 1;
            printf("Game crashed on startup\n");
            return 0;
        }
        
        if (game->shm) {
            game_shm_lock(game->shm);
            int ready = game->shm->ready;
            int has_error = game->shm->has_error;
            if (has_error) {
                strncpy(game->error_msg, game->shm->error_msg, sizeof(game->error_msg)-1);
                game->error_msg[sizeof(game->error_msg)-1] = 0;
            }
            game_shm_unlock(game->shm);
            
            if (ready) {
                if (has_error) {
                    game->has_error = 1;
                    game->error_time = time(NULL);
                    game->error_dialog = 1;
                    printf("Game failed to start: %s\n", game->error_msg);
                    kill(game->pid, SIGTERM);
                    waitpid(game->pid, &status, 0);
                    game->pid = 0;
                    return 0;
                }
                
                game->active = 1;
                game->has_error = 0;
                game->error_dialog = 0;
                printf("Game started successfully (PID: %d)\n", game->pid);
                return 1;
            }
        } else {
            snprintf(game->error_msg, sizeof(game->error_msg), "Shared memory lost during startup");
            game->has_error = 1;
            game->error_time = time(NULL);
            game->error_dialog = 1;
            kill(game->pid, SIGTERM);
            waitpid(game->pid, &status, 0);
            game->pid = 0;
            return 0;
        }
        
        if (time(NULL) - start > 5) {
            snprintf(game->error_msg, sizeof(game->error_msg), "Game startup timeout");
            game->has_error = 1;
            game->error_time = time(NULL);
            game->error_dialog = 1;
            kill(game->pid, SIGTERM);
            waitpid(game->pid, &status, 0);
            game->pid = 0;
            return 0;
        }
        
        usleep(10000);
    }
}

static void stop_game_process(GameManager* gm, GameProcess* game) {
    if (!game) return;
    
    if (game->pid > 0) {
        if (game->pipe_in[1] >= 0) {
            char cmd = 'Q';
            ssize_t ret __attribute__((unused));
            ret = write(game->pipe_in[1], &cmd, 1);
        }
        
        usleep(100000);
        
        int status;
        if (waitpid(game->pid, &status, WNOHANG) == 0) {
            kill(game->pid, SIGTERM);
            usleep(100000);
            
            if (waitpid(game->pid, &status, WNOHANG) == 0) {
                kill(game->pid, SIGKILL);
                waitpid(game->pid, &status, 0);
            }
        }
    }
    
    cleanup_game_resources(game);
}

static void send_input_to_game(GameManager* gm, GameProcess* game, int type, int key_code, int pressed, int x, int y) {
    if (!game || !game->active || game->pid <= 0 || game->has_error) return;
    if (game->pipe_in[1] < 0) return;
    
    int status;
    if (waitpid(game->pid, &status, WNOHANG) != 0) {
        return;
    }
    
    ssize_t ret __attribute__((unused));
    char cmd = 'I';
    ret = write(game->pipe_in[1], &cmd, 1);
    if (ret != 1) return;
    ret = write(game->pipe_in[1], &type, sizeof(int));
    if (ret != sizeof(int)) return;
    ret = write(game->pipe_in[1], &key_code, sizeof(int));
    if (ret != sizeof(int)) return;
    ret = write(game->pipe_in[1], &pressed, sizeof(int));
    if (ret != sizeof(int)) return;
    ret = write(game->pipe_in[1], &x, sizeof(int));
    if (ret != sizeof(int)) return;
    ret = write(game->pipe_in[1], &y, sizeof(int));
    if (ret != sizeof(int)) return;
}

static void check_game_processes(GameManager* gm) {
    if (!gm) return;
    
    for (int i = 0; i < gm->game_count; i++) {
        GameProcess* game = gm->games[i];
        if (!game) continue;
        if (!game->active || game->pid <= 0) continue;
        
        int status;
        pid_t result = waitpid(game->pid, &status, WNOHANG);
        if (result > 0) {
            printf("Game process terminated (PID: %d)\n", game->pid);
            
            if (WIFEXITED(status)) {
                snprintf(game->error_msg, sizeof(game->error_msg), 
                        "Game exited with code %d", WEXITSTATUS(status));
            } else if (WIFSIGNALED(status)) {
                snprintf(game->error_msg, sizeof(game->error_msg), 
                        "Game crashed: signal %d (%s)", WTERMSIG(status), strsignal(WTERMSIG(status)));
            } else {
                snprintf(game->error_msg, sizeof(game->error_msg), 
                        "Game terminated unexpectedly");
            }
            
            game->has_error = 1;
            game->error_time = time(NULL);
            game->error_dialog = 1;
            game->active = 0;
            game->pid = 0;
            game->clipboard_copied = 0;
            
            if (game->pipe_in[0] >= 0) { close(game->pipe_in[0]); game->pipe_in[0] = -1; }
            if (game->pipe_in[1] >= 0) { close(game->pipe_in[1]); game->pipe_in[1] = -1; }
            if (game->pipe_out[0] >= 0) { close(game->pipe_out[0]); game->pipe_out[0] = -1; }
            if (game->pipe_out[1] >= 0) { close(game->pipe_out[1]); game->pipe_out[1] = -1; }
            
            printf("Game error: %s\n", game->error_msg);
        }
    }
}

// =============================================================================
// GAME LOADER
// =============================================================================

static int load_game_from_so(GameManager* gm, const char* path) {
    if (!gm || !path || gm->game_count >= MAX_GAMES) return 0;
    
    GameProcess* game = calloc(1, sizeof(GameProcess));
    if (!game) return 0;
    
    strncpy(game->path, path, MAX_PATH-1);
    game->path[MAX_PATH-1] = 0;
    game->index = gm->game_count;
    
    game->pipe_in[0] = -1;
    game->pipe_in[1] = -1;
    game->pipe_out[0] = -1;
    game->pipe_out[1] = -1;
    game->shm_fd = -1;
    game->pid = 0;
    game->active = 0;
    game->has_error = 0;
    game->error_dialog = 0;
    game->error_dialog_button = -1;
    game->restart_count = 0;
    game->shm = NULL;
    game->shm_name[0] = 0;
    game->clipboard_copied = 0;
    game->error_msg[0] = 0;
    
    void* handle = dlopen(path, RTLD_NOW | RTLD_LOCAL);
    if (!handle) {
        snprintf(game->error_msg, sizeof(game->error_msg), "Cannot load library: %s", dlerror());
        game->has_error = 1;
        game->error_time = time(NULL);
        game->error_dialog = 1;
        strncpy(game->name, "Invalid Game", sizeof(game->name)-1);
        game->name[sizeof(game->name)-1] = 0;
        gm->games[gm->game_count++] = game;
        return 0;
    }
    
    typedef Game* (*create_game_fn)();
    create_game_fn create = (create_game_fn)dlsym(handle, "create_game");
    Game* temp = NULL;
    
    if (create) {
        temp = create();
    }
    
    if (temp && temp->name[0]) {
        strncpy(game->name, temp->name, sizeof(game->name)-1);
        game->name[sizeof(game->name)-1] = 0;
    } else {
        const char* fname = strrchr(path, '/');
        if (!fname) fname = path;
        else fname++;
        strncpy(game->name, fname, sizeof(game->name)-1);
        game->name[sizeof(game->name)-1] = 0;
        char* dot = strstr(game->name, ".so");
        if (dot) *dot = 0;
    }
    
    if (!create) {
        snprintf(game->error_msg, sizeof(game->error_msg), "Missing 'create_game' symbol");
        game->has_error = 1;
        game->error_time = time(NULL);
        game->error_dialog = 1;
    }
    
    dlclose(handle);
    
    gm->games[gm->game_count++] = game;
    
    if (!game->has_error) {
        if (!start_game_process(gm, game)) {
            printf("Game '%s' loaded with errors\n", game->name);
        }
    }
    
    return 1;
}

static void scan_games_directory(GameManager* gm, const char* dir_path) {
    if (!gm || !dir_path) return;
    
    DIR* dir = opendir(dir_path);
    if (!dir) return;
    
    struct dirent* entry;
    char full_path[MAX_PATH];
    
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') continue;
        int len = strlen(entry->d_name);
        if (len < 3 || strcmp(entry->d_name + len - 3, ".so") != 0) continue;
        snprintf(full_path, MAX_PATH, "%s/%s", dir_path, entry->d_name);
        struct stat st;
        if (stat(full_path, &st) == 0 && S_ISREG(st.st_mode))
            load_game_from_so(gm, full_path);
    }
    closedir(dir);
}

static void remove_game_instance(GameManager* gm, int index) {
    if (!gm || index < 0 || index >= gm->game_count) return;
    
    GameProcess* game = gm->games[index];
    if (!game) return;
    
    printf("Removing: %s\n", game->name);
    
    stop_game_process(gm, game);
    free(game);
    gm->games[index] = NULL;
    
    for (int i = index; i < gm->game_count - 1; i++) {
        gm->games[i] = gm->games[i+1];
        if (gm->games[i]) {
            gm->games[i]->index = i;
        }
    }
    gm->games[gm->game_count - 1] = NULL;
    gm->game_count--;
    
    if (gm->current_game >= gm->game_count)
        gm->current_game = gm->game_count > 0 ? gm->game_count - 1 : -1;
    
    sync_to_shared(gm);
}

// =============================================================================
// RENDERING
// =============================================================================

static void gm_render(GameManager* gm) {
    if (!gm) return;
    
    pthread_mutex_lock(&gm->fb_mutex);
    Framebuffer* fb = &gm->framebuffer;
    
    // Render to base resolution framebuffer
    fb_fill(fb, 0, 0, BASE_WINDOW_WIDTH, BASE_WINDOW_HEIGHT, 0x1A, 0x1A, 0x1A);
    
    // Sidebar
    fb_fill(fb, 0, 0, BASE_SIDEBAR_WIDTH, BASE_WINDOW_HEIGHT, 0x15, 0x15, 0x18);
    fb_text_center(fb, 0, 15, BASE_SIDEBAR_WIDTH, "SPANE ENGINE", 0x00, 0xCC, 0xFF);
    fb_fill(fb, 10, 35, BASE_SIDEBAR_WIDTH-20, 2, 0x00, 0x88, 0xCC);
    
    if (gm->dev_mode) {
        fb_text(fb, 10, 40, "DEV", 0x00, 0xFF, 0x00);
    }
    
    int by = gm->dev_mode ? 65 : 55;
    
    for (int i = 0; i < gm->game_count; i++) {
        GameProcess* gp = gm->games[i];
        if (!gp) continue;
        
        int act = (i == gm->current_game);
        int hov = (i == gm->hover_button);
        unsigned char sr, sg, sb;
        
        if (gp->has_error) { sr = 0x88; sg = 0x22; sb = 0x22; }
        else if (act) { sr = 0x00; sg = 0x88; sb = 0xCC; }
        else if (hov) { sr = 0xCC; sg = 0x88; sb = 0x00; }
        else { sr = 0x25; sg = 0x25; sb = 0x2A; }
        
        fb_fill(fb, 10, by, BASE_SIDEBAR_WIDTH-35, 35, sr, sg, sb);
        fb_rect(fb, 10, by, BASE_SIDEBAR_WIDTH-35, 35, 
                gp->has_error ? 0xFF : (act ? 0x00:0x44),
                gp->has_error ? 0x44 : (act ? 0xCC:0x44),
                gp->has_error ? 0x44 : (act ? 0xCC:0x44));
        fb_text_center(fb, 10, by+12, BASE_SIDEBAR_WIDTH-35, gp->name, 0xFF, 0xFF, 0xFF);
        
        int cx = BASE_SIDEBAR_WIDTH - 30;
        fb_fill(fb, cx, by + 8, 16, 16, 0x88, 0x22, 0x22);
        fb_rect(fb, cx, by + 8, 16, 16, 0xFF, 0x44, 0x44);
        fb_text(fb, cx + 4, by + 12, "X", 0xFF, 0xFF, 0xFF);
        
        by += 45;
    }
    
    int lby = BASE_WINDOW_HEIGHT - 110;
    
    fb_fill(fb, 10, lby, BASE_SIDEBAR_WIDTH-20, 28, 0x00, 0x66, 0x00);
    fb_rect(fb, 10, lby, BASE_SIDEBAR_WIDTH-20, 28, 0x00, 0x88, 0x00);
    fb_text_center(fb, 10, lby+8, BASE_SIDEBAR_WIDTH-20, "[ Load Game ]", 0xFF, 0xFF, 0xFF);
    
    fb_fill(fb, 10, lby+33, BASE_SIDEBAR_WIDTH-20, 28, 0x66, 0x00, 0x00);
    fb_rect(fb, 10, lby+33, BASE_SIDEBAR_WIDTH-20, 28, 0x88, 0x00, 0x00);
    fb_text_center(fb, 10, lby+41, BASE_SIDEBAR_WIDTH-20, "[ Clear All ]", 0xFF, 0xFF, 0xFF);
    
    char info[64];
    snprintf(info, sizeof(info), "FPS:%.1f Inst:%d %s%s", gm->fps, gm->instance_id + 1,
             gm->web_mode ? "WEB" : "X11", gm->dev_mode ? " DEV" : "");
    fb_text(fb, 10, BASE_WINDOW_HEIGHT - 25, info, 0x88, 0x88, 0x88);
    
    fb_rect(fb, BASE_GAME_AREA_X-1, BASE_GAME_AREA_Y-1, BASE_GAME_AREA_WIDTH+2, BASE_GAME_AREA_HEIGHT+2, 0x44, 0x44, 0x55);
    fb_rect(fb, BASE_GAME_AREA_X-2, BASE_GAME_AREA_Y-2, BASE_GAME_AREA_WIDTH+4, BASE_GAME_AREA_HEIGHT+4, 0x44, 0x44, 0x55);
    
    if (gm->current_game >= 0 && gm->current_game < gm->game_count) {
        GameProcess* game = gm->games[gm->current_game];
        if (!game) goto no_game;
        
        if (game->shm && !game->has_error) {
            game_shm_lock(game->shm);
            if (game->shm->frame_ready) {
                // Copy game framebuffer directly (both use base resolution)
                for (int gy = 0; gy < BASE_GAME_AREA_HEIGHT; gy++) {
                    for (int gx = 0; gx < BASE_GAME_AREA_WIDTH; gx++) {
                        int src_idx = ((gy + BASE_GAME_AREA_Y) * BASE_WINDOW_WIDTH + (gx + BASE_GAME_AREA_X)) * 4;
                        int dst_idx = src_idx;
                        fb->pixels[dst_idx] = game->shm->framebuffer[src_idx];
                        fb->pixels[dst_idx + 1] = game->shm->framebuffer[src_idx + 1];
                        fb->pixels[dst_idx + 2] = game->shm->framebuffer[src_idx + 2];
                        fb->pixels[dst_idx + 3] = 255;
                    }
                }
            }
            game_shm_unlock(game->shm);
        }
        
        char lb[128];
        snprintf(lb, sizeof(lb), "Game: %s%s", 
                game->name[0] ? game->name : "Unknown",
                game->has_error ? " [CRASHED]" : (game->active ? "" : " [LOADING]"));
        fb_text(fb, BASE_GAME_AREA_X, BASE_GAME_AREA_Y-15, lb, 
                game->has_error ? 0xFF : 0xCC, 
                game->has_error ? 0x44 : 0xCC, 
                game->has_error ? 0x44 : 0xCC);
        
        if (game->error_dialog && game->has_error) {
            render_error_dialog(fb, game, gm);
        }
    } else {
no_game:
        if (gm->game_count == 0) {
            fb_text_center(fb, BASE_GAME_AREA_X, BASE_GAME_AREA_Y + BASE_GAME_AREA_HEIGHT/2, BASE_GAME_AREA_WIDTH,
                           "Click [Load Game] to add a game", 0x66, 0x66, 0x66);
        }
    }
    
    pthread_mutex_unlock(&gm->fb_mutex);
}

static void gm_switch(GameManager* gm, int i) {
    if (!gm || i < 0 || i >= gm->game_count) return;
    if (!gm->games[i]) return;
    
    gm->current_game = i;
    sync_to_shared(gm);
}

// =============================================================================
// gm_click - Process sidebar clicks FIRST, then error dialog
// =============================================================================

static void gm_click(GameManager* gm, int x, int y) {
    if (!gm) return;
    
    // Map screen coordinates to base coordinates
    int base_x, base_y;
    screen_to_base_coords(gm, x, y, &base_x, &base_y);
    
    // Check Load Game and Clear All buttons (bottom of sidebar)
    int lby = BASE_WINDOW_HEIGHT - 110;
    if (base_x >= 10 && base_x < BASE_SIDEBAR_WIDTH-10) {
        if (base_y >= lby && base_y < lby+28) {
            printf("Load Game button clicked\n");
            int ret __attribute__((unused));
            ret = system("zenity --file-selection --title=\"Select Game .so file\" "
                   "--file-filter=\"Shared Objects (*.so) | *.so\" "
                   "> /tmp/spane_load.txt 2>/dev/null");
            FILE* f = fopen("/tmp/spane_load.txt", "r");
            if (f) {
                char path[MAX_PATH];
                if (fgets(path, sizeof(path), f)) {
                    path[strcspn(path, "\n")] = 0;
                    if (strlen(path) > 0 && load_game_from_so(gm, path)) {
                        gm->current_game = gm->game_count - 1;
                        sync_to_shared(gm);
                        save_disk_backup(gm);
                    }
                }
                fclose(f);
            }
            remove("/tmp/spane_load.txt");
            return;
        }
        
        if (base_y >= lby+33 && base_y < lby+61) {
            printf("Clear All button clicked\n");
            for (int i = 0; i < gm->game_count; i++) {
                if (gm->games[i]) {
                    stop_game_process(gm, gm->games[i]);
                    free(gm->games[i]);
                    gm->games[i] = NULL;
                }
            }
            gm->game_count = 0;
            gm->current_game = -1;
            clear_all_state(gm);
            return;
        }
    }
    
    // Check sidebar game list (X buttons and game selection)
    if (base_x < BASE_SIDEBAR_WIDTH) {
        int by = gm->dev_mode ? 65 : 55;
        for (int i = 0; i < gm->game_count; i++) {
            GameProcess* gp = gm->games[i];
            if (!gp) { by += 45; continue; }
            
            int cx = BASE_SIDEBAR_WIDTH - 30;
            // X button to remove game
            if (base_x >= cx && base_x < cx+16 && base_y >= by+8 && base_y < by+24) {
                printf("X button clicked for game: %s\n", gp->name);
                remove_game_instance(gm, i);
                return;
            }
            // Game selection
            if (base_y >= by && base_y < by+35 && base_x >= 10 && base_x < cx-2) {
                printf("Switching to game: %s (index %d)\n", gp->name, i);
                gm_switch(gm, i);
                return;
            }
            by += 45;
        }
        return;
    }
    
    // Handle error dialog buttons
    if (gm->current_game >= 0 && gm->current_game < gm->game_count) {
        GameProcess* game = gm->games[gm->current_game];
        if (game && game->error_dialog && game->has_error) {
            int dx = BASE_GAME_AREA_X + 100;
            int dy = BASE_GAME_AREA_Y + 150;
            int dw = BASE_GAME_AREA_WIDTH - 200;
            int dh = gm->dev_mode ? 320 : 280;
            int btn_w = 130;
            int btn_h = 40;
            int btn_y = dy + dh - 80;
            
            if (base_x >= dx && base_x < dx + dw && base_y >= dy && base_y < dy + dh) {
                int restart_x = dx + (dw/2) - btn_w - 15;
                int close_x = dx + (dw/2) + 15;
                
                if (base_x >= restart_x && base_x < restart_x + btn_w &&
                    base_y >= btn_y && base_y < btn_y + btn_h) {
                    printf("Restarting game '%s'...\n", game->name);
                    game->restart_count++;
                    game->has_error = 0;
                    game->error_msg[0] = 0;
                    game->error_dialog = 0;
                    game->error_dialog_button = -1;
                    game->clipboard_copied = 0;
                    start_game_process(gm, game);
                    return;
                }
                
                if (base_x >= close_x && base_x < close_x + btn_w &&
                    base_y >= btn_y && base_y < btn_y + btn_h) {
                    printf("Closing game '%s'\n", game->name);
                    remove_game_instance(gm, gm->current_game);
                    return;
                }
                
                if (gm->dev_mode) {
                    int copy_y = btn_y + btn_h + 10;
                    int copy_x = dx + (dw/2) - btn_w/2;
                    
                    if (base_x >= copy_x && base_x < copy_x + btn_w &&
                        base_y >= copy_y && base_y < copy_y + btn_h - 5) {
                        printf("Copying error to clipboard\n");
                        char clipboard_text[2048];
                        snprintf(clipboard_text, sizeof(clipboard_text), 
                                "SPANE Engine - Game Error Report\n"
                                "================================\n"
                                "Game: %s\n"
                                "Path: %s\n"
                                "Time: %s"
                                "Error: %s\n"
                                "Restart Attempts: %d/3\n",
                                game->name, game->path, ctime(&game->error_time), 
                                game->error_msg, game->restart_count);
                        copy_to_clipboard(clipboard_text);
                        game->clipboard_copied = 1;
                        return;
                    }
                }
                
                return;
            }
        }
        
        // Forward click to active game (in base coordinates)
        if (game && game->active && !game->has_error && !game->error_dialog) {
            send_input_to_game(gm, game, 2, 0, 0, base_x, base_y);
        }
    }
}

static void gm_fps(GameManager* gm) {
    if (!gm) return;
    gm->frame_count++;
    time_t n = time(NULL);
    if (n - gm->last_fps_update >= 1) {
        gm->fps = gm->frame_count / (double)(n - gm->last_fps_update);
        gm->frame_count = 0;
        gm->last_fps_update = n;
    }
}

// =============================================================================
// X11 - FIXED for windowed mode with mouse cursor and resize support
// =============================================================================

static void x11_mirror_frame(GameManager* gm) {
    if (!gm || !gm->display || !gm->ximage) return;
    
    pthread_mutex_lock(&gm->fb_mutex);
    
    // Scale the base resolution framebuffer to window resolution
    unsigned char* src = gm->framebuffer.pixels;
    char* dst = gm->ximage->data;
    
    // Clear destination
    memset(dst, 0, gm->screen.window_width * gm->screen.window_height * 4);
    
    // Nearest-neighbor scaling from base resolution to window resolution
    for (int sy = 0; sy < gm->screen.window_height; sy++) {
        for (int sx = 0; sx < gm->screen.window_width; sx++) {
            // Map screen pixel to base resolution pixel
            int bx = (int)(sx / gm->screen.scale_x);
            int by = (int)(sy / gm->screen.scale_y);
            
            if (bx >= 0 && bx < BASE_WINDOW_WIDTH && by >= 0 && by < BASE_WINDOW_HEIGHT) {
                int src_idx = (by * BASE_WINDOW_WIDTH + bx) * 4;
                int dst_idx = (sy * gm->screen.window_width + sx) * 4;
                
                dst[dst_idx + 0] = src[src_idx + 2];  // BGR format for X11
                dst[dst_idx + 1] = src[src_idx + 1];
                dst[dst_idx + 2] = src[src_idx + 0];
                dst[dst_idx + 3] = 0;
            }
        }
    }
    
    XPutImage(gm->display, gm->window, gm->gc, gm->ximage, 0, 0, 0, 0, 
              gm->screen.window_width, gm->screen.window_height);
    XFlush(gm->display);
    pthread_mutex_unlock(&gm->fb_mutex);
}

static int x11_init(GameManager* gm) {
    if (!gm) return 0;
    
    gm->display = XOpenDisplay(NULL);
    if (!gm->display) return 0;
    
    // Calculate screen dimensions for windowed mode
    calculate_screen_dimensions(gm);
    
    gm->wm_delete_window = XInternAtom(gm->display, "WM_DELETE_WINDOW", False);
    gm->wm_state = XInternAtom(gm->display, "_NET_WM_STATE", False);
    gm->wm_fullscreen = XInternAtom(gm->display, "_NET_WM_STATE_FULLSCREEN", False);
    
    int s = DefaultScreen(gm->display);
    int black = BlackPixel(gm->display, s);
    int white = WhitePixel(gm->display, s);
    
    // Create a normal window with title bar and borders
    gm->window = XCreateSimpleWindow(gm->display, RootWindow(gm->display, s),
                                     0, 0, gm->screen.window_width, gm->screen.window_height, 1,
                                     black, 0x1A1A1A);
    
    // Set window title and class
    XStoreName(gm->display, gm->window, "SPANE Game Engine");
    XSetIconName(gm->display, gm->window, "SPANE");
    
    // Set WM_CLASS for proper window management
    XClassHint class_hint;
    class_hint.res_name = "spane";
    class_hint.res_class = "SPANE";
    XSetClassHint(gm->display, gm->window, &class_hint);
    
    // Set window manager protocols (close button)
    XSetWMProtocols(gm->display, gm->window, &gm->wm_delete_window, 1);
    
    // Set window size hints with minimum and maximum sizes
    XSizeHints size_hints;
    size_hints.flags = PPosition | PSize | PMinSize | PMaxSize | PResizeInc;
    size_hints.x = 0;
    size_hints.y = 0;
    size_hints.width = gm->screen.window_width;
    size_hints.height = gm->screen.window_height;
    size_hints.min_width = 640;
    size_hints.min_height = 480;
    size_hints.max_width = gm->screen.screen_width;
    size_hints.max_height = gm->screen.screen_height;
    size_hints.width_inc = 1;
    size_hints.height_inc = 1;
    XSetNormalHints(gm->display, gm->window, &size_hints);
    
    // Set window manager hints for proper decoration
    XWMHints wm_hints;
    wm_hints.flags = InputHint | StateHint;
    wm_hints.input = True;
    wm_hints.initial_state = NormalState;
    XSetWMHints(gm->display, gm->window, &wm_hints);
    
    // Create cursor (standard arrow cursor for windowed mode)
    gm->cursor = XCreateFontCursor(gm->display, XC_left_ptr);
    XDefineCursor(gm->display, gm->window, gm->cursor);
    
    // Select input events
    XSelectInput(gm->display, gm->window,
                 ExposureMask | KeyPressMask | KeyReleaseMask | ButtonPressMask | 
                 ButtonReleaseMask | PointerMotionMask | StructureNotifyMask | 
                 EnterWindowMask | LeaveWindowMask);
    
    // Map the window
    XMapWindow(gm->display, gm->window);
    XFlush(gm->display);
    
    // Wait for window to be mapped
    XEvent e;
    do { XNextEvent(gm->display, &e); } while (e.type != MapNotify);
    
    // Create graphics context
    gm->gc = XCreateGC(gm->display, gm->window, 0, NULL);
    
    // Create XImage for window resolution
    char* imgdata = malloc(gm->screen.window_width * gm->screen.window_height * 4);
    gm->ximage = XCreateImage(gm->display, DefaultVisual(gm->display, s), 24, ZPixmap, 0,
                              imgdata, gm->screen.window_width, gm->screen.window_height, 32, 0);
    
    printf("X11 ready (Windowed mode %dx%d)\n", gm->screen.window_width, gm->screen.window_height);
    return 1;
}

static int x11_to_keycode(KeySym ks) {
    if (ks == XK_Escape) return 27;
    if (ks >= XK_F1 && ks <= XK_F12) return 111 + (ks - XK_F1);
    if (ks == XK_Up || ks == XK_w || ks == XK_W) return 38;
    if (ks == XK_Down || ks == XK_s || ks == XK_S) return 40;
    if (ks == XK_Left || ks == XK_a || ks == XK_A) return 37;
    if (ks == XK_Right || ks == XK_d || ks == XK_D) return 39;
    if (ks == XK_Return || ks == XK_KP_Enter) return 13;
    if (ks == XK_space || ks == XK_KP_Space) return 32;
    if (ks == XK_BackSpace) return 8;
    if (ks == XK_Delete || ks == XK_KP_Delete) return 127;
    if (ks == XK_Tab) return 9;
    if (ks == XK_Shift_L || ks == XK_Shift_R) return 16;
    if (ks == XK_Control_L || ks == XK_Control_R) return 17;
    if (ks >= XK_A && ks <= XK_Z) return 'A' + (ks - XK_A);
    if (ks >= XK_a && ks <= XK_z) return 'A' + (ks - XK_a);
    if (ks >= XK_0 && ks <= XK_9) return '0' + (ks - XK_0);
    if (ks >= XK_KP_0 && ks <= XK_KP_9) return '0' + (ks - XK_KP_0);
    if (ks == XK_KP_Add) return '+';
    if (ks == XK_KP_Subtract) return '-';
    if (ks == XK_KP_Multiply) return '*';
    if (ks == XK_KP_Divide) return '/';
    if (ks == XK_KP_Decimal) return '.';
    return 0;
}

static void x11_process(GameManager* gm, int* running) {
    if (!gm || !running) return;
    
    while (XPending(gm->display)) {
        XEvent e;
        XNextEvent(gm->display, &e);
        
        if (e.type == ClientMessage) {
            if ((Atom)e.xclient.data.l[0] == gm->wm_delete_window) {
                printf("Window close requested via WM\n");
                *running = 0;
                return;
            }
        }
        
        if (e.type == ConfigureNotify) {
            // Window was resized
            int new_width = e.xconfigure.width;
            int new_height = e.xconfigure.height;
            
            if (new_width != gm->screen.window_width || new_height != gm->screen.window_height) {
                printf("Window resized to %dx%d\n", new_width, new_height);
                
                // Update window dimensions
                gm->screen.window_width = new_width;
                gm->screen.window_height = new_height;
                
                // Recalculate scale factors
                gm->screen.scale_x = (float)gm->screen.window_width / BASE_WINDOW_WIDTH;
                gm->screen.scale_y = (float)gm->screen.window_height / BASE_WINDOW_HEIGHT;
                
                // Recalculate sidebar width
                gm->screen.sidebar_width = (int)(BASE_SIDEBAR_WIDTH * gm->screen.scale_x);
                if (gm->screen.sidebar_width < 120) gm->screen.sidebar_width = 120;
                if (gm->screen.sidebar_width > 250) gm->screen.sidebar_width = 250;
                
                // Recalculate game area
                gm->screen.game_area_width = gm->screen.window_width - gm->screen.sidebar_width;
                gm->screen.game_area_height = gm->screen.window_height;
                gm->screen.game_area_x = gm->screen.sidebar_width;
                gm->screen.game_area_y = 0;
                
                float game_scale_x = (float)gm->screen.game_area_width / BASE_GAME_AREA_WIDTH;
                float game_scale_y = (float)gm->screen.game_area_height / BASE_GAME_AREA_HEIGHT;
                gm->screen.game_scale = (game_scale_x < game_scale_y) ? game_scale_x : game_scale_y;
                
                // Recreate XImage with new dimensions
                if (gm->ximage) {
                    free(gm->ximage->data);
                    gm->ximage->data = NULL;
                    XDestroyImage(gm->ximage);
                }
                
                int s = DefaultScreen(gm->display);
                char* imgdata = malloc(gm->screen.window_width * gm->screen.window_height * 4);
                gm->ximage = XCreateImage(gm->display, DefaultVisual(gm->display, s), 24, ZPixmap, 0,
                                          imgdata, gm->screen.window_width, gm->screen.window_height, 32, 0);
            }
        }
        
        if (e.type == KeyPress) {
            int k = x11_to_keycode(XLookupKeysym(&e.xkey, 0));
            if (k >= 112 && k <= 115) gm_switch(gm, k - 112);
            else if (gm->current_game >= 0 && gm->current_game < gm->game_count) {
                GameProcess* game = gm->games[gm->current_game];
                if (game && game->active && !game->has_error && !game->error_dialog)
                    send_input_to_game(gm, game, 1, k, 1, 0, 0);
            }
        }
        else if (e.type == KeyRelease) {
            int k = x11_to_keycode(XLookupKeysym(&e.xkey, 0));
            if (k && gm->current_game >= 0 && gm->current_game < gm->game_count) {
                GameProcess* game = gm->games[gm->current_game];
                if (game && game->active && !game->has_error && !game->error_dialog)
                    send_input_to_game(gm, game, 1, k, 0, 0, 0);
            }
        }
        else if (e.type == ButtonPress) {
            gm_click(gm, e.xbutton.x, e.xbutton.y);
        }
        else if (e.type == MotionNotify) {
            gm->mouse_x = e.xmotion.x;
            gm->mouse_y = e.xmotion.y;
            
            // Map to base coordinates for hover tracking
            int base_x, base_y;
            screen_to_base_coords(gm, e.xmotion.x, e.xmotion.y, &base_x, &base_y);
            
            gm->hover_button = -1;
            
            // Hover tracking for error dialog buttons
            if (gm->current_game >= 0 && gm->current_game < gm->game_count) {
                GameProcess* game = gm->games[gm->current_game];
                if (game && game->error_dialog && game->has_error) {
                    int dx = BASE_GAME_AREA_X + 100;
                    int dy = BASE_GAME_AREA_Y + 150;
                    int dw = BASE_GAME_AREA_WIDTH - 200;
                    int dh = gm->dev_mode ? 320 : 280;
                    int btn_w = 130;
                    int btn_h = 40;
                    int btn_y = dy + dh - 80;
                    int restart_x = dx + (dw/2) - btn_w - 15;
                    int close_x = dx + (dw/2) + 15;
                    
                    if (base_x >= restart_x && base_x < restart_x + btn_w &&
                        base_y >= btn_y && base_y < btn_y + btn_h) {
                        game->error_dialog_button = 0;
                    } else if (base_x >= close_x && base_x < close_x + btn_w &&
                               base_y >= btn_y && base_y < btn_y + btn_h) {
                        game->error_dialog_button = 1;
                    } else if (gm->dev_mode) {
                        int copy_y = btn_y + btn_h + 10;
                        int copy_x = dx + (dw/2) - btn_w/2;
                        
                        if (base_x >= copy_x && base_x < copy_x + btn_w &&
                            base_y >= copy_y && base_y < copy_y + btn_h - 5) {
                            game->error_dialog_button = 2;
                        } else {
                            game->error_dialog_button = -1;
                        }
                    } else {
                        game->error_dialog_button = -1;
                    }
                }
            }
            
            // Sidebar hover
            if (base_x < BASE_SIDEBAR_WIDTH) {
                int by = gm->dev_mode ? 65 : 55;
                for (int i = 0; i < gm->game_count; i++) {
                    if (base_y >= by && base_y < by+35 
                        && base_x >= 10 && base_x < BASE_SIDEBAR_WIDTH-32) {
                        gm->hover_button = i;
                        break;
                    }
                    by += 45;
                }
            }
        }
        
        // Handle mouse enter/leave for cursor visibility
        if (e.type == EnterNotify) {
            XDefineCursor(gm->display, gm->window, gm->cursor);
        }
        if (e.type == LeaveNotify) {
            XUndefineCursor(gm->display, gm->window);
        }
    }
}

static void x11_cleanup(GameManager* gm) {
    if (!gm) return;
    if (gm->cursor) { XFreeCursor(gm->display, gm->cursor); gm->cursor = 0; }
    if (gm->ximage) { free(gm->ximage->data); gm->ximage->data = NULL; XDestroyImage(gm->ximage); gm->ximage = NULL; }
    if (gm->gc) { XFreeGC(gm->display, gm->gc); gm->gc = NULL; }
    if (gm->window) { XDestroyWindow(gm->display, gm->window); gm->window = 0; }
    if (gm->display) { XCloseDisplay(gm->display); gm->display = NULL; }
}

// =============================================================================
// WEB SERVER (unchanged, serves base resolution)
// =============================================================================

typedef struct {
    int fd;
    GameManager* gm;
    pthread_t th;
    int run;
} WebServer;

static WebServer ws;

static const char* html =
    "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nConnection: close\r\n\r\n"
    "<!DOCTYPE html><html><head><title>SPANE Engine</title><style>"
    "body{margin:0;background:#000;display:flex;justify-content:center;align-items:center;height:100vh;overflow:hidden}"
    "canvas{image-rendering:pixelated;cursor:crosshair}"
    "#s{position:fixed;top:10px;left:10px;color:#0f0;font-size:14px;background:rgba(0,0,0,.7);padding:8px;border-radius:4px}"
    "</style></head><body><div id=s>Connecting...</div><canvas id=g></canvas><script>"
    "const s=document.getElementById('s'),c=document.getElementById('g'),x=c.getContext('2d'),W=1000,H=700;c.width=W;c.height=H;"
    "let img=x.createImageData(W,H),fc=0,lt=Date.now(),fps=0,con=0;"
    "function rs(){let sc=Math.min((innerWidth-20)/W,(innerHeight-20)/H,2);c.style.width=Math.floor(W*sc)+'px';c.style.height=Math.floor(H*sc)+'px';}"
    "addEventListener('resize',rs);rs();"
    "function ff(){fetch('/f?'+Date.now()).then(r=>r.arrayBuffer()).then(b=>{"
    "let p=new Uint8Array(b);if(p.length===W*H*4){img.data.set(p);x.putImageData(img,0,0);"
    "if(!con){con=1}s.textContent='Connected | FPS: '+fps;fc++;let n=Date.now();"
    "if(n-lt>=1000){fps=fc;fc=0;lt=n;}}setTimeout(ff,16);}).catch(e=>{con=0;s.textContent='Reconnecting...';setTimeout(ff,500);});}"
    "function si(p){fetch('/i?'+p,{method:'POST',cache:'no-cache'}).catch(()=>{});}"
    "c.addEventListener('mousemove',e=>{let r=c.getBoundingClientRect();"
    "si('t=m&x='+Math.round((e.clientX-r.left)*W/r.width)+'&y='+Math.round((e.clientY-r.top)*H/r.height));});"
    "c.addEventListener('mousedown',e=>{let r=c.getBoundingClientRect();"
    "si('t=c&x='+Math.round((e.clientX-r.left)*W/r.width)+'&y='+Math.round((e.clientY-r.top)*H/r.height)+'&b='+e.button);e.preventDefault();});"
    "c.addEventListener('contextmenu',e=>e.preventDefault());"
    "document.addEventListener('keydown',e=>{si('t=kd&k='+e.keyCode);e.preventDefault();});"
    "document.addEventListener('keyup',e=>{si('t=ku&k='+e.keyCode);e.preventDefault();});"
    "ff();</script></body></html>\n";

static void whandle(WebServer* w, int cf) {
    if (!w || !w->gm) return;
    
    char b[16384];
    int n = recv(cf, b, sizeof(b)-1, 0);
    if (n <= 0) return;
    b[n] = 0;
    
    char m[16], p[512];
    sscanf(b, "%15s %511s", m, p);
    
    if (strcmp(p, "/") == 0 || strcmp(p, "/index.html") == 0) {
        send(cf, html, strlen(html), 0);
    }
    else if (strncmp(p, "/f", 2) == 0) {
        pthread_mutex_lock(&w->gm->fb_mutex);
        int sz = BASE_WINDOW_WIDTH * BASE_WINDOW_HEIGHT * 4;
        char h[256];
        int hl = snprintf(h, sizeof(h),
            "HTTP/1.1 200 OK\r\nContent-Type: application/octet-stream\r\n"
            "Content-Length: %d\r\nCache-Control: no-cache,no-store\r\nConnection: close\r\n\r\n", sz);
        send(cf, h, hl, 0);
        int sent = 0;
        while (sent < sz) {
            int c = send(cf, w->gm->framebuffer.pixels + sent, sz - sent, 0);
            if (c <= 0) break;
            sent += c;
        }
        pthread_mutex_unlock(&w->gm->fb_mutex);
    }
    else if (strncmp(p, "/i?", 3) == 0) {
        char* q = strchr(p, '?') + 1;
        char t[8] = "";
        int x = 0, y = 0, k = 0;
        char* tk = strtok(q, "&");
        while (tk) {
            if (strncmp(tk, "t=", 2) == 0) sscanf(tk+2, "%7s", t);
            else if (strncmp(tk, "x=", 2) == 0) sscanf(tk+2, "%d", &x);
            else if (strncmp(tk, "y=", 2) == 0) sscanf(tk+2, "%d", &y);
            else if (strncmp(tk, "k=", 2) == 0) sscanf(tk+2, "%d", &k);
            tk = strtok(NULL, "&");
        }
        pthread_mutex_lock(&w->gm->fb_mutex);
        if (strcmp(t, "m") == 0) {
            w->gm->mouse_x = x;
            w->gm->mouse_y = y;
            w->gm->hover_button = -1;
            
            if (w->gm->current_game >= 0 && w->gm->current_game < w->gm->game_count) {
                GameProcess* game = w->gm->games[w->gm->current_game];
                if (game && game->error_dialog && game->has_error) {
                    int dx = BASE_GAME_AREA_X + 100;
                    int dy = BASE_GAME_AREA_Y + 150;
                    int dw = BASE_GAME_AREA_WIDTH - 200;
                    int dh = w->gm->dev_mode ? 320 : 280;
                    int btn_w = 130;
                    int btn_h = 40;
                    int btn_y = dy + dh - 80;
                    int restart_x = dx + (dw/2) - btn_w - 15;
                    int close_x = dx + (dw/2) + 15;
                    
                    if (x >= restart_x && x < restart_x + btn_w &&
                        y >= btn_y && y < btn_y + btn_h) {
                        game->error_dialog_button = 0;
                    } else if (x >= close_x && x < close_x + btn_w &&
                               y >= btn_y && y < btn_y + btn_h) {
                        game->error_dialog_button = 1;
                    } else if (w->gm->dev_mode) {
                        int copy_y = btn_y + btn_h + 10;
                        int copy_x = dx + (dw/2) - btn_w/2;
                        
                        if (x >= copy_x && x < copy_x + btn_w &&
                            y >= copy_y && y < copy_y + btn_h - 5) {
                            game->error_dialog_button = 2;
                        } else {
                            game->error_dialog_button = -1;
                        }
                    } else {
                        game->error_dialog_button = -1;
                    }
                }
            }
            
            if (x < BASE_SIDEBAR_WIDTH) {
                int by = w->gm->dev_mode ? 65 : 55;
                for (int i = 0; i < w->gm->game_count; i++) {
                    if (y >= by && y < by+35 && x >= 10 && x < BASE_SIDEBAR_WIDTH-32) {
                        w->gm->hover_button = i;
                        break;
                    }
                    by += 45;
                }
            }
        }
        else if (strcmp(t, "c") == 0) {
            gm_click(w->gm, x, y);
        }
        else if (strcmp(t, "kd") == 0 || strcmp(t, "ku") == 0) {
            int pr = (t[1] == 'd');
            if (k == 27) w->run = 0;
            else if (k >= 112 && k <= 115 && pr) gm_switch(w->gm, k - 112);
            else if (w->gm->current_game >= 0 && w->gm->current_game < w->gm->game_count) {
                GameProcess* game = w->gm->games[w->gm->current_game];
                if (game && game->active && !game->has_error && !game->error_dialog)
                    send_input_to_game(w->gm, game, 1, k, pr, 0, 0);
            }
        }
        pthread_mutex_unlock(&w->gm->fb_mutex);
        send(cf, "HTTP/1.1 200 OK\r\nContent-Length: 2\r\n\r\nOK", 40, 0);
    }
    else {
        send(cf, "HTTP/1.1 404\r\nContent-Length: 0\r\n\r\n", 38, 0);
    }
}

static void* wthread(void* a) {
    WebServer* w = (WebServer*)a;
    if (!w) return NULL;
    
    w->fd = socket(AF_INET, SOCK_STREAM, 0);
    int o = 1;
    setsockopt(w->fd, SOL_SOCKET, SO_REUSEADDR, &o, sizeof(o));
    
    struct sockaddr_in ad;
    memset(&ad, 0, sizeof(ad));
    ad.sin_family = AF_INET;
    ad.sin_addr.s_addr = INADDR_ANY;
    ad.sin_port = htons(WEB_PORT);
    
    bind(w->fd, (struct sockaddr*)&ad, sizeof(ad));
    listen(w->fd, 10);
    
    printf("\n========================================\n");
    printf("  Web: http://localhost:%d\n", WEB_PORT);
    printf("========================================\n\n");
    
    w->run = 1;
    while (w->run) {
        fd_set f;
        FD_ZERO(&f);
        FD_SET(w->fd, &f);
        struct timeval tv = {0, 50000};
        
        if (select(w->fd+1, &f, 0, 0, &tv) > 0) {
            struct sockaddr_in ca;
            socklen_t cl = sizeof(ca);
            int cf = accept(w->fd, (struct sockaddr*)&ca, &cl);
            if (cf >= 0) {
                struct timeval to = {5, 0};
                setsockopt(cf, SOL_SOCKET, SO_RCVTIMEO, &to, sizeof(to));
                whandle(w, cf);
                close(cf);
            }
        }
    }
    close(w->fd);
    return NULL;
}

// =============================================================================
// MAIN
// =============================================================================

int main(int argc, char** argv) {
    signal(SIGCHLD, SIG_IGN);
    signal(SIGPIPE, SIG_IGN);
    
    GameManager gm;
    memset(&gm, 0, sizeof(gm));
    pthread_mutex_init(&gm.fb_mutex, NULL);
    gm.current_game = -1;
    gm.shm_fd = -1;
    gm.wm_delete_window = 0;
    gm.dev_mode = 0;
    
    int web = 0, clear_state = 0;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--web") == 0) web = 1;
        else if (strcmp(argv[i], "--clear") == 0) clear_state = 1;
        else if (strcmp(argv[i], "--dev") == 0) gm.dev_mode = 1;
    }
    
    ensure_state_directory(&gm);
    
    if (!init_shared_state(&gm)) {
        printf("Running without shared memory sync\n");
    }
    
    if (clear_state) {
        clear_all_state(&gm);
        printf("State cleared.\n");
        cleanup_shared_state(&gm);
        return 0;
    }
    
    int has_x11 = 0;
    if (!web) {
        has_x11 = x11_init(&gm);
        if (!has_x11) {
            printf("No display available - using web mode\n");
            web = 1;
        }
    }
    
    gm.web_mode = web;
    
    if (web) {
        ws.gm = &gm;
        pthread_create(&ws.th, NULL, wthread, &ws);
        usleep(500000);
    }
    
    printf("SPANE Engine - Instance #%d (%s%s)\n", 
           gm.instance_id + 1, 
           web ? "Web" : "X11",
           gm.dev_mode ? " DEV" : "");
    printf("Process isolation: ENABLED\n");
    if (gm.dev_mode) {
        printf("Developer mode: ENABLED (clipboard copy available)\n");
    }
    
    srand(time(NULL));
    gm.last_fps_update = time(NULL);
    
    if (!load_disk_backup(&gm)) {
        scan_games_directory(&gm, "./games");
    }
    if (gm.shared) sync_from_shared(&gm);
    
    if (gm.game_count > 0) {
        gm.current_game = 0;
        sync_to_shared(&gm);
    }
    
    int running = 1;
    int counter = 0;
    
    while (running) {
        if (has_x11) x11_process(&gm, &running);
        if (web && !ws.run) running = 0;
        
        check_game_processes(&gm);
        
        gm_render(&gm);
        if (has_x11) x11_mirror_frame(&gm);
        gm_fps(&gm);
        
        counter++;
        if (counter >= 10) {
            if (gm.shared) sync_from_shared(&gm);
            if (counter >= 300) {
                save_disk_backup(&gm);
                counter = 0;
            }
        }
        
        usleep(16667);
    }
    
    printf("Shutting down...\n");
    save_disk_backup(&gm);
    
    for (int i = 0; i < gm.game_count; i++) {
        if (gm.games[i]) {
            stop_game_process(&gm, gm.games[i]);
            free(gm.games[i]);
            gm.games[i] = NULL;
        }
    }
    
    if (has_x11) x11_cleanup(&gm);
    if (web) {
        ws.run = 0;
        pthread_join(ws.th, NULL);
    }
    cleanup_shared_state(&gm);
    pthread_mutex_destroy(&gm.fb_mutex);
    
    return 0;
}