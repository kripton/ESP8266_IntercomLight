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

// Locally attached LED pixel string (P1)
#define LED1_DATA 19
//#define     LED1_CLK      23
#define LED1_NUMLEDS 12

unsigned long debounceTime = 500; // ms
unsigned int pingCount = 0;

///// Globals /////
NeoPixelBus<NeoGrbFeature, Neo800KbpsMethod> pixels(LED1_NUMLEDS, LED1_DATA);
unsigned long lastDetection = 0;

// Current state of running calls, talks or texts per channel
DynamicJsonDocument state(2048);

// The pre-defined steps for a call, talk or text
DynamicJsonDocument animationSteps(2048);

// The actual running animations
DynamicJsonDocument animations(2048);

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
  pixels.ClearTo(RgbColor(0, 0, 0));
  for (int i = 0; i < LED1_NUMLEDS; i++)
  {
    pixels.SetPixelColor(i, RgbColor((i * 20) + 10, 0, 0));
  }
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

  OscWiFi.subscribe(53000, "/intercom/*/*", [](const OscMessage &m) {
    // Split the address to get the channel and property names
    String chan = m.address().substring(10, 11);
    String property = m.address().substring(12);
    Serial.print(chan);
    Serial.print(" ");
    Serial.print(property);
    Serial.print(" ");

    state[chan][property] = m.arg<int>(0);

    Serial.print(m.remoteIP());
    Serial.print(" ");
    Serial.print(m.remotePort());
    Serial.print(" ");
    Serial.print(m.size());
    Serial.print(" ");
    Serial.print(m.address());
    Serial.print(" ");
    Serial.print(m.arg<int>(0));
    if (m.size() > 1) {
      Serial.print(" ");
      Serial.print(m.arg<int>(1));
      Serial.print(" ");
      Serial.print(m.arg<int>(2));

      state[chan]["color_r"] = m.arg<int>(0);
      state[chan]["color_g"] = m.arg<int>(1);
      state[chan]["color_b"] = m.arg<int>(2);
    } else {
      // It's not a color information => time to add or remove an animation
      if (state[chan][property]) {
        // Add an animation
        StaticJsonDocument<512> doc;
        JsonObject anim = doc.to<JsonObject>();
        anim["color_r"] = state[chan]["color_r"];
        anim["color_g"] = state[chan]["color_g"];
        anim["color_b"] = state[chan]["color_b"];
        anim["curstep"] = 0;
        anim["type"] = property;
        serializeJson(anim, Serial);
        animations[chan + property] = anim;
      } else {
        // Find the animation and remove it
        animations.remove(chan + property);
      }
    }
    Serial.println();
  });

  // Set the LEDs to GREEN briefly
  pixels.ClearTo(RgbColor(0, 0, 0));
  for (int i = 0; i < LED1_NUMLEDS; i++)
  {
    pixels.SetPixelColor(i, RgbColor(0, (i * 20) + 10, 0));
  }
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

  pixels.ClearTo(RgbColor(0, 0, 0));
  for (int i = 0; i < LED1_NUMLEDS; i++)
  {
    ledValues[i][0] = 0;
    ledValues[i][1] = 0;
    ledValues[i][2] = 0;
  }

  // Iterate over all running animations and add together the LED's values
  for (JsonObject::iterator it=animations.as<JsonObject>().begin(); it!=animations.as<JsonObject>().end(); ++it) {
    Serial.printf("Animation \"%s\"\n", it->key());
    JsonObject anim = it->value();

    Serial.printf("CurStep: %d\n", anim["curstep"].as<int>());

    for (int i = 0; i < LED1_NUMLEDS; i++)
    {
      float factor = animationSteps[anim["type"].as<String>()][i%2 ? "odd": "even"][anim["curstep"].as<int>()].as<float>();
      Serial.printf("LED %d OddEven: %s Factor: %f\n", i, i%2 ? "odd": "even", factor);
      serializeJson(animationSteps[anim["type"].as<String>()][i%2 ? "odd": "even"][anim["curstep"].as<int>()], Serial);
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

  for (int i = 0; i < LED1_NUMLEDS; i++)
  {
    if (ledValues[i][0] > 255) {
      ledValues[i][0] = 255;
    }
    if (ledValues[i][1] > 255) {
      ledValues[i][1] = 255;
    }
    if (ledValues[i][2] > 255) {
      ledValues[i][2] = 255;
    }
    pixels.SetPixelColor(i, RgbColor(ledValues[i][0], ledValues[i][1],ledValues[i][2]));
  }
  pixels.Show();

  delay(50);
}
