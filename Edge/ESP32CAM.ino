#include "CamConfig.h"
#include <U8g2lib.h>
#include <WiFi.h>
#include "esp_camera.h"
#include <PubSubClient.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif

const char* mqttServer = "0.0.0.0";              // MQTT 伺服器位址
const char* clientID = "";                       // 用戶端 ID
const char* mqttUserName = "";                   // 使用者名稱
const char* mqttPwd = "";                        // MQTT 密碼

const char* Pubtopic = "ESP32CAM/img";
const char* Subtopic = "ESP32CAM/msg";    

unsigned long prevMillis = 0;                           // 暫存經過時間 (毫秒)
const long interval = 20000;                            // 上傳資料的間隔時間: 20秒
String msgStr = "";                                     // 暫存 MQTT 訊息字串

WiFiClient espClient;
PubSubClient client(espClient);

#define KEY_PIN       13

U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE, 15, 14);

char ssid[] = "WiFi name";                                // your network SSID name
char pass[] = "WiFi password";                            // your network password

int buttonState = 0;
int btnHold = 0;

static camera_config_t camera_config = {
  .pin_pwdn = PWDN_GPIO_NUM,
  .pin_reset = RESET_GPIO_NUM,
  .pin_xclk = XCLK_GPIO_NUM,
  .pin_sscb_sda = SIOD_GPIO_NUM,
  .pin_sscb_scl = SIOC_GPIO_NUM,
  
  .pin_d7 = Y9_GPIO_NUM,
  .pin_d6 = Y8_GPIO_NUM,
  .pin_d5 = Y7_GPIO_NUM,
  .pin_d4 = Y6_GPIO_NUM,
  .pin_d3 = Y5_GPIO_NUM,
  .pin_d2 = Y4_GPIO_NUM,
  .pin_d1 = Y3_GPIO_NUM,
  .pin_d0 = Y2_GPIO_NUM,
  .pin_vsync = VSYNC_GPIO_NUM,
  .pin_href = HREF_GPIO_NUM,
  .pin_pclk = PCLK_GPIO_NUM,
  
  .xclk_freq_hz = 20000000,
  .ledc_timer = LEDC_TIMER_0,
  .ledc_channel = LEDC_CHANNEL_0,
  
  .pixel_format = PIXFORMAT_JPEG,
  .frame_size = FRAMESIZE_XGA,
  .jpeg_quality = 12,
  .fb_count = 1,
};

// 透過MQTT傳遞照片
void MQTT_picture() {
  //camera_fb_t * fb;                                     // camera frame buffer.
  camera_fb_t * fb = NULL;
  fb = esp_camera_fb_get();
  if (!fb) {
    delay(100);
    Serial.println("Camera capture failed, Reset");
    ESP.restart();
  }

  char* logIsPublished;

  client.subscribe(Pubtopic);
  if (! client.connected())
    logIsPublished = "  No MQTT Connection, Photo NOT Published !";
  else {
    int imgSize = fb->len;
    int ps = MQTT_MAX_PACKET_SIZE;
    // start to publish the picture
    client.beginPublish(Pubtopic, imgSize, false);
    for (int i = 0; i < imgSize; i += ps) {
      int s = (imgSize - i < s) ? (imgSize - i) : ps;
      client.write((uint8_t *)(fb->buf) + i, s);
    }

    boolean isPublished = client.endPublish();
    if (isPublished) {
      logIsPublished = "  Publishing Photo to MQTT Successfully !";

      // 使用自定義的字型
      u8g2.setFont(u8g2_font_unifont_t_chinese1); 
      u8g2.setFontDirection(0);
      u8g2.clearBuffer();
      u8g2.setCursor(0, 25);
      u8g2.print("Send Photo");
      u8g2.sendBuffer();
    }
    else {
      logIsPublished = "  Publishing Photo to MQTT Failed !";
    }
  }
  Serial.println(logIsPublished);

  // 清除緩衝區
  esp_camera_fb_return(fb);                                           
}

String getValue(String data, char separator, int index){
    int found = 0;
    int strIndex[] = { 0, -1 };
    int maxIndex = data.length() - 1;
    for (int i = 0; i <= maxIndex && found <= index; i++) {
        if (data.charAt(i) == separator || i == maxIndex) {
            found++;
            strIndex[0] = strIndex[1] + 1;
            strIndex[1] = (i == maxIndex) ? i+1 : i;
        }
    }
    return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}

