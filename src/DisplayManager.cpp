#include "main.h"
#include "DisplayManager.h"

#include "LayoutItem.h"
#include "TrackDetails.h"
#include "SpotifyController.h"

#include "Icons.h"

#include "Timer.h"

DisplayManager BaseDisplayManager;

DisplayManager::DisplayManager()
: Canvas(&M5.EPD)
, TempJpegCanvas(&M5.EPD)
{

}

M5EPD_Canvas& DisplayManager::GetCanvas()
{
    return Canvas;
}

void DisplayManager::Init( bool appInit )
{
    if( appInit )
    {
        M5.begin();

        preferences.begin(Preferences_App);
        CurrentLayout = (eLayout)preferences.getChar("Layout",(int8_t)CurrentLayout);
        Rotation = preferences.getShort("Rotation",(int16_t)Rotation);
        MaxPreferredImageSize = preferences.getShort("MaxJpeg",(int16_t)MaxPreferredImageSize);
        preferences.end();
    }

    SetLayout(CurrentLayout);
}

void DisplayManager::SetLayout( eLayout layout )
{
    Canvas.deleteCanvas();

    LayoutItems.clear();

    bool clearCanvas = true;
    bool flip = Rotation >= 180;
    switch( layout )
    {
    case eLayout::eLandscape_BigArt:
    default:
        Rotation = 0 + (flip?180:0);
        CanvasPos = {0,0};
        CanvasSize = {960,540};
        LayoutItems.push_back( std::make_shared<LayoutItem_AlbumArtAutoScaled>(Rect<uint16_t>{0,0,540,540}) );
        LayoutItems.push_back( std::make_shared<LayoutItem_SongTitle>(Rect<uint16_t>{540+16,32,960,32+50},&FreeSansBold24pt7b) );
        LayoutItems.push_back( std::make_shared<LayoutItem_AlbumTitle>(Rect<uint16_t>{540+16,128,960,128+50},&FreeSansOblique18pt7b) );
        LayoutItems.push_back( std::make_shared<LayoutItem_AlbumArtists>(Rect<uint16_t>{540+16,192,960,192+50},&FreeSans18pt7b) );
        LayoutItems.push_back( std::make_shared<LayoutItem_ProgressBar>(Rect<uint16_t>{540,520,960,540}) );

        {
            int iButton = 0;
            uint16_t margin = 540 + 18;
            uint8_t spacing = 16;
            uint8_t size = 64;
            LayoutItems.push_back( std::make_shared<LayoutItem_Button>(Rect<uint16_t>{{margin + (iButton++)*(size+spacing), 540-120},{size, size}}, icons8_prev_64_4bpp_bmp, sizeof(icons8_prev_64_4bpp_bmp), std::make_shared<LayoutItemAction_StdFunction>([](){ SpotifyController::PlayPreviousTrack(); }), true) );
            LayoutItems.push_back( std::make_shared<LayoutItem_Button>(Rect<uint16_t>{{margin + (iButton++)*(size+spacing), 540-120},{size, size}}, icons8_first_64_4bpp_bmp, sizeof(icons8_first_64_4bpp_bmp), std::make_shared<LayoutItemAction_StdFunction>([](){ SpotifyController::RestartCurrentTrack(); }), true) );
            LayoutItems.push_back( std::make_shared<LayoutItem_Button>(Rect<uint16_t>{{margin + (iButton++)*(size+spacing), 540-120},{size, size}}, icons8_resume_64_4bpp_bmp, sizeof(icons8_resume_64_4bpp_bmp), std::make_shared<LayoutItemAction_StdFunction>([](){ SpotifyController::PlayOrPauseCurrentTrack(); }), true) );
            LayoutItems.push_back( std::make_shared<LayoutItem_Button>(Rect<uint16_t>{{margin + (iButton++)*(size+spacing), 540-120},{size, size}}, icons8_right_button_64_4bpp_bmp, sizeof(icons8_right_button_64_4bpp_bmp), std::make_shared<LayoutItemAction_StdFunction>([](){ SpotifyController::PlayNextTrack(); }), true) );
            LayoutItems.push_back( std::make_shared<LayoutItem_Button>(Rect<uint16_t>{{margin + (iButton++)*(size+spacing), 540-120},{size, size}}, icons8_automatic_64_4bpp_bmp, sizeof(icons8_automatic_64_4bpp_bmp), std::make_shared<LayoutItemAction_StdFunction>([this](){ this->ShowSettingsMenu(); })) );
        }
        break;
    case eLayout::eLandscape_SmallArt:
        Rotation = 0 + (flip?180:0);
        CanvasPos = {0,0};
        CanvasSize = {960,540};
        LayoutItems.push_back( std::make_shared<LayoutItem_AlbumArt>(Rect<uint16_t>{0,0,300,300},1) );
        LayoutItems.push_back( std::make_shared<LayoutItem_SongTitle>(Rect<uint16_t>{16,330,960,380},&FreeSansBold18pt7b) );
        LayoutItems.push_back( std::make_shared<LayoutItem_AlbumTitle>(Rect<uint16_t>{16,400,960,460},&FreeSansOblique12pt7b) );
        LayoutItems.push_back( std::make_shared<LayoutItem_AlbumArtists>(Rect<uint16_t>{16,480,960,540},&FreeSans12pt7b) );
        LayoutItems.push_back( std::make_shared<LayoutItem_ProgressBar>(Rect<uint16_t>{0,300,300,320}) );

        {
            int iButton = 0;
            uint16_t margin = 300 + 46;
            uint8_t spacing = 32;
            uint8_t size = 64;
            LayoutItems.push_back( std::make_shared<LayoutItem_Button>(Rect<uint16_t>{{margin + (iButton++)*(size+spacing), 300 - 64},{size, size}}, icons8_prev_64_4bpp_bmp, sizeof(icons8_prev_64_4bpp_bmp), std::make_shared<LayoutItemAction_StdFunction>([](){ SpotifyController::PlayPreviousTrack(); }), true) );
            LayoutItems.push_back( std::make_shared<LayoutItem_Button>(Rect<uint16_t>{{margin + (iButton++)*(size+spacing), 300 - 64},{size, size}}, icons8_first_64_4bpp_bmp, sizeof(icons8_first_64_4bpp_bmp), std::make_shared<LayoutItemAction_StdFunction>([](){ SpotifyController::RestartCurrentTrack(); }), true) );
            LayoutItems.push_back( std::make_shared<LayoutItem_Button>(Rect<uint16_t>{{margin + (iButton++)*(size+spacing), 300 - 64},{size, size}}, icons8_resume_64_4bpp_bmp, sizeof(icons8_resume_64_4bpp_bmp), std::make_shared<LayoutItemAction_StdFunction>([](){ SpotifyController::PlayOrPauseCurrentTrack(); }), true) );
            LayoutItems.push_back( std::make_shared<LayoutItem_Button>(Rect<uint16_t>{{margin + (iButton++)*(size+spacing), 300 - 64},{size, size}}, icons8_right_button_64_4bpp_bmp, sizeof(icons8_right_button_64_4bpp_bmp), std::make_shared<LayoutItemAction_StdFunction>([](){ SpotifyController::PlayNextTrack(); }), true) );
            LayoutItems.push_back( std::make_shared<LayoutItem_Button>(Rect<uint16_t>{{margin + (iButton++)*(size+spacing), 300 - 64},{size, size}}, icons8_automatic_64_4bpp_bmp, sizeof(icons8_automatic_64_4bpp_bmp), std::make_shared<LayoutItemAction_StdFunction>([this](){ this->ShowSettingsMenu(); })) );
        }
        break;
    case eLayout::ePortrait_BigArt:
        Rotation = 90 + (flip?180:0);
        CanvasPos = {0,0};
        CanvasSize = {540,960};
        LayoutItems.push_back( std::make_shared<LayoutItem_AlbumArtAutoScaled>(Rect<uint16_t>{0,0,540,540}) );
        LayoutItems.push_back( std::make_shared<LayoutItem_SongTitle>(Rect<uint16_t>{16,570,540,630},&FreeSansBold18pt7b) );
        LayoutItems.push_back( std::make_shared<LayoutItem_AlbumTitle>(Rect<uint16_t>{16,640,540,690},&FreeSansOblique12pt7b) );
        LayoutItems.push_back( std::make_shared<LayoutItem_AlbumArtists>(Rect<uint16_t>{16,700,540,760},&FreeSans12pt7b) );
        LayoutItems.push_back( std::make_shared<LayoutItem_ProgressBar>(Rect<uint16_t>{0,540,540,560}) );

        {
            int iButton = 0;
            uint16_t margin = 46;
            uint8_t spacing = 32;
            uint8_t size = 64;
            LayoutItems.push_back( std::make_shared<LayoutItem_Button>(Rect<uint16_t>{{margin + (iButton++)*(size+spacing), 840},{size, size}}, icons8_prev_64_4bpp_bmp, sizeof(icons8_prev_64_4bpp_bmp), std::make_shared<LayoutItemAction_StdFunction>([](){ SpotifyController::PlayPreviousTrack(); }), true) );
            LayoutItems.push_back( std::make_shared<LayoutItem_Button>(Rect<uint16_t>{{margin + (iButton++)*(size+spacing), 840},{size, size}}, icons8_first_64_4bpp_bmp, sizeof(icons8_first_64_4bpp_bmp), std::make_shared<LayoutItemAction_StdFunction>([](){ SpotifyController::RestartCurrentTrack(); }), true) );
            LayoutItems.push_back( std::make_shared<LayoutItem_Button>(Rect<uint16_t>{{margin + (iButton++)*(size+spacing), 840},{size, size}}, icons8_resume_64_4bpp_bmp, sizeof(icons8_resume_64_4bpp_bmp), std::make_shared<LayoutItemAction_StdFunction>([](){ SpotifyController::PlayOrPauseCurrentTrack(); }), true) );
            LayoutItems.push_back( std::make_shared<LayoutItem_Button>(Rect<uint16_t>{{margin + (iButton++)*(size+spacing), 840},{size, size}}, icons8_right_button_64_4bpp_bmp, sizeof(icons8_right_button_64_4bpp_bmp), std::make_shared<LayoutItemAction_StdFunction>([](){ SpotifyController::PlayNextTrack(); }), true) );
            LayoutItems.push_back( std::make_shared<LayoutItem_Button>(Rect<uint16_t>{{margin + (iButton++)*(size+spacing), 840},{size, size}}, icons8_automatic_64_4bpp_bmp, sizeof(icons8_automatic_64_4bpp_bmp), std::make_shared<LayoutItemAction_StdFunction>([this](){ this->ShowSettingsMenu(); })) );
        }
        break;
    case eLayout::eSettings:
        // Rotation unchanged
        if( Rotation % 180 == 0 )
            CanvasPos = {280,70};
        else
            CanvasPos = {70,280};
        CanvasSize = {400,400};
        clearCanvas = false;

        LayoutItems.push_back( std::make_shared<LayoutItem_Rectangle>(Rect<uint16_t>{0,0,400,400}) );
        LayoutItems.push_back( std::make_shared<LayoutItem_Rectangle>(Rect<uint16_t>{5,5,395,395}) );
        LayoutItems.push_back( std::make_shared<LayoutItem_StaticText>(Rect<uint16_t>{10,10,390,40},&FreeSansBold24pt7b,LayoutItemWithFont::eAlign::eCentre,String("Settings"),nullptr) );

        {
            LayoutItems.push_back( std::make_shared<LayoutItem_StaticText>(Rect<uint16_t>{10,60,390,80},&FreeSans12pt7b,LayoutItemWithFont::eAlign::eLeft,String("Layout"),nullptr) );

            int iButton = 0;
            uint8_t margin = 10 + 19;
            uint8_t spacing = 24;
            uint8_t size = 64;
            LayoutItems.push_back( std::make_shared<LayoutItem_ButtonWithHighlight>(Rect<uint16_t>{{margin + (iButton++)*(size+spacing), 100},{64, 64}}, icon_layout_landscape_big_4bpp_bmp, sizeof(icon_layout_landscape_big_4bpp_bmp)
            , [](){ return BaseDisplayManager.CurrentLayout == DisplayManager::eLayout::eLandscape_BigArt; }
            , std::make_shared<LayoutItemAction_StdFunction>([this](){ BaseDisplayManager.SetLayout(DisplayManager::eLayout::eLandscape_BigArt); this->Rotation = BaseDisplayManager.Rotation; if( this->Rotation % 180 == 0 ) this->CanvasPos = {280,70}; else this->CanvasPos = {70,280}; BaseDisplayManager.redraw(); this->redraw(); })) );
            LayoutItems.push_back( std::make_shared<LayoutItem_ButtonWithHighlight>(Rect<uint16_t>{{margin + (iButton++)*(size+spacing), 100},{64, 64}}, icon_layout_landscape_small_4bpp_bmp, sizeof(icon_layout_landscape_small_4bpp_bmp)
            , [](){ return BaseDisplayManager.CurrentLayout == DisplayManager::eLayout::eLandscape_SmallArt; }
            , std::make_shared<LayoutItemAction_StdFunction>([this](){ BaseDisplayManager.SetLayout(DisplayManager::eLayout::eLandscape_SmallArt); this->Rotation = BaseDisplayManager.Rotation; if( this->Rotation % 180 == 0 ) this->CanvasPos = {280,70}; else this->CanvasPos = {70,280}; BaseDisplayManager.redraw(); this->redraw(); })) );
            LayoutItems.push_back( std::make_shared<LayoutItem_ButtonWithHighlight>(Rect<uint16_t>{{margin + (iButton++)*(size+spacing), 100},{64, 64}}, icon_layout_portait_big_4bpp_bmp, sizeof(icon_layout_portait_big_4bpp_bmp)
            , [](){ return BaseDisplayManager.CurrentLayout == DisplayManager::eLayout::ePortrait_BigArt; }
            , std::make_shared<LayoutItemAction_StdFunction>([this](){ BaseDisplayManager.SetLayout(DisplayManager::eLayout::ePortrait_BigArt); this->Rotation = BaseDisplayManager.Rotation; if( this->Rotation % 180 == 0 ) this->CanvasPos = {280,70}; else this->CanvasPos = {70,280}; BaseDisplayManager.redraw(); this->redraw(); })) );
            LayoutItems.push_back( std::make_shared<LayoutItem_ButtonWithHighlight>(Rect<uint16_t>{{margin + (iButton++)*(size+spacing), 100},{64, 64}}, icon_rotate_4bpp_bmp, sizeof(icon_rotate_4bpp_bmp)
            , [](){ return BaseDisplayManager.Rotation >= 180; }
            , std::make_shared<LayoutItemAction_StdFunction>([this](){ BaseDisplayManager.Rotation = (BaseDisplayManager.Rotation + 180) % 360; this->Rotation = (this->Rotation + 180) % 180; BaseDisplayManager.SetLayout(BaseDisplayManager.CurrentLayout); this->Rotation = BaseDisplayManager.Rotation; if( this->Rotation % 180 == 0 ) this->CanvasPos = {280,70}; else this->CanvasPos = {70,280}; BaseDisplayManager.redraw(); this->redraw(); })) );
        }
        {
            uint16_t baseY = 180;
            LayoutItems.push_back( std::make_shared<LayoutItem_StaticText>(Rect<uint16_t>{10,baseY,390,baseY+20},&FreeSans12pt7b,LayoutItemWithFont::eAlign::eLeft,String("Album Art Source Resolution"),nullptr) );

            int iButton = 0;
            uint8_t margin = 10 + 19;
            uint8_t spacing = 24;
            uint8_t size = 64;
            LayoutItems.push_back( std::make_shared<LayoutItem_ButtonWithHighlight>(Rect<uint16_t>{{margin + (iButton++)*(size+spacing), baseY+40},{64, 64}}, icon_image_size_small_4bpp_bmp, sizeof(icon_image_size_small_4bpp_bmp)
            , [](){ return BaseDisplayManager.MaxPreferredImageSize == 128; }
            , std::make_shared<LayoutItemAction_StdFunction>([this](){ BaseDisplayManager.MaxPreferredImageSize = 128; BaseDisplayManager.redraw(); this->redraw(); })) );
            LayoutItems.push_back( std::make_shared<LayoutItem_ButtonWithHighlight>(Rect<uint16_t>{{margin + (iButton++)*(size+spacing), baseY+40},{64, 64}}, icon_image_size_medium_4bpp_bmp, sizeof(icon_image_size_medium_4bpp_bmp)
            , [](){ return BaseDisplayManager.MaxPreferredImageSize == 300; }
            , std::make_shared<LayoutItemAction_StdFunction>([this](){ BaseDisplayManager.MaxPreferredImageSize = 300; BaseDisplayManager.redraw(); this->redraw(); })) );
            LayoutItems.push_back( std::make_shared<LayoutItem_ButtonWithHighlight>(Rect<uint16_t>{{margin + (iButton++)*(size+spacing), baseY+40},{64, 64}}, icon_image_size_large_4bpp_bmp, sizeof(icon_image_size_large_4bpp_bmp)
            , [](){ return BaseDisplayManager.MaxPreferredImageSize == 0; }
            , std::make_shared<LayoutItemAction_StdFunction>([this](){ BaseDisplayManager.MaxPreferredImageSize = 0; BaseDisplayManager.redraw(); this->redraw(); })) );
        }

        LayoutItems.push_back( std::make_shared<LayoutItem_StaticText>(Rect<uint16_t>{10,400-36,390,400-24},&FreeSans12pt7b,LayoutItemWithFont::eAlign::eLeft,String("Icons by Icons8"),nullptr) );
        LayoutItems.push_back( std::make_shared<LayoutItem_Button>(Rect<uint16_t>{{400-64-12, 400-64-12},{64, 64}}, icons8_close_window_64_4bpp_bmp, sizeof(icons8_close_window_64_4bpp_bmp), std::make_shared<LayoutItemAction_StdFunction>([this](){ this->ShouldClose = true; })) );

        for( auto& item : LayoutItems )
            item->DrawOnlyOnNewTrack = false;

        break;
    }

    CurrentLayout = layout;

    Rotation = Rotation % 360;
    log_d("New rotation %d",Rotation);

    M5.TP.SetRotation(Rotation);
    M5.EPD.SetRotation(Rotation);
    if( clearCanvas )
        M5.EPD.Clear(true);

    log_d("Canvas size (%d,%d) at (%d,%d)", CanvasSize.cx, CanvasSize.cy, CanvasPos.x, CanvasPos.y);
    Canvas.createCanvas(CanvasSize.cx,CanvasSize.cy);
}

