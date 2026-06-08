// =============================================================================
// GUESSING GAME - SPANE Engine SDK (VERSÃO FINAL CORRIGIDA - 10000 SESSÕES)
// =============================================================================
// Compile: gcc -shared -fPIC -O3 -o jogodeadivinhacao.so jogodeadivinhacao.c
// =============================================================================

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// =============================================================================
// SDK Constants
// =============================================================================
#define MAIN_WINDOW_WIDTH  1000
#define MAIN_WINDOW_HEIGHT 700
#define GAME_AREA_WIDTH    800
#define GAME_AREA_HEIGHT   600
#define GAME_AREA_X        180
#define GAME_AREA_Y        50

// =============================================================================
// Game Constants
// =============================================================================
#define MIN_GUESS 1
#define MAX_GUESS 100
#define MAX_GUESSES 20
#define MAX_SCORES 10000  // ALTERAÇÃO 1: era 100, agora 10000
#define HISTORY_FILE "gamedata/guessing_history.txt"

// Keycodes do SPANE
#define KEY_UP      38
#define KEY_DOWN    40
#define KEY_LEFT    37
#define KEY_RIGHT   39
#define KEY_ENTER   13
#define KEY_SPACE   32
#define KEY_N       78
#define KEY_S       83
#define KEY_R       82
#define KEY_ESC     27
#define KEY_BACK    8
#define KEY_DEL     127

// Telas
#define SCREEN_MAIN_MENU    0
#define SCREEN_GAME         1
#define SCREEN_STATS        2
#define SCREEN_HISTORY      3
#define SCREEN_REPORT       4

// Menu
#define MENU_PLAY      0
#define MENU_ANALYZE   1
#define MENU_HISTORY   2
#define MENU_REPORT    3
#define MENU_COUNT     4

// =============================================================================
// Tipos
// =============================================================================
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

typedef struct {
    time_t timestamp;
    int target;
    int num_guesses;
    int guesses[MAX_GUESSES];
} Session;

typedef struct {
    int total_sessions;
    double avg_guesses;
    int best_session;
    int worst_session;
    double std_dev;
    char strategy_rating[128];
    char improvement_tip[256];
    char strength_area[128];
    char weakness_area[128];
    int easy_wins;
    int medium_wins;
    int hard_wins;
    int total_wins;
} Report;

typedef struct {
    int screen;
    int menu_selection;
    
    int target;
    int guesses[MAX_GUESSES];
    int num_guesses;
    int game_over;
    int game_active;
    
    char guess_str[8];
    int guess_len;
    
    int cursor_visible;
    int frame_count;
    
    char message[256];
    char hint_message[128];
    char status_message[128];
    
    // Scroll e relatório
    int history_scroll;
    Report report;
    int report_generated;
    int report_scroll;
} GuessingData;

static Session sessions[10000];  // ALTERAÇÃO 2: era [MAX_SCORES], agora [10000] diretamente
static int num_sessions = 0;

// =============================================================================
// Fonte 5x7
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
// Desenho
// =============================================================================
static inline int is_valid_game_coord(int x, int y) {
    return (x >= 0 && x < GAME_AREA_WIDTH && y >= 0 && y < GAME_AREA_HEIGHT);
}

static inline void fb_pixel_rel(Framebuffer* fb, int x, int y, unsigned char r, unsigned char g, unsigned char b) {
    if (!fb || !is_valid_game_coord(x, y)) return;
    int ax = GAME_AREA_X + x;
    int ay = GAME_AREA_Y + y;
    if (ax < 0 || ax >= MAIN_WINDOW_WIDTH || ay < 0 || ay >= MAIN_WINDOW_HEIGHT) return;
    int i = (ay * MAIN_WINDOW_WIDTH + ax) * 4;
    fb->pixels[i] = r; fb->pixels[i+1] = g; fb->pixels[i+2] = b; fb->pixels[i+3] = 255;
}

static void fb_fill_rel(Framebuffer* fb, int x, int y, int w, int h, unsigned char r, unsigned char g, unsigned char b) {
    if (!fb) return;
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > GAME_AREA_WIDTH) w = GAME_AREA_WIDTH - x;
    if (y + h > GAME_AREA_HEIGHT) h = GAME_AREA_HEIGHT - y;
    if (w <= 0 || h <= 0) return;
    for (int dy = 0; dy < h; dy++)
        for (int dx = 0; dx < w; dx++)
            fb_pixel_rel(fb, x + dx, y + dy, r, g, b);
}

static void fb_rect_rel(Framebuffer* fb, int x, int y, int w, int h, unsigned char r, unsigned char g, unsigned char b) {
    if (!fb || w <= 0 || h <= 0) return;
    for (int dx = 0; dx < w; dx++) { fb_pixel_rel(fb, x+dx, y, r, g, b); if (h>1) fb_pixel_rel(fb, x+dx, y+h-1, r, g, b); }
    for (int dy = 1; dy < h-1; dy++) { fb_pixel_rel(fb, x, y+dy, r, g, b); fb_pixel_rel(fb, x+w-1, y+dy, r, g, b); }
}

static void fb_char_rel(Framebuffer* fb, int x, int y, char c, unsigned char r, unsigned char g, unsigned char b) {
    if (!fb || c < 32 || c > 126) return;
    const unsigned char* glyph = font_5x7[c - 32];
    for (int row = 0; row < 7; row++)
        for (int col = 0; col < 5; col++)
            if (glyph[col] & (1 << row)) fb_pixel_rel(fb, x+col, y+row, r, g, b);
}

static void fb_text_rel(Framebuffer* fb, int x, int y, const char* s, unsigned char r, unsigned char g, unsigned char b) {
    if (!fb || !s) return;
    for (int i = 0; s[i] && i < 200; i++) fb_char_rel(fb, x+i*6, y, s[i], r, g, b);
}

static void fb_text_center_at(Framebuffer* fb, int cx, int y, const char* s, unsigned char r, unsigned char g, unsigned char b) {
    if (!fb || !s) return;
    int len = (int)strlen(s);
    if (len > 200) len = 200;
    fb_text_rel(fb, cx - (len*6)/2, y, s, r, g, b);
}

