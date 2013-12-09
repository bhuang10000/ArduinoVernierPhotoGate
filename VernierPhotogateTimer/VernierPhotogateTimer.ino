/*
 VernierPhotogateTimmer (v 2013.12.09)
 Monitors a Vernier Photogate connected to BTD connector. 
 
 This sketch lists the time in microseconds since the program started running.
 
 To ensure the greatest accuracy, this code is written using interrupts.
 For more information about using interrupts, see: 
 http://playground.arduino.cc/Code/Interrupts
 
 For more details around using Arduino with Vernier see 
 www.vernier.com/arduino. 
 
 Modified by: B. Huang, SparkFun Electronics
 December 9, 2013
 
 This version incorporates a "circular" buffer of 150 elements and stores all events 
 (blocking and unblocking) of the photogate to an precision of 1 us. In addition, 
 the data is streamed to the Serial buffer and can be captured, copied, exported, 
 and analyzed in your favorite analysis tool -- Graphical Analysis, LoggerPro, Excel,
 Google Sheets, Matlab, etc...
 
 */
#include <SoftwareSerial.h>

#define bufferSize 150  // Sets the size of the circular buffer for storing interrupt data events. Increasing this may cause erratic behavior of the code.
#define DELIM '\t'   // this is the data delimitter. Default setting is a tab character ("\t") 

const int baudRate = 9600;  // Baud rate for serial communications. Increase this for high data rate applications (i.e. smart pulley)

unsigned int refreshRate = 250;  // sets # of milliseconds between refreshes of LED Display

const int buttonPin = 12;   // default buttonPin on Vernier Shield
const int s7segPin = 13;    // re-purposed pin 13 to tie to the Serial 7 Segment
const int photogatePin = 2; // default pin if plugged into Digital 1 on SparkFun Vernier Shield 

/* The following variables are all used and modified by the code. These should not be changed or re-named. */
int mode = 1; // sets the default mode of operation
// Mode 1 -- Gate, Mode 2 -- Pulse, Mode 3 -- Pendulum 

int lastState; 
int currTimeDigits;
unsigned long currTime; 
unsigned int displayIndex; // the current item that has been displayed to the Serial Monitor from the data buffer.
unsigned int count;  // tracks the total # of data entries 

SoftwareSerial s7s1(0, s7segPin);  // sets up Software Serial to interface to Seven Segment Display

char tempString[4];   // String buffer to store for sending to the serial 7 segment display
unsigned long timeRef = 0;  // timeRef for use with refreshRate of the Serial 7 segment display

/* These variables are all accessed and modified by the interrupt handler "PhotogateEvent" 
Variables used by the Interrupt handler must be defined as volatile. */
volatile int photogate = HIGH;
volatile int start = 0;  // 1 == start, 0 == stop
volatile unsigned int numBlocks;
volatile unsigned long startTime;  //Time in us
volatile unsigned long stopTime;  //Time in us
volatile byte dataIndex;
volatile byte displayCount;  // stores the number of items in data Buffer to be displayed
volatile byte state[bufferSize];
volatile unsigned long time_us[bufferSize]; // Time in us

