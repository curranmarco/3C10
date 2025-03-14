 #include <stdio.h>
 #include "pico/stdlib.h"
 #include "hardware/gpio.h"
 #include "hardware/adc.h"
 #include "hardware/dma.h"
 
 // Configuration constants
 #define ADC_PIN 26  // GPIO26 corresponds to ADC0
 #define ADC_INPUT 0  // ADC channel 0
 #define THRESHOLD 100  // Minimum ADC value to be considered a strike
 #define RECOVERY_TIME_MS 50 // Time before allowing a new strike to be detected
 
 volatile bool ready_for_strike = true;
 volatile uint16_t max_reading = 0;
 
 int main() {
     stdio_init_all();
     
     adc_init();
     adc_gpio_init(ADC_PIN);
     adc_select_input(ADC_INPUT);
     
     adc_set_round_robin(0);
     
     // Set maximum sample rate
     adc_set_clkdiv(0);
     

     while (true) {
         if (ready_for_strike) {
             uint16_t current_reading = adc_read();
             
             if (current_reading > THRESHOLD) {
                 ready_for_strike = false;
                 max_reading = current_reading;
                 
                 uint32_t start_time = time_us_32();
                 
                 while (time_us_32() - start_time < 1000) {
                     current_reading = adc_read();
                     
                     if (current_reading > max_reading) {
                         max_reading = current_reading;
                     }
                     
                     busy_wait_us(5);
                 }
                 
                 printf("Strike intensity: %u (raw ADC value)\n", max_reading);
                 
                 sleep_ms(RECOVERY_TIME_MS);
                 ready_for_strike = true;
             }
         }
         
         sleep_us(10);
     }
     
     return 0;
 }