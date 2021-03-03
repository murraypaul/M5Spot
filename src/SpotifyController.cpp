#include "main.h"
#include "SpotifyController.h"

TrackDetails SpotifyController::CurrentTrack;
bool SpotifyController::IsPlaying = false;

void SpotifyController::PlayOrPauseCurrentTrack()
{
    log_d("PauseOrPlayCurrentTrack");
    sptfToggle();
}

void SpotifyController::PlayCurrentTrack()
{
    log_d("PlayCurrentTrack");
}

void SpotifyController::PauseCurrentTrack()
{
    log_d("PauseCurrentTrack");
}

void SpotifyController::RestartCurrentTrack()
{
    log_d("RestartCurrentTrack");
}

void SpotifyController::PlayNextTrack()
{
    log_d("PlayNextTrack");
    sptfNext();
}

void SpotifyController::PlayPreviousTrack()
{
    log_d("PlayPreviousTrack");
    sptfPrevious();
}
