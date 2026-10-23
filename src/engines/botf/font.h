// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Reluva contributors

#pragma once

#include "common/image.h"
#include "common/types.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace reluva::botf {

// A .fnt font from stbof.res (docs/formats/botf/fnt.md): a 260-byte header
// (the line height, 120 advance widths) and a 16-bit TGA sheet holding the
// 120 glyphs in framed cells, in rows of 10, 26, 26, 32 and 26.
class Font {
public:
    static constexpr int kGlyphs = 120;
    // The character of each glyph, in sheet order.
    static const std::array<char32_t, kGlyphs> kCharacters;

    struct Glyph {
        int x = 0, y = 0, w = 0, h = 0; // the cell's inside, in the sheet
        int advance = 0;                // from the header
    };

    Error load(const std::vector<uint8_t>& data);

    int lineHeight() const { return lineHeight_; } // the sheet's row pitch
    int glyphHeight() const { return lineHeight_ - 1; }
    int spaceAdvance() const { return spaceAdvance_; }
    const image::Image& sheet() const { return sheet_; }
    const std::array<Glyph, kGlyphs>& glyphs() const { return glyphs_; }
    // The glyph for a character, or nullptr (space has none: it advances
    // spaceAdvance()).
    const Glyph* glyph(char32_t c) const;

    // The width of UTF-8 text in pixels; characters without a glyph count as
    // spaces.
    int measure(std::string_view utf8) const;
    // Draws UTF-8 text with its top-left corner at (x, y) into an RGBA32
    // surface, clipped to it: each glyph's pixels as they are, its
    // background left out; with `tint`, multiplied by it. Returns the x after
    // the text.
    int draw(Surface& surface, int x, int y, std::string_view utf8, std::optional<Color> tint = std::nullopt,
             int scale = 1) const;
    // Draws the text in a colour, as the game's screens do: the glyphs are
    // shades of grey, and the brightest shade in the sheet comes out as
    // `color`, the others in proportion.
    int drawInColor(Surface& surface, int x, int y, std::string_view utf8, Color color) const;
    // Splits UTF-8 text into lines at most `width` pixels wide, breaking at
    // spaces and newlines; a word wider than a line gets a line of its own.
    std::vector<std::string> wrap(std::string_view utf8, int width) const;

private:
    image::Image sheet_;
    std::array<Glyph, kGlyphs> glyphs_{};
    int lineHeight_ = 0;
    int spaceAdvance_ = 0;
    uint32_t background_ = 0; // the sheet's background colour, RGBA as stored
    int brightest_ = 255;     // the brightest channel of any glyph pixel

    int drawImpl(Surface& surface, int x, int y, std::string_view utf8, std::optional<Color> color, bool shaded,
                 int scale) const;
};

} // namespace reluva::botf
