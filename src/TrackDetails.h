#pragma once

#include <HTTPClient.h>

struct TrackDetails
{
    bool        IsPlaying = false;
    String      ID;
    String      Name;
    String      AlbumName;
    String      ArtistsName;
    std::pair<Size<uint16_t>,String>      ArtURL[3];
    uint32_t    ProgressMS = 0;
    uint32_t    DurationMS = 0;

    static TrackDetails PopulateFromCurrentlyPlaying( HTTP_response_t response );
};
