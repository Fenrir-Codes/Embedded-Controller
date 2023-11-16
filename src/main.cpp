/**
 * @file main.cpp
 * @author Jozsef Bali
 * @brief 
 * @version 0.1
 * @date 2023-11-16
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <ESPAsyncWebServer.h>

/**
 * @brief Setup oneWire instance.
 * Setup oneWire instance to communicatemed with OneWire devices
 */
#define ONE_WIRE_BUS 4
OneWire oneWire(ONE_WIRE_BUS); /**< Setup oneWire instance to communicate with OneWire devices */
DallasTemperature sensors(&oneWire); /**< oneWire reference to Dallas Temperature sensor */

/**
 * @brief Temperature variables.
 * 
 */
String temperatureC = ""; /**< Current temperature in Celsius */
String ComfortTemperatureC = "22"; /**< Comfort temperature setting in Celsius */
String ConserveTemperatureC = "16"; /**< Conserve temperature setting in Celsius */
const char *ComfortParameter = "comfort_input"; /**< Comfort temperature input parameter name */
const char *ConserveParameter = "conserve_input"; /**< Conserve temperature input parameter name */


/**
 * @brief Boolean for state of the heater.
 * 
 * If true the heater is ON indicating the heating turned on
 * if OFF the heater is turned off.
 * 
 */
bool heaterState = false; /**< Heater state: true if ON, false if OFF */

/**
 * @brief Wi-fi credentials
 * 
 * used to connet to the local wifi.
 * 
 * ssid: This variable holds the SSID (Service Set Identifier) of the Wi-Fi network to which the ESP8266 will connect.
 * password:The password is used to authenticate and secure the connection to the Wi-Fi network.
 * 
 * These credentials are then used in the WiFi.begin(ssid, password); statement in the setup() function to initiate the connection to the Wi-Fi network.
 * The ESP8266 will attempt to connect to the Wi-Fi network using the provided SSID and password during runtime.
 * 
 */
const char *ssid = "E308";
const char *password = "98806829";

/**
 * @brief Make of the AsyncWebServer object on port 80.
 * 
 *  it's setting up a web server that will handle incoming HTTP requests on port 80. 
 *  Port 80 is the default port for HTTP communication. This server will be responsible for processing requests and responding accordingly.
 */
AsyncWebServer server(80); /**< Make of the AsyncWebServer object on port 80 */

/**
 * @brief HTML content.
 * 
  * HTML content stored in PROGMEM
  * HTML content inside this array is essentially the structure of an HTML page with embedded JavaScript.
  * 
 */
const char webpage_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="stylesheet" href="https://use.fontawesome.com/releases/v5.7.2/css/all.css" integrity="sha384-fnmOCqbTlWIlj8LyTjo7mOUStjsKC4pOpQbqyi7RrhN7udi9RwhKkMHpvLbHG9Sr" crossorigin="anonymous">
  <style>
    html {
      font-family: Arial;
      display: inline-block;
      margin: 0px auto;
      text-align: center;
    }
    h2 { font-size: 3.0rem; }
    p { font-size: 3.0rem; }
    .units { font-size: 1.2rem; }
    .ds-labels {
      font-size: 1.5rem;
      vertical-align: middle;
      padding-bottom: 15px;
    }
  </style>
</head>
<body>
  <h2>SmartWarmth Station</h2>
  <p>
    <i class="fas fa-thermometer-half" style="color:#059e8a;"></i>
    <span class="ds-labels">Current Temperature in Celsius</span>
    <span id="temperaturec">%TEMPERATUREC%</span>
    <sup class="units">&deg;C</sup>
  </p>
  <p id='heaterStatus'>Heater is %HEATER_STATE%</p>
  <form action="/desiredc" id="desireForm">
     Comfort Temperature <input type="number" step="0.1" name="comfort_input" value="%COMFORT%" required><br>
     Conserve Temperature <input type="number" step="0.1" name="conserve_input" value="%CONSERVE%" required><br>
    <input id="submit_btn" type="submit" value="Submit">
  </form>
</body>
<script>
setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("temperaturec").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/temperaturec", true);
  xhttp.send();
}, 10000) ;
</script>
</html>)rawliteral";

