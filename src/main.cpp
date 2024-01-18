#include <Arduino.h>
#include <Preferences.h>
#include <SPI.h>
#include <WiFi.h>
#include <SD.h>
#include <FS.h>
#include <vector>  // for arrays
#include "Ticker.h"
#include "Audio.h" // "https://github.com/schreibfaul1/ESP32-audioI2S"

#define I2S_DOUT      13
#define I2S_BCLK      14
#define I2S_LRC       12
#define SD_CS          5
#define SPI_MOSI      23
#define SPI_MISO      19
#define SPI_SCK       18

Preferences pref;
Audio audio;
Ticker ticker;

String ssid =     "foz iPhone13";
String password = "bunnybella";

std::vector<String> media;
String stations[] ={
        "http://stream.srg-ssr.ch/m/rsj/mp3_128",
        "http://stream01.3fach.ch:8001/live",
        "https://stream.srg-ssr.ch/rsp/aacp_48.asx", // SWISS POP
        "http://centova.radios.pt/proxy/496?mp=/stream",
        "http://radiomeuh.ice.infomaniak.ch:8000/radiomeuh-128.mp3"
};

//some global variables

uint8_t max_volume   = 21;
uint8_t max_stations = 0;   //will be set later
uint8_t cur_station  = 0;   //current station(nr), will be set later
uint8_t cur_volume   = 0;   //will be set from stored preferences
int8_t  cur_btn      =-1;   //current button (, -1 means idle)

enum action{VOLUME_UP=0, VOLUME_DOWN=1, STATION_UP=2, STATION_DOWN=3};
enum staus {RELEASED=0, PRESSED=1};

struct _btns{
    uint16_t x; //PosX
    uint16_t y; //PosY
    uint16_t w; //Width
    uint16_t h; //Hight
    uint8_t  a; //Action
    uint8_t  s; //Status
};
typedef _btns btns;

btns btn[4];

// List contents of SD card
void loadMedia(File dir) {
  while (true) {
    File entry =  dir.openNextFile();

    if (! entry) { // no more files
      break;
    }

    if (String(entry.name()).startsWith(".")) continue; // Skip hidden files

    // Serial.print(entry.name());
    if (entry.isDirectory()) {
      //Serial.println("/");
      loadMedia(entry);
    } else {
      // files have sizes, directories do not
      //Serial.print(entry.size()/1024, DEC);
      //Serial.println("KB");
      media.push_back(entry.name());
    }
    entry.close();
  }
}

void loadStations(){
  Serial.println("Stations: ");
  for (int i=0; i<max_stations; i++){
    // Serial.print(" > ");
    // Serial.println(stations[i]);
    media.push_back(stations[i]);
  }
}

void statusReport(){
    Serial.println("------------------");
    // print uptime
    uint32_t ms = millis();
    uint32_t s = ms / 1000;
    uint32_t m = s / 60;
    uint32_t h = m / 60;
    Serial.printf("Uptime: %02d:%02d:%02d\n", h, m % 60, s % 60);
    // Wifi strength
    Serial.printf("WiFi (%s) RSSI: %ddB\n", WiFi.SSID(), WiFi.RSSI());
    // song position
    if (audio.isRunning()){
        if (media.at(cur_station).startsWith("http")){
            Serial.printf("Playback: web stream (%ds)\n", audio.getTotalPlayingTime()/1000);
        } else {
            Serial.printf("Playback: %ds (%d%%)\n", audio.getTotalPlayingTime()/1000, 100 - 100*(audio.getAudioFileDuration() - audio.getAudioCurrentTime())/audio.getAudioFileDuration(), audio.getAudioFileDuration());   
        }
    } else {
        Serial.println("Playback: stopped");
    }
    // memory stats    
    Serial.printf("Used PSRAM: %u KB (%d%%)\n", (ESP.getPsramSize() - ESP.getFreePsram())/1024, 100 - ESP.getFreePsram()*100/ESP.getPsramSize());
    Serial.printf("Used heap: %u KB (%d%%)\n", (ESP.getHeapSize() - ESP.getFreeHeap())/1024, 100 - ESP.getFreeHeap()*100/ESP.getHeapSize());  
    // audio buffer
    Serial.printf("Audio buffer: %dKB (%d%%)\n", audio.inBufferFilled()/1024, 100 - audio.inBufferFree()*100/(audio.inBufferFree() + audio.inBufferFilled()));
    
}

