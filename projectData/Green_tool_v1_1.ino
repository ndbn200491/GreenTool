/*
 Basic ESP8266 MQTT example
Bnag Nguyen

 This sketch demonstrates the capabilities of the pubsub library in combination
 with the ESP8266 board/library.

 It connects to an MQTT server then:
  - publishes "hello world" to the topic "outTopic" every two seconds
  - subscribes to the topic "inTopic", printing out any messages
    it receives. NB - it assumes the received payloads are strings not binary
  - If the first character of the topic "inTopic" is an 1, switch ON the ESP Led,
    else switch it off

 It will reconnect to the server if the connection is lost using a blocking
 reconnect function. See the 'mqtt_reconnect_nonblocking' example for how to
 achieve the same result without blocking the main loop.

 To install the ESP8266 board, (using Arduino 1.6.4+):
  - Add the following 3rd party board manager under "File -> Preferences -> Additional Boards Manager URLs":
       http://arduino.esp8266.com/stable/package_esp8266com_index.json
  - Open the "Tools -> Board -> Board Manager" and click install for the ESP8266"
  - Select your ESP8266 in "Tools -> Board"

*/

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "parson.h"
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <ArduinoJson.h>
#include "WiFisetup.h"
//#define DEBUG_SYSN
//#define USART_CHECK
//#define DEBUG
//#define DEBUG_USART
//#define DEBUG_USART_MQTT_REC
//#define MQTT_DEBUG_CONNECTION
//#define DEBUG_MQTT_PRO
//#define MQTT_RX
#define SERIAL_RX_BUFEER_SIZE 22
#define SERIAL_TX_BUFFER_SIZE 41
#define CHAR_TAIL_TX 255
#define CHAR_TAIL_RX 255
//#define DEBUG_USART_Tx

// Update these with values suitable for your network.

//const char* ssid = "AP_cisco";
//const char* password = "Chantroiviet@2014";

const char* mqtt_server = "iot.eclipse.org";
const char* userName = "ndbn200491";
const char* userPassword = "ndbn1909";
//const char* mqtt_server = "broker.mqtt-dashboard.com";
const char* ssid_ap = "greenturaHost";
const char* pass_ap = "12345678" ;
const char* pubTopic = "ndbn200491";
//const char* mqtt_server = "broker.hivemq.com";
int cntStatus= 0;
//ESP8266WebServer server(80);
WiFiClient espClient;
WiFiServer espServer(80);
PubSubClient client(espClient);
//String content;
int seriCnt = 0 ;
long lastMsg = 0;
long lastWiFiConnect ;
char msg[50];
String httpMsgPayload ;
char jsonTxMsg[300];
int value = 0;
int is = 0  ;
StaticJsonBuffer<300> jsonBuffer;
 //
 // Step 2: Build object tree in memory
 //
 JsonObject& root = jsonBuffer.createObject();
//JsonArray& data = root.createNestedArray("data");

/* typedef union ctrDataPackage{
	char bufferDrvRx[25];
	 struct
	    {
		float humi;
		float temp;
		float ph;
		float curr;
		float vol ;
		uint16_t timeCalib;
		uint8_t ctrlBot1Satus ;
		uint8_t ctrlBot2Satus ;
		uint8_t ctrlBot3Satus ;
	    };
}sensorDataStruct_t;
*/
typedef union {
  char bufferDrvRx[SERIAL_RX_BUFEER_SIZE-1];
  struct{

	  uint16_t tempVal;
	  uint16_t humdVal;
	  uint16_t ecVal;
	  uint16_t ppmVal;
	  uint16_t PHVal;
	  uint16_t ldrVal;
	  uint16_t lastUpdateAll;
	  uint8_t sysn;
	  uint8_t sst;
	  uint8_t relay1Stt;
	  uint8_t relay2Stt;
	  uint8_t relay3Stt;
	  uint8_t relay4Stt;
	  uint8_t wLevel ;

  };
}driverDataStruct_t;

typedef union {
	char ctrAppData[SERIAL_TX_BUFFER_SIZE];
	 struct
	    {
		uint16_t  time1Bot1On;
		uint16_t  time1Bot2On;
		uint16_t  time1Bot3On;
		uint16_t  time2Bot1On;
		uint16_t  time2Bot2On;
		uint16_t  time2Bot3On;
		uint16_t  time3Bot1On;
		uint16_t  time3Bot2On;
		uint16_t  time3Bot3On;
		uint16_t  time1Bot1Off;
		uint16_t  time1Bot2Off;
		uint16_t  time1Bot3Off;
		uint16_t  time2Bot1Off;
		uint16_t  time2Bot2Off;
		uint16_t  time2Bot3Off;
		uint16_t  time3Bot1Off;
		uint16_t  time3Bot2Off;
		uint16_t  time3Bot3Off;
		uint8_t ctrlBot1;
		uint8_t ctrlBot2 ;
		uint8_t ctrlBot3;
		uint8_t ctrlMode; //
		uint8_t sysTail;
	    };

}appPackagestruct_t;




