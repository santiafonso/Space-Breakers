#include "sim/Entities.hpp"

#include <algorithm>
#include <cmath>

namespace sb {

const char* powerUpName(PowerUp p) {
    switch (p) {
        case PowerUp::Points2x:  return "DOUBLE POINTS";
        case PowerUp::SlowMo:    return "SLOW MOTION";
        case PowerUp::Surge:     return "SPEED SURGE";
        case PowerUp::Golden:    return "GOLDEN BOUNCE";
        case PowerUp::Overdrive: return "OVERDRIVE";
    }
    return "";
}

const char* powerUpDesc(PowerUp p) {
    switch (p) {
        case PowerUp::Points2x:  return "kills score double";
        case PowerUp::SlowMo:    return "enemies move slower";
        case PowerUp::Surge:     return "your balls cruise much faster";
        case PowerUp::Golden:    return "the damage combo climbs faster";
        case PowerUp::Overdrive: return "your balls hit twice as hard";
    }
    return "";
}

sf::Color powerUpColor(PowerUp p) {
    switch (p) {
        case PowerUp::Points2x:  return theme::puPoints;
        case PowerUp::SlowMo:    return theme::puSlow;
        case PowerUp::Surge:     return theme::puSurge;
        case PowerUp::Golden:    return theme::puGolden;
        case PowerUp::Overdrive: return theme::puOverdrive;
    }
    return theme::accent;
}

float powerUpDuration(PowerUp p) {
    switch (p) {
        case PowerUp::Points2x:  return cfg::powerup::durPoints2x;
        case PowerUp::SlowMo:    return cfg::powerup::durSlowMo;
        case PowerUp::Surge:     return cfg::powerup::durSurge;
        case PowerUp::Golden:    return cfg::powerup::durGolden;
        case PowerUp::Overdrive: return cfg::powerup::durOverdrive;
    }
    return 6.f;
}

const char* elementName(Element e) {
    switch (e) {
        case Element::Plain:    return "Plain";
        case Element::Fire:     return "Fire";
        case Element::Poison:   return "Poison";
        case Element::Water:    return "Water";
        case Element::Ice:      return "Ice";
        case Element::Stone:    return "Stone";
        case Element::Electric: return "Electric";
    }
    return "Plain";
}

sf::Color elementColor(Element e) {
    switch (e) {
        case Element::Plain:    return theme::textLo;   // neutral grey - see theme::speedColor
        case Element::Fire:     return theme::elemFire;
        case Element::Poison:   return theme::elemPoison;
        case Element::Water:    return theme::elemWater;
        case Element::Ice:      return theme::elemIce;
        case Element::Stone:    return theme::elemStone;
        case Element::Electric: return theme::elemElectric;
    }
    return theme::ballMid;
}

const char* enemyName(EnemyKind k) {
    switch (k) {
        case EnemyKind::Grunt:    return "Grunt";
        case EnemyKind::Runner:   return "Runner";
        case EnemyKind::Tank:     return "Tank";
        case EnemyKind::Splitter: return "Splitter";
        case EnemyKind::Shard:    return "Shard";
        case EnemyKind::Shielded: return "Shielded";
    }
    return "";
}

const char* enemyDesc(EnemyKind k) {
    switch (k) {
        case EnemyKind::Grunt:    return "walks straight at the core";
        case EnemyKind::Runner:   return "fast and fragile - ice and support marks catch it";
        case EnemyKind::Tank:     return "slow, very tough, hard to push; hits the core twice as hard";
        case EnemyKind::Splitter: return "bursts into two shards when it dies";
        case EnemyKind::Shard:    return "a splitter's fragment - weak but quick";
        case EnemyKind::Shielded: return "its shield faces the core and blocks hits - strike it from the side or behind";
    }
    return "";
}

// Class names and texts. Each class agent owns its own line in each switch.
const char* roleName(BallRole r) {
    switch (r) {
        case BallRole::Normal:   return "Normal";
        case BallRole::Striker:  return "Striker";
        case BallRole::Guardian: return "Guardian";
        case BallRole::Support:  return "Support";
        case BallRole::Mage:     return "Mage";
        case BallRole::Shooter:  return "Shooter";
        case BallRole::Assassin: return "Assassin";
        case BallRole::Summoner: return "Summoner";
        case BallRole::Jester:   return "Jester";
    }
    return "Normal";
}

const char* roleDesc(BallRole r) {
    switch (r) {
        case BallRole::Normal:   return "no class yet - 2 items of one tag give it that class";
        case BallRole::Striker:  return "hits far harder when flung fast - the one to throw";
        case BallRole::Guardian: return "big; bounces toward the closest threat, shoves and staggers it, smashes through shields";
        case BallRole::Support:  return "weak hits, but marks enemies so every ball hits them harder";
        case BallRole::Mage:     return "carries more abilities: 2 ability slots, 3 once ascended";
        case BallRole::Shooter:  return "fires small bullets at the nearest enemy as it flies - faster the faster it goes";
        case BallRole::Assassin: return "after a kill it teleports to the nearest enemy";
        case BallRole::Summoner: return "summons helpers: short-lived balls, turrets, a small dragon...";
        case BallRole::Jester:   return "plays on chance: wild odds, wild results";
    }
    return "";
}

const char* ascendedName(BallRole r) {
    switch (r) {
        case BallRole::Normal:   return "Normal";
        case BallRole::Striker:  return "Mega Striker";
        case BallRole::Guardian: return "Iron Guardian";
        case BallRole::Support:  return "Grand Support";
        case BallRole::Mage:     return "Ancient Mage";
        case BallRole::Shooter:  return "Deadeye";
        case BallRole::Assassin: return "Shadow Assassin";
        case BallRole::Summoner: return "Archsummoner";
        case BallRole::Jester:   return "Grand Jester";
    }
    return "Normal";
}

const char* ascendedDesc(BallRole r) {
    switch (r) {
        case BallRole::Normal:   return "";
        case BallRole::Striker:  return "hits well above its cruise speed also throw a shockwave";
        case BallRole::Guardian: return "every core bounce sends out a pulse that shoves and staggers";
        case BallRole::Support:  return "its marks spread to the enemies around the one it hits";
        case BallRole::Mage:     return "a third ability slot";
        case BallRole::Shooter:  return "every 4th volley is also a rail shot through the whole line; bullets hop once more";
        case BallRole::Assassin: return "teleports chain from kill to kill";
        case BallRole::Summoner: return "more summons, and stronger";
        case BallRole::Jester:   return "the odds bend even further";
    }
    return "";
}

const char* abilityName(Ability a) {
    switch (a) {
        case Ability::None:      return "";
        case Ability::Dash:      return "Dash";
        case Ability::Nova:      return "Nova";
        case Ability::Split:     return "Split";
        case Ability::Bulwark:   return "Bulwark";
        case Ability::Overclock: return "Overclock";
    }
    return "";
}

float abilityCooldown(Ability a, int level) {
    namespace A = cfg::ability;
    float base = 0.f;
    switch (a) {
        case Ability::None:      return 0.f;
        case Ability::Dash:      base = A::dashCooldown; break;
        case Ability::Nova:      base = A::novaCooldown; break;
        case Ability::Split:     base = A::splitCooldown; break;
        case Ability::Bulwark:   base = A::bulwarkCooldown; break;
        case Ability::Overclock: base = A::overclockCooldown; break;
    }
    return base * std::pow(A::cooldownPerLevel, static_cast<float>(std::max(0, level - 1)));
}

}  // namespace sb
