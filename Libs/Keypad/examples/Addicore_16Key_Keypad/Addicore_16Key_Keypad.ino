/*
|| Example sketch for the Addicore 4x3 matrix array 12-key membrane switch keypad,
|| found at: https://www.addicore.com/product-p/142.htm
|| After connecting your keypad to your Arduino (as detailed below) and uploading
|| this sketch to your Arduino, you can then open the Serial Monitor on your
|| computer and ensure that the baud is set to 9600. Then press any of the buttons
|| on the keypad to see the pressed button show up on the Serial Monitor.
||
|| Wire the keypad to your Arduino using information below:
|| Note: Pins one and seven on the keypad are labeled with little embossed plastic
|| numbers on the black connector
|| Wiring:
|| Keypad ---> Arduino
|| 1 3
|| 2 4
|| 3 5
|| 4 6
|| 5 7
|| 6 8
|| 7 9
||
|| Modified by Craig Thompson (craig@addicore.com), December 15, 2014
|| Based on CustomKeypad.pde by Alexander Brevig (alexanderbrevig@gmail.com)
*/
#include <Keypad.h>
const byte ROWS = 4; //four rows
const byte COLS = 4; //four columns
//define the cymbols on the buttons of the keypads
char hexaKeys[ROWS][COLS] = {
 {'1','2','3','A'},
 {'4','5','6','B'},
 {'7','8','9','C'},
 {'*','0','#','D'}
};
byte rowPins[ROWS] = {10, 9, 8, 7}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {6, 5, 4, 3}; //connect to the column pinouts of the keypad
//initialize an instance of class NewKeypad
Keypad customKeypad = Keypad( makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS);
void setup(){
 Serial.begin(9600);
}

void loop(){
 char customKey = customKeypad.getKey();

 if (customKey){
 Serial.println(customKey);
 }
}

