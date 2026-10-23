// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Reluva contributors

#pragma once

#include "common/types.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

// lexicon.dic in stbof.res (docs/formats/botf/lexicon.md): the words and
// phrases the interface shows (option values, race and star names, labels),
// by number. Which number is which comes from the screens; see lexicon.md.

namespace reluva::botf {

class Lexicon {
public:
    Error load(const std::vector<uint8_t>& data);
    size_t size() const { return entries_.size(); }
    // The entry as UTF-8, or empty past the end.
    const std::string& operator[](size_t i) const;

private:
    std::vector<std::string> entries_;
};

// Entries the screens use, numbered as in the English lexicon.dic. Found by
// the modding community by changing an entry and seeing which screen
// changed (Ultimate Editor, ue/edit/res/stbof/files/dic/idx/LexDataIdx.java).
namespace lex {
constexpr int Advanced = 4, Automatic = 52, Beginning = 88, Developed = 234, Domination = 248, Early = 252,
              Easy = 256, Elliptical = 264, Expanded = 312, Few = 358, Hard = 433, Impossible = 482,
              Irregular = 555, Large = 596, Manual = 621, Many = 622, Medium = 627, None = 872, Normal = 873,
              Off = 880, On = 882, Ring = 1056, Simple = 1150, Small = 1158, Some = 1161, Spiral = 1172,
              TeamPlay = 1253, Vendetta = 1407;
} // namespace lex

} // namespace reluva::botf
