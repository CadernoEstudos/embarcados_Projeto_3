# Sistema de Controle de Casa Inteligente com ESP-RainMaker 🏠

Projeto final focado em Internet das Coisas (IoT). Este sistema transforma uma placa Franzininho WiFi (com o chip ESP32-S2) em um pequeno "cérebro" de automação residencial, conectado diretamente à nuvem oficial da Espressif, o RainMaker.

## 📋 O que o projeto faz?
O sistema não depende apenas do celular, ele tem inteligência própria. Ele junta o controle pelo aplicativo com a leitura física de sensores:
- **Luz Inteligente:** Um LED simulando a luz da sala, que pode ser ligado/desligado pelo celular, com suporte a horários programados (Timers).
- **Sensor de Presença (PIR):** Se alguém passar pelo sensor, a luz acende na hora (controle local) e o aplicativo no celular atualiza o status em tempo real.
- **Climatização Automática:** O sensor DHT11 monitora a temperatura e mostra no aplicativo. Se passar de 26ºC, o sistema liga automaticamente um segundo atuador (simulando o Ar Condicionado).
- **Integração com Assistentes:** Como usamos os "Tipos Padrão" do RainMaker, o projeto já nasce compatível para ser vinculado à Alexa ou ao Google Assistant.

## ⚙️ Como simular e usar
Para rodar este projeto, utilizamos a ferramenta oficial ESP-IDF.

**Pinos utilizados:**
- Botão Reset Wi-Fi: GPIO 0
- Luz da Sala (LED): GPIO 10
- Ar Condicionado (LED): GPIO 12
- Sensor de Movimento (PIR): GPIO 14
- Sensor DHT11: GPIO 15

**Passo a passo para compilar:**
1. No terminal do ESP-IDF, faça uma limpeza com `idf.py fullclean`.
2. Configure a placa com `idf.py set-target esp32s2`.
3. Compile, grave e abra o monitor com `idf.py build flash monitor`.

**O Provisionamento (QR Code):**
Quando a placa ligar pela primeira vez, ela vai desenhar um QR Code no terminal do computador. Abra o aplicativo **ESP RainMaker** no seu celular, clique em "Adicionar Dispositivo" e escaneie a tela. O celular vai passar a senha do seu Wi-Fi para a placa, e os controles vão aparecer na tela do seu aplicativo!
