#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include "auto.h"
#include "puente.h"
#include "visual.h"

void auto_finalizado();

void* auto_thread(void* arg)
{
    Auto* a = (Auto*)arg;

    printf("Auto %d llega (%s)%s\n",
           a->id,
           a->direccion==ESTE?"ESTE":"OESTE",
           a->ambulancia?" [AMBULANCIA]":"");
    fflush(stdout);


    visual_esperando(a->direccion, a->ambulancia);

    entrar_puente(a->direccion, a->ambulancia);

    visual_deja_esperar(a->direccion, a->ambulancia);

    printf("Auto %d entra al puente\n", a->id);
    fflush(stdout);

    if(a->direccion == ESTE)
        avanzar_este(a->velocidad);
    else
        avanzar_oeste(a->velocidad);

    printf("Auto %d sale del puente\n", a->id);
    fflush(stdout);

    salir_puente(a->direccion);

    auto_finalizado();

    free(a);
    return NULL;
}