// Required:
// - https://github.com/me-no-dev/AsyncTCP
// - https://github.com/me-no-dev/ESPAsyncWebServer

#include <Arduino.h>
#include <WiFiMulti.h>
#include <ESPmDNS.h>
#include <HTTPClient.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <base64.h>
#include "main.h"
#include "config.h"

#include <M5EPD.h>

#include "SpotifyController.h"
#include "DisplayManager.h"
#include "TrackDetails.h"

// MISC GLOBALS

WiFiMulti wifiMulti;
AsyncWebServer server(80);
AsyncEventSource events("/events");

bool send_events = true;
static bool sptf_is_playing = true;

Preferences preferences;
const char* Preferences_App = "M5Spot";



/**
 * Setup
 */
void setup() {

    //-----------------------------------------------
    // Initialize M5Stack
    //-----------------------------------------------
    BaseDisplayManager.Init(true);

    char title[17];
    snprintf(title, sizeof(title), "M5Spot v%s", M5S_VERSION);

    //-----------------------------------------------
    // Initialize SPIFFS
    //-----------------------------------------------

//    if (!SPIFFS.begin()) {
//        m5sEpitaph("Unable to begin SPIFFS");
//    }

    //-----------------------------------------------
    // Initialize Wifi
    //-----------------------------------------------

//    Canvas.drawJpgFile(SPIFFS, "/logo128.jpg", 96, 50, 128, 128);

    BaseDisplayManager.drawString(&FreeSansBoldOblique12pt7b,TC_DATUM, title, 160, 10);

    BaseDisplayManager.drawString(&FreeSans9pt7b, BC_DATUM, "Connecting to WiFi...", 160, 215);

    WiFi.mode(WIFI_STA);
    for (auto i : AP_LIST) {
        wifiMulti.addAP(i.ssid, i.passphrase);
    }

    uint8_t count = 20;
    while (count-- && (wifiMulti.run() != WL_CONNECTED)) {
        delay(500);
    }

    if (!WiFi.isConnected()) {
        m5sEpitaph("Unable to connect to WiFi");
    }

    WiFi.setHostname("m5spot");

    if(!MDNS.begin("m5spot")) {
         Serial.println("Error starting mDNS");
         return;
    }

    MDNS.addService("http", "tcp", 80);

    //-----------------------------------------------
    // Display some infos
    //-----------------------------------------------

//    Canvas.fillScreen(BLACK);
//    Canvas.drawJpgFile(SPIFFS, "/logo128d.jpg", 96, 50, 128, 128);

    BaseDisplayManager.drawString(&FreeSansBoldOblique12pt7b, TC_DATUM, title, 160, 10);

    BaseDisplayManager.GetCanvas().setFreeFont(&FreeMono9pt7b);
//    Canvas.setTextColor(WHITE);
    BaseDisplayManager.GetCanvas().setTextSize(1);
    BaseDisplayManager.GetCanvas().setCursor(0, 75);
    BaseDisplayManager.GetCanvas().printf(" SSID:      %s\n", WiFi.SSID().c_str());
    BaseDisplayManager.GetCanvas().printf(" IP:        %s\n", WiFi.localIP().toString().c_str());
    BaseDisplayManager.GetCanvas().printf(" STA MAC:   %s\n", WiFi.macAddress().c_str());
    BaseDisplayManager.GetCanvas().printf(" AP MAC:    %s\n", WiFi.softAPmacAddress().c_str());
    BaseDisplayManager.GetCanvas().printf(" Chip size: %s\n", prettyBytes(ESP.getFlashChipSize()).c_str());
    BaseDisplayManager.GetCanvas().printf(" Free heap: %s\n", prettyBytes(ESP.getFreeHeap()).c_str());

//    Canvas.setFreeFont(&FreeSans9pt7b);
//    Canvas.setTextColor(sptf_green);
//    Canvas.setTextSize(1);
//    Canvas.setTextDatum(BC_DATUM);
//    Canvas.drawString("Press any button to continue...", 160, 230);

    BaseDisplayManager.GetCanvas().pushCanvas(0, 0, UPDATE_MODE_GC16);
    
//    Serial.println("Init done");
    
//    uint32_t pause = millis();
//    while (true) {
//        M5.update();
//        if (M5.BtnL.wasPressed() || M5.BtnR.wasPressed() || M5.BtnR.wasPressed() || (millis() - pause > 20000)) {
//            break;
//        }
//        yield();
//    }

    //-----------------------------------------------
    // Initialize HTTP server handlers
    //-----------------------------------------------
    events.onConnect([](AsyncEventSourceClient *client) {
        log_i("> [%d] events.onConnect\n", micros());
    });
    server.addHandler(&events);

//    server.serveStatic("/favicon.ico", SPIFFS, "/favicon.ico");

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        Serial.println("/");
        uint32_t ts = micros();
        log_i("> [%d] server.on /\n", ts);
        SpotifyController::AuthoriseIfNeeded(request);
    });

    server.on("/callback", HTTP_GET, [](AsyncWebServerRequest *request) {
        Serial.println("/callback");
        bool bRedirect = false;
        uint8_t paramsNr = request->params();
        for (uint8_t i = 0; i < paramsNr; i++) {
            AsyncWebParameter *p = request->getParam(i);
            if (p->name() == "code") {
                SpotifyController::GetToken(p->value(), SpotifyController::gt_authorization_code);
                bRedirect = true;
                break;
            }
        }
        if (bRedirect) {
            request->redirect("/");
        } else {
            request->send(204);
        }
    });

    server.on("/resettoken", HTTP_GET, [](AsyncWebServerRequest *request) {
        SpotifyController::access_token = "";
        SpotifyController::refresh_token = "";
        SpotifyController::DeleteRefreshToken();
        request->send(200, "text/plain", "Tokens deleted, M5Spot will restart");
        uint32_t start = millis();
        while (true) {
            if(millis() -  start > 5000) {
                ESP.restart();
            }
            yield();
        }
    });
    server.on("/resetwifi", HTTP_GET, [](AsyncWebServerRequest *request) {
        WiFi.disconnect(true);
        request->send(200, "text/plain", "WiFi credentials deleted, M5Spot will restart");
        uint32_t start = millis();
        while (true) {
            if(millis() -  start > 5000) {
                ESP.restart();
            }
            yield();
        }
    });

    server.onNotFound([](AsyncWebServerRequest *request) {
        request->send(404);
    });

    server.begin();

    //-----------------------------------------------
    // Get refresh token from EEPROM
    //-----------------------------------------------
    SpotifyController::ReadRefreshToken();

    //-----------------------------------------------
    // End of setup
    //-----------------------------------------------
