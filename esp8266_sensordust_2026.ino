#include "SPI.h"
#include "Adafruit_GFX.h"
#include <Adafruit_ST7735.h>
#include "Adafruit_PM25AQI.h"
#include <SoftwareSerial.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

// ==========================================
// CONFIGURAÇÕES
// ==========================================
const char* ssid = "pi_3d";
const char* password = "fr@nco9173";
const char* thingSpeakApiKey = "XUXKQW00OOQV61FL";

// Sinric Pro (Alexa)
const char* sinricAppKey = "b32da206-ab2d-42e2-bdd3-51da06c8f79c";
const char* deviceId = "69f37a91ad44f4047d0acf3a";

// ==========================================
// PINAGEM
// ==========================================
#define TFT_CS    D8
#define TFT_DC    D2
#define TFT_RST   D1
#define PM_RX     D3
#define PM_TX     D4

// ==========================================
// OBJETOS
// ==========================================
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);
SoftwareSerial pmSerial(PM_RX, PM_TX);
Adafruit_PM25AQI aqi = Adafruit_PM25AQI();
ESP8266WebServer server(80);

// ==========================================
// VARIÁVEIS
// ==========================================
int pm10 = 0, pm25 = 0, pm100 = 0;

// Histórico (últimos 20 valores)
#define HIST_SIZE 20
int pm25History[HIST_SIZE];
int histIndex = 0;

unsigned long lastThingSpeakSend = 0;
unsigned long lastAlexaSend = 0;

// Cores personalizadas (já que ST7735_DARKGREY não existe)
#define DARKGREY 0x7BEF  // Cinza escuro RGB565

// Cores da escala
const uint16_t pm25color[] = {0x37E0, 0xFFE0, 0xF800, 0xC99F};

// ==========================================
// CONECTAR WIFI
// ==========================================
void connectWiFi() {
  tft.fillScreen(ST7735_BLACK);
  tft.setCursor(10, 70);
  tft.setTextSize(1);
  tft.setTextColor(ST7735_WHITE);
  tft.print("WiFi...");
  
  WiFi.begin(ssid, password);
  int tentativas = 0;
  
  while (WiFi.status() != WL_CONNECTED && tentativas < 30) {
    delay(500);
    tentativas++;
  }
  
  tft.fillRect(10, 70, 100, 10, ST7735_BLACK);
  tft.setCursor(10, 70);
  
  if (WiFi.status() == WL_CONNECTED) {
    tft.print("OK");
  } else {
    tft.print("Fail");
  }
  delay(1000);
}

// ==========================================
// SERVIDOR WEB (MagicMirror)
// ==========================================
void handleJSON() {
  String json = "{\"pm25\":" + String(pm25) + 
                ",\"pm10\":" + String(pm10) + 
                ",\"pm1_0\":" + String(pm100) + "}";
  server.send(200, "application/json", json);
}

// ==========================================
// ENVIAR PARA ALEXA
// ==========================================
void sendToAlexa() {
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClient alexaClient;
    if (alexaClient.connect("api.sinric.pro", 80)) {
      String json = "{\"deviceId\":\"" + String(deviceId) + 
                    "\",\"action\":\"setPm25\",\"value\":" + String(pm25) + "}";
      
      String request = "POST / HTTP/1.1\r\n";
      request += "Host: api.sinric.pro\r\n";
      request += "x-api-key: " + String(sinricAppKey) + "\r\n";
      request += "Content-Length: " + String(json.length()) + "\r\n\r\n";
      request += json;
      
      alexaClient.print(request);
    }
    alexaClient.stop();
  }
}

// ==========================================
// ENVIAR PARA THINGSPEAK
// ==========================================
void sendToThingSpeak() {
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClient tsClient;
    String url = "/update?api_key=" + String(thingSpeakApiKey) +
                 "&field1=" + String(pm25);
    
    if (tsClient.connect("api.thingspeak.com", 80)) {
      tsClient.print("GET " + url + " HTTP/1.1\r\n");
      tsClient.print("Host: api.thingspeak.com\r\n");
      tsClient.print("Connection: close\r\n\r\n");
    }
    tsClient.stop();
  }
}

// ==========================================
// FUNÇÃO PARA CORES DO PM
// ==========================================
uint16_t getPMColor(int value) {
  if(value < 12) return 0x37E0;  // Verde
  if(value < 36) return 0xFFE0;  // Amarelo
  if(value < 55) return 0xF800;  // Vermelho
  return 0xC99F;  // Marrom
}

// ==========================================
// DISPLAY - VERSÃO COMPACTA
// ==========================================
void drawUI() {
  tft.fillScreen(ST7735_BLACK);
  tft.setTextSize(1);
  
  // Título PM2.5
  tft.fillRect(0, 0, 128, 45, ST7735_BLACK);
  tft.setCursor(5, 2);
  tft.setTextColor(ST7735_WHITE);
  tft.print("PM2.5");
  
  // Labels PM10 e PM1.0
  tft.setCursor(5, 48);
  tft.setTextColor(ST7735_WHITE);
  tft.print("PM10");
  
  tft.setCursor(70, 48);
  tft.print("PM1.0");
  
  // Labels da barra
  tft.setCursor(5, 80);
  tft.setTextSize(0);
  tft.print("0");
  tft.setCursor(55, 80);
  tft.print("50");
  tft.setCursor(105, 80);
  tft.print("100");
  
  // Label do gráfico
  tft.setCursor(5, 110);
  tft.print("Hist");
  
  // IP na última linha
  tft.setCursor(5, 152);
  tft.setTextSize(0);
  tft.print(WiFi.localIP());
}

