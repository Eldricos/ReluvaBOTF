// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Reluva contributors

#pragma once

#include "common/archive.h"
#include "common/audio/mixer.h"
#include "common/audio/sound.h"
#include "common/image.h"
#include "common/system.h"
#include "common/types.h"
#include "engines/botf/data_table.h"
#include "engines/botf/font.h"
#include "engines/botf/snd_archive.h"
#include "engines/botf/text_table.h"
#include "engines/botf/window_layout.h"

#include <cstddef>
#include <cstdint>
#include <map>
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

// The interface pictures mark what isn't drawn with magenta (red and blue
// full, no green), the colour key MPR's blits skip: makes those pixels
// transparent, and returns how many there were. The key is set by the
// executable, not stored in the files; magenta is what the pictures show.
size_t applyColorKey(image::Image& img);

// Browses the TGA images of the given archives (stbof.res, alt.res).
//   Left/Right: previous/next   PageUp/PageDown: jump 10   Home/End: first/last
//   S: the next archive's first image   I: the status line on/off
//   K: the colour key (magenta shown as transparent) on/off
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
    bool info_ = true, key_ = false;
};

// Shows the .fnt fonts of an archive (stbof.res): the glyph sheet, and
// sample text drawn with the font.
//   Left/Right: previous/next font   +/-: the sample text's size
class FontViewer {
public:
    FontViewer(System& sys, Surface& screen, const Archive& archive);
    size_t count() const { return names_.size(); }
    void activate();
    void handleKey(uint32_t key);

private:
    void show();

    System& sys_;
    Surface& screen_;
    const Archive& archive_;
    std::vector<std::string> names_;
    size_t current_ = 0;
    int scale_ = 1;
};

// Shows the .tdb text tables of an archive (stbof.res), each entry's key
// and its text drawn in one of the game's fonts.
//   Left/Right: previous/next table   Up/Down: scroll   PageUp/PageDown:
//   scroll a screen   Home/End: first/last entry   F: the next font
class TextViewer {
public:
    TextViewer(System& sys, Surface& screen, const Archive& archive);
    size_t count() const { return tables_.size(); }
    void activate();
    void handleKey(uint32_t key);

private:
    void load();
    void show();

    System& sys_;
    Surface& screen_;
    const Archive& archive_;
    std::vector<std::string> tables_, fonts_;
    size_t current_ = 0, font_ = 0, first_ = 0;
    TextTable table_;
    Font fontData_;
    std::string error_;
};

// Shows the data tables worked out so far (tableLayouts()): the records by
// name, and the selected one's fields, the bytes no field covers yet, and
// its description.
//   Left/Right: previous/next table   Up/Down: previous/next record
//   PageUp/PageDown: jump 20   Home/End: first/last record
class DataViewer {
public:
    DataViewer(System& sys, Surface& screen, const Archive& archive);
    size_t count() const { return files_.size(); }
    void activate();
    void handleKey(uint32_t key);

private:
    void load();
    void show();

    System& sys_;
    Surface& screen_;
    const Archive& archive_;
    std::vector<const TableLayout*> files_; // the layouts whose file is in the archive
    size_t current_ = 0, record_ = 0;
    DataTable table_;
    std::string error_;
};

// Shows the .wdf window layouts of an archive (stbof.res): each widget's
// pictures where the layout puts them, outlined by type, and the selected
// widget's details.
//   Left/Right: previous/next layout   Up/Down: previous/next widget
//   PageUp/PageDown: jump 10 layouts   O: outlines on/off
//   S: buttons' selected pictures   I: the details on/off
class LayoutViewer {
public:
    LayoutViewer(System& sys, Surface& screen, const Archive& archive);
    size_t count() const { return names_.size(); }
    void activate();
    void handleKey(uint32_t key);

private:
    void load();
    void show();
    const image::Image* picture(const std::string& name); // cached; null if missing

    System& sys_;
    Surface& screen_;
    const Archive& archive_;
    std::vector<std::string> names_;
    size_t current_ = 0, widget_ = 0;
    WindowLayout layout_;
    std::vector<WindowLayout::Placed> placed_;
    std::string error_;
    std::map<std::string, image::Image> pictures_;
    std::map<std::string, std::string> missing_; // name: why
    bool outlines_ = true, selected_ = false, info_ = true;
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
