/*
 * This program uses the BleKeyboard library to convert the ESP32 into a programmable "bluetooth keyboard".
 * Using buttons connected to IO pins, the ESP32 can be triggered to send keystroke (including hot-key combination) messages via bluetooth to Ableton Live.
 * 
 * FUNCTIONALITY:
 * 
 * PRESS TYPE | LEFT PEDAL  |  RIGHT PEDAL
 * ---------------------------------------
 *  Short     | Next Track  |  Toggle: Record/Play/Restart
 *  Long      | Stop Track  |  N/A
 *  
 *  NOTES: 
 *  "Next Track" will go to the track directly below the current track (equivalent to Down Arrow).
 *  "Record" only activates on first press of a new track.
 *  "Play" activates upon second press in existing track.
 *  "Restart" activates upon all subsequent presses in an existing track.
 */
#include <BleKeyboard.h>

BleKeyboard bleKeyboard;    //Create bluetooth keyboard object

//Create constants for the pedals and assign pin values
const int pedal_left = 4; //Select or Clear track
const int pedal_right = 5;  //Record or Play track
int select = 0;  //flag for selecting a track (pedal pressed = 1, released = 0)
int record = 0;  //flag for current action in Ableton (recording = 1, playing = 0)
unsigned long pedal_delay = 0;
unsigned long last_delay = 0;
int long_press = 1000;
int new_track = 0;

void setup() {
  //Pin Setup
  pinMode(pedal_left, INPUT_PULLUP);    //pedal pin states are pulled high as their default state
  pinMode(pedal_right, INPUT_PULLUP);
  
  //Display feedback Setup
  Serial.begin(115200);
  Serial.println("Starting BLE Guitar Pedal!");
  
  bleKeyboard.begin();
}

void loop() {
  if(bleKeyboard.isConnected()) {   //Only respond if the ESP32 is paired with the target device
    //LEFT PEDAL press
    if(record == 0) {   //Only accept Select Pedal presses if track is NOT recording
      if(digitalRead(pedal_left) == LOW && select == 0) {    //Select Pedal (left) is pressed
        select = 1;   //set flag that pedal has been pressed
        last_delay = millis();    //record time of pedal press
        Serial.println();
        Serial.println("LEFT Pressed...");
      }
      else if(digitalRead(pedal_left) == HIGH && select == 1) {    //Select Pedal (left) is released
        select = 0;   //reset flag that pedal was released
        pedal_delay = millis();   //record time of pedal release
        Serial.print("LEFT Released...");
        if(pedal_delay - last_delay >= long_press) {   //check if "long press" threshold has been reached
          bleKeyboard.write(48);   //send ASCII code for numeric zero key
          delay(500);
          Serial.println("TOGGLE: STOP/PLAY");
        }
        else {    //otherwise if "short press"
          bleKeyboard.write(217);   //send ASCII code for "Down Arrow" key to select next track
          delay(500);
          new_track = 1;    //set flag that we have switched to a new track
          Serial.println("NEXT TRACK");
        }
      }
    }
    //RIGHT PEDAL press
    if(digitalRead(pedal_right) == LOW && record == 0) {    //if pedal pressed while NOT recording
      Serial.println();
      Serial.print("RIGHT Pressed...");
      bleKeyboard.write(KEY_RETURN);
      delay(1000);
      if(new_track == 1) {
        record = 1;   //set flag that recording is in progress
        new_track = 0;    //set flag that we are currently in an active recording track and have not switched to a new track
        Serial.println("RECORDING");
      }
      else {
        Serial.println("RESTARTING");
      }
      
    }
    else if(digitalRead(pedal_right) == LOW && record == 1) {   //if pedal pressed WHILE recording
      record = 0;
      Serial.print("RIGHT Pressed...");
      bleKeyboard.write(KEY_RETURN);
      delay(1000);
      Serial.println("PLAYING");
    }
  }
}
