#include <M5Atom.h>
#include <SPIFFS.h>
#include "AudioFileSourceSPIFFS.h"
#include "AudioFileSourceID3.h"
#include "AudioGeneratorMP3.h"
#include "AudioOutputI2SNoDAC.h"

AudioGeneratorMP3 *mp3;
AudioFileSourceSPIFFS *file;
AudioOutputI2SNoDAC *out;
AudioFileSourceID3 *id3;

void playMP3fromSPIFFS(String filenameString)
{
  const char *filename = filenameString.c_str();
  file = new AudioFileSourceSPIFFS(filename);
  id3 = new AudioFileSourceID3(file);
  out = new AudioOutputI2SNoDAC();
  mp3->begin(id3, out);
}

void setup() {
  M5.begin();
  SPIFFS.begin();
  mp3 = new AudioGeneratorMP3();
  playMP3fromSPIFFS("/modem.mp3");
}

void loop() {  
  if (mp3->isRunning()) {
    if (!mp3->loop()) {
      out->flush();
      mp3->stop();
    }
  }
}