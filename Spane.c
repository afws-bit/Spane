#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/Xutil.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

#define MAIN_WINDOW_WIDTH  1000
#define MAIN_WINDOW_HEIGHT 700
#define GAME_AREA_WIDTH    800
#define GAME_AREA_HEIGHT   600
#define GAME_AREA_X        180
#define GAME_AREA_Y        50
#define SIDEBAR_WIDTH      160

#define TILE_SIZE     32
#define MAP_WIDTH     25
#define MAP_HEIGHT    18
#define PLAYER_SPEED  5

#define WEB_PORT 3000

// 5x7 bitmap font (ASCII 32-126)
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

typedef struct Game Game;
typedef struct GameManager GameManager;

// THE framebuffer - single source of truth for all rendering
typedef struct {
    unsigned char pixels[MAIN_WINDOW_WIDTH * MAIN_WINDOW_HEIGHT * 4];
} Framebuffer;

struct Game {
    char name[32];
    void* data;
    int active;
    void (*init)(Game* game);
    void (*handle_key)(Game* game, int key_code, int pressed);
    void (*handle_click)(Game* game, int x, int y);
    void (*update)(Game* game);
    void (*render)(Game* game, Framebuffer* fb);
    void (*cleanup)(Game* game);
};

struct GameManager {
    // Engine state (runs headless)
    Framebuffer framebuffer;
    Game* games[5];
    int game_count;
    int current_game;
    int hover_button;
    int mouse_x, mouse_y;
    int frame_count;
    time_t last_fps_update;
    double fps;
    pthread_mutex_t fb_mutex;
    
    // X11 mirror (just displays framebuffer)
    Display* display;
    Window window;
    GC gc;
    XImage* ximage;
    int x11_running;
    
    // Web mirror
    int web_mode;
};

// ============================================
// FRAMEBUFFER PRIMITIVES (Headless rendering)
// ============================================

static inline void fb_pixel(Framebuffer* fb, int x, int y, unsigned char r, unsigned char g, unsigned char b) {
    if (x < 0 || x >= MAIN_WINDOW_WIDTH || y < 0 || y >= MAIN_WINDOW_HEIGHT) return;
    int i = (y * MAIN_WINDOW_WIDTH + x) * 4;
    fb->pixels[i]=r; fb->pixels[i+1]=g; fb->pixels[i+2]=b; fb->pixels[i+3]=255;
}

static void fb_fill(Framebuffer* fb, int x, int y, int w, int h, unsigned char r, unsigned char g, unsigned char b) {
    for (int dy=0; dy<h; dy++)
        for (int dx=0; dx<w; dx++)
            fb_pixel(fb, x+dx, y+dy, r, g, b);
}

static void fb_rect(Framebuffer* fb, int x, int y, int w, int h, unsigned char r, unsigned char g, unsigned char b) {
    for (int dx=0; dx<w; dx++) { fb_pixel(fb,x+dx,y,r,g,b); fb_pixel(fb,x+dx,y+h-1,r,g,b); }
    for (int dy=1; dy<h-1; dy++) { fb_pixel(fb,x,y+dy,r,g,b); fb_pixel(fb,x+w-1,y+dy,r,g,b); }
}

static void fb_circle(Framebuffer* fb, int cx, int cy, int r, unsigned char R, unsigned char G, unsigned char B) {
    for (int dy=-r; dy<=r; dy++)
        for (int dx=-r; dx<=r; dx++)
            if (dx*dx+dy*dy <= r*r) fb_pixel(fb, cx+dx, cy+dy, R, G, B);
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

static void fb_text_center(Framebuffer* fb, int x, int y, int w, const char* s, unsigned char r, unsigned char g, unsigned char b) {
    fb_text(fb, x+(w-(int)strlen(s)*6)/2, y, s, r, g, b);
}

// ============================================
// RPG GAME (Headless rendering)
// ============================================

typedef struct {
    int map[18][25];
    int px, py, dir, frame, moving;
    int ku,kd,kl,kr;
} RPGData;

static int rpg_map[18][25] = {
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,4,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,3,3,3,0,0,0,0,0,0,4,0,0,5,0,0,0,0,0,0,0,0,1},
    {1,0,0,3,3,3,0,0,0,0,0,0,0,0,0,5,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,4,0,0,0,0,0,0,0,0,0,0,0,2,2,2,0,1},
    {1,0,0,0,0,0,0,0,4,0,0,0,0,0,0,0,0,3,3,3,2,2,2,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3,3,3,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,4,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,5,0,0,0,0,0,0,0,4,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,5,0,0,0,0,0,0,0,0,0,0,0,0,5,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,3,3,3,0,0,0,0,0,0,5,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,3,3,3,0,0,0,0,0,0,0,0,0,0,4,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,4,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,5,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,5,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}
};

