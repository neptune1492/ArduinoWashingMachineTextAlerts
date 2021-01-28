//ReadingsWithWindow
//Elizabeth Brennan

// Libraries
#include <Adafruit_CC3000.h>
#include <ccspi.h>
#include <SPI.h>
#include <string.h>
#include "utility/debug.h"
#include<stdlib.h>

// Define CC3000 chip pins
#define ADAFRUIT_CC3000_IRQ 3
#define ADAFRUIT_CC3000_VBAT 5
#define ADAFRUIT_CC3000_CS 10

// Create CC3000 instances
Adafruit_CC3000 cc3000 = Adafruit_CC3000(ADAFRUIT_CC3000_CS, ADAFRUIT_CC3000_IRQ, ADAFRUIT_CC3000_VBAT,
                                         SPI_CLOCK_DIV2); // you can change this clock speed
                                
// WLAN parameters
#define WLAN_SSID "fbutemp"
#define WLAN_PASS "fbugriffin"
// Security can be WLAN_SEC_UNSEC, WLAN_SEC_WEP, WLAN_SEC_WPA or WLAN_SEC_WPA2
#define WLAN_SECURITY WLAN_SEC_WPA2

// Carriots parameters
#define WEBSITE "api.carriots.com"
#define API_KEY "42c3132cb37de19a8cdd505e431e411a4b8fc1fda02866fdd9580a853e412a68"
#define DEVICE "LaundryBug@LaundryBugDevice.LaundryBugDevice"

uint32_t ip;

//This program uses a ten-second window reading avg taken at rest
//and looks at the washing machine functioning in ten-seconds segments, 
//and compares the avg readings of these segments to the resting avg

//Declare pins for x, y, and z data
const int xPin = A0;                  // x-axis of the accelerometer
const int yPin = A1;                  // y-axis
const int zPin = A2;                  // z-axis
const int led = 13;                   //LED that tells user they can start machine

long readingSum = 0;                   //Variable used to hold the readings' sum each time
long totalSum = 0;                    //variable for the sum of the sums

double restAvg = 0;                    //Used at initialization
double activityAvg = 0;              //Used with moving window
int incidentCount = 0;

double count = 0;
double windowSize = 0;                    //Used with count
boolean winEnd = false;                  //variable for marking end of window-checking section
boolean progEnd = false;  //variable for marking end of washing machine activity.  Used b/c arduino loops again and again.
double percentage = 0.00015;          //Number used to mark threshhold for rest average variance
//double percentage = 0.0001;
//double percentage = 0.00013;
//double percentage = 0.00005;
double startTime = 1.00;        //Used to keep track of how much time has actually passed when figuring out 10 seconds at the beginning

void setup()
{
  // initialize serial monitor:
  Serial.begin(9600); //9600 is the bits-per-second transmission rate that the serial monitor must be set to

  //Since the AREF pin is being used with the 3.3V pin, the voltage 
  //readings taken by the analog-to-digital converter must come from the AREF pin.  Set this:
  analogReference(EXTERNAL);

//Marks pins as data inputs
  pinMode(xPin, INPUT);
  pinMode(yPin, INPUT);
  pinMode(zPin, INPUT);
  
  pinMode(led, OUTPUT);    //Marks pin for led as Output

  /*set up wi-fi card*/
  Serial.println(F("\nInitializing..."));
  if (!cc3000.begin())
  {
    Serial.println(F("Couldn't begin()! Check your wiring?"));
    while(1);
  }
 
}         //End of set-up

