// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Reluva contributors

#include "engines/botf/viewers.h"

#include "common/font.h"

#include <vector>

namespace reluva::botf {

SetupViewer::SetupViewer(System& sys, Surface& screen, const Archive& archive)
    : sys_(sys), screen_(screen), assets_(archive), setup_(assets_, lexicon_) {
    // Without lexicon.dic the screen shows its English words.
    std::vector<uint8_t> data;
    if (!archive.read("lexicon.dic", data))
        lexicon_.load(data);
    ready_ = archive.has("gmesetup.wdf") && archive.has("opening.wdf") && !setup_.load();
}

void SetupViewer::activate() { show(); }

void SetupViewer::handleEvent(const Event& event) {
    if (!ready_)
        return;
    const SetupScreen::Result r = setup_.handle(event);
    if (r != SetupScreen::Result::Open) {
        const GameSetup& s = setup_.setup();
        std::string levels;
        for (int c : s.civilization)
            levels += std::to_string(c + 1);
        status_ = std::string(r == SetupScreen::Result::Accepted ? "accepted" : "cancelled") + ": levels " +
                  levels + ", minors " + std::to_string(s.minorRaces) + ", difficulty " +
                  std::to_string(s.difficulty) + ", events " + (s.randomEvents ? "on" : "off") + ", combat " +
                  (s.manualCombat ? "manual" : "automatic") + ", victory " + std::to_string(s.victory) +
                  ", shape " + std::to_string(s.galaxyShape) + ", size " + std::to_string(s.galaxySize);
        setup_.setup() = GameSetup{};
    }
    if (r != SetupScreen::Result::Open || event.type == EventType::MouseDown || event.type == EventType::MouseUp)
        show();
}

void SetupViewer::show() {
    fillRect(screen_, 0, 0, screen_.w, screen_.h, {0, 0, 0});
    if (!ready_)
        return;
    setup_.draw(screen_);
    sys_.setWindowTitle("Reluva - Birth of the Federation - game setup" +
                        (status_.empty() ? std::string(" (click the controls, Accept or Return, Tab: next page)")
                                         : " - " + status_));
}

} // namespace reluva::botf
