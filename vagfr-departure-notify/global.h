#ifndef GLOBAL_H
#define GLOBAL_H


ESP8266WebServer server(80);									// The Webserver
boolean firstStart = true;										// On firststart = true, NTP will try to get a valid time
int AdminTimeOutCounter = 0;									// Counter for Disabling the AdminMode
WiFiUDP UDPNTPClient;											// NTP Client
volatile unsigned long UnixTimestamp = 0;								// GLOBALTIME  ( Will be set by NTP)
boolean Refresh = false; // For Main Loop, to refresh things like GPIO / WS2812
int cNTP_Update = 0;											// Counter for Updating the time via NTP
Ticker tkSecond;												// Second - Timer for Updating Datetime Structure
boolean AdminEnabled = true;		// Enable Admin Mode for a given Time

#define ACCESS_POINT_NAME  "ESP"
//#define ACCESS_POINT_PASSWORD  "12345678"
#define AdminTimeOut 60  // Defines the Time in Seconds, when the Admin-Mode will be diabled

#define MAX_CONNECTIONS 3




//custom declarations
int freq = -1; // signal off

#ifdef ESP_12
//ESP-12E
#define BEEPPIN 4
#define LEFTPIN 16
#define RIGHTPIN 14
#define ADMINPIN 12
#else
//NodeMCU
#define BEEPPIN D1
#define LEFTPIN D5
#define RIGHTPIN D4
#define ADMINPIN D3
#endif

int counter = 0;

#define LOOP_FAST 40 * 1000
#define LOOP_SLOW 120 * 1000
#define BEEPTICKER 100

char serverTransport[] = "176.31.196.113";
//char serverTransport[] = "192.168.2.160";
const int httpPort = 11801;
//const int httpPort = 8080;

String url;
const int intensity[] = {1, 4, 10, 20, 20, 40, 40, 80, 80, 160, 160, 160};
unsigned long waitLoopEntry, loopTime = LOOP_SLOW, waitJSONLoopEntry;
bool okNTPvalue = false;  // NTP signal ok
bool requestOK = false;
int minTillDep = -999, secTillDep, lastMinute;
ledColor ledColor;
boolean ledState = false;
unsigned long  ledCounter;
char str[80];
long departureTime, absoluteActualTime, actualTime;
String JSONline;
long plannedDepartureTimeStamp, lastDepartureTimeStamp, customWatchdog;
bool warn_3 = false;
bool warn_2 = false;
bool warn_1 = false;

String lcdToStation;
int lcdDepartureDelay;

int beepOffTimer, beepOnTimer, beepOffTime, beepOnTime ;

enum defDirection {
  none,
  left,
  right
};

enum defBeeper {
  beeperOn,
  beeperOff,
  beeperIdle
};

volatile defBeeper beeperStatus = beeperIdle;

enum defStatus {
  admin,
  idle,
  requestLeft,
  requestRight,
  recovery
};

defStatus status, lastStatus;


struct strConfig {
  String ssid;
  String password;
  byte  IP[4];
  byte  Netmask[4];
  byte  Gateway[4];
  boolean dhcp;
  String ntpServerName;
  long Update_Time_Via_NTP_Every;
  long timeZone;
  boolean isDayLightSaving;
  String DeviceName;
  byte wayToStation;
  byte warningBegin;
  String base;
  String right;
  String left;
  char product;
} config;

byte currentDirection;
defStatus _lastStatus;



/*
**
** CONFIGURATION HANDLING
**
*/
void ConfigureWifi()
{
  SSD1306   display(0x3c, D3, D4);
  Serial.println("Configuring Wifi");

  WiFi.begin ("WLAN", "password");

  WiFi.begin (config.ssid.c_str(), config.password.c_str());
  int counter = 0;
  while (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected");
    led(red);
    display.setFont(ArialMT_Plain_10);
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.setContrast(255);
    display.clear();
    display.drawString(64, 10, "Connecting to WiFi");
    display.drawXbm(46, 30, 8, 8, counter % 3 == 0 ? ANIMATION_activeSymbole : ANIMATION_inactiveSymbole);
    display.drawXbm(60, 30, 8, 8, counter % 3 == 1 ? ANIMATION_activeSymbole : ANIMATION_inactiveSymbole);
    display.drawXbm(74, 30, 8, 8, counter % 3 == 2 ? ANIMATION_activeSymbole : ANIMATION_inactiveSymbole);
    display.display();
    delay(500);
    counter++;
  }
  if (!config.dhcp)
  {
    WiFi.config(IPAddress(config.IP[0], config.IP[1], config.IP[2], config.IP[3] ),  IPAddress(config.Gateway[0], config.Gateway[1], config.Gateway[2], config.Gateway[3] ) , IPAddress(config.Netmask[0], config.Netmask[1], config.Netmask[2], config.Netmask[3] ));
  }
}


