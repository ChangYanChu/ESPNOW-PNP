#include <Arduino.h>
#include <Wire.h>
#include "HCPCA9685.h"
#include <WiFi.h>
#include <ESPAsyncWebServer.h>

#define I2CAdd 0x40
HCPCA9685 pwm(I2CAdd);
#define PCA9685_OE_PIN 10

const char* ssid = "HONOR";
const char* password = "chu107610.";
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

String htmlPage;

void scanI2C() {
  Serial.println("开始扫描 I2C 设备...");
  byte error, address;
  int nDevices = 0;
  for(address = 1; address < 127; address++ ) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
    if (error == 0) {
      Serial.print("发现设备在地址 0x");
      if (address<16)
        Serial.print("0");
      Serial.print(address, HEX);
      Serial.println("");
      nDevices++;
    } else if (error==4) {
      Serial.print("未知错误在地址 0x");
      if (address<16)
        Serial.print("0");
      Serial.println(address, HEX);
    }
  }
  if (nDevices == 0)
    Serial.println("未发现 I2C 设备\n");
  else
    Serial.println("I2C 扫描完成\n");
}

void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  if(type == WS_EVT_DATA) {
    String msg = String((char*)data);
    // 格式: "id:angle" 或 "all:angle"
    if(msg.startsWith("all:")) {
      int angle = msg.substring(4).toInt();
      for(int i=0;i<=12;i++) {
        pwm.Servo(i, map(angle,0,180,10,450));
      }
    } else {
      int sep = msg.indexOf(":");
      if(sep>0) {
        int id = msg.substring(0,sep).toInt();
        int angle = msg.substring(sep+1).toInt();
        pwm.Servo(id, map(angle,0,180,10,450));
      }
    }
  }
}

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  Serial.print("连接WiFi");
  while(WiFi.status()!=WL_CONNECTED){
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi已连接，IP地址:");
  Serial.println(WiFi.localIP());

  htmlPage = "<html><head><meta charset='utf-8'><title>舵机控制</title>"
    "<meta name='viewport' content='width=device-width, initial-scale=1'>"
    "<link rel='stylesheet' href='https://cdn.jsdelivr.net/npm/bootstrap@5.3.0/dist/css/bootstrap.min.css'>"
    "</head><body><div class='container py-4'>"
    "<h2 class='mb-4 text-center'>舵机控制面板</h2>"
    "<table class='table table-bordered table-hover'><thead class='table-light'><tr><th>编号</th><th>角度(0-180)</th></tr></thead><tbody>";
  for(int i=0;i<=12;i++){
    htmlPage += "<tr><td>"+String(i)+"</td><td><input type='range' min='0' max='180' value='90' class='form-range' style='width:150px;' oninput='wsSend("+String(i)+",this.value);this.nextElementSibling.value=this.value'><output class='ms-2'>90</output></td></tr>";
  }
  htmlPage += "</tbody></table>";
  htmlPage += "<div class='d-flex align-items-center mb-3'><input id='allangle' type='range' min='0' max='180' value='90' class='form-range' style='width:150px;' oninput='wsSend(\"all\",this.value);this.nextElementSibling.value=this.value'><output class='ms-2'>90</output></div>";
  htmlPage += "<script>var ws=new WebSocket('ws://'+location.host+'/ws');function wsSend(id,val){ws.send(id+':'+val);}</script>";
  htmlPage += "</div></body></html>";

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/html", htmlPage);
  });
  server.addHandler(&ws);
  ws.onEvent(onWsEvent);
  server.begin();
  Wire.begin();
  delay(3000);
  scanI2C();
  pinMode(PCA9685_OE_PIN, OUTPUT);
  digitalWrite(PCA9685_OE_PIN, LOW);
  pwm.Init(SERVO_MODE);
  pwm.Sleep(false);
}

void loop() {
  // 空循环，所有控制由ws事件处理
}