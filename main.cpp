// Required:
// - https://github.com/me-no-dev/AsyncTCP
// - https://github.com/me-no-dev/ESPAsyncWebServer

#include <M5EPD.h>
//#include <Arduino.h>
#include <WiFiMulti.h>
#include <ESPmDNS.h>
#include <HTTPClient.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <base64.h>
#include <Preferences.h>
#include "main.h"
#include "config.h"

// MISC GLOBALS

WiFiMulti wifiMulti;
AsyncWebServer server(80);
AsyncEventSource events("/events");

String auth_code;
String access_token;
String refresh_token;

uint32_t token_lifetime_ms = 0;
uint32_t token_millis = 0;
uint32_t last_curplay_millis = 0;
uint32_t next_curplay_millis = 0;

bool getting_token = false;
bool sptf_is_playing = true;
bool send_events = true;

SptfActions sptfAction = Idle;

Preferences preferences;
const char* Preferences_App = "M5Spot";
const char* Preferences_Key = "Sptfrftok";

M5EPD_Canvas Canvas(&M5.EPD);

const int AlbumArt_Left = 0;
const int AlbumArt_Right = 300;
const int AlbumArt_Top = 0;
const int AlbumArt_Bottom = 300;
const int Progress_Left = 0;
const int Progress_Right = 300;
const int Progress_Top = 300;
const int Progress_Bottom = 320;
const int SongTitle_Left = 16;
const int SongTitle_Right = 960;
const int SongTitle_Top = 330;
const int SongTitle_Bottom = 380;
const int AlbumTitle_Left = 16;
const int AlbumTitle_Right = 960;
const int AlbumTitle_Top = 400;
const int AlbumTitle_Bottom = 460;
const int AlbumArtists_Left = 16;
const int AlbumArtists_Right = 960;
const int AlbumArtists_Top = 480;
const int AlbumArtists_Bottom = 540;

const int MainTrackDetails_Left = min(min(SongTitle_Left,AlbumTitle_Left),AlbumArtists_Left);
const int MainTrackDetails_Right = max(max(SongTitle_Right,AlbumTitle_Right),AlbumArtists_Right);
const int MainTrackDetails_Top = min(min(SongTitle_Top,AlbumTitle_Top),AlbumArtists_Top);
const int MainTrackDetails_Bottom = max(max(SongTitle_Bottom,AlbumTitle_Bottom),AlbumArtists_Bottom);

/**
 * Setup
 */
