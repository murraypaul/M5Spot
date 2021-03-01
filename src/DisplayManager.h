#pragma once

#include <M5EPD.h>

class TrackDetails;

struct DisplayManager
{
    enum eLayout {
        eLandscape_SmallArt = 0,
        eLandscape_BigArt,
        ePortrait_BigArt
    };
    static eLayout  CurrentLayout;

    static void Init();
    static void NewTrack( const TrackDetails& track );

    static M5EPD_Canvas&   GetCanvas();
    static void drawString( const GFXfont* font, uint8_t size, uint8_t datum, String str, uint32_t x, uint32_t y );
    static void clearScreen();
    static void refreshScreen( m5epd_update_mode_t mode = UPDATE_MODE_GC16 );

    static void drawProgressBar( float val );
};
