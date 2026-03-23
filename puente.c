#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#include "puente.h"
#include "visual.h"
#include "config.h"

static pthread_mutex_t mutex        = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  cond         = PTHREAD_COND_INITIALIZER;
static pthread_cond_t  policia_cond = PTHREAD_COND_INITIALIZER;

static int longitud_puente;
static int modo_control;

static int direccion_semaforo = ESTE;

static int turno        = ESTE;
static int autos_turno  = 0;
static int turno_saved  = ESTE;  /* preserves turno across ambulance crossings */

static pthread_mutex_t *casillas;

static int autos_en_puente  = 0;
static int direccion_actual = -1;

static int esperando_este  = 0;
static int esperando_oeste = 0;
static int ambulancias_este  = 0;
static int ambulancias_oeste = 0;

void* semaforo_thread(void* arg)
{
    (void)arg;
    while(1)
    {
        int t = (direccion_semaforo == ESTE) ? tiempo_semaforo_este
                                             : tiempo_semaforo_oeste;
        sleep(t);
        pthread_mutex_lock(&mutex);
        direccion_semaforo = (direccion_semaforo == ESTE) ? OESTE : ESTE;
        visual_set_direccion(direccion_semaforo);
        pthread_cond_broadcast(&cond);
        pthread_mutex_unlock(&mutex);
    }
}

static void switch_turno()
{
    turno       = (turno == ESTE) ? OESTE : ESTE;
    turno_saved = turno;   /* keep saved value in sync so salir_puente doesn't revert it */
    autos_turno = 0;
    visual_set_direccion(turno);
    visual_set_priority(turno);
    pthread_cond_broadcast(&cond);
}

void* policia_thread(void* arg)
{
    (void)arg;
    pthread_mutex_lock(&mutex);

    while(1)
    {
        int hay_actual = (turno == ESTE) ? (esperando_este  > 0)
                                         : (esperando_oeste > 0);
        int hay_otro   = (turno == ESTE) ? (esperando_oeste > 0)
                                         : (esperando_este  > 0);

        if(hay_otro)
        {
            int limite = (turno == ESTE) ? k_este : k_oeste;
            if(autos_turno >= limite)
            {
                switch_turno();
                continue;
            }
        }

        if(autos_en_puente == 0 && !hay_actual && hay_otro)
        {
            struct timespec ts;
            clock_gettime(CLOCK_REALTIME, &ts);
            ts.tv_sec += secs_espera;

            pthread_cond_timedwait(&policia_cond, &mutex, &ts);

            hay_actual = (turno == ESTE) ? (esperando_este  > 0)
                                         : (esperando_oeste > 0);
            hay_otro   = (turno == ESTE) ? (esperando_oeste > 0)
                                         : (esperando_este  > 0);

            if(autos_en_puente == 0 && !hay_actual && hay_otro)
                switch_turno();
        }
        else
        {
            struct timespec ts;
            clock_gettime(CLOCK_REALTIME, &ts);
            ts.tv_nsec += 200000000;
            if(ts.tv_nsec >= 1000000000)
            {
                ts.tv_sec++;
                ts.tv_nsec -= 1000000000;
            }
            pthread_cond_timedwait(&policia_cond, &mutex, &ts);
        }
    }
}

void puente_init(int modo, int longitud)
{
    modo_control    = modo;
    longitud_puente = longitud;

    casillas = malloc(sizeof(pthread_mutex_t) * longitud_puente);
    for(int i = 0; i < longitud_puente; i++)
        pthread_mutex_init(&casillas[i], NULL);

    autos_en_puente  = 0;
    direccion_actual = -1;
    esperando_este = esperando_oeste = 0;
    ambulancias_este = ambulancias_oeste = 0;
    turno        = ESTE;
    turno_saved  = ESTE;
    autos_turno  = 0;

    if(modo_control == MODO_SEMAFORO)
    {
        pthread_t sem;
        pthread_create(&sem, NULL, semaforo_thread, NULL);
        pthread_detach(sem);
        visual_set_direccion(direccion_semaforo);
    }

    if(modo_control == MODO_POLICIA)
    {
        pthread_t pol;
        pthread_create(&pol, NULL, policia_thread, NULL);
        pthread_detach(pol);
        visual_set_direccion(turno);
        visual_set_priority(turno);
    }
}

