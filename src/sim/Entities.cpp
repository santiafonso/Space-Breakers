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
        case Element::Plague:   return "Black plague";
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
        case Element::Plague:   return theme::elemPlague;
    }
    return theme::ballMid;
}

sf::Color roleColor(BallRole r) {
    switch (r) {
        case BallRole::Normal:   return theme::textLo;
        case BallRole::Striker:  return theme::ballFast;
        case BallRole::Guardian: return theme::core;
        case BallRole::Support:  return theme::puSurge;
        case BallRole::Mage:     return theme::classMage;
        case BallRole::Shooter:  return theme::classShooter;
        case BallRole::Assassin: return theme::classAssassin;
        case BallRole::Summoner: return theme::classSummoner;
        case BallRole::Jester:   return theme::classJester;
        case BallRole::Slinger:  return theme::classSlinger;
        case BallRole::Alchemist: return theme::classAlchemist;
        case BallRole::Astronomer: return theme::classAstronomer;
        case BallRole::Hermes: return theme::classHermes;
        case BallRole::Odin: return theme::classOdin;
        case BallRole::Thanatos: return theme::classThanatos;
        case BallRole::Tiamat: return theme::classTiamat;
        case BallRole::Loki: return theme::classLoki;
        case BallRole::Nergal: return theme::classNergal;
        case BallRole::Apolo: return theme::classApolo;
        case BallRole::Ares: return theme::classAres;
        case BallRole::Artemisa: return theme::classArtemisa;
    }
    return theme::textLo;
}

// Its (lead) class colour; a second class is a ring on the ball (drawBallIdentity).
sf::Color ballHue(const Ball& b) { return roleColor(b.leadRole()); }

const char* enemyName(EnemyKind k) {
    switch (k) {
        case EnemyKind::Grunt:    return "Grunt";
        case EnemyKind::Runner:   return "Runner";
        case EnemyKind::Tank:     return "Tank";
        case EnemyKind::Splitter: return "Splitter";
        case EnemyKind::Shard:    return "Shard";
        case EnemyKind::Shielded: return "Shielded";
        case EnemyKind::Blinker:  return "Blinker";
        case EnemyKind::Mender:   return "Mender";
        case EnemyKind::Brute:    return "Brute";
        case EnemyKind::Snare:    return "Snare";
        case EnemyKind::Caster:   return "Caster";
    }
    return "";
}

const char* bossName(BossKind k) {
    switch (k) {
        case BossKind::Charger: return "Charger";
        case BossKind::Hive:    return "Hive";
        case BossKind::Warden:  return "Warden";
        case BossKind::Dasher:  return "Dasher";
        case BossKind::Orbital: return "Orbital";
    }
    return "";
}

const char* bossDesc(BossKind k) {
    switch (k) {
        case BossKind::Charger: return "walks dead straight at the core - every hit pushes it back; break its pillars to hurt it again";
        case BossKind::Hive:    return "bursts fans of runners at the core; later it hides behind nests - break them";
        case BossKind::Warden:  return "its shield turns and blocks balls; later only its glowing back really hurts it";
        case BossKind::Dasher:  return "dashes across the map; later it is armoured - make it slam a wall, then hit it";
        case BossKind::Orbital: return "spirals in behind a ring of shields; anchors keep it safe - the last boss";
    }
    return "";
}

const char* bossPhaseCall(BossKind k, int stage) {
    if (stage >= 2) return "ENRAGED";
    switch (k) {
        case BossKind::Charger: return "BREAK THE PILLARS";
        case BossKind::Hive:    return "BREAK THE NESTS";
        case BossKind::Warden:  return "HIT ITS BACK";
        case BossKind::Dasher:  return "MAKE IT CRASH";
        case BossKind::Orbital: return "BREAK THE ANCHORS";
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
        case EnemyKind::Blinker:  return "jumps closer to the core every few seconds - catch it between jumps";
        case EnemyKind::Mender:   return "slow; heals every enemy around it - kill it first";
        case EnemyKind::Brute:    return "miniboss: huge, very tough, barely pushed; flattens a chunk of the core";
        case EnemyKind::Snare:    return "catches the first ball that hits it and holds it until you kill it with another";
        case EnemyKind::Caster:   return "stays back and drains the core with a beam - go and get it";
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
        case BallRole::Slinger:  return "Slinger";
        case BallRole::Alchemist: return "Alchemist";
        case BallRole::Astronomer: return "Atlas";
        case BallRole::Hermes: return "Hermes";
        case BallRole::Odin: return "Odin";
        case BallRole::Thanatos: return "Thanatos";
        case BallRole::Tiamat: return "Tiamat";
        case BallRole::Loki: return "Loki";
        case BallRole::Nergal: return "Nergal";
        case BallRole::Apolo: return "Apolo";
        case BallRole::Ares: return "Ares";
        case BallRole::Artemisa: return "Artemisa";
    }
    return "Normal";
}