void DisplayManager::drawJpgUrl( String url, const Rect<uint16_t>& rect )
{
    Canvas.drawJpgUrl(url.c_str(), rect.left, rect.top, rect.width(), rect.height(), 0, 0, JPEG_DIV_NONE);
//    M5EPD_flushAndUpdateArea(rect, UPDATE_MODE_GC16);
}

void DisplayManager::drawJpgUrlScaled( String url, const Size<uint16_t>& sourceSize, const Rect<uint16_t>& targetRect )
{
    FunctionTimer timer("drawJpgUrlScaled");
    if( sourceSize.cx == targetRect.width() && sourceSize.cy == targetRect.height()
    || sourceSize.cx <= 0 || sourceSize.cy <= 0 || targetRect.width() <= 0 || targetRect.height() <= 0 )
    {
        drawJpgUrl(url,targetRect);
        return;
    }

    if( url != CurrentCachedJpegURL )
    {
        TempJpegCanvas.deleteCanvas();
        if( !TempJpegCanvas.createCanvas(sourceSize.cx, sourceSize.cy) )
        {
            drawJpgUrl(url,targetRect);
            return;
        }
        TempJpegCanvas.drawJpgUrl(url.c_str(), 0, 0, sourceSize.cx, sourceSize.cy, 0, 0, JPEG_DIV_NONE);
        CurrentCachedJpegURL = url;
    }


    fillRect(targetRect,0);

    double xScale = (double) sourceSize.cx / targetRect.width();
    double yScale = (double) sourceSize.cy / targetRect.height();
    log_d("Starting scaled copy (%d,%d) -> (%d,%d) = (%lf,%lf)",sourceSize.cx,sourceSize.cy,targetRect.width(),targetRect.height(),xScale,yScale);
    for( uint16_t y = 0 ; y < targetRect.height() ; y++ )
    {
        for( uint16_t x = 0 ; x < targetRect.width() ; x++ )
        {
            uint16_t sx = x * xScale;    
            uint16_t sy = y * yScale;
//            log_d("copying (%d,%d) to (%d,%d)",sx,sy,x,y);
            uint8_t colour = TempJpegCanvas.readPixel(sx,sy);
            Canvas.drawPixel(x,y,colour);
        }
//        auto ts = millis();
//        log_d("%d: row %d done", ts, y);
    }
    log_d("Finished scaled copy");
    
//    M5EPD_flushAndUpdateArea(targetRect, UPDATE_MODE_GC16);
}

