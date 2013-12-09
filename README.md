Arduino Code for Vernier Photogate Timer
===============

Two versions of arduino code are included here. Both integrate interrupts to 
achieve the most accurate timing with the Arduino. 


Repository Contents
-------------------

* **/VernierPhotogateTimer/* - Arduino example code for testing the library
  This operates three different photogate timer modes: 1-Gate, 2-Pulse, 3-Pendulum
  You can switch the modes by interfacing a menu through the Serial Monitor or by
  pushing a button connected to Pin 12 and GND.
  
  You should also connect a Serial7Segment or OpenSegment 7-Segment LED display for full
  functionality. Connect PWR and GND to your Arduino and connect the RX line to Pin 13
  on the Arduino. These configurations can all be changed in the code.

  This code uses interrupts and is accurate down to the microsecond. The code currently provides
  6 sig figs back to the Serial Monitor. Precision down to 1 uS.
  
  Data is collected and stored into a circular array. However - this array can over-run, if the 
  interrupts occur faster than data is displayed to the screen. You can fix this by increasing the
  baud rate of the Serial routine. Be sure to also change the baud rate for the Serial Monitor
  terminal window to match.


License Information
-------------------

All contents of this repository are released under [Creative Commons Share-alike 3.0](http://creativecommons.org/licenses/by-sa/3.0/).

Authors: Brian Huang @ SparkFun Electonics
