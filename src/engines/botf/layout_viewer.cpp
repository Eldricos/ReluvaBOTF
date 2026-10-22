// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Reluva contributors

#include "engines/botf/viewers.h"

#include "common/font.h"
#include "common/fs.h"

#include <algorithm>
#include <cstdio>

namespace reluva::botf {

namespace {

// The part of the image inside the clip rectangle, blended over the screen.
void blitClipped(Surface& dst, const image::Image& img, int x, int y, int cx, int cy, int cw, int ch) {
    const int x0 = std::max({x, cx, 0}), y0 = std::max({y, cy, 0});
    const int x1 = std::min({x + img.w, cx + cw, dst.w}), y1 = std::min({y + img.h, cy + ch, dst.h});
    if (x0 >= x1 || y0 >= y1)
        return;
    image::Image part;
    part.w = x1 - x0;
    part.h = y1 - y0;
    part.rgba.resize(size_t(part.w) * part.h * 4);
    for (int py = 0; py < part.h; ++py)
        std::copy_n(img.pixel(x0 - x, y0 - y + py), size_t(part.w) * 4, part.pixel(0, py));
    image::blit(dst, part, x0, y0);
}

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
    : sys_(sys), screen_(screen), archive_(archive) {
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
    if (Error err = archive_.read(names_[current_], data))
        error_ = err.message;
    else if (Error err2 = layout_.load(data))
        error_ = "can't read: " + err2.message;
    else
        placed_ = layout_.placed();
}

const image::Image* LayoutViewer::picture(const std::string& name) {
    if (auto it = pictures_.find(name); it != pictures_.end())
        return &it->second;
    if (missing_.count(name))
        return nullptr;
    std::vector<uint8_t> data;
    image::Image img;
    if (Error err = archive_.read(name, data)) {
        missing_[name] = "missing";
        return nullptr;
    }
    if (Error err = image::decode(data, img)) {
        missing_[name] = "can't decode";
        return nullptr;
    }
    return &pictures_.emplace(name, std::move(img)).first->second;
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
        if (const image::Image* img = w.background.empty() ? nullptr : picture(w.background))
            blitClipped(screen_, *img, p.x, p.y, p.x, p.y, w.w, w.h);
        if (w.type == WidgetType::Button) {
            const char* want = selected_ ? "selected" : "normal";
            for (const auto& [role, name] : w.pictures)
                if (role == want)
                    if (const image::Image* img = picture(name))
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
            if (auto it = missing_.find(name); it != missing_.end())
                pics += " (" + it->second + ")";
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
