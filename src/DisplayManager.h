#pragma once

#include <list>
#include <memory>

#include <M5EPD.h>

#include "TrackDetails.h"

class LayoutItem;

struct DisplayManager
{
    enum eLayout {
        eLandscape_SmallArt = 0,
        eLandscape_BigArt,
        ePortrait_BigArt,

        eSettings
    };

protected:
    M5EPD_Canvas Canvas;
    Size<uint16_t>  CanvasSize;
    Point<uint16_t>  CanvasPos;
    uint16_t Rotation = 0;
    eLayout  CurrentLayout = ePortrait_BigArt;
    std::list<std::shared_ptr<LayoutItem>>    LayoutItems;
    TrackDetails CurrentTrack;
    bool ShouldClose = false;
    M5EPD_Canvas TempJpegCanvas;
    String  CurrentCachedJpegURL;
    bool PopupDialogActive = false;
    Rect<uint16_t>   DesiredUpdateRect{0,0,0,0};
    m5epd_update_mode_t     DesiredUpdateMode = UPDATE_MODE_NONE;

public:
    uint16_t MaxPreferredImageSize = 0;

    DisplayManager();
    void Init( bool appInit = false );
    void SetLayout( eLayout );

    bool showTrack( const TrackDetails& track );
    void redraw();

    M5EPD_Canvas&   GetCanvas();
    void drawRect( const Rect<uint16_t>& rect, uint32_t colour );
    void fillRect( const Rect<uint16_t>& rect, uint32_t colour );
    void drawString( const GFXfont* font, uint8_t datum, String str, const Rect<uint16_t>& rect );
    void drawString( const GFXfont* font, uint8_t datum, String str, uint32_t x, uint32_t y );
    void drawJpgUrl( String url, const Rect<uint16_t>& rect );
    void drawJpgUrlScaled( String url, const Size<uint16_t>& sourceSize, const Rect<uint16_t>& targetRect );
    
    void clearScreen();
    void refreshScreen( m5epd_update_mode_t mode = UPDATE_MODE_GC16 );
    void M5EPD_flushAndUpdateArea( const Rect<uint16_t>& rect, m5epd_update_mode_t updateMode );

    void drawProgressBar( float val );

    void doLoop( bool enableButtons = true );

    void HandleButtonL();
    void HandleButtonP();
    void HandleButtonR();
    void HandleSingleFinger( const Point<uint16_t>& hit );

    void ShowSettingsMenu();

    void doShutdownIfOnBattery();
    void doShutdown();
};

extern DisplayManager BaseDisplayManager;