void WriteConfig()
{

  Serial.println("Writing Config");
  EEPROM.write(0, 'C');
  EEPROM.write(1, 'F');
  EEPROM.write(2, 'G');

  EEPROM.write(16, config.dhcp);
  EEPROM.write(17, config.isDayLightSaving);

  EEPROMWritelong(18, config.Update_Time_Via_NTP_Every); // 4 Byte
  EEPROMWritelong(22, config.timeZone); // 4 Byte

  EEPROM.write(32, config.IP[0]);
  EEPROM.write(33, config.IP[1]);
  EEPROM.write(34, config.IP[2]);
  EEPROM.write(35, config.IP[3]);

  EEPROM.write(36, config.Netmask[0]);
  EEPROM.write(37, config.Netmask[1]);
  EEPROM.write(38, config.Netmask[2]);
  EEPROM.write(39, config.Netmask[3]);

  EEPROM.write(40, config.Gateway[0]);
  EEPROM.write(41, config.Gateway[1]);
  EEPROM.write(42, config.Gateway[2]);
  EEPROM.write(43, config.Gateway[3]);

  WriteStringToEEPROM(64, config.ssid);
  WriteStringToEEPROM(96, config.password);
  WriteStringToEEPROM(128, config.ntpServerName);

  // Application Settings
  WriteStringToEEPROM(160, config.base);
  WriteStringToEEPROM(192, config.left);
  WriteStringToEEPROM(224, config.right);
  EEPROM.write(256, config.warningBegin);
  EEPROM.write(257, config.wayToStation);

  WriteStringToEEPROM(258, config.DeviceName);
  EEPROM.write(290, config.product);

  EEPROM.commit();
}
boolean ReadConfig()
{
  Serial.println("Reading Configuration");
  if (EEPROM.read(0) == 'C' && EEPROM.read(1) == 'F'  && EEPROM.read(2) == 'G' )
  {
    Serial.println("Configurarion Found!");
    config.dhcp = 	EEPROM.read(16);

    config.isDayLightSaving = EEPROM.read(17);

    config.Update_Time_Via_NTP_Every = EEPROMReadlong(18); // 4 Byte

    config.timeZone = EEPROMReadlong(22); // 4 Byte

    config.IP[0] = EEPROM.read(32);
    config.IP[1] = EEPROM.read(33);
    config.IP[2] = EEPROM.read(34);
    config.IP[3] = EEPROM.read(35);
    config.Netmask[0] = EEPROM.read(36);
    config.Netmask[1] = EEPROM.read(37);
    config.Netmask[2] = EEPROM.read(38);
    config.Netmask[3] = EEPROM.read(39);
    config.Gateway[0] = EEPROM.read(40);
    config.Gateway[1] = EEPROM.read(41);
    config.Gateway[2] = EEPROM.read(42);
    config.Gateway[3] = EEPROM.read(43);
    config.ssid = ReadStringFromEEPROM(64);
    config.password = ReadStringFromEEPROM(96);
    config.ntpServerName = ReadStringFromEEPROM(128);


    // Application parameters
    config.base = ReadStringFromEEPROM(160);
    config.left = ReadStringFromEEPROM(192);
    config.right = ReadStringFromEEPROM(224);
    config.warningBegin = EEPROM.read(256);
    config.wayToStation = EEPROM.read(257);

    config.DeviceName = ReadStringFromEEPROM(258);
    config.product = EEPROM.read(290);
    return true;

  }
  else
  {
    Serial.println("Configurarion NOT FOUND!!!!");
    return false;
  }
}



#endif
