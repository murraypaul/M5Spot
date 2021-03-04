#pragma once

#include <memory>
#include <list>
#include <functional>

#include <M5EPD.h>

class TrackDetails;
class DisplayManager;

class LayoutItemAction
{
public:
    virtual ~LayoutItemAction() = default;

    virtual bool    hasAction() { return true; };
    virtual void    doAction() = 0;
};

class LayoutItemAction_None : public LayoutItemAction
{
public:
    virtual bool    hasAction() { return false; };
    virtual void    doAction() {};
};

class LayoutItemAction_StdFunction : public LayoutItemAction
{
public:
    LayoutItemAction_StdFunction( std::function<void(void)> func ) : Func(func) {};

    std::function<void(void)>   Func;

    virtual bool    hasAction() { return !!Func; };
    virtual void    doAction() { Func(); };
};

class LayoutItem
{
public:
    using tdAction = std::shared_ptr<LayoutItemAction>;
    LayoutItem( Rect<uint16_t> rect, tdAction action = nullptr );
    virtual ~LayoutItem() = default;

    Rect<uint16_t>   Location;
    tdAction    Action;
    bool        DrawOnlyOnNewTrack = true;
    bool        DrawOnActiveDeviceChanged = false;
    bool        HideIfNoActiveDevice = false;

    virtual void draw( DisplayManager&, const TrackDetails& ) = 0;
    virtual bool hitTest( const Point<uint16_t>& );
};

class LayoutItemWithFont : public LayoutItem
{
public:
    using tdAction = std::shared_ptr<LayoutItemAction>;
    enum eAlign {
        eLeft,
        eRight,
        eCentre
    };

    LayoutItemWithFont( Rect<uint16_t> rect, const GFXfont* font = nullptr, eAlign align = eAlign::eLeft, tdAction action = nullptr );
    virtual ~LayoutItemWithFont() = default;

    const GFXfont*  Font = nullptr;
    eAlign  TextAlign = eAlign::eLeft;

    virtual void drawString( DisplayManager&, String str );
};

class LayoutItem_AlbumArt : public LayoutItem
{
public:
    LayoutItem_AlbumArt( Rect<uint16_t> rect, int index, tdAction action = nullptr );

    int         Index = 0;
    virtual void draw( DisplayManager&, const TrackDetails& ) override;
};

class LayoutItem_AlbumArtAutoScaled : public LayoutItem
{
public:
    using LayoutItem::LayoutItem;
    virtual void draw( DisplayManager&, const TrackDetails& ) override;
};

class LayoutItem_SongTitle : public LayoutItemWithFont
{
public:
    using LayoutItemWithFont::LayoutItemWithFont;
    virtual void draw( DisplayManager&, const TrackDetails& ) override;
};

class LayoutItem_AlbumTitle : public LayoutItemWithFont
{
public:
    using LayoutItemWithFont::LayoutItemWithFont;
    virtual void draw( DisplayManager&, const TrackDetails& ) override;
};

class LayoutItem_AlbumArtists : public LayoutItemWithFont
{
public:
    using LayoutItemWithFont::LayoutItemWithFont;
    virtual void draw( DisplayManager&, const TrackDetails& ) override;
};

class LayoutItem_ProgressBar : public LayoutItem
{
public:
    LayoutItem_ProgressBar(Rect<uint16_t> rect, tdAction action = nullptr);

    virtual void draw( DisplayManager&, const TrackDetails& ) override;
};

class LayoutItem_Button : public LayoutItem
{
public:
    LayoutItem_Button( Rect<uint16_t> rect, const unsigned char* data, size_t size, tdAction action = nullptr, bool activeOnly = false );

    const unsigned char* Data = nullptr;
    size_t  DataSize = 0;
    virtual void draw( DisplayManager&, const TrackDetails& ) override;
};

class LayoutItem_ButtonWithHighlight : public LayoutItem_Button
{
public:
    using tdHighlightFunc = std::function<bool(void)>;
    LayoutItem_ButtonWithHighlight( Rect<uint16_t> rect, const unsigned char* data, size_t size, tdHighlightFunc func = nullptr, tdAction action = nullptr, bool activeOnly = false );

    tdHighlightFunc HighlightFunc;
    virtual void draw( DisplayManager&, const TrackDetails& ) override;
};

class LayoutItem_Rectangle : public LayoutItem
{
    using LayoutItem::LayoutItem;
    virtual void draw( DisplayManager&, const TrackDetails& ) override;
};

class LayoutItem_StaticText : public LayoutItemWithFont
{
public:
    LayoutItem_StaticText( Rect<uint16_t> rect, const GFXfont* font, LayoutItemWithFont::eAlign align, String text, tdAction action = nullptr );

    String  Text;
    virtual void draw( DisplayManager&, const TrackDetails& ) override;
};
