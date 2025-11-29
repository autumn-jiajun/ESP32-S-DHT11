#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DHT.h>

// DHT11 配置
#define DHTPIN 21
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// WiFi 配置
const char* ap_ssid = "ESP32-GPIO";
const char* ap_password = "12345678";
const char* sta_ssid = "hpwifi";
const char* sta_password = "hpwifi5511";
WebServer server(80);

// GPIO 控制引脚（不含DHT11引脚）
const uint8_t gpio_pins[] = {2, 3, 4, 12, 13, 14, 15, 16, 17, 18, 19};
const int gpio_count = sizeof(gpio_pins) / sizeof(gpio_pins[0]);

// DHT11 读取缓存
unsigned long lastDhtReadTime = 0;
const long dhtReadInterval = 2000;
float lastTemperature = NAN;
float lastHumidity = NAN;

// 网页生成
String htmlPage() {
  // 每2秒读取一次DHT11
  if (millis() - lastDhtReadTime >= dhtReadInterval) {
    lastDhtReadTime = millis();
    lastHumidity = dht.readHumidity();
    lastTemperature = dht.readTemperature();
  }

  String page = "<!DOCTYPE html><html lang='zh-cn'><head><meta charset='utf-8'>";
  page += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  page += "<title>ESP32 控制中心</title>";
  page += "<meta http-equiv='refresh' content='5'>"; // 自动刷新
  page += "<style>body{font-family:sans-serif;background:#fafafa;margin:20px;}";
  page += ".status-box{background:#e3f2fd;padding:15px;border-radius:8px;margin-bottom:20px;}";
  page += ".gpio-control{background:white;padding:10px;border-radius:8px;margin:10px 0;box-shadow:0 2px 4px #eee;}";
  page += "button{margin:2px;padding:8px 16px;background:#2196f3;color:white;border:none;border-radius:4px;cursor:pointer;}";
  page += "button:hover{background:#0b7dda;}h2{color:#263238;}.status{font-weight:bold;}</style></head><body>";
  page += "<h2>ESP32 控制中心</h2>";

  // WiFi状态
  page += "<div class='status-box'><p>WiFi 状态: ";
  if (WiFi.status() == WL_CONNECTED) {
    page += "<span class='status' style='color:green'>已连接</span> (" + String(sta_ssid) + ")<br>";
    page += "STA IP: " + WiFi.localIP().toString() + "<br>";
  } else {
    page += "<span class='status' style='color:orange'>未连接路由器</span><br>";
  }
  page += "AP IP: " + WiFi.softAPIP().toString() + "</p></div>";

  // 温湿度显示
  page += "<div class='status-box'><p>DHT11 温度: ";
  page += isnan(lastTemperature) ? "<span class='status' style='color:red'>读取失败</span>" : String(lastTemperature) + "°C";
  page += "<br>湿度: ";
  page += isnan(lastHumidity) ? "<span class='status' style='color:red'>读取失败</span>" : String(lastHumidity) + "%";
  page += "</p></div>";

  // GPIO控制
  page += "<h3>GPIO 控制</h3>";
  for(int i=0;i<gpio_count;i++){
    int pin = gpio_pins[i];
    page += "<div class='gpio-control'>GPIO" + String(pin) + ": ";
    page += digitalRead(pin) ? "<span class='status' style='color:green'>开启</span>" : "<span class='status' style='color:gray'>关闭</span>";
    page += "<a href='/on?pin=" + String(pin) + "'><button>开启</button></a>";
    page += "<a href='/off?pin=" + String(pin) + "'><button>关闭</button></a></div>";
  }

  page += "</body></html>";
  return page;
}

// 路由处理
void handleRoot() {
  server.send(200, "text/html", htmlPage());
}

void handleOn() {
  if(server.hasArg("pin")){
    int pin = server.arg("pin").toInt();
    for(int i=0;i<gpio_count;i++){
      if(pin==gpio_pins[i]){
        digitalWrite(pin, HIGH);
        break;
      }
    }
  }
  server.sendHeader("Location", "/", true);
  server.send(302, "text/plain", "");
}

void handleOff() {
  if(server.hasArg("pin")){
    int pin = server.arg("pin").toInt();
    for(int i=0;i<gpio_count;i++){
      if(pin==gpio_pins[i]){
        digitalWrite(pin, LOW);
        break;
      }
    }
  }
  server.sendHeader("Location", "/", true);
  server.send(302, "text/plain", "");
}

void setup() {
  Serial.begin(115200);
  dht.begin();

  // WiFi AP+STA
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(ap_ssid, ap_password);
  WiFi.begin(sta_ssid, sta_password);

  // 初始化GPIO
  for(int i=0;i<gpio_count;i++){
    pinMode(gpio_pins[i], OUTPUT);
    digitalWrite(gpio_pins[i], LOW);
  }

  // 路由注册
  server.on("/", handleRoot);
  server.on("/on", handleOn);
  server.on("/off", handleOff);
  server.begin();

  Serial.println("系统启动完成");
  Serial.println("AP模式已启动，SSID: " + String(ap_ssid) + "，密码: " + String(ap_password));
  Serial.println("AP IP地址: " + WiFi.softAPIP().toString());
  Serial.print("正在连接WiFi: ");
  Serial.println(sta_ssid);
}

void loop() {
  server.handleClient();

  // 每5秒打印一次STA连接状态
  static unsigned long lastStatusPrint = 0;
  if (millis() - lastStatusPrint >= 5000) {
    lastStatusPrint = millis();
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("STA模式已连接，IP地址: " + WiFi.localIP().toString());
    } else {
      Serial.println("正在尝试连接到 " + String(sta_ssid));
    }
  }
}