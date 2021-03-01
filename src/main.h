#ifndef M5SPOT_MAIN_H
#define M5SPOT_MAIN_H

#include <Arduino.h>

//#define min(X, Y) (((X)<(Y))?(X):(Y))
#define strStartsWith(STR, SEARCH) (strncmp(STR, SEARCH, strlen(SEARCH)) == 0)

template<class T> struct Rect {
  T left;
  T right;
  T top;
  T bottom;

  Rect( T l, T t, T r, T b ): left(l), top(t), right(r), bottom(b) {};

  T width() const { return right-left; };
  T height() const { return bottom-top; };

  Rect<T>   outersect( const Rect<T>& other ) const
  {
      return {min(left,other.left), min(top,other.top), max(right,other.right), max(bottom,other.bottom)};
  }
  Rect<T>   scaleBy( double x, double y ) const
  {
      return {left,top,left+width()*x,top+height()*y};
  }
  Rect<T>   shrinkBy( T x, T y ) const
  {
      return {left+x,top+y,right-2*x,bottom-2*y};
  }
};

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