static int rpg_walkable(RPGData* d, int x, int y) {
    return (x>=0&&x<MAP_WIDTH&&y>=0&&y<MAP_HEIGHT&&(d->map[y][x]==0||d->map[y][x]==3));
}

static void rpg_tile(Framebuffer* fb, int x, int y, int t) {
    unsigned char r,g,b;
    switch(t) {
        case 0: r=0x33;g=0xAA;b=0x33;break; case 1: r=0x66;g=0x66;b=0x66;break;
        case 2: r=0x33;g=0x55;b=0xAA;break; case 3: r=0xCC;g=0xAA;b=0x66;break;
        case 4: r=0x22;g=0x66;b=0x22;break; case 5: r=0x88;g=0x44;b=0x22;break;
        default: r=g=b=0;
    }
    fb_fill(fb,x,y,TILE_SIZE,TILE_SIZE,r,g,b);
    if(t==4){fb_circle(fb,x+16,y,TILE_SIZE/2+2,0x00,0xCC,0x00);fb_fill(fb,x+13,y+16,6,16,0x66,0x44,0x22);}
    if(t==5){fb_fill(fb,x+4,y+8,24,24,0xCC,0x33,0x33);for(int i=0;i<12;i++)fb_fill(fb,x+12-i,y+8-i,i*2+4,1,0x88,0x44,0x22);}
}

static void rpg_player(Framebuffer* fb, int x, int y, int dir, int frame) {
    fb_fill(fb,x+8,y+12,16,14,0x33,0x66,0xFF);
    fb_circle(fb,x+16,y+10,8,0xFF,0xCC,0x99);
    if(dir==1) fb_fill(fb,x+9,y+8,3,3,0,0,0);
    else if(dir==2) fb_fill(fb,x+20,y+8,3,3,0,0,0);
    else {fb_fill(fb,x+12,y+8,3,3,0,0,0);fb_fill(fb,x+19,y+8,3,3,0,0,0);}
    if(frame%20<10){fb_fill(fb,x+9,y+26,5,8,0,0,0x88);fb_fill(fb,x+18,y+26,5,8,0,0,0x88);}
    else{fb_fill(fb,x+10,y+26,5,8,0,0,0x88);fb_fill(fb,x+17,y+26,5,8,0,0,0x88);}
}

