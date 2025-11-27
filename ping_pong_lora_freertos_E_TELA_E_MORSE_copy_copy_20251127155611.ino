#include "Arduino.h"
#include "LoRaWan_APP.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <Wire.h>  
#include "HT_SSD1306Wire.h"
// #include "heltec.h"

#define LED_GPIO 25

// -----------------------------
// LORAWAN VARIABLES 
// -----------------------------

#define RF_FREQUENCY                865000000 // Hz
#define TX_OUTPUT_POWER             5         // dBm

#define LORA_BANDWIDTH              0
#define LORA_SPREADING_FACTOR       7
#define LORA_CODINGRATE             1
#define LORA_PREAMBLE_LENGTH        8
#define LORA_SYMBOL_TIMEOUT         0
#define LORA_FIX_LENGTH_PAYLOAD_ON  false
#define LORA_IQ_INVERSION_ON        false

#define RX_TIMEOUT_VALUE            10000
#define BUFFER_SIZE                 300

char txpacket[BUFFER_SIZE];
char rxpacket[BUFFER_SIZE];


struct MorseEntry {
  const char* code;
  char letter;
};

MorseEntry morseTable[] = {
  {".-", 'A'},   {"-...", 'B'}, {"-.-.", 'C'}, {"-..", 'D'},
  {".", 'E'},    {"..-.", 'F'}, {"--.", 'G'},  {"....", 'H'},
  {"..", 'I'},   {".---", 'J'}, {"-.-", 'K'},  {".-..", 'L'},
  {"--", 'M'},   {"-.", 'N'},   {"---", 'O'},  {".--.", 'P'},
  {"--.-", 'Q'}, {".-.", 'R'},  {"...", 'S'},  {"-", 'T'},
  {"..-", 'U'},  {"...-", 'V'}, {".--", 'W'},  {"-..-", 'X'},
  {"-.--", 'Y'}, {"--..", 'Z'},
  {"-----", '0'}, {".----", '1'}, {"..---", '2'}, {"...--", '3'},
  {"....-", '4'}, {".....", '5'}, {"-....", '6'}, {"--...", '7'},
  {"---..", '8'}, {"----.", '9'}
};
char decodeMorse(String code) {
  for (int i = 0; i < sizeof(morseTable) / sizeof(MorseEntry); i++) {
    if (code == morseTable[i].code) return morseTable[i].letter;
  }
  return '?';  // unknown sequence
}
static RadioEvents_t RadioEvents;
void OnTxDone(void);
void OnTxTimeout(void);
void OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr);
SSD1306Wire  factory_display(0x3c, 500000, SDA_OLED, SCL_OLED, GEOMETRY_128_64, RST_OLED); // addr , freq , i2c group , resolution , rst

typedef enum
{
  LOWPOWER,
  STATE_RX,
  STATE_TX
} States_t;

int16_t txNumber;
States_t state;
bool sleepMode = false;
String mensagem_enviada = "";

