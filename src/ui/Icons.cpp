#include "ui/Icons.hpp"

#include "core/Math.hpp"
#include "core/Theme.hpp"
#include "progression/Offers.hpp"
#include "render/Draw.hpp"
#include "ui/Widgets.hpp"

#include <cmath>
#include <initializer_list>

namespace sb {

Glyph glyphOf(UpgradeKind k) {
    switch (k) {
        case UpgradeKind::AddBall:
            return Glyph::Plus;
        case UpgradeKind::ElemFire: case UpgradeKind::Afterburner: case UpgradeKind::Phoenix:
        case UpgradeKind::PlanetMars: case UpgradeKind::SunChariot: case UpgradeKind::WarChariot:
            return Glyph::Flame;
        case UpgradeKind::ElemPoison: case UpgradeKind::ToxicTrail: case UpgradeKind::Contagion:
        case UpgradeKind::AbilityCloud: case UpgradeKind::PlanetVenus:
            return Glyph::Bubbles;
        case UpgradeKind::ElemWater: case UpgradeKind::Seep: case UpgradeKind::Tincture: case UpgradeKind::Bleed:
        case UpgradeKind::Residue: case UpgradeKind::Saturate: case UpgradeKind::BloodRain:
            return Glyph::Drop;
        case UpgradeKind::ElemIce: case UpgradeKind::Shatter: case UpgradeKind::PlanetNeptune:
        case UpgradeKind::FrostWyrm:
            return Glyph::Frost;
        case UpgradeKind::ElemStone: case UpgradeKind::Ballast: case UpgradeKind::Bedrock: case UpgradeKind::Golem:
            return Glyph::Rock;
        case UpgradeKind::ElemElectric: case UpgradeKind::AbilityArc: case UpgradeKind::Conductor:
        case UpgradeKind::Tesla: case UpgradeKind::Resonance: case UpgradeKind::Storm: case UpgradeKind::Conflux:
        case UpgradeKind::Lightspeed: case UpgradeKind::IonCannon: case UpgradeKind::ChainReaction:
        case UpgradeKind::AbilitySmite: case UpgradeKind::Caduceus: case UpgradeKind::StormDrake:
            return Glyph::Bolt;
        case UpgradeKind::AbilityDash: case UpgradeKind::Swift: case UpgradeKind::Coil: case UpgradeKind::Momentum:
        case UpgradeKind::Strafe: case UpgradeKind::Wake: case UpgradeKind::AetherStep: case UpgradeKind::Quickstep:
        case UpgradeKind::Adrenaline: case UpgradeKind::AbilityTailwind: case UpgradeKind::PlanetMercury:
        case UpgradeKind::Talaria: case UpgradeKind::Messenger: case UpgradeKind::Wyrmling:
        case UpgradeKind::PaleHorse:
            return Glyph::Speed;
        case UpgradeKind::AbilityNova: case UpgradeKind::Overkill: case UpgradeKind::Bomber:
        case UpgradeKind::HairTrigger: case UpgradeKind::SmokeBomb: case UpgradeKind::Aftershock:
        case UpgradeKind::HeavyLanding: case UpgradeKind::Fission: case UpgradeKind::Martyr:
        case UpgradeKind::FragRounds: case UpgradeKind::LastLine: case UpgradeKind::Shrapnel:
        case UpgradeKind::LeadStorm: case UpgradeKind::Primal: case UpgradeKind::AbilityMines:
        case UpgradeKind::Eclipse:
            return Glyph::Blast;
        case UpgradeKind::AbilitySplit: case UpgradeKind::Echo: case UpgradeKind::SplitShot:
        case UpgradeKind::Mitosis: case UpgradeKind::Gemini: case UpgradeKind::Twincast:
        case UpgradeKind::Scattershot: case UpgradeKind::Phantom: case UpgradeKind::Relay: case UpgradeKind::HandOff:
        case UpgradeKind::Mirror: case UpgradeKind::Projection: case UpgradeKind::Formation:
        case UpgradeKind::TwinCall: case UpgradeKind::Gunship: case UpgradeKind::AbilitySwap:
        case UpgradeKind::CrescentMoon:
            return Glyph::Twin;
        case UpgradeKind::AbilityBulwark: case UpgradeKind::CoveringFire: case UpgradeKind::Plating:
        case UpgradeKind::Sentinel: case UpgradeKind::GuardDog: case UpgradeKind::ShieldBash:
        case UpgradeKind::WardRing: case UpgradeKind::DragonTeeth:
            return Glyph::Shield;
        case UpgradeKind::AbilityOverclock: case UpgradeKind::HeavyImpact: case UpgradeKind::Crucible:
        case UpgradeKind::Edge: case UpgradeKind::Bully: case UpgradeKind::GlassCannon:
        case UpgradeKind::AbilityEmpower:
            return Glyph::Burst;
        case UpgradeKind::AbilityMeteor: case UpgradeKind::Comet: case UpgradeKind::Muninn:
        case UpgradeKind::WrathOlympus:
            return Glyph::Meteor;
        case UpgradeKind::AbilityMissile: case UpgradeKind::Seeker: case UpgradeKind::ArcaneMissile:
        case UpgradeKind::SmartRounds: case UpgradeKind::PlanetEarth: case UpgradeKind::SilverBow:
        case UpgradeKind::ArrowRain:
            return Glyph::Missile;
        case UpgradeKind::BigBall: case UpgradeKind::Bumper: case UpgradeKind::Glutton: case UpgradeKind::Giant:
        case UpgradeKind::Bulk: case UpgradeKind::Juggernaut: case UpgradeKind::Titan:
        case UpgradeKind::AbilityShrink:
            return Glyph::Size;
        case UpgradeKind::Keen: case UpgradeKind::Crit: case UpgradeKind::TrickShot: case UpgradeKind::Reagent:
        case UpgradeKind::Opportunist: case UpgradeKind::HollowPoint: case UpgradeKind::FirstBlood:
        case UpgradeKind::Marksman: case UpgradeKind::PerfectRelease: case UpgradeKind::Primed:
            return Glyph::Crit;
        case UpgradeKind::Reach: case UpgradeKind::HeavyThrow: case UpgradeKind::LongPull:
        case UpgradeKind::StrongArm:
            return Glyph::Sling;
        case UpgradeKind::Tempered: case UpgradeKind::Attune: case UpgradeKind::Elixir: case UpgradeKind::Catalyst:
            return Glyph::Flask;
        case UpgradeKind::Spin: case UpgradeKind::Berserk: case UpgradeKind::KillingSpree:
        case UpgradeKind::CatchRelease: case UpgradeKind::HotStreak: case UpgradeKind::PackLeader:
        case UpgradeKind::ComboHit: case UpgradeKind::Minigun: case UpgradeKind::Overcharge:
            return Glyph::Combo;
        case UpgradeKind::Leech: case UpgradeKind::Mender: case UpgradeKind::Distill: case UpgradeKind::Medic:
        case UpgradeKind::Lyre:
            return Glyph::Heal;
        case UpgradeKind::QuickMind: case UpgradeKind::Focus: case UpgradeKind::ManaSpring: case UpgradeKind::Flux:
        case UpgradeKind::LuckyBreak: case UpgradeKind::SecondWind: case UpgradeKind::Sync:
        case UpgradeKind::ArcaneCharge: case UpgradeKind::ChainKill: case UpgradeKind::Transmute:
        case UpgradeKind::Cascade:
            return Glyph::Recharge;
        case UpgradeKind::Bouncy: case UpgradeKind::Ricochet: case UpgradeKind::Boomerang: case UpgradeKind::Rebound:
        case UpgradeKind::ChaosBounce: case UpgradeKind::BankShot: case UpgradeKind::AmmoFeed:
        case UpgradeKind::PingPong: case UpgradeKind::Carom: case UpgradeKind::SkippingStone:
        case UpgradeKind::Homecoming: case UpgradeKind::CoreSpring: case UpgradeKind::LokiPachinko:
            return Glyph::Bounce;
        case UpgradeKind::Cleave: case UpgradeKind::Piercing: case UpgradeKind::DrillRounds: case UpgradeKind::Blur:
        case UpgradeKind::Gungnir: case UpgradeKind::TrojanWar:
            return Glyph::Pierce;
        case UpgradeKind::Executioner: case UpgradeKind::Cull: case UpgradeKind::ExecutionThrow:
        case UpgradeKind::DeathSentence: case UpgradeKind::Reaper: case UpgradeKind::BlackPlague:
        case UpgradeKind::DeathMark: case UpgradeKind::Devourer: case UpgradeKind::BlackDeath:
            return Glyph::Skull;
        case UpgradeKind::Rampart: case UpgradeKind::Plow: case UpgradeKind::Pass:
            return Glyph::Shove;
        case UpgradeKind::Hunter: case UpgradeKind::Ambush: case UpgradeKind::Lurk: case UpgradeKind::MarkForDeath:
        case UpgradeKind::Spotter: case UpgradeKind::Predator: case UpgradeKind::Flicker: case UpgradeKind::Contract:
        case UpgradeKind::AbilityWarp: case UpgradeKind::Huginn: case UpgradeKind::Oracle:
        case UpgradeKind::HuntNight:
            return Glyph::Eye;
        case UpgradeKind::Tether: case UpgradeKind::Railgun: case UpgradeKind::AbilityBeam:
            return Glyph::Beam;
        case UpgradeKind::BlackHole: case UpgradeKind::Maelstrom: case UpgradeKind::AbilityVortex:
            return Glyph::Vortex;
        case UpgradeKind::GravityWell: case UpgradeKind::Grip: case UpgradeKind::Beckon: case UpgradeKind::Anchor:
        case UpgradeKind::Decoy: case UpgradeKind::MagneticCore: case UpgradeKind::AbilityHook:
            return Glyph::Magnet;
        case UpgradeKind::Satellite: case UpgradeKind::SummonWarden: case UpgradeKind::AbilityPortal:
        case UpgradeKind::AbilityLeash: case UpgradeKind::PhobosDeimos:
            return Glyph::Orbit;
        case UpgradeKind::Midas: case UpgradeKind::CoinFlip: case UpgradeKind::Pickpocket:
        case UpgradeKind::HouseEdge: case UpgradeKind::Aurum: case UpgradeKind::AbilityBounty:
        case UpgradeKind::AncientGold: case UpgradeKind::LokiHighTable:
            return Glyph::Coin;
        case UpgradeKind::Attunement: case UpgradeKind::Leyline: case UpgradeKind::Sigil: case UpgradeKind::Conduit:
        case UpgradeKind::Spellweave: case UpgradeKind::SpellChorus: case UpgradeKind::Archmage:
        case UpgradeKind::Runes: case UpgradeKind::Draupnir:
            return Glyph::Rune;
        case UpgradeKind::RapidFire: case UpgradeKind::Tracer:
            return Glyph::Bullets;
        case UpgradeKind::Backstab: case UpgradeKind::ShadowTrail: case UpgradeKind::SummonSword:
        case UpgradeKind::Spellblade: case UpgradeKind::VenomBlade: case UpgradeKind::RazorWire:
        case UpgradeKind::AbilityBlades: case UpgradeKind::ShadowStep: case UpgradeKind::ReaperScythe:
            return Glyph::Blade;
        case UpgradeKind::SummonTurret: case UpgradeKind::SummonTotem: case UpgradeKind::DropTurret:
            return Glyph::Turret;
        case UpgradeKind::SummonWisps: case UpgradeKind::Kennel: case UpgradeKind::Lingering:
        case UpgradeKind::Hatchery: case UpgradeKind::Soulbind: case UpgradeKind::SpiritHost:
            return Glyph::Ghost;
        case UpgradeKind::LuckyCharm: case UpgradeKind::Reroll: case UpgradeKind::HighRoller:
        case UpgradeKind::LuckySeven: case UpgradeKind::LuckyNumber: case UpgradeKind::Rigged:
        case UpgradeKind::LuckyClover: case UpgradeKind::Roulette: case UpgradeKind::LokiDice:
        case UpgradeKind::LokiRoulette:
            return Glyph::Dice;
        case UpgradeKind::WildCard: case UpgradeKind::Sleight: case UpgradeKind::WildMagic:
        case UpgradeKind::LokiCards:
            return Glyph::Card;
        case UpgradeKind::Jackpot: case UpgradeKind::Beacon: case UpgradeKind::PrismHit: case UpgradeKind::Confetti:
        case UpgradeKind::Anthem: case UpgradeKind::Fortune: case UpgradeKind::Slot777: case UpgradeKind::Showtime:
        case UpgradeKind::PrismCore: case UpgradeKind::Cocoon: case UpgradeKind::StarryNight:
        case UpgradeKind::Petasos: case UpgradeKind::Laurel: case UpgradeKind::GoldenStag:
            return Glyph::Star;
        case UpgradeKind::Slug: case UpgradeKind::Meditate: case UpgradeKind::Volatile: case UpgradeKind::Insight:
        case UpgradeKind::Brace: case UpgradeKind::SuppressingFire: case UpgradeKind::WindUp:
        case UpgradeKind::Chronomancy: case UpgradeKind::TimeStop: case UpgradeKind::SteadyHand:
        case UpgradeKind::CoreSlowField: case UpgradeKind::TimeDilation: case UpgradeKind::Quicksilver:
        case UpgradeKind::FinalHour:
            return Glyph::Clock;
        case UpgradeKind::BabyDragon: case UpgradeKind::Hive: case UpgradeKind::ElderDragon:
        case UpgradeKind::LizardHorde: case UpgradeKind::BroodMother: case UpgradeKind::RatTide:
        case UpgradeKind::Locusts: case UpgradeKind::SilverHounds:
            return Glyph::Claw;
        case UpgradeKind::PlanetJupiter: case UpgradeKind::PlanetSaturn:
            return Glyph::Planet;
    }
    return Glyph::Burst;
}

sf::Color iconColor(UpgradeKind k) {
    const UpgradeCat cat = upgradeCat(k);
    if (cat == UpgradeCat::Element) return elementColor(static_cast<Element>(elementItemSlot(k) + 1));
    if (const ItemTag t = itemTag(k); t != ItemTag::None) return tagColor(t);
    return catColor(cat);
}

namespace {

// A glyph is drawn on a [-1, 1] square around its centre, strokes a fixed
// share of its size, so it reads the same in a slot and on a card.
struct Pen {
    sf::RenderTarget& t;
    sf::Vector2f c;
    float r;
    sf::Color col;
    float th;

