// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Reluva contributors

#include "engines/botf/viewers.h"

#include "common/font.h"
#include "common/fs.h"
#include "common/image.h"

#include <algorithm>
#include <filesystem>

namespace reluva::botf {

void fillRect(Surface& s, int x, int y, int w, int h, Color c) {
    const int x0 = std::max(x, 0), y0 = std::max(y, 0);
    const int x1 = std::min(x + w, s.w), y1 = std::min(y + h, s.h);
    for (int py = y0; py < y1; ++py) {
        uint8_t* p = s.row(py) + size_t(x0) * 4;
        for (int px = x0; px < x1; ++px, p += 4) {
            p[0] = c.r;
            p[1] = c.g;
            p[2] = c.b;
            p[3] = 255;
        }
    }
}

ImageViewer::ImageViewer(System& sys, Surface& screen, std::vector<const Archive*> archives)
    : sys_(sys), screen_(screen) {
    for (const Archive* a : archives) {
        std::vector<std::string> names = a->list();
        std::sort(names.begin(), names.end(),
                  [](const std::string& x, const std::string& y) { return fs::toLower(x) < fs::toLower(y); });
        for (std::string& n : names)
            if (fs::toLower(n).ends_with(".tga"))
                images_.push_back({a, std::move(n)});
    }
}

void ImageViewer::activate() { show(current_); }

void ImageViewer::handleKey(uint32_t key) {
    if (images_.empty())
        return;
    const size_t n = images_.size();
    size_t next = current_;
    if (key == key::Right)
        next = (current_ + 1) % n;
    else if (key == key::Left)
        next = (current_ + n - 1) % n;
    else if (key == key::PageDown)
        next = std::min(current_ + 10, n - 1);
    else if (key == key::PageUp)
        next = current_ >= 10 ? current_ - 10 : 0;
    else if (key == key::Home)
        next = 0;
    else if (key == key::End)
        next = n - 1;
    else if (key == 's') {
        // The first image of the next archive, wrapping round.
        next = current_;
        do
            next = (next + 1) % n;
        while (next != current_ && images_[next].archive == images_[current_].archive);
        while (next > 0 && images_[next - 1].archive == images_[next].archive)
            --next;
    } else
        return;
    show(next);
}

void ImageViewer::show(size_t index) {
    fillRect(screen_, 0, 0, screen_.w, screen_.h, {40, 40, 48});
    if (images_.empty())
        return;
    current_ = index;
    const ImageRef& ref = images_[index];
    std::string status;
    std::vector<uint8_t> data;
    image::Image img;
    if (Error err = ref.archive->read(ref.name, data))
        status = err.message;
    else if (Error err2 = image::decode(data, img))
        status = "can't decode: " + err2.message;
    else {
        // Larger images are shown at half or a third of their size; smaller
        // ones in the middle at their own.
        int scale = 1;
        while (img.w / scale > screen_.w || img.h / scale > screen_.h - 14)
            ++scale;
        image::Image shown = img;
        if (scale > 1) {
            shown.w = img.w / scale;
            shown.h = img.h / scale;
            shown.rgba.resize(size_t(shown.w) * shown.h * 4);
            for (int y = 0; y < shown.h; ++y)
                for (int x = 0; x < shown.w; ++x)
                    std::copy_n(img.pixel(x * scale, y * scale), 4, shown.pixel(x, y));
        }
        image::blit(screen_, shown, (screen_.w - shown.w) / 2, (screen_.h - 14 - shown.h) / 2);
        status = std::to_string(img.w) + "x" + std::to_string(img.h) + (scale > 1 ? " (shown at 1/" +
                 std::to_string(scale) + ")" : "");
    }
    const std::string line = std::to_string(index + 1) + "/" + std::to_string(images_.size()) + "  " +
                             std::filesystem::path(ref.archive->describe()).filename().string() + ": " + ref.name + "  " + status;
    fillRect(screen_, 0, screen_.h - 14, screen_.w, 14, {0, 0, 0});
    font::drawText(screen_, 4, screen_.h - 12, line, {230, 230, 230});
    sys_.setWindowTitle("Reluva - Birth of the Federation - images " + std::to_string(index + 1) + "/" +
                        std::to_string(images_.size()) + ": " + ref.name +
                        " (Left/Right, S: next archive, Tab: sounds)");
}

} // namespace reluva::botf
