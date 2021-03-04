#include "main.h"
#include "LayoutItem.h"

#include <limits>

#include "SpotifyController.h"
#include "DisplayManager.h"
#include "TrackDetails.h"

bool drawIcon( M5EPD_Canvas& canvas, const unsigned char* bmpFS, size_t size, uint16_t x, uint16_t y );

LayoutItem::LayoutItem( Rect<uint16_t> rect, tdAction action )
: Location(rect)
, Action(action)
{
}

LayoutItemWithFont::LayoutItemWithFont( Rect<uint16_t> rect, const GFXfont* font, LayoutItemWithFont::eAlign align, tdAction action )
: LayoutItem(rect,action)
, Font(font)
, TextAlign(align)
{
}

bool LayoutItem::hitTest( const Point<uint16_t>& hit )
{
    if( Action.get() && Action->hasAction() )
    {
        log_d("Possible hit, testing location (%d,%d) vs (%d,%d,%d,%d)"
            , hit.x, hit.y
            , Location.left,Location.top,Location.right,Location.bottom );
        if( Location.contains(hit) )
        {
            Action->doAction();
            return true;
        }
    } 

    return false;
}

LayoutItem_AlbumArt::LayoutItem_AlbumArt( Rect<uint16_t> rect, int index, tdAction action )
: LayoutItem(rect,action)
, Index(index)
{
}

void LayoutItem_AlbumArt::draw( DisplayManager& displayManager, const TrackDetails& track )
{
    displayManager.drawJpgUrl( track.ArtURL[Index].second, Location);
}

void LayoutItem_AlbumArtAutoScaled::draw( DisplayManager& displayManager, const TrackDetails& track )
{
    if( Location.width() == 0 )
        return;
    // Find images closest in size
    int Index = 0;
    double fBestScale = std::numeric_limits<double>::max();
    for( int i = 0 ; i < 3 ; i++ )
    {
        if( track.ArtURL[i].first.cx == 0 )
            continue;
        log_d("Checking size of (%d,%d) against max preferred of %d", track.ArtURL[i].first.cx, track.ArtURL[i].first.cy, displayManager.MaxPreferredImageSize);
        if( displayManager.MaxPreferredImageSize > 0 && track.ArtURL[i].first.cx > displayManager.MaxPreferredImageSize )
            continue;
        double fScale = (double)track.ArtURL[i].first.cx / Location.width();
        if( fScale < 1 )
            fScale = 1 / fScale;
        if( fabs(fScale) < fBestScale )
        {
            log_d("New best index is %d",Index);
            Index = i;
            fBestScale = fScale;
        }
    }
    displayManager.drawJpgUrlScaled( track.ArtURL[Index].second, track.ArtURL[Index].first, Location);
}

void LayoutItemWithFont::drawString( DisplayManager& displayManager, String str )
{
    switch( TextAlign )
    {
        case eAlign::eLeft:
            displayManager.drawString( Font, TL_DATUM, str, Location);
            break;
        case eAlign::eRight:
            displayManager.drawString( Font, TR_DATUM, str, Location);
            break;
        case eAlign::eCentre:
            displayManager.drawString( Font, TC_DATUM, str, Location);
            break;
    }
}

void LayoutItem_SongTitle::draw( DisplayManager& displayManager, const TrackDetails& track )
{
    drawString( displayManager, track.Name);
}

void LayoutItem_AlbumTitle::draw( DisplayManager& displayManager, const TrackDetails& track )
{
    drawString( displayManager, track.AlbumName);
}

void LayoutItem_AlbumArtists::draw( DisplayManager& displayManager, const TrackDetails& track )
{
    drawString( displayManager, track.ArtistsName);
}

LayoutItem_ProgressBar::LayoutItem_ProgressBar( Rect<uint16_t> rect, tdAction action )
: LayoutItem(rect,action)
{
    DrawOnlyOnNewTrack = false;
}

void LayoutItem_ProgressBar::draw( DisplayManager& displayManager, const TrackDetails& track )
{
    double val = 0.0;
    if( track.DurationMS > 0 )
        val = (double)track.ProgressMS/track.DurationMS;
    displayManager.fillRect(Location, 0);
    displayManager.drawRect(Location.shrinkBy(2,2), 15);
    displayManager.fillRect(Location.shrinkBy(2,2).scaleBy(val,1), 15);
//    displayManager.M5EPD_flushAndUpdateArea(Location, UPDATE_MODE_DU);    
}

LayoutItem_Button::LayoutItem_Button( Rect<uint16_t> rect, const unsigned char* data, size_t size, tdAction action, bool activeOnly )
: LayoutItem(rect,action)
, Data(data), DataSize(size)
{
    DrawOnActiveDeviceChanged = activeOnly;
    HideIfNoActiveDevice = activeOnly;
}

void LayoutItem_Button::draw( DisplayManager& displayManager, const TrackDetails& track )
{
    if( HideIfNoActiveDevice && !SpotifyController::HasActiveDevice )
        displayManager.fillRect(Location,0);
    else
        drawIcon(displayManager.GetCanvas(),Data,DataSize,Location.left,Location.top);
}

LayoutItem_ButtonWithHighlight::LayoutItem_ButtonWithHighlight( Rect<uint16_t> rect, const unsigned char* data, size_t size, tdHighlightFunc func, tdAction action, bool activeOnly )
: LayoutItem_Button(rect,data,size,action,activeOnly)
, HighlightFunc(func)
{
}

void LayoutItem_ButtonWithHighlight::draw( DisplayManager& displayManager, const TrackDetails& track )
{
    if( HideIfNoActiveDevice && !SpotifyController::HasActiveDevice )
        displayManager.fillRect(Location,0);
    else
    {
        drawIcon(displayManager.GetCanvas(),Data,DataSize,Location.left,Location.top);
        if( HighlightFunc && HighlightFunc() )
            displayManager.drawRect(Location,15);
    }
}

LayoutItem_StaticText::LayoutItem_StaticText( Rect<uint16_t> rect, const GFXfont* font, LayoutItemWithFont::eAlign align, String text, tdAction action )
: LayoutItemWithFont(rect,font,align,action)
, Text(text)
{
}

void LayoutItem_Rectangle::draw( DisplayManager& displayManager, const TrackDetails& track )
{
    displayManager.drawRect(Location, 15);
}

void LayoutItem_StaticText::draw( DisplayManager& displayManager, const TrackDetails& track )
{
    drawString( displayManager, Text);
}
