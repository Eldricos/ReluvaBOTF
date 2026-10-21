// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Reluva contributors

#include "engines/botf/viewers.h"

#include "common/font.h"
#include "common/fs.h"

#include <algorithm>
#include <cstdio>

namespace reluva::botf {

namespace {

constexpr int kTop = 22, kBottom = 20, kKeyWidth = 70;

std::vector<std::string> sortedNames(const Archive& archive, const char* ext) {
    std::vector<std::string> names;
    for (std::string& n : archive.list())
        if (fs::toLower(n).ends_with(ext))
            names.push_back(std::move(n));
    std::sort(names.begin(), names.end(),
              [](const std::string& x, const std::string& y) { return fs::toLower(x) < fs::toLower(y); });
    return names;
}

} // namespace

TextViewer::TextViewer(System& sys, Surface& screen, const Archive& archive)
    : sys_(sys), screen_(screen), archive_(archive), tables_(sortedNames(archive, ".tdb")),
      fonts_(sortedNames(archive, ".fnt")) {
    // The Federation's 15-point font to start with: readable, with every glyph.
    for (size_t i = 0; i < fonts_.size(); ++i)
        if (fs::toLower(fonts_[i]) == "human15.fnt")
            font_ = i;
}

void TextViewer::activate() {
    load();
    show();
}

void TextViewer::load() {
    error_.clear();
    table_ = TextTable{};
    first_ = 0;
    std::vector<uint8_t> data;
    if (Error err = archive_.read(tables_[current_], data))
        error_ = err.message;
    else if (Error err2 = table_.load(data))
        error_ = "can't read: " + err2.message;
    fontData_ = Font{};
    if (!fonts_.empty()) {
        if (Error err = archive_.read(fonts_[font_], data); err || fontData_.load(data))
            fontData_ = Font{};
    }
}

void TextViewer::handleKey(uint32_t key) {
    if (tables_.empty())
        return;
    const size_t n = table_.entries().size();
    if (key == key::Right || key == key::Left) {
        current_ = key == key::Right ? (current_ + 1) % tables_.size()
                                     : (current_ + tables_.size() - 1) % tables_.size();
        load();
    } else if (key == 'f' && !fonts_.empty()) {
        font_ = (font_ + 1) % fonts_.size();
        const size_t keep = first_;
        load();
        first_ = keep;
    } else if (key == key::Down)
        first_ = std::min(first_ + 1, n ? n - 1 : 0);
    else if (key == key::Up)
        first_ = first_ ? first_ - 1 : 0;
    else if (key == key::PageDown)
        first_ = std::min(first_ + 10, n ? n - 1 : 0);
    else if (key == key::PageUp)
        first_ = first_ >= 10 ? first_ - 10 : 0;
    else if (key == key::Home)
        first_ = 0;
    else if (key == key::End)
        first_ = n ? n - 1 : 0;
    else
        return;
    show();
}

void TextViewer::show() {
    fillRect(screen_, 0, 0, screen_.w, screen_.h, {16, 20, 32});
    if (tables_.empty())
        return;
    const Color dim{140, 140, 160}, text{230, 230, 230};
    const auto& entries = table_.entries();
    std::string head = std::to_string(current_ + 1) + "/" + std::to_string(tables_.size()) + "  " +
                       tables_[current_] + "  ";
    head += error_.empty() ? std::to_string(entries.size()) + " entries, longest " +
                                 std::to_string(table_.longest()) + " bytes"
                           : error_;
    if (!fonts_.empty())
        head += "  font " + fonts_[font_];
    font::drawText(screen_, 8, 6, head, text);

    // Each entry: its key on the left, its text wrapped in the game font
    // (or the built-in one if the font didn't load).
    const bool gameFont = fontData_.lineHeight() > 0;
    const int lineHeight = gameFont ? fontData_.lineHeight() : font::kLineHeight;
    const int textX = 8 + kKeyWidth, textW = screen_.w - textX - 8;
    int y = kTop;
    for (size_t i = first_; i < entries.size() && y < screen_.h - kBottom; ++i) {
        char key[16];
        std::snprintf(key, sizeof key, "%08x", entries[i].key);
        font::drawText(screen_, 8, y + 2, key, dim);
        const std::string shown = displayText(entries[i].text);
        const std::vector<std::string> lines =
            gameFont ? fontData_.wrap(shown, textW) : std::vector<std::string>{shown};
        for (const std::string& line : lines) {
            if (y + lineHeight > screen_.h - kBottom)
                break;
            if (gameFont)
                fontData_.draw(screen_, textX, y, line);
            else
                font::drawText(screen_, textX, y, line, text);
            y += lineHeight;
        }
        y += 4;
    }
    font::drawText(screen_, 8, screen_.h - 14,
                   "Left/Right: table  Up/Down, PageUp/PageDown: scroll  F: font  Tab: next page", dim);
    sys_.setWindowTitle("Reluva - Birth of the Federation - text " + std::to_string(current_ + 1) + "/" +
                        std::to_string(tables_.size()) + ": " + tables_[current_] + ", entry " +
                        std::to_string(entries.empty() ? 0 : first_ + 1) + "/" + std::to_string(entries.size()));
}

} // namespace reluva::botf
