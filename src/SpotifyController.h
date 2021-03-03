#pragma onca

#include "TrackDetails.h"

class SpotifyController
{
public:
    static TrackDetails CurrentTrack;
    static bool IsPlaying;

    static void PlayOrPauseCurrentTrack();
    static void PlayCurrentTrack();
    static void PauseCurrentTrack();
    static void RestartCurrentTrack();
    static void PlayNextTrack();
    static void PlayPreviousTrack();
};
