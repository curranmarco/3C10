Double buffer between CPU and DMA:
- CPU will take button inputs, add array values and normalise to create mixed signal buffer
- DMA will then take this mixed signal buffer and send to the DAC at required frequency while the CPU mixes the next buffer
- This significantly reduces load on CPU as it does not have to send each sample at the required frequency while trying to mix signals and take button inputs
- Allows for smooth audio playback

Normalising:
- Ideally, don't want to be using basic normalisation where the mixed signal = (sample sum)/(number of sounds playing)
- Want to use non linear normalisation - Number of potential options:
  - May want to split each sound into its ADSR intervals
  - Peak based scaling
  - Soft clipping
