
#ifndef _DEF_TIME_NTP_ST_
#define _DEF_TIME_NTP_ST_

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

typedef struct
{
    unsigned char second;
    unsigned char minute;
    unsigned char hour;
    unsigned char day; 
    unsigned char month;
    unsigned char year;  
}
date_time_t;

unsigned long getNTPTimestamp();
unsigned long sendNTPpacket(IPAddress& address);
unsigned int date_time_to_epoch(date_time_t* date_time);
void epoch_to_date_time(date_time_t* date_time,unsigned int epoch);
String epoch_to_string(unsigned int epoch);

#endif
