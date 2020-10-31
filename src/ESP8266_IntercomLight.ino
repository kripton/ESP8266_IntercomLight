///// INCLUDES /////
#include <ESP8266WiFi.h>
#include <ArduinoOSC.h>
#include <NeoPixelBus.h>
#include <ArduinoJson.h>

///// CONFIG / CONSTANTS /////
const char *wifi_ssid = "Netgear";
const char *wifi_password = "0x01234%&/";

//const char* wifi_ssid = "HBVA";
//const char* wifi_password =  "HammerLicht159";

IPAddress my_ip(192, 168, 2, 231);
IPAddress my_gw(0, 0, 0, 0);
IPAddress my_netmask(255, 255, 255, 0);

// Locally attached LED pixel string + ring (Pin: RX hardcoded for ESP8266)
#define LED1_NUMLEDS 20 // 8 String + 12 Ring

unsigned long debounceTime = 500; // ms
unsigned int pingCount = 0;
unsigned int connected = 0;

///// Globals /////
NeoPixelBus<NeoGrbFeature, Neo800KbpsMethod> pixels(LED1_NUMLEDS);
unsigned long lastDetection = 0;

// Current state of running calls, talks or texts per channel
DynamicJsonDocument state(2048);

// The pre-defined steps for a call, talk or text
DynamicJsonDocument animationSteps(2048);

// The actual running animations
DynamicJsonDocument animations(4096);

// To add together the values of the individual animations
unsigned long ledValues[LED1_NUMLEDS][3];

// Define LED "animation steps"
void initAnimationSteps()
{
  animationSteps["call"]["odd"][0] = 0.5;
  animationSteps["call"]["odd"][1] = 1;
  animationSteps["call"]["odd"][2] = 1;
  animationSteps["call"]["odd"][3] = 1;
  animationSteps["call"]["odd"][4] = 1;
  animationSteps["call"]["odd"][5] = 1;
  animationSteps["call"]["odd"][6] = 0.8;
  animationSteps["call"]["odd"][7] = 0.6;
  animationSteps["call"]["odd"][8] = 0.4;
  animationSteps["call"]["odd"][9] = 0.2;
  animationSteps["call"]["even"][0] = 0.5;
  animationSteps["call"]["even"][1] = 1;
  animationSteps["call"]["even"][2] = 1;
  animationSteps["call"]["even"][3] = 1;
  animationSteps["call"]["even"][4] = 1;
  animationSteps["call"]["even"][5] = 1;
  animationSteps["call"]["even"][6] = 0.8;
  animationSteps["call"]["even"][7] = 0.6;
  animationSteps["call"]["even"][8] = 0.4;
  animationSteps["call"]["even"][9] = 0.2;
  animationSteps["talk"]["odd"][0] = 0.1;
  animationSteps["talk"]["odd"][1] = 0.1;
  animationSteps["talk"]["odd"][2] = 0.1;
  animationSteps["talk"]["odd"][3] = 0.1;
  animationSteps["talk"]["odd"][4] = 0.1;
  animationSteps["talk"]["odd"][5] = 0.1;
  animationSteps["talk"]["odd"][6] = 0.1;
  animationSteps["talk"]["odd"][7] = 0.1;
  animationSteps["talk"]["odd"][8] = 0.1;
  animationSteps["talk"]["odd"][9] = 0.1;
  animationSteps["talk"]["even"][0] = 0.1;
  animationSteps["talk"]["even"][1] = 0.1;
  animationSteps["talk"]["even"][2] = 0.1;
  animationSteps["talk"]["even"][3] = 0.1;
  animationSteps["talk"]["even"][4] = 0.1;
  animationSteps["talk"]["even"][5] = 0.1;
  animationSteps["talk"]["even"][6] = 0.1;
  animationSteps["talk"]["even"][7] = 0.1;
  animationSteps["talk"]["even"][8] = 0.1;
  animationSteps["talk"]["even"][9] = 0.1;
  animationSteps["text"]["odd"][0] = 0.2;
  animationSteps["text"]["odd"][1] = 0.2;
  animationSteps["text"]["odd"][2] = 0.2;
  animationSteps["text"]["odd"][3] = 0.2;
  animationSteps["text"]["odd"][4] = 0.2;
  animationSteps["text"]["odd"][5] = 0;
  animationSteps["text"]["odd"][6] = 0;
  animationSteps["text"]["odd"][7] = 0;
  animationSteps["text"]["odd"][8] = 0;
  animationSteps["text"]["odd"][9] = 0;
  animationSteps["text"]["even"][0] = 0;
  animationSteps["text"]["even"][1] = 0;
  animationSteps["text"]["even"][2] = 0;
  animationSteps["text"]["even"][3] = 0;
  animationSteps["text"]["even"][4] = 0;
  animationSteps["text"]["even"][5] = 0.4;
  animationSteps["text"]["even"][6] = 0.4;
  animationSteps["text"]["even"][7] = 0.4;
  animationSteps["text"]["even"][8] = 0.4;
  animationSteps["text"]["even"][9] = 0.4;
}

