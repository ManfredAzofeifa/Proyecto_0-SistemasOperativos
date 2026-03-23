#ifndef VISUAL_H
#define VISUAL_H

void  visual_init(int longitud);

void  visual_set(int pos, int valor);
void  visual_clear(int pos);
int   visual_get(int pos);

void  visual_esperando(int direccion, int ambulancia);
void  visual_deja_esperar(int direccion, int ambulancia);

void  visual_set_direccion(int dir);
void  visual_set_priority(int dir);

void  visual_stop();
void* visual_loop(void* arg);

#endif