static void rpg_init(Game* g) {
    RPGData* d=calloc(1,sizeof(RPGData));
    memcpy(d->map,rpg_map,sizeof(rpg_map));
    d->px=5*TILE_SIZE; d->py=5*TILE_SIZE;
    g->data=d; printf("RPG ready\n");
}
static void rpg_key(Game* g, int k, int p) {
    RPGData* d=g->data;
    switch(k){
        case 38:case 87: d->ku=p; if(p)d->dir=3; break;
        case 40:case 83: d->kd=p; if(p)d->dir=0; break;
        case 37:case 65: d->kl=p; if(p)d->dir=1; break;
        case 39:case 68: d->kr=p; if(p)d->dir=2; break;
    }
}
static void rpg_click(Game* g, int x, int y) {
    RPGData* d=g->data;
    if(x>=GAME_AREA_X&&x<GAME_AREA_X+GAME_AREA_WIDTH&&y>=GAME_AREA_Y&&y<GAME_AREA_Y+GAME_AREA_HEIGHT){
        int nx=x-GAME_AREA_X-16, ny=y-GAME_AREA_Y-16;
        if(rpg_walkable(d,nx/TILE_SIZE,ny/TILE_SIZE)&&rpg_walkable(d,(nx+31)/TILE_SIZE,(ny+31)/TILE_SIZE))
            {d->px=nx;d->py=ny;}
    }
}
static void rpg_update(Game* g) {
    RPGData* d=g->data; d->moving=0;
    int nx=d->px, ny=d->py;
    if(d->ku){ny-=PLAYER_SPEED;d->moving=1;}
    if(d->kd){ny+=PLAYER_SPEED;d->moving=1;}
    if(d->kl){nx-=PLAYER_SPEED;d->moving=1;}
    if(d->kr){nx+=PLAYER_SPEED;d->moving=1;}
    if(d->moving&&rpg_walkable(d,nx/TILE_SIZE,ny/TILE_SIZE)&&rpg_walkable(d,(nx+31)/TILE_SIZE,ny/TILE_SIZE)
       &&rpg_walkable(d,nx/TILE_SIZE,(ny+31)/TILE_SIZE)&&rpg_walkable(d,(nx+31)/TILE_SIZE,(ny+31)/TILE_SIZE))
        {d->px=nx;d->py=ny;}
    if(d->px<0)d->px=0; if(d->py<0)d->py=0;
    if(d->px>(MAP_WIDTH-1)*TILE_SIZE)d->px=(MAP_WIDTH-1)*TILE_SIZE;
    if(d->py>(MAP_HEIGHT-1)*TILE_SIZE)d->py=(MAP_HEIGHT-1)*TILE_SIZE;
    if(d->moving)d->frame++; else d->frame=0;
}
static void rpg_render(Game* g, Framebuffer* fb) {
    RPGData* d=g->data;
    fb_fill(fb,GAME_AREA_X,GAME_AREA_Y,GAME_AREA_WIDTH,GAME_AREA_HEIGHT,0,0,0);
    for(int y=0;y<MAP_HEIGHT;y++)
        for(int x=0;x<MAP_WIDTH;x++)
            rpg_tile(fb,GAME_AREA_X+x*TILE_SIZE,GAME_AREA_Y+y*TILE_SIZE,d->map[y][x]);
    rpg_player(fb,GAME_AREA_X+d->px,GAME_AREA_Y+d->py,d->dir,d->frame);
    char s[200]; snprintf(s,sizeof(s),"RPG Adventure - Pos:(%d,%d) - ArrowKeys/WASD",d->px/TILE_SIZE,d->py/TILE_SIZE);
    fb_text(fb,GAME_AREA_X+10,GAME_AREA_Y+GAME_AREA_HEIGHT-10,s,0xFF,0xFF,0xFF);
}
static void rpg_cleanup(Game* g) { free(g->data); }

// ============================================
// SNAKE GAME (Headless rendering)
// ============================================

#define SG 20
#define SML 500
#define SAW 800
#define SAH 600
#define SNC (SAW/SG)
#define SNR (SAH/SG)

typedef struct {int x,y;} Pt;
typedef struct {
    Pt s[SML]; int len,dir,ndir,score,go,fc,spd; Pt food;
} SnakeData;

