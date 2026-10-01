#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

#include "audio.hpp"
#include "sound_data.h" // generated at build time from assets/sounds/*.wav

#include <chrono>
#include <random>
#include <thread>

namespace audio {

void play_random_embedded_sound() {
    if (g_embedded_sounds_count == 0) return;

    static thread_local std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<std::size_t> pick(0, g_embedded_sounds_count - 1);
    const EmbeddedSound& asset = g_embedded_sounds[pick(rng)];

    ma_engine engine;
    if (ma_engine_init(nullptr, &engine) != MA_SUCCESS) return;

    ma_decoder_config decoder_config = ma_decoder_config_init_default();
    ma_decoder decoder;
    if (ma_decoder_init_memory(asset.data, asset.size, &decoder_config, &decoder) != MA_SUCCESS) {
        ma_engine_uninit(&engine);
        return;
    }

    ma_sound sound;
    if (ma_sound_init_from_data_source(&engine, &decoder, 0, nullptr, &sound) != MA_SUCCESS) {
        ma_decoder_uninit(&decoder);
        ma_engine_uninit(&engine);
        return;
    }

    ma_sound_start(&sound);
    while (!ma_sound_at_end(&sound)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    // Teardown order matters: sound reads from decoder, decoder feeds engine.
    ma_sound_uninit(&sound);
    ma_decoder_uninit(&decoder);
    ma_engine_uninit(&engine);
}

} // namespace audio
