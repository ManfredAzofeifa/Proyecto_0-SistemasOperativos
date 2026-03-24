#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "puente.h"
#include "config.h"

#define VISUAL_CELLS 20
#define CELL_W       52
#define CELL_H       36
#define CAR_W        44
#define CAR_H        28
#define BRIDGE_Y     130
#define QUEUE_SLOTS   8
#define LEFT_MARGIN  220
#define WIN_HEIGHT   320
#define SEM_R         18
#define NUM_MARKS     5
#define MAX_QUEUE 1000

static int queue_este[MAX_QUEUE];
static int queue_oeste[MAX_QUEUE];

static int head_este = 0, tail_este = 0;
static int head_oeste = 0, tail_oeste = 0;

static int          *bridge_state;
static float        *smooth_pos;
static int           bridge_len;

static int  cola_este  = 0;
static int  cola_oeste = 0;
static int  ambul_este = 0;
static int  ambul_oeste= 0;

static int  dir_semaforo    = ESTE;
static int  priority_dir    = ESTE;
static int  modo_visual     = 0;

static int  corriendo = 1;

static pthread_mutex_t vmutex = PTHREAD_MUTEX_INITIALIZER;

typedef struct { Uint8 r,g,b,a; } Color;

static const Color C_BG       = {15, 15, 20, 255};
static const Color C_ROAD     = {45, 45, 55, 255};
static const Color C_BRIDGE   = {60, 60, 75, 255};
static const Color C_LINE     = {200,200,100,255};
static const Color C_ESTE     = {60, 140, 255, 255};
static const Color C_OESTE    = {255, 80,  80, 255};
static const Color C_AMBUL    = {255, 220,  40, 255};
static const Color C_GREEN    = {50,  220,  80, 255};
static const Color C_RED      = {220,  50,  50, 255};
static const Color C_PRIO_BOX = {40,  100, 220, 255};
static const Color C_TEXT     = {220, 220, 220, 255};
static const Color C_SHADOW   = {0,     0,   0, 120};

static void set_color(SDL_Renderer *r, Color c)
{
    SDL_SetRenderDrawColor(r, c.r, c.g, c.b, c.a);
}

static void fill_rect(SDL_Renderer *r, int x, int y, int w, int h, Color c)
{
    set_color(r, c);
    SDL_Rect rect = {x, y, w, h};
    SDL_RenderFillRect(r, &rect);
}

static void draw_rect(SDL_Renderer *r, int x, int y, int w, int h, Color c)
{
    set_color(r, c);
    SDL_Rect rect = {x, y, w, h};
    SDL_RenderDrawRect(r, &rect);
}

static TTF_Font *font_sm  = NULL;
static TTF_Font *font_med = NULL;
static TTF_Font *font_lg  = NULL;

static void render_text(SDL_Renderer *ren, TTF_Font *f,
                        const char *txt, int x, int y, Color c, int center)
{
    if(!f) return;
    SDL_Color sc = {c.r, c.g, c.b, c.a};
    SDL_Surface *surf = TTF_RenderUTF8_Blended(f, txt, sc);
    if(!surf) return;
    SDL_Texture *tex = SDL_CreateTextureFromSurface(ren, surf);
    int tw = surf->w, th = surf->h;
    SDL_FreeSurface(surf);
    if(!tex) return;
    SDL_Rect dst = {center ? x - tw/2 : x, y - th/2, tw, th};
    SDL_RenderCopy(ren, tex, NULL, &dst);
    SDL_DestroyTexture(tex);
}

