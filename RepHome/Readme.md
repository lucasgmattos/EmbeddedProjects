Project created on STM32F103 for learning purposes.

The idea of this implementation is to have a display that shows, at home, messages sent by anyone through a Webpage (managed by an internal webserver). It has a buffer of 4 messages of up to 65 characters. The Message screens also display the current date and time synchronized through a NTP server (GMT-2 time).

Currently has the following implemented features:
- Monochromatic Display (SSD1306 128x64px);
- WiFi connection via ESP8266, communicating through USART;
- Interface push-button for screen switch;

Future functionalities (TO DO):
- beep when new message arrives;
- temperature sensor to show home temperature and report it to the web (DHT11 sensor);
- noise detector with over-limit alert;
