// Include necessary libraries
#include "pico/stdlib.h"    // Standard library for Raspberry Pi Pico
#include "hardware/spi.h"    // SPI library for communication with DAC
#include "hardware/dma.h"    // DMA library for direct memory access

// Define constants for buffer size and SPI settings
#define BUFFER_SIZE 64       // Number of audio samples per buffer
#define SPI_PORT spi0        // Use SPI0 for communication
#define SPI_SCK 18            // SPI clock pin (SCK)
#define SPI_MOSI 19           // SPI data pin (MOSI)
#define SPI_CS 17             // Chip select pin for DAC

// Declare double buffers (to store audio samples)
uint16_t buffer_A[BUFFER_SIZE];  // First buffer (active)
uint16_t buffer_B[BUFFER_SIZE];  // Second buffer (inactive)
volatile bool buffer_A_active = true;  // Track which buffer is active

// Declare DMA variables
int dma_channel;                // DMA channel number
dma_channel_config dma_config;   // DMA configuration struct

uint16_t dac_value = 0;

// -----------------------------------------
// Function to Initialize SPI for DAC
// -----------------------------------------
void spi_init_dac() {
    spi_init(SPI_PORT, 1000000);  // Initialize SPI at 1 MHz speed
    gpio_set_function(SPI_SCK, GPIO_FUNC_SPI);   // Set SCK pin function to SPI
    gpio_set_function(SPI_MOSI, GPIO_FUNC_SPI);  // Set MOSI pin function to SPI
    gpio_init(SPI_CS);              // Initialize chip select (CS) pin
    gpio_set_dir(SPI_CS, GPIO_OUT);  // Set CS pin as output
    gpio_put(SPI_CS, 1);             // Set CS high (inactive)
}

// -----------------------------------------
// Function to Initialize DMA for Audio Output
// -----------------------------------------
void dma_init_audio() {
    // Claim a free DMA channel
    dma_channel = dma_claim_unused_channel(true);

    // Get default DMA settings for the channel
    dma_config = dma_channel_get_default_config(dma_channel);

    // Configure DMA to transfer 16-bit values (since DAC expects 12-bit/16-bit data)
    channel_config_set_transfer_data_size(&dma_config, DMA_SIZE_16);

    // Link DMA to SPI (so it sends data only when SPI is ready)
    channel_config_set_dreq(&dma_config, spi_get_dreq(SPI_PORT, true));

    // Configure DMA to transfer audio data from memory (buffer_A initially) to SPI
    dma_channel_configure(
        dma_channel, &dma_config,
        &spi_get_hw(SPI_PORT)->dr,  // Destination: SPI data register
        buffer_A,                    // Initial source: buffer_A
        BUFFER_SIZE,                  // Number of samples to send
        false                         // Do not start immediately
    );

    // Enable DMA interrupt (to handle buffer swapping when a transfer is done)
    dma_channel_set_irq0_enabled(dma_channel, true);
    irq_set_exclusive_handler(DMA_IRQ_0, dma_irq_handler);
    irq_set_enabled(DMA_IRQ_0, true);
}

// -----------------------------------------
// DMA Interrupt Handler (Runs When DMA Completes a Transfer)
// -----------------------------------------
void dma_irq_handler() {
    // Clear the DMA interrupt flag (so we don’t trigger it again immediately)
    dma_hw->ints0 = 1u << dma_channel;

    // Swap buffers: If buffer_A was active, switch to buffer_B, and vice versa
    buffer_A_active = !buffer_A_active;

    // Start new DMA transfer using the other buffer (while CPU fills the old one)
    dma_channel_transfer_to_buffer_now(
        dma_channel, 
        buffer_A_active ? buffer_A : buffer_B,  // Select the inactive buffer
        BUFFER_SIZE
    );
}

// -----------------------------------------
// Timer Callback to Fill the Next Buffer
// -----------------------------------------
bool audio_timer_callback(struct repeating_timer *t) {
    uint16_t *buffer_to_fill = buffer_A_active ? buffer_B : buffer_A;  // Fill the inactive buffer

    // Generate or load next audio samples
    for (int i = 0; i < BUFFER_SIZE; i++) {
        buffer_to_fill[i] = get_next_audio_sample();  // Replace with your sample generation function
    }

    return true;  // Keep the timer running
}

// -----------------------------------------
// Function to Start Audio Playback
// -----------------------------------------
void start_audio_playback(float sample_rate) {
    struct repeating_timer timer;

    // Set up a repeating timer to refill buffers at the correct sample rate
    add_repeating_timer_us(
        -1000000 / sample_rate,   // Interval (in microseconds) for each sample
        audio_timer_callback,     // Function to call
        NULL,                     // No additional parameters
        &timer                     // Store the timer reference
    );

    // Start the first DMA transfer
    dma_channel_start(dma_channel);
}

// -----------------------------------------
// Main Program Entry Point
// -----------------------------------------
int main() {
    stdio_init_all();  // Initialize standard I/O (for debugging if needed)
    spi_init_dac();    // Initialize SPI
    dma_init_audio();  // Initialize DMA for audio output

    start_audio_playback(22050);  // Start playback at 22.05 kHz sample rate

    while (1) {
        tight_loop_contents();  // Main loop does nothing (DMA handles everything)
    }
}

// ----------------------------------------
// Audio Sample Generation
// ----------------------------------------
unit16_t get_next_audio_sample() {
    dac_value += 10;

    if (dac_value >= 3900) dac_value = 0;
    
    // Build the 16-bit command word for MCP4922.
    // Command word format:
    //   Bit 15: Channel Select (0 = Channel A)
    //   Bit 14: Buffer bit (1 = Buffered)
    //   Bit 13: Gain (1 = 1×, i.e., Vout = (DAC/4095)*Vref)
    //   Bit 12: Shutdown (1 = Active mode)
    //   Bits 11-0: 12-bit DAC value.
    uint16_t command = (0 << 15)     // Channel A
        | (1 << 14)     // Buffered output
        | (1 << 13)     // Gain = 1× (Vref)
        | (1 << 12)     // Active mode (not shutdown)
        | (dac_value & 0x0FFF); // 12-bit data

    return command;
}

