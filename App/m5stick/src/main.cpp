#include <M5Unified.h>
#include <Kalman.h>
#include <WiFi.h>

Kalman kalmanX, kalmanY, kalmanZ;
WiFiClient wifiClient;

// wifi設定
const char* SSID = "Buffalo-G-1AF0";
const char* PASSWORD  = "7nyh4sj46px64";

const char* SERVER_IP = "192.168.11.20";
const int SERVER_PORT = 3002;
const int DEVICE_ID = 4;

float acc[3], gyro[3], kalacc[3] = {0, 0, 0};
float accnorm, dt = 0;
unsigned long prevTime, currentTime;
float battery = 100.0f;
bool display = true;
int displaycount = 0;

void deltaTime(void) {
    currentTime = millis();
    dt = (currentTime - prevTime) / 1000.0f;
    prevTime = currentTime;
}

void getIMU(void) {
    M5.Imu.getAccel(&acc[0], &acc[1], &acc[2]);
    M5.Imu.getGyro(&gyro[0], &gyro[1], &gyro[2]);
}

void kalmanAccel(void) {
    kalacc[0] = kalmanX.getAngle(acc[0], gyro[0], dt);
    kalacc[1] = kalmanY.getAngle(acc[1], gyro[1], dt);
    kalacc[2] = kalmanZ.getAngle(acc[2], gyro[2], dt);
}
/*
void accelNorm(void) {
    accnorm = sqrt(kalacc[0]*kalacc[0] + kalacc[1]*kalacc[1] + kalacc[2]*kalacc[2]);
}
*/

void sendData(void) {
    if (wifiClient.connected()) {
        byte data[24];
        float m5time = millis() / 1000.0f;
        *((int*)data) = DEVICE_ID;
        *((float*)(data + 4)) = kalacc[0];
        *((float*)(data + 8)) = kalacc[1];
        *((float*)(data + 12)) = kalacc[2];
        *((float*)(data + 16)) = m5time;

        size_t bytesToSend = sizeof(data);
        size_t bytesSent = wifiClient.write(data, bytesToSend);
        if (bytesSent < bytesToSend) {
            int retries = 3;
            while (retries-- > 0 && bytesSent < bytesToSend) {
                delay(100);
                bytesSent += wifiClient.write(data + bytesSent, bytesToSend - bytesSent);
            }
        }
    }
}

void BasicInfo(void) {
    M5.Lcd.setTextSize(2);
    M5.Lcd.setCursor(3,181);
    M5.Lcd.println("ID");
    M5.Lcd.setCursor(3,201);
    M5.Lcd.println("WiFi");
    M5.Lcd.setCursor(3,221);
    M5.Lcd.println("Server");
}

void line(void) {
    M5.Lcd.fillRect(75,179,1,61,WHITE);
    M5.Lcd.fillRect(0,179,135,1,WHITE);
    M5.Lcd.fillRect(0,199,135,1,WHITE);
    M5.Lcd.fillRect(0,219,135,1,WHITE);
}

void netWorkStatus(const char *wifistatus, const char *serverstatus) {
    M5.Lcd.setTextSize(2);
    M5.Lcd.setCursor(97,181);
    M5.Lcd.println(DEVICE_ID);
    M5.Lcd.fillRect(76,200,59,19,BLACK);
    M5.Lcd.setCursor(80,201);
    M5.Lcd.println(wifistatus);
    M5.Lcd.fillRect(76,220,59,19,BLACK);
    M5.Lcd.setCursor(80,221);
    M5.Lcd.println(serverstatus);
}

void ConnectMonitor(void) {
    if (WiFi.status() != WL_CONNECTED) {
        if (display) {
            netWorkStatus("No", "No");
        }
        return;
    }

    if (wifiClient.connected()) {
        if (display) {
            netWorkStatus("Yes", "Yes");
        }

    } else {
        wifiClient.stop();
        if (display) {
            netWorkStatus("Yes", "No");
        }
        wifiClient.connect(SERVER_IP, SERVER_PORT);
    }
}

void visualdenchi(void) {
    M5.Lcd.fillRect(80, 150, 3, 12, WHITE);
    M5.Lcd.fillRect(83, 142, 1, 27, WHITE);
    M5.Lcd.fillRect(83, 142, 48, 1, WHITE);
    M5.Lcd.fillRect(83, 169, 48, 1, WHITE);
    M5.Lcd.fillRect(130, 142, 1, 27, WHITE);
}