static void draw_car(SDL_Renderer *ren, int x, int y, int tipo)
{

    fill_rect(ren, x+4, y+4, CAR_W, CAR_H, C_SHADOW);

    Color body = (tipo==1) ? C_ESTE : (tipo==2) ? C_OESTE : C_AMBUL;
    Color roof = {body.r/2, body.g/2, body.b/2, 255};


    fill_rect(ren, x, y, CAR_W, CAR_H, body);

    fill_rect(ren, x+8, y-10, CAR_W-16, CAR_H/2+2, roof);

    fill_rect(ren, x+4,          y+CAR_H-5, 10, 8, C_SHADOW);
    fill_rect(ren, x+CAR_W-14,   y+CAR_H-5, 10, 8, C_SHADOW);

    Color glass = {180,220,255,200};
    fill_rect(ren, x+10, y-8, CAR_W-20, CAR_H/2-1, glass);


    if(tipo == 3)
    {
        Color cross = {255, 30, 30, 255};
        fill_rect(ren, x+CAR_W/2-3, y+3,  6, CAR_H-6, cross);
        fill_rect(ren, x+CAR_W/2-9, y+9, 18,       6, cross);
    }
}

static void draw_circle(SDL_Renderer *ren, int cx, int cy, int r, Color c)
{
    set_color(ren, c);
    for(int dy = -r; dy <= r; dy++)
        for(int dx = -r; dx <= r; dx++)
            if(dx*dx+dy*dy <= r*r)
                SDL_RenderDrawPoint(ren, cx+dx, cy+dy);
}

static void draw_semaphore(SDL_Renderer *ren, int cx, int pole_y,
                           int is_green, const char *label)
{

    fill_rect(ren, cx-3, pole_y, 6, 80, (Color){80,80,80,255});


    fill_rect(ren, cx-20, pole_y-50, 40, 48, (Color){30,30,30,255});
    draw_rect(ren, cx-20, pole_y-50, 40, 48, (Color){60,60,60,255});


    draw_circle(ren, cx, pole_y-36, SEM_R-5,
                is_green ? (Color){80,20,20,255} : C_RED);

    draw_circle(ren, cx, pole_y-12, SEM_R-5,
                is_green ? C_GREEN : (Color){20,60,20,255});

    render_text(ren, font_sm, label, cx, pole_y+90, C_TEXT, 1);
}

static void draw_priority_box(SDL_Renderer *ren, int win_w)
{
    int bw = 180, bh = 80;
    int bx = win_w/2 - bw/2;
    int by = WIN_HEIGHT - 100;

    fill_rect(ren, bx+3, by+3, bw, bh, C_SHADOW);
    fill_rect(ren, bx, by, bw, bh, C_PRIO_BOX);
    draw_rect(ren, bx, by, bw, bh, (Color){120,180,255,255});

    render_text(ren, font_sm, "TURNO ACTUAL", bx+bw/2, by+14, C_TEXT, 1);

    const char *dir_str = (priority_dir == ESTE) ? "ESTE  ->" : "<-  OESTE";
    render_text(ren, font_med, dir_str, bx+bw/2, by+32, C_AMBUL, 1);

    int turno_actual, autos_turno_actual;
    puente_get_turno_info(&turno_actual, &autos_turno_actual);

    int limite = (turno_actual == ESTE) ? k_este : k_oeste;

    char buf[48];
    snprintf(buf, sizeof(buf), "Max: %d", limite);
    render_text(ren, font_sm, buf, bx+bw/2, by+52, C_TEXT, 1);

    snprintf(buf, sizeof(buf), "Permitidos: %d", autos_turno_actual);
    render_text(ren, font_sm, buf, bx+bw/2, by+68, C_AMBUL, 1);
}

static void draw_queue_este(SDL_Renderer *ren, int bridge_x0)
{
    int size = tail_este - head_este;
    if(size > QUEUE_SLOTS) size = QUEUE_SLOTS;

    for(int i = 0; i < size; i++)
    {
        int tipo = queue_este[head_este + i];
        int x = bridge_x0 - (i+1)*CELL_W - 4;
        draw_car(ren, x, BRIDGE_Y - CAR_H/2, tipo);
    }
}

static void draw_queue_oeste(SDL_Renderer *ren, int bridge_x1)
{
    int size = tail_oeste - head_oeste;
    if(size > QUEUE_SLOTS) size = QUEUE_SLOTS;

    for(int i = 0; i < size; i++)
    {
        int tipo = queue_oeste[head_oeste + i];
        int x = bridge_x1 + i*CELL_W + 4;
        draw_car(ren, x, BRIDGE_Y - CAR_H/2, tipo);
    }
}

