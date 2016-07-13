/* Author: WilliamH
   Date: 14-10-2015
   Version: 0.1
   Description: control LED strip lights in locations around the house.  include motion detection along with web interface for manual control.  Slow on and off using PWM
   UNO PWM Pins are 3, 5, 6, 9, 10*, 11* ** Ethernet Shield uses pins 10,11,12,13 & 4 for SD Card
   Future add:  Add LUX meter to control light activationonly when dark.  
                Add  mechanical switch for path and others if required.
                Sort out why interupt control is troublesome
                ******** CLEAN CODE UP ********
*/

#include <Ethernet.h>
#include <SPI.h>


// *****************************
// ******** Setup times ******** 
const long OneSecond = 1000;  // a second is a thousand milliseconds
const long TwoSecond = OneSecond * 2;
const long ThreeSecond = OneSecond * 3;
const long FiveSecond = OneSecond * 5;
const long TenSecond = OneSecond * 10;
const long OneMinute = OneSecond * 60;
const long TwoMinute = OneMinute * 2;
const long ThreeMinute =  OneMinute * 3;
const long FourMinute =  OneMinute * 4;
const long OneHour   = OneMinute * 60;
const long OneDay    = OneHour * 24;


// ***************************************************
// ******** Setup LED strip output pins (PWM) ******** 
int FrontPathLEDPin = 3;
int FrontPorchLEDPin = 5;
int BackPorchLEDPin = 6;
int SPARELEDSTRIPPin = 9;
bool FrontPathLEDManOn = 0; // are the LEDs manually turned on.  do not turn off with motion timout

// **********************************************
// ******** Setup LED start state of off ******** 
bool FrontPathLEDState = digitalRead(FrontPathLEDPin);
bool FrontPorchLEDState = digitalRead(FrontPorchLEDPin);
bool backPorchLEDState = digitalRead(BackPorchLEDPin);
bool SPARELEDSTRIPState = digitalRead(SPARELEDSTRIPPin);

// *****************************************
// ******** Setup Lighting Controls ******** 
int FadeSpeed = 5;

// ****************************************************
// ******** Setup Motion Sensor input controls ******** 
int FrontPathSensor1Pin = 2;   // set the pin for switch on PIR is on - needs to be on an interupt pin
int calibrationTime = 5; //the time we give the sensor to calibrate (10-60 secs according to the datasheet)       
bool MotionDetectOn = 1; //do we want to run detection code.  set via Lux meter at some stage
int FrontPathSensorState = LOW;  
int FrontPathSensorVal = 0; // variable for reading the sensor status
int MotionActivationDelay = TwoSecond;  // How long to wait till next motion is tested for
int MotionLEDOnTime = FiveSecond;  // How long to leave LEDs on after motion has been detected
unsigned long MotionActivationTime = 0;  // store when last motion was detected
unsigned long ledOnTime = 0; //record time when motion activation happened


// **************************************
// ******** Setup Remote Control ******** 
int RemoteButAPin = 19;
int RemoteButBPin = 18;
int RemoteButCPin = 17;
int RemoteButDPin = 16;



// ********************************************
// ******** Setup Network &  Webserver ******** 
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };   //physical mac address
EthernetServer server(80);                             //server port     
String readString;
String PageTitle = "<TITLE>Manage Exterior Lights</TITLE>";
String PageHeader1 = "<H1>Manage Exterior Lights</H1>";
String PageHeader2 = "<H2>Select lights to control below</H2>";


void setup() {

 // Open serial communications and wait for port to open:
  Serial.begin(9600);
  pinMode(FrontPathLEDPin, OUTPUT);
  pinMode(FrontPorchLEDPin, OUTPUT);
  pinMode(BackPorchLEDPin, OUTPUT);
  pinMode(SPARELEDSTRIPPin, OUTPUT);
  pinMode(FrontPathSensor1Pin, INPUT);
//  pinMode(RemoteButAPin, INPUT);
//  pinMode(RemoteButBPin, INPUT);
  pinMode(RemoteButCPin, INPUT);
  pinMode(RemoteButDPin, INPUT);
    //give the sensor some time to calibrate
  Serial.print("calibrating sensor ");
    for(int i = 0; i < calibrationTime; i++){
      Serial.print(".");
      delay(1000);
      }
    Serial.println(" done");
    Serial.println("SENSOR ACTIVE");
    delay(50);

  // start the Ethernet connection and the server:
//  Ethernet.begin(mac, ip, gateway, subnet);
  Ethernet.begin(mac);
  server.begin();
  Serial.print("server is at ");
  Serial.println(Ethernet.localIP());
//  attachInterrupt(digitalPinToInterrupt(MotionSensor1), MovementTriggered, RISING); // Use interupt to execute function - not f***ing working for some reason????
}

