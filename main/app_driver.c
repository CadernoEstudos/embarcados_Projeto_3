#include <driver/gpio.h>
#include <iot_button.h>
#include <app_reset.h>
#include "app_priv.h"

// Biblioteca do sensor de temperatura
#include "dht.h"

// --- ONDE OS FIOS ESTÃO LIGADOS ---
#define PINO_BOTAO_RESET 0  // Botão BOOT da placa (já estava no código do curso)
#define PINO_LUZ_SALA    10 // LED que imita a lâmpada
#define PINO_AR_COND     12 // LED que imita o Ar Condicionado
#define PINO_SENSOR_PIR  14 // Sensor de Movimento
#define PINO_DHT         15 // Sensor de Temperatura

void app_driver_init(void) {
    // 1. Configura o botão que apaga o Wi-Fi salvo (Código do curso)
    button_handle_t btn_handle = iot_button_create(PINO_BOTAO_RESET, 0);
    if (btn_handle) {
        app_reset_button_register(btn_handle, 3, 10);
    }

    // 2. Avisa a placa que esses pinos vão "jogar energia para fora" (OUTPUT)
    gpio_reset_pin(PINO_LUZ_SALA);
    gpio_set_direction(PINO_LUZ_SALA, GPIO_MODE_OUTPUT);
    gpio_set_level(PINO_LUZ_SALA, 0); // Começa apagado

    gpio_reset_pin(PINO_AR_COND);
    gpio_set_direction(PINO_AR_COND, GPIO_MODE_OUTPUT);
    gpio_set_level(PINO_AR_COND, 0); // Começa apagado

    // 3. Avisa a placa que o sensor de movimento vai "mandar energia pra dentro" (INPUT)
    gpio_reset_pin(PINO_SENSOR_PIR);
    gpio_set_direction(PINO_SENSOR_PIR, GPIO_MODE_INPUT);
}

// Quando o celular mandar ligar a luz, essa função acende o pino 10
void app_driver_set_luz(bool estado) {
    gpio_set_level(PINO_LUZ_SALA, estado);
}

// Quando o celular mandar ligar o ar, essa função acende o pino 12
void app_driver_set_ar_condicionado(bool estado) {
    gpio_set_level(PINO_AR_COND, estado);
}

// Checa se tem alguém passando na frente do sensor
bool app_driver_ler_pir(void) {
    return gpio_get_level(PINO_SENSOR_PIR) == 1;
}

// Lê o clima do DHT11
float app_driver_ler_temperatura(void) {
    int16_t temp = 0, hum = 0;
    // Se a leitura der certo, devolve a temperatura dividida por 10 (exigência do DHT11)
    if (dht_read_data(DHT_TYPE_DHT11, PINO_DHT, &hum, &temp) == ESP_OK) {
        return temp / 10.0;
    }
    return -999.0; // Se o sensor der erro, devolve esse número doido
}
