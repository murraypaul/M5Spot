#include "main.h"
#include "TrackDetails.h"

#include <HTTPClient.h>
#include <ArduinoJson.h>

TrackDetails TrackDetails::PopulateFromCurrentlyPlaying( HTTP_response_t response )
{
    TrackDetails track;

    StaticJsonDocument<256> filter;
    filter["is_playing"] = true;
    filter["progress_ms"] = true;
    filter["item"]["duration_ms"] = true;
    filter["item"]["id"] = true;
    filter["item"]["name"] = true;
    filter["item"]["artists"][0]["name"] = true;
    filter["item"]["album"]["name"] = true;
    filter["item"]["album"]["images"][0]["url"] = true;
    filter["item"]["album"]["images"][0]["height"] = true;
    filter["item"]["album"]["images"][0]["width"] = true;
    DynamicJsonDocument json(5120);
    DeserializationError error = deserializeJson(json,response.payload, DeserializationOption::Filter(filter));
    if (!error) 
    {
        track.IsPlaying = json["is_playing"];
        track.ProgressMS = json["progress_ms"];
        track.DurationMS = json["item"]["duration_ms"];

        // Get song ID
        track.ID = json["item"]["id"].as<String>();
        track.Name = json["item"]["name"].as<String>();

        track.ArtURL[0] = {{json["item"]["album"]["images"][0]["width"],json["item"]["album"]["images"][0]["height"]},json["item"]["album"]["images"][0]["url"].as<String>()};
        track.ArtURL[1] = {{json["item"]["album"]["images"][1]["width"],json["item"]["album"]["images"][1]["height"]},json["item"]["album"]["images"][1]["url"].as<String>()};
        track.ArtURL[2] = {{json["item"]["album"]["images"][2]["width"],json["item"]["album"]["images"][2]["height"]},json["item"]["album"]["images"][2]["url"].as<String>()};
        track.AlbumName = json["item"]["album"]["name"].as<String>();

        JsonArray arr = json["item"]["artists"];
        track.ArtistsName.reserve(150);
        bool first = true;
        for (auto a : arr) {
            if (first) {
                track.ArtistsName += a["name"].as<String>();
                first = false;
            } else {
                track.ArtistsName += ", ";
                track.ArtistsName += a["name"].as<String>();
            }
        }
    } else {
        log_i("Unable to parse response payload:\n  %s\n", response.payload.c_str());
        eventsSendError(500, "Unable to parse response payload", response.payload.c_str());
    }

    return track;
}