//    Canvas.fillScreen(BLACK);
//    Canvas.drawJpgFile(SPIFFS, "/logo128.jpg", 96, 50, 128, 128);

    BaseDisplayManager.drawString(&FreeSansBoldOblique12pt7b, TC_DATUM,title, 160, 10);

    if (SpotifyController::refresh_token == "") {
        BaseDisplayManager.drawString(&FreeSans9pt7b, BC_DATUM, "Point your browser to", 160, 205);
        BaseDisplayManager.drawString(&FreeSans12pt7b, BC_DATUM, "http://m5spot.local", 160, 235);
    } else {
//        Canvas.setFreeFont(&FreeSans9pt7b);
//        Canvas.drawString("Ready...", 160, 230);

        BaseDisplayManager.clearScreen();       
    }
    BaseDisplayManager.refreshScreen();
}

/**
 * Main loop
 */
void loop() {
    if (!WiFi.isConnected()) {
        log_i("Lost WiFi connection");
        uint8_t count = 20;
        while (count-- && (wifiMulti.run() != WL_CONNECTED)) {
            delay(500);
        }
    }    
    if (!WiFi.isConnected()) 
        return;

    static bool bFirstLoop = true;
    if( bFirstLoop )
    {
        SpotifyController::UpdateActiveDevice();
        bFirstLoop = false;
    }

    // Refreh Spotify access token either on M5Spot startup or at token expiration delay
    // The number of requests is limited to 1 every 5 seconds
    if(SpotifyController::refresh_token != "")
        BaseDisplayManager.doLoop();
}


/**
 * Send log to browser
 *
 * @param logData
 * @param event_type
 */
void eventsSendLog(const char *logData, EventsLogTypes type) {
    if(!send_events) return;
    events.send(logData, type == log_line ? "line" : "raw");
}


/**
 * Send infos to browser
 *
 * @param msg
 * @param payload
 */
void eventsSendInfo(const char *msg, const char *payload) {
    if(!send_events) return;

    DynamicJsonDocument json(256);
    json["msg"] = msg;
    if (strlen(payload)) {
        json["payload"] = payload;
    }

    String info;
    serializeJson(json,info);
    events.send(info.c_str(), "info");
}


/**
 * Send errors to browser
 *
 * @param errCode
 * @param errMsg
 * @param payload
 */
void eventsSendError(int code, const char *msg, const char *payload) {
    if(!send_events) return;

    DynamicJsonDocument json(256);
    json["code"] = code;
    json["msg"] = msg;
    if (strlen(payload)) {
        json["payload"] = payload;
    }

    String error;
    serializeJson(json,error);
    events.send(error.c_str(), "error");
}