static void sn_food(SnakeData* d) {
    int v; do{v=1;d->food.x=rand()%SNC;d->food.y=rand()%SNR;
        for(int i=0;i<d->len;i++) if(d->s[i].x==d->food.x&&d->s[i].y==d->food.y){v=0;break;}
    }while(!v);
}
static void sn_init(Game* g) {
    SnakeData* d=calloc(1,sizeof(SnakeData)); d->len=3; int sx=SNC/2,sy=SNR/2;
    for(int i=0;i<d->len;i++){d->s[i].x=sx-i;d->s[i].y=sy;}
    d->spd=10; sn_food(d); g->data=d; printf("Snake ready\n");
}
static void sn_key(Game* g, int k, int p) {
    SnakeData* d=g->data; if(!p)return;
    if(d->go){if(k==82){d->len=3;int sx=SNC/2,sy=SNR/2;for(int i=0;i<d->len;i++){d->s[i].x=sx-i;d->s[i].y=sy;}
        d->dir=0;d->ndir=0;d->score=0;d->go=0;d->fc=0;sn_food(d);}return;}
    switch(k){case 38:case 87:if(d->dir!=1)d->ndir=3;break;case 40:case 83:if(d->dir!=3)d->ndir=1;break;
        case 37:case 65:if(d->dir!=0)d->ndir=2;break;case 39:case 68:if(d->dir!=2)d->ndir=0;break;}
}
static void sn_click(Game* g, int x, int y) { (void)g;(void)x;(void)y; }
static void sn_update(Game* g) {
    SnakeData* d=g->data; if(d->go)return;
    if(++d->fc<d->spd)return; d->fc=0; d->dir=d->ndir;
    Pt nh=d->s[0]; switch(d->dir){case 0:nh.x++;break;case 1:nh.y++;break;case 2:nh.x--;break;case 3:nh.y--;break;}
    if(nh.x<0||nh.x>=SNC||nh.y<0||nh.y>=SNR){d->go=1;return;}
    for(int i=0;i<d->len;i++) if(d->s[i].x==nh.x&&d->s[i].y==nh.y){d->go=1;return;}
    int ate=(nh.x==d->food.x&&nh.y==d->food.y);
    for(int i=d->len-1;i>0;i--) d->s[i]=d->s[i-1]; d->s[0]=nh;
    if(ate){d->len++;d->score+=10;if(d->spd>3)d->spd--;sn_food(d);}
}
static void sn_render(Game* g, Framebuffer* fb) {
    SnakeData* d=g->data;
    fb_fill(fb,GAME_AREA_X,GAME_AREA_Y,SAW,SAH,0x1A,0x1A,0x2E);
    for(int i=0;i<=SNC;i++)fb_fill(fb,GAME_AREA_X+i*SG,GAME_AREA_Y,1,SAH,0x25,0x25,0x40);
    for(int i=0;i<=SNR;i++)fb_fill(fb,GAME_AREA_X,GAME_AREA_Y+i*SG,SAW,1,0x25,0x25,0x40);
    int fx=GAME_AREA_X+d->food.x*SG,fy=GAME_AREA_Y+d->food.y*SG;
    fb_circle(fb,fx+SG/2,fy+SG/2,SG/2-2,0xFF,0x33,0x33);
    for(int i=0;i<d->len;i++){
        int sx=GAME_AREA_X+d->s[i].x*SG+1,sy=GAME_AREA_Y+d->s[i].y*SG+1;
        fb_fill(fb,sx,sy,SG-2,SG-2,i==0?0x00:0x00,i==0?0xFF:0xAA,0x00);
    }
    char t[50]; snprintf(t,sizeof(t),"Score: %d",d->score);
    fb_text(fb,GAME_AREA_X+10,GAME_AREA_Y+20,t,0xFF,0xFF,0xFF);
    if(d->go){fb_fill(fb,GAME_AREA_X+250,GAME_AREA_Y+250,300,100,0,0,0);
        fb_text(fb,GAME_AREA_X+340,GAME_AREA_Y+280,"GAME OVER!",0xFF,0,0);
        fb_text(fb,GAME_AREA_X+320,GAME_AREA_Y+310,"Press R to restart",0xFF,0xFF,0xFF);}
    snprintf(t,sizeof(t),"Snake - Arrows/WASD - Length:%d",d->len);
    fb_text(fb,GAME_AREA_X+10,GAME_AREA_Y+SAH-10,t,0x88,0x88,0x88);
}
static void sn_cleanup(Game* g) { free(g->data); }

// ============================================
// ENGINE (Headless game manager)
// ============================================

