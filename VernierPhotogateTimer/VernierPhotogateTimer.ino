/*
 VernierPhotogateTimmer (v 2013.11)
 Monitors a Vernier Photogate connected to BTD connector. 
 
 This sketch lists the time in microseconds since the program started running.
 
 To ensure the greatest accuracy, this code is written using interrupts.
 For more information about using interrupts, see: 
 http://playground.arduino.cc/Code/Interrupts
 
 For more details around using Arduino with Vernier see 
 www.vernier.com/arduino. 

 Modified by: B. Huang, SparkFun Electronics
 December 4, 2013
  
*/

#include <SoftwareSerial.h>

int refreshRate = 5;  // sets # of milliseconds between refreshes of LED Display
int mode = 1; // sets the default mode of operation
// Mode 1 -- Gate, Mode 2 -- Pulse, Mode 3 -- Pendulum 

const int buttonPin = 12;   // default buttonPin on Vernier Shield
const int s7segPin = 13;    // re-purposed pin 13 to tie to the Serial 7 Segment
const int photogatePin = 2; // default pin if plugged into Digital 1 on SparkFun Vernier Shield 

volatile int photogate = HIGH;
volatile int state;  // 1 == start, 0 == stop
int lastState; 
int currTimeDigits;
unsigned long currTime;

SoftwareSerial s7s1(0, s7segPin);  // sets up Software Serial to interface to Seven Segment Display

char tempString[4];
unsigned long timeRef = 0;
int numBlocks;
int numInterrupts = 0;

volatile unsigned long startTime;  //Time in ms
volatile unsigned long stopTime;  //Time in ms

void setup() 
{
  s7s1.begin(9600);

  attachInterrupt(0, photogateEvent, CHANGE); // photogate_event
  pinMode(12, INPUT_PULLUP);

  clearDisplay();
  Serial.begin(9600);           // set up Serial library at 9600 bps

  if(Serial)
  {
    timeRef = millis();
    displayStartUpMenu();
    clearDisplay();
    delay(10);

    while((Serial.available() == 0) &&  ((millis() - timeRef) < 5000) && (digitalRead(buttonPin) == HIGH))  // defaults out to mode = 1 after 5 seconds
    {  
      sprintf(tempString, "%4d", (millis() - timeRef));     
      s7sprint(tempString);
    }; // hold and wait until data is available.

    int userInput = Serial.parseInt();
    Serial.println();

    if ( (userInput >= 1) || (userInput <= 3))
      mode = userInput;
    else
      mode = 1;
    Serial.print("Starting up... ");

    clearDisplay();
    switch (mode)
    {
    case 1:
      Serial.println("Gate Mode"); 
      s7s1.print("Gate");
      break;
    case 2:
      Serial.println("Pulse Mode"); 
      s7s1.print("Puls");
      break;
    case 3:
      Serial.println("Pendulum Mode"); 
      s7s1.print("Pend");
      break;
    default:
      Serial.println("Gate Mode*"); 
      s7s1.print("Gate");
      mode = 1;
      break;
    } // end switch mode
  } // end if Serial
  else
  {
    mode = 1; 
  }
    displayHeader();

};// end of setup

void loop ()
{ 
  if (digitalRead(12) == LOW)
  {
    mode++;
    numBlocks = 0;
    numInterrupts = 0;
    state = 0;
    if (mode > 3) mode = 1;
    clearDisplay();
    switch (mode)
    {
    case 1:
      Serial.println("Gate Mode"); 
      s7s1.print("Gate");
      break;
    case 2:
      Serial.println("Pulse Mode"); 
      s7s1.print("Puls");
      break;
    case 3:
      Serial.println("Pendulum Mode"); 
      s7s1.print("Pend");
      break;
    }    

    while((digitalRead(buttonPin) == LOW));
    delay(50);
    displayHeader();
  } // if button is pressed
  
  startTimer();
} // end of loop


/*************************************************
 * photogateEvent()
 * 
 * Interrupt service routine. Handles capturing 
 * the time and saving this to memory when the 
 * photogate issues an interrupt on pin 2.
 * 
 * As it is currently written, the photogate 
 * will only work on Digital Port 1.
 *************************************************/