void setup()
{
  Serial.begin(115200);
  Serial.println("IL3000 getting ready :)");

  // Initialize the LED string in RED
  pixels.Begin();
  pixels.ClearTo(RgbColor(255, 0, 0));
  pixels.Show();

  // Wifi config + connect
  WiFi.mode(WIFI_STA);
  WiFi.config(my_ip, my_gw, my_netmask);
  WiFi.begin(wifi_ssid, wifi_password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
  }

  initAnimationSteps();

  OscWiFi.subscribe(53000, "/intercom/*", [](const OscMessage &m) {
    uint16 fieldsConsumed = 0;

    // Split the address to get the channel and property names
    String type = m.address().substring(10);

    Serial.printf("OSC FROM %s:%d. \"%s\" Size: %d\n", m.remoteIP().c_str(), m.remotePort(), m.address().c_str(), m.size());

    if (type == "chanColors") {
      while (fieldsConsumed < m.size()) {
        String chanName = m.arg<String>(fieldsConsumed);
        state[chanName]["color_r"] = m.arg<int>(fieldsConsumed + 1);
        state[chanName]["color_g"] = m.arg<int>(fieldsConsumed + 2);
        state[chanName]["color_b"] = m.arg<int>(fieldsConsumed + 3);
        fieldsConsumed += 4;
      }
      serializeJson(state, Serial);
      connected = 1;
    } else if (type == "call" || type == "talk" || type == "text") {
      String chan = m.arg<String>(0);
      Serial.printf("Chan: %s ÖnÖff: %d\n", chan.c_str(), m.arg<int>(1));
      if (m.arg<int>(1)) {
        // Add an animation
        StaticJsonDocument<256> doc;
        JsonObject anim = doc.to<JsonObject>();
        anim["color_r"] = state[chan]["color_r"];
        anim["color_g"] = state[chan]["color_g"];
        anim["color_b"] = state[chan]["color_b"];
        anim["curstep"] = 0;
        anim["type"] = type;
        //serializeJson(anim, Serial);
        animations[chan + type] = anim;
      } else {
        // Find the animation and remove it
        animations.remove(chan + type);
        animations.garbageCollect();
      }
    }
  });

  // Set the LEDs to YELLOW (= waiting for Callboy)
  pixels.ClearTo(RgbColor(255, 255, 0));
  pixels.Show();

  // Wait for the OSC message to come in
  while (!connected) {
    OscWiFi.update();
    pixels.Show();
    delay(200);
    OscWiFi.send("255.255.255.255", 57121, "/intercom/ping");
  }

  // Set the LEDs to GREEN briefly
  pixels.ClearTo(RgbColor(0, 255, 0));
  pixels.Show();
  delay(500);

  // Clear LEDs
  pixels.ClearTo(RgbColor(0, 0, 0));
  pixels.Show();
}

// Looooooooop
void loop()
{
  OscWiFi.update(); // should be called

  // Initialise all to 0
  pixels.ClearTo(RgbColor(0, 0, 0));
  for (int i = 0; i < LED1_NUMLEDS; i++)
  {
    ledValues[i][0] = 0;
    ledValues[i][1] = 0;
    ledValues[i][2] = 0;
  }

  // Iterate over all running animations and add together the LED's values
  for (JsonObject::iterator it=animations.as<JsonObject>().begin(); it!=animations.as<JsonObject>().end(); ++it) {
    //Serial.printf("Animation \"%s\"\n", it->key());
    JsonObject anim = it->value();

    //Serial.printf("CurStep: %d\n", anim["curstep"].as<int>());

    for (int i = 8; i < LED1_NUMLEDS; i++)
    {
      float factor = animationSteps[anim["type"].as<String>()][i%2 ? "odd": "even"][anim["curstep"].as<int>()].as<float>();
      //Serial.printf("LED %d OddEven: %s Factor: %f\n", i, i%2 ? "odd": "even", factor);
      //serializeJson(animationSteps[anim["type"].as<String>()][i%2 ? "odd": "even"][anim["curstep"].as<int>()], Serial);
      ledValues[i][0] += anim["color_r"].as<int>() * factor;
      ledValues[i][1] += anim["color_g"].as<int>() * factor;
      ledValues[i][2] += anim["color_b"].as<int>() * factor;
    }

    // Increase curstep with overflow
    anim["curstep"] = anim["curstep"].as<int>() + 1;
    if (anim["curstep"] == 10) {
      anim["curstep"] = 0;
    }
  }

  // Now display the current config (Signal Channels and CALL Channels)
  // on the LED string


  for (int i = 0; i < LED1_NUMLEDS; i++)
  {
    // Cap the values to 255
    if (ledValues[i][0] > 255) {
      ledValues[i][0] = 255;
    }
    if (ledValues[i][1] > 255) {
      ledValues[i][1] = 255;
    }
    if (ledValues[i][2] > 255) {
      ledValues[i][2] = 255;
    }
    // And write the ledValues to the actual pixel object
    pixels.SetPixelColor(i, RgbColor(ledValues[i][0], ledValues[i][1],ledValues[i][2]));
  }
  pixels.Show();

  delay(10);
  OscWiFi.update();
  delay(10);
  OscWiFi.update();
  delay(10);
  OscWiFi.update();
  delay(10);
  OscWiFi.update();
  delay(10);
}
