
// ESP32 montagem
// Vcc <-> 3V3 (ou Vin(5V), dependendo da versão do módulo)
// RST (Reset) <-> D0
// GND (Masse) <-> GND
// MISO (Master Input Slave Output) <-> 19
// MOSI (Master Output Slave Input) <-> 23
// SCK (Serial Clock) <-> 18
// SS/SDA (Slave select) <-> 5


// // LED PIN MODE
// #define pinVerde 15
// #define pinVermelho 2
// #define sensorSobrepor 17
// #define tranca 21


//Libraries
#include <SPI.h>      //https://www.arduino.cc/en/reference/SPI
#include <MFRC522.h>  //https://github.com/miguelbalboa/rfid
#include <stdio.h>
#include <string.h>
#include <ctype.h>  // Para toupper()
#include <WiFi.h>
#include <HTTPClient.h>
#include <NewPing.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>  // Inclui a biblioteca PubSubClient para a comunicação MQTT



//Constants
#define SS_PIN 5
#define RST_PIN 0

// LED PIN MODE
#define pinVerde 15
#define pinVermelho 2
#define sensorSobrepor 17
#define tranca 21

byte ping = 0;


const char* ssid = "FIESC_IOT";       // Nome da rede Wi-Fi (SSID)
const char* password = "C6qnM4ag81";  // Senha da rede Wi-Fi

// Substitua pelo seu token do TagoIo
const char* deviceToken = "d7fc3690-68de-4021-85c7-1c8084d92e45";


const char* mqtt_server = "mqtt.tago.io";                            // Endereço do servidor MQTT
const char* mqtt_topic = "rfid/command";                             // Tópico MQTT ao qual o ESP32 se inscreve
const char* mqtt_username = "token";                                 // Nome de usuário para a conexão MQTT (neste caso, tipo de autenticação)
const char* mqtt_password = "d7fc3690-68de-4021-85c7-1c8084d92e45";  // Token de dispositivo para autenticação no servidor MQTT

const char* mqtt_statusTopic = "status/command";                             // Tópico MQTT ao qual o ESP32 se inscreve

WiFiClient espClient;            // Cria um cliente WiFi para o ESP32
PubSubClient client(espClient);  // Cria um cliente MQTT passando o cliente WiFi




//Parameters
const int ipaddress[4] = { 103, 97, 67, 25 };

//Variables
byte nuidPICC[4] = { 0, 0, 0, 0 };
MFRC522::MIFARE_Key key;
MFRC522 rfid = MFRC522(SS_PIN, RST_PIN);
MFRC522::StatusCode status;
byte bloco = 1;

  int contagem = 0;

void setup_wifi() {
     
     //  Inicializa a conexão WiFi

    Serial.print("Conectando ao WiFi...");
    WiFi.begin(ssid, password); // Conecta ao WiFi usando o SSID e senha fornecidos

    while (WiFi.status() != WL_CONNECTED) { // Espera até que a conexão seja estabelecida
        delay(500);
        Serial.print(".");
    }

    Serial.println("\nWiFi conectado");
    Serial.println("Endereço IP: " + WiFi.localIP().toString()); // Exibe o endereço IP do ESP32

 

}


void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Mensagem recebida, tópico: ");
  Serial.print(topic);
  Serial.print(". Mensagem: ");

  String message;
  for (unsigned int i = 0; i < length; i++) {  // Monta a string da mensagem recebida
    message += (char)payload[i];
  }
  Serial.println(message);

  StaticJsonDocument<256> doc;          // Cria um documento JSON para armazenar e analisar a mensagem
  deserializeJson(doc, message);        // Deserializa a mensagem JSON
  int fechadura = doc["tranca"];  // Extrai o valor da frequência

  if (fechadura == 1) {

    digitalWrite(pinVerde, HIGH);
    digitalWrite(tranca, LOW);
    delay(3000);  // Mantém o buzzer ativo por 1 segundo
    digitalWrite(tranca, HIGH);
     digitalWrite(pinVerde, LOW);
  }

  bool statusMostrar = doc["mostrarStatus"];

  if (statusMostrar){
    // Serial.println("Entrou no if");
    sendStatusSobrepor();
  }
}

