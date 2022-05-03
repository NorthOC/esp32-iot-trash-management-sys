#include "Secrets.h"
#include <WiFiClientSecure.h>
#include <MQTTClient.h>
#include <ArduinoJson.h>
#include <SPI.h>
#include <MFRC522.h>
#include "WiFi.h"

#define RST_PIN         2          // Configurable, see typical pin layout above
#define SS_PIN          21         // Configurable, see typical pin layout above

MFRC522 mfrc522(SS_PIN, RST_PIN);  // Create MFRC522 instance

//70 F5 F5 32 -card
//39 EF E6 B8 -chip
 
#define AWS_IOT_PUBLISH_TOPIC   "esp32/pub"
#define AWS_IOT_SUBSCRIBE_TOPIC "esp32/sub"

int current_queue = 0;
int n = 2;
int total[2] = {0, 0};
String name1 = "Denis";
String id1 = "70 F5 F5 32";
String name2 = "Rosita";
String id2 = "39 EF E6 B8";
String current_user = id1;
 
WiFiClientSecure wifi_client = WiFiClientSecure();
MQTTClient mqtt_client = MQTTClient(256);
const char* ssid = "HH40V_466B";
const char* password = "82860669";

void connectAWS()
{
  // Configure wifi_client with the correct certificates and keys
  wifi_client.setCACert(AWS_CERT_CA);
  wifi_client.setCertificate(AWS_CERT_CRT);
  wifi_client.setPrivateKey(AWS_CERT_PRIVATE);

  //Connect to AWS IOT Broker. 8883 is the port used for MQTT
  mqtt_client.begin(AWS_IOT_ENDPOINT, 8883, wifi_client);

  //Set action to be taken on incoming messages
  mqtt_client.onMessage(incomingMessageHandler);

  Serial.print("Connecting to AWS IOT...");

  //Wait for connection to AWS IoT
  while (!mqtt_client.connect(THINGNAME)) {
    Serial.print(".");
    delay(100);
  }
  Serial.println();

  if(!mqtt_client.connected()){
    Serial.println("AWS IoT Timeout!");
    return;
  }

  //Subscribe to a topic
  mqtt_client.subscribe(AWS_IOT_SUBSCRIBE_TOPIC);

  Serial.println("AWS IoT Connected!");
}
 
void publishMessage(String nam, int current, int total)
{
  StaticJsonDocument<200> doc;
  doc["name"] = nam;
  doc["current_queue"] = current;
  doc["lifetime"] = total;
  char jsonBuffer[512];
  serializeJson(doc, jsonBuffer); // print to client
 
  mqtt_client.publish(AWS_IOT_PUBLISH_TOPIC, jsonBuffer);
  Serial.print("Data sent over to ");
  Serial.println(AWS_IOT_PUBLISH_TOPIC);
  Serial.println();
}
 
void incomingMessageHandler(String &topic, String &payload) {
  Serial.println("Message received!");
  Serial.println("Topic: " + topic);
  Serial.println("Payload: " + payload);
}
 
void setup()
{
  Serial.begin(115200);
  
    //Begin WiFi in station mode
  WiFi.mode(WIFI_STA); 
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.println("Connecting to Wi-Fi");
  //Wait for WiFi connection
  while (WiFi.status() != WL_CONNECTED){
    delay(500);
    Serial.print(".");
  }
  
  Serial.println();
  SPI.begin();      // Initiate  SPI bus
  mfrc522.PCD_Init();   // Initiate MFRC522
  Serial.println();
  Serial.println("Currently, it is Deni's turn to take out the trash");
  Serial.println("Please, scan your card:");
  Serial.println();
}

void loop()
{
  // Look for new cards
  if ( ! mfrc522.PICC_IsNewCardPresent()) 
  {
    return;
  }
  // Select one of the cards
  if ( ! mfrc522.PICC_ReadCardSerial()) 
  {
    return;
  }
 
  Serial.print("UID tag :");
  String content= "";
  byte letter;
  for (byte i = 0; i < mfrc522.uid.size; i++) 
  {
     Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
     Serial.print(mfrc522.uid.uidByte[i], HEX);
     content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
     content.concat(String(mfrc522.uid.uidByte[i], HEX));
  }
  Serial.println();
  Serial.print("Message : ");
  content.toUpperCase();

  if (current_user ==  id1 && content.substring(1) == id1)
  {
    current_queue += 1;
    total[0] += 1;
    Serial.println("Denis took the trash out");
    Serial.println("Sending info over to AWS...");
    connectAWS();
    Serial.println();
    publishMessage(name1, current_queue, total[0]);
    if (current_queue == 3)
    {
      current_queue = 0;
      current_user = id2;
      Serial.println("Denis has taken out 3 trashbags. Now it is Rosita's turn");
    }
  }

  else if (current_user == id2 && content.substring(1) == id2)
  {
    current_queue += 1;
    total[1] += 1;
    Serial.println("Rosita took the trash out");
    Serial.println("Sending info over to AWS...");
    connectAWS();
    Serial.println();
    publishMessage(name2, current_queue, total[1]);
    if (current_queue == 3)
    {
      current_queue = 0;
      current_user = id1;
      Serial.println("Rosita has taken out 3 trashbags. Now it is Denis's turn");
    }
  }
 
  else   
  {
    Serial.println("Person not recognised");
    Serial.print("Currently it is ");
    if (current_user == id1)
    {
      Serial.print(name1);
    }
    else if (current_user == id2)
    {
      Serial.print(name2);
    }
    Serial.println(" who has to take the trash out.");
    Serial.println();
    delay(5000);
  }
  delay(4000);
}
