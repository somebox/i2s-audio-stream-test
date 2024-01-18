/**
 * @file streams-url_mp3-i2s.ino
 * @author Phil Schatzmann
 * @brief decode MP3 stream from url and output it on I2S
 * @version 0.1
 * @date 2021-96-25
 * 
 * @copyright Copyright (c) 2021
 */

// install https://github.com/pschatzmann/arduino-libhelix.git

#include "AudioTools.h"
// #include "AudioCodecs/CodecMP3MAD.h"
#include "AudioCodecs/CodecMP3Helix.h"

// #define MP3_MAX_OUTPUT_SIZE 2048 * 5
// #define MP3_MAX_FRAME_SIZE 3200

URLStream url("X9-2G","bunnybella");

MetaDataOutput out1; // final output of metadata
I2SStream i2s; // I2S output
MP3DecoderHelix helix; // MP3 Decoder
EncodedAudioStream out2dec(&i2s, &helix); // Decoding stream
MultiOutput out;
StreamCopy copier(out, url); // copy url to decoder

/// callback for meta data
void printMetaData(MetaDataType type, const char* str, int len){
  Serial.print("==> ");
  Serial.print(toStr(type));
  Serial.print(": ");
  Serial.println(str);
}

static int count = 0;
void setup(){
  Serial.begin(115200);
  delay(1000);

  AudioLogger::instance().begin(Serial, AudioLogger::Warning);  

// setup multi output
  out.add(out1);
  out.add(out2dec);

  // mp3 radio
  url.httpRequest().header().put("Icy-MetaData","1");
  url.begin("http://stream.srg-ssr.ch/m/rsj/mp3_128","audio/mp3");

  // setup metadata
  out1.setCallback(printMetaData);
  out1.begin(url.httpRequest());

  // setup i2s
  auto config = i2s.defaultConfig(TX_MODE);

  // define pins
  config.pin_ws = 12;
  config.pin_bck = 14;
  config.pin_data = 13;
  config.i2s_format = I2S_LSB_FORMAT;
  config.buffer_size = 1024;
  config.channels = 1;
  config.sample_rate = 44100;
  i2s.begin(config);

  // setup I2S based on sampling rate provided by decoder
  out2dec.setNotifyAudioChange(i2s);
  out2dec.begin();

  // setup I2S based on sampling rate provided by decoder
  helix.setMaxPCMSize(2048 * 5);
  helix.setMaxFrameSize(3200);
  out2dec.setNotifyAudioChange(i2s);
  out2dec.begin();
}

void loop(){
  if (count++ % 100 == 0){
    Serial.print("Total heap: ");
    Serial.println(ESP.getHeapSize());
    Serial.print("Free heap: ");
    Serial.println(ESP.getFreeHeap());
    Serial.print("Total PSRAM: ");
    Serial.println(ESP.getPsramSize());
    Serial.print("Free PSRAM: ");
    Serial.println(ESP.getFreePsram());
  }
  copier.copy();
}