const char* roleDesc(BallRole r) {
    switch (r) {
        case BallRole::Normal:   return "no class yet - 2 items of one tag give it that class";
        case BallRole::Striker:  return "hits far harder when flung fast - the one to throw";
        case BallRole::Guardian: return "big; bounces toward the closest threat, shoves and staggers it, smashes through shields";
        case BallRole::Support:  return "weak hits, but marks enemies so every ball hits them harder";
        case BallRole::Mage:     return "carries more abilities: a 2nd ability slot, and every hit it lands brings its abilities 0.2 s closer";
        case BallRole::Shooter:  return "fires small bullets at the nearest enemy as it flies - faster the faster it goes";
        case BallRole::Assassin: return "after a kill it blinks next to the nearest enemy and dives at it";
        case BallRole::Summoner: return "calls a small spriteling ball every few seconds; all its summons hit harder and last longer";
        case BallRole::Jester:   return "chaos: every few seconds it pulls a random trick from another class - a blink, missiles, a burst of bullets, a launch, a nova, a zap, blades, a smite (more often with more Jester items) - and each hit may land twice";
        case BallRole::Slinger:  return "made for your hands: catching it pays 50% more, and the first hit after you throw it lands 25% harder";
        case BallRole::Alchemist: return "carries 2 elements and switches between them hit by hit - its own elements react with each other";
        case BallRole::Astronomer: return "the Guardian ascended: its planets orbit the core and guard it, and every bounce off the core sets them off - one planet is enough for the class";
        case BallRole::Hermes: return "the Striker ascended, the messenger: far faster, a much higher top speed, and its hits grow with its speed - one of his items is enough for the class";
        case BallRole::Thanatos: return "the Assassin ascended, death itself: it never stops blinking - from enemy to enemy, striking from behind, and every kill gathers a soul (harder hits) - one of his items is enough for the class";
        case BallRole::Artemisa: return "the Slinger ascended, the huntress: every throw looses a silver arrow down its line that marks the first enemy it meets as her prey, and if you leave her be she throws herself - every one of her items fires on the throw or the catch - one of her items is enough for the class";
        case BallRole::Ares: return "the Shooter ascended, the god of war: a ring of ghostly bronze cannons turns round it, firing at everything without pause, and every kill feeds its bloodlust - its whole arsenal fires faster; every one of his items is one more weapon - one of his items is enough for the class";
        case BallRole::Apolo: return "the Support ascended, lord of the sun and the muses: four golden muses fly with your balls, each lending one a great gift - strength, speed, a twin, giant size - and marking a weak point on the enemy nearest it; every 5 s they move on to the next ball - one of his items is enough for the class";
        case BallRole::Nergal: return "the Alchemist ascended, lord of plagues: his element is his own - the black plague, which reacts with every other element its own way - every hit infects, a trail of swarms hunts behind him, and the infected die into rats - one of his items is enough for the class";
        case BallRole::Loki: return "the Jester ascended, the trickster: it keeps splitting off clones of itself that fly about playing games of chance - and what comes up, happens - one of his items is enough for the class";
        case BallRole::Tiamat: return "the Summoner ascended, mother of dragons: she rides a great dragon - slow, but it rains fire on the packs and roars, gusts and sweeps on its own - and every kill hatches a lizard - one of her items is enough for the class";
        case BallRole::Odin: return "the Mage ascended, the all-father: it casts in volleys - all of its abilities at once every 8 s; 3 ability slots, abilities 30% stronger and wider, recharging faster - one of his items is enough for the class";
    }
    return "";
}

