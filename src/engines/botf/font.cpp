// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Reluva contributors

#include "engines/botf/font.h"

#include "common/text.h"

#include <cstring>
#include <string>

namespace reluva::botf {

namespace {

constexpr size_t kHeaderSize = 260;
constexpr size_t kWidths = 20; // offset of the 120 u16 widths
constexpr int kRowLengths[] = {10, 26, 26, 32, 26};

uint32_t rgbaAt(const image::Image& img, int x, int y) {
    uint32_t v;
    std::memcpy(&v, img.pixel(x, y), 4);
    return v;
}

} // namespace

// The sheet's order, read from the glyphs: digits, capitals, small letters,
// the US keyboard's punctuation (unshifted, then shifted), then accented
// letters and symbols. The last glyph is a short dash, taken as the en dash.
const std::array<char32_t, Font::kGlyphs> Font::kCharacters = {
    U'1', U'2', U'3', U'4', U'5', U'6', U'7', U'8', U'9', U'0',
    U'A', U'B', U'C', U'D', U'E', U'F', U'G', U'H', U'I', U'J', U'K', U'L', U'M',
    U'N', U'O', U'P', U'Q', U'R', U'S', U'T', U'U', U'V', U'W', U'X', U'Y', U'Z',
    U'a', U'b', U'c', U'd', U'e', U'f', U'g', U'h', U'i', U'j', U'k', U'l', U'm',
    U'n', U'o', U'p', U'q', U'r', U's', U't', U'u', U'v', U'w', U'x', U'y', U'z',
    U'`', U'-', U'=', U'\\', U'[', U']', U';', U'\'', U',', U'.', U'/',
    U'~', U'!', U'@', U'#', U'$', U'%', U'^', U'&', U'*', U'(', U')', U'_', U'+',
    U'|', U'{', U'}', U':', U'"', U'<', U'>', U'?',
    U'ä', U'â', U'à', U'ê', U'è', U'é', U'ï', U'î', U'ö', U'ô', U'ü', U'ñ', U'Ç',
    U'ç', U'Ä', U'Ö', U'Ü', U'ß', U'®', U'©', U'™', U'…', U'—', U'¿', U'¡', U'–',
};

Error Font::load(const std::vector<uint8_t>& data) {
    if (data.size() < kHeaderSize + 18)
        return Error::fail("too small for a font");
    lineHeight_ = data[0];
    spaceAdvance_ = data[2];
    const std::vector<uint8_t> tga(data.begin() + kHeaderSize, data.end());
    if (Error err = image::decode(tga, sheet_))
        return Error::fail("glyph sheet: " + err.message);
    if (lineHeight_ < 2 || sheet_.h < lineHeight_ * 5 + 1 || sheet_.w < 3)
        return Error::fail("glyph sheet of " + std::to_string(sheet_.w) + "x" + std::to_string(sheet_.h) +
                           " doesn't fit 5 rows of " + std::to_string(lineHeight_));

    // Cells are framed by lines of one colour (the sheet's top row); their
    // inside's corner is the background.
    const uint32_t frame = rgbaAt(sheet_, 1, 0);
    background_ = rgbaAt(sheet_, 1, 1);
    int glyph = 0;
    for (int row = 0; row < 5; ++row) {
        const int top = row * lineHeight_ + 1, height = lineHeight_ - 1;
        // A column belongs to the frame if it's the frame colour all the way
        // down the row; the cells are the runs between such columns.
        int found = 0, start = -1;
        for (int x = 0; x < sheet_.w; ++x) {
            bool line = true;
            for (int y = top; y < top + height && line; ++y)
                line = rgbaAt(sheet_, x, y) == frame;
            if (!line) {
                if (start < 0)
                    start = x;
                continue;
            }
            if (start >= 0) {
                if (found < kRowLengths[row]) {
                    Glyph& g = glyphs_[size_t(glyph)];
                    g = {start, top, x - start, height, 0};
                    g.advance = data[kWidths + 2 * size_t(glyph)] | data[kWidths + 2 * size_t(glyph) + 1] << 8;
                    ++glyph;
                }
                ++found;
                start = -1;
            }
        }
        if (found != kRowLengths[row])
            return Error::fail("glyph row " + std::to_string(row + 1) + " has " + std::to_string(found) +
                               " framed cells, not " + std::to_string(kRowLengths[row]));
    }
    return Error::success();
}

const Font::Glyph* Font::glyph(char32_t c) const {
    for (size_t i = 0; i < kCharacters.size(); ++i)
        if (kCharacters[i] == c)
            return &glyphs_[i];
    return nullptr;
}

int Font::measure(std::string_view utf8) const {
    int w = 0;
    for (size_t i = 0; i < utf8.size();) {
        const Glyph* g = glyph(text::nextUtf8(utf8, i));
        w += g ? g->advance : spaceAdvance_;
    }
    return w;
}

int Font::draw(Surface& s, int x, int y, std::string_view utf8, std::optional<Color> tint, int scale) const {
    for (size_t i = 0; i < utf8.size();) {
        const Glyph* g = glyph(text::nextUtf8(utf8, i));
        if (!g) {
            x += spaceAdvance_ * scale;
            continue;
        }
        for (int gy = 0; gy < g->h; ++gy)
            for (int gx = 0; gx < g->w; ++gx) {
                if (rgbaAt(sheet_, g->x + gx, g->y + gy) == background_)
                    continue;
                const uint8_t* p = sheet_.pixel(g->x + gx, g->y + gy);
                uint8_t c[3] = {p[0], p[1], p[2]};
                if (tint) {
                    c[0] = uint8_t(c[0] * tint->r / 255);
                    c[1] = uint8_t(c[1] * tint->g / 255);
                    c[2] = uint8_t(c[2] * tint->b / 255);
                }
                for (int sy = 0; sy < scale; ++sy)
                    for (int sx = 0; sx < scale; ++sx) {
                        const int px = x + gx * scale + sx, py = y + gy * scale + sy;
                        if (px < 0 || py < 0 || px >= s.w || py >= s.h)
                            continue;
                        uint8_t* d = s.row(py) + size_t(px) * 4;
                        d[0] = c[0];
                        d[1] = c[1];
                        d[2] = c[2];
                        d[3] = 255;
                    }
            }
        x += g->advance * scale;
    }
    return x;
}

std::vector<std::string> Font::wrap(std::string_view utf8, int width) const {
    std::vector<std::string> lines;
    std::string line;
    size_t start = 0;
    while (start <= utf8.size()) {
        size_t end = utf8.find('\n', start);
        if (end == std::string_view::npos)
            end = utf8.size();
        const std::string_view paragraph = utf8.substr(start, end - start);
        line.clear();
        size_t w = 0;
        while (w < paragraph.size()) {
            size_t space = paragraph.find(' ', w);
            if (space == std::string_view::npos)
                space = paragraph.size();
            const std::string_view word = paragraph.substr(w, space - w);
            const std::string candidate = line.empty() ? std::string(word) : line + " " + std::string(word);
            if (!line.empty() && measure(candidate) > width) {
                lines.push_back(line);
                line = std::string(word);
            } else {
                line = candidate;
            }
            w = space + 1;
        }
        lines.push_back(line);
        start = end + 1;
    }
    return lines;
}

} // namespace reluva::botf
