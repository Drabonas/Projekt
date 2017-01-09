#include <ESP8266WiFi.h>
#include <WiFiClient.h>

char ssid[] = "000D0B6BB42B";
char pass[] = "";
int keyIndex = 0;

int status = WL_IDLE_STATUS;

WiFiServer server(80);

void setup() {

  Serial.begin(9600); 
  while (!Serial) {
  }
  
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("WiFi shield not present"); 
    // don't continue:
    while(true);
  } 
  
  status = WiFi.begin(ssid, pass);
  while ( status != WL_CONNECTED) { 
    status = WiFi.status();
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);

    delay(10000);
  } 
  server.begin();
  printWifiStatus();
}


void loop() {
  int SensorValue = analogRead(A0);
  char linebuf[255], getvalue[255], *lineptr;
  int linecount;
  
  WiFiClient client = server.available();
  if (client) {
#ifdef sdebug
    Serial.println("new client");
#endif
    boolean currentLineIsBlank = true;
    lineptr = linebuf;
    linecount = 0;
    
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
#ifdef sdebug
        Serial.write(c);
#endif
        // if you've gotten to the end of the line (received a newline
        // character) and the line is blank, the http request has ended,
        // so you can send a reply
        if (c == '\n' && currentLineIsBlank) {
          // send a standard http response header
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println("Connection: close");  // the connection will be closed after completion of the response
          client.println();
          client.println("<!DOCTYPE HTML>");
          client.println("<html>");


    client.print("Humidity : "); 
    if(SensorValue >= 1000) {
     client.println("Sensor is not in the Soil or DISCONNECTED");
    }
    if(SensorValue < 1000 && SensorValue >= 600) { 
     client.println("Soil is DRY");
    }
    if(SensorValue < 600 && SensorValue >= 370) {
     client.println("Soil is HUMID"); 
    }
    if(SensorValue < 370) {
     client.println("Sensor in WATER");
    } 

          
          client.println("<br></html>");
           break;
        }
        if (c == '\n') {
          currentLineIsBlank = true;
          if (memcmp(linebuf, "GET ", 4) == 0)
          {
            strcpy(getvalue, &(linebuf[4]));
#ifdef sdebug
            Serial.print("GET buffer = ");
            Serial.println(getvalue);
#endif
          }
          lineptr = linebuf;
          linecount = 0;
        } 
        else if (c != '\r') {
          currentLineIsBlank = false;
          if (linecount < sizeof(linebuf))
          {
            *lineptr++ = c;
            linecount++;
          }
        }
      }
    }
    delay(1);
    
    // close the connection:
    client.stop();
#ifdef sdebug
    Serial.println("client disonnected");
#endif
  }
}


void printWifiStatus() {
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}

