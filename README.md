# Space-Breakers

A roguelite about defending a core. You start a run with a single ball that
bounces freely around the arena — off the walls and off the core — clearing
enemies that march on the core. It tracks nothing; you fling it to aim it into
a cluster. After each wave you pick 1 of 4 rolled upgrades (more balls, core
buffs, a fire ball, …). A run is ten waves: clear the tenth to win, or lose it
when the core falls. Either way you keep *cores*, spent in the game menu on
permanent unlocks before the next run.

Work in progress on the `combat-rework` branch. See `DESIGN.md` for the full
design and roadmap.

## Build

Needs a C++17 compiler. CMake is the cross-platform path (Linux + Windows);
the `Makefile` is a Linux-only shortcut.

### CMake (Linux or Windows)

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release -j
```

The executable lands in `build/` (Linux) or `build/Release/` (Windows) with a
copy of `assets/` — and `openal32.dll` — placed next to it, so it runs when
launched from anywhere, not just the project root.

If SFML is installed system-wide (`libsfml-dev` on Linux) CMake uses it.
Otherwise it downloads and statically links SFML 2.6.2 — no manual install,
which is what makes a fresh Windows machine build with nothing but CMake + the
MSVC or MinGW toolchain.

### Makefile (Linux shortcut)

```bash
make            # produces ./space_breakers  (needs libsfml-dev)
make run
```

### Prebuilt binaries

Every push builds a Linux and a Windows package on CI
(`.github/workflows/build.yml`); grab them from the run's **Artifacts**.
Pushing a `vX.Y.Z` tag also attaches both as zips to a GitHub Release.

Set `SPACE_BREAKERS_NO_AUDIO=1` to run without sound (headless / SSH).

## Controls

| Input | Action |
|-------|--------|
| Left mouse | grab a ball and fling it to redirect it |
| `ESC` | pause (and back out of any screen) |
| `F` / `F11` | toggle fullscreen |
| `M` | toggle sound |
| `1`-`4` | choice screen: pick that upgrade |
| `1`-`5` | game menu: spend cores on a permanent unlock &nbsp;&nbsp; `Enter` start run |

## Layout

```
CMakeLists.txt     cross-platform build (system SFML, or fetch+static)
Makefile           Linux-only shortcut
.github/workflows  CI: Linux + Windows packages on every push
assets/arial.ttf   the only bundled asset (SFX and graphics are procedural)
src/
  main.cpp
  core/
    App.*         window + subsystems + screen stack, the fixed-step loop
    Config.hpp    every gameplay / physics / economy tuning number
    Math.hpp      vector maths, colour helpers, RNG
    Theme.hpp     palette, type sizes, ball speed->colour ramp
  sim/
    World.*       the simulation: bouncing balls, enemies, the core, waves, elements
    Entities.hpp  Ball(+Element) / Enemy / Projectile / Puddle / Obstacle / Core
    Collision.*   circle-vs-bounds, circle-vs-circle, ball-vs-ball resolution
  render/
    WorldRenderer.*  draws puddles, obstacles, the core, enemies, bolts and balls
    Effects.*        impact rings, edge glow, floating labels, screen tint
  ui/
    Screen.hpp    screen interface (input / update / draw, stackable)
    Screens.*     menu, loadout (game menu), play, choice, pause, stats, how-to
    Menu.* Hud.* Widgets.*   the individual widgets
  platform/
    Window.*      video mode, letterboxed view, fullscreen
    Audio.*       procedural sound effects (no audio assets)
    Save.*        plain-text save file (v5: persistent meta only)
  progression/
    Offers.hpp    the 12-upgrade between-wave pool + permanent meta-unlock table
    GameData.hpp  MetaState (persists) + RunState + RunMods (in memory only)
```

The simulation runs on a fixed timestep (`cfg::loop::fixedDt`), so the ball
behaves identically at any frame rate; rendering still happens once per frame.

Progress is saved to `saves/save.txt` on exit, on autosave (every 20s) and at
each wave boundary. Permanent progress (`meta.*`) always persists; a run in
progress (`run.*`) is saved so it can be resumed.
