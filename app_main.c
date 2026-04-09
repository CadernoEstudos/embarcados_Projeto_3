#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <esp_event.h>
#include <nvs_flash.h>

#include <esp_rmaker_core.h>
#include <esp_rmaker_standard_types.h>
#include <esp_rmaker_standard_params.h>
#include <esp_rmaker_standard_devices.h>
#include <esp_rmaker_schedule.h>
#include <esp_rmaker_scenes.h>
#include <esp_rmaker_ota.h>
#include <app_network.h>

#include "app_priv.h"

static const char *TAG = "CASA_INTELIGENTE";

// Variáveis globais para guardar nossos "aparelhos" do aplicativo
esp_rmaker_device_t *luz_device;
esp_rmaker_device_t *ar_device;
esp_rmaker_device_t *temp_device;

/* * Essa função é "acordada" toda vez que alguém aperta algo no App do celular.
 * Ela descobre quem foi apertado e manda a placa agir.
 */
static esp_err_t write_cb(const esp_rmaker_device_t *device, const esp_rmaker_param_t *param,
            const esp_rmaker_param_val_t val, void *priv_data, esp_rmaker_write_ctx_t *ctx)
{
    const char *nome_aparelho = esp_rmaker_device_get_name(device);
    const char *nome_comando = esp_rmaker_param_get_name(param);

    // Alguém mexeu na Luz pelo celular?
    if (strcmp(nome_aparelho, "Luz da Sala") == 0) {
        if (strcmp(nome_comando, ESP_RMAKER_DEF_POWER_NAME) == 0) {
            // Liga ou desliga o pino físico!
            app_driver_set_luz(val.val.b);
            esp_rmaker_param_update(param, val); // Avisa o app que deu certo
        } 
        else {
            // Se o usuário mexer na barra de brilho ou cor, só atualizamos a barrinha no app
            // (Para não complicar o código físico com PWM e matemática de cores)
            esp_rmaker_param_update(param, val);
        }
    } 
    // Alguém mexeu no Ar Condicionado pelo celular?
    else if (strcmp(nome_aparelho, "Ar Condicionado") == 0) {
        if (strcmp(nome_comando, ESP_RMAKER_DEF_POWER_NAME) == 0) {
            app_driver_set_ar_condicionado(val.val.b);
            esp_rmaker_param_update(param, val);
        }
    }
    return ESP_OK;
}

/*
 * TAREFA DE AUTOMAÇÃO (DESAFIO DO PROFESSOR)
 * Roda em paralelo sozinha, olhando os sensores.
 */
void task_automacao(void *pvParameters) {
    bool ar_ligado_automatico = false;

    while(1) {
        // --- 1. LÓGICA DO SENSOR DE PRESENÇA ---
        // Se alguém passou e a luz tá apagada, acende!
        if (app_driver_ler_pir()) {
            app_driver_set_luz(true);
            // Avisa o celular que a luz acendeu sozinha
            esp_rmaker_param_update_and_report(
                esp_rmaker_device_get_param(luz_device, ESP_RMAKER_DEF_POWER_NAME),
                esp_rmaker_bool(true)
            );
        }

        // --- 2. LÓGICA DO AR CONDICIONADO (TEMPERATURA) ---
        float temperatura = app_driver_ler_temperatura();
        
        // Se o sensor leu certinho (não deu -999)
        if (temperatura != -999.0) {
            // Atualiza o gráfico no celular
            esp_rmaker_param_update_and_report(
                esp_rmaker_device_get_param(temp_device, ESP_RMAKER_DEF_TEMPERATURE_NAME),
                esp_rmaker_float(temperatura)
            );

            // Passou de 26 graus? Liga o Ar!
            if (temperatura > 26.0 && !ar_ligado_automatico) {
                app_driver_set_ar_condicionado(true);
                esp_rmaker_param_update_and_report(
                    esp_rmaker_device_get_param(ar_device, ESP_RMAKER_DEF_POWER_NAME),
                    esp_rmaker_bool(true)
                );
                ar_ligado_automatico = true;
            } 
            // Esfriou? Desliga o ar!
            else if (temperatura <= 26.0 && ar_ligado_automatico) {
                app_driver_set_ar_condicionado(false);
                esp_rmaker_param_update_and_report(
                    esp_rmaker_device_get_param(ar_device, ESP_RMAKER_DEF_POWER_NAME),
                    esp_rmaker_bool(false)
                );
                ar_ligado_automatico = false;
            }
        }
        
        // Espera 3 segundos pra ler os sensores de novo
        vTaskDelay(pdMS_TO_TICKS(3000));
    }
}

// O programa principal!
void app_main()
{
    // Prepara as portas (LEDs e Sensores)
    app_driver_init();

    // Inicia a memória (NVS) pra guardar a senha do Wi-Fi
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK( err );

    // Prepara a rede (código do professor)
    app_network_init();

    // Configura o Nó do RainMaker
    esp_rmaker_config_t rainmaker_cfg = {
        .enable_time_sync = true, // Tem que ser true pros agendamentos/timers funcionarem!
    };
    esp_rmaker_node_t *node = esp_rmaker_node_init(&rainmaker_cfg, "Smart Home", "Casa Inteligente");

    // --- CRIANDO OS APARELHOS NO APP ---

    // 1. A Luz da Sala
    luz_device = esp_rmaker_lightbulb_device_create("Luz da Sala", NULL, false);
    esp_rmaker_device_add_cb(luz_device, write_cb, NULL);
    // Adicionamos as barras de brilho e cor no App
    esp_rmaker_device_add_param(luz_device, esp_rmaker_brightness_param_create(ESP_RMAKER_DEF_BRIGHTNESS_NAME, 100));
    esp_rmaker_device_add_param(luz_device, esp_rmaker_hue_param_create(ESP_RMAKER_DEF_HUE_NAME, 0));
    esp_rmaker_device_add_param(luz_device, esp_rmaker_saturation_param_create(ESP_RMAKER_DEF_SATURATION_NAME, 100));
    esp_rmaker_node_add_device(node, luz_device);

    // 2. O Ar Condicionado (Um botão simples de Liga/Desliga)
    ar_device = esp_rmaker_switch_device_create("Ar Condicionado", NULL, false);
    esp_rmaker_device_add_cb(ar_device, write_cb, NULL);
    esp_rmaker_node_add_device(node, ar_device);

    // 3. O Sensor de Clima (Mostra a temperatura na tela inicial do app)
    temp_device = esp_rmaker_temp_sensor_device_create("Sensor de Clima", NULL, 25.0);
    esp_rmaker_node_add_device(node, temp_device);

    // --- LIGA OS SERVIÇOS DO RAINMAKER ---
    esp_rmaker_ota_enable_default();
    esp_rmaker_timezone_service_enable(); // Pro timer funcionar no horário certo
    esp_rmaker_schedule_enable();         // Ativa as rotinas de agendamento (Timers)
    esp_rmaker_scenes_enable();           // Ativa as Cenas
    
    esp_rmaker_start();

    // Inicia o Wi-Fi. Se não tiver senha salva, vai gerar aquele QR Code no terminal!
    err = app_network_start(POP_TYPE_RANDOM);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Falha no Wi-Fi. Fim do programa.");
        abort();
    }

    // Inicia
    xTaskCreate(task_automacao, "Automacao", 4096, NULL, 5, NULL);
}