appPackagestruct_t appDataRx;
//sensorDataStruct_t sensorData;
driverDataStruct_t driverDataRx;

void jsonTxMessageUpdate(){
	String msg;
	//root["humi"] = 75.0; //(%)
	//root["temp"] = 30.5; // (C Degree
	/*root["PH+-"] =   is;//  (+/-)
	root["ctrBoardCmd"]= 15;
	root.printTo(jsonTxMsg, 200);
	root.end();
	is++;
	*/


	root["stt"]  = driverDataRx.sst ;
	root["humi"] = driverDataRx.humdVal ;
	root["temp"] = driverDataRx.tempVal;
	root["ec"]   = driverDataRx.ecVal;
	root["ppm"]	 = driverDataRx.ppmVal;
	root["PH+-"] =  driverDataRx.PHVal;
	root["ldr"]  = driverDataRx.ldrVal;
	root["rel1"] = driverDataRx.relay1Stt;
	root["rel2"] = driverDataRx.relay2Stt;
	root["rel3"] = driverDataRx.relay3Stt;
	root["rel4"] = driverDataRx.relay4Stt;
	root["wLv"]  = driverDataRx.wLevel;
	root.printTo(jsonTxMsg, 200);
	root.end();	 //root.operator [](MqttMes);
}







void setup() {
  //pinMode(BUILTIN_LED, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
  Serial.begin(115200);
#ifdef DEBUG_USART
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
#endif
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  //client.unsubscribe(pubTopic);
  for(int i = 0; i<19; i++){
	  driverDataRx.bufferDrvRx[i] = 0;
  }
  //dataTransfer.ctrData[2] = 5;
  //dataTransfer.ctrData[0] = 4;
  //dataTransfer.ctrData[1] = 3;
 // root["sensor"] = "Humidity";
 // root["time"] = 14378434;
  //data.add(48.756080, 6);  // 6 is the number of decimals to print
 // data.add(2.302038, 6);   // if not specified, 2 digits are printed
 // root.printTo(Serial);
  jsonBuffer.alloc(300);

}


void wifiConnect(void){
	WiFi.begin(ssid, password);
	for(int i = 0; i <=10; i++ ){
		if(WiFi.status() != WL_CONNECTED){
			delay(500);
			cntStatus = 0; //Can not connect to WiFi (Internet)
		#ifdef DEBUG_USART
		 Serial.print(".");
		#endif
		}else{
			cntStatus = 1; // Connected to Internet
			#ifdef DEBUG_USART
			  Serial.println("");
			  Serial.println("WiFi connected");
			  Serial.println("IP address: ");
			  Serial.println(WiFi.localIP());
			#endif

		}
	}
}
void setup_wifi() {
  // We start by connecting to a WiFi network

  WiFi.softAP(ssid_ap, pass_ap);
  wifiConnect();
  if(cntStatus == 0){
	  espServer.begin();
	  delay(100);
	 // espClient = espServer.available();
	  delay(500);
	  if(espServer.available()){
	  espClient = espServer.available();
	  delay(10);
	  espClient.println("HTTP/1.1 200 OK");
	  espClient.println("Content-Type: text/html");
	  espClient.println(""); //  do not forget ths one
	  espClient.println("<!DOCTYPE HTML>");
  	  }
  }
 }


void httpReconnect(void){
	espServer.begin();
	delay(100);
	if(espServer.available()){
		espClient = espServer.available();
		espClient.println("HTTP/1.1 200 OK");
			  espClient.println("Content-Type: text/html");
			  espClient.println(""); //  do not forget ths one
			  espClient.println("<!DOCTYPE HTML>");
		cntStatus = 3; // connect to http local

	}
}

void mqttReconnect() {
#ifdef DEBUG_USART_MQTT_REC
	Serial.println("Attempting MQTT connection...");
#endif
	//client.setServer(mqtt_serve                                                                                                                            r, 1883);
	/*while (!client.connected()) {
	   // Serial.print("Attempting MQTT connection...");
	    // Attempt to connect
	    if (client.connect("ESPClient")) {
	     // Serial.println("connected");
	      // Once connected, publish an announcement...
	     // client.publish("outTopic", "hello world");
	      // ... and resubscribe
	      client.subscribe("d1");
	    } else {
	     // Serial.print("failed, rc=");
	     // Serial.print(client.state());
	      //Serial.println(" try again in 5 seconds");
	      // Wait 5 seconds before retrying
	      delay(5000);
	    }

	  }*/


}

