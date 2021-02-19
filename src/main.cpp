#include <Arduino.h>
#include <WiFi.h>
#include <WiFiMulti.h>
#include <WiFiClientSecure.h>
#include <WebSocketsClient.h>
#include <constants.h>
#include <ArduinoJson.h>
#include <FastLED.h>
#include <SHA256.h>
#include <Base64.h>

/* YOU MUST EDIT "constants.h" file to input individual specific settings */

/*
  This code was hacked together and could use lots of improvments, I'm not a C dev, but it works okay for what it needs to do.
  Hardcoded message-id's are used since I'm lazy and they work.
  This is designed to interface with OBS through the web-socket plugin: https://github.com/Palakis/obs-websocket
*/

#define NUM_LEDS 10
#define LED_BRIGHTNESS 10 // between 0-255, where 255 is max brightness
#define DATA_PIN 13

CRGB leds[NUM_LEDS];

WiFiMulti WiFiMulti;
WebSocketsClient webSocket;

#define HASH_SIZE 32
SHA256 sha256;

void setLed_STREAMING() {
  fill_solid(leds, NUM_LEDS, CRGB::Green);
  FastLED.show();
}

void setLed_NOT_STREAMING() {
  fill_solid(leds, NUM_LEDS, CRGB::Red);
  FastLED.show();
}

void setLed_NOT_CONNECTED() {
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  leds[0] = CRGB::Purple;
  leds[NUM_LEDS -1] = CRGB::Purple;
  FastLED.show();
}

void handleWebSocketEvent(WStype_t type, uint8_t * payload, size_t length) {

	switch(type) {
		case WStype_DISCONNECTED:
			Serial.printf("[WSc] Disconnected!\n");
      setLed_NOT_CONNECTED();
			break;
		case WStype_CONNECTED:
			Serial.printf("[WSc] Connected to url: %s\n", payload);

      // Obs websocket auth happens after connecting, process kicked off with GetAuthRequired request, responce handled below in case WStype_TEXT
      // https://github.com/Palakis/obs-websocket/blob/4.x-current/docs/generated/protocol.md#authentication
      
      webSocket.sendTXT("{\"request-type\":\"GetAuthRequired\",\"message-id\":\"1\"}");
			break;
		case WStype_TEXT: {
        Serial.printf("[WSc] get text: %s\n", payload);

        StaticJsonDocument<5000> doc;
        DeserializationError error = deserializeJson(doc, payload);
        if (error) {
          Serial.print(F("deserializeJson() failed: "));
          Serial.println(error.f_str());
          setLed_NOT_CONNECTED(); // indicate potential issue if JSON parsing failed, could be message was too large
          // recover by asking for streaming status, if reply is recieved LED will update to current state
          webSocket.sendTXT("{\"request-type\":\"GetStreamingStatus\",\"message-id\":\"3\"}");
          break;
        }

        /* Handle GetAuthRequired response & do auth */
        if (doc.containsKey("authRequired")) {
          Serial.println("handling auth");
          sha256.reset();
          sha256.update(OBS_PASS, strlen(OBS_PASS));
          const char* salt = doc["salt"];
          sha256.update(salt, strlen(salt));
          char value[HASH_SIZE];
          sha256.finalize(value, HASH_SIZE);
          // Serial.print("sha256 is:"); // this is super helpful debugging auth
          // for (size_t i = 0; i < HASH_SIZE; i++) {
          //   Serial.print(static_cast<unsigned int>(value[i]), HEX);
          // }
          // Serial.println();

          int encodedLength = Base64.encodedLength(HASH_SIZE);
          char encodedPassSaltHash[encodedLength];
          Base64.encode(encodedPassSaltHash, value, HASH_SIZE);
 

          const char* challenge = doc["challenge"];
          sha256.reset();
          sha256.update(encodedPassSaltHash, encodedLength);
          sha256.update(challenge, strlen(challenge));
          sha256.finalize(value, HASH_SIZE);
          // Serial.print("sha256 is:");
          // for (size_t i = 0; i < HASH_SIZE; i++) {
          //   Serial.print(static_cast<unsigned int>(value[i]), HEX);
          // }
          // Serial.println();

          char encodedAuthString[encodedLength];
          Base64.encode(encodedAuthString, value, HASH_SIZE);

          String authRequest = String("{\"request-type\":\"Authenticate\",\"message-id\":\"2\",\"auth\":\"") + encodedAuthString + "\"}";
          webSocket.sendTXT(authRequest);
          break;
        }

        /* Handle Authenticate response */
        if (doc.containsKey("message-id") && (doc["message-id"]) == "2") {
          if (strcmp(doc["status"], "ok") == 0) {
            Serial.println("Authenticated successfully");
            // request current streaming state
            webSocket.sendTXT("{\"request-type\":\"GetStreamingStatus\",\"message-id\":\"3\"}");
          } else {
            Serial.println("Authenticated FAILED");
          }
          break;
        }

        /* Handle OBS state changes. We only care about when it stops streaming */
        if (doc.containsKey("update-type")) {
          const char* updateType = doc["update-type"];
          if (strcmp(updateType, "StreamStopped") == 0) {
            setLed_NOT_STREAMING();
            break;
          }
        }

        /* Handle any other message with "streaming" in it, assuming it's indicating the current streaming state.
           While streaming, OBS publishes the current state every 2sec.
        */
        if (doc.containsKey("streaming")) {
          Serial.println("handling update message");
          bool isStreaming = doc["streaming"];
          if (isStreaming) {
            Serial.printf("[OBS] isStreaming: TRUE\n");
            setLed_STREAMING();
          } else {
            Serial.printf("[OBS] isStreaming: FALSE\n");
            setLed_NOT_STREAMING();
          }
          break;
        }

        break;
      }
		case WStype_BIN:
			Serial.printf("[WSc] get binary length: %u\n", length);
      Serial.println("Binary data is not expected, something might be wrong :(");
			break;
		case WStype_ERROR:			
		case WStype_FRAGMENT_TEXT_START:
		case WStype_FRAGMENT_BIN_START:
		case WStype_FRAGMENT:
		case WStype_FRAGMENT_FIN:
    case WStype_PING:
    case WStype_PONG:
			break;
	}

}

void setup() {
	Serial.begin(115200);

  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS); //change LED type if not using WS2812B's
  FastLED.setBrightness(LED_BRIGHTNESS);

  setLed_NOT_CONNECTED(); // default state is not connected

	WiFiMulti.addAP(WIFI_SSID, WIFI_PASS);

	while(WiFiMulti.run() != WL_CONNECTED) {
    Serial.println("Connecting to WIFI...");
		delay(250);
	}

	webSocket.begin(OBS_SERVER_ADDRESS, OBS_SERVER_PORT, "/");
	webSocket.onEvent(handleWebSocketEvent);
	webSocket.setReconnectInterval(2000);
}

void loop() {
	webSocket.loop();
}
