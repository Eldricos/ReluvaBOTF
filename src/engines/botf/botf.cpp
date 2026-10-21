// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Reluva contributors

// Star Trek: Birth of the Federation engine (clean-room reimplementation).
//
// Everything in this engine must come from the game's data files, observed
// behaviour of the original, or documented reverse engineering of a legally
// owned copy. Never from leaked source code. See CONTRIBUTING.md and
// docs/engines/botf.md.
//
// For now the engine is a data viewer: the images of stbof.res and alt.res,
// and the sounds of sfx.snd, the language's speech archive and music.snd
// (see viewers.h for the keys). Tab switches between them; Esc goes back to
// the launcher.

#include "engines/engine.h"
#include "engines/botf/alt_res.h"
#include "engines/botf/game_files.h"
#include "engines/botf/snd_archive.h"
#include "engines/botf/viewers.h"
#include "common/zip.h"

#include <memory>

namespace reluva {

namespace {

using botf::AltResArchive;
using botf::GameFiles;
using botf::ImageViewer;
using botf::SndArchive;
using botf::SoundViewer;

// Detection table. Built with `reluva --hash`: each entry needs the size and
// the MD5 of the first kDetectionHashBytes bytes. Detection keys on the data
// files: trek.exe is only SafeDisc's loader (docs/engines/botf.md), and
// patches or replacements of it shouldn't stop a copy being recognised.
const std::vector<GameVariant> kVariants = {
    {"botf", "Star Trek: Birth of the Federation", "en", "windows",
     {{"stbof.res", "2e888c1eaefa901da9e2dbd6229798a5", 79901501},
      {"english.snd", "806bf768303d60745c792fae58ec506f", 39509316}}},
    // stbof.res as on the CD's GERMAN folder; german.snd not hashed yet.
    {"botf", "Star Trek: Birth of the Federation", "de", "windows",
     {{"stbof.res", "21df64c3479690c70f50bfc0bed3792b", 79385299}, {"german.snd", "", -1}}},
};

// The same game by file names alone, for releases not in kVariants yet.
// Used only when nothing there matches.
const std::vector<GameVariant> kFallback = {
    {"botf", "Star Trek: Birth of the Federation (unknown release)", "", "windows",
     {{"stbof.res", "", -1}, {"sfx.snd", "", -1}}},
};

class BotfEngine : public Engine {
public:
    BotfEngine(System& system, GameTarget target) : sys_(system), target_(std::move(target)) {}

    Error run() override {
        GameFiles files(target_.path);
        if (Error err = openArchives(files))
            return err;
        if (Error err = sys_.initGraphics(botf::kScreenW, botf::kScreenH, PixelFormat::RGBA32))
            return err;
        screen_.create(botf::kScreenW, botf::kScreenH, PixelFormat::RGBA32);

        // One mixer for as long as the engine runs; without an audio device
        // sounds are listed and not played.
        audio::Mixer mixer(sys_.audioRate());
        const bool audioOut = !audio::startMixerOutput(sys_, mixer);
        struct AudioStop {
            System& sys;
            bool on;
            ~AudioStop() {
                if (on)
                    sys.stopAudio();
            }
        } audioStop{sys_, audioOut};

        std::vector<const Archive*> imageArchives = {&stbof_};
        if (alt_)
            imageArchives.push_back(alt_.get());
        ImageViewer images(sys_, screen_, imageArchives);
        std::vector<std::pair<std::string, const SndArchive*>> soundArchives;
        for (const auto& [name, snd] : sounds_)
            soundArchives.push_back({name, snd.get()});
        SoundViewer sounds(sys_, screen_, soundArchives, audioOut ? &mixer : nullptr);

        // Page 0 shows images, 1 sounds; Tab moves to the other if it has
        // something to show.
        int page = images.count() ? 0 : 1;
        auto activate = [&] {
            if (page == 0)
                images.activate();
            else
                sounds.activate();
        };
        activate();
        while (true) {
            Event ev;
            while (sys_.pollEvent(ev)) {
                if (ev.type == EventType::Quit || ev.type == EventType::ReturnToLauncher)
                    return Error::success();
                if (ev.type != EventType::KeyDown)
                    continue;
                if (ev.key == key::Escape)
                    return Error::success();
                if (ev.key == key::Tab) {
                    const int other = 1 - page;
                    if ((other == 0 ? images.count() : sounds.count()) == 0)
                        continue;
                    if (page == 1)
                        sounds.deactivate();
                    page = other;
                    activate();
                    continue;
                }
                if (page == 0)
                    images.handleKey(ev.key);
                else
                    sounds.handleKey(ev.key);
            }
            if (page == 1)
                sounds.update(sys_.getMillis());
            sys_.updateScreen(screen_);
            sys_.delayMillis(10);
        }
    }

private:
    // stbof.res is required; alt.res and the sound archives are shown when
    // found (music.snd is often only on the CD).
    Error openArchives(const GameFiles& files) {
        const auto stbof = files.find("stbof.res");
        if (!stbof)
            return Error::fail("No stbof.res in " + target_.path.string());
        if (Error err = stbof_.open(*stbof))
            return err;
        if (const auto alt = files.find("alt.res")) {
            alt_ = std::make_unique<AltResArchive>();
            if (Error err = alt_->open(*alt))
                return err;
        }
        const std::pair<std::string, bool> sndFiles[] = {
            {"sfx.snd", false}, {files.speechFile(), false}, {"music.snd", true}};
        for (const auto& [name, music] : sndFiles) {
            const auto path = files.find(name);
            if (!path)
                continue;
            auto snd = std::make_unique<SndArchive>();
            if (Error err = snd->open(*path, music))
                return err;
            sounds_.push_back({name, std::move(snd)});
        }
        return Error::success();
    }

    System& sys_;
    GameTarget target_;
    Surface screen_;
    ZipArchive stbof_;
    std::unique_ptr<AltResArchive> alt_;
    std::vector<std::pair<std::string, std::unique_ptr<SndArchive>>> sounds_;
};

class BotfMetaEngine : public MetaEngine {
public:
    const char* id() const override { return "botf"; }
    const char* name() const override { return "Birth of the Federation"; }

    std::vector<DetectedGame> detect(const std::filesystem::path& dir) const override {
        auto found = detectFromTable(id(), kVariants, dir);
        if (found.empty())
            found = detectFromTable(id(), kFallback, dir);
        return found;
    }

    std::unique_ptr<Engine> create(System& system, const GameTarget& target) const override {
        return std::make_unique<BotfEngine>(system, target);
    }
};

} // namespace

const MetaEngine& botfMetaEngine() {
    static const BotfMetaEngine instance;
    return instance;
}

} // namespace reluva
