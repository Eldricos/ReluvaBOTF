// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Reluva contributors

#include "engines/botf/data_table.h"

#include "common/text.h"

#include <cstdio>
#include <cstring>
#include <string_view>

namespace reluva::botf {

namespace {

using F = FieldType;
using C = TableLayout::Count;

uint32_t fieldBytes(const Field& f) {
    switch (f.type) {
    case F::Text: return f.size;
    case F::U8: return f.size;
    case F::U16: return 2;
    case F::U32: return 4;
    case F::F32: return 4;
    case F::F64: return 8;
    }
    return 0;
}

uint32_t u32(const uint8_t* p) { return uint32_t(p[0] | p[1] << 8 | p[2] << 16) | uint32_t(p[3]) << 24; }

} // namespace

// Fields are listed only where the evidence is in tables.md; the rest of
// each record shows as bytes.
const std::vector<TableLayout>& tableLayouts() {
    static const std::vector<TableLayout> layouts = {
        {"race.rst", "racedesc.rst", C::U16AtStart, 2, 108,
         {{"name", 0, F::Text, 40},
          {"picture", 40, F::Text, 16},
          {"description", 60, F::U32},
          {"attitude to the 5 major races", 72, F::U8, 5}},
         0, 2},
        {"shiplist.sst", "shipdesc.sst", C::FileSize, 0, 288,
         {{"name", 0, F::Text, 40},
          {"class", 40, F::Text, 20},
          {"description", 84, F::U32},
          {"model", 88, F::Text, 4}},
         0, 2},
        {"techtree.tec", "techdesc.tec", C::FileSize, 0, 72,
         {{"name", 0, F::Text, 30},
          {"number", 40, F::U16},
          {"field's short name", 42, F::Text, 6},
          {"field", 55, F::U8},
          {"level", 56, F::U8},
          {"cost", 60, F::U32},
          {"description", 68, F::U32}},
         0, 6},
        {"tecfield.tec", nullptr, C::U16AtStart, 2, 41, {{"name", 0, F::Text, 40}, {"number", 40, F::U8}}, 0, -1},
        {"edifice.bst", "edifdesc.bst", C::U32AtStart, 740, 176,
         {{"name", 0, F::Text, 40},
          {"plural", 40, F::Text, 48},
          {"picture", 88, F::Text, 8},
          {"description", 172, F::U32}},
         0, 3},
    };
    return layouts;
}

Error DataTable::load(const TableLayout& layout, std::vector<uint8_t> data, std::vector<uint8_t> descriptions) {
    layout_ = &layout;
    data_ = std::move(data);
    descriptions_ = std::move(descriptions);
    count_ = 0;
    if (data_.size() < layout.headerSize)
        return Error::fail("too small for its header");
    const size_t room = (data_.size() - layout.headerSize) / layout.recordSize;
    switch (layout.count) {
    case C::U16AtStart: count_ = data_.size() >= 2 ? size_t(data_[0] | data_[1] << 8) : 0; break;
    case C::U32AtStart: count_ = data_.size() >= 4 ? u32(data_.data()) : 0; break;
    case C::FileSize:
        count_ = room;
        if ((data_.size() - layout.headerSize) % layout.recordSize)
            return Error::fail(std::to_string(data_.size()) + " bytes isn't a whole number of " +
                               std::to_string(layout.recordSize) + "-byte records");
        break;
    }
    if (count_ > room) {
        const size_t claimed = count_;
        count_ = 0;
        return Error::fail(std::to_string(claimed) + " records don't fit in " + std::to_string(data_.size()) +
                           " bytes");
    }
    return Error::success();
}

std::string DataTable::value(size_t r, const Field& f) const {
    const uint8_t* p = record(r) + f.offset;
    char buf[64];
    switch (f.type) {
    case F::Text: {
        size_t n = 0;
        while (n < f.size && p[n])
            ++n;
        return text::windows1252ToUtf8(std::string_view(reinterpret_cast<const char*>(p), n));
    }
    case F::U8: {
        std::string s;
        for (uint32_t i = 0; i < f.size; ++i)
            s += (i ? " " : "") + std::to_string(p[i]);
        return s;
    }
    case F::U16: return std::to_string(p[0] | p[1] << 8);
    case F::U32: return std::to_string(u32(p));
    case F::F32: {
        float v;
        std::memcpy(&v, p, 4);
        std::snprintf(buf, sizeof buf, "%g", double(v));
        return buf;
    }
    case F::F64: {
        double v;
        std::memcpy(&v, p, 8);
        std::snprintf(buf, sizeof buf, "%g", v);
        return buf;
    }
    }
    return {};
}

std::string DataTable::name(size_t r) const { return value(r, layout_->fields[size_t(layout_->nameField)]); }

std::string DataTable::description(size_t r) const {
    if (layout_->descriptionField < 0 || descriptions_.empty())
        return {};
    const uint32_t off = u32(record(r) + layout_->fields[size_t(layout_->descriptionField)].offset);
    if (off >= descriptions_.size())
        return {};
    const char* s = reinterpret_cast<const char*>(descriptions_.data() + off);
    const size_t n = strnlen(s, descriptions_.size() - off);
    return text::windows1252ToUtf8(std::string_view(s, n));
}

std::vector<std::pair<uint32_t, uint32_t>> DataTable::unknownRanges() const {
    std::vector<bool> known(layout_->recordSize, false);
    for (const Field& f : layout_->fields)
        for (uint32_t i = 0; i < fieldBytes(f) && f.offset + i < known.size(); ++i)
            known[f.offset + i] = true;
    std::vector<std::pair<uint32_t, uint32_t>> ranges;
    for (uint32_t i = 0; i < known.size();) {
        if (known[i]) {
            ++i;
            continue;
        }
        uint32_t j = i;
        while (j < known.size() && !known[j])
            ++j;
        ranges.push_back({i, j});
        i = j;
    }
    return ranges;
}

} // namespace reluva::botf