/**
 * Base 64 encode
 *
 * @param str
 * @return
 */
String b64Encode(String str) {

    String encodedStr = base64::encode(str);

    // Remove unnecessary linefeeds
    int idx = -1;
    while ((idx = encodedStr.indexOf('\n')) != -1) {
        encodedStr.remove(idx, 1);
    }

    return encodedStr;
}

/**
 * HTTP request
 *
 * @param host
 * @param port
 * @param headers
 * @param content
 * @return
 */
HTTP_response_t httpRequest(const char *host, uint16_t port, const char *headers, const char *content) {
    uint32_t ts = micros();
    log_v("\n> [%d] httpRequest(%s, %d, ...)\n", ts, host, port);

    WiFiClientSecure client;

    if (!client.connect(host, port)) {
        return {503, "Service unavailable (unable to connect)"};
    }

    /*
     * Send HTTP request
     */

    log_v("  [%d] Request:\n%s%s\n", ts, headers, content);
    eventsSendLog(">>>> REQUEST");
    eventsSendLog(headers);
    eventsSendLog(content);

    client.print(headers);
    if (strlen(content)) {
        client.print(content);
    }

    /*
     * Get HTTP response
     */

    uint32_t timeout = millis();
    while (!client.available()) {
        if (millis() - timeout > 5000) {
            client.stop();
            return {503, "Service unavailable (timeout)"};
        }
        yield();
    }

    log_v("  [%d] Response:\n", ts);
    eventsSendLog("<<<< RESPONSE");

    HTTP_response_t response = {0, ""};
    boolean EOH = false;
    uint32_t contentLength = 0;
    uint16_t buffSize = 1024;
    uint32_t readSize = 0;
    uint32_t totatlReadSize = 0;
    uint32_t lastAvailableMillis = millis();
    char buff[buffSize];

    // !HERE
    // client.setNoDelay(false);

    while (client.connected()) {
        int availableSize = client.available();
        if (availableSize) {
            lastAvailableMillis = millis();

            if (!EOH) {
                // Read response headers
                readSize = client.readBytesUntil('\n', buff, buffSize);
                buff[readSize - 1] = '\0'; // replace /r by \0
                log_v("%s\n", buff);
                eventsSendLog(buff);
                if (strStartsWith(buff, "HTTP/1.")) {
                    buff[12] = '\0';
                    response.httpCode = atoi(&buff[9]);
                    if (response.httpCode == 204) {
                        break;
                    }
                } else if (strStartsWith(buff, "Content-Length:")) {
                    contentLength = atoi(&buff[16]);
                    if (contentLength == 0) {
                        break;
                    }
                    response.payload.reserve(contentLength + 1);
                } else if (buff[0] == '\0') {
                    // End of headers
                    EOH = true;
                    log_v("<EOH>\n");
                    eventsSendLog("");
                }
            } else {
                // Read response content
                readSize = client.readBytes(buff, min(buffSize - 1, availableSize));
                buff[readSize] = '\0';
                log_v("%s",buff);
                eventsSendLog(buff, log_raw);
                response.payload += buff;
                totatlReadSize += readSize;
                if (totatlReadSize >= contentLength) {
                    break;
                }
            }
        } else {
            if ((millis() - lastAvailableMillis) > 5000) {
                response = {504, "Response timeout"};
                break;
            }
            delay(100);
        }
    }
    client.stop();

    if( response.payload.indexOf("Invalid refresh token") != -1 )
    {
        log_e("  [%d] Invalid refresh token, clearing and restarting\n", ts);
        SpotifyController::access_token = "";
        SpotifyController::refresh_token = "";
        SpotifyController::DeleteRefreshToken();
        ESP.restart();
        while (true) {
            yield();
        }
    }

    return response;
}

/**
 * Display bytes in a pretty format
 *
 * @param Bytes value
 * @return Bytes value prettified
 */
String prettyBytes(uint32_t bytes) {

    const char *suffixes[7] = {"B", "KB", "MB", "GB", "TB", "PB", "EB"};
    uint8_t s = 0;
    double count = bytes;

    while (count >= 1024 && s < 7) {
        s++;
        count /= 1024;
    }
    if (count - floor(count) == 0.0) {
        return String((int) count) + suffixes[s];
    } else {
        return String(round(count * 10.0) / 10.0, 1) + suffixes[s];
    };
}

/**
 * Display error message and stop execution
 *
 * @param errMsg
 */
void m5sEpitaph(const char *errMsg) {
    BaseDisplayManager.drawString(&FreeSans12pt7b, CC_DATUM, errMsg, 160, 120);
    BaseDisplayManager.refreshScreen(UPDATE_MODE_GC16);
}
