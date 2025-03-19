// Peak-Based Audio Scaling
int16_t mix_drums(int16_t *samples, int num_samples) {
    int32_t mix = 0;
    int32_t peak = 0;  // Track peak of the mixed signal

    // Sum all active samples
    for (int i = 0; i < num_samples; i++) {
        mix += samples[i];  
    }

    peak = abs(mix);  // Track peak of the mixed sum

    // Scale only if the mixed value exceeds 12-bit DAC range (2047)
    if (peak > 2047) {
        float scale = 2047.0 / peak;
        mix = (int16_t)(mix * scale);
    }

    return mix;
}
