

/*

server for TECHIN514 Team Finder Exercise using Arduino toolchain

in case anyone is curious what is running behind the curtain

not the cleanest or most efficient implementation, 
just something quick that should work for class.  prototyping style!

first version was written in micropython, but was having http reliability issues. 
this one really makes you appreciate python for its string handling, lists, and dictionaries

pull requests welcome if anyone wants to do a better job with this.

works with an I2C OLED common on ESP32 boards

provided under MIT license

*/

#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiAP.h>


#define USE_LCD true

#ifdef USE_LCD
#include <Wire.h>
#include "SSD1306Wire.h"
SSD1306Wire display(0x3c, 5, 4);  // ADDRESS, SDA, SCL
#endif

const char* WIFI_SSID = "TEAM_FINDER_AP";
const char* WIFI_PASSWORD = "teamwork-makes-the-dream-work";

#define CLASS_SIZE 56
#define TEAMS 14

struct name_and_team {
  String name;
  String id;
  int sequenceNumber;
};

name_and_team teamList[CLASS_SIZE];

WiFiServer server(80);

void setup()
{

  Serial.begin(115200);
  Serial.printf("TECHIN514 Team Selection Server\n");

  Serial.printf("Initializing team list...\n");
  for (int i=0; i < CLASS_SIZE; i++)
  {
    teamList[i].name = "";
    teamList[i].id = "";
    teamList[i].sequenceNumber = -1;
  }

  Serial.printf("Configuring access point %s ...\n", WIFI_SSID);
  WiFi.softAP (WIFI_SSID, WIFI_PASSWORD);
  Serial.printf("server IP will be at : %s\n", WiFi.softAPIP().toString().c_str());

  server.begin();
  Serial.printf("server is going online!\n");

#ifdef USE_LCD
  display.init();
  display.flipScreenVertically();
  display.clear();
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.drawString(0,0,"TEAM FINDER ONLINE AT");
  display.drawString(0,20, WiFi.softAPIP().toString());
  display.drawString(0,30, "SSID");
  display.drawString(30,30, WIFI_SSID);
  display.drawString(0,40, "PWD");
  display.drawString(30,40, WIFI_PASSWORD);
  display.display();
#endif

}

int teamNumber(int sequenceNumber)
{
  return (sequenceNumber % TEAMS) + 1;
}

void debugListTeams()
{

  Serial.printf("Team Directory:\n");
  Serial.printf("NAME / ID / TEAM\n");
  for (int i=0; i<CLASS_SIZE; i++)
  {
   // Serial.printf("%s %s %d:2\n", teamList[i].name.c_str(), teamList[i].id.c_str(), teamNumber(teamList[i].sequenceNumber) );    
    
    if (teamList[i].sequenceNumber == -1)
    {
      Serial.printf("[end of team list at position %d]", i);
      break;
    }

    Serial.printf("%s / %s / %d\n", teamList[i].name.c_str(), teamList[i].id.c_str(), teamNumber(teamList[i].sequenceNumber) );    
      
  }

}

String teamSelector(String urlRequest)
{

  int idIndex = urlRequest.indexOf("?id=");
  int nameStartIndex = urlRequest.indexOf("&name=");
  int nameEndIndex = urlRequest.indexOf(" HTTP");

  Serial.print("Team selector called with : ");
  Serial.println(urlRequest);

  Serial.printf("id at %d / name at %d\n", idIndex, nameStartIndex);
  
  // TODO : verify that indexof returns -1 for not found. 
  if ((idIndex == -1) || (nameStartIndex == -1)) 
  {
    Serial.println("ERROR : MALFORMED REQUEST!");
    return "ERROR : MALFORMED REQUEST";
  }
  else
  {
    idIndex += 4;
    nameStartIndex += 6;
  }

  String id = urlRequest.substring(idIndex, nameStartIndex-6);
  String name = urlRequest.substring(nameStartIndex, nameEndIndex);
  int newStudent = -1;
  int nextSequenceNumber = -1;

  // first check if the team is already assigned otherwise find the next open slot in the directory
  for (int i=0; i < CLASS_SIZE; i++)
  {
    if (teamList[i].sequenceNumber == -1)
    {
      newStudent = i;
      break;
    }

    if (teamList[i].id == id)
    {
      Serial.printf("student already found in database %s %s %s\n",id.c_str(), teamList[i].id.c_str(), teamList[i].name.c_str());
      return String( teamNumber(teamList[i].sequenceNumber) ) ;
    }
  }

  Serial.printf("adding new student at slot %d\n", newStudent);

  if (newStudent == -1) 
  {
    return "INTERNAL ERROR. CANNOT FIND OPEN SLOT IN TEAM LIST";
  }

  // find an unused team number via an annoying bit of brute force.  clearly there are better ways.
  int i;
  while (true)
  {
    nextSequenceNumber = random(CLASS_SIZE);
    for (i=0; i < CLASS_SIZE; i++)
    {
      if (teamList[i].sequenceNumber == nextSequenceNumber)
      {
        break;
      }
    }
    if (i == CLASS_SIZE)
    {
      break;
    } 
  }

  Serial.printf("Found unused sequence number : %d\n", nextSequenceNumber);

  teamList[newStudent].name = name;
  teamList[newStudent].id = id;
  teamList[newStudent].sequenceNumber = nextSequenceNumber;

  return String(teamNumber(nextSequenceNumber));
}

void loop()
{

  WiFiClient client = server.available();

  if (client) {
    Serial.println("New Client.");          // print a message out the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    String requestLine = "";                // buffer to hold the request URL
    while (client.connected()) {            // loop while the client's connected
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        if (c == '\n') {                    // if the byte is a newline character

          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) 
          {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println();

            // the content of the HTTP response follows the header:
            client.println(teamSelector(requestLine));

            // The HTTP response ends with another blank line:
            client.println();
            // break out of the while loop:
            break;
          } 
          else
          {    // if you got a newline, then clear currentLine:
            if (currentLine.startsWith("GET"))
            {
              requestLine = currentLine;
            }
            currentLine = "";
          }
        } 
        else if (c != '\r') 
        {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
      } 
    }

    client.stop();
    Serial.printf("client disconnected.\n");
    debugListTeams();
  }


}