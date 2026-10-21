// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Reluva contributors

// Tests for the Birth of the Federation readers against files built here,
// following docs/formats/botf/, so no game data is needed.

#include "common/audio/wav.h"
#include "engines/botf/alt_res.h"
#include "engines/botf/data_table.h"
#include "engines/botf/font.h"
#include "engines/botf/game_files.h"
#include "engines/botf/snd_archive.h"
#include "engines/botf/text_table.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

using namespace reluva;
using namespace reluva::botf;

static int failures = 0;

#define CHECK(cond)                                                     \
    do {                                                                \
        if (!(cond)) {                                                  \
            std::fprintf(stderr, "%s:%d: CHECK failed: %s\n", __FILE__, \
                         __LINE__, #cond);                              \
            ++failures;                                                 \
        }                                                               \
    } while (0)

namespace {

using Bytes = std::vector<uint8_t>;

void put16(Bytes& b, uint32_t v) {
    b.push_back(uint8_t(v));
    b.push_back(uint8_t(v >> 8));
}
void put32(Bytes& b, uint32_t v) {
    put16(b, v & 0xffff);
    put16(b, v >> 16);
}
void set32(Bytes& b, size_t at, uint32_t v) {
    for (int i = 0; i < 4; ++i)
        b[at + size_t(i)] = uint8_t(v >> (8 * i));
}
void writeFile(const std::filesystem::path& p, const Bytes& b) {
    std::filesystem::create_directories(p.parent_path());
    std::ofstream out(p, std::ios::binary);
    out.write(reinterpret_cast<const char*>(b.data()), std::streamsize(b.size()));
}

// A small LZSS encoder in alt.res's format, writing the window from
// position 1 as its tool does: runs of a byte copied from 1 byte back,
// repeats of the last 2 bytes (a pixel) from 2 back, and literals for the
// rest. A decoder starting its window anywhere else reads the wrong bytes.
Bytes compress(const Bytes& raw) {
    std::vector<Bytes> items;
    size_t r = 1;
    for (size_t i = 0; i < raw.size();) {
        size_t len = 0, back = 0;
        for (size_t d : {size_t(1), size_t(2)}) {
            size_t n = 0;
            if (i >= d)
                while (i + n < raw.size() && n < 17 && raw[i + n] == raw[i - d + n % d])
                    ++n;
            if (n > len)
                len = n, back = d;
        }
        if (len >= 2) {
            const size_t from = (r + 4096 - back) % 4096;
            items.push_back({uint8_t((len - 2) << 4 | from >> 8), uint8_t(from & 0xff)});
            i += len;
            r = (r + len) % 4096;
        } else {
            items.push_back({raw[i]});
            ++i;
            r = (r + 1) % 4096;
        }
    }
    Bytes out;
    for (size_t k = 0; k < items.size(); k += 8) {
        uint8_t flags = 0;
        for (size_t j = 0; j < 8 && k + j < items.size(); ++j)
            if (items[k + j].size() == 1)
                flags |= uint8_t(1 << j);
        out.push_back(flags);
        for (size_t j = 0; j < 8 && k + j < items.size(); ++j)
            out.insert(out.end(), items[k + j].begin(), items[k + j].end());
    }
    return out;
}

// A small 16-bit TGA: runs of colours that change every few pixels, and
// some black, so copies read from the wrong place give the wrong bytes.
Bytes makeTga(int w, int h, uint8_t seed) {
    Bytes t = {0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    put16(t, uint32_t(w));
    put16(t, uint32_t(h));
    t.push_back(16);
    t.push_back(1);
    for (int i = 0; i < w * h; ++i) {
        const int run = i / (3 + (i / 50) % 9);
        put16(t, run % 5 == 0 ? 0 : uint32_t((run * 2654435761u + seed) >> 17) & 0x7fff);
    }
    return t;
}

// A .fnt as docs/formats/botf/fnt.md describes: the header (line height,
// space advance, 120 advances) and a 16-bit TGA sheet of framed cells, in
// rows of 10, 26, 26, 32 and 26. Glyph i is a cell of width 2 + i % 4 filled
// with colour 0x0400 + i, except one pixel left as background; the first
// cell of rows 2-5 is a pixel wider than its advance says, as in the game.
Bytes makeFont(int lineHeight, int rowsToDraw = 5) {
    const int rows[5] = {10, 26, 26, 32, 26};
    std::vector<uint16_t> advances;
    int sheetW = 0;
    for (int r = 0, i = 0; r < 5; ++r) {
        int x = 1;
        for (int k = 0; k < rows[r]; ++k, ++i) {
            const int cell = 2 + i % 4 + (r > 0 && k == 0 ? 1 : 0);
            advances.push_back(uint16_t(2 + i % 4 + 1));
            x += cell + 1;
        }
        sheetW = std::max(sheetW, x);
    }
    const int sheetH = 5 * lineHeight + 1;
    std::vector<uint16_t> px(size_t(sheetW) * sheetH, 0); // top row first
    auto at = [&](int x, int y) -> uint16_t& { return px[size_t(y) * sheetW + x]; };
    const uint16_t frame = 0x03f1;
    for (int r = 0, i = 0; r < 5; ++r) {
        for (int x = 0; x < sheetW; ++x)
            at(x, r * lineHeight) = at(x, (r + 1) * lineHeight) = frame;
        int x = 1;
        for (int k = 0; k < rows[r]; ++k, ++i) {
            const int cell = 2 + i % 4 + (r > 0 && k == 0 ? 1 : 0);
            for (int y = r * lineHeight; y <= (r + 1) * lineHeight; ++y)
                at(x - 1, y) = at(x + cell, y) = frame;
            if (r < rowsToDraw)
                for (int y = r * lineHeight + 1; y < (r + 1) * lineHeight; ++y)
                    for (int cx = 0; cx < cell; ++cx)
                        if (!(cx == 0 && y == r * lineHeight + 1))
                            at(x + cx, y) = uint16_t(0x0400 + i);
            x += cell + 1;
        }
    }
    if (rowsToDraw < 5) // a full-height frame-coloured column splits a cell
        for (int y = 4 * lineHeight + 1; y < 5 * lineHeight; ++y)
            at(2, y) = frame;
    Bytes f(260, 0);
    f[0] = uint8_t(lineHeight);
    f[1] = 1;
    f[2] = 4;
    f[3] = 1;
    set32(f, 4, 10 | 120u << 16);
    set32(f, 8, uint32_t(sheetW) | uint32_t(sheetH) << 16);
    set32(f, 12, uint32_t(px.size() * 2));
    f[16] = 1;
    f[17] = 6;
    f[18] = 86;
    for (int i = 0; i < 120; ++i) {
        f[20 + 2 * size_t(i)] = uint8_t(advances[size_t(i)]);
        f[21 + 2 * size_t(i)] = uint8_t(advances[size_t(i)] >> 8);
    }
    Bytes tga = {0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    put16(tga, uint32_t(sheetW));
    put16(tga, uint32_t(sheetH));
    tga.push_back(16);
    tga.push_back(1); // rows from the bottom up
    for (int y = sheetH - 1; y >= 0; --y)
        for (int x = 0; x < sheetW; ++x)
            put16(tga, at(x, y));
    f.insert(f.end(), tga.begin(), tga.end());
    return f;
}

void testFont() {
    Font font;
    CHECK(!font.load(makeFont(8)));
    CHECK(font.lineHeight() == 8 && font.glyphHeight() == 7 && font.spaceAdvance() == 4);
    // 'A' is glyph 10, the first of row 2: a pixel wider than its advance - 1.
    const Font::Glyph* a = font.glyph(U'A');
    CHECK(a && a->x == 1 && a->y == 9 && a->w == 2 + 10 % 4 + 1 && a->h == 7 && a->advance == 2 + 10 % 4 + 1);
    const Font::Glyph* one = font.glyph(U'1');
    CHECK(one && one->x == 1 && one->y == 1 && one->w == 2 && one->advance == 3);
    const Font::Glyph* two = font.glyph(U'2');
    CHECK(two && two->x == 4 && two->w == 3);
    CHECK(font.glyph(U'é') == &font.glyphs()[5 + 62 + 32]);
    CHECK(font.glyph(U'–') == &font.glyphs()[119]);
    CHECK(font.glyph(U' ') == nullptr && font.glyph(U'€') == nullptr);
    CHECK(font.measure("1 2") == 3 + 4 + 4);

    // Drawing: the glyph's pixels, not its background pixel.
    Surface s;
    s.create(20, 10, PixelFormat::RGBA32);
    CHECK(font.draw(s, 2, 1, "1") == 5);
    auto lit = [&](int x, int y) { return s.row(y)[size_t(x) * 4 + 3] != 0; };
    CHECK(!lit(2, 1) && lit(3, 1) && lit(2, 2) && lit(3, 7) && !lit(4, 2) && !lit(2, 8));
    s.create(20, 10, PixelFormat::RGBA32);
    font.draw(s, 0, 0, "1", Color{0, 0, 0});
    CHECK(s.row(1)[3] == 255 && s.row(1)[0] == 0 && s.row(1)[1] == 0); // tinted black, still drawn

    // A row whose cells can't be told apart fails.
    CHECK(font.load(makeFont(8, 4)));
    CHECK(font.load(Bytes(100, 0)));
}

// A .tdb as docs/formats/botf/tdb.md describes: count, a leftover value,
// the longest length, 0; per string key, length, a leftover value, offset;
// the strings, stored in another order than the records.
Bytes makeTable(const std::vector<std::pair<uint32_t, std::string>>& strings) {
    Bytes text;
    std::vector<uint32_t> offsets(strings.size());
    for (size_t i = strings.size(); i-- > 0;) { // last string first
        offsets[i] = uint32_t(text.size());
        text.insert(text.end(), strings[i].second.begin(), strings[i].second.end());
        text.push_back(0);
    }
    Bytes f;
    uint32_t longest = 0;
    for (const auto& s : strings)
        longest = std::max(longest, uint32_t(s.second.size() + 1));
    put32(f, uint32_t(strings.size()));
    put32(f, 0x005a0628);
    put32(f, longest);
    put32(f, 0);
    for (size_t i = 0; i < strings.size(); ++i) {
        put32(f, strings[i].first);
        put32(f, uint32_t(strings[i].second.size() + 1));
        put32(f, 0x005a0a02 + uint32_t(i));
        put32(f, offsets[i]);
    }
    f.insert(f.end(), text.begin(), text.end());
    return f;
}

void testTextTable() {
    TextTable table;
    const Bytes data = makeTable({{0x00410008, " Greetings from %1.\n"},
                                  {0x00481307, " %s1 and %2 have formed an alliance.\n"},
                                  {0x00410018, " Gr\xf6\xdf" "e \x85\n"}});
    CHECK(!table.load(data));
    CHECK(table.entries().size() == 3 && table.longest() == 38);
    CHECK(table.entries()[0].key == 0x00410008 && table.entries()[0].text == " Greetings from %1.\n");
    CHECK(table.find(0x00481307) == &table.entries()[1]);
    CHECK(table.find(0x00410018)->text == " Gr\xc3\xb6\xc3\x9f" "e \xe2\x80\xa6\n"); // Windows-1252
    CHECK(!table.find(1));
    CHECK(displayText(table.entries()[0].text) == "Greetings from %1.");
    CHECK(displayText("") == "" && displayText(" \n") == "");

    // Damaged tables fail: a count too large, a string past the end, a
    // string without its zero byte.
    Bytes bad = data;
    set32(bad, 0, 1000);
    CHECK(table.load(bad));
    bad = data;
    set32(bad, 16 + 12, 5000);
    CHECK(table.load(bad));
    bad = data;
    bad.pop_back();
    CHECK(table.load(bad));
    CHECK(table.load(Bytes(8, 0)));
    CHECK(!table.load(makeTable({})) && table.entries().empty());
}

void testWrap() {
    Font font;
    CHECK(!font.load(makeFont(8)));
    // Every glyph advances 3 to 6 pixels, a space 4: '1' 3, '2' 4, '3' 5.
    CHECK(font.measure("12") == 7);
    CHECK(font.wrap("12 12 12", 18) == std::vector<std::string>({"12 12", "12"}));
    CHECK(font.wrap("12 12 12", 100) == std::vector<std::string>({"12 12 12"}));
    CHECK(font.wrap("123123 1", 6) == std::vector<std::string>({"123123", "1"})); // too wide: on its own
    CHECK(font.wrap("1\n\n2", 100) == std::vector<std::string>({"1", "", "2"}));
    CHECK(font.wrap("", 100) == std::vector<std::string>({""}));
}

// A data table laid out as TableLayout describes: here a u16 count, then
// 12-byte records (a name, a U8 pair, a U32 description offset), and a
// companion file of descriptions.
void testDataTable() {
    using F = FieldType;
    const TableLayout layout = {"test.tbl", "testdesc.tbl", TableLayout::Count::U16AtStart, 2, 12,
                                {{"name", 0, F::Text, 6}, {"pair", 6, F::U8, 2}, {"description", 8, F::U32}}, 0, 2};
    Bytes data = {2, 0};
    auto record = [&](const char* name, uint8_t a, uint8_t b, uint32_t desc) {
        Bytes r(6, 0xcc); // leftover bytes after the name's zero
        std::memcpy(r.data(), name, std::strlen(name) + 1);
        r.push_back(a);
        r.push_back(b);
        put32(r, desc);
        data.insert(data.end(), r.begin(), r.end());
    };
    record("Farm", 1, 2, 6);
    record("Gr\xf6\xdf", 30, 40, 0);
    const std::string descs("Large\0Small\0", 12);
    DataTable table;
    CHECK(!table.load(layout, data, Bytes(descs.begin(), descs.end())));
    CHECK(table.count() == 2);
    CHECK(table.name(0) == "Farm" && table.name(1) == "Gr\xc3\xb6\xc3\x9f");
    CHECK(table.value(0, layout.fields[1]) == "1 2" && table.value(1, layout.fields[2]) == "0");
    CHECK(table.description(0) == "Small" && table.description(1) == "Large");
    CHECK(table.unknownRanges().empty());

    // Without descriptions, and with fields that leave gaps.
    const TableLayout gaps = {"gaps.tbl", nullptr, TableLayout::Count::FileSize, 0, 8,
                              {{"value", 2, F::U16}, {"real", 4, F::F32}, {"pair", 2, F::U16, 3}}, 0, -1};
    Bytes two(16, 0);
    two[2] = 0x34;
    two[3] = 0x12;
    const float half = 0.5f;
    std::memcpy(&two[4], &half, 4);
    CHECK(!table.load(gaps, two));
    CHECK(table.count() == 2 && table.value(0, gaps.fields[0]) == "4660" && table.value(0, gaps.fields[1]) == "0.5");
    CHECK(table.value(0, gaps.fields[2]) == "4660 0 16128"); // a U16 array: 0x1234, then the float's bytes
    CHECK(table.description(0).empty());
    CHECK(table.unknownRanges() == (std::vector<std::pair<uint32_t, uint32_t>>{{0, 2}}));

    // Counts that don't fit, and sizes that aren't whole records, fail.
    data[0] = 3;
    CHECK(table.load(layout, data));
    CHECK(table.count() == 0);
    CHECK(table.load(gaps, Bytes(12, 0)));

    // Every built-in layout's fields fit inside its records.
    for (const TableLayout& l : tableLayouts())
        for (const Field& f : l.fields)
            CHECK(f.offset + (f.type == F::Text || f.type == F::U8 ? f.size : f.type == F::F64 ? 8u
                                                                    : f.type == F::U16    ? 2u * f.size : 4u) <=
                  l.recordSize);
}

void testLzss() {
    // The start of the retail alt.res's first file: a flag byte, literals
    // and two references that fill the TGA header's zeros; the decoded
    // header (800x600, 16-bit) is how the format was worked out.
    const Bytes start = {0xe7, 0x00, 0x00, 0x02, 0x00, 0x01, 0x50, 0x05, 0x20, 0x03,
                         0x58, 0x07, 0x02, 0x10, 0x01, 0x70, 0x04};
    Bytes out;
    size_t used = 0;
    CHECK(!decompressLzss(start.data(), start.size(), 18, out, &used));
    const Bytes header = {0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x20, 0x03, 0x58, 0x02, 0x10, 0x01};
    CHECK(out == header);
    CHECK(used == 15);

    // Round trip through the encoder above.
    const Bytes raw = makeTga(40, 30, 3);
    const Bytes packed = compress(raw);
    CHECK(packed.size() < raw.size());
    CHECK(!decompressLzss(packed.data(), packed.size(), raw.size(), out, &used));
    CHECK(out == raw);
    CHECK(used == packed.size());

    // Too little input fails.
    CHECK(decompressLzss(packed.data(), packed.size() / 2, raw.size(), out));
}

void testAltRes(const std::filesystem::path& tmp) {
    // Two files, stored last first as in the retail alt.res.
    const Bytes a = makeTga(16, 8, 1), b = makeTga(20, 10, 2);
    const Bytes pa = compress(a), pb = compress(b);
    Bytes file(24, 0);
    std::memcpy(file.data(), "alt.res", 7);
    const uint32_t offB = 24, offA = uint32_t(24 + pb.size());
    file.insert(file.end(), pb.begin(), pb.end());
    file.insert(file.end(), pa.begin(), pa.end());
    set32(file, 16, uint32_t(file.size()));
    set32(file, 20, 2);
    auto entry = [&](const char* name, uint32_t off, uint32_t index, const Bytes& raw, const Bytes& packed) {
        Bytes e(16, 0);
        std::memcpy(e.data(), name, std::strlen(name));
        put32(e, off);
        put32(e, index);
        put32(e, uint32_t(raw.size()));
        put32(e, uint32_t(packed.size()));
        put32(e, 1);
        put32(e, 0x1234);
        file.insert(file.end(), e.begin(), e.end());
    };
    entry("a.tga", offA, 0, a, pa);
    entry("b.tga", offB, 1, b, pb);
    file.resize(file.size() + 100, 0); // padding, as at the retail file's end
    writeFile(tmp / "alt.res", file);

    AltResArchive alt;
    CHECK(!alt.open(tmp / "alt.res"));
    CHECK(alt.entries().size() == 2);
    CHECK(alt.list() == std::vector<std::string>({"a.tga", "b.tga"}));
    CHECK(alt.has("A.TGA") && !alt.has("c.tga"));
    CHECK(alt.entries()[1].offset == offB && alt.entries()[1].check == 0x1234);
    Bytes out;
    CHECK(!alt.read("a.tga", out) && out == a);
    CHECK(!alt.read("B.tga", out) && out == b);
    CHECK(alt.read("c.tga", out));

    // A directory that doesn't fit is refused.
    set32(file, 20, 1000);
    writeFile(tmp / "bad.res", file);
    CHECK(alt.open(tmp / "bad.res"));
}

// One mono IMA ADPCM block of `size` bytes.
Bytes imaBlock(size_t size, int16_t first, uint8_t seed) {
    Bytes b;
    put16(b, uint16_t(first));
    b.push_back(10);
    b.push_back(0);
    for (size_t i = 4; i < size; ++i)
        b.push_back(uint8_t(seed + i * 37));
    return b;
}

Bytes wavFile(uint16_t format, int channels, int rate, int bits, int blockAlign, const Bytes& data) {
    Bytes w = {'R', 'I', 'F', 'F'};
    put32(w, uint32_t(36 + data.size()));
    w.insert(w.end(), {'W', 'A', 'V', 'E', 'f', 'm', 't', ' '});
    put32(w, 16);
    put16(w, format);
    put16(w, uint32_t(channels));
    put32(w, uint32_t(rate));
    put32(w, uint32_t(rate * channels * 2));
    put16(w, uint32_t(blockAlign));
    put16(w, uint32_t(bits));
    w.insert(w.end(), {'d', 'a', 't', 'a'});
    put32(w, uint32_t(data.size()));
    w.insert(w.end(), data.begin(), data.end());
    return w;
}

void testImaWithoutHeader() {
    // The same blocks decode alike with and without a WAV file round them.
    Bytes blocks = imaBlock(512, 100, 1);
    const Bytes second = imaBlock(512, -300, 2);
    blocks.insert(blocks.end(), second.begin(), second.end());
    audio::Sound bare, wrapped;
    CHECK(!audio::decodeImaAdpcm(blocks.data(), blocks.size(), 1, 22050, 512, 0, bare));
    CHECK(!audio::decodeWav(wavFile(0x11, 1, 22050, 4, 512, blocks), wrapped));
    CHECK(bare.samples == wrapped.samples);
    CHECK(bare.frames() == 2 * 1017 && bare.rate == 22050 && bare.channels == 1 && bare.format == 0x11);
    CHECK(bare.samples[0] == 100 && bare.samples[1017] == -300);
    CHECK(!audio::decodeImaAdpcm(blocks.data(), blocks.size(), 1, 11025, 512, 1500, bare));
    CHECK(bare.frames() == 1500 && bare.rate == 11025);
    CHECK(audio::decodeImaAdpcm(blocks.data(), blocks.size(), 3, 22050, 512, 0, bare));
}

void testSnd(const std::filesystem::path& tmp) {
    // sfx.snd: WAV files, then the directory.
    Bytes pcm;
    for (int i = 0; i < 100; ++i)
        put16(pcm, uint32_t(i * 50));
    const Bytes wav = wavFile(1, 1, 22050, 16, 2, pcm);
    Bytes sfx(4, 0);
    sfx.insert(sfx.end(), wav.begin(), wav.end());
    sfx.insert(sfx.end(), wav.begin(), wav.end());
    set32(sfx, 0, uint32_t(sfx.size()));
    for (uint32_t i = 0; i < 2; ++i) {
        put32(sfx, uint32_t(4 + i * wav.size()));
        put32(sfx, uint32_t(wav.size()));
        put32(sfx, 0x0040ca9a);
        put32(sfx, i * 2);
    }
    writeFile(tmp / "sfx.snd", sfx);
    SndArchive snd;
    CHECK(!snd.open(tmp / "sfx.snd", false));
    CHECK(snd.entries().size() == 2 && snd.entries()[1].flags == 2 && !snd.isMusic());
    audio::Sound s;
    CHECK(!snd.decode(1, 11025, s));
    CHECK(s.rate == 22050 && s.frames() == 100 && s.format == 1 && s.samples[3] == 150);
    CHECK(snd.decode(2, 22050, s));

    // english.snd: bare mono ADPCM, its length from the decoded size.
    const Bytes clip = imaBlock(512, 7, 3);
    Bytes speech(4, 0);
    speech.insert(speech.end(), clip.begin(), clip.end());
    set32(speech, 0, uint32_t(speech.size()));
    put32(speech, 4);
    put32(speech, uint32_t(clip.size()));
    put32(speech, 44 + 2 * 1000);
    put32(speech, 0);
    writeFile(tmp / "english.snd", speech);
    CHECK(!snd.open(tmp / "english.snd", false));
    CHECK(!snd.decode(0, 22050, s));
    CHECK(s.format == 0x11 && s.channels == 1 && s.rate == 22050 && s.frames() == 1000 && s.samples[0] == 7);

    // music.snd: 16 lists after the segments; two pieces of stereo blocks.
    Bytes music(128, 0);
    Bytes stereo = imaBlock(1024, 5, 4);
    stereo[4] = 9; // the right channel's header: sample 9, index 0
    stereo[5] = 0;
    stereo[6] = 0;
    stereo[7] = 0;
    const uint32_t segA = uint32_t(music.size());
    music.insert(music.end(), stereo.begin(), stereo.end());
    const uint32_t segB = uint32_t(music.size());
    music.insert(music.end(), stereo.begin(), stereo.end());
    const uint32_t lists = uint32_t(music.size());
    auto segment = [&](uint32_t off, uint32_t frames) {
        put32(music, off);
        put32(music, uint32_t(stereo.size()));
        put32(music, 44 + 4 * frames);
        put32(music, 3);
    };
    segment(segA, 1017); // piece 0: two segments
    segment(segB, 500);
    segment(segB, 1017); // piece 1: one
    set32(music, 0, 2);
    set32(music, 4, lists);
    set32(music, 8, 1);
    set32(music, 12, lists + 32);
    for (int p = 2; p < 16; ++p)
        set32(music, size_t(p) * 8 + 4, uint32_t(music.size())); // empty lists
    writeFile(tmp / "music.snd", music);
    CHECK(!snd.open(tmp / "music.snd", true));
    CHECK(snd.isMusic() && snd.entries().size() == 3 && snd.pieces().size() == 16);
    CHECK(snd.pieces()[0] == std::vector<size_t>({0, 1}) && snd.pieces()[1] == std::vector<size_t>({2}));
    CHECK(snd.pieces()[2].empty() && snd.entries()[2].piece == 1);
    CHECK(!snd.decodePiece(0, 22050, s));
    CHECK(s.channels == 2 && s.frames() == 1517 && s.samples[0] == 5 && s.samples[1] == 9);
    CHECK(s.samples[1017 * 2] == 5); // the second segment starts afresh
    CHECK(snd.decodePiece(16, 22050, s));
}

void testGameFiles(const std::filesystem::path& tmp) {
    const auto ini = parseStbofIni("CDPATH=D:\\\r\nHDPATH=C:\\botf\\\r\nlanguage = German \r\nINSTALL=MINIMUM\r\n"
                                   "DIRECT X NOT 6.0 INSTALLED BY BOTF\r\n");
    CHECK(ini.at("CDPATH") == "D:\\" && ini.at("LANGUAGE") == "German" && ini.at("INSTALL") == "MINIMUM");
    CHECK(ini.size() == 4);

    // A game folder with the CD's language folder copied in, and a CD
    // folder named by Stbof.ini.
    const auto game = tmp / "game", cd = tmp / "cd";
    writeFile(game / "STBOF.RES", {1});
    writeFile(game / "German" / "german.snd", {2});
    writeFile(cd / "MUSIC.SND", {3});
    const std::string text = "CDPATH=" + cd.generic_string() + "\r\nLANGUAGE=GERMAN\r\n";
    writeFile(game / "Stbof.ini", Bytes(text.begin(), text.end()));
    GameFiles files(game);
    CHECK(files.language() == "GERMAN" && files.speechFile() == "german.snd");
    CHECK(files.cdDir().has_value());
    CHECK(files.find("stbof.res") == game / "STBOF.RES");
    CHECK(files.find("german.snd") == game / "German" / "german.snd");
    CHECK(files.find("music.snd") == cd / "MUSIC.SND");
    CHECK(!files.find("alt.res"));

    // Without Stbof.ini: English, no CD.
    GameFiles plain(tmp / "cd");
    CHECK(plain.language() == "ENGLISH" && plain.speechFile() == "english.snd" && !plain.cdDir());
    CHECK(plain.find("music.snd").has_value());
}

} // namespace

int main() {
    const auto tmp = std::filesystem::temp_directory_path() / "reluva_botf_tests";
    std::error_code ec;
    std::filesystem::remove_all(tmp, ec);
    testLzss();
    testFont();
    testWrap();
    testTextTable();
    testDataTable();
    testAltRes(tmp / "alt");
    testImaWithoutHeader();
    testSnd(tmp / "snd");
    testGameFiles(tmp / "files");
    std::filesystem::remove_all(tmp, ec);
    if (failures)
        std::fprintf(stderr, "%d check(s) failed\n", failures);
    else
        std::printf("All tests passed\n");
    return failures ? 1 : 0;
}
