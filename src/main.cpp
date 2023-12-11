//=====[Libraries]=============================================================
#include <Arduino.h>
#include <WiFi.h>
#include "PubSubClient.h"
#include "ascon/crypto_aead.h"
#include "ascon/api.h"
#include <stdio.h>

//=====[Declaration of private defines]========================================
#define CRYPTO_BYTES 64

//=====[Declaration and initialization of private global variables]============
//=====[ASCON]====================================
static unsigned long long mlen;
static unsigned long long clen;

static unsigned char plaintextDecrypt[CRYPTO_BYTES];
static unsigned char cipher[CRYPTO_BYTES]; 
static unsigned char npub[CRYPTO_NPUBBYTES]="";
static unsigned char ad[CRYPTO_ABYTES]="";
static unsigned char nsec[CRYPTO_ABYTES]="";

static unsigned char key[CRYPTO_KEYBYTES];
static char chex[CRYPTO_BYTES]="";
static char keyhex[2*CRYPTO_KEYBYTES+1]="0123456789ABCDEF0123456789ABCDEF";
static char nonce[2*CRYPTO_NPUBBYTES+1]="00000000000000001111111111111111";
static char add[CRYPTO_ABYTES]="";

//=====[WIFI AND MQTT BROKER]==================================== 
const char *ssid = "Fibertel WiFi367 2.4GHz"; // Nombre WiFi
const char *password = "0141200866";  // Contraseña del WiFi
//const char *ssid = "Moto G (5) Plus 6864";
//const char *password = "manuel123";

const char *mqtt_broker = "192.168.0.248"; // MQTT Broker
//const char *mqtt_broker = "192.168.255.99";
const char *topic = "mosquitto/esp32";  //MOSQUITTO Topic
const int mqtt_port = 1883; //MQTT Port
static char stringToSendToMosquitto[500];

//=====[AUXILIAR ASCON]===========================================
const char defaultNonce[2*CRYPTO_NPUBBYTES+1]="00000000000000001111111111111111";

//=====[AUXILIAR VARIABLES]=======================================
static char strAux[100];

//=====[Declaration and initialization of public global objects]===============
WiFiClient espClient;
PubSubClient client(espClient);

//=====[Declarations (prototypes) of functions]========================
void callback(char *topic, byte *payload, unsigned int length);

void setup() {
    // Se setea el baudrate a 9600;
    Serial.begin(9600);     
    // Conexion con la red WiFi
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.println("Connecting to WiFi network..");
    }
    
    Serial.println("Connected to WiFi network");
    // Conexión con el broker MQTT
    client.setServer(mqtt_broker, mqtt_port);
    client.setCallback(callback);
    
    while (!client.connected()) {
	      String client_id = "esp32-client-";
        client_id += String(WiFi.macAddress());
        Serial.printf("The client %s is connecting to broker MQTT\n", client_id.c_str());
        if (client.connect(client_id.c_str())) {
            Serial.println("Broker Mosquitto MQTT connected");
        } 
        else {
            Serial.print("failed with state ");
            Serial.print(client.state());
            delay(2000);
        }
    }
    
  // Suscripción al topico de mqtt
    client.subscribe(topic);
}

void loop() {
    client.loop();
    delay(1000);
}


void callback(char *topic, byte *payload, unsigned int length) {
    String encryptedTemp;
    Serial.print("Message arrived in topic: ");
    Serial.println(topic);
    Serial.print("Message:");
    for(int i = 0; i < length; i++) {
        Serial.print((char) payload[i]);
        encryptedTemp += (char)payload[i];
    }
    if(!encryptedTemp.compareTo("1")) {
        Serial.print("Temperature reading initialized\n");
        strcpy(nonce, defaultNonce);
    }
    else {
        strcpy(chex, encryptedTemp.c_str());
        clen = strlen(chex)-1;
        hexString2string(chex, clen, cipher);
        hextobyte(keyhex,key);
        hextobyte(nonce,npub);
        crypto_aead_decrypt(plaintextDecrypt,&mlen,nsec,cipher,clen,ad,strlen(ad),npub,key);
        plaintextDecrypt[mlen]='\0';
        sprintf(strAux, "\nMessage decrypted: %s\n", plaintextDecrypt);
        Serial.print(strAux);
        Serial.println();
        Serial.println("-----------------------");
        for(int i = 0; i < 2*CRYPTO_NPUBBYTES + 1; i++) {
            nonce[i] = plaintextDecrypt[i+23];
        }
        nonce[2*CRYPTO_NPUBBYTES] = '\0';
    }
} 