// =============================================================================
// sqrt customizada
// =============================================================================
static double my_sqrt(double x) {
    if (x <= 0.0) return 0.0;
    double guess = x / 2.0;
    for (int i = 0; i < 20; i++) {
        double ng = (guess + x / guess) / 2.0;
        if (ng == guess) break;
        guess = ng;
    }
    return guess;
}

// =============================================================================
// FUNÇÕES RECURSIVAS
// =============================================================================
static int rec_sum(int* arr, int n) { 
    if (!arr || n <= 0) return 0;
    return arr[0] + rec_sum(arr + 1, n - 1); 
}

static int rec_min(int* arr, int n) { 
    if (!arr || n <= 0) return 999999;
    if (n == 1) return arr[0];
    int m = rec_min(arr + 1, n - 1);
    return arr[0] < m ? arr[0] : m;
}

static int rec_max(int* arr, int n) {
    if (!arr || n <= 0) return -1;
    if (n == 1) return arr[0];
    int m = rec_max(arr + 1, n - 1);
    return arr[0] > m ? arr[0] : m;
}

static double rec_sum_sq(int* arr, int n, double mean) {
    if (!arr || n <= 0) return 0.0;
    double d = arr[0] - mean;
    return d * d + rec_sum_sq(arr + 1, n - 1, mean);
}

// =============================================================================
// Histórico e Arquivo
// =============================================================================
static void save_session(GuessingData* d) {
    if (!d) return;
    
    #ifdef _WIN32
        system("if not exist gamedata mkdir gamedata");
    #else
        system("mkdir -p gamedata");
    #endif
    
    FILE* f = fopen("gamedata/guessing_history.txt", "a");
    if (!f) return;
    time_t now = time(NULL);
    fprintf(f, "%ld %d %d", (long)now, d->target, d->num_guesses);
    for (int i = 0; i < d->num_guesses && i < MAX_GUESSES; i++)
        fprintf(f, " %d", d->guesses[i]);
    fprintf(f, "\n");
    fclose(f);
}

static void load_history(GuessingData* d) {
    num_sessions = 0;
    if (d) d->history_scroll = 0;
    
    FILE* f = fopen("gamedata/guessing_history.txt", "r");
    if (!f) return;
    
    char line[2048];
    while (fgets(line, sizeof(line), f) && num_sessions < 10000) {  // ALTERAÇÃO 3: era MAX_SCORES, agora 10000
        Session* s = &sessions[num_sessions];
        memset(s, 0, sizeof(Session));
        
        char* token = strtok(line, " \n");
        if (!token) continue;
        s->timestamp = (time_t)atol(token);
        
        token = strtok(NULL, " \n");
        if (!token) continue;
        s->target = atoi(token);
        
        token = strtok(NULL, " \n");
        if (!token) continue;
        s->num_guesses = atoi(token);
        
        if (s->num_guesses > MAX_GUESSES) s->num_guesses = MAX_GUESSES;
        if (s->num_guesses < 0) s->num_guesses = 0;
        
        int i = 0;
        while ((token = strtok(NULL, " \n")) != NULL && i < MAX_GUESSES)
            s->guesses[i++] = atoi(token);
        
        if (s->num_guesses > 0) num_sessions++;
    }
    fclose(f);
}

// =============================================================================
// RELATÓRIO ANALÍTICO
// =============================================================================
static void generate_report(GuessingData* d) {
    Report* r = &d->report;
    memset(r, 0, sizeof(Report));
    
    r->total_sessions = num_sessions;
    
    if (num_sessions == 0) {
        strcpy(r->strategy_rating, "Sem dados - jogue algumas partidas!");
        strcpy(r->improvement_tip, "Complete pelo menos 3 partidas para receber analise.");
        strcpy(r->strength_area, "A determinar");
        strcpy(r->weakness_area, "A determinar");
        d->report_generated = 1;
        return;
    }
    
    int gc[10000];  // ALTERAÇÃO 4: era [MAX_SCORES], agora [10000]
    int count = num_sessions;
    
    for (int i = 0; i < count; i++)
        gc[i] = sessions[i].num_guesses;
    
    int sum = rec_sum(gc, count);
    r->avg_guesses = (double)sum / count;
    r->best_session = rec_min(gc, count);
    r->worst_session = rec_max(gc, count);
    double var = rec_sum_sq(gc, count, r->avg_guesses) / count;
    r->std_dev = my_sqrt(var);
    
    for (int i = 0; i < count; i++) {
        if (sessions[i].num_guesses <= 3) r->easy_wins++;
        else if (sessions[i].num_guesses <= 7) r->medium_wins++;
        else r->hard_wins++;
        r->total_wins++;
    }
    
    if (r->avg_guesses <= 5) {
        strcpy(r->strategy_rating, "EXCELENTE - Busca binaria perfeita!");
        strcpy(r->strength_area, "Estrategia de divisao de intervalos");
    } else if (r->avg_guesses <= 7) {
        strcpy(r->strategy_rating, "MUITO BOM - Proximo da estrategia otima");
        strcpy(r->strength_area, "Boa intuicao para reducao de intervalo");
    } else if (r->avg_guesses <= 10) {
        strcpy(r->strategy_rating, "BOM - Pode melhorar a estrategia");
        strcpy(r->strength_area, "Consistencia nas tentativas");
    } else if (r->avg_guesses <= 15) {
        strcpy(r->strategy_rating, "REGULAR - Reveja a busca binaria");
        strcpy(r->strength_area, "Persistencia");
    } else {
        strcpy(r->strategy_rating, "INICIANTE - Estude busca binaria");
        strcpy(r->strength_area, "Vontade de jogar");
    }
    
    if (r->worst_session > 15)
        strcpy(r->weakness_area, "Inconsistencia - oscila entre muito bom e ruim");
    else if (r->std_dev > 5)
        strcpy(r->weakness_area, "Falta de concentracao em algumas partidas");
    else if (r->avg_guesses > 10)
        strcpy(r->weakness_area, "Estrategia de busca ineficiente");
    else
        strcpy(r->weakness_area, "Nenhum ponto fraco significativo");
    
    if (r->avg_guesses > 10)
        snprintf(r->improvement_tip, sizeof(r->improvement_tip),
                 "Busca binaria: comece em 50. Se 'baixo', va para 75. Se 'alto', va para 25. "
                 "Divida o intervalo pela metade a cada tentativa.");
    else if (r->std_dev > 4)
        snprintf(r->improvement_tip, sizeof(r->improvement_tip),
                 "Seus resultados variam muito. Tente manter a calma e aplicar a mesma estrategia "
                 "em todas as partidas. Consistencia e mais importante que sorte.");
    else
        snprintf(r->improvement_tip, sizeof(r->improvement_tip),
                 "Voce esta no caminho certo! Continue praticando para reduzir ainda mais a media.");
    
    d->report_generated = 1;
}

