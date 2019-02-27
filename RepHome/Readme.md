Project created on STM32F103 for learning purposes.

The idea of this implementation is to have a display that shows, at home, messages sent by anyone through a Webpage (managed by an internal webserver). It has a buffer of 4 messages of up to 65 characters. The Message screens also display the current date and time synchronized through a NTP server (GMT-2 time).

Currently has the following implemented features:
- Monochromatic Display (SSD1306 128x64px) 4Hz update rate;
- WiFi connection via ESP8266, communicating through USART;
- Interface push-button for screen switch;
- Auto message screen cycling (15s)
- Auto network health check (5min)
- Auto NTP clock refresh (1h)

Kmown bugs:
- Intermitent fail on public ip fetch from ipify
- Intermitent out of bounds values for date and time (create assertion and race condition prevention measures) 

Future functionalities (TO DO):
- beep when new message arrives;
- temperature sensor to show home temperature and report it to the web (DHT11 sensor);
- noise detector with over-limit alert;

IMPORTANT: create your own wifipass.h header inside Includes/Utilities folder with your WiFi SSID and PASSWORD.
example:

#define SSID "my_home_ssid"
#define PASSWORD "my_wifi_password"

Demo video: https://youtu.be/zcq8XZtVCGI
