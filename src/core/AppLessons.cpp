// App: the tutorial's lessons (2026-10-19) - the first time each kind of stop
// comes up in the tutorial run, a card stops the game and says plainly what
// the stop is and how to play it: the map and its stops, protecting the core,
// moving the balls (click, slingshot, Q), recruiting, items and TAB, the "?",
// the shop and the finale. Click / Space / Enter turns the page. The bottom
// band's one-line hints (AppHints.cpp) still follow, as reminders.

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

#include "core/App.hpp"
#include "core/Config.hpp"
#include "core/Theme.hpp"
#include "render/Draw.hpp"
#include "ui/Screens.hpp"
#include "ui/Widgets.hpp"

namespace sb {

namespace {

// One line of a lesson: plain text, or text beside a key cap or a map stop.
struct LessonRow {
    enum Kind { Text, Key, Node, Note } kind = Text;
    std::string text;
    std::string key;                          // Key: what's on the cap
    MapNodeType node = MapNodeType::Combat;   // Node: which stop
};
struct LessonPage {
    std::string kicker, title;
    std::vector<LessonRow> rows;
};

LessonRow text(std::string t) { return {LessonRow::Text, std::move(t), "", MapNodeType::Combat}; }
LessonRow key(std::string k, std::string t) { return {LessonRow::Key, std::move(t), std::move(k), MapNodeType::Combat}; }
LessonRow node(MapNodeType n, std::string t) { return {LessonRow::Node, std::move(t), "", n}; }

std::vector<LessonPage> lessonPages(App::Lesson l) {
    using T = MapNodeType;
    switch (l) {
        case App::Lesson::Map:
            return {{"Tutorial", "The map",
                     {text("Climb from the bottom. Click a lit stop."),
                      node(T::Combat, "Fight"),
                      node(T::Recruit, "New ball"),
                      node(T::Elite, "Hard fight, strong item"),
                      node(T::Upgrade, "Event"),
                      node(T::Shop, "Shop")}}};
        case App::Lesson::Fight:
            return {{"Fight", "Protect the core",
                     {text("Enemies walk to the core in the middle. Your balls hit them."),
                      key("Q", "Throw every ball at once.")}}};
        case App::Lesson::Recruit:
            return {{"Recruit", "Pick a card",
                     {text("Extra ball: one more ball. The rest upgrade your balls.")}}};
        case App::Lesson::Elite:
            return {{"Elite", "A tougher fight",
                     {text("More enemies. Win it for a strong item.")}}};
        case App::Lesson::Reward:
            return {{"Item", "Equip it",
                     {text("Take the card, then click a ball's slot."),
                      key("TAB", "See all your balls.")}}};
        case App::Lesson::Event:
            return {{"Event", "A ? stop",
                     {text("A stranger with deals. Take one, or walk away."),
                      text("Here: a free item, or a coin flip for gold. +" + std::to_string(cfg::tutorial::eventGold) +
                           " gold to start.")}}};
        case App::Lesson::Shop:
            return {{"Shop", "Spend your gold",
                     {text("Buy a card, then pick the ball."),
                      key("R", "New cards, for a little gold.")}}};
        case App::Lesson::Finale:
            return {{"Final", "The last fight",
                     {text("Hold the core until the last enemy falls.")}}};
        case App::Lesson::Cards:
            return {{"Cards", "What each card is",
                     {text("ITEM - fills an item slot. Two of one class give the ball that class."),
                      text("ABILITY - a power the ball fires by itself every few seconds."),
                      text("MODIFIER - a straight boost to one ball: harder, bigger, faster."),
                      text("ELEMENT - the ball's type: fire burns, ice freezes...")}}};
        case App::Lesson::Equip:
            return {{"Equip", "Give it to a ball",
                     {text("Click the ball that gets it:"),
                      text("Item - a bar under the ball."),
                      text("Ability - the diamond on its right."),
                      text("Element - the slot on its left."),
                      text("Modifier - anywhere on the ball."),
                      text("Right-click: back, if you change your mind.")}}};
        case App::Lesson::Count: break;
    }
    return {};
}

constexpr float kLessonW = 340.f;
constexpr float kLessonPad = 18.f;
constexpr float kLessonIconW = 56.f;    // the key cap / stop column
constexpr float kLessonLineH = 19.f;
constexpr float kLessonRowGap = 6.f;
constexpr float kLessonHead = 60.f;     // kicker + title
constexpr float kLessonFoot = 50.f;     // the button row
constexpr float kLessonTop = 112.f;    // under the map's status line / the fight's top bar
constexpr float kLessonMinShow = 0.35f; // a click sooner than this doesn't count

float rowTextW(const LessonRow& r) {
    return kLessonW - 2.f * kLessonPad - (r.kind == LessonRow::Key || r.kind == LessonRow::Node ? kLessonIconW : 0.f);
}
unsigned rowSize(const LessonRow&) { return theme::fsSmall; }

sf::FloatRect lessonRect(const sf::Font& font, const LessonPage& p, sf::Vector2f s) {
    float h = kLessonHead + kLessonFoot;
    for (const LessonRow& r : p.rows) {
        const float n = static_cast<float>(wrapText(font, r.text, rowSize(r), rowTextW(r)).size());
        h += std::max(n * kLessonLineH, r.kind == LessonRow::Node ? 24.f : 0.f) + kLessonRowGap;
    }
    // A box in the top right corner: the game stays in view.
    return {std::round(s.x - kLessonW - 20.f), kLessonTop, kLessonW, std::round(h)};
}

sf::FloatRect lessonButton(sf::FloatRect r) {
    return {r.left + r.width - kLessonPad - 80.f, r.top + r.height - kLessonPad - 26.f, 80.f, 28.f};
}

}  // namespace

void App::showLesson(Lesson l) {
    const std::uint32_t bit = 1u << static_cast<int>(l);
    if (lessonsSeen_ & bit) return;
    lessonsSeen_ |= bit;
    lesson_ = static_cast<int>(l);
    lessonPage_ = 0;
    lessonA_ = 0.f;
    lessonT_ = 0.f;
}

void App::updateLessons(float dt) {
    if (lesson_ >= 0) {
        if (!data_.run.active || stack_.empty()) {   // the run ended under it
            lesson_ = -1;
            return;
        }
        lessonA_ = std::min(1.f, lessonA_ + dt * 4.f);
        lessonT_ += dt;
        return;
    }
    if (!data_.run.active || !data_.run.tutorial || stack_.empty() || peek_.open || onPauseMenus()) return;
    const RunState& r = data_.run;
    const MapNodeType here = r.mapNode >= 0 && r.mapNode < static_cast<int>(r.map.nodes.size())
                                 ? r.map.nodes[static_cast<std::size_t>(r.mapNode)].type
                                 : MapNodeType::Combat;
    const Screen* top = stack_.back().get();
    // The drill stops the clock, so it needs a ball the player can grab now
    // (a Snare or a fresh throw would hold it for good).
    const bool grabbable = world_.heldBall() || std::any_of(world_.balls().begin(), world_.balls().end(), [](const Ball& c) {
        return c.regrabT <= 0.f && c.snaredBy < 0 && c.hexT <= 0.f;
    });
    if (drill_) {
        drillT_ += dt;
        if (!simulating() || !world_.waveRunning()) drill_ = false;   // the fight ended under it
        else if (!grabbable || world_.launching()) drill_ = drillDone_ = false;   // let the fight run, try again
        return;
    }
    if (dynamic_cast<const MapScreen*>(top)) {
        showLesson(Lesson::Map);
    } else if (simulating() && world_.waveRunning() && r.mapNode >= 0) {
        showLesson(here == MapNodeType::Boss ? Lesson::Finale : here == MapNodeType::Elite ? Lesson::Elite : Lesson::Fight);
        // (after the opening whirl: balls can't be grabbed while it spins, and the drill stops the clock)
        if (lesson_ < 0 && !drillDone_ && here == MapNodeType::Combat && grabbable && !world_.launching() &&
            (lessonsSeen_ >> static_cast<int>(Lesson::Fight) & 1u)) {   // the card closed: now try it
            drill_ = drillDone_ = true;
            drillClicked_ = false;
            drillT_ = 0.f;
        }
    } else if (dynamic_cast<const ChoiceScreen*>(top)) {
        if (here == MapNodeType::Recruit) showLesson(Lesson::Recruit);
        else if (here == MapNodeType::Elite) showLesson(Lesson::Reward);
        if (lesson_ < 0) showLesson(Lesson::Cards);   // the stop's card first, then the kinds of card
    } else if (dynamic_cast<const ShopScreen*>(top)) {
        showLesson(Lesson::Shop);
        if (lesson_ < 0) showLesson(Lesson::Cards);
    } else if (dynamic_cast<const EquipScreen*>(top)) {
        showLesson(Lesson::Equip);   // the first pick to place: where it goes
    }
}

bool App::lessonEvent(const sf::Event& e) {
    if (lesson_ < 0) {   // the release of the click that closed it isn't a throw
        if (lessonEatRelease_ && e.type == sf::Event::MouseButtonReleased) {
            lessonEatRelease_ = false;
            return true;
        }
        return false;
    }
    bool next = false;
    if (e.type == sf::Event::MouseButtonPressed && e.mouseButton.button == sf::Mouse::Left) {
        next = true;
        lessonEatRelease_ = true;
    }
    if (e.type == sf::Event::KeyPressed &&
        (e.key.code == sf::Keyboard::Space || e.key.code == sf::Keyboard::Enter || e.key.code == sf::Keyboard::Escape))
        next = true;
    if (next && lessonT_ >= kLessonMinShow) {
        audio_.uiClick();
        lessonT_ = 0.f;
        if (++lessonPage_ >= static_cast<int>(lessonPages(static_cast<Lesson>(lesson_)).size())) lesson_ = -1;
    }
    return true;
}

// The grab drill: the fight stands still until a slingshot throw. The play
// screen grabs and aims as always (a frozen world still grabs); keys wait.
bool App::drillEvent(const sf::Event& e) {
    if (!drill_) return false;
    if (e.type == sf::Event::KeyPressed) return e.key.code != sf::Keyboard::Escape;   // Esc: the pause menu
    if ((e.type == sf::Event::MouseButtonPressed || e.type == sf::Event::MouseButtonReleased) &&
        e.mouseButton.button != sf::Mouse::Left)
        return true;
    return false;
}

void App::drillThrown(bool aimed) {
    if (!drill_) return;
    if (!aimed) {   // a click: the ball stays in place (PlayScreen), the drill asks for a pull
        drillClicked_ = true;
        return;
    }
    drill_ = false;
    effects_.flash(theme::puGolden, 0.3f);
    effects_.addLabel("Nice throw!", {size().x * 0.5f, size().y * 0.3f}, theme::puGolden, 30, 1.2f);
}

void App::drawDrill(sf::RenderWindow& w) const {
    if (!drill_ || world_.balls().empty()) return;
    // The ball in hand, else the one nearest the core: a hole in the dark round it.
    const Ball* b = world_.heldBall();
    if (!b) {
        float best = 1e18f;
        for (const Ball& c : world_.balls()) {
            if (c.regrabT > 0.f || c.snaredBy >= 0 || c.hexT > 0.f) continue;   // not grabbable right now
            const sf::Vector2f d = c.pos - world_.core().pos;
            if (const float dd = d.x * d.x + d.y * d.y; dd < best) { best = dd; b = &c; }
        }
    }
    if (!b) return;
    const bool held = world_.heldBall() != nullptr;
    const float a = std::min(1.f, drillT_ * 3.f);
    const sf::Vector2f s = size();
    const sf::Vector2f c = worldToUi(b->pos);
    const float px = length(worldToUi(b->pos + sf::Vector2f{1.f, 0.f}) - c);   // UI units per arena unit
    // Held: a wide hole, so the slingshot's band and line show.
    const float hole = (held ? cfg::app::slingMaxPull * world_.arenaScale() * 1.25f : b->radius + 46.f) * px;
    const float far = length(s) * 2.f;
    const float dark = (held ? 0.3f : 0.5f) * a;
    draw::ring(w, c, (hole + far) * 0.5f, far - hole, withAlpha(theme::panel, dark), 0.f, kPi * 2.f, 96);
    for (int i = 0; i < 6; ++i) {   // a soft edge on the hole
        const float t = static_cast<float>(i) / 6.f;
        draw::ring(w, c, hole - 3.f * px - 6.f * t * px, 6.f * px, withAlpha(theme::panel, dark * (1.f - t) * 0.5f), 0.f,
                   kPi * 2.f, 96);
    }
    if (!held) {   // a ring that shrinks onto the ball, again and again: here
        const float p = std::fmod(drillT_ * 1.1f, 1.f);
        draw::ring(w, c, (b->radius + 10.f + 40.f * (1.f - p)) * px, 3.f,
                   withAlpha(theme::puGolden, a * (0.3f + 0.6f * p)), 0.f, kPi * 2.f, 48);
    }

    // What to do: a small box in the top right, like the lesson cards.
    std::string title, sub;
    if (!held) {
        title = drillClicked_ ? "Hold and drag" : "Grab the ball";
        sub = drillClicked_ ? "Don't just click: drag away." : "Hold the click on it.";
    } else {
        title = "Pull back, let go";
        sub = "Pull further: harder hit.";
    }
    const sf::FloatRect r{std::round(s.x - kLessonW - 20.f), kLessonTop, kLessonW, 72.f};
    draw::panel(w, r, theme::puGolden, a, 0.f);
    auto line = [&](const std::string& str, unsigned sz, sf::Color col, float y) {
        sf::Text t = makeText(font_, str, sz, withAlpha(col, a));
        const sf::FloatRect b = t.getLocalBounds();
        t.setOrigin(b.left, b.top);
        t.setPosition(std::round(r.left + kLessonPad), std::round(y));
        w.draw(t);
    };
    line(title, 20, theme::puGolden, r.top + 16.f);
    line(sub, theme::fsSmall, theme::textHi, r.top + 46.f);
}

void App::drawLesson(sf::RenderWindow& w) const {
    if (lesson_ < 0) return;
    const std::vector<LessonPage> pages = lessonPages(static_cast<Lesson>(lesson_));
    if (pages.empty()) return;
    const LessonPage& p = pages[static_cast<std::size_t>(std::clamp(lessonPage_, 0, static_cast<int>(pages.size()) - 1))];
    const sf::Vector2f s = size();
    const float a = lessonA_ * lessonA_ * (3.f - 2.f * lessonA_);

    const sf::FloatRect r = lessonRect(font_, p, s);
    draw::panel(w, r, theme::accent, a, 0.f);
    const float left = r.left + kLessonPad;

    // Kicker (where you are in the tutorial) and the title.
    drawLabel(w, font_, p.kicker, 12, {left, r.top + kLessonPad - 2.f}, withAlpha(theme::accent, a), -1);
    {
        sf::Text t = makeText(font_, p.title, 20, withAlpha(theme::textHi, a));
        const sf::FloatRect b = t.getLocalBounds();
        t.setOrigin(b.left, b.top);
        t.setPosition(std::round(left), std::round(r.top + kLessonPad + 12.f));
        w.draw(t);
    }

    float y = r.top + kLessonHead;
    for (const LessonRow& row : p.rows) {
        const auto lines = wrapText(font_, row.text, rowSize(row), rowTextW(row));
        const float blockH = std::max(static_cast<float>(lines.size()) * kLessonLineH, row.kind == LessonRow::Node ? 24.f : 0.f);
        float tx = left;
        if (row.kind == LessonRow::Key) {   // the key, as a cap, beside what it does
            const float cw = std::max(34.f, makeLabel(font_, row.key, 12, theme::textHi).getLocalBounds().width + 18.f);
            const sf::FloatRect cap{left, y + 1.f, cw, 24.f};
            draw::box(w, cap, 0.f, withAlpha(theme::accent, 0.22f * a), withAlpha(theme::accent, 0.1f * a),
                      withAlpha(theme::accent, 0.85f * a), 1.f);
            drawLabel(w, font_, row.key, 12, {cap.left + cap.width * 0.5f + 1.f, cap.top + cap.height * 0.5f},
                      withAlpha(theme::textHi, a));
            tx = left + kLessonIconW;
        } else if (row.kind == LessonRow::Node) {   // the stop, as on the map
            drawMapNode(w, font_, {left + 16.f, y + blockH * 0.5f}, row.node, 9.f, a);
            tx = left + kLessonIconW;
        }
        const sf::Color col = row.kind == LessonRow::Note ? theme::textLo : theme::textHi;
        float ly = y + (row.kind == LessonRow::Node ? (blockH - static_cast<float>(lines.size()) * kLessonLineH) * 0.5f : 0.f);
        for (const std::string& l : lines) {
            sf::Text t = makeText(font_, l, rowSize(row), withAlpha(col, a));
            const sf::FloatRect b = t.getLocalBounds();
            t.setOrigin(b.left, 0.f);
            t.setPosition(std::round(tx), std::round(ly + 2.f));
            w.draw(t);
            ly += kLessonLineH;
        }
        y += blockH + kLessonRowGap;
    }

    // The button row: page dots on the left, the button on the right.
    const sf::FloatRect btn = lessonButton(r);
    const bool last = lessonPage_ + 1 >= static_cast<int>(pages.size());
    const bool hot = btn.contains(uiMouse());
    draw::panel(w, btn, theme::accent, a, hot ? 1.f : 0.4f);
    drawLabel(w, font_, last ? "OK" : "Next", 13, {btn.left + btn.width * 0.5f, btn.top + btn.height * 0.5f},
              withAlpha(theme::textHi, a));
    const float cy = btn.top + btn.height * 0.5f;
    float dx = left + 4.f;
    if (pages.size() > 1) {
        for (int i = 0; i < static_cast<int>(pages.size()); ++i) {
            const bool on = i == lessonPage_;
            draw::polygon(w, {dx, cy}, on ? 4.5f : 3.5f, 4, 0.f, withAlpha(on ? theme::accent : theme::arenaEdge, a),
                          withAlpha(on ? theme::accent : theme::arenaEdge, a));
            dx += 14.f;
        }
        dx += 8.f;
    }
    drawLabel(w, font_, "click or space", 11, {dx - 4.f, cy}, withAlpha(theme::textDim, a), -1);
}

}  // namespace sb
