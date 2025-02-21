**Drum Kit**

_Features:_
  - Able to synthesise multiple sounds using functions on Pico -> DAC -> speaker
      - Downloading sounds from online as arrays
      - Converting to arrays and sending through Pico to DAC
      - Signal amplified before being sent to speaker
  - Able to create samples:
      - Button to start sample
      - Makes note of time when buttons are pressed for each sound
      - Stops recording when button pressed for second time (Memory constraints - how long can we sample for?)
      - Sample begins looping
  - Able to filter out frequencies at output of op-amp
      - Using potentiometers to vary frequencies to be filtered
  - Can change amplitude of sounds using piezo sensors
  - Display:
      - Show name of sample
      - Have a separate button to change in between sounds (this is what we would see on the display)
