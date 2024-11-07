
#define VERSION "\n=== AJU ESP32 Voice Assistant ==="

#include <WiFi.h>  // only included here
#include <SD.h>    // also needed in other tabs (.ino)

#include <Audio.h>  // needed for PLAYING Audio (via I2S Amplifier, e.g. MAX98357) with ..
                    // Audio.h library from Schreibfaul1: https://github.com/schreibfaul1/ESP32-audioI2S
                    // .. ensure you have actual version (July 18, 2024 or newer needed for 8bit wav files!)
#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <SimpleTimer.h>

#include <ESP32Servo.h>  // Include the ESP32Servo library

String text;
String filteredAnswer = "";
String repeat;
SimpleTimer Timer;

// --- PRIVATE credentials -----

const char* ssid = "YOUR SSID";                      // ## INSERT your wlan ssid
const char* password = "YOUR SSID PASSWORD";        // ## INSERT your password
const char* gemini_KEY = "YOUR GEMINI API KEY";    //gemini api

#define AUDIO_FILE "/Audio.wav"  // mandatory, filename for the AUDIO recording

#define TTS_GOOGLE_LANGUAGE "en-IN"  // needed for Google TTS voices only (not needed for multilingual OpenAI voices :) 
                                     // examples: en-US, en-IN, en-BG, en-AU, nl-NL, nl-BE, de-DE, th-TH etc. 
                                     // more infos: https://cloud.google.com/text-to-speech/docs/voices

// --- PIN assignments ---------

#define pin_RECORD_BTN 36
#define pin_repeat 13

#define pin_LED_RED 15
#define pin_LED_GREEN 2
#define pin_LED_BLUE 4

#define pin_I2S_DOUT 25  // 3 pins for I2S Audio Output (Schreibfaul1 audio.h Library)
#define pin_I2S_LRC 26
#define pin_I2S_BCLK 27

#define pin_SERVO1 12  // Servo 1 pin
#define pin_SERVO2 14  // Servo 2 pin

// --- global Objects ----------

Audio audio_play;

Servo servo1;  // Create Servo object for Servo 1
Servo servo2;  // Create Servo object for Servo 2



// Define constants for servo positions and sweep duration
#define maxAngle 30
#define minAngle 0
#define SWEEP_DELAY 15 // Delay for servo sweeping motion (milliseconds)

unsigned long previousMillis = 0;
int servo1Position = 90; // Initial position for servo 1
int servo2Position = 90; // Initial position for servo 2

bool increasing = true;

// Set the sweep speed (number of degrees to move per iteration)
const int sweepStep = 1; // Change this value to adjust how fast the servo sweeps

// declaration of functions in other modules (not mandatory but ensures compiler checks correctly)
// splitting Sketch into multiple tabs see e.g. here: https://www.youtube.com/watch?v=HtYlQXt14zU

bool I2S_Record_Init();
bool Record_Start(String filename);
bool Record_Available(String filename, float* audiolength_sec);

String SpeechToText_Deepgram(String filename);
void Deepgram_KeepAlive();

// ------------------------------------------------------------------------------------------------------------------------------
void setup() {
  // Initialize serial communication
  Serial.begin(115200);
  Serial.setTimeout(100);  // 10 times faster reaction after CR entered (default is 1000ms)



  // Pin assignments:
  pinMode(pin_LED_RED, OUTPUT);
  pinMode(pin_LED_GREEN, OUTPUT);
  pinMode(pin_LED_BLUE, OUTPUT);
  pinMode(pin_RECORD_BTN, INPUT);  // use INPUT_PULLUP if no external Pull-Up connected ##
  pinMode(pin_repeat, INPUT);
  pinMode(12, OUTPUT);
  digitalWrite(12, LOW);

   // Initialize servos
  servo1.attach(pin_SERVO1);  // Attach servo1 to pin SERVO1
  servo2.attach(pin_SERVO2);  // Attach servo2 to pin SERVO2
  servo1.write(0);
  servo2.write(0);

  
  // on INIT: walk 1 sec thru 3 RGB colors (RED -> GREEN -> BLUE), then stay on GREEN
  led_RGB(50, 0, 0);
  delay(500);
  led_RGB(0, 50, 0);
  delay(500);
  led_RGB(0, 0, 50);
  delay(500);
  led_RGB(0, 0, 0);


  // Hello World
  Serial.println(VERSION);
  Timer.setInterval(10000);
  // Connecting to WLAN
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting WLAN ");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println(". Done, device connected.");
  led_RGB(0, 50, 0);  // GREEN

  // Initialize SD card
  if (!SD.begin()) {
    Serial.println("ERROR - SD Card initialization failed!");
    return;
  }

  // initialize  I2S Recording Services (don't forget!)
  I2S_Record_Init();

  // INIT Audio Output (via Audio.h, see here: https://github.com/schreibfaul1/ESP32-audioI2S)
  audio_play.setPinout(pin_I2S_BCLK, pin_I2S_LRC, pin_I2S_DOUT);

  audio_play.setVolume(21);  //21
  // INIT done, starting user interaction

  // Play the welcome message
  playWelcomeMessage();

  Serial.println("> HOLD button for recording AUDIO .. RELEASE button for REPLAY & Deepgram transcription");
}

