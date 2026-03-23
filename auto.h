#ifndef AUTO_H
#define AUTO_H

typedef struct
{
    int id;
    int direccion;
    int ambulancia;
    int velocidad;

} Auto;

void* auto_thread(void* arg);

#endif