    sf::Vector2f p(float x, float y) const { return c + sf::Vector2f{x * r, y * r}; }
    void line(float x1, float y1, float x2, float y2, float k = 1.f) const {
        draw::line(t, p(x1, y1), p(x2, y2), th * k, col);
        draw::disc(t, p(x1, y1), th * k * 0.5f, col, col, {1.f, 1.f}, 10);   // round caps
        draw::disc(t, p(x2, y2), th * k * 0.5f, col, col, {1.f, 1.f}, 10);
    }
    void ring(float x, float y, float rad, float a0 = 0.f, float a1 = 2.f * kPi, float k = 1.f) const {
        draw::ring(t, p(x, y), rad * r, th * k, col, a0, a1, 32);
    }
    void dot(float x, float y, float rad, sf::Color cc) const { draw::disc(t, p(x, y), rad * r, cc, cc, {1.f, 1.f}, 20); }
    void dot(float x, float y, float rad) const { dot(x, y, rad, col); }
    void poly(float x, float y, float rad, int sides, float rot) const { draw::polygon(t, p(x, y), rad * r, sides, rot, col, col); }
    void fill(std::initializer_list<sf::Vector2f> pts) const {
        sf::ConvexShape s(pts.size());
        std::size_t i = 0;
        for (const sf::Vector2f q : pts) s.setPoint(i++, p(q.x, q.y));
        s.setFillColor(col);
        t.draw(s);
    }
};

}  // namespace

void drawGlyph(sf::RenderTarget& t, Glyph g, sf::Vector2f c, float r, sf::Color col) {
    const Pen d{t, c, r, col, r * 0.17f};
    const sf::Color hole = withAlpha(theme::bg, col.a / 255.f);   // eyes, cut-outs
    switch (g) {
        case Glyph::Plus:
            d.line(-0.65f, 0.f, 0.65f, 0.f, 1.4f);
            d.line(0.f, -0.65f, 0.f, 0.65f, 1.4f);
            break;
        case Glyph::Flame:
            d.dot(0.f, 0.3f, 0.45f);
            d.fill({{-0.43f, 0.2f}, {0.f, -0.85f}, {0.43f, 0.2f}});
            d.fill({{-0.45f, 0.25f}, {-0.45f, -0.35f}, {-0.1f, 0.f}});
            d.fill({{0.45f, 0.25f}, {0.45f, -0.3f}, {0.1f, 0.f}});
            d.dot(0.f, 0.4f, 0.17f, hole);
            break;
        case Glyph::Bubbles:
            d.ring(-0.28f, 0.28f, 0.34f, 0.f, 2.f * kPi, 0.8f);
            d.ring(0.36f, 0.12f, 0.24f, 0.f, 2.f * kPi, 0.8f);
            d.ring(-0.05f, -0.5f, 0.18f, 0.f, 2.f * kPi, 0.8f);
            break;
        case Glyph::Drop:
            d.dot(0.f, 0.25f, 0.48f);
            d.fill({{-0.45f, 0.1f}, {0.f, -0.85f}, {0.45f, 0.1f}});
            break;
        case Glyph::Frost:
            for (int i = 0; i < 3; ++i) {
                const float a = kPi / 2.f + static_cast<float>(i) * kPi / 3.f;
                const float x = std::cos(a) * 0.75f, y = std::sin(a) * 0.75f;
                d.line(-x, -y, x, y);
            }
            d.dot(0.f, 0.f, 0.16f);
            break;
        case Glyph::Rock:
            draw::polygon(t, d.p(0.f, 0.05f), 0.72f * r, 5, 0.3f, col, col);
            d.line(-0.25f, -0.55f, 0.f, -0.05f, 0.6f);
            d.line(0.f, -0.05f, 0.3f, 0.3f, 0.6f);
            draw::line(t, d.p(-0.25f, -0.55f), d.p(0.f, -0.05f), d.th * 0.55f, hole);
            draw::line(t, d.p(0.f, -0.05f), d.p(0.3f, 0.3f), d.th * 0.55f, hole);
            break;
        case Glyph::Bolt:
            d.fill({{0.3f, -0.9f}, {-0.45f, 0.1f}, {0.f, 0.1f}});
            d.fill({{0.f, -0.1f}, {0.45f, -0.1f}, {-0.3f, 0.9f}});
            break;
        case Glyph::Speed:
            for (const float dx : {-0.3f, 0.3f}) {
                d.line(dx - 0.3f, -0.55f, dx + 0.2f, 0.f, 1.2f);
                d.line(dx + 0.2f, 0.f, dx - 0.3f, 0.55f, 1.2f);
            }
            break;
        case Glyph::Blast:
            d.dot(0.f, 0.f, 0.3f);
            for (int i = 0; i < 8; ++i) {
                const float a = static_cast<float>(i) * kPi / 4.f;
                d.line(std::cos(a) * 0.5f, std::sin(a) * 0.5f, std::cos(a) * 0.82f, std::sin(a) * 0.82f, 0.8f);
            }
            break;
        case Glyph::Twin:
            d.ring(-0.28f, 0.f, 0.42f);
            d.ring(0.28f, 0.f, 0.42f);
            break;
        case Glyph::Shield:
            d.fill({{-0.6f, -0.65f}, {0.6f, -0.65f}, {0.6f, 0.f}, {0.f, 0.8f}, {-0.6f, 0.f}});
            d.fill({{-0.35f, -0.42f}, {0.f, -0.42f}, {0.f, 0.45f}, {-0.35f, 0.05f}});
            draw::line(t, d.p(0.f, -0.65f), d.p(0.f, 0.8f), d.th * 0.5f, hole);
            break;
        case Glyph::Burst:
            d.fill({{0.f, -0.9f}, {0.2f, -0.2f}, {0.9f, 0.f}, {0.2f, 0.2f}, {0.f, 0.9f}, {-0.2f, 0.2f}, {-0.9f, 0.f},
                    {-0.2f, -0.2f}});
            break;
        case Glyph::Meteor:
            d.dot(0.28f, 0.28f, 0.36f);
            d.line(-0.75f, -0.4f, -0.05f, 0.3f, 0.7f);
            d.line(-0.4f, -0.75f, 0.3f, -0.05f, 0.7f);
            d.line(-0.7f, -0.7f, 0.05f, 0.05f, 0.9f);
            break;
        case Glyph::Missile:
            d.ring(0.65f, 0.65f, 0.95f, kPi, 1.5f * kPi);
            d.dot(0.65f, -0.3f, 0.24f);
            break;
        case Glyph::Size:
            d.dot(0.f, 0.f, 0.32f);
            for (int i = 0; i < 4; ++i) {
                const float a = kPi / 4.f + static_cast<float>(i) * kPi / 2.f;
                const float cx = std::cos(a), sy = std::sin(a);
                d.line(cx * 0.55f, sy * 0.55f, cx * 0.85f, sy * 0.85f);
                d.line(cx * 0.85f, sy * 0.85f, cx * 0.85f - cx * 0.3f, sy * 0.85f, 0.8f);
                d.line(cx * 0.85f, sy * 0.85f, cx * 0.85f, sy * 0.85f - sy * 0.3f, 0.8f);
            }
            break;
        case Glyph::Crit:
            d.ring(0.f, 0.f, 0.55f, 0.f, 2.f * kPi, 0.8f);
            d.line(0.f, -0.88f, 0.f, -0.3f, 0.8f);
            d.line(0.f, 0.3f, 0.f, 0.88f, 0.8f);
            d.line(-0.88f, 0.f, -0.3f, 0.f, 0.8f);
            d.line(0.3f, 0.f, 0.88f, 0.f, 0.8f);
            d.dot(0.f, 0.f, 0.12f);
            break;
        case Glyph::Sling:
            d.line(0.f, 0.85f, 0.f, 0.1f, 1.3f);
            d.line(0.f, 0.1f, -0.5f, -0.6f, 1.2f);
            d.line(0.f, 0.1f, 0.5f, -0.6f, 1.2f);
            d.line(-0.5f, -0.6f, 0.f, -0.15f, 0.5f);
            d.line(0.5f, -0.6f, 0.f, -0.15f, 0.5f);
            d.dot(0.f, -0.2f, 0.15f);
            break;
        case Glyph::Flask:
            d.line(-0.18f, -0.78f, 0.18f, -0.78f, 0.9f);
            d.line(-0.13f, -0.78f, -0.13f, -0.2f, 0.9f);
            d.line(0.13f, -0.78f, 0.13f, -0.2f, 0.9f);
            d.fill({{-0.15f, -0.3f}, {0.15f, -0.3f}, {0.62f, 0.72f}, {-0.62f, 0.72f}});
            draw::line(t, d.p(-0.38f, 0.25f), d.p(0.38f, 0.25f), d.th * 0.5f, hole);
            break;
        case Glyph::Combo:
            d.line(-0.5f, 0.7f, -0.5f, 0.35f, 1.5f);
            d.line(0.f, 0.7f, 0.f, -0.05f, 1.5f);
            d.line(0.5f, 0.7f, 0.5f, -0.6f, 1.5f);
            break;
        case Glyph::Heal:
            d.line(0.f, -0.62f, 0.f, 0.62f, 2.1f);
            d.line(-0.62f, 0.f, 0.62f, 0.f, 2.1f);
            break;
        case Glyph::Recharge: {
            const float a0 = 0.15f * kPi, a1 = 1.75f * kPi;
            d.ring(0.f, 0.f, 0.58f, a0, a1);
            const float ax = std::cos(a0) * 0.58f, ay = std::sin(a0) * 0.58f;
            const float tx = std::sin(a0), ty = -std::cos(a0);   // backwards along the arc
            d.fill({{ax + tx * 0.35f, ay + ty * 0.35f}, {ax - std::cos(a0) * 0.3f, ay - std::sin(a0) * 0.3f},
                    {ax + std::cos(a0) * 0.3f, ay + std::sin(a0) * 0.3f}});
            break;
        }
        case Glyph::Bounce:
            d.line(-0.85f, 0.72f, 0.85f, 0.72f, 0.7f);
            d.line(-0.7f, -0.6f, 0.f, 0.4f);
            d.line(0.f, 0.4f, 0.55f, -0.45f);
            d.dot(0.62f, -0.55f, 0.2f);
            break;
        case Glyph::Pierce:
            d.line(-0.05f, -0.7f, -0.05f, 0.7f, 0.6f);
            d.line(-0.85f, 0.f, 0.45f, 0.f, 1.1f);
            d.fill({{0.88f, 0.f}, {0.35f, -0.35f}, {0.35f, 0.35f}});
            break;
        case Glyph::Skull:
            d.dot(0.f, -0.12f, 0.6f);
            d.line(-0.3f, 0.5f, 0.3f, 0.5f, 2.2f);
            d.dot(-0.24f, -0.12f, 0.17f, hole);
            d.dot(0.24f, -0.12f, 0.17f, hole);
            draw::line(t, d.p(0.f, 0.38f), d.p(0.f, 0.72f), d.th * 0.45f, hole);
            break;
        case Glyph::Shove:
            d.ring(-0.5f, 0.f, 0.3f, 0.f, 2.f * kPi, 0.8f);
            d.line(-0.1f, 0.f, 0.5f, 0.f, 1.2f);
            d.fill({{0.9f, 0.f}, {0.4f, -0.4f}, {0.4f, 0.4f}});
            break;
        case Glyph::Eye:
            d.ring(0.f, 0.6f, 0.95f, -0.79f * kPi, -0.21f * kPi);
            d.ring(0.f, -0.6f, 0.95f, 0.21f * kPi, 0.79f * kPi);
            d.dot(0.f, 0.f, 0.24f);
            break;
        case Glyph::Beam:
            d.dot(-0.62f, 0.f, 0.26f);
            d.line(-0.45f, 0.f, 0.85f, 0.f, 1.5f);
            d.line(-0.25f, -0.32f, 0.8f, -0.32f, 0.45f);
            d.line(-0.25f, 0.32f, 0.8f, 0.32f, 0.45f);
            break;
        case Glyph::Vortex:
            d.ring(0.f, 0.f, 0.72f, 0.f, 1.3f * kPi, 0.9f);
            d.ring(0.f, 0.f, 0.48f, 0.5f * kPi, 1.9f * kPi, 0.9f);
            d.ring(0.f, 0.f, 0.24f, 1.1f * kPi, 2.5f * kPi, 0.9f);
            break;
        case Glyph::Magnet:
            d.ring(0.f, 0.05f, 0.45f, 0.f, kPi, 1.8f);
            d.line(-0.45f, 0.05f, -0.45f, -0.55f, 1.8f);
            d.line(0.45f, 0.05f, 0.45f, -0.55f, 1.8f);
            draw::line(t, d.p(-0.6f, -0.4f), d.p(-0.3f, -0.4f), d.th * 0.5f, hole);
            draw::line(t, d.p(0.3f, -0.4f), d.p(0.6f, -0.4f), d.th * 0.5f, hole);
            break;
        case Glyph::Orbit:
            d.ring(0.f, 0.f, 0.66f, 0.f, 2.f * kPi, 0.55f);
            d.dot(0.f, 0.f, 0.24f);
            d.dot(0.47f, -0.47f, 0.17f);
            break;
        case Glyph::Coin:
            d.ring(0.f, 0.f, 0.62f, 0.f, 2.f * kPi, 1.1f);
            d.ring(0.f, 0.f, 0.3f, 0.f, 2.f * kPi, 0.7f);
            break;
        case Glyph::Rune:
            draw::polygonOutline(t, c, 0.8f * r, 4, 0.f, d.th, col);
            d.line(0.f, -0.42f, 0.f, 0.42f, 0.8f);
            d.line(-0.25f, -0.1f, 0.25f, -0.1f, 0.8f);
            break;
        case Glyph::Bullets:
            d.line(-0.3f, 0.f, 0.2f, 0.f, 2.6f);
            d.fill({{0.2f, -0.22f}, {0.75f, 0.f}, {0.2f, 0.22f}});
            d.line(-0.88f, -0.3f, -0.6f, -0.3f, 0.7f);
            d.line(-0.88f, 0.3f, -0.6f, 0.3f, 0.7f);
            break;
        case Glyph::Blade:
            d.fill({{0.8f, -0.8f}, {0.55f, -0.25f}, {-0.25f, 0.55f}, {-0.4f, 0.4f}, {0.25f, -0.55f}});
            d.line(-0.55f, 0.15f, -0.15f, 0.55f, 0.9f);
            d.line(-0.35f, 0.35f, -0.65f, 0.65f, 0.9f);
            break;
        case Glyph::Turret:
            d.line(-0.6f, 0.65f, 0.6f, 0.65f, 1.4f);
            d.dot(0.f, 0.2f, 0.38f);
            d.line(0.f, 0.15f, 0.55f, -0.6f, 1.3f);
            break;
        case Glyph::Ghost:
            d.dot(0.f, -0.15f, 0.5f);
            d.fill({{-0.5f, -0.15f}, {0.5f, -0.15f}, {0.5f, 0.75f}, {0.25f, 0.5f}, {0.f, 0.75f}, {-0.25f, 0.5f},
                    {-0.5f, 0.75f}});
            d.dot(-0.18f, -0.12f, 0.11f, hole);
            d.dot(0.18f, -0.12f, 0.11f, hole);
            break;
        case Glyph::Dice:
            draw::polygonOutline(t, c, 0.85f * r, 4, kPi / 4.f, d.th * 0.8f, col);
            d.dot(-0.28f, -0.28f, 0.12f);
            d.dot(0.f, 0.f, 0.12f);
            d.dot(0.28f, 0.28f, 0.12f);
            break;
        case Glyph::Card: {
            const float s = std::sin(0.26f), co = std::cos(0.26f);
            auto rot = [&](float x, float y) { return sf::Vector2f{x * co - y * s, x * s + y * co}; };
            const sf::Vector2f a = rot(-0.45f, -0.7f), b = rot(0.45f, -0.7f), e = rot(0.45f, 0.7f), f = rot(-0.45f, 0.7f);
            d.line(a.x, a.y, b.x, b.y, 0.8f);
            d.line(b.x, b.y, e.x, e.y, 0.8f);
            d.line(e.x, e.y, f.x, f.y, 0.8f);
            d.line(f.x, f.y, a.x, a.y, 0.8f);
            d.poly(0.f, 0.f, 0.28f, 4, 0.26f);
            break;
        }
        case Glyph::Star:
            for (int i = 0; i < 5; ++i) {
                const float a = -kPi / 2.f + static_cast<float>(i) * 2.f * kPi / 5.f;
                const float l = a - kPi / 5.f, rr = a + kPi / 5.f;
                d.fill({{std::cos(a) * 0.9f, std::sin(a) * 0.9f}, {std::cos(rr) * 0.36f, std::sin(rr) * 0.36f},
                        {0.f, 0.f}, {std::cos(l) * 0.36f, std::sin(l) * 0.36f}});
            }
            break;
        case Glyph::Clock:
            d.ring(0.f, 0.f, 0.66f, 0.f, 2.f * kPi, 0.9f);
            d.line(0.f, 0.f, 0.f, -0.42f, 0.9f);
            d.line(0.f, 0.f, 0.3f, 0.12f, 0.9f);
            break;
        case Glyph::Claw:
            for (int i = 0; i < 3; ++i) {
                const float x = -0.45f + static_cast<float>(i) * 0.45f;
                d.fill({{x + 0.18f, -0.8f}, {x + 0.08f, 0.75f}, {x - 0.12f, 0.f}});
            }
            break;
        case Glyph::Planet:
            d.dot(0.f, 0.f, 0.4f);
            d.line(-0.85f, 0.3f, 0.85f, -0.3f, 0.7f);
            draw::line(t, d.p(-0.33f, 0.12f), d.p(0.33f, -0.12f), d.th * 0.7f, col);
            break;
    }
}

void drawIcon(sf::RenderTarget& t, UpgradeKind k, sf::Vector2f c, float r, float alpha) {
    const sf::Color col = iconColor(k);
    const sf::Color edge = withAlpha(col, alpha);
    const sf::Color fill = withAlpha(lerpColor(theme::bg, col, 0.16f), alpha);
    const sf::Color ink = withAlpha(lerpColor(col, sf::Color::White, 0.35f), alpha);
    const float th = std::max(1.f, r * 0.08f);
    float g = 0.7f;   // the glyph, a share of the frame
    sf::Vector2f gc = c;
    switch (upgradeCat(k)) {
        case UpgradeCat::Item:
            draw::box(t, {c.x - r, c.y - r, 2.f * r, 2.f * r}, r * 0.22f, fill, fill, edge, th);
            break;
        case UpgradeCat::Ability:
            draw::polygon(t, c, r * 1.3f, 4, 0.f, fill, fill);
            draw::polygonOutline(t, c, r * 1.3f, 4, 0.f, th, edge);
            g = 0.62f;
            break;
        case UpgradeCat::Element:
            draw::polygon(t, c, r * 1.12f, 6, kPi / 6.f, fill, fill);
            draw::polygonOutline(t, c, r * 1.12f, 6, kPi / 6.f, th, edge);
            break;
        case UpgradeCat::Relic:
            draw::disc(t, c, r * 1.05f, fill, fill, {1.f, 1.f}, 40);
            draw::ring(t, c, r * 1.05f, th, edge);
            break;
        case UpgradeCat::Modifier:
            draw::polygon(t, c + sf::Vector2f{0.f, r * 0.2f}, r * 1.4f, 3, -kPi / 2.f, fill, fill);
            draw::polygonOutline(t, c + sf::Vector2f{0.f, r * 0.2f}, r * 1.4f, 3, -kPi / 2.f, th, edge);
            g = 0.5f;
            gc.y += r * 0.24f;   // the triangle's middle, a hair under its centre
            break;
        case UpgradeCat::NewBall:
            draw::ring(t, c, r, th, edge);
            break;
    }
    drawGlyph(t, glyphOf(k), gc, r * g, ink);
}

}  // namespace sb
