#include <driver/rmt.h>

#define RMT_TX_CHANNEL    RMT_CHANNEL_0  // Правильный тип для канала RMT
#define FM_PIN            GPIO_NUM_21    // Правильный тип для пина GPIO

// Частота FM (например, 88.5 МГц)
#define FM_FREQUENCY      88500000 // 88.5 МГц

void setup() {
  Serial.begin(115200);
  delay(1000); // Даем время на настройку последовательного порта

  // Логирование старта
  Serial.println("FM Transmitter starting...");

  // Настройка пина
  pinMode(FM_PIN, OUTPUT);
  Serial.println("FM pin configured.");

  // Используем новый драйвер для RMT
  rmt_config_t rmt_tx;
  rmt_tx.channel = RMT_CHANNEL_0;  // Использование нового драйвера
  rmt_tx.gpio_num = FM_PIN;
  rmt_tx.mem_block_num = 1;
  rmt_tx.tx_config.loop_en = false;
  rmt_tx.tx_config.carrier_en = false;
  rmt_tx.tx_config.idle_level = RMT_IDLE_LEVEL_LOW;
  rmt_tx.tx_config.carrier_level = RMT_CARRIER_LEVEL_HIGH;
  rmt_tx.tx_config.idle_output_en = false;
  rmt_config(&rmt_tx);
  rmt_driver_install(RMT_CHANNEL_0, 0, 0);  // Установка нового драйвера
  Serial.println("RMT configured and driver installed.");

  // Настройка частоты сигнала
  setFMFrequency(FM_FREQUENCY);
}

void loop() {
  // Логирование состояния работы передатчика
  Serial.println("Generating FM signal...");
  generateFMSignal();
  delay(1000); // Задержка для отслеживания выводимых логов
}

void setFMFrequency(int frequency) {
  // Эта функция будет настраивать частоту передатчика
  Serial.print("Setting FM frequency to: ");
  Serial.println(frequency);
  // Здесь можно добавить дополнительную логику для настройки частоты.
}

void generateFMSignal() {
  // Пример генерации сигнала для FM-передачи
  static int amplitude = 255;
  static int direction = -1;

  // Генерация простого сигнала для проверки
  Serial.println("Starting signal generation loop...");
  for (int i = 0; i < 100; i++) {
    analogWrite(FM_PIN, amplitude);
    amplitude += direction;
    if (amplitude <= 0 || amplitude >= 255) {
      direction = -direction;
    }
    delayMicroseconds(50); // Генерация сигнала с нужной частотой
  }
  Serial.println("Signal generation completed.");
}