static void calc_stats(GuessingData* d) {
    if (!d) return;
    if (num_sessions == 0) {
        snprintf(d->status_message, sizeof(d->status_message), "Nenhuma partida registrada ainda!");
        return;
    }
    generate_report(d);
    snprintf(d->status_message, sizeof(d->status_message), 
             "%d sessoes | Media: %.1f | Melhor: %d | Pior: %d", 
             d->report.total_sessions, d->report.avg_guesses, d->report.best_session, d->report.worst_session);
}

static const char* get_strategy(GuessingData* d) {
    if (!d || num_sessions == 0) return "Sem dados - use busca binaria!";
    if (d->report.avg_guesses < 6) return "Excelente! Busca binaria perfeita!";
    if (d->report.avg_guesses < 10) return "Bom! Tente melhorar a estrategia.";
    return "Use busca binaria: comece em 50";
}

// =============================================================================
// Dica progressiva
// =============================================================================
static void gen_hint(GuessingData* d) {
    if (!d) return;
    if (d->num_guesses == 0) {
        snprintf(d->hint_message, sizeof(d->hint_message), "Digite um numero entre %d e %d", MIN_GUESS, MAX_GUESS);
        return;
    }
    int lo = MIN_GUESS, hi = MAX_GUESS;
    for (int i = 0; i < d->num_guesses && i < MAX_GUESSES; i++) {
        if (d->guesses[i] < d->target && d->guesses[i] >= lo) lo = d->guesses[i] + 1;
        if (d->guesses[i] > d->target && d->guesses[i] <= hi) hi = d->guesses[i] - 1;
    }
    if (lo > hi) { lo = MIN_GUESS; hi = MAX_GUESS; }
    int mid = (lo + hi) / 2;
    int range = hi - lo + 1;
    
    if (d->num_guesses >= 8)
        snprintf(d->hint_message, sizeof(d->hint_message), "ULTIMA DICA: Esta entre %d e %d! Tente %d!", lo, hi, mid);
    else if (d->num_guesses >= 5)
        snprintf(d->hint_message, sizeof(d->hint_message), "Intervalo: [%d, %d] (%d numeros)", lo, hi, range);
    else if (d->num_guesses >= 3)
        snprintf(d->hint_message, sizeof(d->hint_message), range <= 20 ? "Esta perto! Entre %d e %d" : "Sugestao: tente %d", 
                 range <= 20 ? lo : mid, range <= 20 ? hi : 0);
    else
        snprintf(d->hint_message, sizeof(d->hint_message), "O numero esta entre %d e %d", lo, hi);
}

// =============================================================================
// Jogo
// =============================================================================
static void new_game(GuessingData* d) {
    if (!d) return;
    d->game_active = 1;
    d->target = (rand() % MAX_GUESS) + 1;
    d->num_guesses = 0;
    d->game_over = 0;
    d->guess_str[0] = 0;
    d->guess_len = 0;
    memset(d->guesses, 0, sizeof(d->guesses));
    snprintf(d->message, sizeof(d->message), "Nova partida! Adivinhe o numero entre %d e %d", MIN_GUESS, MAX_GUESS);
    snprintf(d->status_message, sizeof(d->status_message), "Digite um numero e pressione ENTER (max %d tentativas)", MAX_GUESSES);
    gen_hint(d);
}

static int get_current_guess(GuessingData* d) {
    if (!d || d->guess_len == 0) return 0;
    return atoi(d->guess_str);
}

static void submit_guess(GuessingData* d) {
    if (!d || d->game_over) return;
    
    if (d->num_guesses >= MAX_GUESSES) {
        d->game_over = 1; d->game_active = 0;
        snprintf(d->message, sizeof(d->message), "FIM DE JOGO! Limite de %d tentativas. O numero era %d.", MAX_GUESSES, d->target);
        save_session(d); load_history(d); calc_stats(d);
        snprintf(d->status_message, sizeof(d->status_message), "ENTER: Novo jogo | R: Relatorio");
        return;
    }
    
    int guess = get_current_guess(d);
    if (guess == 0 && d->guess_len == 0) {
        snprintf(d->message, sizeof(d->message), "Digite um numero primeiro!");
        return;
    }
    if (guess < MIN_GUESS || guess > MAX_GUESS) {
        snprintf(d->message, sizeof(d->message), "Numero deve estar entre %d e %d!", MIN_GUESS, MAX_GUESS);
        d->guess_str[0] = 0; d->guess_len = 0;
        return;
    }
    
    d->guesses[d->num_guesses++] = guess;
    
    if (guess < d->target) {
        int diff = d->target - guess;
        if (diff > 30) snprintf(d->message, sizeof(d->message), "MUITO BAIXO! Tente um numero bem maior.");
        else if (diff > 10) snprintf(d->message, sizeof(d->message), "BAIXO! Tente um numero maior.");
        else snprintf(d->message, sizeof(d->message), "Um pouco baixo... Esta perto!");
        snprintf(d->status_message, sizeof(d->status_message), "Tentativa #%d de %d", d->num_guesses, MAX_GUESSES);
    } else if (guess > d->target) {
        int diff = guess - d->target;
        if (diff > 30) snprintf(d->message, sizeof(d->message), "MUITO ALTO! Tente um numero bem menor.");
        else if (diff > 10) snprintf(d->message, sizeof(d->message), "ALTO! Tente um numero menor.");
        else snprintf(d->message, sizeof(d->message), "Um pouco alto... Esta perto!");
        snprintf(d->status_message, sizeof(d->status_message), "Tentativa #%d de %d", d->num_guesses, MAX_GUESSES);
    } else {
        if (d->num_guesses == 1) snprintf(d->message, sizeof(d->message), "INCRIVEL! Acertou de primeira! O numero e %d!", d->target);
        else if (d->num_guesses <= 5) snprintf(d->message, sizeof(d->message), "PARABENS! %d tentativas! O numero e %d.", d->num_guesses, d->target);
        else snprintf(d->message, sizeof(d->message), "ACERTOU! %d tentativas. O numero era %d.", d->num_guesses, d->target);
        d->game_over = 1; d->game_active = 0;
        save_session(d); load_history(d); calc_stats(d);
        snprintf(d->status_message, sizeof(d->status_message), "ENTER: Novo jogo | R: Relatorio");
        d->guess_str[0] = 0; d->guess_len = 0;
        return;
    }
    
    if (d->num_guesses >= MAX_GUESSES) {
        d->game_over = 1; d->game_active = 0;
        snprintf(d->message, sizeof(d->message), "FIM DE JOGO! %d tentativas. O numero era %d.", MAX_GUESSES, d->target);
        save_session(d); load_history(d); calc_stats(d);
        snprintf(d->status_message, sizeof(d->status_message), "ENTER: Novo jogo | R: Relatorio");
    } else {
        gen_hint(d);
    }
    d->guess_str[0] = 0; d->guess_len = 0;
}

