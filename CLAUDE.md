# Space-Breakers — CLAUDE.md

Roguelite de defensa de núcleo en C++17 + SFML 2.6. Pelotas que rebotan solas, el jugador
las agarra y las tira; clases, items, habilidades, web de meta-progresión, dioses, jefes,
Ascensión. Estado: **beta abierta 0.1.0** en itch, iterando con feedback de playtest.

## Compilar y correr

```bash
make -j && ./space_breakers                                   # Linux, rápido (libsfml-dev)
cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j   # lo que usa CI
```

- Compilar siempre después de tocar código; un cambio no está hecho si no compila sin warnings nuevos.
- CI (`.github/workflows/build.yml`) arma Linux + Windows en cada push a main. El build de
  Windows es MSVC: nada de builtins de GCC (`__builtin_popcount` ya lo rompió una vez).
- `~/Space-Breakers-itch/` tiene los paquetes y la página de itch (fuera del repo).

## Verificar sin jugar (el display cierra ventanas a los segundos)

| Variable | Qué hace |
|---|---|
| `SB_SNAPSHOT=<dir>` | arma cada pantalla, guarda un PNG por pantalla y sale. Save aparte. **Usarlo para revisar UI.** |
| `SB_BALANCE=<oleadas>` (+ `SB_BALANCE_ONLY=<nombre>`) | simula cada clase/dios con sus mejores items |
| `SB_TUTSIM=<n>` (+ `SB_TUTSIM_LAZY=1`) | simula el tutorial con un jugador nuevo |
| `SB_FXSHEET=<dir>` / `SB_ICONSHEET=<dir>` | hojas con todos los efectos / íconos |
| `SB_SHOWCASE=<dir>` (+ `SB_SHOWCASE_FRAMES`, `SB_SHOWCASE_FPS=60`, `SB_SHOWCASE_BUILD=<clase/dios>`, `SB_SHOWCASE_ENEMY=<n>`, `SB_SHOWCASE_SCREEN=map|web`) | graba una pelea cuadro a cuadro (GIF/portada: `tools/showcase/`; videos de TikTok: `~/Space-Breakers-itch/scripts/make_tiktok2.py`) |
| `SB_PROFILE=1` (con `SB_SHOWCASE=<dir>`, + `SB_SHOWCASE_CROWD=<n>`, `SB_SHOWCASE_BALLS=<n>`) | mide update/render por cuadro en una pelea llena, sin PNGs. **Medir acá antes de optimizar.** |
| `SB_DEV=1` (+ `SB_WAVE`, `SB_BALLS`, `SB_UPGRADES`, `SB_TUTORIAL`) | modo dev, F1 = panel |
| `SPACE_BREAKERS_NO_AUDIO=1` | sin sonido |

Los modos de simulación nunca tocan `saves/save.txt`; mantenerlo así.

## Dónde está cada cosa

- `core/Config.hpp`, `core/ConfigClasses.hpp` — todos los números de tuning. `cfg::app::version`:
  subirla con cada build que se publique.
- `core/App.cpp` — loop, stack de pantallas, flujo de run, forja, y los modos SB_* de arriba.
- `sim/World*.cpp` — la simulación, partida por tema (`WorldClasses`, `WorldGods*`,
  `WorldAbilities`, `WorldCommons`, `WorldPlanets`, `WorldNergal`…). Paso fijo (`cfg::loop::fixedDt`).
- `sim/Classes.hpp`, `sim/Entities.hpp` — clases y entidades.
- `progression/` — `Offers.hpp` (pool de mejoras, `upgradeKindId` para SB_UPGRADES),
  `UpgradeKind.hpp`, `ClassItems.hpp`, `Pacts`, `Creeds`, `Events` ("?"), `RunMap`, `Ascension`.
- `render/ClassRender.cpp`, `render/WorldRenderer.cpp`, `render/Effects.*` — dibujo (todo procedural).
- `ui/Screens.cpp`, `ui/RunScreens.cpp`, `ui/Widgets.cpp`, `ui/WebSky.cpp`, `ui/DevScreen.cpp`.
- `platform/` — ventana, audio, save (texto plano, parser tolerante), `Browser.cpp`.

Los archivos grandes (`ClassRender`, `App`, `World`, `Screens`) superan los 130 KB: leerlos
por partes con grep, no enteros.

## DESIGN.md

Doc vivo de diseño, ~3500 líneas: **no leerlo entero**. Buscar con grep la sección que importa.
La §11 es el registro de cambios por fecha y está al final; "Anotado para después" (§11, ~línea
3000) junta lo que Santi pidió dejar para más adelante. Cada cambio de diseño o balance se
anota en §11 con fecha, en el mismo estilo (qué cambió, por qué, función/archivo).

## Convenciones

- Commits en español, una línea que describe lo que cambia para el jugador, y `(DESIGN §11)`
  si se anotó ahí. Sin atribución a Claude.
- Textos del juego en inglés (todavía no hay localización; la base del español está pausada
  en la rama `worktree-agent-af94b0df16c9019aa` — no mergear).
- Remotos: `origin` (santiafonso) y `axo` (Git-Axo, colaborador). Push solo si se pide.

## Cómo trabajar con agentes acá

- Por defecto una sola sesión, sin agentes en paralelo: los archivos centrales son pocos y
  enormes, y dos agentes en `App.cpp` o `World.cpp` terminan en conflictos.
- Explore para ubicar código; agente en background solo para simulaciones largas.
- Esfuerzo: `medium` para fixes de playtest, `high` + plan mode para sistemas nuevos,
  `xhigh` solo para bugs que no salen.
- `/code-review` antes de subir un build a itch.
