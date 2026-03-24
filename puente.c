#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "puente.h"
#include "visual.h"
#include "config.h"

#include <time.h>
#include <errno.h>

/*--------------------------------------*/
/* SINCRONIZACION */
/*--------------------------------------*/

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
static pthread_cond_t policia_cond = PTHREAD_COND_INITIALIZER;

/*--------------------------------------*/

static int longitud_puente;
static int modo_control;

/*--------------------------------------*/
/* SEMAFORO */
/*--------------------------------------*/

static int direccion_semaforo = ESTE;

/*--------------------------------------*/
/* POLICIA */
/*--------------------------------------*/

static int turno = ESTE;
static int autos_turno = 0;

/*--------------------------------------*/
/* ESTADO */
/*--------------------------------------*/

static pthread_mutex_t *casillas;

static int autos_en_puente = 0;
static int direccion_actual = -1;

/*--------------------------------------*/
/* COLAS */
/*--------------------------------------*/

static int esperando_este = 0;
static int esperando_oeste = 0;

static int ambulancias_este = 0;
static int ambulancias_oeste = 0;

/*--------------------------------------*/
/* TICKETS */
/*--------------------------------------*/

static int sig_ticket_este = 0;
static int sig_ticket_oeste = 0;

static int turno_ticket_este = 0;
static int turno_ticket_oeste = 0;

/*--------------------------------------*/
/* SEMAFORO THREAD */
/*--------------------------------------*/

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

/*--------------------------------------*/
/* POLICIA THREAD */
/*--------------------------------------*/

static void cambiar_turno()
{
    turno = (turno == ESTE) ? OESTE : ESTE;
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
        int hay_otro = (turno == ESTE) ? esperando_oeste > 0
                                      : esperando_este > 0;

        int limite = (turno == ESTE) ? k_este : k_oeste;

        /* cambio por limite */
        if(hay_otro && autos_turno >= limite)
            cambiar_turno();

        /* timeout */
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += secs_espera;

        int res = pthread_cond_timedwait(&policia_cond, &mutex, &ts);

        if(res == ETIMEDOUT && autos_en_puente == 0)
        {
            int hay_otro = (turno == ESTE) ? esperando_oeste > 0
                                          : esperando_este > 0;

            if(hay_otro)
                cambiar_turno();
        }
    }
}

/*--------------------------------------*/
/* INIT */
/*--------------------------------------*/

void puente_init(int modo, int longitud)
{
    modo_control = modo;
    longitud_puente = longitud;

    casillas = malloc(sizeof(pthread_mutex_t) * longitud_puente);

    for(int i = 0; i < longitud_puente; i++)
        pthread_mutex_init(&casillas[i], NULL);

    autos_en_puente = 0;
    direccion_actual = -1;

    esperando_este = esperando_oeste = 0;
    ambulancias_este = ambulancias_oeste = 0;

    turno = ESTE;
    autos_turno = 0;

    sig_ticket_este = sig_ticket_oeste = 0;
    turno_ticket_este = turno_ticket_oeste = 0;

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

/*--------------------------------------*/
/* ESPERAR */
/*--------------------------------------*/

void esperar_puente(int direccion, int ambulancia)
{
    pthread_mutex_lock(&mutex);

    int mi_ticket;

    if(direccion == ESTE)
    {
        mi_ticket = sig_ticket_este++;
        esperando_este++;
        if(ambulancia) ambulancias_este++;
    }
    else
    {
        mi_ticket = sig_ticket_oeste++;
        esperando_oeste++;
        if(ambulancia) ambulancias_oeste++;
    }

    pthread_cond_signal(&policia_cond);

    while(1)
    {
        int capacidad_ok = autos_en_puente < longitud_puente;
        int direccion_ok = 0;

        int fifo_ok = (direccion == ESTE)
            ? (mi_ticket == turno_ticket_este)
            : (mi_ticket == turno_ticket_oeste);

        /*----------------------------------*/
        /* PRIORIDAD AMBULANCIAS */
        /*----------------------------------*/

        int prioridad_ok = 1;

        if(!ambulancia)
        {
            int amb_primera_otro_lado = 0;

            if(direccion == ESTE)
            {
                if(ambulancias_oeste > 0 && esperando_oeste == ambulancias_oeste)
                    amb_primera_otro_lado = 1;
            }
            else
            {
                if(ambulancias_este > 0 && esperando_este == ambulancias_este)
                    amb_primera_otro_lado = 1;
            }

            if(amb_primera_otro_lado)
            {
                if(modo_control == MODO_POLICIA)
                {
                    if(direccion != turno)
                        prioridad_ok = 0;
                }
                else
                {
                    prioridad_ok = 0;
                }
            }
        }

        /*----------------------------------*/
        /* MODOS */
        /*----------------------------------*/

        if(modo_control == MODO_CARNAGE)
        {
            direccion_ok =
                (autos_en_puente == 0 || direccion_actual == direccion);
        }
        else if(modo_control == MODO_SEMAFORO)
        {
            if(ambulancia)
            {
                direccion_ok =
                    (autos_en_puente == 0 || direccion_actual == direccion);
            }
            else
            {
                direccion_ok =
                    (direccion == direccion_semaforo) &&
                    (autos_en_puente == 0 || direccion_actual == direccion);
            }
        }
        else if(modo_control == MODO_POLICIA)
        {
            if(ambulancia)
            {
                direccion_ok =
                    (autos_en_puente == 0 || direccion_actual == direccion);
            }
            else
            {
                direccion_ok =
                    (direccion == turno) &&
                    (autos_en_puente == 0 || direccion_actual == direccion);
            }

            int limite = (turno == ESTE) ? k_este : k_oeste;

            int hay_otro_lado = (turno == ESTE)
                ? esperando_oeste > 0
                : esperando_este > 0;

            if(autos_turno >= limite && hay_otro_lado)
            {
                if(!ambulancia)
                    direccion_ok = 0;
            }
        }

        if(capacidad_ok && direccion_ok && prioridad_ok && fifo_ok)
        {
            if(autos_en_puente == 0)
                direccion_actual = direccion;

            if(modo_control == MODO_POLICIA && direccion == turno)
                autos_turno++;

            if(direccion == ESTE)
                turno_ticket_este++;
            else
                turno_ticket_oeste++;

            break;
        }

        pthread_cond_wait(&cond, &mutex);
    }

    autos_en_puente++;
    pthread_cond_broadcast(&cond);

    if(direccion == ESTE)
    {
        esperando_este--;
        if(ambulancia) ambulancias_este--;
    }
    else
    {
        esperando_oeste--;
        if(ambulancia) ambulancias_oeste--;
    }

    pthread_mutex_unlock(&mutex);
}

/*--------------------------------------*/
/* ENTRAR */
/*--------------------------------------*/

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

/*--------------------------------------*/
/* AVANZAR */
/*--------------------------------------*/

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

/*--------------------------------------*/
/* SALIR */
/*--------------------------------------*/

void salir_puente(int direccion)
{
    (void)direccion;

    pthread_mutex_lock(&mutex);

    autos_en_puente--;

    if(autos_en_puente == 0)
        direccion_actual = -1;

    pthread_cond_broadcast(&cond);

    pthread_mutex_unlock(&mutex);
}