static void gm_add(GameManager* gm, Game* g) {
    if(gm->game_count<5) gm->games[gm->game_count++]=g;
}
static void gm_switch(GameManager* gm, int i) {
    if(i>=0&&i<gm->game_count&&i!=gm->current_game){gm->current_game=i;printf("Switch: %s\n",gm->games[i]->name);}
}
static void gm_click(GameManager* gm, int x, int y) {
    if(x<SIDEBAR_WIDTH){
        int by=70;
        for(int i=0;i<gm->game_count;i++){
            if(y>=by&&y<by+40&&x>=15&&x<SIDEBAR_WIDTH-15){gm_switch(gm,i);return;}
            by+=55;
        }
    } else if(gm->games[gm->current_game]->active)
        gm->games[gm->current_game]->handle_click(gm->games[gm->current_game],x,y);
}
static void gm_render(GameManager* gm) {
    pthread_mutex_lock(&gm->fb_mutex);
    Framebuffer* fb=&gm->framebuffer;
    
    // Clear
    fb_fill(fb,0,0,MAIN_WINDOW_WIDTH,MAIN_WINDOW_HEIGHT,0x1A,0x1A,0x1A);
    
    // Sidebar
    fb_fill(fb,0,0,SIDEBAR_WIDTH,MAIN_WINDOW_HEIGHT,0x1A,0x1A,0x1A);
    fb_text_center(fb,0,25,SIDEBAR_WIDTH,"GAME HUB",0xFF,0xFF,0xFF);
    fb_fill(fb,10,45,SIDEBAR_WIDTH-20,2,0xCC,0xCC,0xCC);
    int by=70;
    for(int i=0;i<gm->game_count;i++){
        int act=(i==gm->current_game), hov=(i==gm->hover_button);
        unsigned char r,g,b;
        if(act){r=0x33;g=0x55;b=0xAA;}else if(hov){r=0xCC;g=0xAA;b=0x33;}else{r=0x33;g=0x33;b=0x33;}
        fb_fill(fb,15,by,SIDEBAR_WIDTH-30,40,r,g,b);
        fb_rect(fb,15,by,SIDEBAR_WIDTH-30,40,0xCC,0xCC,0xCC);
        fb_text_center(fb,15,by+16,SIDEBAR_WIDTH-30,gm->games[i]->name,0xFF,0xFF,0xFF);
        by+=55;
    }
    int bot=MAIN_WINDOW_HEIGHT-120;
    fb_fill(fb,10,bot,SIDEBAR_WIDTH-20,2,0xCC,0xCC,0xCC);
    char fs[32]; snprintf(fs,sizeof(fs),"FPS: %.1f",gm->fps);
    fb_text(fb,15,bot+15,fs,0xFF,0xFF,0xFF);
    fb_text(fb,15,bot+35,"ESC to quit",0xFF,0xFF,0xFF);
    
    // Border
    for(int t=0;t<3;t++)
        fb_rect(fb,GAME_AREA_X-t-1,GAME_AREA_Y-t-1,GAME_AREA_WIDTH+2*t+2,GAME_AREA_HEIGHT+2*t+2,0xCC,0xCC,0xCC);
    char lb[64]; snprintf(lb,sizeof(lb),"Current Game: %s",gm->games[gm->current_game]->name);
    fb_text(fb,GAME_AREA_X,GAME_AREA_Y-15,lb,0xFF,0xFF,0xFF);
    
    // Game
    if(gm->games[gm->current_game]->active)
        gm->games[gm->current_game]->render(gm->games[gm->current_game],fb);
    
    pthread_mutex_unlock(&gm->fb_mutex);
}
static void gm_fps(GameManager* gm) {
    gm->frame_count++; time_t n=time(NULL);
    if(n-gm->last_fps_update>=1){gm->fps=gm->frame_count/(double)(n-gm->last_fps_update);gm->frame_count=0;gm->last_fps_update=n;}
}

// ============================================
// X11 MIRROR (Just displays the framebuffer)
// ============================================

static void x11_mirror_frame(GameManager* gm) {
    if(!gm->display||!gm->ximage) return;
    pthread_mutex_lock(&gm->fb_mutex);
    // Convert RGBA framebuffer to X11's BGRX format
    unsigned char* src = gm->framebuffer.pixels;
    char* dst = gm->ximage->data;
    for(int i=0; i<MAIN_WINDOW_WIDTH*MAIN_WINDOW_HEIGHT; i++) {
        dst[i*4+0] = src[i*4+2]; // B
        dst[i*4+1] = src[i*4+1]; // G
        dst[i*4+2] = src[i*4+0]; // R
        dst[i*4+3] = 0;           // X (unused)
    }
    XPutImage(gm->display, gm->window, gm->gc, gm->ximage, 0,0,0,0, MAIN_WINDOW_WIDTH, MAIN_WINDOW_HEIGHT);
    XFlush(gm->display);
    pthread_mutex_unlock(&gm->fb_mutex);
}

