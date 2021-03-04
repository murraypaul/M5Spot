#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>

#include "main.h"
#include "SpotifyController.h"
#include "DisplayManager.h"
#include "config.h"

TrackDetails SpotifyController::CurrentTrack;
bool SpotifyController::IsPlaying = false;
bool SpotifyController::HasActiveDevice = false;
bool SpotifyController::GettingToken = false;
String SpotifyController::access_token;
String SpotifyController::refresh_token; 
uint32_t token_lifetime_ms = 0;
uint32_t token_millis = 0;
uint32_t last_curplay_millis = 0;
uint32_t next_curplay_millis = 0;

const char* Preferences_RefreshKey = "Sptfrftok";

/**
 * Write refresh token to EEPROM
 */
void SpotifyController::WriteRefreshToken() {
  Serial.println("Writing refresh token");
  Serial.println(refresh_token);
  preferences.begin(Preferences_App);
  preferences.putString(Preferences_RefreshKey,refresh_token);
  preferences.end();
}


/**
 * Delete refresh token from EEPROM
 */
void SpotifyController::DeleteRefreshToken() {
  Serial.println("Deleting refresh token");
  preferences.begin(Preferences_App);
//  if( preferences.isKey(Preferences_Key) )
    preferences.remove(Preferences_RefreshKey);
  preferences.end();
}


/**
 * Read refresh token from EEPROM
 *
 * @return String
 */
String SpotifyController::ReadRefreshToken() {
  Serial.println("Reading refresh token");
  log_i("\n> [%d] readRefreshToken()\n", micros());

  String tok;
  preferences.begin(Preferences_App);
//  if( preferences.isKey(Preferences_Key) )
    tok = preferences.getString(Preferences_RefreshKey);
  preferences.end();
  refresh_token = tok;
}

void SpotifyController::GetTokenIfNeeded()
{
   uint32_t cur_millis = millis();    
    if (refresh_token != ""
        && (token_millis == 0 || (cur_millis - token_millis >= token_lifetime_ms))) {
        static uint32_t gettoken_millis = 0;
        if (cur_millis - gettoken_millis >= 5000) {
            GetToken(refresh_token);
            gettoken_millis = cur_millis;
        }
    }
}

/**
 * Get Spotify token
 *
 * @param code          Either an authorization code or a refresh token
 * @param grant_type    [gt_authorization_code|gt_refresh_token]
 */
void SpotifyController::GetToken(const String &code, GrantTypes grant_type) {
    uint32_t ts = micros();
    log_v("> [%d] sptfGetToken(%s, %s)\n", ts, code.c_str(), grant_type == gt_authorization_code ? "authorization" : "refresh");

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
                    WriteRefreshToken();
                }
            }
        } else {
            if( response.payload.indexOf("Invalid refresh token") != -1 )
            {
                log_i("  [%d] Invalid refresh token, clearing and restarting\n", ts);
                access_token = "";
                refresh_token = "";
                DeleteRefreshToken();
                ESP.restart();
                while (true) {
                    yield();
                }
            }
            else
            {
                log_v("  [%d] Unable to parse response payload:\n  %s\n", ts, response.payload.c_str());
                eventsSendError(500, "Unable to parse response payload", response.payload.c_str());
            }
        }
    } else {
        log_v("  [%d] %d - %s\n", ts, response.httpCode, response.payload.c_str());
        eventsSendError(response.httpCode, "Spotify error", response.payload.c_str());
    }
    
    GettingToken = false;
}

/**
 * Call Spotify API
 *
 * @param method
 * @param endpoint
 * @return
 */
