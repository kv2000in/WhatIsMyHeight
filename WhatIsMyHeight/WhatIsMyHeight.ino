#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <WebSocketsServer.h>
#include <Hash.h>
#include <ESP8266WebServer.h>
#include <string.h>
#define MAX_STRING_LEN  32



const char *ssid = "***********";
const char *password = "***********";



ESP8266WiFiMulti WiFiMulti;

ESP8266WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(8000);


int trigPin = 4;    
int echoPin = 5;    
long duration, cm, inches; 
char measurement[10] = {'D','A','T','A','-'};
char calibration[10] = {'C','A','L','A','-'};
static const char PROGMEM INDEX_HTML[] = R"rawliteral(<!DOCTYPE HTML>
<html>
  <head>
    <meta name="viewport" content="width=device-width, initial-scale=2.7">
      
      <meta  content="text/html; charset=utf-8">
        <style>
          * {
            box-sizing: border-box;
          }
        
        [class*="col-"] {
          float: left;
          padding: 15px;
        }
        /* For mobile phones: */
        [class*="col-"] {
          width: 100%;
        }
        @media only screen and (min-width: 1024px) {
          /* For desktop: */
          .col-1 {width: 8.33%;}
          .col-2 {width: 16.66%;}
          .col-3 {width: 25%;}
          .col-4 {width: 33.33%;}
          .col-5 {width: 41.66%;}
          .col-6 {width: 50%;}
          .col-7 {width: 58.33%;}
          .col-8 {width: 66.66%;}
          .col-9 {width: 75%;}
          .col-10 {width: 83.33%;}
          .col-11 {width: 91.66%;}
          .col-12 {width: 100%;}
        }
        
        
                    input {
                        border: 2px solid red;
                        border-radius: 5px;
                    }
                    button{
                        background-color: #4CAF50; /* Green */
                        border: none;
                        color: white;
                        padding: 15px 32px;
                        text-align: center;
                        text-decoration: none;
                        display: inline-block;
                        font-size: 16px;
                    }
        
          </style>
        <script language="javascript" type="text/javascript">
          
          var boolConnected=false;
          var calibrate=0;
          var measurement=0;
          function doConnect()
          {
            if (!(boolConnected)){
              /*websocket = new WebSocket(document.myform.url.value);*/
              /*
               websocket = new WebSocket('ws://192.168.1.106:8000/');
               */
              
              
              websocket = new WebSocket('ws://' + window.location.hostname + ':8000/'); 
              boolConnected=true;
              websocket.onopen = function(evt) { onOpen(evt) };
              websocket.onclose = function(evt) { onClose(evt) };
              websocket.onmessage = function(evt) { onMessage(evt) };
              websocket.onerror = function(evt) { onError(evt) };
            }
            
          }
        function onOpen(evt)
        {
          console.log("connected\n");
          
        }
        
        function onClose(evt)
        {
          console.log("disconnected\n");
          /* if server disconnected - change the color to red*/
          
          boolConnected=false;
        }
        
        function onMessage(evt)
        {
          console.log("response: " + evt.data + '\n');
          if (evt.data.slice(0,3) == "CALA"){
            calibrate= evt.data.slice(5,9);
            }
          else if  (evt.data.slice(0,3) == "DATA"){
            measurement= evt.data.slice(5,9);
            }
            document.getElementById("heightincms").value= calibrate-measurement;
            document.getElementById("heightininches").value= (calibrate-measurement)/2.54;
        }
        function onError(evt)
        {
          console.log('error: ' + evt.data + '\n');
          websocket.close();
        }
        function doSend(message)
        {
          console.log("sent: " + message + '\n');
          websocket.send(message);
        }
        function closeWS()
        {
        /* Probable bug in arduino websocket - hangs if not closed properly, specially by a phone browser entering a powersaving mode*/
          websocket.close();
          boolConnected=false;
        }
        /*    On android - when page loads - focus event isn't fired so websocket doesn't connect*/                     
        window.addEventListener("focus",doConnect, false);
        window.addEventListener("blur",closeWS, false);
        window.addEventListener('load', function() {
        foo(true); 
        /*After page loading blur doesn't fire until focus has fired at least once*/
        },{once:true}, false);
        function foo(bool) 
        {
          if (bool)
          {
            doConnect();
          } else 
          {
          websocket.close();    
          }
        }
        </script>
        
  <title>What is my Height</title></head>
  <body>
    
    
    <div class = "col-12">
      
      <button onclick="doSend('CALIBRATE')">Calibrate</button>
            <button onclick="doSend('MEASURE')">Measure</button><br><br>
            <label>Your height = </label> <input type='number' id='heightincms'> <strong>CMs , </strong>  <input type='number' id='heightininches'> <strong>inches </strong> 
    </div>
    
  </body>
  
  