static int x11_init(GameManager* gm) {
    gm->display = XOpenDisplay(NULL);
    if(!gm->display) return 0;
    
    int s = DefaultScreen(gm->display);
    gm->window = XCreateSimpleWindow(gm->display, RootWindow(gm->display,s),
                                     50,50,MAIN_WINDOW_WIDTH,MAIN_WINDOW_HEIGHT,1,
                                     BlackPixel(gm->display,s), 0x1A1A1A);
    XSelectInput(gm->display, gm->window,
                 ExposureMask|KeyPressMask|KeyReleaseMask|ButtonPressMask|ButtonReleaseMask|PointerMotionMask|StructureNotifyMask);
    XStoreName(gm->display, gm->window, "Game Hub");
    XMapWindow(gm->display, gm->window);
    gm->gc = XCreateGC(gm->display, gm->window, 0, NULL);
    
    // Create XImage for mirroring
    char* imgdata = malloc(MAIN_WINDOW_WIDTH * MAIN_WINDOW_HEIGHT * 4);
    gm->ximage = XCreateImage(gm->display, DefaultVisual(gm->display,s), 24, ZPixmap, 0,
                              imgdata, MAIN_WINDOW_WIDTH, MAIN_WINDOW_HEIGHT, 32, 0);
    
    // Wait for window
    XEvent e;
    do { XNextEvent(gm->display, &e); } while(e.type != MapNotify);
    printf("X11 mirror ready\n");
    return 1;
}

static int x11_to_keycode(KeySym ks) {
    if(ks==XK_Escape) return 27;
    if(ks==XK_F1) return 112; if(ks==XK_F2) return 113;
    if(ks==XK_Up||ks==XK_w||ks==XK_W) return 38;
    if(ks==XK_Down||ks==XK_s||ks==XK_S) return 40;
    if(ks==XK_Left||ks==XK_a||ks==XK_A) return 37;
    if(ks==XK_Right||ks==XK_d||ks==XK_D) return 39;
    if(ks==XK_r||ks==XK_R) return 82;
    return 0;
}

static void x11_process(GameManager* gm, int* running) {
    while(XPending(gm->display)) {
        XEvent e; XNextEvent(gm->display, &e);
        if(e.type == KeyPress) {
            int k = x11_to_keycode(XLookupKeysym(&e.xkey,0));
            if(k==27) *running=0;
            else if(k==112) gm_switch(gm,0);
            else if(k==113) gm_switch(gm,1);
            else if(gm->games[gm->current_game]->active)
                gm->games[gm->current_game]->handle_key(gm->games[gm->current_game],k,1);
        }
        else if(e.type == KeyRelease) {
            int k = x11_to_keycode(XLookupKeysym(&e.xkey,0));
            if(k&&gm->games[gm->current_game]->active)
                gm->games[gm->current_game]->handle_key(gm->games[gm->current_game],k,0);
        }
        else if(e.type == ButtonPress) gm_click(gm, e.xbutton.x, e.xbutton.y);
        else if(e.type == MotionNotify) {
            gm->mouse_x=e.xmotion.x; gm->mouse_y=e.xmotion.y; gm->hover_button=-1;
            if(e.xmotion.x<SIDEBAR_WIDTH) {
                int by=70;
                for(int i=0;i<gm->game_count;i++) {
                    if(e.xmotion.y>=by&&e.xmotion.y<by+40&&e.xmotion.x>=15&&e.xmotion.x<SIDEBAR_WIDTH-15)
                        {gm->hover_button=i;break;}
                    by+=55;
                }
            }
        }
    }
}