void playWelcomeMessage() {
  String welcome = "hello, how can i assist you!";
  speakTextInChunks(welcome, 93);  
  Serial.println("welcome");
  while (audio_play.isRunning())  // wait here until finished 
  {
    audio_play.loop();  // Wait here until done
  }
}

// -------------------------------------------------------------------------------------------
void loop() {


here:

  if (digitalRead(pin_RECORD_BTN) == LOW)  // Recording started (ongoing)
  {
    led_RGB(50, 0, 0);  //  RED means 'Recording ongoing'
    delay(30);          // unbouncing & suppressing button 'click' noise in begin of audio recording

    // Before we start any recording we stop any earlier Audio Output or streaming (e.g. radio)
    if (audio_play.isRunning()) {
      audio_play.connecttohost("");  // 'audio_play.stopSong()' wouldn't be enough (STT wouldn't reconnect)
    }

    //Start Recording
    Record_Start(AUDIO_FILE);
  }

  if (digitalRead(pin_RECORD_BTN) == HIGH)  // Recording not started yet .. OR stopped now (on release button)
  {
    led_RGB(0, 0, 0);

    float recorded_seconds;
    if (Record_Available(AUDIO_FILE, &recorded_seconds))  //  true once when recording finalized (.wav file available)
    {
      if (recorded_seconds > 0.4)  // ignore short btn TOUCH (e.g. <0.4 secs, used for 'audio_play.stopSong' only)
      {


        String transcription = SpeechToText_Deepgram(AUDIO_FILE);

        //led_RGB(HIGH, LOW, HIGH);  // GREEN means: 'Ready for recording'
        String again = "i can't understand.... can you Ask Again . . . ";
        

        Serial.println(transcription);
        if (transcription == "") {
          led_RGB(0, 0, 255);
          speakTextInChunks(again, 93);  // ( Uncomment this to use Google TTS )
          Serial.println("i can't understand.... can you Ask Again . . .");
          while (audio_play.isRunning())  // wait here until finished (just for Demo purposes, before we play Demo 4)
          {
            audio_play.loop();
          }
          goto here;
          //return;
        }



        //----------------------------------------------------
        WiFiClientSecure client;
        client.setInsecure();  // Disable SSL verification for simplicity (not recommended for production)
        String Answer = "";    // Declare Answer variable here

        text = "";

        if (client.connect("generativelanguage.googleapis.com", 443)) {
          String url = "/v1beta/models/gemini-1.5-flash:generateContent?key=" + String(gemini_KEY);

         String payload = String("{\"contents\": [{\"parts\":[{\"text\":\"" + transcription + "\"}]}],\"generationConfig\": {\"maxOutputTokens\": 150}}");



          // Send the HTTP POST request
          client.println("POST " + url + " HTTP/1.1");
          client.println("Host: generativelanguage.googleapis.com");
          client.println("Content-Type: application/json");
          client.print("Content-Length: ");
          client.println(payload.length());
          client.println();
          client.println(payload);

          // Read the response
          String response;
          while (client.connected()) {
            String line = client.readStringUntil('\n');
            if (line == "\r") {
              break;
            }
          }

          // Read the actual response
          response = client.readString();
          parseResponse(response);
        } else {
          Serial.println("Connection failed!");
        }

        client.stop();  // End the connection
        //----------------------------------------------------

        if (filteredAnswer != "")  // we found spoken text .. now starting Demo examples:
        {
          led_RGB(0, 0, 255);
          Serial.print("Gemini speaking: ");
          Serial.println(filteredAnswer);


         // Speak the filtered response from Gemini
        speakTextInChunks(filteredAnswer, 93);
        }
      }
    }
  }



  //for repeat-------------------------
  if (digitalRead(pin_repeat) == LOW) {
    delay(500);
    analogWrite(pin_LED_BLUE, 255);
    Serial.print("repeat - ");
    Serial.println(repeat);
    speakTextInChunks(repeat, 93);  // ( Uncomment this to use Google TTS )
  }

  audio_play.loop();

  if (audio_play.isRunning()) {

    analogWrite(pin_LED_BLUE, 255);
    if (digitalRead(pin_RECORD_BTN) == LOW) {
      goto here;
    }
  } else {


    analogWrite(pin_LED_BLUE, 0);
  }

 
  // Schreibfaul1 loop für Play Audio

  if (digitalRead(pin_RECORD_BTN) == HIGH && !audio_play.isRunning())  // but don't do it during recording or playing
  {
    static uint32_t millis_ping_before;
    if (millis() > (millis_ping_before + 5000)) {
      millis_ping_before = millis();
      led_RGB(0, 0, 0);  // short LED OFF means: 'Reconnection server, can't record in moment'
      Deepgram_KeepAlive();
    }
  }
}

