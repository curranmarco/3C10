// drum-kit.c
// Need to check configurations and pin initialisations match with hardware set up

// Initialise Libraries
#include "pico/stdlib.h"
#include "hardware/spi.h"    // SPI library for sending signals to DAC
#include "hardware/dma.h"    // DMA library for direct memory access

// SPI configuration
#define SPI_PORT    spi0
#define PIN_MISO    16   // Not used here since we're only transmitting
#define PIN_MOSI    19
#define PIN_SCK     18
#define PIN_CS      17   // Chip Select for the MCP4922

// Declare double buffers (to store audio samples)
#define BUFFER_SIZE  64          // Buffer 64 samples long, approximately 3 ms
uint16_t buffer_A[BUFFER_SIZE];  // First buffer (active)
uint16_t buffer_B[BUFFER_SIZE];  // Second buffer (inactive)
volatile bool buffer_A_active = true;  // Track which buffer is active

// Declare DMA variables
int dma_channel;                // DMA channel number
dma_channel_config dma_config;   // DMA configuration struct

// To track where the next sample should go when filling buffers
volatile int buffer_position = 0;  

// Button Initialisation for Drum Triggering
#define KICK_PIN 26
//#define SNARE_PIN 24
//#define HIHAT_PIN 25

#define KICK_LENGTH 16848
//#define SNARE_LENGTH 800
//#define HIHAT_LENGTH 500

// Will want to add actual array values here, at end of file or in header files
// May also want to consider storing in flash due to RAM constraints
uint16_t kick_sample[KICK_LENGTH]; 
//uint16_t snare_sample[SNARE_LENGTH];
//uint16_t hihat_sample[HIHAT_LENGTH];

// Track playback position for each sound
volatile int kick_position = -1;   // -1 means not currently playing
volatile int snare_position = -1;
volatile int hihat_position = -1;

// ----------------------------------------
// Order of function use:
// SPI initialisation is straightforward, runs once, reasonably incoonsequential to remaining code
//   - Speed needs to be significantly faster than DACs sample requests to send 16 bits per sample 
//     request (44.1kHz)
// DMA initialisation:
//   - dma_channel configured to send 16-bit data samples from buffers directly to DAC via SPI port
//   - DMA is set up to transfer samples at the rate requested by the DAC (44.1kHz)
//   - when BUFFER_SIZE = 64 samples have been transferred, a predefined hardware interrupt DMA_IRQ_0 is
//     automatically triggered
//   - dma_channel is configured to run dma_irq_handler() function when interrupt triggered by DMA_IRQ_0
// Interrupt handler:
//   - When dma_channel completes buffer transfer, interrupt triggered, and this function swaps buffers
//   - buffer A, just transferred by DMA, swaps with buffer B to be written to by CPU
// Audio Timer Callback:
//   - Managed by a timer function, defined in start_audio_playback() function
//   - When function is called by timer, loads the next sample value, calculated by get_next_sample() 
//     function
//   - Need to write one sample into the next buffer position per timer callback
// Sample Playback:
//   - Sets up timer for callback function to fill buffers at 44.1kHz 
//   - We want to write to buffers at this rate to avoid latency, i.e. if we fill buffer at each interrupt, 
//     we may miss out on capturing the attack when new beats are triggered
//   - Starts DMA transfer
// Button Check
//     - Constantly polls for button press, if press detected, corresponding drum has its array position incremented
// Get Next Sample
//     - Checks each drum to see if they are active
//     - If drum active, their current sample position added to mixed signal
//     - Mixed signal scaled down if it exceeds DAC input limits
//     - 12 bit input signal is converted to 16 bit DAC command line to be written into buffer