const char* roleTraits(BallRole r) {
    switch (r) {
        case BallRole::Normal:   return "";
        case BallRole::Striker:  return "+8% damage, +10% top speed";
        case BallRole::Guardian: return "much slower (x0.65 speed), 75% hits; +20% knockback, heals the core a little per core bounce";
        case BallRole::Support:  return "+10% element potency, +5% ability recharge";
        case BallRole::Mage:     return "half damage on contact; +15% ability recharge, +5% element potency";
        case BallRole::Shooter:  return "70% damage on contact; +10% bullet damage, +5% crit";
        case BallRole::Assassin: return "fast (x1.25 speed), hits from behind (the side away from the core) x1.8; +10% crit, +25% crit damage";
        case BallRole::Summoner: return "60% damage on contact; summons +15% stronger, +5% ability recharge";
        case BallRole::Jester:   return "every contact hit rolls x0.5 to x1.7; +8% crit, +5% chance a hit lands twice";
        case BallRole::Slinger:  return "your throw x1.6, but left alone it slows down to half speed - throw it again; grabbed from further, +15% thrown hit";
        case BallRole::Alchemist: return "85% damage on contact; +15% element potency, +5% ability recharge";
        case BallRole::Astronomer: return "75% damage on contact; +10% planet size, +5 core max health; every 2 bounces it goes for the core; every kill makes it (and its planets) 1.5% bigger for the fight, up to +30%";
        case BallRole::Hermes: return "x1.5 speed, x1.7 top speed, only ever gains speed (till it delivers); light hits (x0.3), many of them; won't touch the core till 60 bounces - each one faster - then goes for it and delivers its message to the whole arena";
        case BallRole::Thanatos: return "blinks behind an enemy and stabs it (x1.7) till it dies, then flies free 0.9 s before its next blink; every kill: +8% damage for the fight (up to +120%) and the wait halved";
        case BallRole::Artemisa: return "your throw x1.6; every throw fires a silver arrow through its whole line (80% of its hit) and marks the first enemy on it as prey (+35% from everything, 6 s) - the ball's first hit on its prey lands x2; left alone 4 s, she throws herself at the enemies as if you had";
        case BallRole::Ares: return "40% damage on contact; 6 ghost cannons fire a heavy bullet every 0.1 s (55% of its hit) at the nearest enemies; bloodlust: every kill +8% fire rate for all its weapons (up to x2), fading when the killing stops";
        case BallRole::Apolo: return "90% damage on contact; four muses, one per ball (never two on one): Strength x2 damage, Speed x1.8, Twin a copy of the ball with its items and abilities, Giant x1.5 size and +50% damage; each marks a weak point (+35% from everything) on the enemy nearest its ball; they rotate every 5 s (with fewer balls the gifts come round in turn), and every kill holds them 0.5 s longer";
        case BallRole::Nergal: return "85% damage on contact; its element is the black plague (with fire a pyre, ice cold death, poison pestilence, electric fever, water a miasma, stone a mass grave); every hit infects (a rot, 4 s); it leaves a swarm behind every 0.5 s that hunts what's near; every infected enemy that dies lets out a rat (up to 20)";
        case BallRole::Loki: return "80% damage on contact; every 3 s it splits off a clone (up to 3, 7 s each) that flies about flipping a coin every 1.6 s - heads, a bolt from the sky (3x its hit); tails, a jab (1x); every kill may split off another at once";
        case BallRole::Tiamat: return "slow and heavy (x0.55 speed); 70% damage on contact; her dragon lobs a fireball at the thickest pack every second, breathes fire at what's near every 1.2 s, rakes what's close and leaves burning embers, and every 3.5 s (2.5 s grown) does a move of its own (roar, dive, inferno, meteors); every kill hatches a lizard (up to 18, 24 with Brood mother)";
        case BallRole::Odin: return "60% damage on contact; casts only in volleys - all its abilities at once every 12 s (sooner with recharge); +0.3 recharge bonus; every kill brings the next volley 4% sooner";
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
        case BallRole::Slinger:  return "Master Slinger";
        case BallRole::Alchemist: return "Archalchemist";
        case BallRole::Astronomer: return "Atlas";
        case BallRole::Hermes: return "Hermes";
        case BallRole::Odin: return "Odin";
        case BallRole::Thanatos: return "Thanatos";
        case BallRole::Apolo: return "Apolo";
        case BallRole::Ares: return "Ares";
        case BallRole::Artemisa: return "Artemisa";
        case BallRole::Nergal: return "Nergal";
        case BallRole::Loki: return "Loki";
        case BallRole::Tiamat: return "Tiamat";
    }
    return "Normal";
}

