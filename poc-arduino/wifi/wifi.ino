#include <SoftwareSerial.h>

#define DEBUG_WIFI 1
#define WIFI_TIMEOUT 3000

SoftwareSerial wifiSerial(10, 9);      // RX, TX for ESP8266

String influx_host = "192.168.2.31";
String influx_port = "8086";
String influx_db   = "planta";

int temperature = 43;

void setup()
{
  // Open serial communications and wait for port to open esp8266:
  Serial.begin(115200);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }
  wifiSerial.begin(115200);
  while (!wifiSerial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }

  wifiSerial.setTimeout(2000);

  if (!setupWifi()) {
    Serial.println("Failed to setup wifi... dying");
    while(1) {}
  }

  Serial.println("Sending data to Influx every 10 seconds.");
}

void loop()
{
  temperature++;
  sendTemperatureToInflux(temperature);
  delay(10000);
}

void sendTemperatureToInflux(int temperature) {
  String strData = "temperatura,local=tenda1 value="+ String(temperature);
  sendDataToInflux(strData);
}

boolean setupWifi()
{
  Serial.println("Setting up WiFi module");
  
  Serial.println("Restarting");
  if (sendDataToWifiWithRetry("AT+RST", "Jan", 2)) {
    Serial.println("Restarted. Changing mode.");
  } else {
    Serial.println("Can't find OK.. Continuing");
  }

  sendDataToWifi("AT+CWMODE=1"); // configure as client
  Serial.println("Configuring client-mode");

  if (sendDataToWifiWithRetry("AT+CWJAP=\"Meow\",\"2208Bobgigi\"", "OK", 2)) {
    Serial.println("Connected to SSID");
  }else {
    Serial.println("Can't connect to SSID");
    return false;
  }

  return true;
}

boolean sendDataToWifiWithRetry(String cmd, char* ok, int max_retries) {
  for (int i = 0; i < max_retries; i++)
  {
    String resp = sendDataToWifi(cmd);
    if (resp.indexOf(ok) >= 0) {
      return true;
    } else {
      Serial.println("Can't find " + String(ok) + ". Retrying.. " + String(i));
    }
  }

  return false;
}

String sendDataToWifi(String cmd) {
  if (DEBUG_WIFI) {
    Serial.println("[WiFi Debug Cmd] " + cmd);
  }

  wifiSerial.println(cmd);
  delay(WIFI_TIMEOUT);

  String resp;
  while (wifiSerial.available()) {
    resp += wifiSerial.readString();
  }

  if (DEBUG_WIFI) {
    Serial.println("[WiFi Debug Resp] " + resp);
  }

  return resp;
}

void sendDataToInflux(String data) {
  Serial.println("Sending data to Influx now.");

  String resp = sendDataToWifi("AT+CIPSTART=\"TCP\",\"" + influx_host + "\"," + influx_port);
  if (resp.indexOf("OK")) {
    Serial.println("TCP connection ready");
  } else {
    Serial.println("Can't start TCP connection");
    return;
  }

  String postRequest =
    String("POST /write?db=" + influx_db + " HTTP/1.0\r\nAccept: */*\r\n") +
    "Content-Length: " + data.length() + "\r\n" +
    "Content-Type: application/x-www-form-urlencoded\r\n" +
    "\r\n" + data;

  Serial.println("Transmitting...");
  sendDataToWifi("AT+CIPSEND=" + String(postRequest.length()));

  resp = sendDataToWifi(postRequest);
  if (resp.indexOf("SEND OK") >= 0) {
    Serial.println("Packet sent");
  } else {
    Serial.println("Can't send packet");
  }

  sendDataToWifi("AT+CIPCLOSE");
}
