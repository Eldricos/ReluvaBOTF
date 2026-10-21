// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Reluva contributors

#include "engines/botf/viewers.h"

#include "common/font.h"

#include <algorithm>
#include <cstdio>

namespace reluva::botf {

namespace {

constexpr int kTop = 24, kListW = 240, kPanelX = 256;

// Splits text into lines of at most `chars` characters at spaces, for the
// built-in font.
std::vector<std::string> wrapChars(const std::string& text, size_t chars) {
    std::vector<std::string> lines;
    std::string line;
    size_t i = 0;
    while (i < text.size()) {
        size_t space = text.find(' ', i);
        if (space == std::string::npos)
            space = text.size();
        const std::string word = text.substr(i, space - i);
        if (!line.empty() && font::length(line) + 1 + font::length(word) > chars) {
            lines.push_back(line);
            line.clear();
        }
        line += (line.empty() ? "" : " ") + word;
        i = space + 1;
    }
    if (!line.empty())
        lines.push_back(line);
    return lines;
}

} // namespace

DataViewer::DataViewer(System& sys, Surface& screen, const Archive& archive)
    : sys_(sys), screen_(screen), archive_(archive) {
    for (const TableLayout& layout : tableLayouts())
        if (archive.has(layout.file))
            files_.push_back(&layout);
}

void DataViewer::activate() {
    if (table_.count() == 0 && error_.empty() && !files_.empty())
        load();
    show();
}

void DataViewer::load() {
    error_.clear();
    record_ = 0;
    const TableLayout& layout = *files_[current_];
    std::vector<uint8_t> data, descriptions;
    if (Error err = archive_.read(layout.file, data)) {
        error_ = err.message;
        table_ = DataTable{};
        return;
    }
    if (layout.descriptions && archive_.has(layout.descriptions))
        archive_.read(layout.descriptions, descriptions);
    if (Error err = table_.load(layout, std::move(data), std::move(descriptions)))
        error_ = "can't read: " + err.message;
}

void DataViewer::handleKey(uint32_t key) {
    if (files_.empty())
        return;
    const size_t n = table_.count();
    if (key == key::Right || key == key::Left) {
        current_ = key == key::Right ? (current_ + 1) % files_.size()
                                     : (current_ + files_.size() - 1) % files_.size();
        load();
    } else if (n == 0)
        return;
    else if (key == key::Down)
        record_ = (record_ + 1) % n;
    else if (key == key::Up)
        record_ = (record_ + n - 1) % n;
    else if (key == key::PageDown)
        record_ = std::min(record_ + 20, n - 1);
    else if (key == key::PageUp)
        record_ = record_ >= 20 ? record_ - 20 : 0;
    else if (key == key::Home)
        record_ = 0;
    else if (key == key::End)
        record_ = n - 1;
    else
        return;
    show();
}

void DataViewer::show() {
    fillRect(screen_, 0, 0, screen_.w, screen_.h, {16, 22, 30});
    if (files_.empty())
        return;
    const Color text{230, 230, 230}, dim{140, 140, 160}, hilite{255, 220, 120}, label{150, 200, 255};
    const TableLayout& layout = *files_[current_];
    std::string head = std::to_string(current_ + 1) + "/" + std::to_string(files_.size()) + "  " + layout.file;
    head += error_.empty() ? "  " + std::to_string(table_.count()) + " records of " +
                                 std::to_string(layout.recordSize) + " bytes"
                           : "  " + error_;
    font::drawText(screen_, 8, 6, head, text);
    const int bottom = screen_.h - 18;
    font::drawText(screen_, 8, bottom + 4,
                   "Left/Right: table  Up/Down, PageUp/PageDown: record  Tab: next page", dim);
    const size_t n = table_.count();
    if (n == 0)
        return;

    // The list of records around the current one.
    const int rows = (bottom - kTop) / font::kLineHeight;
    size_t first = record_ > size_t(rows / 2) ? record_ - size_t(rows / 2) : 0;
    first = std::min(first, n > size_t(rows) ? n - size_t(rows) : 0);
    for (int r = 0; r < rows && first + size_t(r) < n; ++r) {
        const size_t i = first + size_t(r);
        const int y = kTop + r * font::kLineHeight;
        if (i == record_)
            fillRect(screen_, 4, y - 1, kListW, font::kLineHeight, {50, 60, 90});
        char num[24];
        std::snprintf(num, sizeof num, "%3zu ", i);
        font::drawText(screen_, 8, y, num + table_.name(i), i == record_ ? hilite : text);
    }

    // The current record: its fields, its unexplained bytes, its description.
    int y = kTop;
    const size_t chars = size_t(screen_.w - kPanelX - 8) / font::kAdvance;
    for (const Field& f : layout.fields) {
        char off[16];
        std::snprintf(off, sizeof off, "%3u ", f.offset);
        int x = font::drawText(screen_, kPanelX, y, off, dim);
        x = font::drawText(screen_, x, y, std::string(f.name) + ": ", label);
        font::drawText(screen_, x, y, table_.value(record_, f), text);
        y += font::kLineHeight;
    }
    y += 4;
    font::drawText(screen_, kPanelX, y, "Bytes not worked out yet:", label);
    y += font::kLineHeight;
    const uint8_t* rec = table_.record(record_);
    for (const auto& [begin, end] : table_.unknownRanges())
        for (uint32_t o = begin; o < end && y < bottom - 60; o += 16) {
            char line[96];
            int p = std::snprintf(line, sizeof line, "%3u ", o);
            for (uint32_t k = o; k < std::min(end, o + 16); ++k)
                p += std::snprintf(line + p, sizeof line - size_t(p), "%02x ", rec[k]);
            font::drawText(screen_, kPanelX, y, line, text);
            y += font::kLineHeight;
        }
    const std::string desc = table_.description(record_);
    if (!desc.empty()) {
        y += 4;
        for (const std::string& line : wrapChars(desc, chars)) {
            if (y > bottom - font::kLineHeight)
                break;
            font::drawText(screen_, kPanelX, y, line, {200, 220, 200});
            y += font::kLineHeight;
        }
    }
    sys_.setWindowTitle("Reluva - Birth of the Federation - data " + std::string(layout.file) + " " +
                        std::to_string(record_ + 1) + "/" + std::to_string(n) + ": " + table_.name(record_));
}

} // namespace reluva::botf
