/*
 VernierPhotogate (v 2013.11)
 Monitors a Vernier Photogate connected to BTD connector. 
 
 This sketch captures the time in microseconds. It normalizes the
 data to the first event (interrupt / change of state) for the photogate.

 Modified by: B. Huang, SparkFun Electronics
 Date: December 4, 2013
 
 Updated code to use interrupts. Uses a circular buffer array of 200 elements. 
 This can over-run if a LOT of events occur and the baud rate is slow (9600) 
 for someone spinning a smart pulley, for example. 
 
 If you *really* need to get all of this data, change the baudrate to something
 higher. Just remember to switch this in the Serial Monitor terminal, as well.
 
 See www.vernier.com/arduino for more information.
 
 */
#define bufferSize 300

const int baudRate = 9600;  // Baud rate for serial comm. Change this to 57600 or 115200 if you are seeing
// data buffer over-runs -- this can happen with the pulley wheel spinning VERY 
// fast.
int LEDpin = 13;      // line for LED to turn on when photogate is blocked
int photogatePin = 2; // Pin 2 is default on the SparkFun Vernier Shield on port Digital 1
                      // unfortunately, this code does not support a photogate on Digital 2.

unsigned long time_us[bufferSize]; // Time in us
byte state[bufferSize];

unsigned long timeRef = 0;
unsigned long startTime;

int gate = HIGH; // default state is HIGH
int status = 0;  // 0 = off, 1 = timing

/*-------------------------------------------------------------*/
/* Index Variables for keeping track of the elements in the array
for recording data and displaying data */

int displayIndex = 0;
int dataIndex = 0;
int displayCount = 0;  // counts # of things to be displayed
int count;

/*-------------------------------------------------------------*/


void setup() 
{
  Serial.begin(baudRate);  // set up Serial library

  attachInterrupt(0, photogateEvent, CHANGE); // photogate_event any change

  pinMode(LEDpin, OUTPUT);
  Serial.println("Vernier Format 2");
  Serial.println("Photogate blocked times taken using Ardunio");

  Serial.print("Event");
  Serial.print("\t"); //tab character
  Serial.print ("State"); //change to match sensor
  Serial.print("\t"); //tab character
  //  Serial.print ("Time(raw)"); //change to match sensor
  //  Serial.print("\t"); //tab character
  Serial.println ("Time(offset)"); //change to match sensor

  Serial.print("#");
  Serial.print("\t"); // tab character
  Serial.print("(n\\a)");
  Serial.print("\t"); // tab character
  //  Serial.print("(s)");
  //  Serial.print("\t"); // tab character
  Serial.println("(s)"); 
};// end of setup

void loop ()
{
  if (displayCount > 0)
  {
    count++;

    Serial.print(count);
    Serial.print("\t"); //tab character
    Serial.print(state[displayIndex]);
    //    Serial.print("\t"); //tab character
    //    Serial.print(decimalTimeRaw, 6);  // at least 6 sig figs
    Serial.print("\t"); //tab character
    Serial.println((time_us[displayIndex] - startTime) / 1E6, 6);  // at least 6 sig figs
    displayIndex++;
    if(displayIndex >= bufferSize)
    {
      displayIndex = 0;
    }
    displayCount--; // deduct one
  }
}
;// end of loop

void photogateEvent()
{ 
  gate = digitalRead(photogatePin);
  time_us[dataIndex] = micros();
  state[dataIndex] = gate;

  if (status == 0)
  {    
    startTime = time_us[dataIndex];
    status = 1;
  }

  dataIndex++;
  displayCount++;

  if(dataIndex >= bufferSize)
  {
    dataIndex = 0;
  }

  if(gate == LOW)
    digitalWrite(LEDpin, HIGH);// turn on LED
  else
    digitalWrite(LEDpin, LOW);// turn on LED
}

