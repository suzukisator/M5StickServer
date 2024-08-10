#include <M5Unified.h>
#include <Kalman.h>
#include <WiFi.h>

// WiFi設定
const char* ssid = "elecom2g-EEB358";
const char* password = "3455965811392";

const char* server_ip = "192.168.1.18";
const int server_port = 3002;
const int device_id = 1;

Kalman kalmanX, kalmanY, kalmanZ;
float acc[3], gyro[3], kalacc[3] = {0, 0, 0};
float accnorm, dt = 0;
unsigned long prevTime, currentTime, startTime, batteryUpdateTime;
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
  unsigned long startAttemptTime = millis();

  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 10000) {
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

void deltaTime(void) {
  currentTime = millis();
  dt = (currentTime - prevTime) / 1000.0f;
  prevTime = currentTime;
}

// センサーデータ取得
void getData() {
  M5.Imu.getAccel(&acc[0], &acc[1], &acc[2]);
  M5.Imu.getGyro(&gyro[0], &gyro[1], &gyro[2]);
}

void kalmanAccel(void) {
  kalacc[0] = kalmanX.getAngle(acc[0], gyro[0], dt);
  kalacc[1] = kalmanY.getAngle(acc[1], gyro[1], dt);
  kalacc[2] = kalmanZ.getAngle(acc[2], gyro[2], dt);
}

void accelNorm(void) {
  accnorm = sqrt(kalacc[0] * kalacc[0] + kalacc[1] * kalacc[1] + kalacc[2] * kalacc[2]);
}

// データ送信
void sendData() {
  if (wifiClient.connected()) {
    byte data[24];
    float m5time = millis() / 1000.0f;
    *((int*)data) = device_id;
    *((float*)(data + 4)) = kalacc[0];
    *((float*)(data + 8)) = kalacc[1];
    *((float*)(data + 12)) = kalacc[2];
    *((float*)(data + 16)) = accnorm;
    *((float*)(data + 20)) = m5time;

    wifiClient.write(data, sizeof(data));
  } else {
    wifiClient.stop();
    while (!connectToServer()) {
      if (WiFi.status() != WL_CONNECTED) {
        displayStatus("Failed to connect WiFi", false, false);
        while (WiFi.status() != WL_CONNECTED) delay(1000);
          M5.Lcd.print(".");
      } else {
        displayStatus("WiFi connected", true, false);
      }
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
  M5.Lcd.setBrightness(30); // 明るさを設定（0-255）
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

  deltaTime();

  if (dt > 0) {
    getData();
    kalmanAccel();
    accelNorm();
    sendData();
  }

  delay(20);
}