void photogateEvent()
{ 
  numInterrupts++;
  photogate = digitalRead(photogatePin);
  if (photogate == LOW)  // blocked -- start timer
  {
    numBlocks++;  
    if (state == 0)
    {
      startTime = millis();
      state = 1; // start timer    
    }
    else if (mode == 2)  // 
    {
      stopTime = millis();
      state = 0;
    }
    else if ((mode == 3) && ((numBlocks % 3) == 0)) // 
    {
      stopTime = millis();
      state = 0;  
    }
  }

  else
  {
    if (mode == 1) // gate mode
    {
      stopTime = millis();
      state = 0;
    }  
  }

  Serial.print(numInterrupts);
  Serial.print("\t");
  Serial.print(numBlocks);
  Serial.print("\t");
  Serial.print(photogate);
  Serial.print("\t");
  Serial.print(millis());
  Serial.print("\t");
  Serial.println(millis() - startTime);
}

/*************************************************
 * displayHeader()
 *
 * Presents the data header to Serial Monitor
 * This data is tab delimitted and can be copied 
 * and pasted directly into Excel or spreadsheet
 * / graphing program. 
 *************************************************/
void displayHeader()
{
  Serial.print("Event");
  Serial.print("\t");
  Serial.print("Blocks");
  Serial.print("\t");
  Serial.print("Gate");
  Serial.print("\t");
  Serial.print("Time");  
  Serial.print("\t");
  Serial.println("dTime");  
  Serial.println("-------------------------------------");  
}

void displayStartUpMenu()
{
     Serial.println("Vernier Photogate Timer");
    Serial.println("");
    Serial.println("Connect the RX pin of any SparkFun Serial 7 Segment");
    Serial.println("LED Display to pin 12.");
    Serial.println("");
    Serial.println("The display will show the time elapsed for the photogate.");

    Serial.println("Select the operating mode: ");
    Serial.println("1) Gate mode");  
    // Gate timing begins when the photogate is first blocked. The timing will continue until the gate is unblocked.
    Serial.println("2) Pulse mode");
    // Pulse timing begins when the photogate is first blocked. The timing will continue until the gate is blocked again.
    Serial.println("3) Pendulum mode");
 
}

/*************************************************
 * setDecimals(byte decimals)
 * 
 * Sets the decimal LEDs on the 7 segment display
 *
 * https://github.com/sparkfun/Serial7SegmentDisplay/wiki/Special-Commands#wiki-decimal
 *************************************************/
void setDecimals(byte decimals)
{
  s7s1.write(0x77);
  s7s1.write(decimals);
}

/*************************************************
 * clearDisplay()
 * 
 * Clears the serial7segment display
 *************************************************/
void clearDisplay()
{
  setDecimals(0b0);
  s7s1.write(0x76); // clear & reset 
}

/*************************************************
 * s7sprint(String toSend)
 * 
 * Handles data to the serial seven segment display
 * Places decimals in correct places and only shows
 * 4 sig figs of data.
 **************************************************/
void s7sprint(String toSend)
{
  //  Serial.print(toSend);
  //  Serial.print("\t");
  //  Serial.println(toSend.length());
  s7s1.print(toSend.substring(0, 4));

  if (toSend.length() == 1)
    setDecimals(0b000000);  // turn off all decimals
  else
    setDecimals(0b000001 << constrain((toSend.length() - 4), 0, 3));  
  // Turn on decimals based on the length of string -- only displaying 4
  // sig figs.
}

/*************************************************
 * This is the entire timer routine to display to
 * the the seven segment displays. It refreshes only 
 * as often as the refreshRate. 
 * 
 * state = 1 --> start timer
 * state = 0 --> stop timer
 **************************************************/
void startTimer()
{
  if ((millis() - timeRef) >= refreshRate)
  { 
    timeRef = millis();
    if (state == 1)
    { 
      currTime = millis() - startTime;
      currTimeDigits = log10(currTime);
      sprintf(tempString, "%4lu", currTime);     

      for(int x = 0; x <  3 - currTimeDigits; x++)  // zero padding
      {
        tempString[x] = '0';        
      } 

      s7sprint(tempString);
      lastState = 1;  
    }
    else // if (state == 1)
    { 
      if (lastState == 1)
      {
        currTimeDigits = log10(stopTime - startTime);
        sprintf(tempString, "%4lu", (stopTime - startTime));

        for(int x = 0; x <  3 - currTimeDigits; x++)  // zero padding 
        {
          tempString[x] = '0';        
        } 
        clearDisplay();
        s7sprint(tempString);
      }
      lastState = 0;  
    }
  }
}



































