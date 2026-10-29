#pragma once

#include <string>

#include "core/Config.hpp"
#include "sim/Entities.hpp"

// Act rules (2026-10-29): every run rolls one rule per act, all five shown from
// the start, so the build gets planned around them. A rule tilts damage: one
// source hits softer, another harder (sim DmgSrc). Act 1's is half as strong.
// Numbers in cfg::actrule.

namespace sb {

enum class ActRule { ThickHull, NullField, ColdVoid, BrokenSignal, DenseEther, StaticStorm, OpenSky, Count };
inline constexpr int kActRuleCount = static_cast<int>(ActRule::Count);

struct ActRuleDef {
    const char* name;
    DmgSrc weak;     // hits softer
    DmgSrc strong;   // hits harder
};

inline ActRuleDef actRuleDef(ActRule r) {
    switch (r) {
        case ActRule::ThickHull:    return {"Thick hull", DmgSrc::Contact, DmgSrc::Ability};
        case ActRule::NullField:    return {"Null field", DmgSrc::Ability, DmgSrc::Contact};
        case ActRule::ColdVoid:     return {"Cold void", DmgSrc::Element, DmgSrc::Contact};
        case ActRule::BrokenSignal: return {"Broken signal", DmgSrc::Summon, DmgSrc::Ability};
        case ActRule::DenseEther:   return {"Dense ether", DmgSrc::Contact, DmgSrc::Element};
        case ActRule::StaticStorm:  return {"Static storm", DmgSrc::Ability, DmgSrc::Element};
        case ActRule::OpenSky:      return {"Open sky", DmgSrc::Element, DmgSrc::Summon};
        case ActRule::Count:        break;
    }
    return {"", DmgSrc::Other, DmgSrc::Other};
}

inline const char* dmgSrcName(DmgSrc s) {
    switch (s) {
        case DmgSrc::Contact: return "contact";
        case DmgSrc::Ability: return "abilities";
        case DmgSrc::Element: return "elements";
        case DmgSrc::Summon:  return "summons and shots";
        case DmgSrc::Other:   break;
    }
    return "";
}

// How hard the rule bites in act `act` (1-5): the weak source's loss and the
// strong one's gain, as fractions.
inline float actRuleWeak(int act) { return act <= 1 ? cfg::actrule::weakAct1 : cfg::actrule::weak; }
inline float actRuleStrong(int act) { return act <= 1 ? cfg::actrule::strongAct1 : cfg::actrule::strong; }

inline std::string pctText(float f) { return std::to_string(static_cast<int>(f * 100.f + 0.5f)) + "%"; }

// "ball hits -40%, abilities +25%" - the rule's effect in act `act`.
inline std::string actRuleDesc(int rule, int act) {
    if (rule < 0 || rule >= kActRuleCount) return "";
    const ActRuleDef d = actRuleDef(static_cast<ActRule>(rule));
    return std::string(dmgSrcName(d.weak)) + " -" + pctText(actRuleWeak(act)) + ", " + dmgSrcName(d.strong) + " +" +
           pctText(actRuleStrong(act));
}

// The rule as the sim sees it. `rule` < 0 = none (the tutorial, old runs).
inline SourceMul actRuleMul(int rule, int act) {
    SourceMul m;
    if (rule < 0 || rule >= kActRuleCount) return m;
    const ActRuleDef d = actRuleDef(static_cast<ActRule>(rule));
    auto set = [&](DmgSrc s, float v) {
        switch (s) {
            case DmgSrc::Contact: m.contact = v; break;
            case DmgSrc::Ability: m.ability = v; break;
            case DmgSrc::Element: m.element = v; break;
            case DmgSrc::Summon:  m.summon = v; break;
            case DmgSrc::Other:   break;
        }
    };
    set(d.weak, 1.f - actRuleWeak(act));
    set(d.strong, 1.f + actRuleStrong(act));
    return m;
}

}  // namespace sb
