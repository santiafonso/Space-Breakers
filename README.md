# Space-Breakers

A tiny idle game: a ball bounces around the arena and every bounce scores.
Grab the ball with the mouse and fling it — a faster ball bounces more often and
each bounce is worth more. Keep it bouncing to build a combo multiplier, steer
it into glowing orbs for short power-ups, and spend points on upgrades.

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
| Left mouse | grab a wall or the nearest ball, drag, release to throw |
| Right mouse | on a wall: start / stop it drifting |
| `TAB` | open / close upgrades |
| `ESC` | pause (and back out of any screen) |
| `F` / `F11` | toggle fullscreen |
| `M` | toggle sound |

Walls are yours to arrange: drag them where you like, fling one to send it
drifting and bouncing around the arena, right-click to settle it again.

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
    World.*       the simulation: balls, walls, power-ups, combo streak
    Entities.hpp  Ball / Wall / Pickup / ActiveEffect / FrameEvents
    Collision.*   circle-vs-bounds, circle-vs-wall, ball-vs-ball resolution
  render/
    WorldRenderer.*  draws balls, walls and pickups
    Effects.*        bounce rings, edge glow, floating labels, screen tint
  ui/
    Screen.hpp    screen interface (input / update / draw, stackable)
    Screens.*     main menu, play, shop, pause, stats, how-to
    Menu.* Hud.* Shop.* Widgets.*   the individual widgets
  platform/
    Window.*      video mode, letterboxed view, fullscreen
    Audio.*       procedural sound effects (no audio assets)
    Save.*        plain-text save file
  progression/
    Upgrades.hpp  upgrade table and cost curves
    GameData.hpp  what persists between sessions
```

The simulation runs on a fixed timestep (`cfg::loop::fixedDt`), so the ball
behaves identically at any frame rate; rendering still happens once per frame.

Progress is saved to `saves/save.txt` on exit, on autosave (every 20s) and when
returning to the main menu.
