# ESP32 I2S Audio Example
This demostrates playback of both SD card audio files and internet radio streaming URLs from a playlist, and cycling between them with a button press.

# About the PCB

An integrated PCB designed for use with internet radio. 
Its an ESP32 board based on 38-pin WROVER (with PSRAM) designed for audio streaming
or playback from the onboard SD Card slot. It can be used to retrofit an old radio or electronic toy; 
by interfacing with switches, controls, and LEDs the device bring a non-digital device back to the 
modern world and make use of the existing controls. 

The PCB includes an integrated power and protection circuit, so it's safe to connect usb in any 
situation. There's an on-board MAX98357 DACs, allowing for a 3W amplified speaker. 
Connections in JST SH (1.0m) for I2C, speaker, and a neopixel strip 
(which is also level-shifted and buffered for 5v). 

<img src="assets/v1-build-pic.png" />
