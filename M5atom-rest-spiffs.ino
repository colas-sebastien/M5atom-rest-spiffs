/*
    M5atom-rest-spiffs

    Author: Sébastien Colas 2025-04-27

    Tool to
    - create SPIFFS on your M5Atom (format)
    - write a file on SPIFFS
    - list files on SPIFFS
    - play an MP3 file store on SPIFFS

    Tested with:
    - M5 Atom Lite
    - M5 Atom Echo

    Usage:
    - Specify your wifi_ssid and wifi_pwd
    - Upload the code
    - retreive tour IP from the Serial Monitor (assuming your IP is 192.168.0.1)
    - to format the SPIFFS system
      curl http://192.168.0.1/format
    - to upload a file
      curl -F "file=@modem.mp3" http://192.168.0.1/upload
    - list files on SPIFFS
      curl http://192.168.0.1/list
    - to play a mp3 (only m5 Atom Echo)
      curl "http://192.168.0.1/play?mp3=modem"
*/

#include <M5Atom.h>
#include <WiFi.h>
#include <WiFiMulti.h>
#include <WebServer.h>
#include <SPIFFS.h>
#include "AudioFileSourceSPIFFS.h"
#include "AudioFileSourceID3.h"
#include "AudioGeneratorMP3.h"
#include "AudioOutputI2SNoDAC.h"

const char *wifi_ssid="YOUR WIFI SSID";
const char *wifi_pwd="YOUR WIFI PASSWORD";


// Web server running on port 80
WebServer server(80);
WiFiMulti wifiMulti;

AudioGeneratorMP3 *mp3;
AudioFileSourceSPIFFS *file;
AudioOutputI2SNoDAC *out;
AudioFileSourceID3 *id3;

File fsUploadFile; 
// upload a new file to the SPIFFS
void handleFileUpload() 
{   
    HTTPUpload& upload = server.upload();
    if(upload.status == UPLOAD_FILE_START)
    {
        String filename = upload.filename;
        if(!filename.startsWith("/")) filename = "/"+filename;
        Serial.print("handleFileUpload Name: "); 
        Serial.println(filename);
        fsUploadFile = SPIFFS.open(filename, "w"); // Open the file for writing in FS (create if it doesn't exist)                
    }
    else if(upload.status == UPLOAD_FILE_WRITE)
    {
        if(fsUploadFile) fsUploadFile.write(upload.buf, upload.currentSize); // Write the received bytes to the file
        Serial.print("=");
    }
    else if(upload.status == UPLOAD_FILE_END)
    {
        if(fsUploadFile)
        {
          Serial.println("> OK");
        }
        else
        {
          Serial.println("> KO");
        }        

        if(fsUploadFile) // If the file was successfully created
        {
            fsUploadFile.close();            
            Serial.print("handleFileUpload Size: "); 
            Serial.println(upload.totalSize);
            server.send(200, "application/json", "{\"upload\":\"success\"}");
        }
        else
        {
            server.send(500, "application/json", "{\"upload\":\"failed\"}");
        }
    }
}
 
void formatSPIFFS() {
  Serial.println("Formating SPIFFS");
  SPIFFS.format();
  bool success = SPIFFS.begin();
  if (success)
  {
    server.send(200, "application/json", "{\"format\":\"success\"}");
  }
  else
  {
    server.send(500, "application/json", "{\"format\":\"failed\"}");
  }  
}

void listDir() 
{
    File root = SPIFFS.open("/");
    String json = "[";
    String fileName;
    String fileSize;    

    Serial.println("File list:");
    File file = root.openNextFile();
    while (file) {
        if (file.isDirectory()) {
            Serial.print("  DIR : ");
            Serial.println(file.name());
        } else {
            fileName = file.name();      
            fileSize = file.size();          
            Serial.print("  FILE: ");
            Serial.print(fileName);
            Serial.print("  SIZE: ");
            Serial.println(fileSize);
            json+="{\"filename\":\"" + fileName + "\",\"size\":"+fileSize+"}";
        }
        file = root.openNextFile();
    }
    json += "]";

    server.send(200, "application/json", json);    
}

// Called when a metadata event occurs (i.e. an ID3 tag, an ICY block, etc.
void MDCallback(void *cbData, const char *type, bool isUnicode, const char *string) {
  (void)cbData;
  Serial.printf("ID3 callback for: %s = '", type);

  if (isUnicode) {
    string += 2;
  }

  while (*string) {
    char a = *(string++);
    if (isUnicode) {
      string++;
    }
    Serial.printf("%c", a);
  }
  Serial.printf("'\n");
  Serial.flush();
}

void playMP3fromSPIFFS(String filenameString)
{
  const char *filename = filenameString.c_str();
  audioLogger = &Serial;
  file = new AudioFileSourceSPIFFS(filename);
  id3 = new AudioFileSourceID3(file);
  id3->RegisterMetadataCB(MDCallback, (void*)"ID3TAG");
  out = new AudioOutputI2SNoDAC();
  mp3->begin(id3, out);
}

void playMP3()
{
  if (server.hasArg("mp3"))
  {
    String filenameString="/"+server.arg("mp3")+".mp3";    
    if (SPIFFS.exists(filenameString))
    {
      playMP3fromSPIFFS(filenameString);      
      server.send(200, "application/json", "{\"mp3\":\"playing\"}");
    }
    else
    {
      Serial.println("File does not exist: "+filenameString);
      server.send(400, "application/json", "{\"mp3\":\"file not found\"}");
    }
  }
}

void setupApi() {
  server.on("/format", formatSPIFFS);
  server.on("/list", listDir);
  server.on("/play", playMP3);
  server.on("/upload", HTTP_POST, []() {  }, handleFileUpload);
  server.begin();
}

void setup() {
  // put your setup code here, to run once:
  M5.begin();

  Serial.begin(115200);
  // This delay gives the chance to wait for a Serial Monitor without blocking if none is found
  delay(1500); 

  wifiMulti.addAP(wifi_ssid, wifi_pwd);
                                           
  Serial.print("\nConnecting Wifi...\n");                                             

  if (wifiMulti.run() == WL_CONNECTED) 
  {
    Serial.print("\nConnected...\n");
    Serial.println(WiFi.localIP()); 
  }

  setupApi();

  SPIFFS.begin(); // Start the SPI Flash File System (SPIFFS)
  Serial.println("SPIFFS started.\n");
  mp3 = new AudioGeneratorMP3();

}

void loop() {
  server.handleClient();
  if (mp3->isRunning()) {
    if (!mp3->loop()) {
      out->flush();
      mp3->stop();
    }
  }
}