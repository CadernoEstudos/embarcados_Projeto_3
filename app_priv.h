#pragma once
#include <stdbool.h>

// Prepara os pinos
void app_driver_init(void);

// Funções para ligar as coisas
void app_driver_set_luz(bool estado);
void app_driver_set_ar_condicionado(bool estado);

// Funções para ler os sensores
bool app_driver_ler_pir(void);
float app_driver_ler_temperatura(void);