// =============================================================================
// Input Handler
// =============================================================================
static void guessing_handle_key(Game* game, int keycode, int pressed) {
    if (!game || !game->data || !pressed) return;
    GuessingData* d = (GuessingData*)game->data;
    
    if (d->screen == SCREEN_REPORT) {
        if (keycode == KEY_ESC || keycode == KEY_ENTER || keycode == KEY_SPACE) d->screen = SCREEN_MAIN_MENU;
        if (keycode == KEY_UP && d->report_scroll > 0) d->report_scroll--;
        if (keycode == KEY_DOWN) d->report_scroll++;
        return;
    }
    
    if (keycode == KEY_S || keycode == 's' || keycode == 'S') {
        load_history(d); calc_stats(d); d->screen = SCREEN_STATS; d->history_scroll = 0; return;
    }
    if (keycode == KEY_N || keycode == 'n' || keycode == 'N') {
        new_game(d); d->screen = SCREEN_GAME; return;
    }
    if (keycode == KEY_R && d->screen == SCREEN_GAME && d->game_over) {
        generate_report(d); d->screen = SCREEN_REPORT; d->report_scroll = 0; return;
    }
    if (keycode == KEY_ESC) {
        if (d->screen == SCREEN_GAME) {
            d->screen = SCREEN_MAIN_MENU;
            if (d->game_active && !d->game_over) { d->game_active = 0; snprintf(d->status_message, sizeof(d->status_message), "Jogo pausado."); }
        } else if (d->screen == SCREEN_STATS || d->screen == SCREEN_HISTORY) {
            d->screen = SCREEN_MAIN_MENU;
        }
        return;
    }
    
    if (d->screen == SCREEN_MAIN_MENU) {
        switch (keycode) {
            case KEY_UP: d->menu_selection--; if (d->menu_selection < 0) d->menu_selection = MENU_COUNT-1; break;
            case KEY_DOWN: d->menu_selection++; if (d->menu_selection >= MENU_COUNT) d->menu_selection = 0; break;
            case KEY_ENTER: case KEY_SPACE:
                switch (d->menu_selection) {
                    case MENU_PLAY: new_game(d); d->screen = SCREEN_GAME; break;
                    case MENU_ANALYZE: load_history(d); calc_stats(d); d->screen = SCREEN_STATS; d->history_scroll = 0; break;
                    case MENU_HISTORY: load_history(d); calc_stats(d); d->screen = SCREEN_HISTORY; d->history_scroll = 0; break;
                    case MENU_REPORT: generate_report(d); d->screen = SCREEN_REPORT; d->report_scroll = 0; break;
                }
                break;
        }
        return;
    }
    
    if (d->screen == SCREEN_STATS || d->screen == SCREEN_HISTORY) {
        if (keycode == KEY_UP || keycode == KEY_LEFT) { if (d->history_scroll > 0) d->history_scroll--; }
        if (keycode == KEY_DOWN || keycode == KEY_RIGHT) { d->history_scroll++; }
        return;
    }
    
    if (d->screen == SCREEN_GAME) {
        if (d->game_over) {
            if (keycode == KEY_ENTER || keycode == KEY_SPACE || keycode == KEY_N) new_game(d);
            return;
        }
        if (!d->game_active) return;
        
        if (keycode >= '0' && keycode <= '9') {
            if (d->guess_len >= (int)(sizeof(d->guess_str) - 2)) return;
            d->guess_str[d->guess_len++] = (char)keycode;
            d->guess_str[d->guess_len] = '\0';
            int val = atoi(d->guess_str);
            if (val > MAX_GUESS) {
                d->guess_str[--d->guess_len] = '\0';
                if (d->guess_len == 0) { d->guess_str[0] = (char)keycode; d->guess_len = 1; d->guess_str[1] = '\0'; }
            }
            if (d->guess_len > 1 && d->guess_str[0] == '0') { d->guess_str[0] = d->guess_str[1]; d->guess_str[1] = '\0'; d->guess_len = 1; }
        }
        
        switch (keycode) {
            case KEY_BACK: case KEY_DEL: if (d->guess_len > 0) d->guess_str[--d->guess_len] = '\0'; break;
            case KEY_UP: { int g = get_current_guess(d); if (g==0&&d->guess_len==0) g=50; g++; if(g>MAX_GUESS)g=MAX_GUESS; snprintf(d->guess_str,sizeof(d->guess_str),"%d",g); d->guess_len=strlen(d->guess_str); } break;
            case KEY_DOWN: { int g = get_current_guess(d); if (g==0&&d->guess_len==0) g=50; g--; if(g<MIN_GUESS)g=MIN_GUESS; snprintf(d->guess_str,sizeof(d->guess_str),"%d",g); d->guess_len=strlen(d->guess_str); } break;
            case KEY_R: d->guess_str[0] = '\0'; d->guess_len = 0; break;
            case KEY_ENTER: case KEY_SPACE: submit_guess(d); break;
        }
    }
}

