#pragma once

namespace audio {

// Picks one of the embedded sounds at random and plays it, blocking until
// playback finishes. No-op if no sound was embedded at build time.
void play_random_embedded_sound();

} // namespace audio
