#include "driver/dac.h"
#include "driver/ledc.h"

#define DAC_CHANNEL DAC_CHANNEL_1  // GPIO 25
#define BASE_FREQUENCY 100000  // Base frequency in Hz
#define MODULATION_INDEX 20000  // Frequency deviation in Hz

void setup() {
  Serial.begin(115200);
  Serial.println("FM Radio Simulation");

  // Initialize DAC
  dac_output_enable(DAC_CHANNEL);

  // Initialize LEDC timer for PWM
  ledc_timer_config_t ledc_timer = {
    .duty_resolution = LEDC_TIMER_10_BIT,
    .freq_hz = BASE_FREQUENCY,
    .speed_mode = LEDC_HIGH_SPEED_MODE,
    .timer_num = LEDC_TIMER_0,
    .clk_cfg = LEDC_AUTO_CLK
  };
  ledc_timer_config(&ledc_timer);

  // Initialize LEDC channel for PWM
  ledc_channel_config_t ledc_channel = {
    .channel = LEDC_CHANNEL_0,
    .duty = 512,  // 50% duty cycle
    .gpio_num = GPIO_NUM_25,
    .speed_mode = LEDC_HIGH_SPEED_MODE,
    .hpoint = 0,
    .timer_sel = LEDC_TIMER_0
  };
  ledc_channel_config(&ledc_channel);
}

void loop() {
  // Simulate FM modulation by varying the frequency
  for (int i = 0; i < 1000; i++) {
    int freq = BASE_FREQUENCY + (sin(i * 0.01) * MODULATION_INDEX);
    ledc_set_freq(LEDC_HIGH_SPEED_MODE, LEDC_TIMER_0, freq);
    delay(10);
  }
}
