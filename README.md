# Space-Breakers

A roguelite about defending a core. Your balls bounce freely around the arena —
off the walls and off the core — clearing waves of enemies that march on the
core. They don't track anything; you fling a ball to aim it into a cluster.
Clear a wave, then spend scrap on an elemental ball — fire (burns), wind (fires
bolts), water (damaging trail) or stone (drops blocking rubble). When the core
falls the run ends and leaves you *cores* to spend on permanent unlocks before
the next run.

Work in progress on the `combat-rework` branch. See `DESIGN.md` for the full
design and roadmap.

## Build

Requires SFML 2.5+ (`libsfml-dev`) and a C++17 compiler.

```bash
make            # produces ./space_breakers
make run
```

Or without the Makefile:

```bash
g++ -std=c++17 -O2 -Isrc $(find src -name '*.cpp') -o space_breakers \
    -lsfml-graphics -lsfml-window -lsfml-audio -lsfml-system
```

Run it from the project root so it can find `assets/arial.ttf`.
Set `SPACE_BREAKERS_NO_AUDIO=1` to run without sound (headless / SSH).

## Controls

| Input | Action |
|-------|--------|
| Left mouse | grab a ball and fling it to redirect it |
| `ESC` | pause (and back out of any screen) |
| `F` / `F11` | toggle fullscreen |
| `M` | toggle sound |
| `1`-`4` | choice screen: buy an elemental ball &nbsp;&nbsp; `S` next wave |
| `1`-`2` | hub: spend cores on a permanent unlock |

## Layout

```
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
    Screens.*     hub, play, choice, run-summary, pause, stats, how-to
    Menu.* Hud.* Widgets.*   the individual widgets
  platform/
    Window.*      video mode, letterboxed view, fullscreen
    Audio.*       procedural sound effects (no audio assets)
    Save.*        plain-text save file (v4: meta.* always, run.* when resuming)
  progression/
    Offers.hpp    the four elemental-ball offers + permanent meta-unlock table
    GameData.hpp  MetaState (persists) + RunState (resumable)
```

The simulation runs on a fixed timestep (`cfg::loop::fixedDt`), so the ball
behaves identically at any frame rate; rendering still happens once per frame.

Progress is saved to `saves/save.txt` on exit, on autosave (every 20s) and at
each wave boundary. Permanent progress (`meta.*`) always persists; a run in
progress (`run.*`) is saved so it can be resumed.
