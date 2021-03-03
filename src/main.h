#ifndef M5SPOT_MAIN_H
#define M5SPOT_MAIN_H

#include <Arduino.h>
#include <Preferences.h>

extern Preferences preferences;
extern const char* Preferences_App;

//#define min(X, Y) (((X)<(Y))?(X):(Y))
#define strStartsWith(STR, SEARCH) (strncmp(STR, SEARCH, strlen(SEARCH)) == 0)

template <class T> struct Point {
    T x;
    T y;

    Point() : x(T{}), y(T{}) {}; 
    Point( T x_, T y_ ) : x(x_), y(y_) {}; 

    bool operator==( const Point<T>& other ) const { return x == other.x && y == other.y; };
    bool operator!=( const Point<T>& other ) const { return !(*this  == other); };
    Point<T> operator+( const Point<T>& other ) const { return {x+other.x,y+other.y}; };
    Point<T> operator-( const Point<T>& other ) const { return {x-other.x,y-other.y}; };
};

template <class T> struct Size {
    T cx;
    T cy;

    Size() : cx(T{}), cy(T{}) {}; 
    Size( T x, T y ) : cx(x), cy(y) {}; 

    bool operator==( const Size<T>& other ) { return cx == other.cx && cy == other.cy; };
    bool operator!=( const Size<T>& other ) { return !(*this  == other); };
};

template<class T> struct Rect {
  T left;
  T right;
  T top;
  T bottom;

  Rect(): left(T{}), top(T{}), right(T{}), bottom(T{}) {};
  Rect( T l, T t, T r, T b ): left(l), top(t), right(r), bottom(b) {};
  Rect( Point<T> p, Size<T> s ) : left(p.x), top(p.y), right(p.x+s.cx), bottom(p.y+s.cy) {};

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
  Rect<T>   shrinkBy( const Size<T>& sz ) const
  {
      return {left+sz.cx,top+sz.cy,right-2*sz.cx,bottom-2*sz.cy};
  }
  Rect<T>   shrinkBy( T x, T y ) const
  {
      return {left+x,top+y,right-2*x,bottom-2*y};
  }
  bool      contains( const Point<T>& pt ) const
  {
      return left <= pt.x && pt.x <= right && top <= pt.y && pt.y <= bottom;
  }
  bool      contains( T x, T y ) const
  {
      return left <= x && x <= right && top <= y && y <= bottom;
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
