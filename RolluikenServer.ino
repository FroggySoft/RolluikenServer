/*
 *  This sketch switches two pair of relais:
 *  
 *    http://server_ip/1/up or /2/up will set power and up relais for 30 seconds
 *    http://server_ip/1/down or /2/down will only set power relais on for 30 seconds
 *    
 *    NodeMCU 1.0 (ESP-12E module)
 */

#include <ESP8266WiFi.h>

#define ESP8266_LED   2
#define POWER_RELAIS0 15  // D8 
#define UP_RELAIS0    13  // D7
#define POWER_RELAIS1 12  // D6
#define UP_RELAIS1    14  // D5

#define RELAIS_OFF 1
#define RELAIS_ON  0

#define NOT_MOVING   0
#define MOVING_DOWN  1
#define MOVING_UP    2

const char* ssid = <enter your ssid>;
const char* password = <enter your password>;
IPAddress ip(192,168,1,113);  //Node static IP
IPAddress gateway(192,168,1,1);
IPAddress subnet(255,255,255,0);

// Create an instance of the server
// specify the port to listen on as an argument
WiFiServer server(80);

int mMovingTime0 = 0;
bool mMoving0 = false;
int mDirection0 = 0;
int mMovingTime1 = 0;
bool mMoving1 = false;
int mDirection1 = 0;

void setup()
{
  Serial.begin(115200);
  delay(10);

  // prepare GPIO's
  pinMode(POWER_RELAIS0, OUTPUT);
  digitalWrite(POWER_RELAIS0, RELAIS_OFF);
  pinMode(UP_RELAIS0, OUTPUT);
  digitalWrite(UP_RELAIS0, RELAIS_OFF);
  pinMode(POWER_RELAIS1, OUTPUT);
  digitalWrite(POWER_RELAIS1, RELAIS_OFF);
  pinMode(UP_RELAIS1, OUTPUT);
  digitalWrite(UP_RELAIS1, RELAIS_OFF);
  pinMode(ESP8266_LED, OUTPUT);
  digitalWrite(ESP8266_LED, HIGH); // = led off
  
  // Connect to WiFi network
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  WiFi.config(ip, gateway, subnet);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  
  // Start the server
  server.begin();
  Serial.println("Server started");

  // Print the IP address
  Serial.println(WiFi.localIP());
}

void loop()
{ 
  if(mMoving0)
  {
    digitalWrite(ESP8266_LED,(mMovingTime0%2)==1);
    delay( 500 );
  }
  else if(mMoving1)
  {
    digitalWrite(ESP8266_LED,(mMovingTime1%2)==1);
    delay( 500 );
  }
  else
  {
    digitalWrite(ESP8266_LED, HIGH);
  }
  
  if(mMoving0)
  {
    mMovingTime0++;
    if(mMovingTime0>=60)
    {
      digitalWrite(POWER_RELAIS0, RELAIS_OFF);
      digitalWrite(UP_RELAIS0, RELAIS_OFF);
      mMoving0 = false;
    }
  }
  if(mMoving1)
  {
    mMovingTime1++;
    if(mMovingTime1>=60)
    {
      digitalWrite(POWER_RELAIS1, RELAIS_OFF);
      digitalWrite(UP_RELAIS1, RELAIS_OFF);
      mMoving1 = false;
    }
  }

    // Check if a client has connected
    WiFiClient client = server.available();
    if (!client)
    {
      return;
    }

    // Wait until the client sends some data
    int counter=0;
    while(!client.available() && counter<100)
    {
      delay(1);
      if(mMoving0 || mMoving1)  counter++;  // don't wait to long
    }
    
    // Read the first line of the request
    String req = client.readStringUntil('\r');
    client.flush();

    Serial.println(req);

    // Match the request
    bool lValidUrl = false;
    int lStart0 = NOT_MOVING;
    int lStart1 = NOT_MOVING;
    String lReply = "HTTP/1.1 200 OK\r\nContent-type: application/json\r\n\r\n{\n";
    
    if (req.indexOf("index") != -1)
    {
      lReply = "<html>";
      lReply += "Available commands:<br>";
      lReply += "/1/up<br>/1/down<br>/2/up<br>/2/down<br>";
      lReply += "/*/up<br>";
      lReply += "/get<br>";
      lReply += "/stop<br><br>";
      lReply += "Reply is a JSON string with state of blinds<br><br>";
      lReply += "</html>{";      
      lValidUrl = true;
    }
    if (req.indexOf("/1/up") != -1)
    {
      lStart0 = MOVING_UP ;
      lValidUrl = true;
    }
    else if (req.indexOf("/1/down") != -1)
    {
      lStart0 = MOVING_DOWN ;
      lValidUrl = true;
    }
    else if (req.indexOf("/2/up") != -1)
    {
      lStart1 = MOVING_UP ;
      lValidUrl = true;
    }
    else if (req.indexOf("/2/down") != -1)
    {
      lStart1 = MOVING_DOWN ;
      lValidUrl = true;
    }
    else if (req.indexOf("/*/up") != -1)
    {
      lStart0 = MOVING_UP ;
      lStart1 = MOVING_UP ;
      lValidUrl = true;
    }
    else if (req.indexOf("/*/down") != -1)
    {
      lStart0 = MOVING_DOWN ;
      lStart1 = MOVING_DOWN ;
      lValidUrl = true;
    }
    else if (req.indexOf("get") != -1)
    {
      lReply += "\"screen 1\" : \"";
      if(mMoving0)
      {
        lReply += "Moving ";
      }
      lReply += (mDirection0==MOVING_UP)?"up":"down";
      lReply += "\"";
      lReply += ",\n\"screen 2\" : \"";
      if(mMoving1)
      {
        lReply += "Moving ";
      }
      lReply += (mDirection1==MOVING_UP)?"up":"down";
      lReply += "\",\n";
      lValidUrl = true;
    }
    else if (req.indexOf("stop") != -1)
    {
      digitalWrite(POWER_RELAIS0, RELAIS_OFF);
      digitalWrite(UP_RELAIS0, RELAIS_OFF);
      mMovingTime0 = 0;
      mMoving0 = false;
      digitalWrite(POWER_RELAIS1, RELAIS_ON);
      digitalWrite(UP_RELAIS1, RELAIS_OFF);
      mMovingTime1 = 0;
      mMoving1 = false;
      lValidUrl = true;
    }
    client.flush();

    if(lStart0 != NOT_MOVING)
    {
      digitalWrite(POWER_RELAIS0, RELAIS_ON);
      mDirection0 = lStart0;
      digitalWrite(UP_RELAIS0, ( mDirection0 == MOVING_UP) ? RELAIS_ON : RELAIS_OFF);
      mMovingTime0 = 0;
      mMoving0 = true;
    }
    if(lStart1 != NOT_MOVING)
    {
      digitalWrite(POWER_RELAIS1, RELAIS_ON);
      mDirection1 = lStart1;
      digitalWrite(UP_RELAIS1, ( mDirection1 == MOVING_UP) ? RELAIS_ON : RELAIS_OFF);
      mMovingTime1 = 0;
      mMoving1 = true;
    }

    // Prepare the response
    //String s = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html><h1>";

    lReply += "\"status\" : \"";
    lReply += lValidUrl ? "OK" : "ERR";
    lReply += "\"\n}\n";
    
    // Send the response to the client
    client.print(lReply);

    delay(100);
  
  // The client will actually be disconnected 
  // when the function returns and 'client' object is detroyed
}