static void guessing_handle_click(Game* game, int x, int y) {
    if (!game || !game->data) return;
    GuessingData* d = (GuessingData*)game->data;
    int gx = x - GAME_AREA_X, gy = y - GAME_AREA_Y;
    
    if (d->screen == SCREEN_REPORT) { d->screen = SCREEN_MAIN_MENU; return; }
    
    if (d->screen == SCREEN_MAIN_MENU) {
        int my_start = 130, mw = 460, mh = 50, ms = 70, mx = (GAME_AREA_WIDTH - mw) / 2;
        for (int i = 0; i < MENU_COUNT; i++) {
            int my = my_start + i * ms;
            if (gx >= mx && gx <= mx+mw && gy >= my && gy <= my+mh) {
                d->menu_selection = i;
                if (i == MENU_PLAY) { new_game(d); d->screen = SCREEN_GAME; }
                else if (i == MENU_ANALYZE) { load_history(d); calc_stats(d); d->screen = SCREEN_STATS; }
                else if (i == MENU_HISTORY) { load_history(d); calc_stats(d); d->screen = SCREEN_HISTORY; }
                else if (i == MENU_REPORT) { generate_report(d); d->screen = SCREEN_REPORT; }
                return;
            }
        }
    } else if (d->screen == SCREEN_GAME && d->game_over) {
        new_game(d);
    } else if (d->screen == SCREEN_STATS || d->screen == SCREEN_HISTORY) {
        d->screen = SCREEN_MAIN_MENU;
    }
}

static void guessing_update(Game* game) {
    if (!game || !game->data) return;
    GuessingData* d = (GuessingData*)game->data;
    d->frame_count++;
    if (d->frame_count % 30 == 0) d->cursor_visible = !d->cursor_visible;
}

// =============================================================================
// RENDER
// =============================================================================
#define MARGIN_X 40
#define MARGIN_TOP 15

static void render_menu(Game* game, Framebuffer* fb) {
    GuessingData* d = (GuessingData*)game->data;
    int cx = GAME_AREA_WIDTH / 2;
    
    fb_fill_rel(fb, 0, 0, GAME_AREA_WIDTH, GAME_AREA_HEIGHT, 0x11, 0x11, 0x1A);
    fb_text_center_at(fb, cx, 60, "JOGO DE ADIVINHACAO", 0xFF, 0xCC, 0x00);
    fb_text_center_at(fb, cx, 95, "Use CIMA/BAIXO e ENTER para selecionar", 0x88, 0x88, 0x88);
    
    const char* items[] = { "[ JOGAR ]", "[ ANALISAR ESTATISTICAS ]", "[ VER HISTORICO ]", "[ RELATORIO ANALITICO ]" };
    const char* descs[] = { "Iniciar nova partida", "Ver estatisticas de desempenho", "Visualizar partidas anteriores", "Relatorio com heuristicas e dicas" };
    
    int mw = 460, mh = 50, ms = 70, mx = cx - mw/2;
    
    for (int i = 0; i < MENU_COUNT; i++) {
        int my = 130 + i * ms;
        int sel = (i == d->menu_selection);
        fb_fill_rel(fb, mx, my, mw, mh, sel?0x00:0x22, sel?0x66:0x22, sel?0xAA:0x33);
        if (sel) fb_rect_rel(fb, mx, my, mw, mh, 0x00, 0xAA, 0xFF);
        fb_text_center_at(fb, cx, my+10, items[i], sel?0xFF:0xAA, sel?0xFF:0xAA, sel?0x00:0xAA);
        fb_text_center_at(fb, cx, my+30, descs[i], sel?0xAA:0x66, sel?0xCC:0x66, sel?0xFF:0x66);
        if (sel && d->cursor_visible) fb_text_rel(fb, mx-30, my+18, ">>", 0xFF, 0xFF, 0x00);
    }
    
    fb_text_center_at(fb, cx, GAME_AREA_HEIGHT-40, "ESC: Sair  |  S: Estatisticas  |  N: Novo Jogo", 0x66, 0x66, 0x66);
    if (num_sessions > 0) {
        char info[128];
        snprintf(info, sizeof(info), "%d partidas | Melhor: %d tentativa(s)", num_sessions, d->report.best_session);
        fb_text_center_at(fb, cx, GAME_AREA_HEIGHT-20, info, 0x55, 0x55, 0x55);
    }
}

