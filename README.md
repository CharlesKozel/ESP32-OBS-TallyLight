# ESP32 OBS Tally Light
A cheap LED controller which indicated the current OBS streaming status to indicate if you are live or not. Its powered over USB, and can be placed anywhere as long as its on the same WIFI network as the computer running OBS.

There are 3 states:
* Not Connected: First/Last LED are purple
* Streaming: All LEDs green
* Not-Streaming: All LEDs red

Change as needed, aparently this is in inverse of what the industry standard is for tally lights.

![ExampleImage](https://imgur.com/d9Qt8Ro.jpg)

**NOTE:** This is the result of 2 days of work, there are many improvments to the code/instructions that could be made. 

# Requirements / Parts
* OBS with [WebSocket Plugin](https://github.com/Palakis/obs-websocket) installed.
* An ESP32 [Amazon Link](https://www.amazon.com/HiLetgo-ESP-WROOM-32-Development-Microcontroller-Integrated/dp/B0718T232Z)
* WS2812B LEDs (or similar) [Amazon Link](https://www.amazon.com/BTF-LIGHTING-Flexible-Individually-Addressable-Non-waterproof/dp/B07JBYZRWD)
* Wire or breadboard jumper cables and some soldering equipment to wire the ESP32 to the LED strip.
* Whatever you want to use for a case, I used some water cooling hard tubing to hold the LEDs and glued it to the ESP32 and shrink wrapped it all.

# Building/Flashing
This is a PlatformIO project, see [Getting Started with PlatformIO](https://platformio.org/platformio-ide) for setup instructions.

Edit the **constants.h** file to input your user specific configuration like WIFI / OBS address. "OBS_PASS" is the password to the OBS WebSocket plugin, which is set the first time you run the plugin.

After that it should be as simple as opening this project, and using the upload tool in the IDE. 
If you run into issues Google is your friend, PlatformIO and the ESP32 are very popular for Arduino-like projects.

# Wiring Diagram
![Wiring Diagram](https://imgur.com/C9v290g.png)

The code uses pin 13 as the data pin for the LEDs. 