// -----------------------------------------
// Function to Initialize SPI for DAC
// -----------------------------------------
void spi_init_dac() {
    spi_init(SPI_PORT, 1000000);  // Initialize SPI at 1 MHz speed
    spi_set_format(spi, 16, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST); // 16-bit mode
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
    uint16_t *buffer_to_fill = buffer_A_active ? buffer_B : buffer_A;

    // Write one new sample at the current buffer position
    buffer_to_fill[buffer_position] = get_next_audio_sample();

    // Move to the next position
    buffer_position++;

    // If we reach the end of the buffer, reset the position (DMA IRQ will swap buffers)
    if (buffer_position >= BUFFER_SIZE) {
        buffer_position = 0;  // Reset for next cycle
    }
    return true;
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
        &timer                    // Store the timer reference
    );

    // Start the first DMA transfer
    dma_channel_start(dma_channel);
}

// ----------------------------------------
// Audio Sample Generation
// ----------------------------------------
unit16_t get_next_audio_sample() {
    int16_t mix = 0;

    // Add kick drum sample if active
    if (kick_position >= 0 && kick_position < KICK_LENGTH) {
        mix += kick_sample[kick_position];
        kick_position++;
        // Stop playback when we reach the end
        if (kick_position >= KICK_LENGTH) kick_position = -1;
    }
    
    // Add snare drum sample if active
    if (snare_position >= 0 && snare_position < SNARE_LENGTH) {
        mix += snare_sample[snare_position];
        snare_position++;
        if (snare_position >= SNARE_LENGTH) snare_position = -1;
    }
    
    // Add hi-hat sample if active
    if (hihat_position >= 0 && hihat_position < HIHAT_LENGTH) {
        mix += hihat_sample[hihat_position];
        hihat_position++;
        if (hihat_position >= HIHAT_LENGTH) hihat_position = -1;
    }
    
    // Scale only if the mixed value exceeds 12-bit DAC range (4096)
    // If, and it seems likely from the array values, the mixed sample is having to be constantly 
    // clipped, then may have to pass array values through function to scale down
    if (mix > 4095) {
        float scale = 4095 / mix;
        mix *= scale;
    }

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
}

// -----------------------------------------
// Button Trigger 
// -----------------------------------------
void check_button_triggers() {
    // Check kick drum button
    static bool kick_pressed = false;
    bool kick_current = !gpio_get(KICK_PIN); // Assumes active low buttons
    
    if (kick_current && !kick_pressed) {
        // Button just pressed, trigger kick drum
        kick_position = 0;
    }
    kick_pressed = kick_current;
    
    // Check snare drum button
    static bool snare_pressed = false;
    bool snare_current = !gpio_get(SNARE_PIN);
    
    if (snare_current && !snare_pressed) {
        // Button just pressed, trigger snare drum
        snare_position = 0;
    }
    snare_pressed = snare_current;
    
    // Check hi-hat button
    static bool hihat_pressed = false;
    bool hihat_current = !gpio_get(HIHAT_PIN);
    
    if (hihat_current && !hihat_pressed) {
        // Button just pressed, trigger hi-hat
        hihat_position = 0;
    }
    hihat_pressed = hihat_current;
}

// -----------------------------------------
// Main Program Entry Point
// -----------------------------------------
int main() {
    stdio_init_all();  // Initialize standard I/O (for debugging if needed)
    spi_init_dac();    // Initialize SPI
    dma_init_audio();  // Initialize DMA for audio output

    start_audio_playback(44100);  // Start playback at 44.1 kHz sample rate

    while (1) {
        tight_loop_contents();  // Main loop does nothing (DMA handles everything)
    }
}

// -------------------------------------
// Left over code from DAC sample generation
// -------------------------------------
/*
    uint16_t dac_values[4] = { 1000, 1800, 2500, 3400 };
    uint16_t state = 0;


    while (true) {
        // about 1.65V (4096 / 2)
        uint16_t dac_value = dac_values[state++];  // 12-bit value
        state %= 4;

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

        // Split the 16-bit command into two 8-bit bytes (MSB first)
        uint8_t data[2];
        data[0] = command >> 8;       // Most Significant Byte
        data[1] = command & 0xFF;     // Least Significant Byte

        // Transmit the command to the DAC via SPI
        gpio_put(PIN_CS, 0);          // Assert chip select (active low)
        spi_write_blocking(SPI_PORT, data, 2);
        gpio_put(PIN_CS, 1);          // Deassert chip select


        sleep_ms(1);

    }

    return 0;
*/