void setup() 
{
  s7s1.begin(9600);

  attachInterrupt(0, photogateEvent, CHANGE); // photogate_event
  pinMode(12, INPUT_PULLUP);

  clearDisplay();
  Serial.begin(9600);           // set up Serial library at 9600 bps

  timeRef = micros();
  displayStartUpMenu();
  clearDisplay();
  delay(10);

  while((Serial.available() == 0) &&  ((micros() - timeRef) < 10E6) && (digitalRead(buttonPin) == HIGH))  // defaults out to mode = 1 after 10 seconds
  {  
    sprintf(tempString, "%4d", (micros() - timeRef)/1000); 
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

  displayHeader();

};// end of setup

void loop ()
{ 
  // check for button press to change modes
  if (digitalRead(12) == LOW)
  {
    mode++;
    resetCount();

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
    while((digitalRead(buttonPin) == LOW)); // hold until button is released
    delay(10); // for de-bouncing
    displayHeader();
  } // if button is pressed

  if ((micros() - timeRef) >= refreshRate*1000) // for refreshing serial 7 segment 
  { 
    timeRef = micros();

    if (start == 1)
    { 
      currTime = (micros() - startTime) / 1000;
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
        currTimeDigits = log10(stopTime - startTime) - 3; // because we're using micros, - 3 is the same as dividing by 1000
        sprintf(tempString, "%4lu", (stopTime - startTime)/1000);

        for(int x = 0; x <  3 - currTimeDigits; x++)  // zero padding 
        {
          tempString[x] = '0';        
        } 
        clearDisplay();
        s7sprint(tempString);
      }
      lastState = 0;  
    }
  } //   if ((millis() - timeRef) >= refreshRate) // for refreshing serial 7 segment 

  uint8_t oldSREG = SREG;
  cli();

  if (displayCount > 0)  // only display to Serial monitor if an interrupt has added data to the data buffer.
  {
    count++;

    Serial.print(count);
    Serial.print(DELIM); //tab character
    Serial.print(state[displayIndex]);
    Serial.print(DELIM); //tab character
    Serial.print(time_us[displayIndex] / 1E6, 6);  // at least 6 sig figs

    Serial.print(DELIM);
    Serial.print((time_us[displayIndex] - startTime) / 1E6, 6);
    Serial.print(DELIM);
    Serial.print(startTime == time_us[displayIndex]);

    Serial.print(DELIM);
    Serial.print(time_us[displayIndex] == stopTime);

    Serial.println();

    displayIndex++;
    if(displayIndex >= bufferSize)
    {
      displayIndex = 0;
    }
    displayCount--; // deduct one
  }
  SREG = oldSREG;
  sei();

} // end of loop

void resetCount()
{
  dataIndex = 0;
  displayIndex = 0;
  count = 0;
  numBlocks = 0;
  start = 0;
  Serial.println();
  Serial.println("*****Reset*****");
  Serial.println();

}

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
  time_us[dataIndex] = micros();

  photogate = digitalRead(photogatePin);
  state[dataIndex] = photogate;

  displayCount++;  // add one to "to be displayed" buffer

  if(photogate == LOW)                                  // trigger only when gate is blocked
  {
    numBlocks++;  
    if (start == 0) // if first event -- start timer

    {    
      startTime = time_us[dataIndex];
      start = 1;    // sets start flag to TRUE  
    }

    else if (mode == 2)  // Pulse Mode
    {
      stopTime = time_us[dataIndex]; 
      start = 0;
    }
    else if ((mode == 3) && ((numBlocks % 3) == 0))     // Pedulum Mode
    {
      stopTime = time_us[dataIndex];
      start = 0;  
    }
  } // if(photogate == LOW)

  else
  {
    if (mode == 1) // gate mode
    {
      stopTime = time_us[dataIndex];
      start = 0;  // reset
    }  
  }

  dataIndex++;
  if(dataIndex >= bufferSize)
  {
    dataIndex = 0;
  }

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
  Serial.println("Vernier Format 2");
  Serial.println();
  Serial.print("Event");
  Serial.print(DELIM);
  Serial.print("Blocked");
  Serial.print(DELIM);
  Serial.print("Time       ");    
  Serial.print(DELIM);
  Serial.print("dTime      ");  
  Serial.print(DELIM);
  Serial.print("Start?");  
  Serial.print(DELIM);
  Serial.print("Stop?");  
  Serial.println();

// Units
  Serial.print("#");
  Serial.print(DELIM);
  Serial.print("(n/a)");
  Serial.print(DELIM);
  Serial.print("(s)         ");    
  Serial.print(DELIM);
  Serial.print("(s)         ");  
  Serial.print(DELIM);
  Serial.print("(n/a)");  
  Serial.print(DELIM);
  Serial.print("(n/a)");  
  Serial.println();
  
  
  Serial.println("---------------------------------------------------------------");  
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
  s7s1.print(toSend.substring(0, 4));

  if (toSend.length() == 1)
    setDecimals(0b000000);  // turn off all decimals
  else
    setDecimals(0b000001 << constrain((toSend.length() - 4), 0, 3));  
  // Turn on decimals based on the length of string -- only displaying 4
  // sig figs.
}






