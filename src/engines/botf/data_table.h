// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Reluva contributors

#pragma once

#include "common/types.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace reluva::botf {

// The game's data tables in stbof.res (docs/formats/botf/tables.md): fixed-size
// records, some with a count or a header in front, many with a description
// in a companion file of zero-terminated strings. Each table is described by
// a TableLayout; fields not worked out yet aren't listed, and show as bytes.

enum class FieldType {
    Text, // zero-terminated within `size` bytes (what follows the zero is leftover)
    U8,   // `size` of them
    U16,
    U32,
    F32,
    F64,
};

struct Field {
    const char* name;
    uint32_t offset;
    FieldType type;
    uint32_t size = 1; // Text: bytes; U8: how many
};

struct TableLayout {
    enum class Count { U16AtStart, U32AtStart, FileSize };

    const char* file;         // "race.rst"
    const char* descriptions; // the companion file of descriptions, or nullptr
    Count count;
    uint32_t headerSize;      // bytes before the first record
    uint32_t recordSize;
    std::vector<Field> fields;
    int nameField = 0;        // index into fields
    int descriptionField = -1; // a U32 offset into the descriptions, or -1
};

// Every table worked out so far.
const std::vector<TableLayout>& tableLayouts();

class DataTable {
public:
    // `descriptions` may be empty when the table has none or it's missing.
    Error load(const TableLayout& layout, std::vector<uint8_t> data, std::vector<uint8_t> descriptions = {});

    const TableLayout& layout() const { return *layout_; }
    size_t count() const { return count_; }
    const uint8_t* record(size_t i) const { return data_.data() + layout_->headerSize + i * layout_->recordSize; }
    // A field's value as text: strings as they are (Windows-1252 decoded),
    // numbers in decimal, U8 arrays space-separated.
    std::string value(size_t record, const Field& field) const;
    std::string name(size_t record) const;
    std::string description(size_t record) const; // empty if none
    // Which of a record's bytes no field covers, as [begin, end) ranges.
    std::vector<std::pair<uint32_t, uint32_t>> unknownRanges() const;

private:
    const TableLayout* layout_ = nullptr;
    std::vector<uint8_t> data_, descriptions_;
    size_t count_ = 0;
};

} // namespace reluva::botf
