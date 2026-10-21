// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Reluva contributors

#include "engines/botf/viewers.h"

#include "common/font.h"
#include "common/fs.h"

#include <algorithm>

namespace reluva::botf {

namespace {

constexpr const char* kSamples[] = {
    "The quick brown fox jumps over the lazy dog.",
    "THE QUICK BROWN FOX JUMPS OVER THE LAZY DOG! 1234567890",
    "`-=\\[];',./ ~!@#$%^&*()_+|{}:\"<>?",
    "Ça, élève! Où est la Fédération? Größe ¿Qué? ¡Sí! ® © ™ … — –",
};

} // namespace

FontViewer::FontViewer(System& sys, Surface& screen, const Archive& archive)
    : sys_(sys), screen_(screen), archive_(archive) {
    for (std::string& n : archive.list())
        if (fs::toLower(n).ends_with(".fnt"))
            names_.push_back(std::move(n));
    std::sort(names_.begin(), names_.end(),
              [](const std::string& x, const std::string& y) { return fs::toLower(x) < fs::toLower(y); });
}

void FontViewer::activate() { show(); }

void FontViewer::handleKey(uint32_t key) {
    if (names_.empty())
        return;
    if (key == key::Right)
        current_ = (current_ + 1) % names_.size();
    else if (key == key::Left)
        current_ = (current_ + names_.size() - 1) % names_.size();
    else if (key == '+' || key == '=')
        scale_ = std::min(scale_ + 1, 4);
    else if (key == '-')
        scale_ = std::max(scale_ - 1, 1);
    else
        return;
    show();
}

void FontViewer::show() {
    fillRect(screen_, 0, 0, screen_.w, screen_.h, {24, 24, 32});
    if (names_.empty())
        return;
    const std::string& name = names_[current_];
    std::string status;
    std::vector<uint8_t> data;
    Font font;
    if (Error err = archive_.read(name, data))
        status = err.message;
    else if (Error err2 = font.load(data))
        status = "can't read: " + err2.message;
    else {
        // The glyph sheet as stored, frames and all, at twice its size if
        // it fits; then the samples.
        const image::Image& sheet = font.sheet();
        const int zoom = sheet.w * 2 <= screen_.w - 16 && sheet.h * 2 <= 300 ? 2 : 1;
        for (int y = 0; y < sheet.h; ++y)
            for (int x = 0; x < sheet.w; ++x) {
                const uint8_t* p = sheet.pixel(x, y);
                fillRect(screen_, 8 + x * zoom, 24 + y * zoom, zoom, zoom, {p[0], p[1], p[2]});
            }
        int y = 24 + sheet.h * zoom + 12;
        for (const char* sample : kSamples) {
            font.draw(screen_, 8, y, sample, std::nullopt, scale_);
            y += font.lineHeight() * scale_ + 4;
        }
        status = "line height " + std::to_string(font.lineHeight()) + ", space " +
                 std::to_string(font.spaceAdvance()) + ", sheet " + std::to_string(sheet.w) + "x" +
                 std::to_string(sheet.h) + ", samples at " + std::to_string(scale_) + "x";
    }
    font::drawText(screen_, 8, 6, std::to_string(current_ + 1) + "/" + std::to_string(names_.size()) + "  " +
                                      name + "  " + status, {230, 230, 230});
    font::drawText(screen_, 8, screen_.h - 16, "Left/Right: font  +/-: sample size  Tab: next page",
                   {140, 140, 160});
    sys_.setWindowTitle("Reluva - Birth of the Federation - fonts " + std::to_string(current_ + 1) + "/" +
                        std::to_string(names_.size()) + ": " + name);
}

} // namespace reluva::botf
