#include <PMS.h>
#include <DHT.h>
#include <Adafruit_GFX.h>    
#include <Adafruit_ST7735.h> 
#include <SPI.h>
#include <ESP8266WiFi.h>
#include <Adafruit_MQTT.h>
#include <Adafruit_MQTT_Client.h>


#define WLAN_SSID       "KIKA"
#define WLAN_PASS       ""

#define MQTT_SERVER      "192.168.88.2"
#define MQTT_SERVERPORT  1883                   // use 8883 for SSL
#define MQTT_USERNAME    ""
#define MQTT_KEY         ""

WiFiClient client;
Adafruit_MQTT_Client mqtt(&client, MQTT_SERVER, MQTT_SERVERPORT, MQTT_USERNAME, MQTT_USERNAME, MQTT_KEY);

Adafruit_MQTT_Subscribe feedScreen = Adafruit_MQTT_Subscribe(&mqtt, "haklab/hardware/tinytft");
Adafruit_MQTT_Publish feedDust = Adafruit_MQTT_Publish(&mqtt, "haklab/hardware/dust");
Adafruit_MQTT_Publish feedHumid = Adafruit_MQTT_Publish(&mqtt, "haklab/hardware/humid");
Adafruit_MQTT_Publish feedTemp = Adafruit_MQTT_Publish(&mqtt, "haklab/hardware/temp");


#define DHTPIN 4     // what pin we're connected to
#define DHTTYPE DHT22   // DHT 22  (AM2302)

DHT dht(DHTPIN, DHTTYPE);
/*
 * ESP8266-12        HY-1.8 SPI
 * GPIO5             Pin 06 (RESET)
 * GPIO2             Pin 07 (A0)
 * GPIO13 (HSPID)    Pin 08 (SDA)
 * GPIO14 (HSPICLK)  Pin 09 (SCK)
 * GPIO15 (HSPICS)   Pin 10 (CS)
  */
/*
#define TFT_PIN_CS   15
#define TFT_PIN_DC   2
#define TFT_PIN_RST  5
*/
#define TFT_PIN_CS   D0
#define TFT_PIN_DC   D1
#define TFT_PIN_RST  10

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_PIN_CS, TFT_PIN_DC, TFT_PIN_RST);

PMS pms(Serial);
PMS::DATA data;

bool MQTT_connect();

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600); 
  dht.begin();
  
  tft.initR(INITR_BLACKTAB);  // You will need to do this in every sketch
  tft.fillScreen(ST7735_WHITE); 
 

  //tft print function!
  tft.setTextColor(ST7735_BLACK);
  tft.setTextSize(1);
  tft.setCursor(0,0);
  //tft.println("Hello World!");  
  delay(500);
  
  WiFi.begin(WLAN_SSID, WLAN_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("WiFi connected");
  Serial.println("IP address: "); Serial.println(WiFi.localIP());
}

int screenRow = 0;
int loopsForPing = 0;
const int avgLoops = 60;
float temps[avgLoops];
float humids[avgLoops];
float pm1s[avgLoops];
float pm25s[avgLoops];
float pm10s[avgLoops];
int looper = 0;

void loop() {
  bool mqttconnected = MQTT_connect();

  float h = dht.readHumidity();
  // Read temperature as Celsius
  float t = dht.readTemperature();

  tft.print("Humidity: "); 
  tft.print(h);
  humids[looper] = h;

  tft.println("%");
  tft.print("Temperature: "); 
  tft.print(t);
  tft.println("*C");
  temps[looper] = t;

  bool pmsgood = false;
  if (pms.read(data, 500))
  {
    pmsgood = true;
    
    float dust1 = data.PM_AE_UG_1_0;
    tft.print("PM 1.0 (ug/m3): ");
    tft.println(dust1);
    pm1s[looper] = dust1;

    float dust25 = data.PM_AE_UG_2_5;
    tft.print("PM 2.5 (ug/m3): ");
    tft.println(dust25);
    pm25s[looper] = dust25;

    float dust10 = data.PM_AE_UG_10_0;
    tft.print("PM 10.0 (ug/m3): ");
    tft.println(dust10);
    pm10s[looper] = dust10;  
  }

  looper++;
  if (looper == avgLoops) {
    if (mqttconnected) {
      tft.println("Sending data to MQTT");
      feedTemp.publish(getaverage(temps));
      feedHumid.publish(getaverage(humids));
      if (pmsgood) {
        feedDust.publish(strplusfloat("PM1.0 ", getaverage(pm1s)));
        feedDust.publish(strplusfloat("PM2.5 ", getaverage(pm25s)));
        feedDust.publish(strplusfloat("PM10 ", getaverage(pm10s)));
      }
    }
    looper = 0;
  }
  
  tft.println("");
  delay(1000);

  screenRow = screenRow + 1;
  if (screenRow == 3) {
    tft.fillScreen(ST7735_WHITE); 
    tft.setCursor(0,0);
    screenRow = 0;
  }
  

  // ping the server to keep the mqtt connection alive
  if (loopsForPing > 30) {
    if(!mqtt.ping())
      mqtt.disconnect();
    loopsForPing = 0;
  }
  loopsForPing++;
}


// Function to connect and reconnect as necessary to the MQTT server.
// Should be called in the loop function and it will take care if connecting.
bool MQTT_connect() {
  int8_t ret;

  // Stop if already connected.
  if (mqtt.connected()) {
    return true;
  }

  Serial.print("Connecting to MQTT... ");

  uint8_t retries = 5; // retry for one minute
  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
    Serial.println(mqtt.connectErrorString(ret));
    Serial.println("Retrying MQTT connection in 5 seconds...");
    mqtt.disconnect();
    
    delay(1000);  // wait 1 second
    retries--;
    if (retries == 0) {
      return false;
    }
  }
  Serial.println("MQTT Connected!");
  return true;
}

const char* strplusfloat(String str, float f) {
  String res = str + String(f);
  const char *ch = res.c_str();
  return ch;
}

float getaverage(float *floats)  // assuming array is int.
{
  float sum = 0;
  for (int i = 0 ; i < avgLoops ; i++)
    sum += floats[i];
  return  sum / avgLoops;
}

