/*
 * ESP32-C3 Super Mini RF FM Radio
 * 
 * Плата: ESP32-C3 Super Mini
 * Особенности платы:
 * - 4MB Flash
 * - RGB LED (WS2812)
 * - USB-Serial
 * - GPIO: 13 доступных пинов
 * 
 * Author: GitHub Copilot
 * Date: 2025-01-02
 * License: MIT
 */

#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <BluetoothA2DPSink.h>
#include <driver/ledc.h>
#include <NeoPixelBus.h>

// Определение пинов ESP32-C3 Super Mini
#define RGB_LED_PIN     8     // WS2812 RGB LED
#define RF_TX_PIN       2     // Пин для RF передачи
#define FM_AUDIO_PIN    1     // Пин для аудио вывода (DAC)

// Настройки RGB LED
#define RGB_BRIGHTNESS  50
NeoPixelBus<NeoGrbFeature, Neo800KbpsMethod> rgb_led(1, RGB_LED_PIN);

// Веб сервер
AsyncWebServer server(80);

// Bluetooth A2DP Sink
BluetoothA2DPSink a2dp_sink;

// Глобальные переменные
uint32_t fm_frequency = 100000000; // 100 MHz
bool is_transmitting = false;
const char* ap_ssid = "FM-Radio";
const char* ap_password = "12345678";

// HTML интерфейс
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html>
<head>
    <title>ESP32-C3 FM Radio</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body { font-family: Arial; text-align: center; margin: 0px auto; padding: 20px; }
        .button { background-color: #4CAF50; color: white; padding: 10px 20px; border: none; border-radius: 4px; margin: 5px; }
        .button.red { background-color: #f44336; }
        .frequency { margin: 10px; padding: 10px; background-color: #f1f1f1; border-radius: 4px; }
    </style>
</head>
<body>
    <h1>ESP32-C3 FM Radio</h1>
    
    <div>
        <button class="button" onclick="toggleTransmission()" id="transmitButton">Начать передачу</button>
    </div>

    <div>
        <h2>Частота FM:</h2>
        <input type="number" id="frequencyInput" value="100000000" step="100000" min="87500000" max="108000000">
        <button class="button" onclick="setFrequency()">Установить частоту</button>
    </div>

    <script>
        function toggleTransmission() {
            const btn = document.getElementById('transmitButton');
            const isTransmitting = btn.textContent.includes('Начать');
            
            fetch('/transmit?state=' + (isTransmitting ? 'start' : 'stop'))
            .then(response => response.text())
            .then(data => {
                btn.textContent = isTransmitting ? 'Остановить передачу' : 'Начать передачу';
            });
        }

        function setFrequency() {
            const freq = document.getElementById('frequencyInput').value;
            fetch('/frequency?set=' + freq)
            .then(response => response.text())
            .then(data => alert('Частота установлена: ' + freq));
        }
    </script>
</body>
</html>
)rawliteral";

// Настройка RGB LED
void setLedColor(uint8_t r, uint8_t g, uint8_t b) {
    rgb_led.SetPixelColor(0, RgbColor(r, g, b));
    rgb_led.Show();
}

// Настройка FM передатчика
void setupFMTransmitter() {
    // Настройка таймера и канала LEDC для генерации FM сигнала
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_LOW_SPEED_MODE,
        .duty_resolution  = LEDC_TIMER_10_BIT,
        .timer_num        = LEDC_TIMER_0,
        .freq_hz          = fm_frequency / 1000, // Устанавливаем частоту в кГц
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ledc_timer_config(&ledc_timer);

    ledc_channel_config_t ledc_channel = {
        .gpio_num       = RF_TX_PIN,
        .speed_mode     = LEDC_LOW_SPEED_MODE,
        .channel        = LEDC_CHANNEL_0,
        .timer_sel      = LEDC_TIMER_0,
        .duty           = 512, // 50% duty cycle
        .hpoint         = 0
    };
    ledc_channel_config(&ledc_channel);
}

// Настройка веб-сервера
void setupWebServer() {
    // Главная страница
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send_P(200, "text/html", index_html);
    });
    
    // API для управления передачей
    server.on("/transmit", HTTP_GET, [](AsyncWebServerRequest *request){
        if(request->hasParam("state")) {
            String state = request->getParam("state")->value();
            is_transmitting = (state == "start");
            if(is_transmitting) {
                ledc_timer_resume(LEDC_LOW_SPEED_MODE, LEDC_TIMER_0);
                setLedColor(0, RGB_BRIGHTNESS, 0); // Зеленый - передача включена
            } else {
                ledc_timer_pause(LEDC_LOW_SPEED_MODE, LEDC_TIMER_0);
                setLedColor(RGB_BRIGHTNESS, 0, 0); // Красный - передача выключена
            }
            request->send(200, "text/plain", "OK");
        }
    });
    
    // API для установки частоты
    server.on("/frequency", HTTP_GET, [](AsyncWebServerRequest *request){
        if(request->hasParam("set")) {
            fm_frequency = request->getParam("set")->value().toInt();
            ledc_timer_set(LEDC_LOW_SPEED_MODE, LEDC_TIMER_0, fm_frequency / 1000, LEDC_AUTO_CLK);
            request->send(200, "text/plain", "OK");
        }
    });
    
    server.begin();
}

// Настройка Bluetooth A2DP Sink
void audio_data_callback(const uint8_t* data, uint32_t len) {
    // Вывод аудио данных через DAC
    for (uint32_t i = 0; i < len; i += 2) {
        int16_t sample = (data[i + 1] << 8) | data[i];
        dacWrite(FM_AUDIO_PIN, (sample >> 8) + 128);
    }
}

void setupBluetooth() {
    a2dp_sink.set_stream_reader(audio_data_callback);
    a2dp_sink.start("ESP32-C3 FM Radio");
}

void setup() {
    Serial.begin(115200);
    
    // Инициализация RGB LED
    rgb_led.Begin();
    setLedColor(0, 0, RGB_BRIGHTNESS); // Синий - готов к работе
    
    // Настройка FM передатчика
    setupFMTransmitter();
    
    // Настройка веб-сервера
    setupWebServer();
    
    // Настройка Bluetooth
    setupBluetooth();
    
    // Настройка WiFi точки доступа
    WiFi.softAP(ap_ssid, ap_password);
    
    Serial.print("Access Point Started. IP: ");
    Serial.println(WiFi.softAPIP());
}

void loop() {
    // Основной цикл пуст, так как логика управления реализована через веб-интерфейс
}
