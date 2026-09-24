#include "sim/Entities.hpp"

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

const char* roleName(BallRole r) {
    switch (r) {
        case BallRole::Normal:   return "Normal";
        case BallRole::Striker:  return "Striker";
        case BallRole::Support:  return "Support";
        case BallRole::Guardian: return "Guardian";
    }
    return "Normal";
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

const char* roleDesc(BallRole r) {
    switch (r) {
        case BallRole::Normal:   return "no role yet - a ROLE pick gives it one";
        case BallRole::Striker:  return "hits far harder when flung fast - the one to throw";
        case BallRole::Support:  return "weak hits, but marks enemies so every ball hits them harder";
        case BallRole::Guardian: return "big; bounces toward the closest threat, shoves and staggers it, smashes through shields";
    }
    return "";
}

}  // namespace sb
