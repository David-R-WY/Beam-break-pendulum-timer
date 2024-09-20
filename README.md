# Beam-break-pendulum-timer

This sketch runs on an Arduino Uno R3 and will:
1) use a TT Electronics OPB901W55Z beam break photo sensor from Digikey to detect the swings of a pendulum
2) read environmental data from a MS8607 pressure, humidity & temperature sensor
3) output the data to the screen and also log it to an SD card on an ADAfruit data logging shield
4) use the PCF8523 real time clock on the data logging shield (which is more accurate than the crystal oscillator on the Arduino)

Data is recorded in user configurable time chunks (5 minutes of 1 second swings (= 2 seconds period) = 150 double swings).
Recorded data can then be imported into a spreadsheet for viewing, graphing and further analysis
  