void updateDisplay() {
  // ===== PM2.5 - VALOR GRANDE =====
  tft.fillRect(50, 15, 70, 30, ST7735_BLACK);
  tft.setCursor(50, 15);
  tft.setTextSize(3);
  tft.setTextColor(getPMColor(pm25));
  
  if(pm25 < 10) tft.print(" ");
  tft.print(pm25);
  
  // ===== PM10 =====
  tft.fillRect(5, 60, 55, 15, ST7735_BLACK);
  tft.setCursor(5, 60);
  tft.setTextSize(2);
  tft.setTextColor(getPMColor(pm10));
  if(pm10 < 10) tft.print(" ");
  tft.print(pm10);
  
  // ===== PM1.0 =====
  tft.fillRect(70, 60, 55, 15, ST7735_BLACK);
  tft.setCursor(70, 60);
  tft.setTextColor(getPMColor(pm100));
  if(pm100 < 10) tft.print(" ");
  tft.print(pm100);
  
  // ===== BARRA DE PROGRESSO =====
  int barWidth = map(constrain(pm25, 0, 100), 0, 100, 0, 118);
  
  // Barra verde (preenchida)
  if(barWidth > 0) {
    tft.fillRect(5, 90, barWidth, 8, getPMColor(pm25));
  }
  // Barra cinza (restante)
  if(118 - barWidth > 0) {
    tft.fillRect(5 + barWidth, 90, 118 - barWidth, 8, DARKGREY);
  }
  
  // ===== GRÁFICO MINI (20 barras) =====
  int graphX = 30;
  int graphY = 110;
  int barWidth_g = 4;
  int maxHeight = 35;
  
  // Limpa área do gráfico
  tft.fillRect(29, 110, 95, 38, ST7735_BLACK);
  
  // Desenha borda do gráfico
  tft.drawRect(28, 109, 96, 38, ST7735_WHITE);
  
  // Desenha barras
  for(int i = 0; i < HIST_SIZE; i++) {
    int idx = (histIndex + i) % HIST_SIZE;
    int value = pm25History[idx];
    if(value == 0 && i > histIndex) continue;
    
    int barHeight = map(constrain(value, 0, 100), 0, 100, 0, maxHeight);
    int x = graphX + (i * barWidth_g);
    uint16_t color = getPMColor(value);
    
    if(barHeight > 0) {
      tft.fillRect(x, graphY + maxHeight - barHeight, 3, barHeight, color);
    }
  }
  
  // ===== LED DE STATUS WIFI =====
  if(WiFi.status() == WL_CONNECTED) {
    tft.fillCircle(123, 155, 2, ST7735_GREEN);
  } else {
    tft.fillCircle(123, 155, 2, ST7735_RED);
  }
}

// ==========================================
// SETUP
// ==========================================
void setup() {
  Serial.begin(115200);
  delay(1000);
  
  // Inicializa histórico
  for(int i = 0; i < HIST_SIZE; i++) pm25History[i] = 0;
  
  // Sensor PM
  pmSerial.begin(9600);
  delay(500);
  
  if (!aqi.begin_UART(&pmSerial)) {
    Serial.println("Sensor PM nao encontrado!");
    while(1) {
      delay(1000);
      Serial.print(".");
    }
  }
  Serial.println("PMS7003 OK!");
  
  // Display
  tft.initR(INITR_BLACKTAB);
  tft.setRotation(2);
  
  // WiFi
  connectWiFi();
  
  // Servidor web
  server.on("/json", handleJSON);
  server.begin();
  
  // Interface
  drawUI();
  
  delay(2000);
  sendToAlexa();
}

// ==========================================
// LOOP PRINCIPAL
// ==========================================
void loop() {
  PM25_AQI_Data data;
  
  if (!aqi.read(&data)) {
    delay(500);
    return;
  }
  
  // Atualiza valores
  pm10 = data.pm10_standard;
  pm25 = data.pm25_standard;
  pm100 = data.pm100_standard;
  
  // Adiciona ao histórico
  pm25History[histIndex] = pm25;
  histIndex = (histIndex + 1) % HIST_SIZE;
  
  // Atualiza display
  updateDisplay();
  
  // ThingSpeak a cada 20 segundos
  if(millis() - lastThingSpeakSend > 20000) {
    sendToThingSpeak();
    lastThingSpeakSend = millis();
  }
  
  // Alexa a cada 30 segundos
  if(millis() - lastAlexaSend > 30000) {
    sendToAlexa();
    lastAlexaSend = millis();
  }
  
  // Mantém servidor web
  server.handleClient();
  
  // Debug
  Serial.print("PM2.5: "); Serial.print(pm25);
  Serial.print(" | PM10: "); Serial.print(pm10);
  Serial.print(" | PM1.0: "); Serial.println(pm100);
  
  delay(2000);
}