#ifndef M5SPOT_MAIN_H
#define M5SPOT_MAIN_H

#define min(X, Y) (((X)<(Y))?(X):(Y))
#define startsWith(STR, SEARCH) (strncmp(STR, SEARCH, strlen(SEARCH)) == 0)

typedef struct {
    int httpCode;
    String payload;
} HTTP_response_t;

enum SptfActions {
    Idle, GetToken, CurrentlyPlaying, Next, Previous, Toggle
};

enum GrantTypes {
    gt_authorization_code, gt_refresh_token
};

enum EventsLogTypes {
    log_line, log_raw
};

typedef struct {
    const char *ssid;
    const char *passphrase;
} APlist_t;


/*
 * Function declarations
 */
//@formatter:off
void progressBar(float val);

void eventsSendLog(const char *logData, EventsLogTypes type = log_line);
void eventsSendInfo(const char *msg, const char* payload = "");
void eventsSendError(int code, const char *msg, const char *payload = "");

HTTP_response_t httpRequest(const char *host, uint16_t port, const char *headers, const char *content = "");
HTTP_response_t sptfApiRequest(const char *method, const char *endpoint, const char *content = "");
void sptfGetToken(const String &code, GrantTypes grant_type = gt_refresh_token);
void sptfCurrentlyPlaying();
void sptfNext();
void sptfPrevious();
void sptfToggle();
void sptfDisplayAlbumArt(String url);

void writeRefreshToken();
void deleteRefreshToken();
String readRefreshToken();

void handleGesture();
void IRAM_ATTR interruptRoutine();

void m5sEpitaph(const char *errMsg);
String b64Encode(String str);
String prettyBytes(uint32_t bytes);
//@formatter:on

#endif // M5SPOT_MAIN_H
