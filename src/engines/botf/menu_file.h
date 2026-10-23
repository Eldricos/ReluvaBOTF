// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Reluva contributors

#pragma once

#include "common/types.h"

#include <cstdint>
#include <string>
#include <vector>

// The .mnu menus in stbof.res (docs/formats/botf/wdf.md#menus): the items of
// a pop-up menu that a .wdf menu widget names, each a number the game acts on,
// a picture and a label.

namespace reluva::botf {

struct MenuItem {
    int32_t id = 0;
    std::string picture; // a .tga, or empty for none
    std::string label;   // UTF-8
};

class MenuFile {
public:
    Error load(const std::vector<uint8_t>& data);
    const std::vector<MenuItem>& items() const { return items_; }

private:
    std::vector<MenuItem> items_;
};

} // namespace reluva::botf
