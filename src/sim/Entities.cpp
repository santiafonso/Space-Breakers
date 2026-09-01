#include "sim/Entities.hpp"

namespace sb {

const char* powerUpName(PowerUp p) {
    switch (p) {
        case PowerUp::Points2x: return "DOUBLE POINTS";
        case PowerUp::SlowMo:   return "SLOW MOTION";
        case PowerUp::Surge:    return "SPEED SURGE";
        case PowerUp::Golden:   return "GOLDEN BOUNCE";
        case PowerUp::Ghost:    return "PHASE";
        case PowerUp::Frenzy:   return "FRENZY x3";
    }
    return "";
}

sf::Color powerUpColor(PowerUp p) {
    switch (p) {
        case PowerUp::Points2x: return theme::puPoints;
        case PowerUp::SlowMo:   return theme::puSlow;
        case PowerUp::Surge:    return theme::puSurge;
        case PowerUp::Golden:   return theme::puGolden;
        case PowerUp::Ghost:    return theme::puGhost;
        case PowerUp::Frenzy:   return theme::puFrenzy;
    }
    return theme::accent;
}

float powerUpDuration(PowerUp p) {
    switch (p) {
        case PowerUp::Points2x: return cfg::powerup::durPoints2x;
        case PowerUp::SlowMo:   return cfg::powerup::durSlowMo;
        case PowerUp::Surge:    return cfg::powerup::durSurge;
        case PowerUp::Golden:   return cfg::powerup::durGolden;
        case PowerUp::Ghost:    return cfg::powerup::durGhost;
        case PowerUp::Frenzy:   return cfg::powerup::durFrenzy;
    }
    return 6.f;
}

const char* elementName(Element e) {
    switch (e) {
        case Element::Plain: return "Plain";
        case Element::Fire:  return "Fire";
        case Element::Wind:  return "Wind";
        case Element::Water: return "Water";
        case Element::Stone: return "Stone";
    }
    return "Plain";
}

sf::Color elementColor(Element e) {
    switch (e) {
        case Element::Plain: return theme::ballMid;
        case Element::Fire:  return theme::elemFire;
        case Element::Wind:  return theme::elemWind;
        case Element::Water: return theme::elemWater;
        case Element::Stone: return theme::elemStone;
    }
    return theme::ballMid;
}

}  // namespace sb
