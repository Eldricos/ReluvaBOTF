// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Reluva contributors

#include "engines/botf/viewers.h"

#include <vector>

namespace reluva::botf {

namespace {

const char* const kEmpires[] = {"Cardassians", "Federation", "Ferengi", "Klingons", "Romulans"};

std::string describe(const GameSetup& s) {
    std::string levels;
    for (int c : s.civilization)
        levels += std::to_string(c + 1);
    return "levels " + levels + ", minors " + std::to_string(s.minorRaces) + ", difficulty " +
           std::to_string(s.difficulty) + ", events " + (s.randomEvents ? "on" : "off") + ", combat " +
           (s.manualCombat ? "manual" : "automatic") + ", victory " + std::to_string(s.victory) + ", shape " +
           std::to_string(s.galaxyShape) + ", size " + std::to_string(s.galaxySize);
}

} // namespace

SetupViewer::SetupViewer(System& sys, Surface& screen, const Archive& archive)
    : sys_(sys), screen_(screen), assets_(archive), setup_(assets_, lexicon_), empires_(assets_, lexicon_) {
    // Without lexicon.dic the screens show English words, and no
    // descriptions.
    std::vector<uint8_t> data;
    if (!archive.read("lexicon.dic", data))
        lexicon_.load(data);
    ready_ = archive.has("gmesetup.wdf") && archive.has("emppick.wdf") && archive.has("opening.wdf") &&
             !setup_.load() && !empires_.load();
}

void SetupViewer::activate() { show(); }

void SetupViewer::handleEvent(const Event& event) {
    if (!ready_)
        return;
    if (!choosingEmpire_) {
        const SetupScreen::Result r = setup_.handle(event);
        if (r == SetupScreen::Result::Accepted) {
            choosingEmpire_ = true;
            empires_.reset();
            empires_.setLevels(setup_.setup().civilization);
            status_.clear();
        } else if (r == SetupScreen::Result::Cancelled) {
            status_ = "cancelled";
            setup_.setup() = GameSetup{};
        }
    } else {
        const EmpireScreen::Result r = empires_.handle(event);
        if (r == EmpireScreen::Result::Accepted) {
            status_ = std::string("playing the ") + kEmpires[empires_.chosen()] + "; " + describe(setup_.setup());
            choosingEmpire_ = false;
            setup_.setup() = GameSetup{};
        } else if (r == EmpireScreen::Result::Cancelled) {
            choosingEmpire_ = false;
        }
    }
    if (event.type != EventType::KeyUp)
        show();
}

void SetupViewer::show() {
    if (!ready_) {
        fillRect(screen_, 0, 0, screen_.w, screen_.h, {0, 0, 0});
        return;
    }
    fillRect(screen_, 0, 0, screen_.w, screen_.h, {0, 0, 0});
    if (choosingEmpire_)
        empires_.draw(screen_);
    else
        setup_.draw(screen_);
    sys_.setWindowTitle(std::string("Reluva - Birth of the Federation - ") +
                        (choosingEmpire_ ? "empire selection" : "game setup") +
                        (status_.empty() ? " (click the controls, Accept or Return, Tab: next page)" : " - " + status_));
}

} // namespace reluva::botf
