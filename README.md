# M5atom REST SPIFFS
Manage your SPIFFS on your M5 Atom

## What does the program ?
- create SPIFFS on your M5Atom (format)
- write a file on SPIFFS
- list files on SPIFFS
- play an MP3 file store on SPIFFS (only for M5 Atom Echo)

## Tested with:
- M5 Atom Lite
- M5 Atom Echo

## Usage:
- Specify your *wifi_ssid and *wifi_pwd*
- Upload the code
- retreive tour IP from the Serial Monitor (assuming your IP is 192.168.0.1)
- to format the SPIFFS system
    ```
    curl http://192.168.0.1/format
    ```
- to upload a file
    ```
    curl -F "file=@modem.mp3" http://192.168.0.1/upload
    ```
- list files on SPIFFS
    ```
    curl http://192.168.0.1/list
    ```
- to play a mp3 (only m5 Atom Echo)
    ```
    curl "http://192.168.0.1/play?mp3=modem"
    ```

## Play MP3 from SPIFFS on your projects
Assuming you want to play mp3 on you M5 Atom Project.
Here is the code you need:
```
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
```

The code is available in the *M5atom-mp3-spiffs* folder