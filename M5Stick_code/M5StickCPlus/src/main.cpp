#include <M5Unified.h>
#include <Kalman.h>
#include <WiFi.h>
#include <FS.h>
#include <SPIFFS.h>

// WiFi設定
/*
const char* ssid = "ASUS_28_2G";
const char* password = "morning_6973";
*/
const char *ssid = "Buffalo-G-1AF0";
const char *password = "7nyh4sj46px64";
const char* server_ip = "192.168.11.2";
const int server_port = 3002;
const int device_id = 10;

Kalman kalmanX, kalmanY, kalmanZ;
float acc[3] = {0, 0, 0}, gyro[3] = {0, 0, 0}, filteredacc[3] = {0, 0, 0};
float accnorm = 0;
unsigned long prevTime = 0, startTime = 0, batteryUpdateTime = 0;
int count = 0;
const int interval = 100;
WiFiClient wifiClient;

// ディスプレイに情報を表示する
void displayStatus(const char* status, bool wifiConnected, bool serverConnected) {
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setRotation(1);  // 90度右に回転
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.setTextSize(2);  // 文字サイズを変更
  M5.Lcd.printf("Device_ID : %d\n", device_id);
  M5.Lcd.printf("Battery: %d%%\n", M5.Power.getBatteryLevel());
  M5.Lcd.printf("WiFi: %s\n", wifiConnected ? "Connected" : "Disconnected");
  M5.Lcd.printf("Server: %s\n", serverConnected ? "Connected" : "Disconnected");
  M5.Lcd.println(status);
}

// WiFi接続設定
void setupWiFi() {
  M5.Lcd.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  //unsigned long startAttemptTime = millis();

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    M5.Lcd.print(".");
  }

  if (WiFi.status() != WL_CONNECTED) {
    displayStatus("Failed to connect WiFi", false, false);
    while (1) delay(1000);
  } else {
    displayStatus("WiFi connected", true, false);
  }
}

// サーバー接続設定
bool connectToServer() {
  if (!wifiClient.connect(server_ip, server_port)) {
    displayStatus("Server conn failed", true, false);
    return false;
  } else {
    displayStatus("Connected to server", true, true);
    return true;
  }
}

// センサーデータ取得
void getData() {
  M5.Imu.getAccel(&acc[0], &acc[1], &acc[2]);
  M5.Imu.getGyro(&gyro[0], &gyro[1], &gyro[2]);
}

// データ送信
void sendData() {
  if (wifiClient.connected() && connectToServer()) {
    byte data[24];
    float m5time = millis() / 1000.0f;
    *((int*)data) = device_id;
    *((float*)(data + 4)) = filteredacc[0];
    *((float*)(data + 8)) = filteredacc[1];
    *((float*)(data + 12)) = filteredacc[2];
    *((float*)(data + 16)) = accnorm;
    *((float*)(data + 20)) = m5time;

    wifiClient.write(data, sizeof(data));
  } else if (!wifiClient.connected()){
      while(!wifiClient.connected()){
        setupWiFi();
      }
  } else {
    wifiClient.stop();
    while (!connectToServer()) {
      displayStatus("Reconnection failed. Trying again...", true, false);
      delay(1000);
    }
    displayStatus("Reconnected to server", true, true);
  }
}

void setup() {
  auto cfg = M5.config();
  cfg.internal_imu = true; // 内部IMUを使用する設定
  M5.begin(cfg);
  M5.Lcd.setTextSize(2); // 文字サイズを変更
  M5.Lcd.setRotation(1); // 90度右に回転
  //M5.Lcd.setBrightness(50); // 明るさを設定（0-255）
  setupWiFi();
  if (connectToServer()) {
    displayStatus("Connected to server", true, true);
  }

  prevTime = millis();
  startTime = millis();
  batteryUpdateTime = millis();
}

void loop() {
  M5.update();
  unsigned long currentTime = millis();
  float dt = (currentTime - prevTime) / 1000.0f;
  prevTime = currentTime;

  if (dt > 0) {
    getData();
    filteredacc[0] = kalmanX.getAngle(acc[0], gyro[0], dt);
    filteredacc[1] = kalmanY.getAngle(acc[1], gyro[1], dt);
    filteredacc[2] = kalmanZ.getAngle(acc[2], gyro[2], dt);

    accnorm = sqrt(filteredacc[0] * filteredacc[0] + filteredacc[1] * filteredacc[1] + filteredacc[2] * filteredacc[2]);

    sendData();
  }

  if (currentTime - batteryUpdateTime >= 5000) { // 5秒ごとにバッテリー状況を更新
    displayStatus("Updating status...", WiFi.status() == WL_CONNECTED, wifiClient.connected());
    batteryUpdateTime = currentTime;
  }

  delay(20);
}