void reconnect() {
  while (!client.connected()) {  // Tenta reconectar se a conexão for perdida
    Serial.print("Conectando ao MQTT...");
    if (client.connect("ESP32Client", mqtt_username, mqtt_password)) {  // Conecta ao servidor MQTT com autenticação
      Serial.println("conectado");
      client.subscribe(mqtt_topic);  // Se conectado, inscreve-se no tópico MQTT
      client.subscribe(mqtt_statusTopic);  // Se conectado, inscreve-se no tópico MQTT
    } else {
      Serial.print("falha, rc=");
      Serial.println(client.state());  // Exibe o código de erro da conexão
      delay(5000);                     // Espera 5 segundos antes de tentar novamente
    }
  }
}


void setup() {
  //Init Serial USB
  Serial.begin(115200);
  Serial.println(F("Initialize System"));
  //init rfid D8,D5,D6,D7
  SPI.begin();
  rfid.PCD_Init();

  Serial.print(F("Reader :"));
  rfid.PCD_DumpVersionToSerial();


  pinMode(pinVermelho, OUTPUT);  // Define o pino do LED vermelho como saída
  pinMode(pinVerde, OUTPUT);     // Define o pino do LED verde como saída
  pinMode(sensorSobrepor, INPUT);
  pinMode(tranca, OUTPUT);
  digitalWrite(tranca, HIGH);

  setup_wifi();                         // Configura e conecta ao WiFi
  client.setServer(mqtt_server, 1883);  // Define o servidor MQTT e a porta
  client.setCallback(callback);         // Define a função de callback para mensagens MQTT
 
 digitalWrite(pinVermelho, 1);
 delay(1000);
digitalWrite(pinVermelho, 0);
}



// Definindo a estrutura do objeto
struct CartaoRFID {
  char cartaoID[20];
  char nome[100];  // Supondo que o nome tenha no máximo 100 caracteres
};
unsigned int hexRFID;

char stringRFID[100];
int sensorSobreporEstado;
char statusSobrepor[40] = "Fechada";
char statusSobreporTago[40] = "Fechada";




struct CartaoRFID meuCartao;
void addCartaoDados() {
  // Criando uma variável do tipo CartaoID


  // Preenchendo os dados do cartão
  strcpy(meuCartao.cartaoID, "A9 84 F0 97");
  strcpy(meuCartao.nome, "Fulano de Tal");

  // unsigned int hex;
  // sscanf(meuCartao.cartaoID, "%X", &hex);


  // Exibindo os dados do cartão
  // printf("ID do Cartao: %s \n", meuCartao.cartaoID);
  // printf("Nome: %s\n", meuCartao.nome);
}




void loop() {
 

  lerRFID();

  if (!client.connected()) { // Se a conexão MQTT for perdida, tenta reconectar
        reconnect();
    }
    client.loop(); // Processa mensagens novas e mantém a conexão MQTT viva

  // ping++;
  // Serial.println(ping);
  // delay(333);
}
// ------------  Sensor Sobrepor   ---------------

void sendStatusSobrepor() {

  Serial.println("ENTROU SOBREPOR");
  sensorSobreporEstado = digitalRead(sensorSobrepor);
  
  if (sensorSobreporEstado == 1) {

    strcpy(statusSobrepor, "Aberta");
    Serial.println(statusSobrepor);
  } else {
    strcpy(statusSobrepor, "Fechada");
        Serial.println(statusSobrepor);

  }

    // String jsonSobrepor = "{\"variable\":\"gaveta_id\",\"value\":1,\"metadata\":{\"nome\":\"" + String(meuCartao.nome) + "\",\"status\":\"" + String(statusSobrepor) + "\",\"tag_id\":\"" + String(meuCartao.cartaoID) + "\"}}";
    String jsonSobrepor = "{\"value\":\"" + String(statusSobrepor) + "\", \"variable\":\"status\", \"metadata\":{ \"gaveta_id\":1 }}";

    sendTagoDados(jsonSobrepor);
  // }
   
}
  //                                                        ---------------- Conexão com o Tago --- Mandando dados

