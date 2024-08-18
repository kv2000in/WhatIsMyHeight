/*Set Debug to Serial1 when flashing the sketch. Then all the os_printf will print to Serial1 which is GND to GND and USB-TTL RX to GPIO2 (Tx1)
Initialize Serial but leave it alone in case toggling between Serial or SW Serial for data from US-100 
*/
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>


const char *ssid = "ESP";
const char *password = "monte123";
bool wsclientconnected  = false;
unsigned long previousMillis = 0;
unsigned long datasendinterval = 750;

AsyncWebServer server(80);
AsyncWebSocket webSocket("/ws"); 




char str[16]; //6 bytes + macaddr + 2 bytes distance + 2 bytes temp + 2 bytes battery + 4 bytes zeros
uint8_t macAddr[6];








// the US-100 module has jumper cap on the back for Serial comm mode. No Jumper for direct - trig/echo mode
unsigned int HighLen = 0;
unsigned int LowLen  = 0;
unsigned int Len_mm  = 0;
int Temperature45 = 0;

int batteryVoltage;   
int R1=990;
int R2=298;

int maxmillistowaitforUS100serialdata = 200; //Was working well with 500


#include <SoftwareSerial.h>

#define TRIGGER_PIN  4  // Arduino pin tied to trigger pin on the ultrasonic sensor. //TX pin for swSerial
#define ECHO_PIN     5  // Arduino pin tied to echo pin on the ultrasonic sensor.//RX pin for swSerial


SoftwareSerial swSer;

unsigned int get_distance_via_swserial()
{
int cyclecount=0;
int maxcyclecount=5;
HighLen = 0;
LowLen  = 0;
Len_mm  = 0;
while ((Len_mm==0) && (cyclecount<maxcyclecount)) 
{
swSer.flush();                               // clear receive buffer of serial port
swSer.write(0X55);                           // trig US-100 begin to measure the distance
delay(500);                                   // delay 500ms to wait result
if(swSer.available() >= 2)                   // when receive 2 bytes 
{
HighLen = swSer.read();                   // High byte of distance
LowLen  = swSer.read();                   // Low byte of distance
Len_mm  = HighLen*256 + LowLen;            // Calculate the distance

}
cyclecount++;
delay(50);

}  
os_printf("Distance is %d\n", Len_mm/10);
return Len_mm/10;

}

int get_temp_via_swserial()
{
Temperature45 = 0;
swSer.flush();       // clear receive buffer of serial port
swSer.write(0X50);   // trig US-100 begin to measure the temperature
delay(500);            //delay 500ms to wait result
if(swSer.available() >= 1)            //when receive 1 bytes 
{
Temperature45 = swSer.read();     //Get the received byte (temperature)
if((Temperature45 > 1) && (Temperature45 < 130))   //the valid range of received data is (1, 130)
{
Temperature45 -= 45;                           //Real temperature = Received_Data - 45

}
}

delay(10);   
//delay 500ms
os_printf("Temp is %d\n", Temperature45);
return Temperature45;
}






//unsigned int get_distance_via_serial()
//{
//int cyclecount=0;
//int maxcyclecount=5;
//
//HighLen = 0;
//LowLen  = 0;
//Len_mm  = 0;
//
//while ((Len_mm==0) && (cyclecount<maxcyclecount)) 
//{
//Serial.flush();                               // clear receive buffer of serial port
//Serial.write(0X55);                           // trig US-100 begin to measure the distance
//delay(maxmillistowaitforUS100serialdata);     
//if(Serial.available() >= 2)                   // when receive 2 bytes 
//{
//HighLen = Serial.read();                   // High byte of distance
//LowLen  = Serial.read();                   // Low byte of distance
//Len_mm  = HighLen*256 + LowLen;            // Calculate the distance
//
//}
//cyclecount++;
//delay(50);
//
//}  
//os_printf("Distance is %d\n", Len_mm/10);
//return Len_mm/10;
//
//}
//
//int get_temp_via_serial()
//{
// Temperature45 = 0;
//Serial.flush();       // clear receive buffer of serial port
//Serial.write(0X50);   // trig US-100 begin to measure the temperature
//delay(maxmillistowaitforUS100serialdata);            
//if(Serial.available() >= 1)            //when receive 1 bytes 
//{
//Temperature45 = Serial.read();     //Get the received byte (temperature)
//if((Temperature45 > 1) && (Temperature45 < 130))   //the valid range of received data is (1, 130)
//{
//Temperature45 -= 45;                           //Real temperature = Received_Data - 45
//
//}
//}
//
//delay(10);                            //delay 500ms
//os_printf("Temp is %d\n", Temperature45);
//return Temperature45;
//}



void preparedata(){


WiFi.macAddress(macAddr);
unsigned int mydistance;
int mytemp;
//ADC*(1.1/1024) will give the Vout at the voltage divider
//V=(Vout*((R1+R2)/R2))*1000 miliVolts
//batteryVoltage = ((analogRead(A0)*(1.1/1024))*((R1+R2)/R2))*1000;


memcpy(str,macAddr,6); // First 6 bytes = MAC Address , then 2 bytes of distance, 2 bytes of temp and 2 bytes of batteryvoltage = total 12 bytes. Padded total 16 bytes.
mydistance=get_distance_via_swserial();
mytemp=get_temp_via_swserial();
//mydistance=get_distance_via_serial();
//mytemp=get_temp_via_serial();
memcpy(str+6,&mydistance,2);
memcpy(str+8,&mytemp,2);
memcpy(str+10,&batteryVoltage,2);

delay(10); 


}


void onEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len){
  if(type == WS_EVT_CONNECT){
    //client connected
    wsclientconnected = true;
    os_printf("ws[%s][%u] connect\n", server->url(), client->id());
    //client->printf("Z#a020a60a86ce|-811493|2022-4-12 21:33:49");
    //client->printf("C#a020a60a86ce|500/285|2022-4-12 21:33:49"); 
    //client->ping();
  } else if(type == WS_EVT_DISCONNECT){
    //client disconnected
    wsclientconnected = false;
    os_printf("ws[%s][%u] disconnect: %u\n", server->url(), client->id());
  } else if(type == WS_EVT_ERROR){
    //error was received from the other end
    os_printf("ws[%s][%u] error(%u): %s\n", server->url(), client->id(), *((uint16_t*)arg), (char*)data);
  } else if(type == WS_EVT_PONG){
    //pong message was received (in response to a ping request maybe)
    os_printf("ws[%s][%u] pong[%u]: %s\n", server->url(), client->id(), len, (len)?(char*)data:"");
  } else if(type == WS_EVT_DATA){
  
    AwsFrameInfo * info = (AwsFrameInfo*)arg;
    if(info->final && info->index == 0 && info->len == len){
      //the whole message is in a single frame and we got all of it's data
      os_printf("ws[%s][%u] %s-message[%llu]: ", server->url(), client->id(), (info->opcode == WS_TEXT)?"text":"binary", info->len);
      if(info->opcode == WS_TEXT){
        data[len] = 0;
        os_printf("%s\n", (char*)data);
//        
      } else {
        for(size_t i=0; i < info->len; i++){
          os_printf("%02x ", data[i]);
        }
        os_printf("\n");
      }
      if(info->opcode == WS_TEXT){
        //client->text("I got your text message");
      }
      else{
        //client->binary("I got your binary message");
      }
    
        
    } else {
      //message is comprised of multiple frames or the frame is split into multiple packets
      if(info->index == 0){
        if(info->num == 0)
          os_printf("ws[%s][%u] %s-message start\n", server->url(), client->id(), (info->message_opcode == WS_TEXT)?"text":"binary");
        os_printf("ws[%s][%u] frame[%u] start[%llu]\n", server->url(), client->id(), info->num, info->len);
      }

      os_printf("ws[%s][%u] frame[%u] %s[%llu - %llu]: ", server->url(), client->id(), info->num, (info->message_opcode == WS_TEXT)?"text":"binary", info->index, info->index + len);
      if(info->message_opcode == WS_TEXT){
        data[len] = 0;
        os_printf("%s\n", (char*)data);
      } else {
        for(size_t i=0; i < len; i++){
          os_printf("%02x ", data[i]);
        }
        os_printf("\n");
      }

      if((info->index + len) == info->len){
        os_printf("ws[%s][%u] frame[%u] end[%llu]\n", server->url(), client->id(), info->num, info->len);
        if(info->final){
          os_printf("ws[%s][%u] %s-message end\n", server->url(), client->id(), (info->message_opcode == WS_TEXT)?"text":"binary");
          if(info->message_opcode == WS_TEXT){
           // client->text("I got your text message");
          }
          else {
         //   client->binary("I got your binary message");
        }
        }
      }
    }
 
    
  }
}

void senddata(){
  webSocket.binaryAll(str, 16);
  }

void setup() {

 Serial.begin(9600);
 Serial1.begin(9600);
  delay(10);
 

  Serial1.println();
  Serial1.println();
  Serial1.println();
WiFi.mode(WIFI_STA);
  for(uint8_t t = 4; t > 0; t--) {
    Serial1.printf("[SETUP] BOOT WAIT %d...\r\n", t);
    Serial1.flush();
    delay(1000);
  }

  Serial1.print("Configuring access point...");
  WiFi.softAP(ssid, password,1);

  IPAddress myIP = WiFi.softAPIP();
  Serial1.print("AP IP address: ");
 Serial1.println(myIP);
Serial1.printf("MAC address = %s\n", WiFi.softAPmacAddress().c_str());


memset(str,0,16); //reset our data buffer



pinMode(ECHO_PIN,INPUT); //GPIO 5- Rx PIN for swSer
pinMode(TRIGGER_PIN,OUTPUT); //GPIO 4 - Tx PIN for swSer

swSer.begin(9600, SWSERIAL_8N1, ECHO_PIN, TRIGGER_PIN, false, 95, 11);           

webSocket.onEvent(onEvent);
  server.addHandler(&webSocket);
  server.begin();











}
void loop() {
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >=datasendinterval){
    
    if(wsclientconnected){
      preparedata();
      senddata();
      memset(str,0,16); //reset our data buffer
      }
    previousMillis = millis();
    
  }
webSocket.cleanupClients();

}