static void render_game(Game* game, Framebuffer* fb) {
    GuessingData* d = (GuessingData*)game->data;
    
    fb_text_rel(fb, MARGIN_X, MARGIN_TOP, "JOGO DE ADIVINHACAO", 0xFF, 0xCC, 0x00);
    char hi[64]; snprintf(hi, sizeof(hi), "Tentativa %d de %d", d->num_guesses+1, MAX_GUESSES);
    fb_text_rel(fb, GAME_AREA_WIDTH-280, MARGIN_TOP, hi, 0x88, 0x88, 0x88);
    
    int lx = MARGIN_X, ly = MARGIN_TOP+30;
    fb_text_rel(fb, lx, ly, "SEU PALPITE:", 0xCC, 0xCC, 0xCC);
    
    int bw = 240, bh = 50;
    fb_fill_rel(fb, lx, ly+22, bw, bh, 0x11, 0x11, 0x22);
    fb_rect_rel(fb, lx, ly+22, bw, bh, d->game_over?0x44:0x00, d->game_over?0x44:0x88, d->game_over?0x44:0xFF);
    
    if (d->game_active && !d->game_over) {
        char disp[32]; snprintf(disp, sizeof(disp), "%s", d->guess_len==0?"_":d->guess_str);
        fb_text_rel(fb, lx+20, ly+38, disp, 0xFF, 0xFF, 0xFF);
        if (d->cursor_visible) fb_fill_rel(fb, lx+20+d->guess_len*6, ly+45, 6, 2, 0xFF, 0xFF, 0x00);
    } else if (d->game_over) fb_text_rel(fb, lx+20, ly+38, "FIM", 0x88, 0x88, 0x88);
    
    int iy = ly+25;
    fb_text_rel(fb, lx+bw+20, iy, "Digite o numero:", 0x88, 0x88, 0x88);
    fb_text_rel(fb, lx+bw+20, iy+18, "0-9 + ENTER", 0x66, 0x88, 0x66);
    fb_text_rel(fb, lx+bw+20, iy+36, "CIMA/BAIXO: +1/-1", 0x55, 0x55, 0x66);
    fb_text_rel(fb, lx+bw+20, iy+54, "BACK: Apagar  R: Reset", 0x55, 0x55, 0x66);
    
    int my = ly+90;
    unsigned char mr=0xFF, mg=0xFF, mb=0xFF;
    if (strstr(d->message,"BAIXO")||strstr(d->message,"baixo")){mr=0xFF;mg=0x66;mb=0x66;}
    else if(strstr(d->message,"ALTO")||strstr(d->message,"alto")){mr=0xFF;mg=0x66;mb=0x66;}
    else if(strstr(d->message,"ACERTOU")||strstr(d->message,"PARABENS")||strstr(d->message,"INCRIVEL")||strstr(d->message,"FIM")){mr=0x00;mg=0xFF;mb=0x00;}
    fb_text_rel(fb, lx, my, d->message, mr, mg, mb);
    if (d->game_active && !d->game_over) fb_text_rel(fb, lx, my+25, d->hint_message, 0x00, 0xCC, 0xCC);
    fb_text_rel(fb, lx, my+50, d->status_message, 0x88, 0xAA, 0x88);
    char rm[64]; snprintf(rm, sizeof(rm), "Restam %d tentativas", MAX_GUESSES-d->num_guesses);
    fb_text_rel(fb, lx, my+72, rm, 0xAA, 0xAA, 0x66);
    
    int rx = 490, ry = MARGIN_TOP+10, rw = 270, rh = GAME_AREA_HEIGHT-MARGIN_TOP-100;
    fb_fill_rel(fb, rx, ry, rw, rh, 0x15, 0x15, 0x20);
    fb_rect_rel(fb, rx, ry, rw, rh, 0x33, 0x33, 0x44);
    fb_text_rel(fb, rx+10, ry+10, "HISTORICO:", 0xAA, 0xAA, 0xAA);
    int md = (rh-40)/22; if(md>MAX_GUESSES) md=MAX_GUESSES;
    for (int i=0; i<d->num_guesses&&i<md; i++) {
        char gs[32]; const char* ind = d->guesses[i]<d->target?"<<":d->guesses[i]>d->target?">>":"**";
        snprintf(gs,sizeof(gs),"#%d: %d %s",i+1,d->guesses[i],ind);
        fb_text_rel(fb,rx+10,ry+35+i*22,gs,d->guesses[i]==d->target?0x00:0xFF,d->guesses[i]==d->target?0xFF:0x88,0x88);
    }
    
    if (d->game_over) {
        int by = GAME_AREA_HEIGHT-70, bw2 = GAME_AREA_WIDTH-MARGIN_X*2;
        fb_fill_rel(fb, MARGIN_X, by, bw2, 50, 0x11, 0x33, 0x11);
        fb_rect_rel(fb, MARGIN_X, by, bw2, 50, 0x00, 0xFF, 0x00);
        fb_text_center_at(fb, GAME_AREA_WIDTH/2, by+8, "ENTER: Novo Jogo  |  S: Estatisticas  |  ESC: Menu", 0x00, 0xFF, 0x00);
        fb_text_center_at(fb, GAME_AREA_WIDTH/2, by+28, "NOVO! Pressione R para ver o RELATORIO ANALITICO", 0xFF, 0xCC, 0x00);
    }
}

static void render_stats(Game* game, Framebuffer* fb) {
    GuessingData* d = (GuessingData*)game->data;
    Report* rep = &d->report;
    int cx = GAME_AREA_WIDTH/2, sy = MARGIN_TOP+5;
    
    fb_text_center_at(fb, cx, sy, "ESTATISTICAS", 0xFF, 0xCC, 0x00);
    fb_text_rel(fb, GAME_AREA_WIDTH-300, sy, "ESC:Menu  N:Jogar  SETAS:Scroll", 0x66, 0x66, 0x66);
    
    if (num_sessions == 0) {
        fb_text_center_at(fb, cx, 200, "Nenhuma partida registrada!", 0xFF, 0x88, 0x88);
        return;
    }
    
    int pw = 380, ph = 190, py = sy+30, lx = cx-pw-15, rx2 = cx+15;
    fb_fill_rel(fb, lx, py, pw, ph, 0x15, 0x15, 0x25);
    fb_rect_rel(fb, lx, py, pw, ph, 0x33, 0x55, 0x77);
    
    char lines[7][128];
    snprintf(lines[0],sizeof(lines[0]),"Total de sessoes:      %d",rep->total_sessions);
    snprintf(lines[1],sizeof(lines[1]),"Media de tentativas:   %.1f",rep->avg_guesses);
    snprintf(lines[2],sizeof(lines[2]),"Melhor sessao:         %d tentativa(s)",rep->best_session);
    snprintf(lines[3],sizeof(lines[3]),"Pior sessao:           %d tentativa(s)",rep->worst_session);
    snprintf(lines[4],sizeof(lines[4]),"Desvio padrao:         %.2f",rep->std_dev);
    snprintf(lines[5],sizeof(lines[5])," ");
    snprintf(lines[6],sizeof(lines[6]),"Estrategia: %s",get_strategy(d));
    for (int i=0;i<7;i++) fb_text_rel(fb,lx+20,py+20+i*22,lines[i],i==6?0x00:0xFF,i==6?0xCC:0xFF,i==6?0xCC:0xFF);
    
    fb_fill_rel(fb, rx2, py, pw, ph, 0x15, 0x15, 0x25);
    fb_rect_rel(fb, rx2, py, pw, ph, 0x33, 0x55, 0x77);
    fb_text_center_at(fb, rx2+pw/2, py+20, "AVALIACAO", 0xFF, 0xCC, 0x00);
    const char* stars = rep->avg_guesses<6?"***** EXPERT":rep->avg_guesses<8?"**** AVANCADO":rep->avg_guesses<10?"*** MEDIO":rep->avg_guesses<15?"** BASICO":"* INICIANTE";
    fb_text_center_at(fb, rx2+pw/2, py+60, stars, 0xFF, 0xFF, 0x00);
    fb_text_center_at(fb, rx2+pw/2, py+100, get_strategy(d), 0xAA, 0xCC, 0xAA);
    
    int hy = py+ph+20;
    fb_text_rel(fb, MARGIN_X, hy, "ULTIMAS PARTIDAS:", 0xAA, 0xAA, 0xAA);
    int start = d->history_scroll, maxd = 10;
    if (start > num_sessions-maxd) start = num_sessions-maxd;
    if (start < 0) start = 0;
    for (int i=0; i<maxd&&(start+i)<num_sessions; i++) {
        Session* s = &sessions[start+i];
        char line[128], ts[20];
        strftime(ts,sizeof(ts),"%d/%m %H:%M",localtime(&s->timestamp));
        snprintf(line,sizeof(line),"%s  Alvo:%d  Tentativas:%d",ts,s->target,s->num_guesses);
        unsigned char cr=(s->num_guesses==rep->best_session)?0x00:0xAA;
        unsigned char cg=(s->num_guesses==rep->best_session)?0xFF:0xAA;
        unsigned char cb=(s->num_guesses==rep->worst_session)?0x66:0xAA;
        fb_text_rel(fb,MARGIN_X+10,hy+25+i*22,line,cr,cg,cb);
    }
}

