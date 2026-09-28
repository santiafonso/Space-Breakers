#pragma once

#include <string>
#include <vector>

#include "core/Math.hpp"
#include "core/Theme.hpp"
#include "progression/GameData.hpp"

namespace sb {

// Small shared drawing helpers used across every screen.
// Text at heading size and up is set in the title face when one is registered.
void setTitleFont(const sf::Font* font);
sf::Text makeText(const sf::Font& font, const std::string& str, unsigned size, sf::Color color);
void centerOrigin(sf::Text& t);

// The console's small caption style: uppercase, letter-spaced. Use for
// kickers, units and section headers - never for body text.
sf::Text makeLabel(const sf::Font& font, const std::string& str, unsigned size, sf::Color color);
// A label centred on `pos` (align 0), ending at `pos.x` (1) or starting at it (-1).
void drawLabel(sf::RenderTarget& t, const sf::Font& font, const std::string& str, unsigned size,
               sf::Vector2f pos, sf::Color color, int align = 0);

// A keyboard key cap ([TAB], [O]) for a shortcut. The caption saying what it
// does appears beside it only while `hot` (hovered): on the right when
// `captionRight`, else on the left. keyCapSize = the cap's box for `key`.
sf::Vector2f keyCapSize(const sf::Font& font, const std::string& key);
void drawKeyCap(sf::RenderWindow& w, const sf::Font& font, sf::FloatRect cap, const std::string& key,
                const std::string& caption, bool hot, bool captionRight = true);

// Draw a string centred on `pos`.
void drawCentered(sf::RenderWindow& window, const sf::Font& font, const std::string& str,
                  unsigned size, sf::Vector2f pos, sf::Color color);

// Eased "pop in" factor for an element that starts appearing `delay` seconds
// into a screen's intro and settles over `dur`. Overshoots slightly past 1 for
// a springy feel; clamp to [0,1] when driving alpha.
float introPop(float elapsed, float delay, float dur = 0.30f);

// Like drawCentered, but the text springs up to size and fades in as `pop`
// goes 0 -> ~1.1 (pass an introPop(...) value). A pop <= 0 draws nothing.
void drawCenteredPop(sf::RenderWindow& window, const sf::Font& font, const std::string& str,
                     unsigned size, sf::Vector2f pos, sf::Color color, float pop);

// Full-screen dim, used behind pause / shop overlays.
void drawDim(sf::RenderWindow& window, sf::Vector2f size, float alpha);

// The lifetime records panel (shared by the Stats screen). `intro` staggers the
// rows popping in; pass a large value for no animation.
void drawStatsPanel(sf::RenderWindow& window, const sf::Font& font, sf::Vector2f size,
                    const Stats& stats, float intro = 1e6f);

// Greedy word-wrap: break `str` into lines no wider than `maxW` at `size`.
std::vector<std::string> wrapText(const sf::Font& font, const std::string& str, unsigned size,
                                  float maxW);

// ---- ball loadout panels: one ball's look, classes, slots, modifiers ----
// The ball's identity sits in the header - its type as a circle on the left,
// its abilities as cyan diamonds on the right, the ball between - and its
// gear below: the 4 item slots as chips.
inline constexpr float kPanelW = 180.f;
inline constexpr float kPanelH = 272.f;
inline constexpr float kPanelGap = 16.f;
inline constexpr float kSlotH = 26.f;
inline constexpr float kSlotStep = 31.f;
inline constexpr float kPanelHeadY = 32.f;     // header row (type / ball / abilities), from the panel top
inline constexpr float kPanelItemsTop = 118.f; // first item slot's centre, from the panel top

// How much a row of n loadout panels can be magnified to use the screen
// (`reserve` = width kept for something beside the row): few balls, big panels.
float panelRowZoom(sf::Vector2f size, int n, float reserve = 0.f, float maxZoom = 1.2f);

// Centre of panel i of n in a row at height cy, the row centred on cx (the
// screen centre by default).
sf::Vector2f panelCenter(sf::Vector2f size, int i, int n, float cy, float cx = -1.f);
// Where slot s (0..3 items, kSlotType, kSlotAbility + i) sits on that ball's
// panel. The ability row splits into abilityBoxes(L) boxes.
sf::FloatRect slotRect(sf::Vector2f panelCentre, int slot, const BallLoadout& L);
int abilityBoxes(const BallLoadout& L);   // open ability slots + any filled closed ones
std::string modifierLine(const BallLoadout& L);
sf::Color catColor(UpgradeCat c);
sf::Color tagColor(ItemTag t);   // a class's colour (its items, its name)
sf::Color tierColor(Tier t);     // Common grey .. Legendary gold
// A pick's card frame: dark glass lit from the top by the tier colour, a tier
// band along the top edge and corner brackets; Epic / Legendary get a second
// set of brackets outside that breathes, so the rare ones jump out. `time`
// drives the pulse; `reveal` (0..1) snaps the brackets in as the card appears.
void drawTierFrame(sf::RenderWindow& w, sf::FloatRect r, Tier t, float hover, float alpha, float time,
                   float reveal = 1.f);
// A class item's card, over its tier frame: a class-coloured spine down the
// left edge and a faint wash of the class colour - a class is what's worth
// chasing, so its cards read louder than an element's. When taking it would
// give some ball its class (or ascend it), a chip on the bottom edge says so
// ("MAKES A STRIKER" / "ASCENDS: MEGA STRIKER"). No-op for untagged picks.
void drawClassCardMark(sf::RenderWindow& w, const sf::Font& font, sf::FloatRect r, UpgradeKind k,
                       const std::vector<BallLoadout>& balls, float alpha);

// A pick kind's small mark, the same wherever it's shown: item = square,
// ability = diamond, element = hexagon, relic = circle, modifier = triangle,
// new ball = ring. `size` is its radius.
void drawKindMark(sf::RenderTarget& w, UpgradeCat cat, sf::Vector2f pos, float size, sf::Color color);

// A whole pick card, with its three facts kept apart so they never blur:
// WHAT it is (item / relic / ability...) = a plain label top-left; HOW RARE =
// 1..5 pips (and the tier word when it fits) top-right plus the frame's edge;
// WHICH CLASS = a solid badge in the class colour under the title, with the
// class spine and wash. Then the description, above `bottomReserve`.
struct PickCardStyle {
    float hover = 0.f, alpha = 1.f, time = 0.f, reveal = 1.f;
    float pop = 1.f;              // the title's spring (an introPop value)
    float bottomReserve = 0.f;    // px kept free at the bottom (a price, a reroll strip)
    std::string note;             // a short line under the badge, e.g. "Lv 1 -> 2"
    bool classMark = true;        // the class spine / wash / "makes a" chip
    bool showWhat = true;         // the kind label (off where a group header already says it)
};
void drawPickCard(sf::RenderWindow& w, const sf::Font& font, sf::FloatRect r, UpgradeKind k,
                  const std::vector<BallLoadout>& balls, const PickCardStyle& st);

// What part of a loadout panel centred at `c` the pointer is on: a slot
// (0..kLoadoutSlots-1), kPanelPartBall = the ball / class name,
// kPanelPartMods = the modifier line, -1 = nothing.
inline constexpr int kPanelPartBall = 20;
inline constexpr int kPanelPartMods = 21;
int panelPartAt(sf::Vector2f c, sf::Vector2f mouse, const BallLoadout& L);
// Hover text for that part. False when there's nothing to say.
bool loadoutTooltip(const BallLoadout& L, int part, std::string& title, std::string& desc);

// A small hover box by the pointer: a title and a short wrapped description,
// kept on screen.
void drawTooltip(sf::RenderWindow& w, const sf::Font& font, sf::Vector2f mouse, sf::Vector2f screen,
                 const std::string& title, const std::string& desc, sf::Color titleColor = theme::textHi);
// `placing` = the UpgradeKind being equipped (-1 none): the slots that take it light up.
void drawLoadoutPanel(sf::RenderWindow& w, const sf::Font& font, sf::Vector2f c,
                      const BallLoadout& L, float alpha, float hover, int hoverSlot, bool dim, int placing = -1);

}  // namespace sb
