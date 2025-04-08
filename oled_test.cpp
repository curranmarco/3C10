/**
 * MDOB128064BV-WS OLED Display Test
 * 
 * This example demonstrates initialization and basic usage of the
 * SSD1309 OLED display with the Raspberry Pi Pico.
 */

 #include <stdio.h>
 #include "pico/stdlib.h"
 #include "hardware/spi.h"
 #include "hardware/gpio.h"
 #include <math.h>
 #include <stdint.h>
 
 // Pin definitions
 #define SPI_PORT spi0
 #define PIN_SCK  2
 #define PIN_MOSI 3
 #define PIN_CS   5
 #define PIN_DC   7
 #define PIN_RST  6
 
 // Display dimensions
 #define DISPLAY_WIDTH  128
 #define DISPLAY_HEIGHT 64
 
 // SSD1309 commands
 #define SSD1309_SETLOWCOLUMN         0x00
 #define SSD1309_SETHIGHCOLUMN        0x10
 #define SSD1309_MEMORYMODE           0x20
 #define SSD1309_COLUMNADDR           0x21
 #define SSD1309_PAGEADDR             0x22
 #define SSD1309_SETSTARTLINE         0x40
 #define SSD1309_SETCONTRAST          0x81
 #define SSD1309_SEGREMAP             0xA0
 #define SSD1309_DISPLAYALLON_RESUME  0xA4
 #define SSD1309_DISPLAYALLON         0xA5
 #define SSD1309_NORMALDISPLAY        0xA6
 #define SSD1309_INVERTDISPLAY        0xA7
 #define SSD1309_SETMULTIPLEX         0xA8
 #define SSD1309_DISPLAYOFF           0xAE
 #define SSD1309_DISPLAYON            0xAF
 #define SSD1309_SETPAGE              0xB0
 #define SSD1309_COMSCANINC           0xC0
 #define SSD1309_COMSCANDEC           0xC8
 #define SSD1309_SETDISPLAYOFFSET     0xD3
 #define SSD1309_SETDISPLAYCLOCKDIV   0xD5
 #define SSD1309_SETPRECHARGE         0xD9
 #define SSD1309_SETCOMPINS           0xDA
 #define SSD1309_SETVCOMDETECT        0xDB
 
 // Buffer for the entire display (128x64 pixels = 1024 bytes)
 uint8_t buffer[DISPLAY_WIDTH * DISPLAY_HEIGHT / 8];
 
 // Function prototypes
 void oled_command(uint8_t cmd);
 void oled_data(uint8_t* data, size_t len);
 void oled_init();
 void oled_display();
 void draw_pixel(int16_t x, int16_t y, bool white);
 void draw_waveform(int amplitude, int frequency);
 void draw_text(int x, int y, const char* text);
 void clear_buffer();
 
 int main() {
     stdio_init_all();
     
     // Initialize display
     oled_init();
     clear_buffer();
     
     // Draw text
     draw_text(0, 0, "DRUM KIT");
     draw_text(0, 16, "Effect: Echo");
     draw_text(0, 24, "Decay: 0.6");
     
     // Draw a sample waveform
     draw_waveform(18, 3);
     
     // Send buffer to display
     oled_display();
     
     while (1) {
         sleep_ms(100);
     }
     
     return 0;
 }
 
 void oled_command(uint8_t cmd) {
     gpio_put(PIN_DC, 0); // Command mode
     gpio_put(PIN_CS, 0); // Select OLED
     spi_write_blocking(spi0, &cmd, 1);
     gpio_put(PIN_CS, 1); // Deselect OLED
 }
 
 void oled_data(uint8_t* data, size_t len) {
     gpio_put(PIN_DC, 1); // Data mode
     gpio_put(PIN_CS, 0); // Select OLED
     spi_write_blocking(spi0, data, len);
     gpio_put(PIN_CS, 1); // Deselect OLED
 }
 
 void oled_init() {
     // Initialize SPI
     spi_init(spi0, 8000000); // 8 MHz
     gpio_set_function(PIN_SCK, GPIO_FUNC_SPI);
     gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);
     
     // Initialize control pins
     gpio_init(PIN_CS);
     gpio_init(PIN_DC);
     gpio_init(PIN_RST);
     gpio_set_dir(PIN_CS, GPIO_OUT);
     gpio_set_dir(PIN_DC, GPIO_OUT);
     gpio_set_dir(PIN_RST, GPIO_OUT);
     
     
     
     // Initialize display based on the datasheet's recommended initial code
     oled_command(0xAE); // Display Off
     oled_command(0xA8); // Select Multiplex Ratio
     oled_command(0x3F); // 1/64 Duty
     oled_command(0xD3); // Setting Display Offset
     oled_command(0x00); // 00H Reset
     oled_command(0x20); // Set Memory Addressing Mode
     oled_command(0x02); // Page Addressing Mode
     oled_command(0x00); // Set Column Address LSB
     oled_command(0x10); // Set Column Address MSB
     oled_command(0x40); // Set Display Start Line
     oled_command(0xA6); // Set Normal Display
     oled_command(0xDB); // Set Deselect Vcomh level
     oled_command(0x3C); // ~0.84xVCC
     oled_command(0xA4); // Entire Display ON
     oled_command(0x81); // Set Contrast Control
     oled_command(0x4F);
     oled_command(0xD5); // SET DISPLAY CLOCK
     oled_command(0x70); // 105HZ
     oled_command(0xA1); // Column Address 127 Mapped to SEG0
     oled_command(0xC8); // Scan from COM[N-1] to 0
     oled_command(0xDA); // Set COM Hardware Configuration
     oled_command(0x12); // Alternative COM Pin
     oled_command(0xD9); // Set Pre-Charge period
     oled_command(0x22);
     oled_command(0xAF); // Display ON
 }
 
 void oled_display() {
     // Loop through pages (8 pages for 64 pixel height, each page is 8 pixels tall)
     for (int page = 0; page < 8; page++) {
         // Set page address
         oled_command(0xB0 + page);
         // Set column address to 0
         oled_command(0x00); // Lower column start address
         oled_command(0x10); // Higher column start address
         
         // Send the data for this page
         uint8_t* page_data = &buffer[page * DISPLAY_WIDTH];
         oled_data(page_data, DISPLAY_WIDTH);
     }
 }
 
 void draw_pixel(int16_t x, int16_t y, bool white) {
     if (x < 0 || x >= DISPLAY_WIDTH || y < 0 || y >= DISPLAY_HEIGHT) {
         return;
     }
     
     // Calculate the byte position and bit position within that byte
     uint16_t byte_idx = x + (y / 8) * DISPLAY_WIDTH;
     uint8_t bit_position = y % 8;
     
     if (white) {
         buffer[byte_idx] |= (1 << bit_position);
     } else {
         buffer[byte_idx] &= ~(1 << bit_position);
     }
 }
 
 void draw_waveform(int amplitude, int frequency) {
     int centerY = 56; // Y position from top
     
     for (int x = 0; x < DISPLAY_WIDTH; x++) {
         // Calculate sine wave point
         float angle = (float)x / DISPLAY_WIDTH * 2 * (22/7) * frequency;
         int y = centerY - (int)(amplitude * sin(angle));
         
         // Draw the point
         draw_pixel(x, y, true);
     }
 }
 
 // Simple font (5x7 pixels)
 const uint8_t font5x7[][5] = {
     {0x00, 0x00, 0x00, 0x00, 0x00}, // Space
     {0x00, 0x00, 0x5F, 0x00, 0x00}, // !
     {0x00, 0x07, 0x00, 0x07, 0x00}, // "
     {0x14, 0x7F, 0x14, 0x7F, 0x14}, // #
     {0x24, 0x2A, 0x7F, 0x2A, 0x12}, // $
     {0x23, 0x13, 0x08, 0x64, 0x62}, // %
     {0x36, 0x49, 0x55, 0x22, 0x50}, // &
     {0x00, 0x05, 0x03, 0x00, 0x00}, // '
     {0x00, 0x1C, 0x22, 0x41, 0x00}, // (
     {0x00, 0x41, 0x22, 0x1C, 0x00}, // )
     {0x08, 0x2A, 0x1C, 0x2A, 0x08}, // *
     {0x08, 0x08, 0x3E, 0x08, 0x08}, // +
     {0x00, 0x50, 0x30, 0x00, 0x00}, // ,
     {0x08, 0x08, 0x08, 0x08, 0x08}, // -
     {0x00, 0x60, 0x60, 0x00, 0x00}, // .
     {0x20, 0x10, 0x08, 0x04, 0x02}, // /
     {0x3E, 0x51, 0x49, 0x45, 0x3E}, // 0
     {0x00, 0x42, 0x7F, 0x40, 0x00}, // 1
     {0x42, 0x61, 0x51, 0x49, 0x46}, // 2
     {0x21, 0x41, 0x45, 0x4B, 0x31}, // 3
     {0x18, 0x14, 0x12, 0x7F, 0x10}, // 4
     {0x27, 0x45, 0x45, 0x45, 0x39}, // 5
     {0x3C, 0x4A, 0x49, 0x49, 0x30}, // 6
     {0x01, 0x71, 0x09, 0x05, 0x03}, // 7
     {0x36, 0x49, 0x49, 0x49, 0x36}, // 8
     {0x06, 0x49, 0x49, 0x29, 0x1E}, // 9
     {0x00, 0x36, 0x36, 0x00, 0x00}, // :
     {0x00, 0x56, 0x36, 0x00, 0x00}, // ;
     {0x00, 0x08, 0x14, 0x22, 0x41}, // <
     {0x14, 0x14, 0x14, 0x14, 0x14}, // =
     {0x41, 0x22, 0x14, 0x08, 0x00}, // >
     {0x02, 0x01, 0x51, 0x09, 0x06}, // ?
     {0x32, 0x49, 0x79, 0x41, 0x3E}, // @
     {0x7E, 0x09, 0x09, 0x09, 0x7E}, // A
     {0x7F, 0x49, 0x49, 0x49, 0x36}, // B
     {0x3E, 0x41, 0x41, 0x41, 0x22}, // C
     {0x7F, 0x41, 0x41, 0x22, 0x1C}, // D
     {0x7F, 0x49, 0x49, 0x49, 0x41}, // E
     {0x7F, 0x09, 0x09, 0x01, 0x01}, // F
     {0x3E, 0x41, 0x41, 0x49, 0x7A}, // G
     {0x7F, 0x08, 0x08, 0x08, 0x7F}, // H
     {0x00, 0x41, 0x7F, 0x41, 0x00}, // I
     {0x20, 0x40, 0x41, 0x3F, 0x01}, // J
     {0x7F, 0x08, 0x14, 0x22, 0x41}, // K
     {0x7F, 0x40, 0x40, 0x40, 0x40}, // L
     {0x7F, 0x02, 0x04, 0x02, 0x7F}, // M
     {0x7F, 0x04, 0x08, 0x10, 0x7F}, // N
     {0x3E, 0x41, 0x41, 0x41, 0x3E}, // O
     {0x7F, 0x09, 0x09, 0x09, 0x06}, // P
     {0x3E, 0x41, 0x51, 0x21, 0x5E}, // Q
     {0x7F, 0x09, 0x19, 0x29, 0x46}, // R
     {0x46, 0x49, 0x49, 0x49, 0x31}, // S
     {0x01, 0x01, 0x7F, 0x01, 0x01}, // T
     {0x3F, 0x40, 0x40, 0x40, 0x3F}, // U
     {0x1F, 0x20, 0x40, 0x20, 0x1F}, // V
     {0x7F, 0x20, 0x18, 0x20, 0x7F}, // W
     {0x63, 0x14, 0x08, 0x14, 0x63}, // X
     {0x03, 0x04, 0x78, 0x04, 0x03}, // Y
     {0x61, 0x51, 0x49, 0x45, 0x43}, // Z
     {0x00, 0x00, 0x7F, 0x41, 0x41}, // [
     {0x02, 0x04, 0x08, 0x10, 0x20}, // "\"
     {0x41, 0x41, 0x7F, 0x00, 0x00}, // ]
     {0x04, 0x02, 0x01, 0x02, 0x04}, // ^
     {0x40, 0x40, 0x40, 0x40, 0x40}, // _
     {0x00, 0x01, 0x02, 0x04, 0x00}, // `
     {0x20, 0x54, 0x54, 0x54, 0x78}, // a
     {0x7F, 0x48, 0x44, 0x44, 0x38}, // b
     {0x38, 0x44, 0x44, 0x44, 0x20}, // c
     {0x38, 0x44, 0x44, 0x48, 0x7F}, // d
     {0x38, 0x54, 0x54, 0x54, 0x18}, // e
     {0x08, 0x7E, 0x09, 0x01, 0x02}, // f
     {0x08, 0x14, 0x54, 0x54, 0x3C}, // g
     {0x7F, 0x08, 0x04, 0x04, 0x78}, // h
     {0x00, 0x44, 0x7D, 0x40, 0x00}, // i
     {0x20, 0x40, 0x44, 0x3D, 0x00}, // j
     {0x00, 0x7F, 0x10, 0x28, 0x44}, // k
     {0x00, 0x41, 0x7F, 0x40, 0x00}, // l
     {0x7C, 0x04, 0x18, 0x04, 0x78}, // m
     {0x7C, 0x08, 0x04, 0x04, 0x78}, // n
     {0x38, 0x44, 0x44, 0x44, 0x38}, // o
     {0x7C, 0x14, 0x14, 0x14, 0x08}, // p
     {0x08, 0x14, 0x14, 0x18, 0x7C}, // q
     {0x7C, 0x08, 0x04, 0x04, 0x08}, // r
     {0x48, 0x54, 0x54, 0x54, 0x20}, // s
     {0x04, 0x3F, 0x44, 0x40, 0x20}, // t
     {0x3C, 0x40, 0x40, 0x20, 0x7C}, // u
     {0x1C, 0x20, 0x40, 0x20, 0x1C}, // v
     {0x3C, 0x40, 0x30, 0x40, 0x3C}, // w
     {0x44, 0x28, 0x10, 0x28, 0x44}, // x
     {0x0C, 0x50, 0x50, 0x50, 0x3C}, // y
     {0x44, 0x64, 0x54, 0x4C, 0x44}, // z
     {0x00, 0x08, 0x36, 0x41, 0x00}, // {
     {0x00, 0x00, 0x7F, 0x00, 0x00}, // |
     {0x00, 0x41, 0x36, 0x08, 0x00}, // }
     {0x08, 0x08, 0x2A, 0x1C, 0x08}, // ->
     {0x08, 0x1C, 0x2A, 0x08, 0x08}  // <-
 };
 
 void draw_char(int x, int y, char c) {
     if (c < ' ' || c > '~') {
         c = '?'; // Default to question mark for unknown characters
     }
     
     // Get the character data from the font array
     const uint8_t* char_data = font5x7[c - ' '];
     
     // Draw each column of the character
     for (int col = 0; col < 5; col++) {
         uint8_t column = char_data[col];
         
         for (int row = 0; row < 8; row++) {
             if (column & (1 << row)) {
                 draw_pixel(x + col, y + row, true);
             }
         }
     }
 }
 
 void draw_text(int x, int y, const char* text) {
     int cursor_x = x;
     
     while (*text) {
         draw_char(cursor_x, y, *text);
         cursor_x += 6; // 5 pixels wide + 1 pixel space
         text++;
         
         // Wrap text if it goes off screen
         if (cursor_x > DISPLAY_WIDTH - 6) {
             cursor_x = x;
             y += 8; // Move to next line (font height is 7, add 1 for spacing)
         }
     }
 }
 
 void clear_buffer() {
     for (int i = 0; i < sizeof(buffer); i++) {
         buffer[i] = 0;
     }
 }