#include <ESP8266WiFi.h>

#define MAX_SRV_CLIENTS 1
const char* ssid = "***";
const char* password = "***";

WiFiServer server(60128);
WiFiClient serverClients[MAX_SRV_CLIENTS];

const unsigned char MSG_HEADER[] = { 'I', 'S', 'C', 'P' };
const unsigned char MSG_BIGHDR[] = { 'I', 'S', 'C', 'P', 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00 };
/*
const unsigned char MSG_PWRQSTN[] = { '!', '1', 'P', 'W', 'R', 'Q', 'S', 'T', 'N', 0x0D, 0x0A };
const unsigned char MSG_MVLQSTN[] = { '!', '1', 'M', 'V', 'L', 'Q', 'S', 'T', 'N', 0x0D, 0x0A };

const unsigned char MSG_PWRANSR[] = { '!', '1', 'P', 'W', 'R', '0', '0', 0x1A, 0x0D, 0x0A };
const unsigned char MSG_MVLANSR[] = { '!', '1', 'M', 'V', 'L', '0', '0', 0x1A, 0x0D, 0x0A };
const unsigned char MSG_SSTANSR[] = { '!', '1', 'S', 'S', 'T', '0', '0', 0x1A };
*/

void setup() {
  Serial.begin(9600, SERIAL_8N1);
  //Serial1.setDebugOutput(true);
  WiFi.begin(ssid, password);
  //Serial.print("\nConnecting to "); Serial.println(ssid);
  uint8_t i = 0;
  while (WiFi.status() != WL_CONNECTED && i++ < 20) delay(500);
  if(i == 21){
    Serial.print("Could not connect to"); Serial.println(ssid);
    while(1) delay(500);
  }
  
  server.begin();
  server.setNoDelay(true);
  /*Serial.print("Ready! Use 'telnet ");
  Serial.print(WiFi.localIP());
  Serial.println(" 60128' to connect");*/
}

void loop() {
  uint8_t i;
  if (server.hasClient())
  {
    for(i = 0; i < MAX_SRV_CLIENTS; i++)
    {
      if (!serverClients[i] || !serverClients[i].connected())
      {
        if(serverClients[i]) serverClients[i].stop();
        serverClients[i] = server.available();
        continue;
      }
    }
    //no free spot
    WiFiClient serverClient = server.available();
    serverClient.stop();
  }
  for(i = 0; i < MAX_SRV_CLIENTS; i++){
    if (serverClients[i] && serverClients[i].connected())
    {
      if(serverClients[i].available())
      {
        serverClients[i].find((byte*)MSG_HEADER);
        while(serverClients[i].available() < 8) ;
        uint8_t lenbytes[8];
        serverClients[i].readBytes(lenbytes, 8);
        uint32_t datalen = 0, headerlen = 0;
        for(int i = 0; i < 4; i++)
        {
          headerlen <<= 8; datalen <<= 8; 
          headerlen += lenbytes[i]; datalen += lenbytes[i + 4];
        }
        while(serverClients[i].available() < headerlen - 12) ;
        uint8_t reservebytes[headerlen - 12];
        serverClients[i].readBytes(reservebytes, headerlen - 12);
        uint8_t buf[datalen];
        serverClients[i].readBytes(buf, datalen);
        Serial.write(buf, datalen);
        /*delay(20);
        if(memcmp(buf, MSG_PWRQSTN, 11) == 0)
        {
          Serial.println("Power!");
          SendBuf((byte*)MSG_PWRANSR, 10);
        }
        if(memcmp(buf, MSG_MVLQSTN, 11) == 0)
        {
          Serial.println("Volume!");
          SendBuf((byte*)MSG_MVLANSR, 10);
        }*/
      }
    }
  }
  if(Serial.available()){
    //move to next start
    while(Serial.available() && Serial.peek() != '!')
    {
      Serial.read();
    }
    int bufmax = 128;
    uint8_t buf[bufmax];
    int readbytes = 0;
    while(Serial.available() && Serial.peek() != 0x1A && readbytes < bufmax - 2)
    {
      buf[readbytes] = Serial.read();
      readbytes++; 
    }
    if(Serial.peek() == 0x1A)
    {
      buf[readbytes] = Serial.read();
      buf[readbytes + 1] = 0x0D;
      buf[readbytes + 2] = 0x0A;
      readbytes += 3;
      SendBuf(buf, readbytes);
    }
  }
}

void SendBuf(uint8_t buf[], int len)
{
  uint8_t sendbuf[len + 16];
  for(int i = 0; i < 16; i++)
  {
    sendbuf[i] = MSG_BIGHDR[i];
  }
  sendbuf[11] = (uint8_t)len;
  for(int i = 0; i < len; i++)
  {
    sendbuf[i + 16] = buf[i];
  }

  for(int i = 0; i < MAX_SRV_CLIENTS; i++)
  {
    if (serverClients[i] && serverClients[i].connected())
    {
      serverClients[i].write(sendbuf, len + 16);
      delay(1);
    }
  }
}