HTTP_response_t SpotifyController::ApiRequest(const char *method, const char *endpoint, const char *content) {
    uint32_t ts = micros();
    log_v("> [%d] sptfApiRequest(%s, %s, %s)\n", ts, method, endpoint, content);

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


bool SpotifyController::UpdateFromCurrentlyPlayingIfNeeded()
{
//    log_i("DisplayManager::UpdateFromCurrentlyPlayingIfNeeded");
    uint32_t cur_millis = millis(); 
    if (next_curplay_millis && (cur_millis >= next_curplay_millis)) {
        return UpdateFromCurrentlyPlaying();
    } else if (cur_millis - last_curplay_millis >= SPTF_POLLING_DELAY) {
        return UpdateFromCurrentlyPlaying();
    }    
    return false;
}


/**
 * Get information about the Spotify user's current playback
 */
bool SpotifyController::UpdateFromCurrentlyPlaying() {
    uint32_t ts = micros();
    log_v("> [%d] sptfCurrentlyPlaying()", ts);

    last_curplay_millis = millis();
    next_curplay_millis = 0;

    HTTP_response_t response = ApiRequest("GET", "/currently-playing");

    if (response.httpCode == 200) {
        TrackDetails track = TrackDetails::PopulateFromCurrentlyPlaying(response);
        IsPlaying = track.IsPlaying;
        // Check if current song is about to end
        if (IsPlaying) {
            uint32_t remaining_ms = track.DurationMS - track.ProgressMS;
            if (remaining_ms < SPTF_POLLING_DELAY && remaining_ms > 0) {
                // Refresh at the end of current song,
                // without considering remaining polling delay
                next_curplay_millis = millis() + remaining_ms + 200;
            }
        }
        return BaseDisplayManager.showTrack(track);
    } else if (response.httpCode == 204) {
        // Fallback to last played track
        response = ApiRequest("GET", "/recently-played?limit=1");

        if (response.httpCode == 200) {
            TrackDetails track = TrackDetails::PopulateFromRecentlyPlayed(response);
            IsPlaying = false;
            return BaseDisplayManager.showTrack(track);
        } else if (response.httpCode == 204) {
        } else {
            log_v("  [%d] %d - %s\n", ts, response.httpCode, response.payload.c_str());
            eventsSendError(response.httpCode, "Spotify error", response.payload.c_str());
        }
    }
}

/**
 * Spotify next track
 */
void SpotifyController::PlayNextTrack() {
    HTTP_response_t response = ApiRequest("POST", "/next");
    if (response.httpCode == 204) {
        next_curplay_millis = millis() + 200;
    } else {
        eventsSendError(response.httpCode, "Spotify error", response.payload.c_str());
    }
};


/**
 * Spotify previous track
 */
void SpotifyController::PlayPreviousTrack() {
    HTTP_response_t response = ApiRequest("POST", "/previous");
    if (response.httpCode == 204) {
        next_curplay_millis = millis() + 200;
    } else {
        eventsSendError(response.httpCode, "Spotify error", response.payload.c_str());
    }
};


/**
 * Spotify toggle pause/play
 */
void SpotifyController::PlayOrPauseCurrentTrack()
{
    HTTP_response_t response = ApiRequest("PUT", IsPlaying ? "/pause" : "/play");
    if (response.httpCode == 204) {
        IsPlaying = !IsPlaying;
        next_curplay_millis = millis() + 200;
    } else {
        eventsSendError(response.httpCode, "Spotify error", response.payload.c_str());
    }
};

void SpotifyController::RestartCurrentTrack()
{
    HTTP_response_t response = ApiRequest("PUT", "/seek?position_ms=0");
    if (response.httpCode == 204) {
        next_curplay_millis = millis() + 200;
    } else {
        eventsSendError(response.httpCode, "Spotify error", response.payload.c_str());
    }
};

bool SpotifyController::AuthoriseIfNeeded(AsyncWebServerRequest *request)
{
    if (access_token == "" && !GettingToken) {
        GettingToken = true;
        char auth_url[300] = "";
        snprintf(auth_url, sizeof(auth_url),
                    "https://accounts.spotify.com/authorize/"
                    "?response_type=code"
                    "&scope=user-read-private+user-read-currently-playing+user-read-recently-played&user-read-playback-state+user-modify-playback-state"
                    "&redirect_uri=http%%3A%%2F%%2Fm5spot.local%%2Fcallback%%2F"
                    "&client_id=%s",
                    SPTF_CLIENT_ID
        );
        log_i("  [%d] Redirect to: %s\n", millis(), auth_url);
        request->redirect(auth_url);
        return true;
    }
    return false;
}

void SpotifyController::UpdateActiveDevice()
{
    HTTP_response_t response = ApiRequest("GET", "/devices");

    HasActiveDevice = false;
    if (response.httpCode == 200) {
        StaticJsonDocument<256> filter;
        filter["devices"][0]["is_active"] = true;
        filter["devices"][0]["id"] = true;
        filter["devices"][0]["name"] = true;
        DynamicJsonDocument json(5120);
        DeserializationError error = deserializeJson(json,response.payload, DeserializationOption::Filter(filter));
        if (!error) 
        {
            for( const auto& item : json["devices"].as<JsonArray>() )
                if( item["is_active"] )
                    HasActiveDevice = true;
        }
    }
}
