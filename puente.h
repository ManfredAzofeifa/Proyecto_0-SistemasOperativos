#ifndef PUENTE_H
#define PUENTE_H

#define ESTE 0
#define OESTE 1

#define MODO_CARNAGE 0
#define MODO_SEMAFORO 1
#define MODO_POLICIA 2

void puente_init(int modo, int longitud);

void esperar_puente(int direccion, int ambulancia);

void salir_puente(int direccion);

void entrar_puente(int direccion, int ambulancia);

void avanzar_este(int velocidad);

void avanzar_oeste(int velocidad);
#endif