#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <Adafruit_ADS1X15.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// --- 1. Cấu hình WiFi & Firebase ---
const char* ssid = "Galaxy M55 5G 2918";     // <-- Điền tên WiFi
const char* password = "88888888";    // <-- Điền mật khẩu WiFi
// GIỮ NGUYÊN đuôi /sensors.json ở link bên dưới
const String firebaseURL = "https://aquanexus-d37aa-default-rtdb.firebaseio.com/";

// --- 2. Khai báo phần cứng ---
#define ONE_WIRE_BUS 32 
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
Adafruit_ADS1115 ads;

void setup() {
  // Kết nối WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

  // Khởi động cảm biến
  sensors.begin();
  ads.begin();
  ads.setGain(GAIN_TWOTHIRDS); 
}

// --- Hàm chống nhiễu (Lọc trung bình) ---
float readADC_Average(int channel, int samples) {
  long sum = 0;
  for (int i = 0; i < samples; i++) {
    sum += ads.readADC_SingleEnded(channel);
    delay(5); 
  }
  return (float)sum / samples;
}

void loop() {
  // Chỉ đẩy dữ liệu khi có WiFi
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    
    // 1. Đọc Nhiệt độ
    sensors.requestTemperatures(); 
    float temperature = sensors.getTempCByIndex(0);

    // 2. Đọc các cảm biến Analog (lấy 20 mẫu chống nhiễu)
    float adc0_avg = readADC_Average(0, 20); 
    float adc1_avg = readADC_Average(1, 20); 
    float adc2_avg = readADC_Average(2, 20); 

    // 3. Quy đổi ra Volt
    float vol_turbidity = adc0_avg * 0.1875F / 1000.0F;
    float vol_tds       = adc1_avg * 0.1875F / 1000.0F;
    float vol_ph        = adc2_avg * 0.1875F / 1000.0F;

    // 4. Đóng gói JSON
    String jsonPayload = "{";
    jsonPayload += "\"temperature\":" + String(temperature, 2) + ",";
    jsonPayload += "\"turbidity_vol\":" + String(vol_turbidity, 3) + ",";
    jsonPayload += "\"tds_vol\":" + String(vol_tds, 3) + ",";
    jsonPayload += "\"ph_vol\":" + String(vol_ph, 3);
    jsonPayload += "}";

    // 5. Bắn lên Firebase
    http.begin(firebaseURL);
    http.addHeader("Content-Type", "application/json");
    http.PUT(jsonPayload); 
    http.end();
  }
  
  delay(3000); // 3 giây cập nhật lên web 1 lần
}
