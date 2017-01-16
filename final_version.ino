#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include "time_ntp.h"


const char* ssid = "000D0B6BB42B";
const char* password = "";

unsigned long ulSecs2000_timer=0;

#define KEEP_MEM_FREE 10240
#define MEAS_SPAN_H 24
unsigned long dataMereni=0;    
unsigned long hodnotyMereni=0; 
unsigned long casMereni;   
unsigned long dalsiMereni;    
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
  
  WiFi.mode(WIFI_STA);
  WiFstart();
  
  uint32_t free=system_get_free_heap_size() - KEEP_MEM_FREE;
  hodnotyMereni = free / (sizeof(float)*2+sizeof(unsigned long));
  pulTime = new unsigned long[hodnotyMereni];
  pom = new float[hodnotyMereni];
  
  if (pulTime==NULL ||  pom==NULL){
    hodnotyMereni=0;
    Serial.println("Error in memory allocation!");
  }
  else{
    Serial.print("Allocated storage for ");
    Serial.print(hodnotyMereni);
    Serial.println(" data points.");
    
    float mericiDoba = MEAS_SPAN_H*3600./hodnotyMereni;
    casMereni = ( (unsigned long)(mericiDoba+0.5) ) * 1000 + 1772000;
    Serial.print("Measurements will happen each ");
    Serial.print(casMereni);
    Serial.println(" ms.");
    
    dalsiMereni = millis()+casMereni;
  }
}

void WiFstart(){
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
  if (dataMereni==0) {
    String table = "No datas.<BR>";
    if (bStream){
      pclient->print(table);
    }
    ulLength+=table.length();
  }
  else{ 
    unsigned long ulEnd;
    if (dataMereni>hodnotyMereni){
      ulEnd=dataMereni-hodnotyMereni;
    }
    else{
      ulEnd=0;
    }
    
    String table;
    table = "<table style=\"width:100%\"><tr><th>Time / UTC</th><th>Humidity values;</th><th>State:</th></tr>";
    table += "<style>table, th, td {border: 2px solid black; border-collapse: collapse;} th, td {padding: 5px;} th {text-align: left;}</style>";
    for (unsigned long li=dataMereni;li>ulEnd;li--){
      unsigned long ulIndex=(li-1)%hodnotyMereni;
      table += "<tr><td>";
      table += epoch_to_string(pulTime[ulIndex]).c_str();
      table += "</td><td>";
      table += (sensorReading);
      table += "</td><td>";
      if((sensorReading)>980){
        table += "Dry";
        }else
        table += "Humid";
      table += "</td></tr>";

      if(table.length()>1024){
        if(bStream){
          pclient->print(table);
        }
        ulLength+=table.length();
        table="";
      }
    }
    
    table+="</table>";
    ulLength+=table.length();
    if(bStream){
      pclient->print(table);
    }   
  }
  
  return(ulLength);
}
  
String MakeHTTPodpoved(unsigned long ulLength){
  String sodpoved;
  sodpoved  = F("HTTP/1.1 200 OK\r\nContent-Length: ");
  sodpoved += ulLength;
  sodpoved += F("\r\nContent-Type: text/html\r\nConnection: close\r\n\r\n");
  return(sodpoved);
}

void loop() {
  if (millis()>=dalsiMereni) {
    dalsiMereni = millis()+casMereni;
    pulTime[dataMereni%hodnotyMereni] = millis()/1000+ulSecs2000_timer;
    
    Serial.print("Logging: "); 
    Serial.print("Humidity: "); 
    Serial.print(sensorReading);
    Serial.print(" - Time: ");
    Serial.println(pulTime[dataMereni%hodnotyMereni]);
    
    dataMereni++;
  }
  
  if (WiFi.status() != WL_CONNECTED){
    WiFstart();
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
  
  String pozadavek = client.readStringUntil('\r');
  client.flush();
  
  if(pozadavek==""){
    Serial.println("empty request! - stopping client");
    client.stop();
    return;
  }
  
  String cesta="",parametr="", cmd="";
  String sGetstart="GET ";
  int start,konMez,konCes;
  start = pozadavek.indexOf(sGetstart);
  if (start>=0){
    start+=+sGetstart.length();
    konMez = pozadavek.indexOf(" ",start);
    konCes = pozadavek.indexOf("?",start);
    
    if(konMez>0){
      if(konCes>0){
        cesta  = pozadavek.substring(start,konCes);
        parametr = pozadavek.substring(konCes,konMez);
      }
      else{
        cesta  = pozadavek.substring(start,konMez);
      }
    }
  }
    
  String odpoved,odpoved2,sodpoved;
  if(cesta=="/"){
    
    int iIndex= (dataMereni-1)%hodnotyMereni;
    
    odpoved  = F("<html>\n<head>\n<title>Soil moisture sensor</title>\n");
    odpoved += F("</head>\n<body>\n<font color=\"#000000\"><body bgcolor=\"#33CCCC\"><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=yes\"><h1>Soil moisture sensor</h1><BR><FONT SIZE=+1>Last measurements at ");
    odpoved += epoch_to_string(pulTime[iIndex]).c_str();
    odpoved += F(" UTC<BR>\n<div id=\"gaugetemp_div\" style=\"float:left; width:0px; height: 0px;\"></div> \n<div id=\"gaugehum_div\" style=\"float:left; width: 0px; height: 0px;\"></div>\n<div style=\"clear:both;\"></div>");
    odpoved2 = F("<p><a href=\"/table\">Table</a></p>");

    
    client.print(MakeHTTPodpoved(odpoved.length()+odpoved2.length()).c_str());
    client.print(odpoved);
    client.print(sensorReading);
    if((sensorReading)>980){
        client.print(" - Dry");
        }else
        client.print(" - Humid");
    client.print("");
    client.print("");
    client.print("");
    client.print("");
    client.print(odpoved2);
  }else if(cesta=="/table"){

    unsigned long ulSizeList = MakeTable(&client,false);
    
    odpoved  = F("<html><head><title>Soil moisture sensor</title></head><body>");
    odpoved += F("<font color=\"#000000\"><body bgcolor=\"#33CCCC\">");
    odpoved += F("<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=yes\">");
    odpoved += F("<h1>Soil moisture sensor</h1>");
    odpoved += F("<FONT SIZE=+1>");
    odpoved += F("<a href=\"/\">Front page</a><BR><BR>Last measurements at a distance of ");
    odpoved += casMereni;
    odpoved += F("ms<BR>");

    client.print(MakeHTTPodpoved(odpoved.length()+odpoved2.length()+ulSizeList).c_str()); 
    client.print(odpoved); odpoved="";
    MakeTable(&client,true);
    client.print(odpoved2);
  }

  client.stop();
  Serial.println("Client disconnected");
}