int16_t Rssi, rxSize;
// FreeRTOS task handles
TaskHandle_t blinkTaskHandle = NULL;
TaskHandle_t loraTaskHandle  = NULL;
bool enviar = false;
// ======================================================
// TASK 1: piscar LED
// ======================================================
void BlinkTask(void *pvParameters)
{
const int ponto  = 21;
const int barra = 13;


  pinMode(ponto,  INPUT_PULLUP);
  pinMode(barra, INPUT_PULLUP);
  const unsigned int pausa = 200;  
const unsigned int pausa_letra = 800;   
const unsigned int pausa_palavra   = 5000;  

String input = "";
unsigned long input_tempo = 0;
String mensagem = "";

  pinMode(LED_GPIO, OUTPUT);

  int counter = 0;
  while (true)
  {
    unsigned long now = millis();
  bool dotPressed  = (digitalRead(ponto)  == LOW);
  bool dashPressed = (digitalRead(barra) == LOW);
//   if(dotPressed && dashPressed){
//     mensagem = "";      input = "";

// digitalWrite(LED_GPIO,HIGH);
//     vTaskDelay(200 / portTICK_PERIOD_MS);
// digitalWrite(LED_GPIO,LOW);
//     vTaskDelay(200 / portTICK_PERIOD_MS);
//           Serial.println("(restart)");

//     continue;
//   }
  if (dotPressed) {
    input += '.';
    input_tempo = now;
    Serial.print(".");
    factory_display.clear();
    // Print to the screen
    factory_display.println(input);
    // Draw it to the internal screen buffer
    factory_display.drawLogBuffer(0, 0);
    // Display it on the screen
    factory_display.display();
    vTaskDelay(200 / portTICK_PERIOD_MS);  
  }
  else if (dashPressed) {
    input += '-';
    input_tempo = now;
    Serial.print("-");

      factory_display.clear();
    // Print to the screen
    factory_display.println(input);
    // Draw it to the internal screen buffer
    factory_display.drawLogBuffer(0, 0);
    // Display it on the screen
    factory_display.display();
    vTaskDelay(600 / portTICK_PERIOD_MS);  

  }

  unsigned long sinceLast = now - input_tempo;
if (sinceLast >= pausa_palavra) {input_tempo=now;
      char decoded = decodeMorse(input);
      // Serial.print(" -> ");
      // Serial.print(decoded);
      // Serial.print("   ");
      // Serial.println("(tempo)");
      mensagem += "";
      Serial.println(mensagem);
      input = "";

      mensagem_enviada = "";
      mensagem_enviada=mensagem;

      if(mensagem_enviada.length() > 0){      
        Serial.print("enviando: ");

      Serial.println(mensagem_enviada);
      factory_display.clear();
    // Print to the screen
    factory_display.println("Enviando");
    // Draw it to the internal screen buffer
    factory_display.drawLogBuffer(0, 0);
    // Display it on the screen
    factory_display.display();
    factory_display.clear();
    // Print to the screen
    factory_display.println(mensagem);
    // Draw it to the internal screen buffer
    factory_display.drawLogBuffer(0, 0);
    // Display it on the screen
    factory_display.display();
      enviar = true;
      mensagem = "";
    vTaskDelay(1000 / portTICK_PERIOD_MS);  }
    }
  if (input.length() > 0) {
    if (sinceLast > pausa_letra && sinceLast < pausa_palavra) {
      char decoded = decodeMorse(input);
      Serial.print(" -> ");
      Serial.println(decoded);
      mensagem += decoded;
      Serial.println(mensagem);
      factory_display.clear();
    // Print to the screen
    factory_display.println(mensagem);
    // Draw it to the internal screen buffer
    factory_display.drawLogBuffer(0, 0);
    // Display it on the screen
    factory_display.display();
      input = "";
    }

    
  }
  }
}

