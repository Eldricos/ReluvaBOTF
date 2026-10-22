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
    case F::U16:
    case F::S16: return 2 * f.size;
    case F::U32:
    case F::S32: return 4 * f.size;
    case F::F32: return 4;
    case F::F64: return 8;
    }
    return 0;
}

uint32_t u32(const uint8_t* p) { return uint32_t(p[0] | p[1] << 8 | p[2] << 16) | uint32_t(p[3]) << 24; }

// Tables that come several to a layout: the starting buildings of each era
// (bldset1-5.bin) and each major race's colonisation weights (?colval.wtf).
std::vector<TableLayout> withRepeats(std::vector<TableLayout> layouts) {
    static const char* const bldsets[] = {"bldset1.bin", "bldset2.bin", "bldset3.bin", "bldset4.bin", "bldset5.bin"};
    for (const char* file : bldsets)
        layouts.push_back({file, nullptr, C::FileSize, 0, 8,
                           {{"race", 0, F::S16}, {"building (edifice.bst's number)", 2, F::S16}, {"how many", 4, F::S32}},
                           0, -1});
    static const char* const colvals[] = {"ccolval.wtf", "hcolval.wtf", "fcolval.wtf", "kcolval.wtf", "rcolval.wtf"};
    static const char* const columns[] = {"Worthless",        "Dilithium",      "Food",     "Energy",     "Poor_Industry",
                                          "Average_Industry", "Prime_Industry", "Distance", "Trade",      "Near_Minor",
                                          "Gal_Density",      "Military",       "Rnd"};
    for (const char* file : colvals) {
        TableLayout l{file, nullptr, C::FileSize, 474, 104, {}, -1, -1, {}, 441, 33};
        for (uint32_t i = 0; i < 13; ++i)
            l.fields.push_back({columns[i], 8 * i, F::F64});
        layouts.push_back(std::move(l));
    }
    return layouts;
}

} // namespace