void tryToConnect(void){
	while(!espClient.available() || !client.connected()){
		for(int i = 0; i< 10; i++){
			mqttReconnect();
			delay(10);
			if(cntStatus == 0) httpReconnect();
		}

	}
}

void callback(char* topic, byte* payload, unsigned int length) {

   char* jsonRx = (char*)payload;

  StaticJsonBuffer<512> jsonRxBuffer;
  JsonObject& rootRx = jsonRxBuffer.parseObject(jsonRx);


  appDataRx.time1Bot1On = rootRx["time1Bot1On"];
  appDataRx.time1Bot2On = rootRx["time1Bot2On"];
  appDataRx.time1Bot3On = rootRx["time1Bot3On"];
  appDataRx.time2Bot1On = rootRx["time2Bot1On"];
  appDataRx.time2Bot2On = rootRx["time2Bot2On"];
  appDataRx.time2Bot3On = rootRx["time2Bot3On"];
  appDataRx.time3Bot1On = rootRx["time3Bot1On"];
  appDataRx.time3Bot2On = rootRx["time3Bot2On"];
  appDataRx.time3Bot3On = rootRx["time3Bot3On"];

  appDataRx.time1Bot1Off = rootRx["time1Bot1Off"];
  appDataRx.time1Bot2Off = rootRx["time1Bot2Off"];
  appDataRx.time1Bot3Off = rootRx["time1Bot3Off"];
  appDataRx.time2Bot1Off = rootRx["time2Bot1Off"];
  appDataRx.time2Bot2Off = rootRx["time2Bot2Off"];
  appDataRx.time2Bot3Off = rootRx["time2Bot3Off"];
  appDataRx.time3Bot1Off = rootRx["time3Bot1Off"];
  appDataRx.time3Bot2Off = rootRx["time3Bot2Off"];
  appDataRx.time3Bot3Off = rootRx["time3Bot3Off"];

  appDataRx.ctrlBot1  = rootRx["ctrlBot1"];
  appDataRx.ctrlBot2  = rootRx["ctrlBot2"];
  appDataRx.ctrlBot3  = rootRx["ctrlBot3"];
  appDataRx.ctrlMode  = rootRx["ctrlMode"];
  appDataRx.sysTail  = CHAR_TAIL_TX ;

#ifdef DEBUG_USART_Tx
  Serial.print("time3Bot1On:  ");
  Serial.println(appDataRx.time3Bot1On);
  Serial.print("time3Bot2On:  ");
  Serial.println(appDataRx.time3Bot2On);
  Serial.print("time3Bot3On:  ");
  Serial.println(appDataRx.time3Bot3On);
  Serial.print("time3Bot1Off:  ");
  Serial.println(appDataRx.time3Bot1Off);
  Serial.print("time3Bot2Off:  ");
  Serial.println(appDataRx.time3Bot2Off);
  Serial.print("time3Bot3Off:  ");
  Serial.println(appDataRx.time3Bot3Off);
#endif

 uint16_t bien = (uint16_t)appDataRx.ctrAppData[0]+((uint16_t)appDataRx.ctrAppData[1]<<8);
#ifdef MQTT_RX
  Serial.print("Control Mode:............");
  Serial.println(appDataRx.ctrlMode);
#endif


 for(int i = 0; i <SERIAL_TX_BUFFER_SIZE; i++){

  Serial.print(appDataRx.ctrAppData[i]);
 }
 Serial.flush();
}


