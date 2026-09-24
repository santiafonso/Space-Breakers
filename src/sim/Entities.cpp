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

}  // namespace sb