void DisplayManager::drawString( const GFXfont* font, uint8_t datum, String str, const Rect<uint16_t>& rect )
{
    switch( datum )
    {
        case TL_DATUM:
        default:
            drawString(font, datum, str, rect.left, rect.top);
            break;
        case TR_DATUM:
            drawString(font, datum, str, rect.right, rect.top);
            break;
        case TC_DATUM:
            drawString(font, datum, str, (rect.left+rect.right)/2, rect.top);
            break;
    }
}

void DisplayManager::drawString( const GFXfont* font, uint8_t datum, String str, uint32_t x, uint32_t y )
{
    if( font )
        Canvas.setFreeFont(font);
    Canvas.setTextDatum(datum);
    Canvas.drawString(str, x, y);
}

bool DisplayManager::showTrack( const TrackDetails& track ) 
{
    DesiredUpdateRect = Rect<uint16_t>{0,0,0,0};
    DesiredUpdateMode = UPDATE_MODE_NONE;

    bool bNewTrack = CurrentTrack.ID != track.ID;
    if( bNewTrack )
        clearScreen();
    bool bActiveDeviceChanged = SpotifyController::LastActiveDeviceCleared;
    bool bOldActive = SpotifyController::HasActiveDevice;
    if( !SpotifyController::IsPlaying || !bOldActive )
    {
        SpotifyController::UpdateActiveDevice();
        if( SpotifyController::HasActiveDevice != bOldActive )
            bActiveDeviceChanged = true;
        SpotifyController::LastActiveDeviceCleared = false;
    }
    if( !bNewTrack && !bActiveDeviceChanged && !SpotifyController::IsPlaying )
        return false;

    Rect<uint16_t> redrawRect{0,0,0,0};
    for( auto& item : LayoutItems )
    {
        if( !item.get( ))
            continue;
        if( bNewTrack || !item->DrawOnlyOnNewTrack || (bActiveDeviceChanged && item->DrawOnActiveDeviceChanged) )
        {
            item->draw( *this, track );
            if( redrawRect.width() == 0 )
                redrawRect = item->Location;
            else
                redrawRect = redrawRect.outersect(item->Location);
        }
    }
//    log_i("Redraw rect = (%d,%d,%d,%d)",redrawRect.left,redrawRect.top,redrawRect.right,redrawRect.bottom);
    if( bNewTrack )
    {
//        log_i("New track");
        DesiredUpdateRect = Rect<uint16_t>(CanvasPos,CanvasSize);
        DesiredUpdateMode = UPDATE_MODE_GC16;
    }
    else if( redrawRect.width() > 0 )
    {
//        log_i("Have redraw rect");
        DesiredUpdateRect = redrawRect;
        DesiredUpdateMode = UPDATE_MODE_DU4;
    }

    CurrentTrack = track;

    return true;
}