void batterycolor(int color1 , int color2, int color3, int color4, int color5) {
    M5.Lcd.fillRect(85,144,7,24,color1);
    M5.Lcd.fillRect(94,144,7,24,color2);
    M5.Lcd.fillRect(103,144,7,24,color3);
    M5.Lcd.fillRect(112,144,7,24,color4);
    M5.Lcd.fillRect(121,144,7,24,color5);
}

void VisualBattery(void) {
    battery = M5.Power.getBatteryLevel();
    if (battery > 80) {
        batterycolor(GREEN, GREEN, GREEN, GREEN, GREEN);
    } else if (80 >= battery && battery > 60) {
        batterycolor(BLACK, GREEN, GREEN, GREEN, GREEN);
    }else if (60 >= battery && battery > 40) {
        batterycolor(BLACK, BLACK, GREEN, GREEN, GREEN);
    }else if (40 >= battery && battery > 20) {
        batterycolor(BLACK, BLACK, BLACK, GREEN, GREEN);
    }else if (20 >= battery) {
        batterycolor(BLACK, BLACK, BLACK, BLACK, GREEN);
    }
}

void IMUInfo(void) {
    M5.Lcd.setTextSize(1);
    M5.Lcd.setCursor(3,3);
    M5.Lcd.println("AccelX");
    M5.Lcd.setCursor(3,23);
    M5.Lcd.println("AccelY");
    M5.Lcd.setCursor(3,43);
    M5.Lcd.println("AccelZ");
}

void IMUprint(void) {
    M5.Lcd.setTextSize(2);
    M5.Lcd.fillRect(60,0,75,20,BLUE);
    M5.Lcd.setCursor(60,0);
    M5.Lcd.println(kalacc[0],1);
    M5.Lcd.fillRect(60,20,75,20,RED);
    M5.Lcd.setCursor(60,20);
    M5.Lcd.println(kalacc[1],1);
    M5.Lcd.fillRect(60,40,75,20,DARKGREEN);
    M5.Lcd.setCursor(60,40);
    M5.Lcd.println(kalacc[2],1);
    M5.Lcd.fillRect(59,0,1,61,WHITE);
    M5.Lcd.fillRect(59,61,75,1,WHITE);
}

void UpTime() {
    float seconds = millis() / 1000.0f;
    int minutes = static_cast<int>(seconds) / 60;
    int hours = minutes / 60;
    seconds = static_cast<int>(seconds) % 60;
    minutes = minutes % 60;
    M5.Lcd.setTextSize(2);
    M5.Lcd.setCursor(0,65);
    M5.Lcd.println("Up Time");
    M5.Lcd.setTextSize(3);
    M5.Lcd.setCursor(90,90);
    M5.Lcd.fillRect(90,90,40,35,BLUE);
    M5.Lcd.println(seconds,0);
    M5.Lcd.setCursor(45,90);
    M5.Lcd.fillRect(45,90,40,35,RED);
    M5.Lcd.println(minutes,0);
    M5.Lcd.setCursor(0,90);
    M5.Lcd.fillRect(0,90,40,35,DARKGREEN);
    M5.Lcd.println(hours,0);
    M5.Lcd.setCursor(75,90);
    M5.Lcd.println(":");
    M5.Lcd.setCursor(30,90);
    M5.Lcd.println(":");
}

void screenControler(void) {
    if (M5.BtnA.isPressed()) {
        if (displaycount == 0) {
            display = false;
            displaycount ++;
            delay(100);
        } else if (displaycount == 1) {
            display = true;
            displaycount = 0;
            delay(100);
        }
    }

    if (display) {
        M5.Lcd.setBrightness(70);
        BasicInfo();
        IMUInfo();
        IMUprint();
        UpTime();
        line();
        visualdenchi();
        VisualBattery();
    } else {
        M5.Lcd.setBrightness(0);
        M5.Lcd.fillScreen(BLACK);
    }
}

void setup(void) {
    auto cfg = M5.config();
    cfg.internal_imu = true;
    M5.begin(cfg);
    M5.Lcd.setTextSize(1);
    M5.Lcd.setRotation(0);
    M5.Lcd.setTextColor(WHITE);
    WiFi.begin(SSID, PASSWORD);
    wifiClient.connect(SERVER_IP, SERVER_PORT);

    screenControler();

    prevTime = millis();
}

void loop(void) {
    M5.update();

    deltaTime();
    getIMU();
    kalmanAccel();
    //accelNorm();

    screenControler();
    ConnectMonitor();

    sendData();

    delay(50);
}