// ======================================================
// TASK 2: programa antigo de ping pong
// ======================================================
void LoRaTask(void *pvParameters)
{    

  Serial.println("LoRa Task Started");

  txNumber = 0;
  Rssi = 0;

  factory_display.setLogBuffer(5, 30);
  // Register radio callback handlers
  RadioEvents.TxDone    = OnTxDone;
  RadioEvents.TxTimeout = OnTxTimeout;
  RadioEvents.RxDone    = OnRxDone;

  // Initialize LoRa radio
  Radio.Init(&RadioEvents);
  Radio.SetChannel(RF_FREQUENCY);

  Radio.SetTxConfig(MODEM_LORA, TX_OUTPUT_POWER, 0, LORA_BANDWIDTH,
                    LORA_SPREADING_FACTOR, LORA_CODINGRATE,
                    LORA_PREAMBLE_LENGTH, LORA_FIX_LENGTH_PAYLOAD_ON,
                    true, 0, 0, LORA_IQ_INVERSION_ON, 3000);

  Radio.SetRxConfig(MODEM_LORA, LORA_BANDWIDTH, LORA_SPREADING_FACTOR,
                    LORA_CODINGRATE, 0, LORA_PREAMBLE_LENGTH,
                    LORA_SYMBOL_TIMEOUT, LORA_FIX_LENGTH_PAYLOAD_ON,
                    0, true, 0, 0, LORA_IQ_INVERSION_ON, true);

  state = STATE_TX;
unsigned long previousMillis = 0;  // stores the last time we printed
const unsigned long interval = 200; 
  // ---- FreeRTOS loop ----
  while (true)
  {
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;  // update last print time

    // Serial.printf("\rstate %d, %d\r\n",
    //                   state,STATE_TX);
      }
    if(enviar&& state == LOWPOWER){
      Radio.Sleep();
      state = STATE_TX;
    }
    switch (state)
    {
      case STATE_TX:
        while(!enviar){

  // Serial.print("\n!enviar");

  // Radio.Sleep();
        vTaskDelay(1000 / portTICK_PERIOD_MS);
          }
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        txNumber++;

        snprintf(txpacket, BUFFER_SIZE, "%s", mensagem_enviada.c_str());

        Serial.printf("\r\nsending packet \"%s\" , length %d\r\n",
                      txpacket, strlen(txpacket));
        // antes = now;
        state = LOWPOWER;
        Radio.Send((uint8_t *)txpacket, strlen(txpacket));

        enviar = false;
        memset(txpacket, 0, BUFFER_SIZE);

        break;

      case STATE_RX:
        Serial.println("into RX mode");
        Radio.Rx(0);
        state = LOWPOWER;
        // antes = now;
        break;

      case LOWPOWER:
        Radio.IrqProcess();

        break;
    }

    // Prevent watchdog resets
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

// ======================================================
// RADIO CALLBACKS
// ======================================================
void OnTxDone(void)
{
  Serial.print("TX done......");
  Radio.Sleep();

  state = STATE_RX;
}

void OnTxTimeout(void)
{
  Radio.Sleep();
  Serial.print("TX Timeout......");
  state = STATE_TX;
}

void OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr)
{
  Rssi = rssi;
  rxSize = size;

  memcpy(rxpacket, payload, size);
  rxpacket[size] = '\0';

  Radio.Sleep();

	
    factory_display.clear();
    // Print to the screen
    factory_display.println(rxpacket);
    // Draw it to the internal screen buffer
    factory_display.drawLogBuffer(0, 0);
    // Display it on the screen
    factory_display.display();
  Serial.printf("\r\nreceived packet \"%s\" with Rssi %d , length %d\r\n",
                rxpacket, Rssi, rxSize);
  Serial.println("wait to send next packet");
  // state = STATE_TX;
    state = STATE_RX;

}

// ======================================================
// SETUP
// ======================================================
void setup()
{
  Serial.begin(115200);
  delay(1000);

  delay(500);
  Serial.println("Teste serial");
  delay(500);

  factory_display.init();
	factory_display.clear();
	factory_display.display();
	delay(300);
	factory_display.clear();
  factory_display.drawString(0, 0, "Hello world :)");
		factory_display.display();

  delay(1000);
  // Heltec.begin(true /*Display Enable*/, true /*LoRa Enable*/, true /*Serial Enable*/);
  // delay(1000);
  // Heltec.display->clear();
  // Heltec.display->drawString(0, 0, "Hello from Heltec!");
  // Heltec.display->display();
  // delay(1000);

  Mcu.begin(HELTEC_BOARD, SLOW_CLK_TPYE);  //Usado para a biblioteca de lora


  Serial.println("System starting...");
  // Create FreeRTOS tasks
  xTaskCreatePinnedToCore(
      BlinkTask,
      "BlinkTask",
      2048,
      NULL,
      1,
      &blinkTaskHandle,
      1);

  xTaskCreatePinnedToCore(
      LoRaTask,
      "LoRaTask",
      8192,
      NULL,
      1,
      &loraTaskHandle,
      0);

  Serial.println("System Ready");
}

// ======================================================
// LOOP (unused)
// ======================================================
void loop()
{
  vTaskDelay(portMAX_DELAY);
}