static void x11_cleanup(GameManager* gm) {
    if(gm->ximage) { free(gm->ximage->data); gm->ximage->data=NULL; XDestroyImage(gm->ximage); }
    if(gm->gc) XFreeGC(gm->display, gm->gc);
    if(gm->window) XDestroyWindow(gm->display, gm->window);
    if(gm->display) XCloseDisplay(gm->display);
}

// ============================================
// WEB SERVER MIRROR
// ============================================

typedef struct { int fd; GameManager* gm; pthread_t th; int run; } WS;
static WS ws;

static const char* html =
    "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nConnection: close\r\n\r\n"
    "<!DOCTYPE html><html><head><title>Game Hub</title><style>"
    "body{margin:0;background:#000;display:flex;justify-content:center;align-items:center;height:100vh;overflow:hidden}"
    "canvas{image-rendering:pixelated;cursor:crosshair}"
    "#s{position:fixed;top:10px;left:10px;color:#0f0;font-size:14px;background:rgba(0,0,0,.7);padding:8px 12px;border-radius:4px;z-index:10}"
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

static void whandle(WS* w, int cf) {
    char b[16384]; int n=recv(cf,b,sizeof(b)-1,0);
    if(n<=0) return; b[n]=0;
    char m[16],p[512]; sscanf(b,"%15s %511s",m,p);
    
    if(strcmp(p,"/")==0||strcmp(p,"/index.html")==0) send(cf,html,strlen(html),0);
    else if(strncmp(p,"/f",2)==0) {
        pthread_mutex_lock(&w->gm->fb_mutex);
        int sz=MAIN_WINDOW_WIDTH*MAIN_WINDOW_HEIGHT*4;
        char h[256]; int hl=snprintf(h,sizeof(h),"HTTP/1.1 200 OK\r\nContent-Type: application/octet-stream\r\nContent-Length: %d\r\nCache-Control: no-cache,no-store\r\nConnection: close\r\n\r\n",sz);
        send(cf,h,hl,0);
        int sent=0;
        while(sent<sz){int c=send(cf,w->gm->framebuffer.pixels+sent,sz-sent,0);if(c<=0)break;sent+=c;}
        pthread_mutex_unlock(&w->gm->fb_mutex);
    }
    else if(strncmp(p,"/i?",3)==0) {
        char*q=strchr(p,'?')+1,t[8]=""; int x=0,y=0,k=0;
        char*tk=strtok(q,"&");
        while(tk){
            if(strncmp(tk,"t=",2)==0)sscanf(tk+2,"%7s",t);
            else if(strncmp(tk,"x=",2)==0)sscanf(tk+2,"%d",&x);
            else if(strncmp(tk,"y=",2)==0)sscanf(tk+2,"%d",&y);
            else if(strncmp(tk,"k=",2)==0)sscanf(tk+2,"%d",&k);
            tk=strtok(NULL,"&");
        }
        pthread_mutex_lock(&w->gm->fb_mutex);
        if(strcmp(t,"m")==0){
            w->gm->mouse_x=x;w->gm->mouse_y=y;w->gm->hover_button=-1;
            if(x<SIDEBAR_WIDTH){int by=70;for(int i=0;i<w->gm->game_count;i++){if(y>=by&&y<by+40&&x>=15&&x<SIDEBAR_WIDTH-15){w->gm->hover_button=i;break;}by+=55;}}
        }
        else if(strcmp(t,"c")==0) gm_click(w->gm,x,y);
        else if(strcmp(t,"kd")==0||strcmp(t,"ku")==0){
            int pr=(t[1]=='d');
            if(k==27)w->run=0;
            else if(k==112&&pr)gm_switch(w->gm,0);
            else if(k==113&&pr)gm_switch(w->gm,1);
            else if(w->gm->games[w->gm->current_game]->active)w->gm->games[w->gm->current_game]->handle_key(w->gm->games[w->gm->current_game],k,pr);
        }
        pthread_mutex_unlock(&w->gm->fb_mutex);
        send(cf,"HTTP/1.1 200 OK\r\nContent-Length: 2\r\n\r\nOK",40,0);
    } else send(cf,"HTTP/1.1 404\r\nContent-Length: 0\r\n\r\n",38,0);
}

