// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Reluva contributors

#pragma once

#include "common/archive.h"
#include "common/audio/mixer.h"
#include "common/audio/sound.h"
#include "common/system.h"
#include "common/types.h"
#include "engines/botf/snd_archive.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

// Data viewers used by the Birth of the Federation engine until the game
// itself runs. Each draws into the engine's 800x600 RGBA32 screen (the
// game's own resolution) and reports its state in the window title.

namespace reluva::botf {

constexpr int kScreenW = 800;
constexpr int kScreenH = 600;

// Fills a rectangle of an RGBA32 surface, clipped to it.
void fillRect(Surface& s, int x, int y, int w, int h, Color c);

// Browses the TGA images of the given archives (stbof.res, alt.res).
//   Left/Right: previous/next   PageUp/PageDown: jump 10   Home/End: first/last
//   S: the next archive's first image   I: the status line on/off
class ImageViewer {
public:
    ImageViewer(System& sys, Surface& screen, std::vector<const Archive*> archives);
    size_t count() const { return images_.size(); }
    void activate();
    void handleKey(uint32_t key);

private:
    struct ImageRef {
        const Archive* archive;
        std::string name;
    };
    void show(size_t index);

    System& sys_;
    Surface& screen_;
    std::vector<ImageRef> images_;
    size_t current_ = 0;
    bool info_ = true;
};

// Plays the .snd archives' sounds: sfx.snd's effects, the language's speech,
// and music.snd's pieces (each played whole).
//   Up/Down: previous/next   PageUp/PageDown: jump 20   Home/End: first/last
//   Left/Right: previous/next archive   Space or Enter: play or stop
//   A: play each sound as it's selected   R: the ADPCM sounds' sample rate
//   (11,025, 22,050 or 44,100 Hz; it isn't stored, and 22,050 is right)
class SoundViewer {
public:
    // `mixer` may be null (no audio device): sounds are listed, not played.
    SoundViewer(System& sys, Surface& screen, std::vector<std::pair<std::string, const SndArchive*>> archives,
                audio::Mixer* mixer);
    size_t count() const { return items_.size(); }
    void activate();
    void deactivate(); // stops what's playing
    void handleKey(uint32_t key);
    void update(uint64_t nowMs); // redraws while a sound plays

private:
    struct Item {
        size_t archive; // index into archives_
        size_t index;   // entry, or piece for music.snd
    };
    void select(size_t index);
    void play();
    void stop();
    void decodeCurrent();
    void redraw();
    std::string describe(size_t item) const;

    System& sys_;
    Surface& screen_;
    std::vector<std::pair<std::string, const SndArchive*>> archives_;
    audio::Mixer* mixer_;
    std::vector<Item> items_;
    size_t current_ = 0;
    std::shared_ptr<audio::Sound> sound_; // the current item, decoded
    std::string error_;
    audio::SoundHandle playing_ = audio::kNoSound;
    bool autoPlay_ = false;
    int rate_ = kDefaultAdpcmRate;
    uint64_t lastDrawMs_ = 0;
};

} // namespace reluva::botf