void visual_init(int l)
{
    bridge_len    = l;
    bridge_state  = calloc(l, sizeof(int));
    smooth_pos    = malloc(sizeof(float)*l);
    for(int i=0;i<l;i++) smooth_pos[i] = (float)i;
    modo_visual   = modo;
}

void visual_set(int pos, int valor)
{
    pthread_mutex_lock(&vmutex);
    bridge_state[pos] = valor;
    smooth_pos[pos]   = (float)pos;
    pthread_mutex_unlock(&vmutex);
}

void visual_clear(int pos)
{
    pthread_mutex_lock(&vmutex);
    bridge_state[pos] = 0;
    pthread_mutex_unlock(&vmutex);
}

int visual_get(int pos)
{
    pthread_mutex_lock(&vmutex);
    int v = bridge_state[pos];
    pthread_mutex_unlock(&vmutex);
    return v;
}

void visual_esperando(int direccion, int ambulancia)
{
    pthread_mutex_lock(&vmutex);

    int tipo = ambulancia ? 3 : (direccion == ESTE ? 1 : 2);

    if(direccion == ESTE)
    {
        queue_este[tail_este++] = tipo;
        cola_este++;
        if(ambulancia) ambul_este++;
    }
    else
    {
        queue_oeste[tail_oeste++] = tipo;
        cola_oeste++;
        if(ambulancia) ambul_oeste++;
    }

    pthread_mutex_unlock(&vmutex);
}

void visual_deja_esperar(int direccion, int ambulancia)
{
    pthread_mutex_lock(&vmutex);

    if(direccion == ESTE)
    {
        if(head_este < tail_este)
            head_este++;

        cola_este--;
        if(cola_este < 0) cola_este = 0;

        if(ambulancia)
        {
            ambul_este--;
            if(ambul_este < 0) ambul_este = 0;
        }
    }
    else
    {
        if(head_oeste < tail_oeste)
            head_oeste++;

        cola_oeste--;
        if(cola_oeste < 0) cola_oeste = 0;

        if(ambulancia)
        {
            ambul_oeste--;
            if(ambul_oeste < 0) ambul_oeste = 0;
        }
    }

    pthread_mutex_unlock(&vmutex);
}

void visual_set_direccion(int dir)
{
    pthread_mutex_lock(&vmutex);
    dir_semaforo = dir;
    pthread_mutex_unlock(&vmutex);
}

void visual_set_priority(int dir)
{
    pthread_mutex_lock(&vmutex);
    priority_dir = dir;
    pthread_mutex_unlock(&vmutex);
}

void visual_stop() { corriendo = 0; }

