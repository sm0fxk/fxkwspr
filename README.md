# fxkwspr
A WSPR beacon.

Please note that an amateur radio licence is required to transmit the signals this application is generating.

This WSPR beacon is divided into two parts. 
One server running in an Arduino, and a graphical user interface (GUI)running in your PC.
The communication between the GUI and the server takes place over a serial interface using a 
CAT command like protocol.
Once started, the server can run stand alone, provided it is powered from an external source.
A RTC module is strongly recommended if you run the server stand a lone for a long periode of time.

The RF frequency is generated from either the AD9850 DDS module, or the Adafruit Si5351 module.
Which one to use, is selected by a compilation constant when compiling the Arduino sketch.
The output signal from both the AD9850 and the Si5351 is to low for driving the antenna, 
directly so a power amplifier is required.
The Arduino pins 7,8,9, 10 and 11 are used to signal the current band.
This can be used to control a low pass filter, a power amplifier, or switch antennas.

Requirements for compiling the Arduino sketch
---------------------------------------------
The sketch was compiled using the Arduino IDE version 1.8.5. There are
problems reported with older versions.
The following libraries have to be installed using the IDE library manager
- Etherkit Si5351 
- Etherkit JTEncode
- Time 
- Wire 
- DS3232RTC (if clock is enabled)


Requirements for running the GUI
--------------------------------
- Python3

The following Python libraries must be installed:
- Tcl/Tk (tkinter)
- pyserial


Operating the beacon
--------------------
Connect the Arduino to the PC with a USB cable.
Start the GUI.
Command: python wspr_gui.py
First you need to configure the virtual COM port.
Open Edit -> Serial port
Select the COM-port. This may be a guessing game since the port is dynamically generated when the USB cable is connected.
The main window will become red if the communication fails. That is, wrong port configured.
Secondly you will need to configure your call sign and locator etc.
Open Edit -> config
Enter your configuration. The calibration offset shall initially be set to zero.
You can change this later if the crystal on your module deviates from the nominal frequency.
Press 'Apply' and then 'Close'.
In the main window, select band and transmission interval.
Press start.
The WSPR transmission will start on an even minute.
When status says 'on air', you are On Air!

The Arduino sketch have been tested on Arduino UNO and Arduino Nano.
The Python code is developed on Linux (Ubuntu), but is also tested on Windows10.