void sendTagoDados( String output) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin("http://api.tago.io/data");
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Device-Token", deviceToken);
    
    int httpResponseCode = http.POST(output);
    Serial.print("Código de resposta HTTP: ");
    Serial.println(httpResponseCode);
    http.end();

    String httpRequestData = "{\"value\":\"" + String(meuCartao.cartaoID) + "\", \"variable\":\"tag_id\", \"metadata\":{\"status\":\"" + String(statusSobrepor) + "\", \"gaveta_id\":1, \"nome\":\"" + meuCartao.nome + "\"}}";


  } else {
    Serial.println("WiFi Desconectado");
  }
}





//                                                     -------------------  Função para converter uma string para maiúsculas -------------------
void strParaMaiusculas(char* str) {
  int i = 0;
  while (str[i]) {
    str[i] = toupper(str[i]);
    i++;
  }

  CompararEPrint(stringRFID);
}

//                                                         -------------------  Função para comparar RFIDs e acender LED  -------------------
void CompararEPrint(char* str) {
  addCartaoDados();

  sensorSobreporEstado = digitalRead(sensorSobrepor);
  if (sensorSobreporEstado == 1) {

    strcpy(statusSobrepor, "Aberta");

  } else {
    strcpy(statusSobrepor, "Fechada");
  }
 
  if (strcmp(stringRFID, meuCartao.cartaoID) == 0) {
    // Serial.println({ CartaoID: meuCartao.cartaoID, Nome: meuCartao.nome });

    Serial.println("SUCESSO");
    digitalWrite(tranca, LOW);
    digitalWrite(pinVerde, HIGH);
    String httpRequestData = "{\"variable\":\"gaveta_id\",\"value\":1,\"metadata\":{\"nome\":\"" + String(meuCartao.nome) + "\",\"status\":\"" + String(statusSobrepor) + "\",\"tag_id\":\"" + String(meuCartao.cartaoID) + "\"}}";
    sendTagoDados(httpRequestData);

    delay(5500);
    digitalWrite(pinVerde, LOW);
    digitalWrite(tranca, HIGH);
    digitalWrite(sensorSobrepor, LOW);
  }

  else {
 
    Serial.println("Acesso negado.");
    Serial.println(String(stringRFID));
    digitalWrite(pinVermelho, HIGH);
    delay(2500);
    digitalWrite(pinVermelho, LOW);

    String httpRequestData = "{\"variable\":\"gaveta_id\",\"value\":1,\"metadata\":{\"tag_id\":\"" + String(stringRFID) + "\"}}";

    // String httpRequestData = "{\"value\":\"" + String(stringRFID) + "\", \"variable\":\"tag_id\", \"metadata\":{\"status\":\"" + String(statusSobrepor) + "\", \"gaveta_id\":1}}";
    // sendTagoDados(httpRequestData);
    sendTagoDados(httpRequestData);
  }


}






//                                                                 -------------------- LER DADOS DO CARTAO RFID -----------
void lerRFID(void) { /* function readRFID */
  ////Read RFID card

  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }
  // Look for new 1 cards
  if (!rfid.PICC_IsNewCardPresent())
    return;

  // Verify if the NUID has been readed
  if (!rfid.PICC_ReadCardSerial())
    return;

  // Store NUID into nuidPICC array
  for (byte i = 0; i < 4; i++) {
    nuidPICC[i] = rfid.uid.uidByte[i];
  }
  status = rfid.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, bloco, &key, &(rfid.uid));  //line 834 of MFRC522.cpp file

 if (status != MFRC522::STATUS_OK){
  Serial.println("Erro de autenticação no ler dados");

 }
  // Serial.println(rfid.GetStatusCodeName(status));

  Serial.println(("RFID In Hex: "));
  printHex(rfid.uid.uidByte, rfid.uid.size);
  Serial.println();

  // Halt PICC
  rfid.PICC_HaltA();

  // Stop encryption on PCD
  rfid.PCD_StopCrypto1();
}



//                                                 --------------------- Transformando para Hexadecimal ---------------------------
/**
   Helper routine to dump a byte array as hex values to Serial.
 
*/
void printHex(byte* buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], HEX);
  }
    printf("Oi rfid");
  sprintf(stringRFID, "%x %x %x %x", buffer[0], buffer[1], buffer[2], buffer[3]);

  strParaMaiusculas(stringRFID);
}