void speakTextInChunks(String text, int maxLength) {
  int start = 0;
  while (start < text.length()) {
    int end = start + maxLength;

    // Ensure we don't split in the middle of a word
    if (end < text.length()) {
      while (end > start && text[end] != ' ' && text[end] != '.' && text[end] != ',') {
        end--;
      }
    }

    // If no space or punctuation is found, just split at maxLength
    if (end == start) {
      end = start + maxLength;
    }

    String chunk = text.substring(start, end);
    
    // Start audio playback
    audio_play.connecttospeech(chunk.c_str(), TTS_GOOGLE_LANGUAGE);

    // Sweep servos concurrently
    unsigned long previousMillis = millis();
    while (audio_play.isRunning()) {
      moveServosForSpeech(millis());
      audio_play.loop();
      if (digitalRead(pin_RECORD_BTN) == LOW) {
        break;
      }
    }

    start = end + 1;  // Move to the next part, skipping the space
  }
}

// ------------------------------------------------------------------------------------------------------------------------------

// Revised section to handle response parsing
void parseResponse(String response) {
  repeat = "";
  // Extract JSON part from the response
  int jsonStartIndex = response.indexOf("{");
  int jsonEndIndex = response.lastIndexOf("}");

  if (jsonStartIndex != -1 && jsonEndIndex != -1) {
    String jsonPart = response.substring(jsonStartIndex, jsonEndIndex + 1);
    // Serial.println("Clean JSON:");
    // Serial.println(jsonPart);

    DynamicJsonDocument doc(1024);  // Increase size if needed
    DeserializationError error = deserializeJson(doc, jsonPart);

    if (error) {
      Serial.print("DeserializeJson failed: ");
      Serial.println(error.c_str());
      return;
    }

    if (doc.containsKey("candidates")) {
      for (const auto& candidate : doc["candidates"].as<JsonArray>()) {
        if (candidate.containsKey("content") && candidate["content"].containsKey("parts")) {

          for (const auto& part : candidate["content"]["parts"].as<JsonArray>()) {
            if (part.containsKey("text")) {
              text += part["text"].as<String>();
            }
          }
          text.trim();
          // Serial.print("Extracted Text: ");
          // Serial.println(text);
          filteredAnswer = "";
          for (size_t i = 0; i < text.length(); i++) {
            char c = text[i];
            if (isalnum(c) || isspace(c) || c == ',' || c == '.' || c == '\'') {
              filteredAnswer += c;
            } else {
              filteredAnswer += ' ';
            }
          }
          // filteredAnswer = text;
          // Serial.print("FILTERED - ");
          //Serial.println(filteredAnswer);

          repeat = filteredAnswer;
        }
      }
    } else {
      Serial.println("No 'candidates' field found in JSON response.");
    }
  } else {
    Serial.println("No valid JSON found in the response.");
  }
}


void led_RGB(int red, int green, int blue) {
  static bool red_before, green_before, blue_before;
  // writing to real pin only if changed (increasing performance for frequently repeated calls)
  if (red != red_before) {
    analogWrite(pin_LED_RED, red);
    red_before = red;
  }
  if (green != green_before) {
    analogWrite(pin_LED_GREEN, green);
    green_before = green;
  }
  if (blue != blue_before) {
    analogWrite(pin_LED_BLUE, blue);
    blue_before = blue;
  }
}

void moveServosForSpeech(unsigned long currentMillis) {
    if (currentMillis - previousMillis >= SWEEP_DELAY) {
        previousMillis = currentMillis;

        // Update positions for each servo
        if (increasing) {
            servo1Position -= sweepStep; // Move servo 1 from 90 to 0 degrees
            servo2Position += sweepStep; // Move servo 2 from 90 to 180 degrees
            if (servo1Position <= 0) {  // Stop when servo1 reaches 0 degrees
                increasing = false;     // Change direction
            }
            if (servo2Position >= 180) { // Stop when servo2 reaches 180 degrees
                increasing = false;     // Change direction
            }
        } else {
            servo1Position += sweepStep; // Move servo 1 back from 0 to 90 degrees
            servo2Position -= sweepStep; // Move servo 2 back from 180 to 90 degrees
            if (servo1Position >= 90) {   // Stop when servo1 reaches 90 degrees
                increasing = true;        // Change direction
            }
            if (servo2Position <= 90) {   // Stop when servo2 reaches 90 degrees
                increasing = true;        // Change direction
            }
        }

        // Apply the updated servo positions
        servo1.write(servo1Position);
        servo2.write(servo2Position);
    }
}



