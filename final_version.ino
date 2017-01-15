#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include "time_ntp.h"


const char* ssid = "000D0B6BB42B";
const char* password = "";

unsigned long ulSecs2000_timer=0;

#define KEEP_MEM_FREE 10240
#define MEAS_SPAN_H 24
unsigned long ulMeasCount=0;    
unsigned long ulNoMeasValues=0; 
unsigned long ulMeasDelta_ms;   
unsigned long ulNextMeas_ms;    
unsigned long *pulTime;         
float *pom;
int sensorReading = analogRead(A0);

WiFiServer server(80);

extern "C" 
{
#include "user_interface.h"
}


void setup(){   
  Serial.begin(9600);
  Serial.println("Soil moisture sensor - Dominik Kerlin");
  
  // inital connect
  WiFi.mode(WIFI_STA);
  WiFiStart();
  
  // allocate ram for data storage
  uint32_t free=system_get_free_heap_size() - KEEP_MEM_FREE;
  ulNoMeasValues = free / (sizeof(float)*2+sizeof(unsigned long));  // humidity & temp --> 2 + time
  pulTime = new unsigned long[ulNoMeasValues];
  pom = new float[ulNoMeasValues];
  
  if (pulTime==NULL ||  pom==NULL)
  {
    ulNoMeasValues=0;
    Serial.println("Error in memory allocation!");
  }
  else
  {
    Serial.print("Allocated storage for ");
    Serial.print(ulNoMeasValues);
    Serial.println(" data points.");
    
    float fMeasDelta_sec = MEAS_SPAN_H*3600./ulNoMeasValues;
    ulMeasDelta_ms = ( (unsigned long)(fMeasDelta_sec+0.5) ) * 1000 /*+ 1772000*/;  // round to full sec
    Serial.print("Measurements will happen each ");
    Serial.print(ulMeasDelta_ms);
    Serial.println(" ms.");
    
    ulNextMeas_ms = millis()+ulMeasDelta_ms;
  }
}

void WiFiStart(){
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED){
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  
  server.begin();
  Serial.println("Server started");

  Serial.println(WiFi.localIP());
  
  ulSecs2000_timer=getNTPTimestamp();
  Serial.print("Current Time UTC from NTP server: " );
  Serial.println(epoch_to_string(ulSecs2000_timer).c_str());

  ulSecs2000_timer -= millis()/1000;
}

unsigned long MakeTable (WiFiClient *pclient, bool bStream){
  unsigned long ulLength=0;
  if (ulMeasCount==0) {
    String sTable = "No datas.<BR>";
    if (bStream){
      pclient->print(sTable);
    }
    ulLength+=sTable.length();
  }
  else{ 
    unsigned long ulEnd;
    if (ulMeasCount>ulNoMeasValues){
      ulEnd=ulMeasCount-ulNoMeasValues;
    }
    else{
      ulEnd=0;
    }
    
    String sTable;
    sTable = "<table style=\"width:100%\"><tr><th>Time / UTC</th><th>Humidity values;</th></tr>";
    sTable += "<style>table, th, td {border: 2px solid black; border-collapse: collapse;} th, td {padding: 5px;} th {text-align: left;}</style>";
    for (unsigned long li=ulMeasCount;li>ulEnd;li--){
      unsigned long ulIndex=(li-1)%ulNoMeasValues;
      sTable += "<tr><td>";
      sTable += epoch_to_string(pulTime[ulIndex]).c_str();
      sTable += "</td><td>";
      sTable += (sensorReading);
      sTable += "</td></tr>";

      if(sTable.length()>1024){
        if(bStream){
          pclient->print(sTable);
        }
        ulLength+=sTable.length();
        sTable="";
      }
    }
    
    sTable+="</table>";
    ulLength+=sTable.length();
    if(bStream){
      pclient->print(sTable);
    }   
  }
  
  return(ulLength);
}
  
String MakeHTTPHeader(unsigned long ulLength){
  String sHeader;
  sHeader  = F("HTTP/1.1 200 OK\r\nContent-Length: ");
  sHeader += ulLength;
  sHeader += F("\r\nContent-Type: text/html\r\nConnection: close\r\n\r\n");
  return(sHeader);
}