void DisplayManager::redraw()
{
    for( auto& item : LayoutItems )
    {
        if( !item.get( ))
            continue;
        item->draw( *this, CurrentTrack );
    }
    refreshScreen(UPDATE_MODE_GC16);
}

void DisplayManager::clearScreen()
{
    Canvas.fillCanvas(0);
}

void DisplayManager::refreshScreen( m5epd_update_mode_t mode )
{
    Canvas.pushCanvas(CanvasPos.x,CanvasPos.y,mode);
}

void DisplayManager::drawRect( const Rect<uint16_t>& rect, uint32_t colour )
{
    Canvas.drawRect(rect.left,rect.top,rect.width(),rect.height(),colour);
}
void DisplayManager::fillRect( const Rect<uint16_t>& rect, uint32_t colour )
{
    Canvas.fillRect(rect.left,rect.top,rect.width(),rect.height(),colour);
}
void DisplayManager::M5EPD_flushAndUpdateArea( const Rect<uint16_t>& rect, m5epd_update_mode_t updateMode )
{
    M5.EPD.WriteFullGram4bpp((uint8_t*)Canvas.frameBuffer());
    M5.EPD.UpdateArea(rect.left, rect.top, rect.width(), rect.height(),updateMode);
} 

void DisplayManager::doLoop( bool enableButtons )
{
    SpotifyController::GetTokenIfNeeded();

    // M5Stack handler
    M5.update();
    if (enableButtons && M5.BtnL.wasPressed()) {
        log_i("BtnL - Previous");
        HandleButtonL();
    }
    else if (enableButtons && M5.BtnP.wasPressed()) {
        log_i("BtnP - Toggle");
        HandleButtonP();
    }
    else if (enableButtons && M5.BtnR.wasPressed()) {
        log_i("BtnR - Next");
        HandleButtonR();
    }
    else if( M5.TP.avaliable() )
    {
        // Prevent repeated press detection
        static uint32_t lastFingerDown = 0;
        if( !M5.TP.isFingerUp() )
        {
            M5.TP.update();
            Point<uint16_t> f1 = {M5.TP.readFingerX(0), M5.TP.readFingerY(0)};
            Point<uint16_t> f2 = {M5.TP.readFingerX(1), M5.TP.readFingerY(1)};
            auto numFingers = M5.TP.getFingerNum();
//            delay(100);
            M5.TP.flush();
            // Get spurious touches at startup
            if( f1 != Point<uint16_t>(0,0) )
            {
                auto currentmillis = millis();
                if( currentmillis - lastFingerDown > 500 || currentmillis < lastFingerDown )
                {
                    log_d("Finger down at (%d,%d), time %d vs last time %d",f1.x,f1.y,currentmillis,lastFingerDown);
                    lastFingerDown = currentmillis;
                    switch( numFingers )
                    {
                        case 2:
                //          HandleDoubleFinger( f1.first, f1.second, f2.first, f2.second );
                        break;
                        case 1:
                            HandleSingleFinger( f1 );
                        break;
                    }
                    delay(200);
                }
            }
        }
    }

    SpotifyController::UpdateFromCurrentlyPlayingIfNeeded();
    if( DesiredUpdateMode != UPDATE_MODE_NONE )
    {
//        log_i("Delayed update");
        M5.EPD.WritePartGram4bpp(CanvasPos.x, CanvasPos.y, CanvasSize.cx, CanvasSize.cy, (uint8_t*)Canvas.frameBuffer());
        if( !PopupDialogActive )
        {
//            log_i("Delayed update - no popup");
            M5.EPD.UpdateArea(CanvasPos.x, CanvasPos.y, CanvasSize.cx, CanvasSize.cy, DesiredUpdateMode);
            DesiredUpdateMode = UPDATE_MODE_NONE;
        }
    }
    if( this != &BaseDisplayManager && BaseDisplayManager.DesiredUpdateMode != UPDATE_MODE_NONE )
    {
//        log_i("Delayed update - not base");
        M5.EPD.WritePartGram4bpp(BaseDisplayManager.CanvasPos.x, BaseDisplayManager.CanvasPos.y, BaseDisplayManager.CanvasSize.cx, BaseDisplayManager.CanvasSize.cy, (uint8_t*)BaseDisplayManager.Canvas.frameBuffer());
        M5.EPD.WritePartGram4bpp(CanvasPos.x, CanvasPos.y, CanvasSize.cx, CanvasSize.cy, (uint8_t*)Canvas.frameBuffer());
        Rect<uint16_t> updateRect = Rect<uint16_t>{CanvasPos,CanvasSize}.outersect(BaseDisplayManager.DesiredUpdateRect);        
        M5.EPD.UpdateArea(updateRect.left, updateRect.top, updateRect.right, updateRect.bottom, BaseDisplayManager.DesiredUpdateMode);
        BaseDisplayManager.DesiredUpdateMode = UPDATE_MODE_NONE;
    }
}