// Fields are listed only where the evidence is in tables.md (the numbers:
// matched against the game's own screens in a recording); the rest of each
// record shows as bytes.
const std::vector<TableLayout>& tableLayouts() {
    static const std::vector<TableLayout> layouts = withRepeats({
        {"race.rst", "racedesc.rst", C::U16AtStart, 2, 108,
         {{"name", 0, F::Text, 40},
          {"picture", 40, F::Text, 16},
          {"description", 60, F::U32},
          {"home planet (planet.pst's number)", 64, F::S16},
          {"starting population", 66, F::S16},
          {"growth rate", 68, F::F32},
          {"starting morale under each major race (5), then ?", 72, F::U8, 6},
          {"dilithium at home (0, 1)", 78, F::S16},
          {"unrest threshold", 84, F::S32},
          {"ground combat multiplier", 104, F::F32}},
         0, 2},
        {"shiplist.sst", "shipdesc.sst", C::FileSize, 0, 288,
         {{"name", 0, F::Text, 40},
          {"class", 40, F::Text, 44},
          {"description", 84, F::U32},
          {"model", 88, F::Text, 14},
          {"number", 102, F::U16},
          {"role (0 scout, 1 destroyer, 2 cruiser, 3 strike cruiser, 4 command ship, 5 colony ship, 6 outpost, "
           "7 starbase, 8 monster, 9 troop transport)",
           104, F::U16},
          {"name group (shipname.bin)", 106, F::U16},
          {"beam accuracy", 108, F::U32},
          {"beam weapons", 112, F::U32},
          {"beam damage multiplier", 116, F::U32},
          {"beam damage, each", 120, F::U32},
          {"beam shield penetration", 124, F::U32},
          {"torpedo accuracy", 128, F::U32},
          {"torpedoes", 132, F::U32},
          {"torpedo damage multiplier", 136, F::U32},
          {"torpedo damage, each", 140, F::U32},
          {"shield level", 144, F::U32},
          {"shields", 148, F::U32},
          {"shield recharge", 152, F::U32},
          {"offensive strength, for the AI", 156, F::U32},
          {"defensive strength, for the AI", 160, F::U32},
          {"hit points", 168, F::U32},
          {"defense", 172, F::U32},
          {"stealth", 176, F::U32},
          {"extra beam weapons", 180, F::U32},
          {"extra beam accuracy", 184, F::U32},
          {"extra beam damage multiplier", 188, F::U32},
          {"extra beam damage, each", 192, F::U32},
          {"extra beam shield penetration", 196, F::U32},
          {"agility", 212, F::F64},
          {"firing arc, degrees", 220, F::F64},
          {"beam range", 228, F::F64},
          {"torpedo minimum range", 236, F::F64},
          {"torpedo range", 244, F::F64},
          {"extra beam range", 252, F::F64},
          {"range (0 short, 1 medium, 2 long)", 260, F::F64},
          {"speed", 268, F::F64},
          {"scan range", 276, F::U16},
          {"production (colony ships: terraforming; transports: ground combat)", 278, F::U16},
          {"race (race.rst)", 280, F::U16},
          {"build cost", 284, F::U16},
          {"maintenance cost", 286, F::U16}},
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
          {"plural", 40, F::Text, 40},
          {"number", 80, F::U16},
          {"upgrade of (32767: none)", 82, F::U16},
          {"picture", 88, F::Text, 8},
          {"research field needed (tecfield.tec)", 101, F::U8},
          {"technology level needed", 102, F::U8},
          {"races that can't build it, a bit each", 103, F::U8},
          {"resident races needed, a bit each (low, high)", 104, F::U32, 2},
          {"system status needed (0 home system, 1 native member, 2 non-native member, 3 subjugated, ...)", 112, F::U8},
          {"planet or star needed in the system", 113, F::U8},
          {"output type (its wording in edifbnft.bst)", 114, F::U8},
          {"build limit", 115, F::U8},
          {"group", 116, F::U8},
          {"category (0 research, 1 industry, 2 food, 3 energy, 4 defense)", 117, F::U8},
          {"build cost", 120, F::U16},
          {"energy maintenance", 124, F::U16},
          {"population maintenance", 128, F::U16},
          {"morale", 130, F::S16},
          {"output, per race (5 major races, then ?)", 134, F::S16, 6},
          {"unrest bonus, per race", 146, F::S16, 6},
          {"description", 172, F::U32}},
         0, 21},
        {"planet.pst", nullptr, C::U16AtStart, 260, 76,
         {{"name", 0, F::Text, 40},
          {"animation", 40, F::Text, 14},
          {"number", 54, F::S16},
          {"size (0 small, 1 medium, 2 large)", 56, F::S16},
          {"terraforming points", 58, F::U16},
          {"terraforming done", 60, F::U16},
          {"food bonus (0, 310, 620: levels 0-2)", 62, F::S16},
          {"energy bonus (0, 310, 620)", 64, F::S16},
          {"growth bonus (0, 310, 620)", 66, F::S16},
          {"type (environ.est)", 72, F::U8},
          {"atmosphere (environ.est)", 73, F::U8}},
         0, -1},
        {"environ.est", nullptr, C::U16AtStart, 130, 50, {{"name", 0, F::Text, 40}, {"number", 40, F::U16}}, 0, -1},
        {"starname.bin", nullptr, C::U32AtStart, 4, 24, {{"name", 0, F::Text, 20}, {"name's length + 1", 20, F::U32}},
         0, -1},
        {"shipname.bin", nullptr, C::FileSize, 252, 44, {{"group", 0, F::U16}, {"name", 4, F::Text, 40}}, 1, -1},
        {"shiptech.sst", nullptr, C::FileSize, 0, 16,
         {{"race", 0, F::U16},
          {"ship (shiplist.sst)", 4, F::U16},
          {"upgrades from (65535: none)", 6, F::U16},
          {"technology levels needed", 8, F::U8, 6}},
         1, -1},
        {"shiprace.sst", nullptr, C::FileSize, 0, 4, {{"first ship (4294967295: none)", 0, F::U32}}, 0, -1},
        {"planspac.bin", nullptr, C::FileSize, 0, 40,
         {{"planet type (environ.est)", 0, F::U8},
          {"size", 1, F::U8},
          {"maximum population, by growth bonus level (3) x energy bonus level (3)", 4, F::S32, 9}},
         -1, -1},
        {"planboni.bin", nullptr, C::FileSize, 0, 40,
         {{"planet type (environ.est)", 0, F::U8},
          {"size", 1, F::U8},
          {"bonus in 5% steps, by food bonus level (3) x energy bonus level (3)", 4, F::S32, 9}},
         -1, -1},
        {"racetech.tec", nullptr, C::FileSize, 1300, 108,
         {{"race", 0, F::S16},
          {"biotech level", 4, F::S16},
          {"biotech research done", 8, F::S32},
          {"computer level", 12, F::S16},
          {"computer research done", 16, F::S32},
          {"construction level", 20, F::S16},
          {"construction research done", 24, F::S32},
          {"energy level", 28, F::S16},
          {"energy research done", 32, F::S32},
          {"propulsion level", 36, F::S16},
          {"propulsion research done", 40, F::S32},
          {"weapons level", 44, F::S16},
          {"weapons research done", 48, F::S32},
          {"sociology level", 52, F::S16},
          {"sociology research done", 56, F::S32},
          {"buildings available at the start, a bit each by number", 60, F::U32, 12}},
         0, -1},
        {"morale.bin", nullptr, C::FileSize, 0, 20, {{"change, for each major race", 0, F::S32, 5}}, -1, -1,
         {"Eliminate an empire", "Win a major battle", "Win a significant battle", "Win a minor battle",
          "Lose a major battle", "Lose a significant battle", "Lose a minor battle", "Sign a membership",
          "Subjugate a system", "Colonize a system", "Liberate a minor race", "Liberate natives",
          "Lose the home system", "Lose a member system", "Lose a subjugated system", "Lose a system to rebellion",
          "Bombard a system", "Declare war", "War declared on you", "Reject peace", "Break a treaty", "Sign peace",
          "Sign an alliance"}},
        {"intel.bin", nullptr, C::FileSize, 0, 96,
         {{"espionage base", 0, F::S32},
          {"sabotage base", 4, F::S32},
          {"by target race (5)", 8, F::S32, 5},
          {"by difficulty (simple, easy, normal, hard, impossible)", 28, F::S32, 5},
          {"by morale level (rebellious ... fanatic)", 48, F::S32, 8},
          {"by treaty (neutral, war, alliance, peace)", 80, F::S32, 4}},
         -1, -1, {"Cardassians", "Federation", "Ferengi", "Klingons", "Romulans"}},
        {"meplanet.bin", nullptr, C::FileSize, 0, 88,
         {{"planet (planet.pst)", 0, F::Text, 40},
          {"race", 40, F::Text, 40},
          {"? (10 for the gas giants, 1 for Cardassia 5, otherwise 0)", 80, F::S32},
          {"records for this race", 84, F::S32}},
         0, -1},
        {"aibldreq.bin", nullptr, C::FileSize, 0, 24,
         {{"building (edifice.bst's number)", 0, F::S32},
          {"population needed", 4, F::S32},
          {"maximum population needed", 8, F::S32},
          {"energy needed", 12, F::S32}},
         0, -1},
        {"habit.bin", nullptr, C::FileSize, 0, 8,
         {{"planet type (environ.est)", 0, F::U8},
          {"atmosphere (environ.est)", 1, F::U8},
          {"habitability for each major race, then for minor races and empty systems", 2, F::U8, 6}},
         -1, -1},
        {"meprvnum.bin", nullptr, C::FileSize, 0, 20,
         {{"extra starting systems at each civilization level (Beginning ... Advanced)", 0, F::S32, 5}}, -1, -1,
         {"Cardassians", "Federation", "Ferengi", "Klingons", "Romulans", "?"}},
        {"ppgrowth.bin", nullptr, C::FileSize, 0, 12,
         {{"from", 0, F::U16}, {"to", 2, F::U16}, {"value", 4, F::U32}, {"flag", 8, F::U8}}, -1, -1},
        {"combat.bin", nullptr, C::FileSize, 0, 500,
         {{"first value against each ship (the even bytes)", 0, F::U8, 250},
          {"second value against each ship (the odd bytes)", 250, F::U8, 250}},
         -1, -1},
        // The .wtf tables name their own columns and rows: a u32 (1), u16
        // columns, u16 rows, u32 values, 33-byte column names, 33-byte row
        // names, then f64 values row by row.
        {"aiempire.wtf", nullptr, C::FileSize, 474, 72,
         {{"Default_Espionage_Tolerance", 0, F::F64},
          {"Espionage_Tolerance_Turn_Delta", 8, F::F64},
          {"Espionage_Tolerance_Incident_Delta", 16, F::F64},
          {"Default_Raiding_Tolerance", 24, F::F64},
          {"Raiding_Tolerance_Turn_Delta", 32, F::F64},
          {"Raiding_Tolerance_Incident_Delta", 40, F::F64},
          {"Default_Audience_Tolerance", 48, F::F64},
          {"Audience_Tolerance_Turn_Delta", 56, F::F64},
          {"Audience_Tolerance_Incident_Delta", 64, F::F64}},
         -1, -1, {}, 309, 33},
        {"aiminor.wtf", nullptr, C::FileSize, 1233, 56,
         {{"Personality (0 warlike, 1 peaceful, 2 pacifist, 3 deceptive, 4 spiritual)", 0, F::F64},
          {"Explorer (0 no, 1 yes)", 8, F::F64},
          {"Cardassian_Hostility (0 enraged ... 4 neutral ... 8 worshipful)", 16, F::F64},
          {"Federation_Hostility", 24, F::F64},
          {"Ferengi_Hostility", 32, F::F64},
          {"Klingon_Hostility", 40, F::F64},
          {"Romulan_Hostility", 48, F::F64}},
         -1, -1, {}, 243, 33},
    });
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
    case F::U16: {
        std::string s;
        for (uint32_t i = 0; i < f.size; ++i)
            s += (i ? " " : "") + std::to_string(p[2 * i] | p[2 * i + 1] << 8);
        return s;
    }
    case F::S16: {
        std::string s;
        for (uint32_t i = 0; i < f.size; ++i)
            s += (i ? " " : "") + std::to_string(int16_t(p[2 * i] | p[2 * i + 1] << 8));
        return s;
    }
    case F::U32: {
        std::string s;
        for (uint32_t i = 0; i < f.size; ++i)
            s += (i ? " " : "") + std::to_string(u32(p + 4 * i));
        return s;
    }
    case F::S32: {
        std::string s;
        for (uint32_t i = 0; i < f.size; ++i)
            s += (i ? " " : "") + std::to_string(int32_t(u32(p + 4 * i)));
        return s;
    }
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

std::string DataTable::name(size_t r) const {
    if (r < layout_->recordNames.size())
        return layout_->recordNames[r];
    if (layout_->headerNameSize) {
        const size_t at = layout_->headerNamesAt + r * layout_->headerNameSize;
        if (at + layout_->headerNameSize > layout_->headerSize)
            return std::to_string(r);
        const char* p = reinterpret_cast<const char*>(data_.data() + at);
        return text::windows1252ToUtf8(std::string_view(p, strnlen(p, layout_->headerNameSize)));
    }
    if (layout_->nameField < 0)
        return std::to_string(r);
    return value(r, layout_->fields[size_t(layout_->nameField)]);
}

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
