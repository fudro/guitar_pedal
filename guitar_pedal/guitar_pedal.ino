/*
 * This program uses the BleKeyboard library to convert the ESP32 into a programmable "bluetooth keyboard".
 * Using buttons connected to IO pins, the ESP32 can be triggered to send keystroke messages
 * (including hot-key combinations) to Ableton Live via bluetooth.
 * 
 * FUNCTIONALITY:
 * 
 * PRESS TYPE | LEFT PEDAL                    |  RIGHT PEDAL
 * --------------------------------------------------------
 *  Short     | Toggle: Record/Play/Restart   |  Next Track (Right Arrow)
 *  Long      | N/A                           |  Stop Track/Retry Loop
 *  
 *  NOTES: 
 *  "Next Track" will go to the track directly below the current track (equivalent to Down Arrow).
 *  "Record" only activates on first press of a new track.
 *  "Play" activates upon second press in existing track.
 *  "Restart" activates upon all subsequent presses in an existing track.
 *  
 *  REFERENCE FOR KEYCODES
 *  https://www.arduino.cc/reference/en/language/functions/usb/keyboard/keyboardmodifiers/
 */
#include <BleKeyboard.h>

BleKeyboard bleKeyboard;    //Create bluetooth keyboard object

//Create constants for the pedals and assign pin values
const int pedal_left = 4;  //Record, Play, or Restart track
const int pedal_right = 5; //Select or Stop track(Retry)
int select = 0;  //flag for state of right_pedal used to make track selections (pressed = 1, released = 0)
int record = 0;  //flag for current state of selected track (recording = 1, not-recording = 0)
int playback = 0;   //flag for current state of the selected track (stopped = 0, playback = 1, recording = 2)
int new_track = 1;    //flag to tell if a track contains an existing recording or is "new" (default)
unsigned long pedal_delay = 0;    //Variables to calculate length of Left pedal press
unsigned long last_delay = 0;
int long_press = 1000;    //Delay threshold for the minimum duration of a "long press" in milliseconds


void setup() {
  //create keyboard object
  bleKeyboard.begin();  
    
  //define pedal pins
  pinMode(pedal_left, INPUT_PULLUP);    //pedal pin states are pulled high as their default state
  pinMode(pedal_right, INPUT_PULLUP);    //Due to the special nature of pin 2 ("strapping pin" functionality as well as being connected to built-in LED)...
                                  //...set this pin to INPUT and use an external pull up resistor.
  //Display feedback
  Serial.begin(115200);
  Serial.println("Starting BLE Guitar Pedal!");
}
  

void loop() {
  if(bleKeyboard.isConnected()) {   //Only respond if the ESP32 is paired with the target device
    //RIGHT PEDAL press
    if(record == 0) {   //Only accept Right Pedal presses if track is NOT recording
      if(digitalRead(pedal_right) == LOW && select == 0) {    //Right Pedal is pressed
        select = 1;   //set flag that pedal has been pressed
        last_delay = millis();    //record time of pedal press
        Serial.println();
        Serial.println("RIGHT Pressed...");
      }
      else if(digitalRead(pedal_right) == HIGH && select == 1) {    //Right Pedal is released
        select = 0;   //reset flag now that pedal was released
        pedal_delay = millis();   //record time of pedal release
        Serial.print("RIGHT Released...");
        if(pedal_delay - last_delay >= long_press) {   //check if "long press" threshold has been reached
          bleKeyboard.write(48);   //send ASCII code for numeric zero to toggle track to "stop" mode
          delay(500);
          playback = 0;
          Serial.println("STOP Track");
          bleKeyboard.write(217);   //send ASCII code for "Down Arrow" to select next slot within same track
          delay(500);
          new_track = 1;    //Since we are in a new slot within the old track, set "new_track" to 1 (TRUE)
          bleKeyboard.write(48);   //send ASCII code for numeric zero to toggle track to "play" mode
          delay(500);
          Serial.println("NEXT SLOT");
        }
        else {    //otherwise if "short press"
          bleKeyboard.write(215);   //send ASCII code for "Right Arrow" key to select next track
          delay(500);
          new_track = 1;    //set flag that we have switched to a new track
          playback = 0;
          Serial.println("NEXT TRACK");
        }
      }
    }
    
    
    //LEFT PEDAL press
    if(digitalRead(pedal_left) == LOW && record == 0) {    //if pedal pressed while NOT recording
      if(new_track == 1) {
        Serial.println();
        Serial.print("LEFT Pressed...");
        bleKeyboard.write(KEY_RETURN);
        delay(1000);
        record = 1;   //set flag that recording is in progress
        playback = 1;   //Set this flag to "true" while recording to prevent looping re-triggers if pedal held down.
        new_track = 0;    //set flag that the current track is no longer "new"
        Serial.println("RECORDING");
      }
      else if(new_track == 0) {
        if(playback == 0) {   //Check "playback" flag to prevent looping re-triggers if pedal is held down after stopping recording.
          Serial.print("LEFT Pressed...");
          bleKeyboard.write(KEY_RETURN);
          delay(1000);
          playback = 1;
          Serial.println("RESET");
        }
      }
    }
    else if(digitalRead(pedal_left) == LOW && record == 1) {   //if pedal pressed (or held down) WHILE recording
      if(playback == 0) {   //By checking the "playback" flag, we can be sure to ONLY end recording if the pedal has been released and re-pressed.
        Serial.print("LEFT Pressed...");
        bleKeyboard.write(KEY_RETURN);
        delay(1000);
        record = 0;   //reset recording flag
        playback = 1;
        Serial.println("PLAYING");
      }
    }
    else if(digitalRead(pedal_left) == HIGH) {   //if pedal released at ANY time
      if(playback == 1) {   //Check if the flag has been set (record mode initiated)
        Serial.println("LEFT Released...");
        playback = 0;   //Reset the "playback" flag when the pedal is released. Now recording can be stopped by a second press of the pedal.
        delay(500);
      }
    }

    
  }
}
