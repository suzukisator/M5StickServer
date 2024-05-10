#include <M5StickCPlus.h>
#include <Kalman.h>
#include <WiFi.h>

//大学
const char* ssid = "ASUS_28_2G";
const char* password = "morning_6973";
const char* server_ip = "192.168.50.9";

/*自宅
const char* ssid = "Buffalo-G-1AF0";
const char* password = "7nyh4sj46px64";
const char* server_ip = "192.168.50.9";
*/
const int server_port = 3002;
const int device_id = 3; //IDは1~100番までで設定。サーバー側でおかしなIDをはじく為

Kalman kalmanX;
Kalman kalmanY;
Kalman kalmanZ;

float acc[3] = {0, 0, 0};
float gyro[3] = {0, 0, 0};
float filteredacc[3] = {0, 0, 0};
float sumAcc[3] = {0, 0, 0};
float meanacc[3] = {0, 0, 0};
float sumnormacc = 0;
float meannormacc = 0;
unsigned long prevTime = 0, startTime = 0;
int count = 0;
const int interval = 10000;

WiFiClient wifiClient;

//Displayに文字を映す
void displayConnectionStatus(const char* status) {
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.println("m5stick ver 2.32");
  M5.Lcd.print("Device_ID : ");
  M5.Lcd.println(device_id);
  M5.Lcd.print("interval : ");
  M5.Lcd.print(interval);
  M5.Lcd.println("ms");
  M5.Lcd.println(status);
}

//wifi接続
void setupWiFi() {
  m5.Lcd.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  unsigned long startAttemptTime = millis();

  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 10000) {
    delay(500);
    M5.Lcd.print(".");
  }

  if (WiFi.status() != WL_CONNECTED) {
    displayConnectionStatus("Failed to connect WiFi");
    while(1) delay(1000);
  } else {
    displayConnectionStatus("WiFi connected");
  }
}

//サーバーとの接続を確かめてbool代数で返す
bool connectToServer() {
  if (!wifiClient.connect(server_ip, server_port)) {
    displayConnectionStatus("server conn failed");
    return false;
  } else {
    displayConnectionStatus("Connected to server");
    return true;
  }
}

//m5tickのセンサーデータ取得
void getData() {
  M5.IMU.getAccelData(&acc[0], &acc[1], &acc[2]);
  M5.IMU.getGyroData(&gyro[0], &gyro[1], &gyro[2]);
}

//送信データ作成
void sendData() {
  if (wifiClient.connected()) {
    byte data[24];

    //int deviceIdNetworkOrder = htonl(device_id);
    float m5time = millis() / 1000.0f;
    *((int*)data) = device_id;
    *((float*)(data + 4)) = meanacc[0];
    *((float*)(data + 8)) = meanacc[1];
    *((float*)(data + 12)) = meanacc[2];
    *((float*)(data + 16)) = meannormacc;
    *((float*)(data + 20)) = m5time;
    /*
    memcpy(data, &deviceIdNetworkOrder, sizeof(deviceIdNetworkOrder));
    memcpy(data + 4, &meanacc[0], sizeof(meanacc[0]));
    memcpy(data + 8, &meanacc[1], sizeof(meanacc[1]));
    memcpy(data + 12, &meanacc[2], sizeof(meanacc[2]));
    memcpy(data + 16, &meannormacc, sizeof(meannormacc));
    memcpy(data + 20, &m5time, sizeof((m5time)));
    */

    wifiClient.write(data, sizeof(data));
  } else {
    wifiClient.stop();
    while (!connectToServer()) {
      displayConnectionStatus("Reconnection failed. Trying again...");
      delay(1000);
    }
    displayConnectionStatus("Reconnected to server");
  }
}

void setup() {
  M5.begin();
  M5.IMU.Init();
  M5.Lcd.setTextSize(1);
  //M5.Axp.ScreenBreath(7);
  setupWiFi();
  if (connectToServer()) {
    displayConnectionStatus("Connected to server");
  }

  prevTime = millis();
  startTime= millis();
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

    float normacc = sqrt(filteredacc[0]*filteredacc[0] + filteredacc[1]*filteredacc[1] + filteredacc[2]*filteredacc[2]);

    for (int i = 0; i < 3; i++) {
      sumAcc[i] += filteredacc[i];
      sumnormacc += normacc;
    }
    count++;

    if (currentTime - startTime >= interval) {
      for (int i = 0; i < 3; i++) {
        meanacc[i] = sumAcc[i] / count;
      }
      meannormacc = sumnormacc / count;
      sendData();
      for (int i = 0; i < 3; i++) {
        sumAcc[i] = 0;
      }
      sumnormacc = 0;
      count = 0;
      startTime = currentTime;
    }
  }

  delay(20);
}
