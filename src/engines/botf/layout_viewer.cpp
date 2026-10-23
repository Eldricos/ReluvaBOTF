// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Reluva contributors

#include "engines/botf/viewers.h"

#include "common/font.h"
#include "common/fs.h"

#include <algorithm>
#include <cstdio>

namespace reluva::botf {

namespace {

void outline(Surface& s, int x, int y, int w, int h, Color c) {
    fillRect(s, x, y, w, 1, c);
    fillRect(s, x, y + h - 1, w, 1, c);
    fillRect(s, x, y, 1, h, c);
    fillRect(s, x + w - 1, y, 1, h, c);
}

Color typeColor(WidgetType t) {
    switch (t) {
    case WidgetType::Window: return {120, 120, 255};
    case WidgetType::Button: return {80, 230, 80};
    case WidgetType::Menu: return {255, 160, 60};
    case WidgetType::Slider:
    case WidgetType::ScrollBar: return {230, 80, 230};
    case WidgetType::ListBox: return {80, 220, 230};
    case WidgetType::Picture: return {200, 200, 200};
    case WidgetType::TextBox: return {255, 240, 80};
    }
    return {255, 255, 255};
}

std::string hexColor(Color c) {
    char buf[16];
    std::snprintf(buf, sizeof buf, "#%02x%02x%02x", c.r, c.g, c.b);
    return buf;
}

} // namespace

LayoutViewer::LayoutViewer(System& sys, Surface& screen, const Archive& archive)
    : sys_(sys), screen_(screen), assets_(archive) {
    for (std::string& n : archive.list())
        if (fs::toLower(n).ends_with(".wdf"))
            names_.push_back(std::move(n));
    std::sort(names_.begin(), names_.end(),
              [](const std::string& x, const std::string& y) { return fs::toLower(x) < fs::toLower(y); });
}

void LayoutViewer::activate() {
    if (!names_.empty() && placed_.empty() && error_.empty())
        load();
    show();
}

void LayoutViewer::load() {
    error_.clear();
    placed_.clear();
    widget_ = 0;
    std::vector<uint8_t> data;
    if (Error err = assets_.archive().read(names_[current_], data))
        error_ = err.message;
    else if (Error err2 = layout_.load(data))
        error_ = "can't read: " + err2.message;
    else
        placed_ = layout_.placed();
}

// The menu open where its widget is, tall enough for its items: the inside
// dark, the top and bottom pictures at the top and bottom, the left and
// right ones down the sides, then the items, one a line, the first in the
// second (highlight) font's colour.
void LayoutViewer::drawMenu(const WindowLayout::Placed& p, std::vector<std::string>& lines) {
    const Widget& w = *p.widget;
    const MenuFile* m = w.menuFile.empty() ? nullptr : assets_.menu(w.menuFile);
    if (!m) {
        lines.push_back("menu " + w.menuFile + (assets_.why(w.menuFile).empty() ? "" : " (" + assets_.why(w.menuFile) + ")"));
        return;
    }
    const Font* f = nullptr;
    for (const WidgetFont& wf : w.fonts)
        if (!f && !wf.file.empty())
            f = assets_.font(wf.file);
    const int lineH = f ? f->lineHeight() : font::kLineHeight;
    const image::Image *left = nullptr, *top = nullptr, *right = nullptr, *bottom = nullptr;
    for (const auto& [role, name] : w.pictures) {
        if (role == "left")
            left = assets_.picture(name);
        else if (role == "top")
            top = assets_.picture(name);
        else if (role == "right")
            right = assets_.picture(name);
        else if (role == "bottom")
            bottom = assets_.picture(name);
    }
    const int padTop = top ? top->h : 4, padBottom = bottom ? bottom->h : 4;
    const int padLeft = left ? left->w : 4;
    const int h = std::max(w.h, padTop + padBottom + int(m->items().size()) * lineH);
    const int x = p.x, y = std::min(p.y, screen_.h - h);
    fillRect(screen_, x, y, w.w, h, {0, 0, 0});
    if (left)
        for (int ty = y; ty < y + h; ty += left->h)
            blitClipped(screen_, *left, x, ty, x, y, w.w, h);
    if (right)
        for (int ty = y; ty < y + h; ty += right->h)
            blitClipped(screen_, *right, x + w.w - right->w, ty, x, y, w.w, h);
    if (top)
        blitClipped(screen_, *top, x, y, x, y, w.w, h);
    if (bottom)
        blitClipped(screen_, *bottom, x, y + h - bottom->h, x, y, w.w, h);
    const Color normal = w.fonts.size() > 0 && w.fonts[0].own ? w.fonts[0].color : Color{230, 230, 230};
    const Color hilite = w.fonts.size() > 1 && w.fonts[1].own ? w.fonts[1].color : Color{255, 220, 120};
    std::string list = w.menuFile + ":";
    for (size_t i = 0; i < m->items().size(); ++i) {
        const MenuItem& item = m->items()[i];
        const int ty = y + padTop + int(i) * lineH;
        if (ty + lineH <= y + h) {
            if (f)
                f->draw(screen_, x + padLeft + 2, ty, item.label, i == 0 ? hilite : normal);
            else
                font::drawText(screen_, x + padLeft + 2, ty, item.label, i == 0 ? hilite : normal);
        }
        const std::string entry = " " + std::to_string(item.id) + " " + item.label + (i + 1 < m->items().size() ? "," : "");
        if (list.size() + entry.size() > 125) { // the details strip's width in the built-in font
            lines.push_back(list);
            list = " ";
        }
        list += entry;
    }
    lines.push_back(list);
}

void LayoutViewer::handleKey(uint32_t key) {
    if (names_.empty())
        return;
    const size_t n = names_.size();
    if (key == key::Right || key == key::Left || key == key::PageDown || key == key::PageUp) {
        if (key == key::Right)
            current_ = (current_ + 1) % n;
        else if (key == key::Left)
            current_ = (current_ + n - 1) % n;
        else if (key == key::PageDown)
            current_ = std::min(current_ + 10, n - 1);
        else
            current_ = current_ >= 10 ? current_ - 10 : 0;
        load();
    } else if (key == key::Down && !placed_.empty())
        widget_ = (widget_ + 1) % placed_.size();
    else if (key == key::Up && !placed_.empty())
        widget_ = (widget_ + placed_.size() - 1) % placed_.size();
    else if (key == 'o')
        outlines_ = !outlines_;
    else if (key == 's')
        selected_ = !selected_;
    else if (key == 'i')
        info_ = !info_;
    else
        return;
    show();
}

void LayoutViewer::show() {
    fillRect(screen_, 0, 0, screen_.w, screen_.h, {20, 20, 28});
    if (names_.empty())
        return;
    const std::string& file = names_[current_];

    // Each widget's background, then a button's normal (or selected)
    // picture, clipped to the widget, parents before children.
    for (const WindowLayout::Placed& p : placed_) {
        const Widget& w = *p.widget;
        if (const image::Image* img = w.background.empty() ? nullptr : assets_.picture(w.background))
            blitClipped(screen_, *img, p.x, p.y, p.x, p.y, w.w, w.h);
        if (w.type == WidgetType::Button) {
            const char* want = selected_ ? "selected" : "normal";
            for (const auto& [role, name] : w.pictures)
                if (role == want)
                    if (const image::Image* img = assets_.picture(name))
                        blitClipped(screen_, *img, p.x, p.y, p.x, p.y, w.w, w.h);
        }
    }
    if (outlines_)
        for (const WindowLayout::Placed& p : placed_)
            outline(screen_, p.x, p.y, p.widget->w, p.widget->h, typeColor(p.widget->type));
    if (!placed_.empty()) {
        const WindowLayout::Placed& p = placed_[widget_];
        outline(screen_, p.x - 1, p.y - 1, p.widget->w + 2, p.widget->h + 2, {255, 255, 255});
        outline(screen_, p.x - 2, p.y - 2, p.widget->w + 4, p.widget->h + 4, {0, 0, 0});
    }

    std::string head = std::to_string(current_ + 1) + "/" + std::to_string(names_.size()) + "  " + file;
    if (!error_.empty())
        head += "  " + error_;
    std::vector<std::string> lines = {head};
    if (!placed_.empty()) {
        const WindowLayout::Placed& p = placed_[widget_];
        const Widget& w = *p.widget;
        lines[0] += "  kind " + std::to_string(layout_.kind()) + ", " + std::to_string(placed_.size()) + " widgets";
        lines.push_back(std::to_string(widget_ + 1) + "/" + std::to_string(placed_.size()) + "  " +
                        std::string(p.depth ? std::string(size_t(p.depth) * 2, ' ') : "") + widgetTypeName(w.type) +
                        " " + std::to_string(w.id) + "  at " + std::to_string(w.x) + "," + std::to_string(w.y) +
                        " (screen " + std::to_string(p.x) + "," + std::to_string(p.y) + ")  size " +
                        std::to_string(w.w) + "x" + std::to_string(w.h));
        std::string pics;
        for (const auto& [role, name] : w.pictures) {
            pics += (pics.empty() ? "" : ", ") + role + " " + name;
            if (const std::string why = assets_.why(name); !why.empty())
                pics += " (" + why + ")";
        }
        if (!pics.empty())
            lines.push_back(pics);
        std::string more;
        if (w.type == WidgetType::Button)
            more = "kind " + std::to_string(w.buttonKind) + (w.repeats ? ", repeats " + std::to_string(w.repeatsPerSecond) + "/s" : "");
        if (w.type == WidgetType::Menu || w.type == WidgetType::TextBox)
            more = "text at " + std::to_string(w.textX) + "," + std::to_string(w.textY) + " size " +
                   std::to_string(w.textW) + "x" + std::to_string(w.textH);
        if (!w.menuFile.empty())
            more += "  menu " + w.menuFile;
        if (w.type == WidgetType::Slider || w.type == WidgetType::ScrollBar)
            more = "values " + std::to_string(w.minValue) + "-" + std::to_string(w.maxValue) +
                   (w.units ? ", units " + std::to_string(w.units) : "");
        if (w.type == WidgetType::ListBox) {
            more = std::to_string(w.columns) + " columns:";
            for (int32_t i = 0; i < std::min(w.columns, int32_t(w.columnWidths.size())); ++i)
                more += " " + std::to_string(w.columnWidths[size_t(i)]);
        }
        for (const WidgetFont& f : w.fonts)
            more += "  font " + (f.file.empty() ? std::string("-") : f.file) + " " + hexColor(f.color) +
                    (f.own ? " (own)" : "");
        if (!w.unknown.empty()) {
            more += "  ?";
            for (int32_t v : w.unknown)
                more += " " + std::to_string(v);
        }
        if (!more.empty())
            lines.push_back(more);
    }
    if (!placed_.empty() && placed_[widget_].widget->type == WidgetType::Menu)
        drawMenu(placed_[widget_], lines);
    if (info_) {
        const int h = int(lines.size()) * font::kLineHeight + 4;
        fillRect(screen_, 0, screen_.h - h, screen_.w, h, {0, 0, 0});
        for (size_t i = 0; i < lines.size(); ++i)
            font::drawText(screen_, 4, screen_.h - h + 2 + int(i) * font::kLineHeight, lines[i], {230, 230, 230});
    }
    sys_.setWindowTitle("Reluva - Birth of the Federation - layouts " + std::to_string(current_ + 1) + "/" +
                        std::to_string(names_.size()) + ": " + file +
                        " (Left/Right, Up/Down: widget, O: outlines, S: selected, I: info, Tab: next page)");
}

} // namespace reluva::botf
