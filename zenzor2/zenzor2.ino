#include <DHT.h>
#include <ESP8266WiFi.h>
#include <Adafruit_MQTT.h>
#include <Adafruit_MQTT_Client.h>
#include <SoftwareSerial.h>


#define WLAN_SSID       "KIKA"
#define WLAN_PASS       ""

#define MQTT_SERVER      "192.168.88.2"
#define MQTT_SERVERPORT  1883                   // use 8883 for SSL
#define MQTT_USERNAME    ""
#define MQTT_KEY         ""

WiFiClient client;
Adafruit_MQTT_Client mqtt(&client, MQTT_SERVER, MQTT_SERVERPORT, MQTT_USERNAME, MQTT_USERNAME, MQTT_KEY);

Adafruit_MQTT_Publish feedHumid = Adafruit_MQTT_Publish(&mqtt, "k0doma/vlazhnost");
Adafruit_MQTT_Publish feedTemp = Adafruit_MQTT_Publish(&mqtt, "k0doma/toplina");
Adafruit_MQTT_Publish feedGas = Adafruit_MQTT_Publish(&mqtt, "k0doma/combustible");
Adafruit_MQTT_Publish feedVoc = Adafruit_MQTT_Publish(&mqtt, "k0doma/volatile");


#define DHTPIN   D1      // what pin we're connected to
#define DHTTYPE  DHT22   // DHT 22  (AM2302)

DHT dht(DHTPIN, DHTTYPE);

#define ARDU_RX_PIN      D6
#define ARDU_TX_PIN      D7

SoftwareSerial arduserial(ARDU_RX_PIN, ARDU_TX_PIN);

bool MQTT_connect();

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200); 
  arduserial.begin(4800);
  dht.begin();
  
  WiFi.begin(WLAN_SSID, WLAN_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("WiFi connected");
  Serial.println("IP address: "); Serial.println(WiFi.localIP());
}

int loopsForPing = 0;
const int avgLoops = 60;
float temps[avgLoops];
float humids[avgLoops];
float gass[avgLoops];
float vocs[avgLoops];
int looper = 0;
float prevGas = 0;
float prevVoc = 0;

void loop() {
  bool mqttconnected = MQTT_connect();

  float h = dht.readHumidity();
  float t = dht.readTemperature();

  if (isnan(h) || isnan(t))
    ESP.restart();

  humids[looper] = h;
  temps[looper] = t;

  Serial.print("H=");
  Serial.println(h);
  Serial.print("T=");
  Serial.println(t);

  float gas = prevGas;
  float voc = prevVoc;
  String line = arduserial.readString();
  //Serial.println(line);
  line.trim();
  int lidx = line.indexOf('\n');
  if (lidx > -1) {
    String line1 = line.substring(0, lidx);
    line1.trim();
    Serial.print("L1= ");
    Serial.println(line1);

    int l2idx = line.indexOf('\n', lidx+1);
    if (l2idx > lidx) {
      String line2 = line.substring(lidx+1, l2idx);
      line2.trim();
      Serial.print("L2= ");
      Serial.println(line2);

      if (line1.charAt(0) == 'G')
        parseGasVoc(line1, line2, &gas, &voc);
      else if (line1.charAt(0) == 'V')
        parseGasVoc(line2, line1, &gas, &voc);
    }
  }
  Serial.print("G=");
  Serial.println(gas);
  Serial.print("V=");
  Serial.println(voc);
  gass[looper] = gas;
  vocs[looper] = voc;
  
  looper++;
  if (looper == avgLoops) {
    if (mqttconnected) {
      feedTemp.publish(getaverage(temps));
      feedHumid.publish(getaverage(humids));
      feedGas.publish(getaverage(gass));
      feedVoc.publish(getaverage(vocs));
    }
    looper = 0;
  }

  // ping the server to keep the mqtt connection alive
  if (loopsForPing > 30) {
    if(!mqtt.ping())
      mqtt.disconnect();
    loopsForPing = 0;
  }
  loopsForPing++;
}

void parseGasVoc(String linegas, String linevoc, float *gas, float *voc) {
  if (linegas.charAt(0) == 'G' && linegas.charAt(1) == '=') {
    String gasval = linegas.substring(2);
    float tmpgas = gasval.toFloat();
    if (!isnan(tmpgas) && tmpgas != 0)
      *gas = tmpgas;
  }
  if (linevoc.charAt(0) == 'V' && linevoc.charAt(1) == '=') {
    String vocval = linevoc.substring(2);
    float tmpvoc = vocval.toFloat();
    if (!isnan(tmpvoc) && tmpvoc != 0)
      *voc = tmpvoc;
  }  
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