void DisplayManager::HandleButtonL() { SpotifyController::PlayPreviousTrack(); };
void DisplayManager::HandleButtonP() { SpotifyController::PlayOrPauseCurrentTrack(); };
void DisplayManager::HandleButtonR() { SpotifyController::PlayNextTrack(); };
void DisplayManager::HandleSingleFinger( const Point<uint16_t>& hitIn )
{
    Point<uint16_t> hit = hitIn - CanvasPos;
    log_d("Converted hit from (%d,%d) to (%d,%d), canvas pos (%d,%d)"
        , hitIn.x, hitIn.y, hit.x, hit.y, CanvasPos.x, CanvasPos.y );
    for( auto& item : LayoutItems )
        if( item->hitTest(hit) )
            break;
}

void DisplayManager::ShowSettingsMenu()
{
    DisplayManager settingsManager;
    settingsManager.Rotation = Rotation;
    settingsManager.SetLayout(eLayout::eSettings);

    settingsManager.redraw();
    settingsManager.ShouldClose = false;

    PopupDialogActive = true;
    while( !settingsManager.ShouldClose )
    {
        settingsManager.doLoop(false);
        delay(100);
//        yield();
    }

    preferences.begin(Preferences_App);
    preferences.putChar("Layout",(int8_t)CurrentLayout);
    preferences.putShort("Rotation",(int16_t)Rotation);
    preferences.putShort("MaxJpeg",(int16_t)MaxPreferredImageSize);
    preferences.end();
    
    settingsManager.clearScreen();
    settingsManager.Canvas.pushCanvas(settingsManager.CanvasPos.x,settingsManager.CanvasPos.y,UPDATE_MODE_GC16);
    Canvas.pushCanvas(CanvasPos.x,CanvasPos.y,UPDATE_MODE_GC16);
    PopupDialogActive = false;
}