void esperar_puente(int direccion, int ambulancia)
{
    pthread_mutex_lock(&mutex);

    if(direccion == ESTE) { esperando_este++;  if(ambulancia) ambulancias_este++;  }
    else                  { esperando_oeste++; if(ambulancia) ambulancias_oeste++; }

    if(modo_control == MODO_POLICIA)
        pthread_cond_signal(&policia_cond);

    while(1)
    {
        int capacidad_ok = autos_en_puente < longitud_puente;
        int direccion_ok = 0;
        int prioridad_ok = 1;

        if(!ambulancia)
        {
            if((direccion == ESTE && ambulancias_este > 0) ||
               (direccion == OESTE && ambulancias_oeste > 0))
                prioridad_ok = 0;
        }

        if(ambulancia)
        {
            if(autos_en_puente == 0 || direccion_actual == direccion)
            {
                if(autos_en_puente == 0)
                {
                    turno_saved      = turno;   /* remember whose turn it really is */
                    direccion_actual = direccion;
                }
                break;  /* skip turn/semaphore check entirely */
            }
        }

        if(modo_control == MODO_CARNAGE)
        {
            direccion_ok = (autos_en_puente == 0 || direccion_actual == direccion);
        }
        else if(modo_control == MODO_SEMAFORO)
        {
            direccion_ok =
                (direccion == direccion_semaforo) &&
                (autos_en_puente == 0 || direccion_actual == direccion);
        }
        else if(modo_control == MODO_POLICIA)
        {
            direccion_ok =
                (direccion == turno) &&
                (autos_en_puente == 0 || direccion_actual == direccion);

            int limite = (turno == ESTE) ? k_este : k_oeste;
            if(autos_turno >= limite)
            {
                if((turno == ESTE && esperando_oeste > 0) ||
                   (turno == OESTE && esperando_este > 0))
                    direccion_ok = 0;
            }
        }

        if(capacidad_ok && direccion_ok && prioridad_ok)
        {
            if(autos_en_puente == 0)
                direccion_actual = direccion;
            if(modo_control == MODO_POLICIA)
                autos_turno++;
            break;
        }

        pthread_cond_wait(&cond, &mutex);
    }

    autos_en_puente++;

    if(direccion == ESTE) { esperando_este--;  if(ambulancia) ambulancias_este--;  }
    else                  { esperando_oeste--; if(ambulancia) ambulancias_oeste--; }

    pthread_mutex_unlock(&mutex);
}

void entrar_puente(int direccion, int ambulancia)
{
    esperar_puente(direccion, ambulancia);

    if(direccion == ESTE)
    {
        pthread_mutex_lock(&casillas[0]);
        visual_set(0, ambulancia ? 3 : 1);
    }
    else
    {
        pthread_mutex_lock(&casillas[longitud_puente - 1]);
        visual_set(longitud_puente-1, ambulancia ? 3 : 2);
    }
}

void avanzar_este(int velocidad)
{
    for(int i = 0; i < longitud_puente - 1; i++)
    {
        pthread_mutex_lock(&casillas[i+1]);
        int val = visual_get(i);
        visual_set(i+1, val);
        visual_clear(i);
        pthread_mutex_unlock(&casillas[i]);
        usleep(velocidad);
    }
    visual_clear(longitud_puente-1);
    pthread_mutex_unlock(&casillas[longitud_puente-1]);
}

void avanzar_oeste(int velocidad)
{
    for(int i = longitud_puente - 1; i > 0; i--)
    {
        pthread_mutex_lock(&casillas[i-1]);
        int val = visual_get(i);
        visual_set(i-1, val);
        visual_clear(i);
        pthread_mutex_unlock(&casillas[i]);
        usleep(velocidad);
    }
    visual_clear(0);
    pthread_mutex_unlock(&casillas[0]);
}

void salir_puente(int direccion)
{
    (void)direccion;
    pthread_mutex_lock(&mutex);

    autos_en_puente--;

    if(autos_en_puente == 0)
    {
        direccion_actual = -1;
        turno            = turno_saved;  /* restore turn in case an ambulance changed it */
        if(modo_control == MODO_POLICIA)
            pthread_cond_signal(&policia_cond);
    }

    pthread_cond_broadcast(&cond);
    pthread_mutex_unlock(&mutex);
}