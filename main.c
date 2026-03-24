#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <math.h>

#include "config.h"
#include "auto.h"
#include "puente.h"
#include "visual.h"



static int siguiente_id = 0;
static int autos_terminados = 0;

pthread_mutex_t id_mutex  = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t fin_mutex = PTHREAD_MUTEX_INITIALIZER;

/*--------------------------------------*/

int nuevo_id()
{
    pthread_mutex_lock(&id_mutex);
    int id = siguiente_id++;
    pthread_mutex_unlock(&id_mutex);
    return id;
}

void auto_finalizado()
{
    pthread_mutex_lock(&fin_mutex);
    autos_terminados++;
    pthread_mutex_unlock(&fin_mutex);
}

/*--------------------------------------*/

int es_ambulancia(int direccion)
{
    if(direccion == ESTE)
        return (rand()%100 < porcentaje_ambulancias_este);
    else
        return (rand()%100 < porcentaje_ambulancias_oeste);
}

int velocidad_auto(int direccion)
{
    int min, max, promedio;

    if(direccion == ESTE)
    {
        min = vel_min_este;
        max = vel_max_este;
        promedio = velocidad_promedio_este;
    }
    else
    {
        min = vel_min_oeste;
        max = vel_max_oeste;
        promedio = velocidad_promedio_oeste;
    }

    int v = promedio + (rand()%21 - 10);

    if(v < min) v = min;
    if(v > max) v = max;

    return v;
}

double tiempo_exponencial(double media)
{
    double u = ((double)rand()+1) / (RAND_MAX+1.0);
    return -media * log(u);
}



void* generador_este(void* arg)
{
    (void)arg;

    for(int i = 0; i < max_autos_este; i++)
    {
        usleep((int)(tiempo_exponencial(media_llegadas_este) * 1000000));

        Auto* a = malloc(sizeof(Auto));

        a->id = nuevo_id();
        a->direccion = ESTE;
        a->ambulancia = es_ambulancia(ESTE);

        int v = velocidad_auto(ESTE);
        a->velocidad = (1500000 + rand()%1000000) / v;

        pthread_t t;
        pthread_create(&t, NULL, auto_thread, a);
        pthread_detach(t);
    }

    return NULL;
}

void* generador_oeste(void* arg)
{
    (void)arg;

    for(int i = 0; i < max_autos_oeste; i++)
    {
        usleep((int)(tiempo_exponencial(media_llegadas_oeste) * 1000000));

        Auto* a = malloc(sizeof(Auto));

        a->id = nuevo_id();
        a->direccion = OESTE;
        a->ambulancia = es_ambulancia(OESTE);

        int v = velocidad_auto(OESTE);
        a->velocidad = (1500000 + rand()%1000000) / v;

        pthread_t t;
        pthread_create(&t, NULL, auto_thread, a);
        pthread_detach(t);
    }

    return NULL;
}



int main()
{
    srand(time(NULL));

    cargar_config("puente.config");

    puente_init(modo, longitudpuente);
    visual_init(longitudpuente);

    pthread_t gui;
    pthread_create(&gui, NULL, visual_loop, NULL);

    pthread_t gen_este, gen_oeste;

    pthread_create(&gen_este, NULL, generador_este, NULL);
    pthread_create(&gen_oeste, NULL, generador_oeste, NULL);

    pthread_join(gen_este, NULL);
    pthread_join(gen_oeste, NULL);

    int total_autos = max_autos_este + max_autos_oeste;

    while(1)
    {
        pthread_mutex_lock(&fin_mutex);

        if(autos_terminados >= total_autos)
        {
            pthread_mutex_unlock(&fin_mutex);
            break;
        }

        pthread_mutex_unlock(&fin_mutex);
        usleep(100000);
    }

    printf("\nTodos los autos salieron del puente\n");

    visual_stop();
    pthread_join(gui, NULL);

    return 0;
}