</html>


)rawliteral";





void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length)
{
  Serial.printf("webSocketEvent(%d, %d, ...)\r\n", num, type);
  switch(type) {
    case WStype_DISCONNECTED:
      Serial.printf("[%u] Disconnected!\r\n", num);
      break;
    case WStype_CONNECTED:
      {
        IPAddress ip = webSocket.remoteIP(num);
        Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\r\n", num, ip[0], ip[1], ip[2], ip[3], payload);
      //itoa( cm, str, 10 );
      //  webSocket.sendTXT(num, str, strlen(str));
      }
      break;
    case WStype_TEXT:
     {
      Serial.printf("[%u] get Text: %s\r\n", num, payload);
//Payload will be - FOR REV STOP
      
     char *mystring = (char *)payload;
      
      if (strcmp(mystring,"CALIBRATE") == 0)
      {
        doCalibrate();
         webSocket.broadcastTXT(payload, length);
      }
      else if (strcmp(mystring,"MEASURE") == 0) 
      {
        doMeasure();
           webSocket.broadcastTXT(payload, length);
      } 
      else if (strcmp(mystring,"STOP") == 0)
      {
          webSocket.broadcastTXT(payload, length);
       }

     }
      break;
    case WStype_BIN:
      Serial.printf("[%u] get binary length: %u\r\n", num, length);
      hexdump(payload, length);

      // echo data back to browser
      webSocket.sendBIN(num, payload, length);
      break;
    default:
      Serial.printf("Invalid WStype [%d]\r\n", type);
      break;
  }
}

void handleRoot()
{
  server.send_P(200, "text/html", INDEX_HTML);
}

void handleNotFound()
{
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i=0; i<server.args(); i++){
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}


void doCalibrate()
{
 Serial.println("Calibrating");
  hcsr04();
  itoa( cm, calibration+5, 10 );
  webSocket.broadcastTXT(calibration,10);
    
  }

void doMeasure()
{
   Serial.println("Measuring");
  hcsr04();
  itoa( cm, measurement+5, 10 );
  webSocket.broadcastTXT(measurement,10);
    
  
  }
void hcsr04()
{
   //HR-SC04 
   // The sensor is triggered by a HIGH pulse of 10 or more microseconds.
  // Give a short LOW pulse beforehand to ensure a clean HIGH pulse:
  digitalWrite(trigPin, LOW);
  delayMicroseconds(5);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
 
  // Read the signal from the sensor: a HIGH pulse whose
  // duration is the time (in microseconds) from the sending
  // of the ping to the reception of its echo off of an object.
  //pinMode(echoPin, INPUT);
  duration = pulseIn(echoPin, HIGH);
  //delay(ms) pauses the sketch for a given number of milliseconds and allows WiFi and TCP/IP tasks to run. 
  //delayMicroseconds function, on the other hand, does not yield to other tasks, so using it for delays more than 20 milliseconds is not recommended
  delayMicroseconds(10000);
  // convert the time into a distance
  cm = (duration/2) / 29.1;
  //inches = (duration/2) / 74; 
  
  //Serial.print(inches);
  //Serial.print("in, ");
  Serial.print(cm);
  Serial.print("cm");
  Serial.println();
  }
void setup()
{ 


  Serial.begin(115200);
  delay(10);
 
    // HR-SC04
  //Define inputs and outputs
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  Serial.println();
  Serial.println();
  Serial.println();

  for(uint8_t t = 4; t > 0; t--) {
    Serial.printf("[SETUP] BOOT WAIT %d...\r\n", t);
    Serial.flush();
    delay(1000);
  }

  WiFiMulti.addAP(ssid, password);

  while(WiFiMulti.run() != WL_CONNECTED) {
    Serial.print(".");
    delay(100);
  }

  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());


  Serial.print("Connect to http://");
  Serial.println(WiFi.localIP());
  
  server.on("/", handleRoot);
  server.onNotFound(handleNotFound);

  server.begin();

  webSocket.begin();
  webSocket.onEvent(webSocketEvent);



}

void loop()
{
  webSocket.loop();
  server.handleClient();
  }
