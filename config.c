#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"

int modo;
int longitudpuente;

int max_autos_este;
int max_autos_oeste;

int porcentaje_ambulancias_este;
int porcentaje_ambulancias_oeste;

double media_llegadas_este;
double media_llegadas_oeste;

int velocidad_promedio_este;
int velocidad_promedio_oeste;

int vel_min_este;
int vel_max_este;
int vel_min_oeste;
int vel_max_oeste;

int k_este;
int k_oeste;
int secs_espera;

int tiempo_semaforo_este;
int tiempo_semaforo_oeste;

static void trim(char *str)
{
    char *start = str;

    while(*start == ' ' || *start == '\t')
        start++;

    if(start != str)
        memmove(str, start, strlen(start) + 1);

    char *end = str + strlen(str) - 1;

    while(end >= str && (*end == '\n' || *end == ' ' || *end == '\t'))
        *end-- = '\0';
}

static int parse_modo(char* valor)
{
    if(strcmp(valor,"carnage")==0)  return MODO_CARNAGE;
    if(strcmp(valor,"semaforo")==0) return MODO_SEMAFORO;
    if(strcmp(valor,"oficial")==0)  return MODO_POLICIA;

    printf("Modo desconocido: %s\n", valor);
    exit(1);
}

void cargar_config(const char* archivo)
{
    FILE* f = fopen(archivo,"r");

    if(!f)
    {
        printf("No se pudo abrir %s\n", archivo);
        exit(1);
    }

    char linea[256];

    while(fgets(linea, sizeof(linea), f))
    {
        if(linea[0] == '#' || strlen(linea) < 3)
            continue;

        char* igual = strchr(linea, '=');

        if(!igual)
            continue;

        *igual = '\0';

        char* clave = linea;
        char* valor = igual + 1;

        trim(clave);
        trim(valor);

        if     (strcmp(clave,"modo")==0)                         modo                        = parse_modo(valor);
        else if(strcmp(clave,"longitudpuente")==0)               longitudpuente              = atoi(valor);
        else if(strcmp(clave,"max_autos_este")==0)               max_autos_este              = atoi(valor);
        else if(strcmp(clave,"max_autos_oeste")==0)              max_autos_oeste             = atoi(valor);
        else if(strcmp(clave,"porcentaje_ambulancias_este")==0)  porcentaje_ambulancias_este = atoi(valor);
        else if(strcmp(clave,"porcentaje_ambulancias_oeste")==0) porcentaje_ambulancias_oeste= atoi(valor);
        else if(strcmp(clave,"media_llegadas_este")==0)          media_llegadas_este         = atof(valor);
        else if(strcmp(clave,"media_llegadas_oeste")==0)         media_llegadas_oeste        = atof(valor);
        else if(strcmp(clave,"velocidad_promedio_este")==0)      velocidad_promedio_este     = atoi(valor);
        else if(strcmp(clave,"velocidad_promedio_oeste")==0)     velocidad_promedio_oeste    = atoi(valor);
        else if(strcmp(clave,"vel_min_este")==0)                 vel_min_este                = atoi(valor);
        else if(strcmp(clave,"vel_max_este")==0)                 vel_max_este                = atoi(valor);
        else if(strcmp(clave,"vel_min_oeste")==0)                vel_min_oeste               = atoi(valor);
        else if(strcmp(clave,"vel_max_oeste")==0)                vel_max_oeste               = atoi(valor);
        else if(strcmp(clave,"k_este")==0)                       k_este                      = atoi(valor);
        else if(strcmp(clave,"k_oeste")==0)                      k_oeste                     = atoi(valor);
        else if(strcmp(clave,"segundos_espera")==0)              secs_espera                 = atoi(valor);
        else if(strcmp(clave,"tiempo_semaforo_este")==0)         tiempo_semaforo_este        = atoi(valor);
        else if(strcmp(clave,"tiempo_semaforo_oeste")==0)        tiempo_semaforo_oeste       = atoi(valor);
        else printf("Clave desconocida: %s\n", clave);
    }

    fclose(f);
}