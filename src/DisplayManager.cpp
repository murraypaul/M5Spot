#include "main.h"
#include "DisplayManager.h"

#include "TrackDetails.h"

M5EPD_Canvas Canvas(&M5.EPD);

M5EPD_Canvas& DisplayManager::GetCanvas() { return Canvas; };

DisplayManager::eLayout  DisplayManager::CurrentLayout = eLandscape_BigArt;

const int       LandscapeSmallArt_AlbumArtURLIndex = 1;
const Rect<int> LandscapeSmallArt_AlbumArtRect(0,0,300,300);
const Rect<int> LandscapeSmallArt_ProgressRect(0,300,300,320);
const Rect<int> LandscapeSmallArt_SongTitleRect(16,330,960,380);
const Rect<int> LandscapeSmallArt_AlbumTitleRect(16,400,960,460);
const Rect<int> LandscapeSmallArt_AlbumArtistsRect(16,480,960,540);
const Rect<int> LandscapeSmallArt_MainTrackDetailsRect = LandscapeSmallArt_SongTitleRect.outersect(LandscapeSmallArt_AlbumTitleRect.outersect(LandscapeSmallArt_AlbumArtistsRect));

const int       LandscapeBigArt_AlbumArtURLIndex = 0;
const Rect<int> LandscapeBigArt_AlbumArtRect(0,0,540,540);
const Rect<int> LandscapeBigArt_ProgressRect(540,520,960,540);
const Rect<int> LandscapeBigArt_SongTitleRect(540+16,32,960,32+50);
const Rect<int> LandscapeBigArt_AlbumTitleRect(540+16,128,960,128+50);
const Rect<int> LandscapeBigArt_AlbumArtistsRect(540+16,192,960,192+50);
const Rect<int> LandscapeBigArt_MainTrackDetailsRect = LandscapeBigArt_SongTitleRect.outersect(LandscapeBigArt_AlbumTitleRect.outersect(LandscapeBigArt_AlbumArtistsRect));

#define MAP(ret,name,type) \
const ret name##type() \
{ \
    switch( DisplayManager::CurrentLayout ) \
    { \
        case DisplayManager::eLayout::eLandscape_SmallArt: \
        default: \
            return LandscapeSmallArt_##name##type; \
        case DisplayManager::eLayout::eLandscape_BigArt: \
            return LandscapeBigArt_##name##type; \
    } \
}
MAP(int,AlbumArt,URLIndex);
MAP(Rect<int>,AlbumArt,Rect);
MAP(Rect<int>,Progress,Rect);
MAP(Rect<int>,SongTitle,Rect);
MAP(Rect<int>,AlbumTitle,Rect);
MAP(Rect<int>,AlbumArtists,Rect);
MAP(Rect<int>,MainTrackDetails,Rect);

template <class T>
void Canvas_drawRect( M5EPD_Canvas& canvas, Rect<T> rect, uint32_t colour )
{
    canvas.drawRect(rect.left,rect.top,rect.width(),rect.height(),colour);
}
template <class T>
void Canvas_fillRect( M5EPD_Canvas& canvas, Rect<T> rect, uint32_t colour )
{
    canvas.fillRect(rect.left,rect.top,rect.width(),rect.height(),colour);
}
template <class T>
void Canvas_drawString( M5EPD_Canvas& canvas, Rect<T> rect, String str )
{
    canvas.drawString(str,rect.left,rect.top);
}

template <class T>
void M5EPD_flushAndUpdateArea( M5EPD_Canvas& canvas, Rect<T> rect, m5epd_update_mode_t updateMode )
{
    M5.EPD.WriteFullGram4bpp((uint8_t*)canvas.frameBuffer());
    M5.EPD.UpdateArea(rect.left, rect.top, rect.width(), rect.height(),updateMode);
}

void DisplayManager::Init()
{
    M5.begin();
    M5.TP.SetRotation(0);
    M5.EPD.SetRotation(0);
    M5.EPD.Clear(true);

    Canvas.createCanvas(960, 540);
}

void DisplayManager::NewTrack( const TrackDetails& track )
{
    Canvas.fillCanvas(0);

    // Display album art
    Canvas.drawJpgUrl(track.ArtURL[AlbumArtURLIndex()], AlbumArtRect().top, AlbumArtRect().left, AlbumArtRect().width(), AlbumArtRect().height(), JPEG_DIV_NONE);
    M5EPD_flushAndUpdateArea(Canvas, AlbumArtRect(), UPDATE_MODE_GC16);

    // Display song name
    Canvas_fillRect(Canvas, SongTitleRect(), 0);
    drawString(&FreeSansBold24pt7b, 1, TL_DATUM, track.Name, SongTitleRect().left, SongTitleRect().top);

    // Display album name
    Canvas_fillRect(Canvas, AlbumTitleRect(), 0);
    drawString(&FreeSansOblique18pt7b, 1, TL_DATUM, track.AlbumName, AlbumTitleRect().left, AlbumTitleRect().top);

    // Display artists names
    Canvas_fillRect(Canvas, AlbumArtistsRect(), 0);
    drawString(&FreeSansOblique18pt7b, 1, TL_DATUM, track.ArtistsName, AlbumArtistsRect().left, AlbumArtistsRect().top);

    M5EPD_flushAndUpdateArea(Canvas, MainTrackDetailsRect(), UPDATE_MODE_DU);
}

void DisplayManager::drawString( const GFXfont* font, uint8_t size, uint8_t datum, String str, uint32_t x, uint32_t y )
{
    Canvas.setFreeFont(font);
    Canvas.setTextSize(size);
    Canvas.setTextDatum(datum);
    Canvas.drawString(str, x, y);
}

void DisplayManager::drawProgressBar(float val) 
{
    Canvas_fillRect(Canvas, ProgressRect(), 0);
    Canvas_drawRect(Canvas, ProgressRect().shrinkBy(2,2), 15);
    Canvas_fillRect(Canvas, ProgressRect().shrinkBy(2,2).scaleBy(val,1), 15);
    M5EPD_flushAndUpdateArea(Canvas, ProgressRect(), UPDATE_MODE_DU);
}

void DisplayManager::clearScreen()
{
    Canvas.fillRect(0,0,960,540,0);
}

void DisplayManager::refreshScreen( m5epd_update_mode_t mode )
{
    Canvas.pushCanvas(0,0,mode);
}