void playMedia(int station_num){
    String media_path = media.at(cur_station);            
    Serial.printf("[%d] %s\n", cur_station, media_path.c_str());
    if (media_path.startsWith("http")){
        audio.connecttohost(media_path.c_str());
    } else {
        audio.connecttoFS(SD, media_path.c_str());
    }
}

//**************************************************************************************************
//                                           S E T U P                                             *
//**************************************************************************************************
void setup() {
    btn[0].a=STATION_DOWN; btn[0].s=RELEASED;
    btn[1].a=STATION_UP;   btn[1].s=RELEASED;
    btn[2].a=VOLUME_UP;    btn[2].s=RELEASED;
    btn[3].a=VOLUME_DOWN;  btn[3].s=RELEASED;
    max_stations= sizeof(stations)/sizeof(stations[0]); log_i("max stations %i", max_stations);
    Serial.begin(115200);

    delay(1000);
        // 获取 Flash 大小（以字节为单位）
    uint32_t flashSize = ESP.getFlashChipSize();
    Serial.printf("Flash Size: %u bytes\n", flashSize);

    // 获取 PSRAM 的大小（以字节为单位）
    // uint32_t psramSize = ESP.getMaxAllocPsram();
    // Serial.printf("PSRAM Size: %u bytes\n", psramSize);

      // 获取 PSRAM 的大小（以字节为单位）
    uint32_t psramSize = ESP.getPsramSize();
    Serial.printf("PSRAM Size: %u bytes\n", psramSize);

    // 获取剩余内存
    uint32_t freeMemory = ESP.getFreeHeap();
    Serial.printf("Free Memory: %u bytes\n", freeMemory);

    // 获取总内存
    uint32_t Totalheap = ESP.getHeapSize();
    Serial.printf("Total heap: %u bytes\n", Totalheap);

    // 获取 PSRAM 剩余内存
    uint32_t FreePSRAM = ESP.getFreePsram();
    Serial.printf("Free PSRAM: %u bytes\n", FreePSRAM);

    pinMode(0, INPUT_PULLUP); // user button

    Serial.print("Initializing SD card... ");

    SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);
    if (SD.begin(SD_CS)){
        Serial.println("OK");
        File root = SD.open("/");
        loadMedia(root);
        root.close();
        loadStations();
    } else {
        Serial.println("[!] SD card initialization failed");
        return;
    }

    cur_station = random(media.size());
    cur_volume = 14;

    ticker.attach(10, statusReport); // periodically report status

    Serial.println("Media List:");
    for (int i=0; i<media.size(); i++){
        Serial.print(" [");
        Serial.print(i);
        Serial.print("] ");
        Serial.println(media[i]);
    }

    WiFi.begin(ssid.c_str(), password.c_str());
    Serial.printf("Connect to WIFI: %s ...", ssid.c_str());
    while (WiFi.status() != WL_CONNECTED){
        delay(1000);
        Serial.print(".");
    }
    Serial.println(" OK");

    audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
    audio.setVolume(cur_volume); // 0...21
    audio.forceMono(true); // if you have a mono speaker
    audio.setBufsize(1600*5, UINT16_MAX * 10); // if you experience audio dropouts, try to increase this value

    playMedia(cur_station);
}

void nextStation(){
    audio.stopSong();
    Serial.println("--------------");
    Serial.print("Change station: ");
    cur_station = (cur_station + 1) % media.size();
    playMedia(cur_station);
}

//**************************************************************************************************
//                                            L O O P                                              *
//**************************************************************************************************
static int counter = 0;
void loop()
{
    audio.loop();
    counter++;
    if (digitalRead(0) == LOW) {
        delay(100);
        if (digitalRead(0) == LOW) {
            nextStation();
        }
    }
}
//**************************************************************************************************
//                                           E V E N T S                                           *
//**************************************************************************************************
void audio_info(const char *info){
    Serial.printf("audio_info: %s\n", info);
}
void audio_showstation(const char *info){
    Serial.printf("Station: %s\n", String(info));
}
void audio_showstreamtitle(const char *info){
    String sinfo=String(info);
    sinfo.replace("|", "\n");
    Serial.printf("Stream Title: %s\n", sinfo.c_str());
}
void audio_eof_mp3(const char *info){
    Serial.printf("end of file: %s\n", info);
    nextStation();
}
void audio_eof_stream(const char *info){
    Serial.printf("end of stream: %s\n", info);
    nextStation();
}