static void render_history(Game* game, Framebuffer* fb) {
    GuessingData* d = (GuessingData*)game->data;
    Report* rep = &d->report;
    int cx = GAME_AREA_WIDTH/2, sy = MARGIN_TOP+5;
    
    fb_text_center_at(fb, cx, sy, "HISTORICO COMPLETO", 0xFF, 0xCC, 0x00);
    fb_text_rel(fb, GAME_AREA_WIDTH-250, sy, "ESC:Menu  SETAS:Scroll", 0x66, 0x66, 0x66);
    
    if (num_sessions == 0) {
        fb_text_center_at(fb, cx, 250, "Nenhum historico disponivel", 0xFF, 0x88, 0x88);
        return;
    }
    
    int tx = MARGIN_X+10, ty = sy+35, tw = GAME_AREA_WIDTH-MARGIN_X*2-20;
    fb_fill_rel(fb, tx, ty-5, tw, 25, 0x20, 0x20, 0x30);
    fb_text_rel(fb, tx+5, ty, "DATA/HORA            ALVO   TENTATIVAS   PALPITES", 0xCC, 0xCC, 0xCC);
    
    int start = d->history_scroll, maxd = 16;
    if (start > num_sessions-maxd) start = num_sessions-maxd;
    if (start < 0) start = 0;
    for (int i=0; i<maxd&&(start+i)<num_sessions; i++) {
        Session* s = &sessions[start+i];
        char line[512], ts[20], gs[128]="";
        strftime(ts,sizeof(ts),"%d/%m/%Y %H:%M",localtime(&s->timestamp));
        int ms = s->num_guesses<6?s->num_guesses:6;
        for (int j=0;j<ms;j++){char num[8];snprintf(num,sizeof(num),"%d%s",s->guesses[j],j<ms-1?",":"");strncat(gs,num,sizeof(gs)-strlen(gs)-1);}
        if(s->num_guesses>6)strncat(gs,"...",sizeof(gs)-strlen(gs)-1);
        snprintf(line,sizeof(line),"%-20s %-6d %-12d %s",ts,s->target,s->num_guesses,gs);
        unsigned char cr=0xAA,cg=0xAA,cb=0xAA;
        if(rep->total_sessions>1){if(s->num_guesses==rep->best_session){cr=0x00;cg=0xFF;cb=0x00;}else if(s->num_guesses==rep->worst_session){cr=0xFF;cg=0x66;cb=0x66;}}
        if((start+i)%2==0)fb_fill_rel(fb,tx,ty+25+i*22,tw,22,0x12,0x12,0x18);
        fb_text_rel(fb,tx+5,ty+28+i*22,line,cr,cg,cb);
    }
    if(start>0)fb_text_center_at(fb,cx,ty-15,"[ Mais acima ]",0xFF,0xCC,0x00);
    if(start+maxd<num_sessions)fb_text_center_at(fb,cx,ty+28+maxd*22,"[ Mais abaixo ]",0xFF,0xCC,0x00);
    fb_text_center_at(fb,cx,GAME_AREA_HEIGHT-30,"Verde = Melhor  |  Vermelho = Pior",0x88,0x88,0x88);
}

