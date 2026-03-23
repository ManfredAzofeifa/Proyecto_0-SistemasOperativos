#ifndef CONFIG_H
#define CONFIG_H

#define MODO_CARNAGE  0
#define MODO_SEMAFORO 1
#define MODO_POLICIA  2

extern int modo;
extern int longitudpuente;

extern int max_autos_este;
extern int max_autos_oeste;

extern int porcentaje_ambulancias_este;
extern int porcentaje_ambulancias_oeste;

extern double media_llegadas_este;
extern double media_llegadas_oeste;

extern int velocidad_promedio_este;
extern int velocidad_promedio_oeste;

extern int vel_min_este;
extern int vel_max_este;
extern int vel_min_oeste;
extern int vel_max_oeste;

extern int k_este;
extern int k_oeste;
extern int secs_espera;

extern int tiempo_semaforo_este;
extern int tiempo_semaforo_oeste;

void cargar_config(const char* archivo);

#endif