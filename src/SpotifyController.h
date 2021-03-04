#pragma onca

#include "TrackDetails.h"

class AsyncWebServerRequest;

class SpotifyController
{
public:
    static TrackDetails CurrentTrack;
    static bool IsPlaying;
    static bool HasActiveDevice;
    static void UpdateActiveDevice();

    static bool UpdateFromCurrentlyPlayingIfNeeded();
    static bool UpdateFromCurrentlyPlaying();

    static void PlayOrPauseCurrentTrack();
//    static void PlayCurrentTrack();
//    static void PauseCurrentTrack();
    static void RestartCurrentTrack();
    static void PlayNextTrack();
    static void PlayPreviousTrack();

    enum GrantTypes {
        gt_authorization_code, gt_refresh_token
    };
    static String access_token;
    static String refresh_token; 
    static bool GettingToken;
    static void GetTokenIfNeeded();
    static void GetToken(const String &code, GrantTypes grant_type = gt_refresh_token);
    static void WriteRefreshToken();
    static String ReadRefreshToken();
    static void DeleteRefreshToken();
    static bool AuthoriseIfNeeded(AsyncWebServerRequest *request);

    static HTTP_response_t ApiRequest(const char *method, const char *endpoint, const char *content = "");
};