static void* wthread(void* a) {
    WS* w=(WS*)a;
    w->fd=socket(AF_INET,SOCK_STREAM,0);
    int o=1;setsockopt(w->fd,SOL_SOCKET,SO_REUSEADDR,&o,sizeof(o));
    struct sockaddr_in ad;memset(&ad,0,sizeof(ad));ad.sin_family=AF_INET;ad.sin_addr.s_addr=INADDR_ANY;ad.sin_port=htons(WEB_PORT);
    bind(w->fd,(struct sockaddr*)&ad,sizeof(ad));listen(w->fd,10);
    printf("\n========================================\n  Web: http://localhost:%d\n========================================\n\n",WEB_PORT);
    w->run=1;
    while(w->run){
        fd_set f;FD_ZERO(&f);FD_SET(w->fd,&f);struct timeval tv={0,50000};
        if(select(w->fd+1,&f,0,0,&tv)>0){
            struct sockaddr_in ca;socklen_t cl=sizeof(ca);int cf=accept(w->fd,(struct sockaddr*)&ca,&cl);
            if(cf>=0){struct timeval to={5,0};setsockopt(cf,SOL_SOCKET,SO_RCVTIMEO,&to,sizeof(to));whandle(w,cf);close(cf);}
        }
    }
    close(w->fd);return NULL;
}

// ============================================
// MAIN
// ============================================

int main(int argc, char** argv) {
    GameManager gm; memset(&gm,0,sizeof(gm));
    pthread_mutex_init(&gm.fb_mutex,NULL);
    
    int web=0;
    for(int i=1;i<argc;i++) if(strcmp(argv[i],"--web")==0) web=1;
    
    // Try X11
    int has_x11 = x11_init(&gm);
    if(!has_x11) { printf("No display - web mode\n"); web=1; }
    if(web && has_x11) { x11_cleanup(&gm); memset(&gm.display,0,sizeof(Display*)+sizeof(Window)+sizeof(GC)+sizeof(XImage*)); has_x11=0; }
    
    gm.web_mode = web;
    
    // Create games
    Game r={0},s={0};
    strcpy(r.name,"RPG Adventure"); r.init=rpg_init;r.handle_key=rpg_key;r.handle_click=rpg_click;r.update=rpg_update;r.render=rpg_render;r.cleanup=rpg_cleanup;r.active=1;
    strcpy(s.name,"Snake"); s.init=sn_init;s.handle_key=sn_key;s.handle_click=sn_click;s.update=sn_update;s.render=sn_render;s.cleanup=sn_cleanup;s.active=1;
    gm_add(&gm,&r); gm_add(&gm,&s);
    
    for(int i=0;i<gm.game_count;i++) gm.games[i]->init(gm.games[i]);
    srand(time(NULL)); gm.last_fps_update=time(NULL);
    
    // Start web server if needed
    if(web) { ws.gm=&gm; pthread_create(&ws.th,NULL,wthread,&ws); usleep(500000); }
    
    printf("Engine running. %s mode.\n", web?"Web":"X11");
    
    int running=1;
    while(running) {
        // Process X11 input (if in X11 mode)
        if(has_x11) x11_process(&gm, &running);
        
        // Check web server
        if(web && !ws.run) running=0;
        
        // Update game
        if(gm.games[gm.current_game]->active) gm.games[gm.current_game]->update(gm.games[gm.current_game]);
        
        // Render to framebuffer (headless)
        gm_render(&gm);
        
        // Mirror to X11 (just converts and displays pixels)
        if(has_x11) x11_mirror_frame(&gm);
        
        gm_fps(&gm);
        usleep(16667);
    }
    
    printf("Shutting down...\n");
    for(int i=0;i<gm.game_count;i++) if(gm.games[i]->active) gm.games[i]->cleanup(gm.games[i]);
    if(has_x11) x11_cleanup(&gm);
    if(web) pthread_join(ws.th,NULL);
    pthread_mutex_destroy(&gm.fb_mutex);
    return 0;
}