void setup() {

    //-----------------------------------------------
    // Initialize M5Stack
    //-----------------------------------------------
    M5.begin();
    M5.TP.SetRotation(0);
    M5.EPD.SetRotation(0);
    M5.EPD.Clear(true);

    Canvas.createCanvas(960, 540);
    
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

    Canvas.setFreeFont(&FreeSansBoldOblique12pt7b);
    Canvas.setTextSize(1);
    Canvas.setTextDatum(TC_DATUM);
    Canvas.drawString(title, 160, 10);

    Canvas.setFreeFont(&FreeSans9pt7b);
    Canvas.setTextSize(1);
    Canvas.setTextDatum(BC_DATUM);
    Canvas.drawString("Connecting to WiFi...", 160, 215);

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

    Canvas.setFreeFont(&FreeSansBoldOblique12pt7b);
//    Canvas.setTextColor(sptf_green);
    Canvas.setTextSize(1);
    Canvas.setTextDatum(TC_DATUM);
    Canvas.drawString(title, 160, 10);

    Canvas.setFreeFont(&FreeMono9pt7b);
//    Canvas.setTextColor(WHITE);
    Canvas.setTextSize(1);
    Canvas.setCursor(0, 75);
    Canvas.printf(" SSID:      %s\n", WiFi.SSID().c_str());
    Canvas.printf(" IP:        %s\n", WiFi.localIP().toString().c_str());
    Canvas.printf(" STA MAC:   %s\n", WiFi.macAddress().c_str());
    Canvas.printf(" AP MAC:    %s\n", WiFi.softAPmacAddress().c_str());
    Canvas.printf(" Chip size: %s\n", prettyBytes(ESP.getFlashChipSize()).c_str());
    Canvas.printf(" Free heap: %s\n", prettyBytes(ESP.getFreeHeap()).c_str());

    Canvas.setFreeFont(&FreeSans9pt7b);
//    Canvas.setTextColor(sptf_green);
    Canvas.setTextSize(1);
    Canvas.setTextDatum(BC_DATUM);
    Canvas.drawString("Press any button to continue...", 160, 230);

    Canvas.pushCanvas(0, 0, UPDATE_MODE_GC16);
    
    Serial.println("Init done");
    
    uint32_t pause = millis();
    while (true) {
        M5.update();
        if (M5.BtnL.wasPressed() || M5.BtnR.wasPressed() || M5.BtnR.wasPressed() || (millis() - pause > 20000)) {
            break;
        }
        yield();
    }

    //-----------------------------------------------
    // Initialize HTTP server handlers
    //-----------------------------------------------
    events.onConnect([](AsyncEventSourceClient *client) {
        log_i("\n> [%d] events.onConnect\n", micros());
    });
    server.addHandler(&events);

//    server.serveStatic("/favicon.ico", SPIFFS, "/favicon.ico");

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        Serial.println("/");
        uint32_t ts = micros();
        log_i("\n> [%d] server.on /\n", ts);
        if (access_token == "" && !getting_token) {
            getting_token = true;
            char auth_url[300] = "";
            snprintf(auth_url, sizeof(auth_url),
                     "https://accounts.spotify.com/authorize/"
                     "?response_type=code"
                     "&scope=user-read-private+user-read-currently-playing+user-read-playback-state+user-modify-playback-state"
                     "&redirect_uri=http%%3A%%2F%%2Fm5spot.local%%2Fcallback%%2F"
                     "&client_id=%s",
                     SPTF_CLIENT_ID
            );
            log_i("  [%d] Redirect to: %s\n", ts, auth_url);
            request->redirect(auth_url);
        } else {
//            request->send(SPIFFS, "/index.html");
        }
    });

    server.on("/callback", HTTP_GET, [](AsyncWebServerRequest *request) {
        Serial.println("/callback");
        auth_code = "";
        uint8_t paramsNr = request->params();
        for (uint8_t i = 0; i < paramsNr; i++) {
            AsyncWebParameter *p = request->getParam(i);
            if (p->name() == "code") {
                auth_code = p->value();
                sptfAction = GetToken;
                break;
            }
        }
        if (sptfAction == GetToken) {
            request->redirect("/");
        } else {
            request->send(204);
        }
    });

    server.on("/next", HTTP_GET, [](AsyncWebServerRequest *request) {
        sptfAction = Next;
        request->send(204);
    });

    server.on("/previous", HTTP_GET, [](AsyncWebServerRequest *request) {
        sptfAction = Previous;
        request->send(204);
    });

    server.on("/toggle", HTTP_GET, [](AsyncWebServerRequest *request) {
        sptfAction = Toggle;
        request->send(204);
    });

    server.on("/heap", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "text/plain", String(ESP.getFreeHeap()));
    });

    server.on("/resettoken", HTTP_GET, [](AsyncWebServerRequest *request) {
        access_token = "";
        refresh_token = "";
        deleteRefreshToken();
        sptfAction = Idle;
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

    server.on("/toggleevents", HTTP_GET, [](AsyncWebServerRequest *request) {
        send_events = !send_events;
        request->send(200, "text/plain", send_events ? "1" : "0");
    });

    server.onNotFound([](AsyncWebServerRequest *request) {
        request->send(404);
    });

    server.begin();

    //-----------------------------------------------
    // Get refresh token from EEPROM
    //-----------------------------------------------
    refresh_token = readRefreshToken();

    //-----------------------------------------------
    // End of setup
    //-----------------------------------------------
//    Canvas.fillScreen(BLACK);
//    Canvas.drawJpgFile(SPIFFS, "/logo128.jpg", 96, 50, 128, 128);

    Canvas.setFreeFont(&FreeSansBoldOblique12pt7b);
    Canvas.setTextSize(1);
    Canvas.setTextDatum(TC_DATUM);
    Canvas.drawString(title, 160, 10);

    Canvas.setTextDatum(BC_DATUM);
    if (refresh_token == "") {
        Canvas.setFreeFont(&FreeSans9pt7b);
        Canvas.drawString("Point your browser to", 160, 205);

        Canvas.setFreeFont(&FreeSans12pt7b);
        Canvas.drawString("http://m5spot.local", 160, 235);
    } else {
        Canvas.setFreeFont(&FreeSans9pt7b);
        Canvas.drawString("Ready...", 160, 230);

        sptfAction = CurrentlyPlaying;
    }
    Canvas.pushCanvas(0, 0, UPDATE_MODE_GC16);
}

/**
 * Main loop
 */
void loop() {

    uint32_t cur_millis = millis();

    // Refreh Spotify access token either on M5Spot startup or at token expiration delay
    // The number of requests is limited to 1 every 5 seconds
    if (refresh_token != ""
        && (token_millis == 0 || (cur_millis - token_millis >= token_lifetime_ms))) {
        static uint32_t gettoken_millis = 0;
        if (cur_millis - gettoken_millis >= 5000) {
            sptfGetToken(refresh_token);
            gettoken_millis = cur_millis;
        }
    }

    // M5Stack handler
    M5.update();
    if (M5.BtnL.wasPressed()) {
        sptfAction = Previous;
    }

    if (M5.BtnP.wasPressed()) {
        sptfAction = Toggle;
    }

    if (M5.BtnR.wasPressed()) {
        sptfAction = Next;
    }

    // Spotify action handler
    switch (sptfAction) {
        case Idle:
            break;
        case GetToken:
            sptfGetToken(auth_code, gt_authorization_code);
            break;
        case CurrentlyPlaying:
            if (next_curplay_millis && (cur_millis >= next_curplay_millis)) {
                sptfCurrentlyPlaying();
            } else if (cur_millis - last_curplay_millis >= SPTF_POLLING_DELAY) {
                sptfCurrentlyPlaying();
            }
            break;
        case Next:
            sptfNext();
            break;
        case Previous:
            sptfPrevious();
            break;
        case Toggle:
            sptfToggle();
            break;
    }
}


/**
 * Draw a progress bar
 *
 * @param y
 * @param width
 * @param val
 */
void progressBar(float val) {
    Canvas.fillRect(Progress_Left, Progress_Top, Progress_Right-Progress_Left, Progress_Bottom-Progress_Top, 0);
    Canvas.drawRect(Progress_Left + 2, Progress_Top + 2, Progress_Right-Progress_Left - 4, Progress_Bottom-Progress_Top - 4, 15);
    Canvas.fillRect(Progress_Left + 2, Progress_Top + 2, (Progress_Right-Progress_Left - 4) * val, Progress_Bottom-Progress_Top - 4, 15);
    M5.EPD.WriteFullGram4bpp((uint8_t*)Canvas.frameBuffer());
    M5.EPD.UpdateArea(Progress_Left, Progress_Top, Progress_Right-Progress_Left, Progress_Bottom-Progress_Top,UPDATE_MODE_DU);
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
 * Write refresh token to EEPROM
 */
void writeRefreshToken() {
  Serial.println("Writing refresh token");
  Serial.println(refresh_token);
  preferences.begin(Preferences_App);
  preferences.putString(Preferences_Key,refresh_token);
  preferences.end();
}


/**
 * Delete refresh token from EEPROM
 */
void deleteRefreshToken() {
  Serial.println("Deleting refresh token");
  preferences.begin(Preferences_App);
//  if( preferences.isKey(Preferences_Key) )
    preferences.remove(Preferences_Key);
  preferences.end();
}


/**
 * Read refresh token from EEPROM
 *
 * @return String
 */
String readRefreshToken() {
  Serial.println("Reading refresh token");
  log_i("\n> [%d] readRefreshToken()\n", micros());

  String tok;
  preferences.begin(Preferences_App);
//  if( preferences.isKey(Preferences_Key) )
    tok = preferences.getString(Preferences_Key);
  preferences.end();
  return tok;
}


/**
 * Display album art
 *
 * @param url
 */
void sptfDisplayAlbumArt(String url) {
    uint32_t ts = micros();
    log_i("\n> [%d] sptfDisplayAlbumArt(%s)\n", ts, url.c_str());

    Canvas.fillCanvas(0);
    Canvas.drawJpgUrl(url, AlbumArt_Top, AlbumArt_Left, AlbumArt_Right-AlbumArt_Left, AlbumArt_Bottom-AlbumArt_Top, JPEG_DIV_MAX);
    Canvas.pushCanvas(0, 0, UPDATE_MODE_GC16);
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
    log_i("\n> [%d] httpRequest(%s, %d, ...)\n", ts, host, port);

    WiFiClientSecure client;

    if (!client.connect(host, port)) {
        return {503, "Service unavailable (unable to connect)"};
    }

    /*
     * Send HTTP request
     */

    log_i("  [%d] Request:\n%s%s\n", ts, headers, content);
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

    log_i("  [%d] Response:\n", ts);
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
                log_i("%s\n", buff);
                eventsSendLog(buff);
                if (startsWith(buff, "HTTP/1.")) {
                    buff[12] = '\0';
                    response.httpCode = atoi(&buff[9]);
                    if (response.httpCode == 204) {
                        break;
                    }
                } else if (startsWith(buff, "Content-Length:")) {
                    contentLength = atoi(&buff[16]);
                    if (contentLength == 0) {
                        break;
                    }
                    response.payload.reserve(contentLength + 1);
                } else if (buff[0] == '\0') {
                    // End of headers
                    EOH = true;
                    log_i("<EOH>\n");
                    eventsSendLog("");
                }
            } else {
                // Read response content
                readSize = client.readBytes(buff, min(buffSize - 1, availableSize));
                buff[readSize] = '\0';
                log_i("%s",buff);
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
        log_i("  [%d] Invalid refresh token, clearing and restarting\n", ts);
        access_token = "";
        refresh_token = "";
        deleteRefreshToken();
        sptfAction = Idle;
        ESP.restart();
        while (true) {
            yield();
        }
    }

    log_i("\n< [%d] HEAP: %d\n", ts, ESP.getFreeHeap());

    return response;
}


/**
 * Call Spotify API
 *
 * @param method
 * @param endpoint
 * @return
 */
HTTP_response_t sptfApiRequest(const char *method, const char *endpoint, const char *content) {
    uint32_t ts = micros();
    log_i("\n> [%d] sptfApiRequest(%s, %s, %s)\n", ts, method, endpoint, content);

    char headers[512];
    snprintf(headers, sizeof(headers),
             "%s /v1/me/player%s HTTP/1.1\r\n"
             "Host: api.spotify.com\r\n"
             "Authorization: Bearer %s\r\n"
             "Content-Length: %d\r\n"
             "Connection: close\r\n\r\n",
             method, endpoint, access_token.c_str(), strlen(content)
    );

    return httpRequest("api.spotify.com", 443, headers, content);
}


/**
 * Get Spotify token
 *
 * @param code          Either an authorization code or a refresh token
 * @param grant_type    [gt_authorization_code|gt_refresh_token]
 */
void sptfGetToken(const String &code, GrantTypes grant_type) {
    uint32_t ts = micros();
    log_i("\n> [%d] sptfGetToken(%s, %s)\n", ts, code.c_str(), grant_type == gt_authorization_code ? "authorization" : "refresh");

    bool success = false;

    char requestContent[512];
    if (grant_type == gt_authorization_code) {
        snprintf(requestContent, sizeof(requestContent),
                 "grant_type=authorization_code"
                 "&redirect_uri=http%%3A%%2F%%2Fm5spot.local%%2Fcallback%%2F"
                 "&code=%s",
                 code.c_str()
        );
    } else {
        snprintf(requestContent, sizeof(requestContent),
                 "grant_type=refresh_token&refresh_token=%s",
                 code.c_str()
        );
    }

    uint8_t basicAuthSize = sizeof(SPTF_CLIENT_ID) + sizeof(SPTF_CLIENT_SECRET);
    char basicAuth[basicAuthSize];
    snprintf(basicAuth, basicAuthSize, "%s:%s", SPTF_CLIENT_ID, SPTF_CLIENT_SECRET);

    char requestHeaders[768];
    snprintf(requestHeaders, sizeof(requestHeaders),
             "POST /api/token HTTP/1.1\r\n"
             "Host: accounts.spotify.com\r\n"
             "Authorization: Basic %s\r\n"
             "Content-Length: %d\r\n"
             "Content-Type: application/x-www-form-urlencoded\r\n"
             "Connection: close\r\n\r\n",
             b64Encode(basicAuth).c_str(), strlen(requestContent)
    );

    HTTP_response_t response = httpRequest("accounts.spotify.com", 443, requestHeaders, requestContent);

    if (response.httpCode == 200) {

        DynamicJsonDocument json(572);
        DeserializationError error = deserializeJson(json, response.payload);
        if (!error) {
            access_token = json["access_token"].as<String>();
            if (access_token != "") {
                token_lifetime_ms = (json["expires_in"].as<uint32_t>() - 300) * 1000;
                token_millis = millis();
                success = true;
                if (json.containsKey("refresh_token")) {
                    refresh_token = json["refresh_token"].as<String>();
                    writeRefreshToken();
                }
            }
        } else {
            if( response.payload.indexOf("Invalid refresh token") != -1 )
            {
                log_i("  [%d] Invalid refresh token, clearing and restarting\n", ts);
                access_token = "";
                refresh_token = "";
                deleteRefreshToken();
                sptfAction = Idle;
                ESP.restart();
                while (true) {
                    yield();
                }
            }
            else
            {
                log_i("  [%d] Unable to parse response payload:\n  %s\n", ts, response.payload.c_str());
                eventsSendError(500, "Unable to parse response payload", response.payload.c_str());
            }
        }
    } else {
        log_i("  [%d] %d - %s\n", ts, response.httpCode, response.payload.c_str());
        eventsSendError(response.httpCode, "Spotify error", response.payload.c_str());
    }

    if (success) {
        sptfAction = CurrentlyPlaying;
    }

    getting_token = false;
}


/**
 * Get information about the Spotify user's current playback
 */
void sptfCurrentlyPlaying() {
    uint32_t ts = micros();
    log_i("\n> [%d] sptfCurrentlyPlaying()\n", ts);

    last_curplay_millis = millis();
    next_curplay_millis = 0;

    HTTP_response_t response = sptfApiRequest("GET", "/currently-playing");

    if (response.httpCode == 200) {
        StaticJsonDocument<256> filter;
        filter["is_playing"] = true;
        filter["progress_ms"] = true;
        filter["item"]["duration_ms"] = true;
        filter["item"]["id"] = true;
        filter["item"]["name"] = true;
        filter["item"]["artists"][0]["name"] = true;
        filter["item"]["album"]["name"] = true;
        filter["item"]["album"]["images"][0]["url"] = true;
        DynamicJsonDocument json(5120);
        DeserializationError error = deserializeJson(json,response.payload, DeserializationOption::Filter(filter));
        if (!error) {
            sptf_is_playing = json["is_playing"];
            uint32_t progress_ms = json["progress_ms"];
            uint32_t duration_ms = json["item"]["duration_ms"];

            // Check if current song is about to end
            if (sptf_is_playing) {
                uint32_t remaining_ms = duration_ms - progress_ms;
                if (remaining_ms < SPTF_POLLING_DELAY) {
                    // Refresh at the end of current song,
                    // without considering remaining polling delay
                    next_curplay_millis = millis() + remaining_ms + 200;
                }
            }

            // Get song ID
            const char *song_id = json["item"]["id"];
            const char *song_name = json["item"]["name"];
            static char previousId[32] = {0};

            // If song has changed, refresh display
            if (strcmp(song_id, previousId) != 0) 
              {
                strncpy(previousId, song_id, sizeof(previousId));

                // Display album art
                const char *art_url = json["item"]["album"]["images"][1]["url"];
                sptfDisplayAlbumArt(art_url);

                const char *album_name = json["item"]["album"]["name"];

                // Display song name
                Canvas.fillRect(SongTitle_Left, SongTitle_Top, SongTitle_Right-SongTitle_Left, SongTitle_Bottom-SongTitle_Top, 0);
                Canvas.setFreeFont(&FreeSansBold24pt7b);
                Canvas.setTextSize(1);
                Canvas.setTextDatum(TL_DATUM);
                Canvas.drawString(song_name, SongTitle_Left, SongTitle_Top);

                // Display album name
                Canvas.fillRect(AlbumTitle_Left, AlbumTitle_Top, AlbumTitle_Right-AlbumTitle_Left, AlbumTitle_Bottom-AlbumTitle_Top, 0);
                Canvas.setFreeFont(&FreeSansOblique18pt7b);
                Canvas.setTextSize(1);
                Canvas.setTextDatum(TL_DATUM);
                Canvas.drawString(album_name, AlbumTitle_Left, AlbumTitle_Top);

                // Display artists names
                JsonArray arr = json["item"]["artists"];
                String artists;
                artists.reserve(150);
                bool first = true;
                for (auto a : arr) {
                    if (first) {
                        artists += a["name"].as<String>();
                        first = false;
                    } else {
                        artists += ", ";
                        artists += a["name"].as<String>();
                    }
                }
                Canvas.fillRect(AlbumArtists_Left, AlbumArtists_Top, AlbumArtists_Right-AlbumArtists_Left, AlbumArtists_Bottom-AlbumArtists_Top, 0);
                Canvas.setFreeFont(&FreeSans24pt7b);
                Canvas.setTextSize(1);
                Canvas.setTextDatum(TL_DATUM);
                Canvas.drawString(artists, AlbumArtists_Left, AlbumArtists_Top);

                M5.EPD.WriteFullGram4bpp((uint8_t*)Canvas.frameBuffer());
                M5.EPD.UpdateArea(MainTrackDetails_Left, MainTrackDetails_Top, MainTrackDetails_Right-MainTrackDetails_Left, MainTrackDetails_Bottom-MainTrackDetails_Top, UPDATE_MODE_DU);
            }

            progressBar( (float) progress_ms / duration_ms);
        } else {
            log_i("  [%d] Unable to parse response payload:\n  %s\n", ts, response.payload.c_str());
            eventsSendError(500, "Unable to parse response payload", response.payload.c_str());
        }
    } else if (response.httpCode == 204) {
        // No content
    } else {
        log_i("  [%d] %d - %s\n", ts, response.httpCode, response.payload.c_str());
        eventsSendError(response.httpCode, "Spotify error", response.payload.c_str());
    }

    log_i("< [%d] HEAP: %d\n", ts, ESP.getFreeHeap());
}

/**
 * Spotify next track
 */
void sptfNext() {
    HTTP_response_t response = sptfApiRequest("POST", "/next");
    if (response.httpCode == 204) {
        next_curplay_millis = millis() + 200;
    } else {
        eventsSendError(response.httpCode, "Spotify error", response.payload.c_str());
    }
    sptfAction = CurrentlyPlaying;
};


/**
 * Spotify previous track
 */
void sptfPrevious() {
    HTTP_response_t response = sptfApiRequest("POST", "/previous");
    if (response.httpCode == 204) {
        next_curplay_millis = millis() + 200;
    } else {
        eventsSendError(response.httpCode, "Spotify error", response.payload.c_str());
    }
    sptfAction = CurrentlyPlaying;
};


/**
 * Spotify toggle pause/play
 */
void sptfToggle() {
    HTTP_response_t response = sptfApiRequest("PUT", sptf_is_playing ? "/pause" : "/play");
    if (response.httpCode == 204) {
        sptf_is_playing = !sptf_is_playing;
        next_curplay_millis = millis() + 200;
    } else {
        eventsSendError(response.httpCode, "Spotify error", response.payload.c_str());
    }
    sptfAction = CurrentlyPlaying;
};

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
    Canvas.setFreeFont(&FreeSans12pt7b);
    Canvas.setTextSize(1);
    Canvas.setTextDatum(CC_DATUM);
    Canvas.drawString(errMsg, 160, 120);
    Canvas.pushCanvas(0, 0, UPDATE_MODE_GC16);
}