void callback(char* topic, byte* payload, unsigned int length) {
  char BackString[30];
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] "); 
  
  for (int i = 0; i < length; i++) {
    BackString[i] = (char)payload[i];
  }
  String CallString(BackString);
  Serial.println();
  Serial.println(CallString);
  String s1=getValue(CallString,',', 0);
  String s2=getValue(CallString,',', 1);
  String s3=getValue(CallString,',', 2);

  // 使用自定義的字型
  u8g2.setFont(u8g2_font_unifont_t_chinese1); 
  u8g2.setFontDirection(0);
  u8g2.clearBuffer();
  u8g2.setCursor(0, 25);
  u8g2.print("准考證:" + s1);
  u8g2.setCursor(0, 40);
  u8g2.print("姓名:" + s2);
  u8g2.setCursor(0, 55);
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");

    // 使用自定義的字型
    u8g2.setFont(u8g2_font_unifont_t_chinese1); 
    u8g2.setFontDirection(0);
    u8g2.clearBuffer();
    u8g2.setCursor(0, 25);
    u8g2.print("MQTT connection");
    u8g2.sendBuffer();

    if (client.connect(mqttServer, mqttUserName, mqttPwd )) {
      Serial.println("connected");
    } else {
      delay(2000);
    }
 
  }
}

esp_err_t camera_init() {
  //initialize the camera
  esp_err_t err = esp_camera_init(&camera_config);
  if (err != ESP_OK) {
      Serial.print("Camera Init Failed");

      // 重新啟動 ESP32
      ESP.restart();  
      return err;
  }
  sensor_t * s = esp_camera_sensor_get();
  // initial sensors are flipped vertically and colors are a bit saturated
  if (s->id.PID == OV2640_PID) {
    /*
      s->set_vflip(s, 1);                               // flip it back
      s->set_brightness(s, 1);                          // up the blightness just a bit
      s->set_contrast(s, 1);
     */ 
  }
  
  Serial.println("Camera Init OK");
  return ESP_OK;
}

void setup_wifi() {
  delay(10);
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);

  // attempt to connect to Wifi network:
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP Address:");
  Serial.println(WiFi.localIP());
  
  //使用自定義的字型
  u8g2.setFont(u8g2_font_unifont_t_chinese1); 
  u8g2.setFontDirection(0);
  u8g2.clearBuffer();
  u8g2.setCursor(0, 25);
  u8g2.print("WiFi Connect");
  u8g2.sendBuffer();
}

void setup() {
  u8g2.begin();
  u8g2.enableUTF8Print();

  //Initialize serial and wait for port to open:
  Serial.begin(115200);
  while (!Serial) {
    ; 
    // wait for serial port to connect. Needed for native USB port only
  }

  // 初始化 WiFi與攝像頭
  setup_wifi();
  camera_init(); 

  pinMode(KEY_PIN, INPUT_PULLUP);

  // you're connected now, so print out the data:
  Serial.println("You're connected to the network");

  client.setServer(mqttServer, 1883);
  client.setCallback(callback);
 
  while (!client.connected()) {
    Serial.println("Connecting to MQTT...");

    if (client.connect(mqttServer, mqttUserName, mqttPwd )) {
      Serial.println("connected");  
    } else {
      Serial.print("failed with state ");
      Serial.print(client.state());
      delay(2000);
    }
 
  }
}

void loop() {
  buttonState = digitalRead(KEY_PIN); 
  // Serial.println(buttonState);
  if (buttonState == LOW) {
    // 等待跳過按鍵動的不穩定過程
    delay(100); 

    // 若按鍵被按下
    if (buttonState == LOW && btnHold == 0)   
    {
      Serial.println("Click...\r");  

      //使用自定義的字型
      u8g2.setFont(u8g2_font_unifont_t_chinese1); 
      u8g2.setFontDirection(0);
      u8g2.clearBuffer();
      u8g2.setCursor(0, 25);
      u8g2.print("Click...");
      u8g2.sendBuffer();
      btnHold = 1;  
      reconnect();

      // 用 MQTT 傳照片
      MQTT_picture(); 
      delay(1000);
    }
  }
  if (buttonState == HIGH)
  {
      btnHold = 0;
  }

  reconnect();
  client.subscribe(Subtopic);
  client.loop();
  callback;
  
  delay(2000);  
}