void loop() {
// ***************************************************************
// ***************************************************************
// ***************************************************************
  Serial.println("button check"); 
  if (digitalRead(RemoteButAPin) == HIGH){
    if (digitalRead(FrontPathLEDPin) == HIGH) {
      LedOff(FrontPathLEDPin);
    }
    else {
      LedOn(FrontPathLEDPin);
    }
  }
  if (digitalRead(RemoteButBPin) == HIGH){
    if (digitalRead(FrontPorchLEDPin) == HIGH) {
      LedOff(FrontPorchLEDPin);
    }
    else {
      LedOn(FrontPorchLEDPin);
    }
  }  
// ***************************************************************
// ***************************************************************
// ***************************************************************

ButtonCheck();

if (MotionDetectOn == 1) 
{
  MotionSense();
}
 
// Create a client connection
EthernetClient client = server.available();
if (client) 
{
  while (client.connected()) 
  {   
    if (client.available()) 
    {
      char c = client.read();
      //read char by char HTTP request
      if (readString.length() < 100) 
      {
        //store characters to string
        readString += c;
        //Serial.print(c);
      }

       //if HTTP request has ended
       if (c == '\n') 
       {          
         Serial.println(readString); //print to serial monitor for debuging
         client.println F("HTTP/1.1 200 OK"); //send new page
         client.println F("Content-Type: text/html");
         client.println();     
         client.println F("<HTML><link rel=\"shortcut icon\" href=\"http://cartertonauto.co.nz/favicon.ico\"></link>");
         client.println F("<HEAD> <meta name='apple-mobile-web-app-capable' content='yes' /> <meta name='apple-mobile-web-app-status-bar-style' content='black-translucent' />");
         client.println F("<link rel='stylesheet' type='text/css' href='http://cartertonauto.co.nz/lighting.css' />");
         client.println(PageTitle);
         client.println F("</HEAD> <BODY>");
         client.println(PageHeader1);
         client.println F("<hr />");
         client.println F("<br />");  
         client.println(PageHeader2);
         client.println F("<br />");
         client.println F("<a href=\"/?FrontPathOn\"\">Path LED On</a>");
         client.println F("<a href=\"/?FrontPathOff\"\">Path Off LED</a><br />");   
         client.println F("<br />");  
         client.println F("<a href=\"/?FrontPorchOn\"\">Porch On LED</a>");
         client.println F("<a href=\"/?FrontPorchOff\"\">Porch Off LED</a><br />");   
         client.println F("<br />");  
         client.println F("<a href=\"/?BackPorchOn\"\">Back On LED</a>");
         client.println F("<a href=\"/?BackPorchOff\"\">Back Off LED</a><br />");   
         client.println F("<br />");     
         client.println F("<br />"); 
         client.println F("<br />"); 
         client.println F("</BODY> </HTML>");
     
         delay(1);
         //stopping client
         client.stop();
         //controls the Arduino if you press the buttons
         if (readString.indexOf("?FrontPathOn") >0)
         {
           FrontPathLEDManOn = 1;
           LedOn(FrontPathLEDPin);
         }
         if (readString.indexOf("?FrontPathOff") >0)
         {
           FrontPathLEDManOn = 0;
           LedOff(FrontPathLEDPin);
         }
         if (readString.indexOf("?FrontPorchOn") >0)
         {
           LedOn(FrontPorchLEDPin);
         }
         if (readString.indexOf("?FrontPorchOff") >0)
         {
           LedOff(FrontPorchLEDPin);
         }
         if (readString.indexOf("?BackPorchOn") >0)
         {
           LedOn(BackPorchLEDPin);
         }
         if (readString.indexOf("?BackPorchOff") >0)
         {
           LedOff(BackPorchLEDPin);
         }
         //clearing string for next read
         readString="";  
         }
       }
    }
  }
}

void MotionSense() {
  if (FrontPathLEDManOn == 0) {
    FrontPathSensorState = digitalRead(FrontPathSensor1Pin);  // read input value
    if (FrontPathSensorState == HIGH) {
      MotionActivationTime = millis(); 
      if (MotionLEDOnTime +MotionActivationTime > millis()) {
        Serial.println("LED Stays on or goes on");
        LedOn(FrontPathLEDPin);
      }
    } 
  if (FrontPathSensorState == LOW) {
      if (MotionActivationTime + MotionLEDOnTime < millis()) {   
        LedOff(FrontPathLEDPin);
      }
    }
  }
}

void ButtonCheck() {
  Serial.println("button check"); 
  if (digitalRead(RemoteButAPin) == HIGH){
    if (digitalRead(FrontPathLEDPin) == HIGH) {
      LedOff(FrontPathLEDPin);
    }
    else {
      LedOn(FrontPathLEDPin);
    }
  }
  if (digitalRead(RemoteButBPin) == HIGH){
    if (digitalRead(FrontPorchLEDPin) == HIGH) {
      LedOff(FrontPorchLEDPin);
    }
    else {
      LedOn(FrontPorchLEDPin);
    }
  }  
}



// *************************************************
// ******** Common LED functions On and Off ******** 
void LedOn(int WhichLed) {
  if (digitalRead(WhichLed) == 0){  //check is LED is actually off
    ledOnTime = millis();
    // fade in from min to max in increments of 5 points:
    for (int fadeValue = 0 ; fadeValue <= 255; fadeValue += 5) {
      // sets the value (range from 0 to 255):
      analogWrite(WhichLed, fadeValue);
      // wait for 30 milliseconds to see the dimming effect
      delay(30);
    }
  }
}

void LedOff(int WhichLed) {
  //check is LED is actually on
  if (digitalRead(WhichLed) > 0) {
      Serial.println(digitalRead(WhichLed));
    // fade out from max to min in increments of 5 points:
    for (int fadeValue = 255 ; fadeValue >= 0; fadeValue -= 5) {
      // sets the value (range from 0 to 255):
      analogWrite(WhichLed, fadeValue);
      // wait for 30 milliseconds to see the dimming effect
      delay(30);
    }
  }
}
 