void* visual_loop(void* arg)
{
    (void)arg;

    if(SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
        return NULL;
    }

    if(TTF_Init() != 0)
        fprintf(stderr, "TTF_Init: %s\n", TTF_GetError());


    const char *font_paths[] = {
        "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationSans-Bold.ttf",
        "/usr/share/fonts/truetype/freefont/FreeSansBold.ttf",
        "/usr/share/fonts/TTF/DejaVuSans-Bold.ttf",
        NULL
    };
    for(int i=0; font_paths[i] && !font_sm; i++)
    {
        font_sm  = TTF_OpenFont(font_paths[i], 11);
        font_med = TTF_OpenFont(font_paths[i], 15);
        font_lg  = TTF_OpenFont(font_paths[i], 20);
    }


    int queue_w   = QUEUE_SLOTS * CELL_W + 20;
    int bridge_px = VISUAL_CELLS * CELL_W;
    int win_w     = queue_w + bridge_px + queue_w;
    int bridge_x0 = queue_w;
    int bridge_x1 = bridge_x0 + bridge_px;

    SDL_Window *win = SDL_CreateWindow(
        "Simulacion Puente",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        win_w, WIN_HEIGHT, 0);

    SDL_Renderer *ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);
    SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);

    while(corriendo)
    {
        SDL_Event e;
        while(SDL_PollEvent(&e))
            if(e.type == SDL_QUIT) { corriendo=0; return NULL; }


        set_color(ren, C_BG);
        SDL_RenderClear(ren);

        pthread_mutex_lock(&vmutex);


        for(int i=0;i<bridge_len;i++)
        {
            if(bridge_state[i] == 1 || bridge_state[i] == 3)
                smooth_pos[i] += 0.08f;
            else if(bridge_state[i] == 2)
                smooth_pos[i] -= 0.08f;
        }






        fill_rect(ren, 0, BRIDGE_Y-20, bridge_x0, 40, C_ROAD);

        fill_rect(ren, bridge_x0, BRIDGE_Y-20, bridge_px, 40, C_BRIDGE);

        fill_rect(ren, bridge_x1, BRIDGE_Y-20, queue_w, 40, C_ROAD);


        fill_rect(ren, bridge_x0,     BRIDGE_Y-22, bridge_px, 4,  (Color){100,100,120,255});
        fill_rect(ren, bridge_x0,     BRIDGE_Y+18, bridge_px, 4,  (Color){100,100,120,255});


        set_color(ren, C_LINE);
        for(int x = bridge_x0+6; x < bridge_x1-6; x += 22)
        {
            SDL_Rect dash = {x, BRIDGE_Y-1, 12, 2};
            SDL_RenderFillRect(ren, &dash);
        }


        for(int i=0; i<=VISUAL_CELLS; i++)
        {
            int px = bridge_x0 + i*CELL_W;
            fill_rect(ren, px-2, BRIDGE_Y+22, 4, 30, (Color){80,80,90,255});
        }


        draw_queue_este(ren, bridge_x0);
        draw_queue_oeste(ren, bridge_x1);


        for(int i=0;i<bridge_len;i++)
        {
            if(bridge_state[i] == 0) continue;
            float escala = (float)VISUAL_CELLS / bridge_len;
            int px = bridge_x0 + (int)(smooth_pos[i] * escala * CELL_W);
            int py = BRIDGE_Y - CAR_H/2;
            draw_car(ren, px, py, bridge_state[i]);
        }





        if(modo_visual == MODO_SEMAFORO)
        {

            draw_semaphore(ren,
                           bridge_x0 - 60,
                           BRIDGE_Y - 60,
                           dir_semaforo == ESTE,
                           "ESTE");


            draw_semaphore(ren,
                           bridge_x1 + 60,
                           BRIDGE_Y - 60,
                           dir_semaforo == OESTE,
                           "OESTE");
        }
        else if(modo_visual == MODO_POLICIA)
        {
            draw_priority_box(ren, win_w);
        }





        {
            char buf[32];
            snprintf(buf, sizeof(buf), "Cola ESTE: %d", cola_este);
            render_text(ren, font_sm, buf, bridge_x0/2, WIN_HEIGHT-20, C_TEXT, 1);

            snprintf(buf, sizeof(buf), "Cola OESTE: %d", cola_oeste);
            render_text(ren, font_sm, buf, bridge_x1+queue_w/2, WIN_HEIGHT-20, C_TEXT, 1);


            const char *mode_str =
                (modo_visual==MODO_CARNAGE)  ? "MODO: CARNAGE"  :
                (modo_visual==MODO_SEMAFORO) ? "MODO: SEMAFORO" :
                                               "MODO: POLICIA";
            render_text(ren, font_lg, mode_str, win_w/2, 20, C_TEXT, 1);
        }

        pthread_mutex_unlock(&vmutex);

        SDL_RenderPresent(ren);
        SDL_Delay(16);
    }

    if(font_sm)  TTF_CloseFont(font_sm);
    if(font_med) TTF_CloseFont(font_med);
    if(font_lg)  TTF_CloseFont(font_lg);
    TTF_Quit();

    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();

    return NULL;
}