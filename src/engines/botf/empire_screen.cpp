// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Reluva contributors

#include "engines/botf/empire_screen.h"

#include "engines/botf/setup_screen.h"

#include <string>

namespace reluva::botf {

namespace {

// emppick.wdf's widgets, per empire in race.rst's order (Cardassians,
// Federation, Ferengi, Klingons, Romulans), matched by their pictures (the
// race letter: c, h, f, k, r) and their places on the screen.
constexpr int32_t kAccept = 9110, kCancel = 9111;
constexpr std::array<int32_t, 5> kNames = {9105, 9106, 9107, 9108, 9109};   // buttons, kind 2
constexpr std::array<int32_t, 5> kEmblems = {9100, 9101, 9102, 9103, 9104}; // pictures
constexpr std::array<int32_t, 5> kLevels = {9114, 9115, 9116, 9117, 9118};  // text boxes
constexpr int32_t kDescription = 9112;
// Not shown in a single-player game: a small button with a number beside
// each empire (multiplayer, presumably), and a 250x350 picture under the
// description.
constexpr std::array<int32_t, 11> kHidden = {9124, 9125, 9126, 9127, 9128, 9119,
                                             9120, 9121, 9122, 9123, 9113};
const char* const kEmblemPictures[] = {"es_cicn0.tga", "es_hicn0.tga", "es_ficn0.tga", "es_kicn0.tga",
                                       "es_ricn0.tga"};

// The descriptions: lexicon.dic's entries 270-274, found by their text
// (the recording shows the Federation's), in the lexicon's order:
// Cardassians, Ferengi, Federation, Klingons, Romulans.
constexpr std::array<int, 5> kDescriptions = {270, 272, 271, 273, 274};

// Colours measured from the recording, as on the setup screen.
constexpr Color kLevelColor{120, 180, 150};
constexpr Color kDescriptionColor{133, 115, 75};
constexpr int kDescriptionLineGap = 2; // 23 pixels a line, human20's height 21

} // namespace

Error EmpireScreen::load() {
    if (Error err = loadLayouts("emppick.wdf", {kAccept, kCancel, kNames[0], kNames[1], kNames[2], kNames[3],
                                                kNames[4], kDescription}))
        return err;
    hidden_ = std::set<int32_t>(kHidden.begin(), kHidden.end());
    return Error::success();
}

int EmpireScreen::empireAt(int x, int y) const {
    for (int i = 0; i < 5; ++i)
        for (int32_t id : {kNames[size_t(i)], kEmblems[size_t(i)]})
            if (const Placed* p = find(id); p && p->contains(x, y))
                return i;
    return -1;
}

EmpireScreen::Result EmpireScreen::handle(const Event& e) {
    if (e.type == EventType::KeyDown && (e.key == key::Return || e.key == key::KpEnter))
        return chosen_ >= 0 ? Result::Accepted : Result::Open;
    if (e.type == EventType::KeyDown && e.key == key::Escape)
        return Result::Cancelled;
    if (e.type == EventType::MouseMove || e.type == EventType::MouseDown || e.type == EventType::MouseUp)
        pointed_ = empireAt(e.mouseX, e.mouseY);
    if (e.button != MouseButton::Left)
        return Result::Open;
    if (e.type == EventType::MouseDown) {
        pressed_ = buttonAt(e.mouseX, e.mouseY);
        // Choosing an empire happens on the press, like a radio button; its
        // emblem chooses it too.
        if (const int empire = empireAt(e.mouseX, e.mouseY); empire >= 0)
            chosen_ = empire;
    } else if (e.type == EventType::MouseUp) {
        const int32_t id = pressed_ && buttonAt(e.mouseX, e.mouseY) == pressed_ ? pressed_ : 0;
        pressed_ = 0;
        if (id == kAccept && chosen_ >= 0)
            return Result::Accepted;
        if (id == kCancel)
            return Result::Cancelled;
    }
    return Result::Open;
}

void EmpireScreen::draw(Surface& screen) {
    std::set<int32_t> selected;
    if (chosen_ >= 0)
        selected.insert(kNames[size_t(chosen_)]);
    drawLayouts(screen, selected);
    for (size_t i = 0; i < 5; ++i) {
        drawPicture(screen, kEmblems[i], kEmblemPictures[i]);
        drawText(screen, kLevels[i], civilizationLevel(lexicon_, levels_[i]), kLevelColor);
    }
    // The empire pointed at, or else the one chosen.
    const int shown = pointed_ >= 0 ? pointed_ : chosen_;
    if (shown >= 0)
        drawText(screen, kDescription, lexicon_[size_t(kDescriptions[size_t(shown)])], kDescriptionColor,
                 kDescriptionLineGap);
}

} // namespace reluva::botf