void loop() {
  if (millis()>=ulNextMeas_ms) {
    ulNextMeas_ms = millis()+ulMeasDelta_ms;


    pulTime[ulMeasCount%ulNoMeasValues] = millis()/1000+ulSecs2000_timer;
    
    Serial.print("Logging Temperature: "); 
    Serial.print("Humidity: "); 
    Serial.print(sensorReading);
    Serial.print(" - Time: ");
    Serial.println(pulTime[ulMeasCount%ulNoMeasValues]);
    
    ulMeasCount++;
  }
  
  if (WiFi.status() != WL_CONNECTED){
    WiFiStart();
  }
  
  WiFiClient client = server.available();
  if (!client){
    return;
  }
  
  Serial.println("new client");
  unsigned long ultimeout = millis()+250;
  while(!client.available() && (millis()<ultimeout)){
    delay(1);
  }
  if(millis()>ultimeout){ 
    Serial.println("client connection time-out!");
    return; 
  }
  
  String sRequest = client.readStringUntil('\r');
  client.flush();
  
  if(sRequest==""){
    Serial.println("empty request! - stopping client");
    client.stop();
    return;
  }
  
  String sPath="",sParam="", sCmd="";
  String sGetstart="GET ";
  int iStart,iEndSpace,iEndQuest;
  iStart = sRequest.indexOf(sGetstart);
  if (iStart>=0)
  {
    iStart+=+sGetstart.length();
    iEndSpace = sRequest.indexOf(" ",iStart);
    iEndQuest = sRequest.indexOf("?",iStart);
    
    // are there parameters?
    if(iEndSpace>0)
    {
      if(iEndQuest>0)
      {
        // there are parameters
        sPath  = sRequest.substring(iStart,iEndQuest);
        sParam = sRequest.substring(iEndQuest,iEndSpace);
      }
      else
      {
        // NO parameters
        sPath  = sRequest.substring(iStart,iEndSpace);
      }
    }
  }
    
  String sResponse,sResponse2,sHeader;
  if(sPath=="/"){
    
    int iIndex= (ulMeasCount-1)%ulNoMeasValues;
    
    sResponse  = F("<html>\n<head>\n<title>Soil moisture sensor</title>\n");
    sResponse += F("</head>\n<body>\n<font color=\"#000000\"><body bgcolor=\"#33CCCC\"><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=yes\"><h1>Soil moisture sensor</h1><BR><FONT SIZE=+1>Last measurements at ");
    sResponse += epoch_to_string(pulTime[iIndex]).c_str();
    sResponse += F(" UTC<BR>\n<div id=\"gaugetemp_div\" style=\"float:left; width:0px; height: 0px;\"></div> \n<div id=\"gaugehum_div\" style=\"float:left; width: 0px; height: 0px;\"></div>\n<div style=\"clear:both;\"></div>");
    sResponse2 = F("<p><a href=\"/table\">Table</a></p>");

    
    client.print(MakeHTTPHeader(sResponse.length()+sResponse2.length()).c_str());
    client.print(sResponse);
    client.print(sResponse2);
  }else if(sPath=="/table"){

    unsigned long ulSizeList = MakeTable(&client,false);
    
    sResponse  = F("<html><head><title>Soil moisture sensor</title></head><body>");
    sResponse += F("<font color=\"#000000\"><body bgcolor=\"#33CCCC\">");
    sResponse += F("<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=yes\">");
    sResponse += F("<h1>Soil moisture sensor</h1>");
    sResponse += F("<FONT SIZE=+1>");
    sResponse += F("<a href=\"/\">Front page</a><BR><BR>Last measurements at a distance of ");
    sResponse += ulMeasDelta_ms;
    sResponse += F("ms<BR>");

    client.print(MakeHTTPHeader(sResponse.length()+sResponse2.length()+ulSizeList).c_str()); 
    client.print(sResponse); sResponse="";
    MakeTable(&client,true);
    client.print(sResponse2);
  }

  client.stop();
  Serial.println("Client disconnected");
}

