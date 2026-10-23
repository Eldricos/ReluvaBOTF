// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Reluva contributors

#include "engines/botf/setup_screen.h"

#include <algorithm>
#include <vector>

namespace reluva::botf {

namespace {

// gmesetup.wdf's widgets, matched to the manual's controls by their order
// on the screen and checked against a recording of the game.
constexpr int32_t kAccept = 9201, kCancel = 9202;
constexpr std::array<int32_t, 5> kCivButtons = {9223, 9225, 9227, 9229, 9231};
constexpr std::array<int32_t, 5> kCivTexts = {9224, 9226, 9228, 9230, 9232};
constexpr int32_t kMinorRaces = 9207, kMinorRacesText = 9208;
constexpr int32_t kDifficulty = 9209, kDifficultyText = 9210;
constexpr int32_t kStrategicLess = 9211, kStrategicMore = 9212, kStrategicText = 9213;
constexpr int32_t kTacticalLess = 9214, kTacticalMore = 9215, kTacticalText = 9216;
constexpr int32_t kRandomEvents = 9217, kRandomEventsText = 9218;
constexpr int32_t kCombat = 9219, kCombatText = 9220;
constexpr int32_t kVictory = 9221, kVictoryText = 9222;
constexpr int32_t kShape = 9234, kShapeText = 9206, kShapePicture = 9204;
constexpr int32_t kSize = 9233, kSizeText = 9205, kSizePicture = 9203;

// The texts' colours, measured from the brightest pixels of the text in a
// recording of the game; the files don't hold them (most of these fonts say
// "the game's default"), and the timers' own colour doesn't explain theirs.
constexpr Color kValueColor{51, 90, 140};     // the preferences and the galaxy
constexpr Color kLevelColor{120, 180, 150};   // the civilization levels
constexpr Color kTimerColor{176, 202, 153};   // the two timers

constexpr int kTimerChoices = 1; // "None"; the others aren't known yet

const char* const kShapePictures[] = {"cc_gsir.tga", "cc_gsel.tga", "cc_gsrg.tga", "cc_gssp.tga"};
const char* const kSizePictures[] = {"cc_gzsm.tga", "cc_gzmd.tga", "cc_gzlg.tga"};

Error loadLayout(Assets& assets, const char* name, WindowLayout& layout) {
    std::vector<uint8_t> data;
    if (Error err = assets.archive().read(name, data))
        return err;
    if (Error err = layout.load(data))
        return Error::fail(std::string(name) + ": " + err.message);
    return Error::success();
}

} // namespace

Error SetupScreen::load() {
    if (Error err = loadLayout(assets_, "opening.wdf", menu_))
        return err;
    if (Error err = loadLayout(assets_, "gmesetup.wdf", setupLayout_))
        return err;
    widgets_.clear();
    for (const WindowLayout::Placed& p : setupLayout_.placed())
        widgets_[p.widget->id] = {p.widget, p.x, p.y};
    for (int32_t id : {kAccept, kCancel, kMinorRaces, kDifficulty, kStrategicLess, kStrategicMore, kTacticalLess,
                       kTacticalMore, kRandomEvents, kCombat, kVictory, kShape, kSize})
        if (!find(id))
            return Error::fail("gmesetup.wdf has no widget " + std::to_string(id));
    for (int32_t id : kCivButtons)
        if (!find(id))
            return Error::fail("gmesetup.wdf has no widget " + std::to_string(id));
    return Error::success();
}

const SetupScreen::Placed* SetupScreen::find(int32_t id) const {
    auto it = widgets_.find(id);
    return it == widgets_.end() ? nullptr : &it->second;
}

int32_t SetupScreen::buttonAt(int x, int y) const {
    for (const auto& [id, p] : widgets_)
        if (p.widget->type == WidgetType::Button && x >= p.x && y >= p.y && x < p.x + p.widget->w &&
            y < p.y + p.widget->h)
            return id;
    return 0;
}

std::string SetupScreen::word(int entry, const char* english) const {
    const std::string& w = lexicon_[size_t(entry)];
    return w.empty() ? english : w;
}

void SetupScreen::click(int32_t id, bool shift) {
    GameSetup& s = setup_;
    for (size_t i = 0; i < kCivButtons.size(); ++i)
        if (id == kCivButtons[i]) {
            // The manual: with Shift, every empire takes this one's level;
            // if they already have it, the next level.
            const bool same = std::all_of(s.civilization.begin(), s.civilization.end(),
                                          [&](int c) { return c == s.civilization[i]; });
            const int level = shift && !same ? s.civilization[i] : (s.civilization[i] + 1) % 5;
            if (shift)
                s.civilization.fill(level);
            else
                s.civilization[i] = level;
            return;
        }
    switch (id) {
    case kMinorRaces: s.minorRaces = (s.minorRaces + 1) % 4; break;
    case kDifficulty: s.difficulty = (s.difficulty + 1) % 5; break;
    case kStrategicLess: s.strategicTimer = std::max(s.strategicTimer - 1, 0); break;
    case kStrategicMore: s.strategicTimer = std::min(s.strategicTimer + 1, kTimerChoices - 1); break;
    case kTacticalLess: s.tacticalTimer = std::max(s.tacticalTimer - 1, 0); break;
    case kTacticalMore: s.tacticalTimer = std::min(s.tacticalTimer + 1, kTimerChoices - 1); break;
    case kRandomEvents: s.randomEvents = !s.randomEvents; break;
    case kCombat: s.manualCombat = !s.manualCombat; break;
    case kVictory: s.victory = (s.victory + 1) % 2; break;
    case kShape: s.galaxyShape = (s.galaxyShape + 1) % 4; break;
    case kSize: s.galaxySize = (s.galaxySize + 1) % 3; break;
    default: break;
    }
}

SetupScreen::Result SetupScreen::handle(const Event& e) {
    if (e.type == EventType::KeyDown && (e.key == key::Return || e.key == key::KpEnter))
        return Result::Accepted;
    if (e.type == EventType::KeyDown && e.key == key::Escape)
        return Result::Cancelled;
    if (e.button != MouseButton::Left)
        return Result::Open;
    if (e.type == EventType::MouseDown) {
        pressed_ = buttonAt(e.mouseX, e.mouseY);
    } else if (e.type == EventType::MouseUp) {
        // A button acts when the mouse comes up on it, as Windows buttons do.
        const int32_t id = pressed_ && buttonAt(e.mouseX, e.mouseY) == pressed_ ? pressed_ : 0;
        pressed_ = 0;
        if (id == kAccept)
            return Result::Accepted;
        if (id == kCancel)
            return Result::Cancelled;
        if (id)
            click(id, (e.mods & keymod::Shift) != 0);
    }
    return Result::Open;
}

void SetupScreen::drawButton(Surface& screen, const Placed& p) {
    const Widget& w = *p.widget;
    const char* want = p.widget->id == pressed_ ? "selected" : "normal";
    for (const auto& [role, name] : w.pictures)
        if (role == want)
            if (const image::Image* img = assets_.picture(name))
                blitClipped(screen, *img, p.x, p.y, p.x, p.y, w.w, w.h);
}

void SetupScreen::drawText(Surface& screen, int32_t textBox, const std::string& text, Color color) {
    const Placed* p = find(textBox);
    if (!p || p->widget->fonts.empty())
        return;
    const Widget& w = *p->widget;
    const WidgetFont& wf = w.fonts[0];
    const Font* f = assets_.font(wf.file);
    if (!f)
        return;
    const int width = f->measure(text);
    int x = p->x + w.textX;
    if (wf.align() == WidgetFont::Align::Center)
        x += (w.textW - width) / 2;
    else if (wf.align() == WidgetFont::Align::Right)
        x += w.textW - width;
    const int y = p->y + w.textY + (w.textH - f->glyphHeight()) / 2;
    f->drawInColor(screen, x, y, text, color);
}

void SetupScreen::draw(Surface& screen) {
    // The main menu, then the setup screen over it: each layout's
    // backgrounds and buttons, parents before children.
    for (WindowLayout* layout : {&menu_, &setupLayout_})
        for (const WindowLayout::Placed& lp : layout->placed()) {
            const Widget& w = *lp.widget;
            if (const image::Image* img = w.background.empty() ? nullptr : assets_.picture(w.background))
                blitClipped(screen, *img, lp.x, lp.y, lp.x, lp.y, w.w, w.h);
            if (w.type == WidgetType::Button)
                drawButton(screen, {lp.widget, lp.x, lp.y});
        }

    const GameSetup& s = setup_;
    for (auto [id, file] : {std::pair{kShapePicture, kShapePictures[s.galaxyShape]},
                            std::pair{kSizePicture, kSizePictures[s.galaxySize]}})
        if (const Placed* p = find(id))
            if (const image::Image* img = assets_.picture(file))
                blitClipped(screen, *img, p->x, p->y, p->x, p->y, p->widget->w, p->widget->h);

    const std::pair<int, const char*> levels[] = {{lex::Beginning, "1 BEGINNING"},
                                                  {lex::Early, "2 EARLY"},
                                                  {lex::Developed, "3 DEVELOPED"},
                                                  {lex::Expanded, "4 EXPANDED"},
                                                  {lex::Advanced, "5 ADVANCED"}};
    for (size_t i = 0; i < kCivTexts.size(); ++i) {
        const auto& [entry, english] = levels[size_t(s.civilization[i])];
        drawText(screen, kCivTexts[i], word(entry, english), kLevelColor);
    }
    const std::pair<int, const char*> minors[] = {
        {lex::None, "NONE"}, {lex::Few, "FEW"}, {lex::Some, "SOME"}, {lex::Many, "MANY"}};
    const std::pair<int, const char*> difficulties[] = {{lex::Simple, "SIMPLE"},
                                                        {lex::Easy, "EASY"},
                                                        {lex::Normal, "NORMAL"},
                                                        {lex::Hard, "HARD"},
                                                        {lex::Impossible, "IMPOSSIBLE"}};
    const std::pair<int, const char*> victories[] = {{lex::Domination, "DOMINATION"}, {lex::Vendetta, "VENDETTA"}};
    const std::pair<int, const char*> shapes[] = {
        {lex::Irregular, "Irregular"}, {lex::Elliptical, "Elliptical"}, {lex::Ring, "Ring"}, {lex::Spiral, "Spiral"}};
    const std::pair<int, const char*> sizes[] = {{lex::Small, "Small"}, {lex::Medium, "Medium"}, {lex::Large, "Large"}};
    auto value = [&](int32_t box, const std::pair<int, const char*>& w) {
        drawText(screen, box, word(w.first, w.second), kValueColor);
    };
    value(kMinorRacesText, minors[s.minorRaces]);
    value(kDifficultyText, difficulties[s.difficulty]);
    value(kRandomEventsText, s.randomEvents ? std::pair{lex::On, "ON"} : std::pair{lex::Off, "OFF"});
    value(kCombatText, s.manualCombat ? std::pair{lex::Manual, "MANUAL"} : std::pair{lex::Automatic, "AUTOMATIC"});
    value(kVictoryText, victories[s.victory]);
    value(kShapeText, shapes[s.galaxyShape]);
    value(kSizeText, sizes[s.galaxySize]);
    drawText(screen, kStrategicText, word(lex::None, "NONE"), kTimerColor);
    drawText(screen, kTacticalText, word(lex::None, "NONE"), kTimerColor);
}

} // namespace reluva::botf
