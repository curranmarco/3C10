#include "configs.hpp"
#include "hardware/gpio.h"
#include "loop.hpp"

#define LED_GREEN 21
#define LED_RED 20
#define LED_ORANGE 22

extern LoopTrack loop;
extern uint32_t sample_counter; // need access to this from audio.cpp
constexpr int GPIO_BUTTON_A = 0;
constexpr int GPIO_BUTTON_B = 1;

#define BUTTON_RECORD GPIO_BUTTON_A
#define BUTTON_CLEAR GPIO_BUTTON_B

bool last_record_button = false;
bool last_clear_button = false;

void checkLoopButtons() {
  bool record_button = !gpio_get(BUTTON_RECORD); // active low
  bool clear_button = !gpio_get(BUTTON_CLEAR);

  // Toggle recording or playback when RECORD button is pressed
  if (record_button && !last_record_button) {
    if (!loop.isRecording() && !loop.isPlaying()) {
      loop.startRecording();
      DEBUG_PRINT("Loop recording started\n");
      gpio_put(LED_GREEN, 1);  // turn on the green LED
      gpio_put(LED_RED, 0);    // turn off the red LED
      gpio_put(LED_ORANGE, 0); // turn off the orange LED
                               // sleep_ms(4000);
    } else if (loop.isRecording()) {
      uint32_t current_time = sample_counter;
      loop.stopRecording(current_time);
      DEBUG_PRINT("Loop recording stopped, playback started\n");
      gpio_put(LED_GREEN, 0);  // turn off the green LED
      gpio_put(LED_RED, 0);    // turn on the red LED
      gpio_put(LED_ORANGE, 1); // turn on the orange LED
    } else if (loop.isPlaying()) {
      loop.clear();
      DEBUG_PRINT("Loop stopped and cleared\n");
    }
  }

  // Clear the loop when CLEAR button is pressed
  if (clear_button && !last_clear_button) {
    loop.clear();
    DEBUG_PRINT("Loop manually cleared\n");
    gpio_put(LED_RED, 1);    // turn on the red LED
    gpio_put(LED_GREEN, 0);  // turn off the green LED
    gpio_put(LED_ORANGE, 0); // turn off the orange LED
                             // sleep_ms(4000);
  }

  last_record_button = record_button;
  last_clear_button = clear_button;
}
