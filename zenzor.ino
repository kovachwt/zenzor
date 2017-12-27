#include <PMS.h>
#include <DHT.h>
#include <Adafruit_GFX.h>    
#include <Adafruit_ST7735.h> 
#include <SPI.h>
 
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
  
}

int i = 0;

void loop() {
  float h = dht.readHumidity();
  // Read temperature as Celsius
  float t = dht.readTemperature();

  tft.print("Humidity: "); 
  tft.print(h);
  tft.println("%");
  tft.print("Temperature: "); 
  tft.print(t);
  tft.println("*C");
  if (pms.read(data, 2000))
  {
    tft.print("PM 1.0 (ug/m3): ");
    tft.println(data.PM_AE_UG_1_0);

    tft.print("PM 2.5 (ug/m3): ");
    tft.println(data.PM_AE_UG_2_5);

    tft.print("PM 10.0 (ug/m3): ");
    tft.println(data.PM_AE_UG_10_0);  
  }
  tft.println("");
  delay(1000);

  i = i + 1;
  if (i == 3) {
    i = 0;
    delay(5000);
    tft.fillScreen(ST7735_WHITE); 
    tft.setCursor(0,0);
  }
  
}