/**
 * @brief Process placeholders in HTML content.
 *
 * This function replaces placeholders in the HTML content with actual values.
 *
 * @param var The placeholder variable to be replaced.
 * @return The replaced value.
 */
String processor(const String &var) {
  if (var == "TEMPERATUREC") {
    return temperatureC; // Replace with the current temperature in Celsius
  } else if (var == "COMFORT") {
    return ComfortTemperatureC; /**< Replace with the comfort temperature setting in Celsius */
  } else if (var == "CONSERVE") {
    return ConserveTemperatureC; /**< Replace with the conserve temperature setting in Celsius */
  } else if (var == "HEATER_STATE") {
    return heaterState ? "ON" : "OFF"; /**< Replace with "ON" if the heater is on, "OFF" otherwise */ 
  }
  return String(); /**< Return an empty string if the placeholder is not recognized */ 
}

/**
 * @brief Read temperature from DS18B20 sensor.
 *
 * This function reads the temperature from the DS18B20 sensor.
 *
 * @return The temperature in Celsius.
 */
String readDSTemperatureC() {
  sensors.requestTemperatures(); /**< Initiates a temperature reading from the DS18B20 sensor */
  float tempC = sensors.getTempCByIndex(0); /**< Retrieves the temperature in Celsius from the sensor */

  if (tempC == -127.00) {
    Serial.println("Failed to read from temperature sensor");
    return "--"; /**< Return "--" if reading from the sensor fails */
  } else {
    Serial.print("Temperature Celsius: ");
    Serial.println(tempC); /**< Print the temperature value to the serial monitor */
  }

  return String(tempC); /**< Return the temperature value as a String */
}

/**
 * @brief Control heater based on temperature settings.
 *
 * This function checks the current temperature against the comfort temperature
 * and adjusts the heater state accordingly.
 */
void controlHeater() {
  float currentTemp = temperatureC.toFloat(); /**< Convert current temperature to a float */
  float comfortTemp = ComfortTemperatureC.toFloat(); /**< Convert comfort temperature to a float */

  if (currentTemp < comfortTemp) {
    heaterState = true; /**< Turn on heater if current temperature is lower than comfort temperature */
  } else {
    heaterState = false; /**< Turn off heater otherwise */
  }
}

/**
 * @brief Setup function.
 *
 * This function initializes the necessary components, sets up the server,
 * Initializes the DS18B20 temperature sensor,
 * connects to Wi-Fi, and configures API endpoints.
 */
void setup() {
  Serial.begin(115200); /**< Initialize serial communication with a baud rate of 115200 */
  sensors.begin(); /**< Initialize the DS18B20 temperature sensor */
  temperatureC = readDSTemperatureC(); /**< Read the initial temperature from the sensor */
  WiFi.begin(ssid, password); /**< Connect to Wi-Fi using the provided SSID and password */
  Serial.println("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.println(WiFi.localIP()); /**< Print the local IP address after successful Wi-Fi connection */

  // Configure HTTP endpoints
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", webpage_html, processor);
  });

  server.on("/desiredc", HTTP_GET, [](AsyncWebServerRequest *request) {
    // Update comfort and conserve temperatures if provided in the request parameters
    if (request->hasParam(ComfortParameter)) {
      ComfortTemperatureC = request->getParam(ComfortParameter)->value();
    }
    if (request->hasParam(ConserveParameter)) {
      ConserveTemperatureC = request->getParam(ConserveParameter)->value();
    }
    controlHeater(); /**< Check heater state based on the set temperatures */
    request->send_P(200, "text/html", webpage_html, processor);
  });

  server.on("/temperaturec", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/plain", temperatureC.c_str());
  });

  server.begin(); /**< Start the web server */
}

/**
 * @brief Loop function.
 *
 * This function continuously reads the temperature, checks the heater state
 * based on the set temperatures, and introduces a delay of 1 minute.
 */
void loop() {
  temperatureC = readDSTemperatureC(); /**< Read the current temperature from the DS18B20 sensor */
  controlHeater(); /**< Check the heater state based on the set temperatures */
  delay(60000); /**< Introduce a delay of 1 minute (60,000 milliseconds) */
}
