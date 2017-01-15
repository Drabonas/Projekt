
#include "time_ntp.h"

IPAddress timeServer(129, 6, 15, 28);
WiFiUDP udp;
unsigned int ntpPort = 2390;
const int NTP_PACKET_SIZE = 48; 
byte packetBuffer[ NTP_PACKET_SIZE];


static unsigned short days[4][12] =
{
    {   0,  31,  60,  91, 121, 152, 182, 213, 244, 274, 305, 335},
    { 366, 397, 425, 456, 486, 517, 547, 578, 609, 639, 670, 700},
    { 731, 762, 790, 821, 851, 882, 912, 943, 974,1004,1035,1065},
    {1096,1127,1155,1186,1216,1247,1277,1308,1339,1369,1400,1430},
};


unsigned int date_time_to_epoch(date_time_t* date_time)
{
    unsigned int second = date_time->second;
    unsigned int minute = date_time->minute;  
    unsigned int hour   = date_time->hour;
    unsigned int day    = date_time->day-1;
    unsigned int month  = date_time->month-1;
    unsigned int year   = date_time->year;
    return (((year/4*(365*4+1)+days[year%4][month]+day)*24+hour)*60+minute)*60+second;
}


unsigned long getNTPTimestamp()
{
  unsigned long ulSecs2000;
  
  udp.begin(ntpPort);
  sendNTPpacket(timeServer);
  delay(1000);
  int cb = udp.parsePacket();

  if(!cb)
  {
    Serial.println("Timeserver not accessible! - No RTC support!"); 
    ulSecs2000=0;
  }
  else
  {
    Serial.print("packet received, length=");
    Serial.println(cb);
    udp.read(packetBuffer, NTP_PACKET_SIZE);
    
    unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
    unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);

    ulSecs2000  = highWord << 16 | lowWord;
    ulSecs2000 -= 2208988800UL;
    ulSecs2000 -= 946684800UL;
  }    
  return(ulSecs2000);
}


unsigned long sendNTPpacket(IPAddress& address)
{
  Serial.println("sending NTP packet...");
  memset(packetBuffer, 0, NTP_PACKET_SIZE);

  packetBuffer[0] = 0b11100011; 
  packetBuffer[1] = 0;     
  packetBuffer[2] = 6;   
  packetBuffer[3] = 0xEC;
  
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;


  udp.beginPacket(address, 123);
  udp.write(packetBuffer, NTP_PACKET_SIZE);
  udp.endPacket();
}


void epoch_to_date_time(date_time_t* date_time,unsigned int epoch)
{
    date_time->second = epoch%60; epoch /= 60;
    date_time->minute = epoch%60; epoch /= 60;
    date_time->hour   = epoch%24; epoch /= 24;

    unsigned int years = epoch/(365*4+1)*4; epoch %= 365*4+1;

    unsigned int year;
    for (year=3; year>0; year--)
    {
        if (epoch >= days[year][0])
            break;
    }

    unsigned int month;
    for (month=11; month>0; month--)
    {
        if (epoch >= days[year][month])
            break;
    }

    date_time->year  = years+year;
    date_time->month = month+1;
    date_time->day   = epoch-days[year][month]+1;
}

String epoch_to_string(unsigned int epoch)
{
  date_time_t date_time;
  epoch_to_date_time(&date_time,epoch);
  String s;
  int i;
  
  s = date_time.hour; 
  s+= ":";
  i=date_time.minute;
  if (i<10) {s+= "0";}
  s+= i;
  s+= ":";
  i=date_time.second;
  if (i<10) {s+= "0";}
  s+= i;
  s+= " - ";
  s+= date_time.day;
  s+= ".";
  s+= date_time.month;
  s+= ".";
  s+= 2000+(int)date_time.year;
  
  return(s);
}