const char* ascendedDesc(BallRole r) {
    switch (r) {
        case BallRole::Normal:   return "";
        case BallRole::Striker:  return "its fastest hits also send out a shockwave";
        case BallRole::Guardian: return "every core bounce sends out a pulse that shoves and staggers";
        case BallRole::Support:  return "its marks spread to the enemies around the one it hits";
        case BallRole::Mage:     return "a 3rd ability slot, and every ability it casts also lets out an arcane nova around it";
        case BallRole::Shooter:  return "every 4th volley is also a rail shot through the whole line; bullets hop once more";
        case BallRole::Assassin: return "each blink cuts through up to 3 enemies on the way, and blinks come twice as often";
        case BallRole::Summoner: return "spritelings twice as often, carrying its items; summons stronger and leave their full element";
        case BallRole::Jester:   return "two tricks at a time (three with 6+ Jester items); every Jester roll is taken twice and the best kept";
        case BallRole::Slinger:  return "every catch recharges its abilities by 35%, and its thrown first hit lands 50% harder";
        case BallRole::Alchemist: return "its reactions hit 50% harder";
        case BallRole::Astronomer:
        case BallRole::Hermes:
        case BallRole::Odin:
        case BallRole::Thanatos:
        case BallRole::Tiamat:
        case BallRole::Loki:
        case BallRole::Nergal:
        case BallRole::Apolo:
        case BallRole::Ares:
        case BallRole::Artemisa: return "already ascended";
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
        case Ability::Arc:       return "Arc";
        case Ability::Meteor:    return "Meteor";
        case Ability::MagicMissile: return "Magic missile";
        case Ability::Hook: return "Hook";
        case Ability::Mines: return "Mines";
        case Ability::Empower: return "Empower";
        case Ability::Tailwind: return "Tailwind";
        case Ability::Vortex: return "Vortex";
        case Ability::Beam: return "Beam";
        case Ability::Blades: return "Blades";
        case Ability::Cloud: return "Cloud";
        case Ability::Smite: return "Smite";
        case Ability::Warp: return "Warp";
        case Ability::Portal: return "Portal";
        case Ability::Leash: return "Leash";
        case Ability::Swap: return "Swap";
        case Ability::Shrink: return "Shrink";
        case Ability::Bounty: return "Bounty";
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
        case Ability::Arc:       base = A::arcCooldown; break;
        case Ability::Meteor:    base = A::meteorCooldown; break;
        case Ability::MagicMissile: base = A::missileCooldown; break;
        case Ability::Hook: base = A::hookCooldown; break;
        case Ability::Mines: base = A::minesCooldown; break;
        case Ability::Empower: base = A::empowerCooldown; break;
        case Ability::Tailwind: base = A::tailwindCooldown; break;
        case Ability::Vortex: base = A::vortexCooldown; break;
        case Ability::Beam: base = A::beamCooldown; break;
        case Ability::Blades: base = A::bladesCooldown; break;
        case Ability::Cloud: base = A::cloudCooldown; break;
        case Ability::Smite: base = A::smiteCooldown; break;
        case Ability::Warp: base = A::warpCooldown; break;
        case Ability::Portal: base = A::portalCooldown; break;
        case Ability::Leash: base = A::leashCooldown; break;
        case Ability::Swap: base = A::swapCooldown; break;
        case Ability::Shrink: base = A::shrinkCooldown; break;
        case Ability::Bounty: base = A::bountyCooldown; break;
    }
    return base * std::pow(A::cooldownPerLevel, static_cast<float>(std::max(0, level - 1)));
}

}  // namespace sb