void mqttProcess(void){
	int mqttRecCnt = 0;
	#ifdef DEBUG
		Serial.println(" MQTT mode runing");
	#endif
 	/* while (!client.connected()) {
	  mqttReconnect();
	  delay(100);
	  mqttRecCnt ++;
	  if(mqttRecCnt >= 100){
		  mqttRecCnt = 0;
		  cntStatus = 0; // reconnect wifi
		  return;
	  }
  }*/

	client.loop();
	long now = millis();
	if (now - lastMsg > 2000) {
		lastMsg = now;
		++value;
		#ifdef DEBUG_MQTT_PRO
			Serial.println("Json Message send:.....................................................");
			Serial.println(jsonTxMsg);
		#endif
		// snprintf (msg, 75, "Bang Nguyen #%ld", value);
		client.publish(pubTopic, jsonTxMsg);
	}
}
void httpLocalProcess(void){
#ifdef DEBUG
	Serial.println("Local mode Runing");
#endif
	int httpWaitTime = 0;
	while(!espClient.available()){
		httpWaitTime++;
		delay(200);
		 espClient = espServer.available();
		 espClient.println("HTTP/1.1 200 OK");
		 espClient.println("Content-Type: text/html");
		 espClient.println(""); //  do not forget ths one
		#ifdef DEBUG
			Serial.println("The Json message send");
			Serial.println("jsonMsg");
		#endif

		 espClient.println(jsonTxMsg);
		if(httpWaitTime >100) {
			cntStatus = 0;
			return;
		}
	}
	 httpWaitTime = 0;


	 delay(200);
	 //espServer.send(200, "application/json", "{\"IP\":\"" + MqttMes + "\"}");
	 // Read the first line of the request
	 String request = espClient.readStringUntil('\r');
	 //Serial.println(request);
	 String request1 = espClient.readStringUntil('\r');
	 //Serial.println(request1);
	 String request2 = espClient.readStringUntil('\r');
	 //Serial.println(request2);
	 String request3 = espClient.readStringUntil('\r');
	// Serial.println(request3);
	 String request4 = espClient.readStringUntil('\r');
	// Serial.println(request4);
	 String request5 = espClient.readStringUntil('\r');
	 //Serial.println(request5);

	 String request6 = espClient.readStringUntil('\r');
	 //Serial.println(request6);
	 String request7 = espClient.readStringUntil('\r');
	 //Serial.println(request7);
	 httpMsgPayload = espClient.readStringUntil('\r');
	 Serial.println(httpMsgPayload);
	// String request9 = espClient.readStringUntil('\r');
	 //Serial.println(request9);
	 char* jsonRx = (char*)httpMsgPayload.c_str();
	 Serial.print((const char* )jsonRx);
	 StaticJsonBuffer<500> jsonRxBufferHttp;
	 JsonObject& rootRxHttp = jsonRxBufferHttp.parseObject(jsonRx);
	 appDataRx.time1Bot1On = rootRxHttp["time1Bot1On"];
	 appDataRx.time1Bot2On = rootRxHttp["time1Bot2On"];
	 appDataRx.time1Bot3On = rootRxHttp["time1Bot3On"];
	 appDataRx.time2Bot1On = rootRxHttp["time2Bot1On"];
	 appDataRx.time2Bot2On = rootRxHttp["time2Bot2On"];
	 appDataRx.time2Bot3On = rootRxHttp["time2Bot3On"];
	 appDataRx.time3Bot1On = rootRxHttp["time3Bot1On"];
	 appDataRx.time3Bot2On = rootRxHttp["time3Bot2On"];
	 appDataRx.time3Bot3On = rootRxHttp["time3Bot3On"];
	 appDataRx.time1Bot1Off = rootRxHttp["time1Bot1Off"];
	 appDataRx.time1Bot2Off = rootRxHttp["time1Bot2Off"];
	 appDataRx.time1Bot3Off = rootRxHttp["time1Bot3Off"];
	 appDataRx.time2Bot1Off = rootRxHttp["time2Bot1Off"];
	 appDataRx.time2Bot2Off = rootRxHttp["time2Bot2Off"];
	 appDataRx.time2Bot3Off = rootRxHttp["time2Bot3Off"];
	 appDataRx.time3Bot1Off = rootRxHttp["time3Bot1Off"];
	 appDataRx.time3Bot2Off = rootRxHttp["time3Bot2Off"];
	 appDataRx.time3Bot3Off = rootRxHttp["time3Bot3Off"];
	 appDataRx.ctrlBot1  = rootRxHttp["ctrlBot1"];
	 appDataRx.ctrlBot2  = rootRxHttp["ctrlBot2"];
	 appDataRx.ctrlBot3  = rootRxHttp["ctrlBot3"];
	 appDataRx.ctrlMode  = rootRxHttp["ctrlMode"];
	 appDataRx.sysTail  = CHAR_TAIL_TX ;
	 for(int i = 0; i <SERIAL_TX_BUFFER_SIZE; i++){                   }
	 Serial.flush();
	}


