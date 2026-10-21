// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Reluva contributors

#include "engines/botf/viewers.h"

#include "common/audio/decode.h"
#include "common/font.h"

#include <algorithm>
#include <cstdio>
#include <iterator>

namespace reluva::botf {

namespace {

constexpr int kRates[] = {11025, 22050, 44100};
constexpr int kListTop = 40;
constexpr int kRowHeight = 12;

std::string seconds(double s) {
    char buf[32];
    std::snprintf(buf, sizeof buf, "%.2f s", s);
    return buf;
}

} // namespace

SoundViewer::SoundViewer(System& sys, Surface& screen,
                         std::vector<std::pair<std::string, const SndArchive*>> archives, audio::Mixer* mixer)
    : sys_(sys), screen_(screen), archives_(std::move(archives)), mixer_(mixer) {
    for (size_t a = 0; a < archives_.size(); ++a) {
        const SndArchive& snd = *archives_[a].second;
        const size_t n = snd.isMusic() ? snd.pieces().size() : snd.entries().size();
        for (size_t i = 0; i < n; ++i)
            items_.push_back({a, i});
    }
}

void SoundViewer::activate() {
    if (sound_ == nullptr && error_.empty() && !items_.empty())
        decodeCurrent();
    redraw();
}

void SoundViewer::deactivate() { stop(); }

void SoundViewer::handleKey(uint32_t key) {
    if (items_.empty())
        return;
    const size_t n = items_.size();
    if (key == key::Space || key == key::Return || key == key::KpEnter) {
        if (mixer_ && playing_ != audio::kNoSound && mixer_->isPlaying(playing_))
            stop();
        else
            play();
        redraw();
        return;
    }
    if (key == 'a') {
        autoPlay_ = !autoPlay_;
        redraw();
        return;
    }
    if (key == 'r') {
        size_t r = 0;
        while (r < std::size(kRates) && kRates[r] != rate_)
            ++r;
        rate_ = kRates[(r + 1) % std::size(kRates)];
        stop();
        decodeCurrent();
        if (autoPlay_)
            play();
        redraw();
        return;
    }
    size_t next = current_;
    if (key == key::Down)
        next = (current_ + 1) % n;
    else if (key == key::Up)
        next = (current_ + n - 1) % n;
    else if (key == key::PageDown)
        next = std::min(current_ + 20, n - 1);
    else if (key == key::PageUp)
        next = current_ >= 20 ? current_ - 20 : 0;
    else if (key == key::Home)
        next = 0;
    else if (key == key::End)
        next = n - 1;
    else if (key == key::Right || key == key::Left) {
        // The first item of the next or previous archive, wrapping round.
        const size_t archive = items_[current_].archive;
        const size_t target = key == key::Right ? (archive + 1) % archives_.size()
                                                : (archive + archives_.size() - 1) % archives_.size();
        next = 0;
        while (next < n && items_[next].archive != target)
            ++next;
        if (next == n)
            return;
    } else
        return;
    select(next);
}

void SoundViewer::select(size_t index) {
    stop();
    current_ = index;
    decodeCurrent();
    if (autoPlay_)
        play();
    redraw();
}

void SoundViewer::decodeCurrent() {
    sound_.reset();
    error_.clear();
    const Item& it = items_[current_];
    const SndArchive& snd = *archives_[it.archive].second;
    auto sound = std::make_shared<audio::Sound>();
    Error err = snd.isMusic() ? snd.decodePiece(it.index, rate_, *sound) : snd.decode(it.index, rate_, *sound);
    if (err)
        error_ = err.message;
    else
        sound_ = std::move(sound);
}

void SoundViewer::play() {
    stop();
    if (mixer_ && sound_ && !sound_->samples.empty()) {
        const SndArchive& snd = *archives_[items_[current_].archive].second;
        playing_ = mixer_->play(sound_, snd.isMusic() ? audio::SoundType::Music : audio::SoundType::Effects);
    }
}

void SoundViewer::stop() {
    if (mixer_ && playing_ != audio::kNoSound)
        mixer_->stop(playing_);
    playing_ = audio::kNoSound;
}

void SoundViewer::update(uint64_t nowMs) {
    // While a sound plays, redraw its progress a few times a second; once
    // it ends, once more to show it stopped.
    if (playing_ == audio::kNoSound || nowMs - lastDrawMs_ < 100)
        return;
    lastDrawMs_ = nowMs;
    if (mixer_ && !mixer_->isPlaying(playing_))
        playing_ = audio::kNoSound;
    redraw();
}

std::string SoundViewer::describe(size_t item) const {
    const Item& it = items_[item];
    const auto& [name, snd] = archives_[it.archive];
    char buf[160];
    if (snd->isMusic()) {
        const auto& piece = snd->pieces()[it.index];
        size_t bytes = 0;
        for (size_t e : piece)
            bytes += snd->entries()[e].size;
        std::snprintf(buf, sizeof buf, "%s  piece %2zu  %2zu segment%s  %9zu bytes", name.c_str(), it.index,
                      piece.size(), piece.size() == 1 ? " " : "s", bytes);
    } else {
        const SndArchive::Entry& e = snd->entries()[it.index];
        std::snprintf(buf, sizeof buf, "%s  %4zu  offset %9u  size %8u  flags %u", name.c_str(), it.index, e.offset,
                      e.size, e.flags);
    }
    return buf;
}

void SoundViewer::redraw() {
    fillRect(screen_, 0, 0, screen_.w, screen_.h, {20, 24, 36});
    if (items_.empty())
        return;
    const Color text{220, 220, 220}, dim{140, 140, 160}, hilite{255, 220, 120};

    // The current sound: what it is and how far it has played.
    std::string info;
    if (!error_.empty())
        info = error_;
    else if (sound_) {
        const double length = sound_->rate ? double(sound_->frames()) / sound_->rate : 0;
        info = std::string(audio::formatName(sound_->format)) + ", " + std::to_string(sound_->rate) + " Hz, " +
               (sound_->channels == 2 ? "stereo" : "mono") + ", " + seconds(length);
        if (mixer_ && playing_ != audio::kNoSound && mixer_->isPlaying(playing_))
            info += "  playing " + seconds(mixer_->position(playing_) / sound_->rate);
    }
    font::drawText(screen_, 8, 8, describe(current_), hilite);
    font::drawText(screen_, 8, 20, info, text);

    // The list around the current item, with archive changes marked.
    const int rows = (screen_.h - kListTop - 30) / kRowHeight;
    size_t first = current_ > size_t(rows / 2) ? current_ - size_t(rows / 2) : 0;
    first = std::min(first, items_.size() > size_t(rows) ? items_.size() - size_t(rows) : 0);
    for (int r = 0; r < rows && first + size_t(r) < items_.size(); ++r) {
        const size_t i = first + size_t(r);
        const int y = kListTop + r * kRowHeight;
        if (i == current_)
            fillRect(screen_, 4, y - 2, screen_.w - 8, kRowHeight, {50, 60, 90});
        font::drawText(screen_, 12, y, describe(i), i == current_ ? hilite : text);
    }

    const std::string help = "Up/Down: select  Left/Right: archive  Space: play/stop  A: autoplay " +
                             std::string(autoPlay_ ? "on" : "off") + "  R: ADPCM rate " + std::to_string(rate_) +
                             " Hz" + (mixer_ ? "" : "  (no audio device)") + "  Tab: next page";
    font::drawText(screen_, 8, screen_.h - 16, help, dim);
    sys_.setWindowTitle("Reluva - Birth of the Federation - sounds " + std::to_string(current_ + 1) + "/" +
                        std::to_string(items_.size()));
}

} // namespace reluva::botf