void loop(){
  while (progEnd != true) {
    
    if (startTime == 1) {
      
      startTime = millis();  //Take the time the program's been running at RIGHT NOW.  Also, resets the starttime variable, so that this initialization won't execute again.
      
    //Initialization Phase:
      while (((millis()/1000) - (startTime/1000)) < 21) {           //Ten seconds of recording data
        readingSum = (analogRead(xPin) + analogRead(yPin) + analogRead(zPin));    //Sum up the readings from x, y, and z
        totalSum = totalSum + readingSum;    //Aggregate the reading sums
        count = count + 1;
      }
        restAvg = totalSum/count;    //Calculate the average of the readings taken in 10 seconds
        windowSize = count;        //Mark how many readings can be taken in 10 seconds 
        Serial.print("End of initialization");
    }
     //This part should be repeated every time after initialization
      while (winEnd != true) {
      
        Serial.print("Running.....");
        count = 0;    //Reset stuff for future use
        totalSum = 0;
        readingSum = 0;
        
        while (count < (windowSize)) {
          readingSum = (analogRead(xPin) + analogRead(yPin) + analogRead(zPin));    //Sum up the readings from x, y, and z
          totalSum = totalSum + readingSum;    //Aggregate the reading sums
          count = count + 1;    
        }
        activityAvg = totalSum/windowSize;     //When 10-seconds' worth of data has been taken, find the average
        Serial.print("Rest: ");
        Serial.print(restAvg);
        Serial.print("Activity: ");
        Serial.print(activityAvg);
          if ((activityAvg <= (restAvg + percentage*restAvg)) && (activityAvg >= (restAvg - percentage*restAvg))) {
            Serial.println("Checking to see if the machine is truly at rest");
            startTime = millis(); //Grab the current time
            while (((millis()/1000) - (startTime/1000)) <= 61) {
              readingSum = (analogRead(xPin) + analogRead(yPin) + analogRead(zPin));    //Sum up the readings from x, y, and z
              if ((readingSum > (restAvg + percentage*restAvg)) && (readingSum < (restAvg - percentage*restAvg))) {
                incidentCount = incidentCount + 1;
                Serial.println(incidentCount);
                Serial.print("incident(s) recorded.");
              }
            }
            if (incidentCount = 0){ //Only counts as off if there's been at least one "incident" outside the "Rest" threshold within a minute after things died down.
            winEnd = true;
            
            //Send message w/Cell phone
            // Connect to WiFi network
            cc3000.connectToAP(WLAN_SSID, WLAN_PASS, WLAN_SECURITY);
            Serial.println(F("Connected!"));
  
            /* Wait for DHCP to complete */
            Serial.println(F("Request DHCP"));
            
            while (!cc3000.checkDHCP())
              {
                delay(100);
              }
 
            // Get the website IP & print it
            ip = 0;
            Serial.print(WEBSITE); Serial.print(F(" -> "));
            while (ip == 0) {
              if (! cc3000.getHostByName(WEBSITE, &ip)) {
                Serial.println(F("Couldn't resolve!"));
              }
              delay(500);
            }
            cc3000.printIPdotsRev(ip);
  
 
            // Prepare JSON for Carriots & get length
            int length = 0;
            int state = 1;
            String data = "{\"protocol\":\"v2\",\"device\":\""+String(DEVICE)+"\",\"at\":\"now\",\"data\":{\"Status\":"+String(state)+"}}";
  
            length = data.length();
            Serial.print("Data length");
            Serial.println(length);
            Serial.println();
  
            // Print request for debug purposes
            Serial.println("POST /streams HTTP/1.1");
            Serial.println("Host: api.carriots.com");
            Serial.println("Accept: application/json");
            Serial.println("User-Agent: Arduino-Carriots");
            Serial.println("Content-Type: application/json");
            Serial.println("carriots.apikey: " + String(API_KEY));
            Serial.println("Content-Length: " + String(length));
            Serial.print("Connection: close");
            Serial.println();
            Serial.println(data);
  
            // Send request
            Adafruit_CC3000_Client client = cc3000.connectTCP(ip, 80);
            
            if (client.connected()) {
              Serial.println("Connected!");
              client.println("POST /streams HTTP/1.1");
              client.println("Host: api.carriots.com");
              client.println("Accept: application/json");
              client.println("User-Agent: Arduino-Carriots");
              client.println("Content-Type: application/json");
              client.println("carriots.apikey: " + String(API_KEY));
              client.println("Content-Length: " + String(length));
              client.println("Connection: close");
              client.println();
    
              client.println(data);
    
            }
            else {
              Serial.println(F("Connection failed"));
              return;
            }
  
            Serial.println(F("-------------------------------------"));
            while (client.connected()) {
            while (client.available()) {
                char c = client.read();
                 Serial.print(c);
              }
            }
            client.close();
            Serial.println(F("-------------------------------------"));
        
            Serial.println(F("\n\nDisconnecting"));
            cc3000.disconnect();
  
            // Wait 10 seconds until next update
            delay(10000);
   

          }
      }

      progEnd = true;      
  }  //End of the "while (WinEnd <> true)" loop
  Serial.print("End");
  digitalWrite(led, HIGH);
} //While progEnd <> true
}//End of loop()