//
void readSerial() // baud = 115200
{
	while(Serial.available()){
	Serial.readBytesUntil(CHAR_TAIL_RX,driverDataRx.bufferDrvRx, SERIAL_RX_BUFEER_SIZE);

	}
			/*if(driverDataRx.sysn != 0b10101010){
				seriCnt = 0;
				Serial.readBytes()
			}*/
/*	while(Serial.available()){
	driverDataRx.bufferDrvRx[seriCnt]= Serial.read();
#ifdef USART_CHECK

		Serial.print("sysn:");
		Serial.println(driverDataRx.sysn);
		Serial.print("byte[0]:");
		Serial.println(driverDataRx.bufferDrvRx[0]);
		Serial.println(".........");
		Serial.print("Count : ");
		Serial.println(seriCnt);

		Serial.print(driverDataRx.bufferDrvRx[seriCnt]);
#endif

		seriCnt++;

		if(seriCnt >= SERIAL_RX_BUFEER_SIZE){

			seriCnt = 0 ;
			//Serial.println("...................................");
			#ifdef DEBUG_USART
				Serial.println("ESP Usart Data Rx Packet:");
				Serial.println(driverDataRx.sst);
				//delay(100);
				Serial.println(driverDataRx.tempVal);
				//delay(100);
				Serial.println(driverDataRx.humdVal);
				//delay(100);
				Serial.println(driverDataRx.PHVal);
				//delay(100);
				Serial.println(driverDataRx.ecVal);
				//delay(100);
				Serial.println(driverDataRx.ppmVal);
				//delay(100);
				Serial.println(driverDataRx.ldrVal);
				//delay(100);
				Serial.println(driverDataRx.relay1Stt);
				//delay(100);
				Serial.println(driverDataRx.relay2Stt);
				//delay(100);
				Serial.println(driverDataRx.relay3Stt);
				//delay(100);
				Serial.println(driverDataRx.relay4Stt);
				//delay(100);
				Serial.println("ESP Usart Rx completely!");
			#endif
		}
#ifdef USART_CHECK
	Serial.println("Serial Reading.........................");
#endif

	}
*/
	/*if(driverDataRx.sysn != 0b10101010) {
						seriCnt =  0;
			}*/

	//Serial.readBytes(driverDataRx.bufferDrvRx, 19);
}

void reInit(void){
	client.setServer(mqtt_server, 1883);
				  client.setCallback(callback);
				  setup_wifi();
				//  lastWiFiConnect = nowWiFiConnect ;
}

void loop() {
	readSerial();

	jsonTxMessageUpdate();

#ifdef DEBUG_SYSN

	Serial.print("Sysn .............................:");
	Serial.println(driverDataRx.sysn);

#endif
	/*if(driverDataRx.sysn != 0xAA){
			//Serial.end();
			//Serial.begin(115200);
		ESP.reset();
	}
*/
	static int mqttConnectTime = 0;
	//update
	if( cntStatus == 0){
	  //setup_wifi();
		seriCnt = 0;  /// reset the

		httpReconnect();
	#ifdef DEBUG
		Serial.println("http reconnect...");
	#endif
	}else if(cntStatus == 3){
		httpLocalProcess();
		#ifdef DEBUG
		Serial.println("httpprocessing...");
		#endif

	}else{

		// cntStatus == 1, it 's mean connected to WiFi

		if(!client.connected()){
			mqttReconnect();

			if(client.connect("ESPClient3", userName, userPassword)){
				client.subscribe("d1");
				#ifdef MQTT_DEBUG_CONNECTION
				Serial.println("MQTT subscribe ...");
				#endif
				mqttConnectTime = 0;
			}else{
				#ifdef MQTT_DEBUG_CONNECTION
				Serial.println("MQTT Lost Connection and delay ...");
				#endif
				delay(100);
				mqttConnectTime++;
			}
			delay(200);
			if(mqttConnectTime >=200){
				cntStatus = 0; // reconnect
				#ifdef MQTT_DEBUG_CONNECTION
				Serial.println("MQTT Lost Connection ...");
				#endif
			}
		}else{
				#ifdef MQTT_DEBUG_CONNECTION
				//Serial.println("MQTT processing ........");
				#endif
		mqttProcess();
		#ifdef DEBUG
		//Serial.println("mqtt processing...");
		#endif
		mqttConnectTime = 0;
		}
	}

	long nowWiFiConnect = millis();

	if(cntStatus==0){
		if(nowWiFiConnect - lastWiFiConnect >= 20000){

			  client.setServer(mqtt_server, 1883);
			  client.setCallback(callback);
			  setup_wifi();
			  lastWiFiConnect = millis(); ;
	  	 }
	}else{
		  lastWiFiConnect = nowWiFiConnect ;
		  //String abc = espClient.readStringUntil('\r\r');
		  //String request = espClient.readStringUntil('\r');
		  //String request = espClient.readString();
		  //espClient.
		  //Serial.println(request);
		//  server.handleClient();
	}

}

//https://nodemcu.readthedocs.io/en/dev/en/modules/mqtt/#mqttclientconnect


//http://www.ibm.com/developerworks/cloud/library/cl-mqtt-bluemix-iot-node-red-app/