static void render_report(Game* game, Framebuffer* fb) {
    GuessingData* d = (GuessingData*)game->data;
    Report* r = &d->report;
    int cx = GAME_AREA_WIDTH/2, scroll = d->report_scroll*20;
    
    fb_fill_rel(fb,0,0,GAME_AREA_WIDTH,GAME_AREA_HEIGHT,0x0A,0x0A,0x18);
    fb_fill_rel(fb,0,0,GAME_AREA_WIDTH,55,0x0D,0x0D,0x1E);
    fb_text_center_at(fb,cx,10,"RELATORIO ANALITICO DE DESEMPENHO",0xFF,0xCC,0x00);
    fb_text_center_at(fb,cx,30,"Jogo de Adivinhacao - Estatisticas e Heuristicas",0x88,0x88,0x88);
    
    int y = 65 - scroll;
    char line[256];
    
    // Seção 1: Estatísticas Agregadas
    if (y < 550 && y > -200) {
        fb_fill_rel(fb,20,y,760,155,0x11,0x11,0x22);
        fb_rect_rel(fb,20,y,760,155,0x33,0x55,0x77);
        fb_text_rel(fb,35,y+8,"ESTATISTICAS AGREGADAS (calculadas com recursao)",0x00,0xCC,0xFF);
        
        snprintf(line,sizeof(line),"Total de sessoes: %d",r->total_sessions);
        fb_text_rel(fb,50,y+30,line,0xFF,0xFF,0xFF);
        
        snprintf(line,sizeof(line),"Media: %.1f  |  Melhor: %d  |  Pior: %d  |  Desvio: %.2f",
                 r->avg_guesses,r->best_session,r->worst_session,r->std_dev);
        fb_text_rel(fb,50,y+55,line,0xCC,0xCC,0xCC);
        
        snprintf(line,sizeof(line),"Vitorias faceis (1-3): %d  |  Medias (4-7): %d  |  Dificeis (8+): %d",
                 r->easy_wins,r->medium_wins,r->hard_wins);
        fb_text_rel(fb,50,y+80,line,0xAA,0xAA,0xFF);
        fb_text_rel(fb,50,y+125,"Todas as agregacoes sao calculadas sem loops (apenas recursao)",0x66,0x66,0x66);
    }
    
    y += 170;
    
    // Seção 2: Heurísticas (com dica com quebra de linha)
    if (y < 550 && y > -250) {
        // Calcula quantas linhas a dica vai ocupar
        int tip_len = (int)strlen(r->improvement_tip);
        int chars_per_line = 85;  // Caracteres por linha no painel
        int tip_lines = (tip_len + chars_per_line - 1) / chars_per_line;
        if (tip_lines < 1) tip_lines = 1;
        if (tip_lines > 5) tip_lines = 5;  // Máximo 5 linhas
        
        int panel_h = 105 + tip_lines * 18;  // Altura dinâmica baseada na dica
        
        fb_fill_rel(fb,20,y,760,panel_h,0x11,0x11,0x22);
        fb_rect_rel(fb,20,y,760,panel_h,0x33,0x55,0x77);
        fb_text_rel(fb,35,y+8,"AVALIACAO HEURISTICA",0xFF,0xCC,0x00);
        
        fb_text_rel(fb,50,y+30,"Classificacao:",0xAA,0xAA,0xAA);
        fb_text_rel(fb,180,y+30,r->strategy_rating,0xFF,0xFF,0x00);
        
        fb_text_rel(fb,50,y+52,"Ponto Forte:",0xAA,0xAA,0xAA);
        fb_text_rel(fb,180,y+52,r->strength_area,0x00,0xFF,0x00);
        
        fb_text_rel(fb,50,y+74,"Ponto Fraco:",0xAA,0xAA,0xAA);
        fb_text_rel(fb,180,y+74,r->weakness_area,0xFF,0x66,0x66);
        
        fb_text_rel(fb,50,y+98,"Dica:",0xFF,0xCC,0x00);
        
        // Quebra a dica em múltiplas linhas
        int dica_y = y + 98;
        int pos = 0;
        for (int i = 0; i < tip_lines && pos < tip_len; i++) {
            char tip_line[100];
            int remaining = tip_len - pos;
            int copy_len = remaining < chars_per_line ? remaining : chars_per_line;
            
            // Tenta quebrar em espaço
            if (copy_len == chars_per_line && pos + copy_len < tip_len) {
                int break_pos = copy_len;
                while (break_pos > chars_per_line/2 && r->improvement_tip[pos + break_pos] != ' ') {
                    break_pos--;
                }
                if (break_pos > chars_per_line/2) copy_len = break_pos + 1;
            }
            
            strncpy(tip_line, r->improvement_tip + pos, copy_len);
            tip_line[copy_len] = '\0';
            
            fb_text_rel(fb, 100, dica_y + i * 18, tip_line, 0x88, 0xCC, 0x88);
            pos += copy_len;
            while (pos < tip_len && r->improvement_tip[pos] == ' ') pos++;  // Pula espaços
        }
        
        y += panel_h + 15;
    }
    
    fb_fill_rel(fb,0,GAME_AREA_HEIGHT-40,GAME_AREA_WIDTH,40,0x0D,0x0D,0x1E);
    fb_text_center_at(fb,cx,GAME_AREA_HEIGHT-28,"SETAS: Scroll  |  ESC/ENTER: Voltar ao Menu",0x88,0x88,0x88);
    if (d->report_scroll > 0) fb_text_center_at(fb,cx,58,"[ Mais acima ]",0xFF,0xCC,0x00);
}

static void guessing_render(Game* game, Framebuffer* fb) {
    if (!game || !game->data || !fb) return;
    GuessingData* d = (GuessingData*)game->data;
    fb_fill_rel(fb,0,0,GAME_AREA_WIDTH,GAME_AREA_HEIGHT,0x11,0x11,0x1A);
    switch (d->screen) {
        case SCREEN_MAIN_MENU: render_menu(game,fb); break;
        case SCREEN_GAME:      render_game(game,fb); break;
        case SCREEN_STATS:     render_stats(game,fb); break;
        case SCREEN_HISTORY:   render_history(game,fb); break;
        case SCREEN_REPORT:    render_report(game,fb); break;
    }
}

static void guessing_init(Game* game) {
    if (!game) return;
    GuessingData* d = (GuessingData*)calloc(1, sizeof(GuessingData));
    if (!d) return;
    srand((unsigned int)time(NULL));
    d->screen = SCREEN_MAIN_MENU;
    d->menu_selection = MENU_PLAY;
    d->cursor_visible = 1;
    d->history_scroll = 0;
    d->report_scroll = 0;
    load_history(d);
    if (num_sessions > 0) calc_stats(d);
    snprintf(d->message,sizeof(d->message),"Bem-vindo!");
    snprintf(d->status_message,sizeof(d->status_message),"Selecione JOGAR para comecar");
    game->data = d;
    game->active = 1;
}

static void guessing_cleanup(Game* game) {
    if (game && game->data) { free(game->data); game->data = NULL; }
}

__attribute__((visibility("default"))) Game* create_game() {
    Game* game = (Game*)calloc(1, sizeof(Game));
    if (!game) return NULL;
    game->init = guessing_init;
    game->handle_key = guessing_handle_key;
    game->handle_click = guessing_handle_click;
    game->update = guessing_update;
    game->render = guessing_render;
    game->cleanup = guessing_cleanup;
    strncpy(game->name, "Jogo de Adivinhacao", sizeof(game->name) - 1);
    game->name[sizeof(game->name) - 1] = '\0';
    game->active = 0;
    game->data = NULL;
    return game;
}