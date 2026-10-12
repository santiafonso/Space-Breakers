# Space-Breakers — rework de combate (roguelite)

Documento vivo. Recoge el cambio de concepto y el plan por fases.
Rama: `combat-rework`.

---

## 1. El cambio

**Antes:** una pelota rebota, cada rebote da puntos, la agarras y la lanzas para
que vaya más rápido. El objetivo es la puntuación.

**Ahora:** un **roguelite de defensa de núcleo**. Las pelotas son agentes
autónomos que **derrotan enemigos**. Tú no las pilotas segundo a segundo: las
**lanzas** y **das forma al campo de batalla** con estructuras que desvían su
trayectoria (agujeros negros, rampas, imanes…).

Una **partida (run)** empieza en la arena 1 y avanza por oleadas y arenas cada vez
más difíciles. Entre oleadas eliges mejoras: **añadir pelotas, modificarlas y
cambiar el escenario**. Cuando el **núcleo muere, la run termina**: vuelves al
principio, pero con **cosas nuevas desbloqueadas** de forma permanente.

Referencia mental: Peggle + Vampire Survivors + defensa de núcleo, con la
meta-progresión de un roguelite.

---

## 2. Decisiones ya tomadas

| Tema | Decisión |
|------|----------|
| **Estructura** | Roguelite. Run = arena 1 → oleadas/arenas hasta que cae el núcleo. Al morir: desbloqueos permanentes y vuelta a empezar. |
| **Derrota** | Núcleo en el centro de la arena. Los enemigos avanzan hacia él. Vida del núcleo a 0 → fin de la run. |
| **Control en combate** | Lanzas la pelota al empezar la oleada y es autónoma. Durante el combate solo **colocas / mueves / activas estructuras de campo**. |
| **Economía** | Dos capas: **chatarra** (moneda de la run, se reinicia) para mejoras entre oleadas, y **combo** reconvertido en **multiplicador de daño** dentro de la oleada. Más una **meta-moneda** que persiste (§4). |
| **Progresión de la run** | Entre oleadas, pantalla de **elección**: compras una **pelota elemental** (fuego / viento / agua / piedra) con chatarra, o pasas a la siguiente oleada. |
| **Meta-progresión** | Al terminar la run ganas meta-moneda según lo lejos que llegaste + hitos. Se gasta en un **hub** para desbloquear qué *puede aparecer* en futuras runs y bonus de inicio. |
| **Primer jugable** | MVP: 1 arena que escala, daño por contacto, 1 tipo de enemigo, oleadas, núcleo, compra de pelotas elementales entre oleadas, muerte → resumen → meta → nueva run. Sin paredes móviles. |
| **Paredes móviles** | **Se eliminan.** |
| **Movimiento** (2026-09-01) | Las pelotas **rebotan libres en línea recta** por la pantalla y también **rebotan contra el núcleo** (sólido). **No siguen nada**: sin órbita, sin autoguiado, movimiento aleatorio. Flingear una pelota la redirige — es la habilidad principal. (Órbita y autoguiado se probaron y se descartaron.) |
| **Estructuras de campo** (2026-09-01) | **Aplazadas.** El agujero negro se quita por ahora; se retoman después. |
| **Prioridad ahora** (2026-09-01) | **Modificadores de pelota** = pelotas elementales (fuego: quemadura; viento: dispara; agua: rastro que daña; piedra: suelta obstáculos). Se compran con chatarra al final de cada oleada. |
| **Oferta "ganar chatarra"** (2026-09-01) | **Eliminada.** La chatarra solo cae de los enemigos (+ 25 de semilla al empezar la run). |

---

## 3. Los tres bucles

### 3.1 Oleada (segundo a segundo)
1. **Preparación:** las pelotas aparcadas. Colocas / recolocas estructuras de
   campo dentro del presupuesto de ranuras.
2. **Lanzamiento:** agarras y lanzas (mismo feel de siempre). Empieza la oleada.
   Con varias pelotas: todas salen al lanzar (o en ráfaga rápida).
3. **Combate:** las pelotas rebotan en los bordes, su trayectoria se curva por
   las estructuras. Al tocar un enemigo le hacen daño y rebotan. Los enemigos
   caminan hacia el núcleo. Puedes seguir moviendo/activando estructuras.
4. **Fin de oleada:** sin enemigos → pantalla de elección → siguiente oleada.
   Núcleo a 0 → fin de la run.

### 3.2 Run (una partida)
- Arena 1 → oleadas 1..N → arena limpiada → arena 2 (layout, enemigos y
  presupuesto distintos) → … Escalada continua, sin final fijo. Cada X arenas,
  un **jefe**.
- Moneda de la run: **chatarra**, cae de enemigos, se gasta en las elecciones y
  en un pequeño mercado entre arenas. **Se pierde al morir.**
- "Puntuación" de la run = arena/oleada alcanzada + enemigos derrotados. Va a
  records (Stats).

### 3.3 Meta (entre runs)
- Al morir: **resumen de la run** → ganas **meta-moneda** (p. ej. *Núcleos*)
  según arenas superadas, jefes, y **hitos de primera vez**.
- **Hub / meta-tienda:** gastas Núcleos en desbloqueos **permanentes**:
  - nuevos **arquetipos de pelota** en la reserva de ofertas,
  - nuevos **modificadores** en la reserva,
  - nuevas **estructuras de campo**,
  - nuevos enemigos/modificadores de dificultad (más riesgo, más recompensa),
  - **bonus de inicio**: empezar con +1 pelota, +vida de núcleo, +1 ranura de
    campo, fichas de *reroll*, chatarra inicial…
- Empiezas la siguiente run desde la arena 1 con esos desbloqueos ya en la
  reserva / aplicados.

---

## 4. Economía y monedas

| Moneda | Ámbito | Se gana | Se gasta en |
|--------|--------|---------|-------------|
| **Chatarra** | Una run (se reinicia) | Matar enemigos | Elecciones entre oleadas, mercado entre arenas |
| **Combo de daño** | Una oleada (decae) | Golpes seguidos a enemigos | Multiplica el daño en el momento |
| **Núcleos** (meta) | Permanente | Terminar runs (arenas superadas, jefes, hitos) | Hub: desbloqueos permanentes y bonus de inicio |

`daño = base(velocidad_pelota) * (1 + comboTier*k) * mods_de_pelota`

---

## 5. Sistemas

### 5.1 Pelotas, modificadores y clases
- La run **empieza con 1 pelota** de arquetipo básico. Daño por contacto =
  `f(velocidad, combo)`. Más rápida = más daño (mantiene el sentido de lanzarla
  fuerte).
- **Añadir pelotas:** una de las ofertas entre oleadas. Todas se lanzan al
  empezar la oleada.
- **Modificadores** (estilo roguelite, apilables, temporales a la run):
  - `+X% daño`, `+X% velocidad crucero`, `+tamaño`,
  - **Perforante** (atraviesa un enemigo sin frenar),
  - **Esquirla** (al golpear, suelta un fragmento corto),
  - **Cadena** (el golpe salta a un enemigo cercano),
  - **Órbita** (tiende a orbitar el núcleo),
  - **Imán** (leve auto-guiado hacia el enemigo más cercano).
- **Clases** (fase 3): un modificador "grande" que define un build — Ariete,
  Artillero (dispara), Divisor, Guardián. Se desbloquean en el hub y luego
  aparecen como oferta.
- **Equipo** (fase 4): varias pelotas, cada una con sus propios modificadores.

### 5.2 Estructuras de campo (lo que más te gusta)
Se colocan en preparación y se pueden mover/activar en combate. Presupuesto de
**ranuras por run**, ampliable como oferta o en el hub.

| Estructura | Efecto sobre la pelota |
|------------|------------------------|
| **Agujero negro** (MVP) | Atrae: aceleración hacia el objeto ∝ 1/dist², con radio de influencia y tope. |
| **Repulsor** | Empuja hacia fuera. |
| **Bumper** | Rebote de alta restitución: la pelota sale más rápido. |
| **Rampa / booster** | Zona direccional que añade velocidad en un sentido. |
| **Prisma** | Al atravesarlo, divide la trayectoria / duplica la pelota un instante. |
| **Nodo torreta** (fase 2) | Dispara cuando la pelota pasa cerca. |

Por ahora afectan **solo a las pelotas** (foco y legibilidad). "También curvan a
los enemigos" queda como opción a probar.

### 5.3 Enemigos
- MVP: **Errante** — flota hacia el núcleo despacio, poca vida.
- Después: **Corredor** (rápido, directo), **Tanque** (mucha vida, lento),
  **Escindido** (se parte al morir), **Jefe** (barra grande, patrón simple).
- Los enemigos **no atacan a las pelotas**: su amenaza es llegar al núcleo.
- Al morir sueltan chatarra + anillo de impacto + sonido.
- Escalado: cada oleada/arena sube número, vida y velocidad. La meta puede
  añadir variantes con élite/afijos (más recompensa).

### 5.4 Núcleo
- Círculo en el centro con vida y anillo. Un enemigo que lo alcanza le quita vida
  y desaparece. Vida 0 → fin de la run.
- Mejoras (oferta / hub): vida máx, pulso que repele, regeneración lenta.

### 5.5 Oleadas y arenas
- Arena = layout fijo + generador de oleadas + presupuesto de estructuras.
- Spawner: mete `N` enemigos escalonados desde los bordes. Oleada limpiada cuando
  no quedan y no hay más por aparecer.
- Todas las oleadas superadas → arena limpiada → (mini-mercado) → siguiente arena,
  más difícil.

---

## 6. Encaje en el código actual

La arquitectura modular tras el refactor lo hace abordable:

| Módulo | Cambios |
|--------|---------|
| `sim/Entities.hpp` | quitar `Wall`; añadir `Enemy`, `FieldObject` + `FieldKind`, `Core`; ampliar `FrameEvents` (chatarra, muertes, golpe al núcleo, oleada limpiada, run terminada). |
| `sim/World.*` | quitar todo lo de paredes; añadir `enemies_`, `field_`, `core_`, estado de oleada/arena; en `step()`: fuerzas de campo sobre las pelotas, IA de enemigos, colisión bola↔enemigo con daño, spawner, derrota. `grabAt` reusa el agarre para **estructuras** (prioridad) o pelota. `toggleDriftAt` → `toggleFieldAt`. Nuevo: aplicar `mods` de pelota. |
| `sim/Collision.*` | `circleVsWall` se recicla para bola↔enemigo y bola↔núcleo. Nueva `applyFieldForce`. |
| `progression/` | `Upgrades.hpp` → catálogos: `BallMod`, `FieldKind`, `CoreUpgrade`, y `MetaUnlock`. `GameData` se parte en: **`RunState`** (pelotas+mods, estructuras colocadas, chatarra, arena/oleada) y **`MetaState`** (Núcleos, unlocks, records) — solo `MetaState` se guarda entre sesiones; `RunState` se guarda para reanudar una run en curso. |
| `platform/Save.*` | versión 3: bloque `meta.*` (núcleos, unlocks) + bloque `run.*` opcional (run en curso). Parser sigue tolerante. |
| `render/WorldRenderer.*` | quitar paredes; dibujar núcleo (anillo de vida), enemigos (círculo + arco de vida), estructuras (agujero negro: disco + remolino + radio tenue). |
| `ui/` | nuevas pantallas: **Preparación** (colocar estructuras, lanzar), **Elección** (1 de 3 entre oleadas), **Resumen de run** (al morir), **Hub** (meta-tienda). `ShopScreen` desaparece. `Hud` de combate: vida núcleo, oleada/arena, enemigos restantes, chatarra. |
| `core/Config.hpp` | secciones nuevas: `cfg::combat` (daño, knockback), `cfg::field` (fuerza, radio, tope), `cfg::wave` (nº, cadencia, escalado), `cfg::core` (vida), `cfg::meta` (recompensa por arena). |
| `core/App.*` | el stack de pantallas ya soporta esto. Nuevo flujo: Hub → Preparación → Play(oleada) → Elección → Play → … → ResumenRun → Hub. |

---

## 7. Power-ups, items y web (rework 2026-09-08, elementos 2026-09-09)

**Power-ups de arena** (`enum class PowerUp`, 5). Uno cae por vez, se agarra con
la pelota, dura unos segundos. Salen del pool via `App::powerUpMask` (bitmask):

| Power-up | Cómo se obtiene | Efecto |
|----------|-----------------|--------|
| DOUBLE POINTS | base | x2 **score** mientras dura (no toca daño) |
| SPEED SURGE | base | crucero de la pelota x2 |
| SLOW MOTION | nodo *Damper* (prismas) | enemigos a `slowMoEnemyMul` (0.45) |
| GOLDEN BOUNCE | nodo *Facet* (prismas) | el combo sube `goldenComboRate` (2) pasos por golpe |
| OVERDRIVE (ex-Frenesí) | nodo *Overload* (prismas) | pelota x2 daño |

PHASE eliminado. Cadencia base lenta a propósito (`pickup::spawnMin/Max` 46–78 s);
*Uplink* baja `pickupSpawnMult`, *Capacitor* sube `pickupDurMult`.

**Score** (`RunState::score`): +`cfg::score::perKill` (100) por enemigo, x2 con
DOUBLE POINTS, el boss no da. Se muestra arriba-derecha en el HUD y se guarda
`stat.bestScore`.

**Pelotas elementales** (rework 2026-09-09, `enum class Element`, 7):
Plain + Fire · Poison · Water · Ice · Stone · Electric. Se consiguen con los items
**"add X ball"** entre oleadas: agregan directo una pelota de ese elemento
(`UpgradeKind::Add*Ball`, `elementItemSlot`), elegibles solo si su nodo de web
está desbloqueado. Efectos:

| Elemento | Efecto | Config |
|----------|--------|--------|
| Fire     | golpe de contacto más fuerte (`+fireDamageBonus` en `ballDamage`); con el nodo **Ember** además enciende un DoT (`burn`/`burnDps`, escala con `elemMult[Fire]` y el nivel de Ember) | `fireDamageBonus`, `burnDuration`, `burnDps`, `burnPerEmberLevel` |
| Poison   | DoT que **acumula** dps por golpe y se refresca; se limpia al vencer | `poisonDuration`, `poisonDpsPerHit`, `poisonDpsMax` |
| Water    | **estela "gusano"**: `Ball::waterTrail` (deque de posiciones), ribbon que sigue la trayectoria, ancho de cabeza a cola; daño por distancia a los segmentos | `waterInterval`, `waterTrailPoints`, `waterTrailWidth`, `waterDps` |
| Ice      | congela al enemigo en el lugar (sin steering, sin daño al núcleo) | `freezeDuration` |
| Stone    | `Obstacle` que además de bloquear hace daño por contacto | `stoneInterval`, `stoneDps`, `obstacleLife` |
| Electric | `Bolt` (arco instantáneo) al enemigo más cercano dentro de `boltRadius` (invisible), en cooldown | `boltRadius`, `boltInterval`, `boltDamage` |

`WorldParams::elemMult[7]` lleva la potencia por elemento: nivel 1 del nodo de
web desbloquea, niveles 2-3 multiplican (`powerPerLevel`). `Wind`, `Projectile` y
`Puddle` eliminados; `Bolt` (electric) y `Ball::waterTrail` (water) los reemplazan.

**Color de las pelotas:** las plain usan una rampa **gris** (`theme::speedColor`,
solo se aclara con la velocidad); las elementales mantienen su tono a cualquier
velocidad y se vuelven **más vívidas/saturadas** cuanto más rápido van
(`theme::elementSpeedColor` + `vivify()`: greyer bajo el crucero, hue más
profundo por encima — nada de blanco). Electric pasó a violeta
(`theme::elemElectric`). El tono va en `Ball::color` (lo calcula `World`), así
los anillos de rebote también matchean.

**Sonido de rebotes (hipnótico):** cada choque —contra pared/núcleo y **entre
pelotas** (`BounceFx::ballPair`, ahora `resolveBallPairs` empuja evento + squash)—
toca una nota de una **escala pentatónica** (`Audio::ballHit`: más rápido = más
agudo, ball-vs-ball sube ~una octava, leve wander). El multiplicador de daño
(`comboTier/baseCapTier` → `harmony01`) hace entrar de a poco una capa de
**campana** (`bell()`: seno + parciales) y, ya encadenando fuerte, notas de acorde
cada par de golpes → suena más lleno cuanto más frenético. `Audio` sintetiza
`noteSoft_`/`noteRich_` por grado; `kVoices` 8→16 para que las colas se solapen.
Núcleo golpeado = `coreThud()` (golpe grave). `bounce()` eliminado.

**Items entre oleadas** (`UpgradeKind`, 25; `maxBalls`=16):
- Base: Extra ball · 6× "add X ball" (gated por nodo) · Spring core · Slow field ·
  Reflexes · Heavy impact (+8%, cap 3) · Big ball (+10%, cap 3).
- Velocidad (booleanos, decaen vía `regulateSpeed`): **Wall rush** (×speed por
  rebote de pared) · **Carom** (×speed al chocar otra pelota) · **Ricochet**
  (×daño un instante tras rebotar en pared, `Ball::ricochetT`) · **Ceiling
  break** (`maxSpeedMult`, sube el techo) · **Warm-up** (crucero sube con
  `waveClock_` a lo largo de la oleada) · **Strong arm** (flingás más fuerte,
  en `PlayScreen::release`) · **Spearhead** (`Ball::lead`, la última pelota
  lanzada cruza más rápido; se dibuja **dorada** — tinte + contorno + halo — para
  distinguirla).
- Combate: **Heavy knock** (`knockbackMult`).
- Sinergia elemental (gated por el nodo del elemento): **Conductor** (arco a un
  2º enemigo, electric) · **Shatter** (×daño a congelados, ice) · **Contagion**
  (un envenenado que muere contagia, poison; en `sweepDeadEnemies`) · **Bedrock**
  (rubble dura ×`bedrockLifeMult`, stone) · **Primed** (×daño a enemigos bajo
  cualquier efecto, cualquier elemento).

Botón *"Repair the core - skip this item"* (ahora también avanza de oleada — antes
se quedaba colgado) + **botón "reroll" por carta** en la Choice: cambia esa carta
por otro item elegible que no esté en la mesa, gasta una carga de
`RunState::rerollsLeft` (nodo **Foresight**, `rerollsPerLevel`/nivel).

**Fase A (2026-09-10) — tanda de items de combate + nodos de web.** Los 6
primeros son items de partida; el resto, nodos de web (append-only, `save v11`,
32 nodos). Sinergias implícitas: el pool tira 4 al azar, no hay prerequisitos.
- Items (`UpgradeKind` 25 → 31): **Cleave** (atraviesa al matar) · **Keen eye**
  (`critChance`/`critMult`) · **Battering** (`bruiserPerCruise`, daño ∝ velocidad)
  · **Executioner** (`executeThreshold`/`executeMult` vs enemigos bajos) ·
  **Overkill** (`overkillFrac`/`overkillRange`, el daño sobrante salpica) ·
  **Tempo** (`tempoRecover`, recupera crucero tras pegar). Toda la lógica cuelga
  de `World::ballDamage` y el bloque de contacto bola-enemigo (guard nuevo
  `e.hp <= 0 → continue` para no re-pegar/re-salpicar a un muerto sin barrer).
- Nodos de web: **Aegis** (`aegisHits`, el core aguanta N golpes/oleada) ·
  **Regen** (`coreRegenPerSec`) · **Bastion** (`bastionPerWavePerLevel`, +HP máx
  por oleada, vía `addCoreMaxHp` en `startNextWave`) · **Salvage**
  (`salvagePerKillPerLevel`, cores por kill, independiente de Fortune) ·
  **Interest** (`interestPerLevel`, bonus si `World::coreCleanWave()`) ·
  **Prospector** (recarga de reroll al saltear un pick) · **Stockpile**
  (`reserveFillTime`; reserva un power-up al azar, se activa con **Q** →
  `App::useReserve`/`World::useReserve`; HUD arriba-derecha) · **Magnet**
  (`magnetAccel`/`magnetMaxSpeed`, los pickups van a la pelota más cercana) ·
  **Afterglow** (`afterglowPerLevel`; los efectos continuos —Surge/Overdrive/
  SlowMo— se desvanecen vía `World::effStrength`, no cortan; Points/Golden sí
  cortan, guard `remaining > 0`) · **Charged** (`chargedFracPerLevel`, +duración
  ya cargada, en `World::activateEffect`) · **Ember** (ver arriba).
- **Web más compacta:** `Screens.cpp` `kRingGap` 72→50, `kNodeR` 14→10, `kRootR`
  20→15, `kBackRings` 5→9 — sitio para muchos más nodos.
- Pendiente Fase B: **Appraiser** (5 cartas, toca layout de `ChoiceScreen`) y los
  sub-nodos de elementos (Plague, Undertow, Permafrost, Avalanche, Tesla,
  Alchemy). Tuning de todos los `cfg::combat`/`cfg::powerup` nuevos.

**Web** (`MetaUnlock`, 32 nodos — pre-v9 resetea la web; v9→v11 es append-only,
no resetea):
Squad (max 2) · Bulwark (+20 HP) · Mend (+3 heal) · rama **Special balls** en
cadena: Ignition → Venom → Tide → Frost → Quarry → Arc (prismas, cada uno gatea
al siguiente, `maxLevel 3`: nivel 1 desbloquea el item "add X ball", 2-3 suben
`elemMult`) · Fortune (cores por kill) · Windfall (20% de 2ª prisma) ·
**Foresight** (cargas de reroll por run) · Heft (+8%, max 2) · Mass (+10%, max 2)
· rama **Power-ups**: Uplink · Capacitor · Damper/Facet/Overload · **Ledger** y
**Kinetics** (nuevos: ahora *todos* los power-ups son de web — un save nuevo no
tiene ninguno hasta comprarlos; `powerUpMask()` gatea los 5).
Fase A añade (append, indices 21-31): Aegis · Regen · Bastion · Salvage ·
Interest · Prospector · Stockpile · Magnet · Afterglow · Charged · Ember.
(2026-09-27: la web se reorganizó en **rutas por clase**, save v16, 86 nodos — ver "Web por rutas de clase" en §8.)

---

## 8. Roadmap por fases

- **Fase 0 — MVP roguelite. [IMPLEMENTADO]**
  Bucle completo jugable: Hub -> Play (oleadas) -> Choice -> ... -> RunSummary ->
  Hub. Nucleo central con vida, 1 enemigo (Errante) que va al nucleo, spawner de
  oleadas con escalado, muerte -> resumen -> nucleos -> hub con 2 desbloqueos.
  Guardado v4 (meta siempre, run reanudable con `ball i <elem>`).

- **Fase 0b — pelotas elementales + rebote libre. [IMPLEMENTADO 2026-09-01]**
  - Movimiento: **rebote libre en linea recta** por la pantalla y contra el
    nucleo solido. Sin seguir nada (se probo orbita y autoguiado, descartados).
    Flingear = redirigir, es la habilidad. `cfg::orbit` eliminado.
  - **Agujero negro / estructuras: fuera.** `FieldObject` eliminado del codigo.
  - **Pelotas elementales** (`enum class Element`): Fire (quemadura DoT), Wind
    (dispara `Projectile` al mas cercano), Water (suelta `Puddle` que daña),
    Stone (suelta `Obstacle` que bloquea enemigos). Se compran en la Choice con
    chatarra; el precio sube con el nº de pelotas.
  - Oferta "ganar chatarra": eliminada. Run empieza con 2 pelotas Plain + 25
    chatarra. Nucleo se cura `cfg::core::waveHeal` al limpiar una oleada.
  - Verificado headless: ~15 oleadas, sin NaN, elementos activos, curva de
    dificultad decreciente (afinar escalado de enemigos mas adelante).

- **Fase 1 — run acotada + progresion afuera. [IMPLEMENTADO 2026-09-03]**
  - **Empezas con 1 pelota** (+1 por nivel del unlock "Squad", tope 4).
  - **La run son 10 oleadas.** Limpiar la 10 = victoria -> volves al menu del
    juego. El nucleo muere antes = derrota -> tambien al menu del juego.
  - **Sin scrap.** Las mejoras entre oleadas son **eleccion libre 1 de 4**,
    tiradas al azar de un pool de 12 (`UpgradeKind` en `progression/Offers.hpp`):
    +pelota, encender una pelota (necesita el unlock "Ignition"), +HP nucleo,
    reparar nucleo, resorte (rebotas mas rapido en el nucleo), contraataque
    (onda al golpear el nucleo), reflejos, +30% daño, pelota grande, bala
    perdida, botin (+20% cores), segunda oportunidad.
  - **Menu del juego** (`LoadoutScreen`): gastas **cores** en unlocks
    permanentes (Squad, Bulwark, Ignition, Forge, Fortune) y arrancas la run.
  - **Cores** = `wave * cfg::meta::coresPerWave` + `winBonus` al ganar
    (`endRun(bool won)`).
  - Flujo de pantallas: **Menu -> Loadout -> Play (10 oleadas, Choice entre
    cada una) -> Loadout**. `RunSummaryScreen` eliminada (el resultado se
    muestra arriba del Loadout). Guardado **v5**: solo meta, sin reanudar run.
  - "Bala perdida" (StrayBolt) **eliminada** por completo (enum, RunMods,
    World, config) - las mejoras de disparo se rehacen con el resto mas
    adelante. Pool ahora = 11 mejoras.

- **Fase 1b — oleada 10 = duelo de miniboss. [IMPLEMENTADO 2026-09-03]**
  - Al pasar a la oleada 10, `World::startBossWave`: **arena mas ancha**
    (`cfg::boss::arenaScale*`), el **nucleo pegado a la izquierda**, y la
    **camara se aleja con transicion** (`Window` ahora tiene dos vistas:
    `uiView_` fija para HUD/menus y `worldView_` que se agranda; `App`
    interpola `camSize_/camCenter_` hacia `World::viewSize/viewCenter`).
  - **Miniboss** (`struct Boss`): sale de la mitad del lado derecho, va
    **recto** al nucleo, **inmune a knockback y sin steering**. Barra de
    vida chica encima. Si toca el nucleo -> `runOver_` (derrota directa).
    Las pelotas le pegan y rebotan pero no lo mueven.
  - **Adds infinitos** mientras el boss vive (cap `cfg::boss::maxAdds`),
    y **solo entran por la mitad derecha** de la arena (`spawnEnemy` tiene
    rama `bossWave_`). Al morir el boss **no** se limpian los adds: dejan
    de aparecer nuevos pero la oleada sigue viva hasta matar a los que
    quedan (`updateWaveSpawner`: `!boss_.alive && enemies_.empty()` ->
    `waveCleared`). Los adds siguen siendo peligrosos para el nucleo.
    La pelota se puede agarrar y mover a cualquier lado, sin restriccion
    de mitad de mapa (se probo y molestaba).
  - El borde (`Effects::drawBorder`) se dibuja como siempre en la vista UI,
    en el marco de pantalla fijo. Se probo dibujarlo a `World::size()` en
    la vista de mundo (borde real de la arena grande) pero cambiaba la
    sensacion de la camara, se revirtio.
  - **Cartel al matar al boss** (`BossWinScreen`, **opaco**): pantalla
    limpia con el resultado, sin el mundo detras, para que la camara y el
    mapeo del mouse sean los de UI normal (con la vista de mundo ancha el
    cursor no caia sobre los botones). Botones **Continue** (placeholder
    -> menu, se cablea el post-boss despues) y **Back to menu**; ambos ->
    `App::leaveBossWin`. La camara vuelve al encuadre normal al aparecer
    el cartel (condicion `simulating() && run.active`, sin cambios).
  - **Prisma** (`MetaState::prisms`, `cfg::meta::prismsPerWin`): moneda
    especial que suelta el miniboss al morir. Se guarda (`save v6`), se
    muestra junto a los cores en Menu/Loadout. Uso futuro: desbloqueos
    "grandes". Perder contra el boss (llega al nucleo) no da cartel ni
    prisma, sale directo al menu.
  - Tuning en `cfg::boss` (hp 40, radio 58, vel 54, arena 1.95x/1.45x -
    la camara se aleja bastante). La camara del boss quedo como en
    `2cbbd84`: nada la toco despues.

- **Fase 1c — transicion entre oleadas mas suave. [IMPLEMENTADO 2026-09-05]**
  - Las pelotas **ya no se reposicionan** al cambiar de oleada:
    `World::relaunchBalls` -> `carryBalls`, que mantiene posicion y rumbo
    y solo suelta la agarrada / despierta a las quietas.
  - **Ease-in de la oleada:** `App::update` alimenta el acumulador de paso
    fijo mas lento al principio y sube a tiempo real en
    `cfg::app::waveIntroTime` (desde `waveIntroSlow`), asi la escena de la
    oleada anterior fluye hacia la nueva en vez de saltar.
  - Primer enemigo con un respiro extra (`cfg::wave::introDelay`).

- **Fase 1d — agarre mas suelto. [IMPLEMENTADO 2026-09-05]**
  - `cfg::app::catchRadius` 95 -> 130: agarras la pelota estando cerca,
    no hace falta el cursor justo encima.
  - `grabAt` guarda `heldGrabOffset_ = pos - cursor` y `moveHeld(target,
    dt)` lo va disolviendo con `exp(-cfg::app::grabSettle*dt)`: la pelota
    **no salta** al cursor al agarrarla, converge en ~0.2 s. Menos
    "perseguir la pelota con el mouse", mas dinamico.

- **Fase 1e — run de 20 oleadas + "Continue" tras el miniboss. [IMPLEMENTADO 2026-09-07]**
  - `cfg::run`: se separa `bossWave = 10` de `finalWave = 20`, mas
    `coreSlideTime`. `startNextWave` enruta: `== bossWave` ->
    `startBossWave` (Charger); `== finalWave` -> `startFinalBossWave`
    (Orbital); `> bossWave` -> `startPostBossWave`; resto -> `startWave`.
  - **Gate del "Continue":** `App::continueUnlocked_` = snapshot de
    `stats.wins > 0` al empezar la run (no sube version de guardado).
    - **Primera run** (nunca se gano): matar al miniboss -> `bankRun(true)`
      + `BossWinScreen` con **solo "Back to menu"**. Cobra cores + prisma,
      vuelve al menu a desbloquear.
    - **Run 2+:** matar al miniboss -> `BossWinScreen` con **"Continue"**
      (`continuePastBoss` -> oleada 11, sin cobrar) y **"Back to menu"**
      (`leaveBossWin` -> cobra como victoria de oleada 10).
  - **Oleadas 11-20** (solo desde la run 2): se juegan en la **arena ancha
    del boss** (`wideArenaSize`, factorizada de `startBossWave`) con la
    **camara alejada mantenida** durante toda la run viva
    (`data_.run.active && !runBanked_`, incluye la Choice y el cartel).
    Al entrar a la 11 el **nucleo se desliza** de la izquierda al centro
    (`updateCoreSlide`, smoothstep sobre `coreSlideTime`); camara y arena
    quietas.
  - **Oleada 20 = boss Orbital** (`BossKind::Orbital`, comparte `struct
    Boss`; `cfg::finalBoss`). Mas chico que el de la 10 (r 46 vs 58), hp 64.
    - **Entrada legible:** aparece **fuera del borde izquierdo** y se
      desliza hacia el punto de arranque de la espiral durante `introTime`
      (invulnerable, sin espiralear todavia). El renderer dibuja un **arco
      guia tenue** con la trayectoria que va a seguir, para que se lea de
      donde viene y como se mueve.
    - Despues **espiralea largo hacia el centro** (`ang`/`dist` +
      `spiralOmega`/`spiralShrink`; arranca pegado al borde via
      `spiralStartDist` clampeado, baja lento).
    - **Dano acotado:** tras cada pelotazo queda `cfg::boss::hitCooldown`
      en i-frames, y una pelota **solo hace dano al boss si va a >=
      `cruiseBase * minHitCruiseFrac`** (0.9). Asi una pelota atrapada en
      el anillo no lo derrite, y tampoco sirve "poner el cursor encima del
      boss y clickear mil veces" (soltar la pelota le da solo `nudgeSpeed`,
      muy por debajo del umbral). Vale para los dos bosses.
    - **Anillo de enemigos** (`Enemy{orbiter=true}`, `shieldCount`) **fijado
      rigidamente** al aro que gira alrededor del boss: la posicion se
      setea directo cada frame (`e.pos = boss_.pos + dir(ringAng+phase) *
      shieldRadius`), sin lerp, asi el aro queda centrado en el boss por
      rapido que se mueva (antes se quedaba atras). Tapa las pelotas y se
      **rellena** de a uno cada `shieldRespawn`. Los orbitadores **no tocan
      el nucleo** mientras el boss vive - solo bloquean; la unica amenaza
      al nucleo es el boss llegando al centro.
    - Ademas **entran enemigos por los 4 bordes** mientras el boss vive
      (`addInterval` 1.4 / `addCap` 16 / `addHp` / `addSpeed`, mas blandos
      que un enemigo normal de la 20).
    - Matar al boss -> el anillo se suelta (`orbiter=false`, empujon hacia
      afuera `deathBurst`) y **ahi si** carga al nucleo; hay que limpiar
      todo (anillo suelto + adds) para ganar (`updateWaveSpawner`:
      `!boss_.alive && enemies_.empty()`). Boss al centro = derrota directa.
      Limpiar la 20 = victoria -> `BossWinScreen` "Run complete".
  - **Vida del nucleo:** `cfg::core::baseHp` 140 -> **60** (bajo 45 al
    principio; se subio un poco al meterle mas adds de borde a la 20).
  - **Refactor:** `endRun(bool)` -> `bankRun(bool)` (paga cores/prismas/
    stats, idempotente via `runBanked_`) + `finishToMenu()` (limpia la run
    y va al menu). El caller decide la navegacion.
  - **Mouse por vista:** `App` pasa el puntero en unidades de mundo solo a
    la pantalla que simula (Play, para agarrar/lanzar) y en unidades de UI
    (`Window::uiMousePosition`) al resto, asi menus y carteles siguen
    clicables con la camara alejada.
  - **Dificultad:** `cfg::wave` mas agresivo (`baseCount 4`,
    `countGrowth 1.24`, `maxCount 100`, `hpGrowth 1.17`, `speedMax 165`) +
    `spawnIntervalMin`: la cadencia de spawn baja hacia la oleada final
    (mas denso, no un goteo). Valores de arranque, a afinar jugando.
  - Perder en cualquier oleada 11-20 (nucleo a 0) = derrota directa al
    menu, sin cartel, cobra por oleada alcanzada.

- **Musica de fondo. [IMPLEMENTADO 2026-09-07, por acto 2026-09-28]**
  - Loops en `assets/music/`: `menu.ogg` (fuera de una run), un loop por
    acto que suena todo el acto (mapa, peleas, tienda, cartas: las peleas son
    muy cortas para cambiar en cada una) y `boss.mp3` (solo mientras corre la
    oleada del jefe; arranca siempre de cero, al ganar vuelve el del acto).
    Acto 1..5 -> `map.ogg`, `fight4.mp3` (Neon Action), `fight3.mp3`,
    `fight2.mp3`, `fight3.mp3` (tabla `kActLoop` en `App::update`).
  - **Intensidad (2026-10-03):** el loop del acto suena suave (45 %) en el
    mapa, la tienda y las cartas, y sube a pleno en ~2 s cuando arranca una
    pelea (baja en ~3 s al terminar; `Audio::setIntensity`). El menu no se
    toca.
  - **Web de habilidades (2026-10-03):** entre runs, mientras la web esta
    abierta (aunque haya opciones encima), suena `web.ogg` (Friendly Trap,
    chill; `Track::Web`). Las pantallas cortas (cartas, tienda, eventos) no
    llevan tema propio: el jugador pasa ahi pocos segundos.
  - **Jefe en dos movimientos (2026-10-03):** `boss.mp3` entra despacio (7 s)
    y bajo, y sube de 45 % a 85 % a medida que el jefe pierde vida hasta la
    furia; al enfurecerse cae `boss2.ogg` (Heavy Boss Battle 1, guitarras) a
    pleno en menos de un segundo, despues de un respiro corto. `fight1.ogg`
    (Pinball Royale) esta fuera por ahora, comentado en `App.cpp`.
  - `App::update` elige el track cada frame y llama `Audio::update(dt)`, que
    hace el cruce: el loop que sale baja en 1.5 s y el que entra sube en 2.5 s
    recien cuando el otro ya casi no suena (curva suave), asi dos tempos no
    chocan. Los que salen quedan en pausa y retoman desde ahi.
  - Cada archivo lleva una ganancia (en `App.cpp`) que lo iguala a ~-17 dBFS
    RMS; si se cambia un archivo, medirlo de nuevo.
  - Loop sin corte: los temas que terminan con fundido (`menu.ogg`,
    `map.ogg`, `fight1.ogg`) llevan un `Audio::Loop{start, end, seam}`: se
    cortan antes del fundido y vuelven al inicio con un cruce de 4 s entre dos
    copias del mismo tema (equal-power). Los que ya vienen cortados para loop
    (`fight2`, `fight3`, `boss`) usan el loop simple de SFML.
  - Archivos opcionales: si faltan, ese momento queda en silencio. SFML 2.6
    lee ogg / mp3 / wav / flac; `.m4a` no (AAC). Los originales van en
    `music/` (ignorado por git). Ride junto al resto de `assets/` en el
    copy de CMake y el `install`.

- **Fase 1f — rework de power-ups / items / web + score. [IMPLEMENTADO 2026-09-08]**
  - Detalle completo en §7. Resumen:
  - **Power-ups**: 5 (PHASE fuera, Frenesí -> OVERDRIVE). DOUBLE POINTS pasa a
    dar score de verdad; SLOW MOTION ralentiza **enemigos**, no la pelota;
    GOLDEN BOUNCE acelera el multiplicador de combo. Salen por `powerUpMask`
    en vez de un contador; SlowMo/Golden/Overdrive gated por nodos de web.
    Cadencia y duración salen de `pickupSpawnMult`/`pickupDurMult` (nodos
    Uplink/Capacitor); base deliberadamente mala.
  - **Score nuevo** (`RunState::score`, `cfg::score::perKill`): +100/kill, x2
    con DOUBLE POINTS, boss 0. HUD arriba-derecha + `stat.bestScore` (fila
    nueva en Stats).
  - **Items** (ex "upgrades"): pool de 11 -> 7. Retaliate -> **Slow field**
    (zona lenta, sin pulso de daño). Heavy impact +30% -> +8% cap 3; Big ball
    +25% -> +10%. Fuera Reinforce/Repair/Loot/Second chance. Botón "reparar
    núcleo salteando el item" en la Choice.
  - **Web** renumerada a 13 nodos: fuera Aegis/Forge/Momentum/Prospector;
    Fortune -> cores por kill; Windfall -> 20% de 2ª prisma; Squad max 2;
    Bulwark +20; Mend +3; Heft/Mass reescalados (max 2). Rama nueva
    **Power-ups** (Uplink, Capacitor, Damper, Facet, Overload).
  - **Guardado v8**: `stat.bestScore` nuevo; al cargar un save < v8 se
    descartan los niveles de `meta.unlock` (el renumber los dejaría mal
    mapeados) - cores, prismas y stats se conservan.
  - Pendiente de afinar jugando: `bountyPerKillPerLevel` (0.10, puede quedar
    alto con ~200 kills/run), fórmulas de `pickupSpawnMult`/`pickupDurMult`,
    `slowFieldMul`/`slowMoEnemyMul`, costos/parents de los nodos nuevos.

  Detalle original (Fase 0b):
  - Quitar paredes.
  - Núcleo + 1 tipo de enemigo + spawner de oleadas con escalado + derrota.
  - Daño por contacto `f(velocidad, combo)`; combo realimentado por impactos.
  - 1 estructura de campo: **agujero negro** (colocar hasta N, mover/activar en
    combate).
  - Pantalla de **Preparación** (colocar + lanzar) y de **Elección 1-de-3** entre
    oleadas (ofertas MVP: +1 pelota / +daño / +1 agujero negro / chatarra).
  - **Muerte → Resumen** (arenas/oleadas, enemigos) → **meta mínima**: ganas
    Núcleos = f(progreso); un hub con 2-3 unlocks (p. ej. "empieza con 2
    pelotas", "vida de núcleo +50%", "repulsor en la reserva").
  - **Nueva run** desde arena 1 con los unlocks aplicados.
  - Guardado v3 (solo meta entre sesiones; run en curso reanudable).
  - Debe compilar y ser jugable en bucle completo.
- **Fase 1g — 6 pelotas elementales + afinidad. [IMPLEMENTADO 2026-09-09]**
  - Detalle en §7. `Element` pasa a 7 (Plain + Fire/Poison/Water/Ice/Stone/
    Electric); `Wind`/`Projectile` fuera, `Bolt` (arco electrico) nuevo.
  - Fuego: de DoT a golpe de contacto reforzado. Nuevos: Poison (DoT que
    acumula), Ice (congela), Electric (arco a rango invisible). Water = estela
    "gusano" (`Ball::waterTrail`, ribbon que sigue la trayectoria); Stone =
    rubble que ahora hace dano.
  - Items entre oleadas: 6 items **"add X ball"** que agregan directo una pelota
    de ese elemento, gated por su nodo (se probo un modelo de "afinidad" por
    probabilidad y se descarto — mejor añadir la pelota). Pool 7 -> 12.
    `BallToFire` eliminado.
  - Web: rama Special pasa a una **cadena de 6** (Ignition -> Venom -> Tide ->
    Frost -> Quarry -> Arc), prismas, `maxLevel 3` (1 desbloquea, 2-3 suben
    `elemMult`). 13 -> 18 nodos, cada uno sobre un anillo (eje dominante entero).
    Guardado **v9** (indices corridos: pre-v9 resetea la web, conserva
    cores/prismas/stats).
  - Dev overlay: cheat-sheet de teclas fijo arriba a la derecha en `SB_DEV`
    (`App::drawDevOverlay`); en el menu/web la tecla `C` da 999999 cores+prismas.
  - Colores: plain = gris, elementales mantienen su tono al acelerar, electric
    violeta (ver §7).
  - Sonido de rebotes rehecho: notas pentatónicas por choque (pared + entre
    pelotas), el combo hace crecer la armonía/campana (ver §7).
  - `maxBalls` 8→16. Tanda grande de items nuevos + reroll por carta + power-ups
    movidos a la web (ver §7). Save **v10** (append-only sobre v9).
  - Pendiente: TODO el tuning de los items nuevos (multiplicadores en
    `cfg::combat`, se apilan y empujan al `hardSpeedCap` con 16 pelotas), costos
    de Ledger/Kinetics/Foresight, layout de la web (3 nodos nuevos, sigue
    apretada — repasar legibilidad), `cfg::element::*`, `Audio::ballHit`.

- **Fase A — tanda de items de combate + nodos de web + web compacta. [IMPLEMENTADO 2026-09-10]**
  - Detalle en §7. `save v11` (append-only, 32 nodos). Web comprimida en
    `Screens.cpp` (`kRingGap` 72→50, nodos y raíz más chicos, `kBackRings` 5→9).
  - 6 items nuevos (Cleave, Keen eye, Battering, Executioner, Overkill, Tempo);
    11 nodos nuevos (Aegis, Regen, Bastion, Salvage, Interest, Prospector,
    Stockpile+tecla Q, Magnet, Afterglow, Charged, Ember). Ember devuelve el DoT
    al fuego. Bugfix: "Repair the core - skip" ahora avanza de oleada.
  - Pendiente Fase B: Appraiser (5 cartas) y sub-nodos de elementos (Plague,
    Undertow, Permafrost, Avalanche, Tesla, Alchemy). Tuning de `cfg::combat` /
    `cfg::powerup` / `cfg::core` nuevos.

- **Fase C — pelotas como personajes: roles + 2 slots. [IMPLEMENTADO 2026-09-23]**
  - Motivo (playtest del usuario): tirar era obligatorio porque las pelotas solas
    casi no matan (sim headless: sin input, los 4 enemigos de la oleada 1 llegan
    al núcleo), y "elegir 1 de 4 pelotas/items" daba poca decisión.
  - `cfg::ball::maxBalls` 16 → **5**. Cada pelota = **rol + 2 slots de equipo**
    (`BallLoadout` en `Offers.hpp`; `RunState::balls` = un loadout por pelota,
    mismo orden que `World::balls()`).
  - **Roles** (`enum class BallRole`, `cfg::role`):
    - *Striker* — la que conviene flinguear: daño × (1 + `strikerSpeedDamage` ×
      exceso sobre su crucero), sale de la mano × `strikerFlingMult`, el fling
      decae más lento.
    - *Support* — pega poco (× `supportDamageMul`) pero **marca** (`Enemy::mark`):
      todas las pelotas y los zaps le hacen × `markDamageMul`. Su elemento rinde ×
      `supportElemMul`. Se dibuja con un anillo interior.
    - *Guardian* — grande, empuja × `guardianKnockMul` y **aturde**
      (`Enemy::stagger`: deriva con el knockback, no avanza). **Rebotes
      apuntados** (`World::aimBounce`, `guardianAimsBounces`): cada rebote sale en
      línea recta hacia el enemigo más cercano al núcleo. No es homing (nunca
      curva en el aire). Borde grueso.
    - Arranque: Striker; Squad suma Guardian y después Support.
  - **Picks entre oleadas** (`UpgradeCat`, 32 kinds): *NEW BALL* (recluta de un
    rol) · *ELEMENT* (Fire..Electric son **equipo**: la pelota es de ese
    elemento mientras lo tenga equipado; uno por pelota, ocupa slot → elemental
    vs 2 items de combate; gated por su nodo) · *BALL GEAR* (los items de
    combate, ahora por pelota; Conductor/Bedrock solo con su elemento) ·
    *RELIC* (globales: Spring core, Slow field, Strong arm, Contagion, Primed).
    Spearhead eliminado (lo cubre el Striker).
  - Gear/elemento → la Choice pasa a **modo destino**: paneles de las pelotas con
    sus 2 slots; click en slot (uno lleno se reemplaza), 1-5 por teclado, Esc
    vuelve. TAB en juego muestra el loadout + reliquias.
  - Sim: `BallMods` por pelota (`App::ballSpec` suma el equipo; nivel de forja
    ya previsto en `gearLvl` / `gearLevelBonus`), `World::syncBalls` refresca
    las pelotas en el lugar. `WorldParams` quedó con lo global (Heft/Mass web,
    reliquias, web). Heavy impact +25% / Big ball +20% por pelota.
  - Sim headless sin input: 1 guardian limpia la oleada 1 sin daño; str+guard+supp
    llega a ~4-5; 5 pelotas equipadas a ~7. Falta playtest y tuning.
  - **Próximos pasos acordados (en orden):** mapa de caminos por acto (combate /
    élite / tienda / forja / descanso, oro de partida) → feedback y recompensa por
    combo (fondo que se tiñe, monedas que saltan, más oro por jugar bien) → 4
    tipos de enemigo (corredor, tanque, escindido, blindado de frente) → rehacer
    el fling + opción de **auto-fling** para quien no quiera tirar tanto.
    Fase B (Appraiser, sub-nodos de elementos) queda en pausa.

- **Fase C2 — pelotas normales, roles como picks, 4 slots + modificadores. [IMPLEMENTADO 2026-09-23]**
  - Pedido del usuario: arrancás con pelotas **Normales** (`BallRole::Normal`); el
    rol se consigue después (carta ROLE, más adelante nodos del mapa). **4 slots
    de items** por pelota (`kBallSlots`); los **modificadores** (Heavy impact,
    Big ball, Swift nuevo, Ceiling break, Heavy knock, Reflexes) no ocupan slot
    y se apilan sin límite (`BallLoadout::mods`, `cfg::combat::*PerStack`; Big
    ball topeado en `bigBallMaxMult`). Categorías: NEW BALL · ROLE · ELEMENT ·
    ITEM · MODIFIER · RELIC (34 picks).

- **Fase D — mapa de caminos + oro. [IMPLEMENTADO 2026-09-23]**
  - `progression/RunMap.hpp`: por acto, 9 filas de 2-4 nodos en 4 carriles +
    boss (fila 10). Enlaces a carriles vecinos; todo nodo tiene entrada y
    salida (probado con 2000 mapas). Fila 1 = pelea; fila 9 = descanso/tienda/
    forja. Pesos en `cfg::map`. Mezcla medida: ~51% pelea, 11% élite, 12%
    tienda, 7% forja, 11% descanso, 8% mejora.
  - La **fila = número de oleada** (acto 2 = +10), pelees o no: la curva de
    dificultad y los bosses (10 / 20) no se mueven; un nodo sin pelea se saltea
    esa oleada.
  - Nodos: **Pelea** (paga oro) · **Élite** (`eliteHpMul`/`eliteCountMul`, paga
    el doble + elegir 1 de 4) · **Tienda** (`cfg::gold::shopOffers` picks con
    precio + reparar `repairFrac` del núcleo) · **Forja** (sube 1 item de nivel,
    tope `maxItemLevel`, cada nivel +50% del bono) · **Descanso** (núcleo a full)
    · **Mejora** (elegir 1 de 4 gratis) · **Boss**.
  - Las mejoras ya **no** salen en cada oleada: solo élite / mejora / tienda.
  - Oro de partida (`RunState::gold`, HUD arriba a la derecha): pelea
    `combatBase + perRow*fila`, élite x2, boss +`bossPay` al continuar.
  - Flujo: newRun → Play + **MapScreen** encima. Nodo → `App::travelTo`. Tras
    una pelea → mapa (o Choice si fue élite). Tras el miniboss "Continue" →
    mapa del acto 2 (el mapa se dibuja de abajo hacia arriba: la fila 1 es **un solo nodo de pelea**, el tronco, que abre a todas las ramas de la fila 2; boss arriba). Selector de pelota/slot sacado a **EquipScreen**
    (`EquipSource` Choice / Shop / Forge). Pantallas nuevas en `ui/RunScreens.cpp`;
    los paneles de loadout viven en `ui/Widgets`.
  - Falta: playtest (la UI no se pudo clickear acá), precios y pagos de oro,
    nodos de mapa que den roles (pedido del usuario), eventos.

- **Fase E — tooltips + feedback y recompensa por jugar bien. [IMPLEMENTADO 2026-09-23]**
  - Tooltips (`drawTooltip` en `ui/Widgets`) en casi todo: nodos del mapa y su
    leyenda, cartas de la Choice (qué es cada categoría) y reroll, ofertas y
    reparación de la tienda, slots / rol / modificadores de cada pelota (Equip y
    TAB), reliquias, HUD (combo, score, oro, reserva, power-up, núcleo) y el
    contador de pelotas. Textos cortos: `roleDesc`, `powerUpDesc`,
    `upgradeCatDesc`.
  - **Fondo que se calienta** con el combo (`App::heat_` → `theme::bgHot`, sube
    rápido y se enfría lento, `cfg::app::heat*`).
  - **Monedas**: cada kill suelta una moneda (`Effects::addCoin`) que vuela al
    contador de oro; más grande cuanto más alto el combo; el contador late al
    llegar (`Hud::pulseGold`).
  - **Oro por kill** × (1 + `comboBonusPerTier` × tier del combo) → jugar bien
    paga más. `combatBase` 12 → 8 para compensar.
  - **Multi-kill**: ≥3 kills dentro de `multiKillWindow` → cartel "xN MULTI-KILL"
    + oro extra + monedas. **Oleada limpia** (sin daño al núcleo) → bonus de oro.

- **Fase F — tipos de enemigo. [IMPLEMENTADO 2026-09-23]**
  - `enum class EnemyKind` + `cfg::enemy`. Cada tipo pide otra respuesta:
    **Runner** (rápido, frágil, chico) · **Tank** (lento, x3.6 vida, grande,
    casi no se empuja, pega x2 al núcleo) · **Splitter** (al morir suelta 2
    **Shards**) · **Shielded** (escudo del lado del núcleo, `shieldArc`: los
    golpes de frente rebotan sin daño; hay que pegarle de costado/atrás; la
    **Guardiana lo atraviesa**, congelado no se cubre) · **Grunt** (el de siempre).
  - Aparecen de a poco (`runnerWave` 2, `splitterWave` 4, `tankWave` 5,
    `shieldWave` 6) con pesos por spawn; las élites suman tanques y blindados.
    Los adds del Charger siguen siendo grunts.
  - Dibujo: runner/shard más claros y chicos, tank oscuro con borde grueso,
    splitter con una grieta, shielded con un arco brillante hacia el núcleo.
    Tooltip al pasar el cursor por un enemigo (nombre + qué hacer).
  - Nodo de mapa **Recluta** (`MapNodeType::Recruit`, pedido del usuario: roles
    desde los caminos): elegir entre pelota nueva / Striker / Support / Guardian
    (con 5 pelotas, la primera carta pasa a ser un modificador al azar).

- **Fase G — tiro nuevo (gomera) + auto-tiro. [IMPLEMENTADO 2026-09-23]**
  - Motivo: el tiro "flick" se sentía raro y había que tirar apurado.
  - **Gomera** (default, `MetaState::slingshot`): click en una pelota → queda
    quieta; tirás hacia atrás y se ve una banda + línea punteada hasta la
    primera pared; al soltar sale en esa dirección con fuerza según el estirón
    (`slingMinSpeed..slingMaxSpeed` en `slingMaxPull` px). Un estirón corto
    (`slingDeadzone`) cancela y la pelota sigue como venía (`World::cancelHeld`).
    **Cámara lenta al apuntar** (`aimTimeScale` 0.3, hasta `aimSlowMax` 2.5 s).
  - El flick viejo queda como opción ("Aim: Flick" en pausa).
  - **Auto-tiro** (opción en pausa, `MetaState::autoFling`,
    `World::updateAutoFling`): cada `autoFlingInterval` lanza la pelota más
    "quieta" hacia el enemigo más cercano al núcleo a `autoFlingSpeedMul` x
    crucero. Sim sin input: 3 roles pasan de oleada ~3 a ~5; ayuda, no gana solo.
  - Las dos opciones se guardan (`aim.slingshot`, `aim.auto`; sin bump de
    versión) y tienen tooltip en la pausa.

- **Fase H — arena ancha a la misma velocidad + mapas más largos. [IMPLEMENTADO 2026-09-24]**
  - Problema (usuario): con la cámara alejada (oleada 10 en adelante) las
    pelotas se veían lentas y casi no cambiaban de dirección.
    `World::arenaScale()` (1 en la arena normal, ~1.95 en la ancha) multiplica
    el crucero, el techo y los tiros (gomera incluida); el radio crece
    `ballRadiusArenaFrac` de eso. El daño, el sonido, el rastro y la stat de
    velocidad leen la velocidad **en pantalla** (dividida por la escala), así
    no pega más solo por ir más rápido en unidades del mundo. Sim: 300 px/s en
    pantalla en las dos arenas.
  - Mapas de **14 filas** por acto (`cfg::map::rows`) + boss. `mapRowWave`
    reparte las 9 oleadas del acto entre las filas (1 2 2 3 4 4 5 6 6 7 8 8 9 9,
    boss = 10 / 20): la curva y los bosses no se mueven, hay más paradas para
    armar la composición (~40 nodos por acto). HUD y cartel muestran
    "Act N - Stage R / 15". Nodos un poco más chicos; el click toma el más cercano.

- **Fase I — sinergias: roles por items, reacciones, procs y reliquias fuertes. [IMPLEMENTADO 2026-09-25]**
  Norte (usuario): priorizar diversión; inspiración Isaac / Slay the Spire —
  combinaciones que, bien ordenadas, se vuelven **rotas y catastróficas**, con
  probabilidades en el medio.
  - **Roles por items.** Cada item tiene una etiqueta (STRIKER / GUARDIAN /
    SUPPORT). Una pelota toma el rol de la etiqueta con **2+ items**; con **4**
    llega a la **maestría** (efecto nuevo). Las cartas ROLE desaparecen; el
    nodo Recluta ofrece pelota nueva + un item de cada etiqueta.
    - Maestría Striker: los golpes por encima del crucero sueltan una onda
      expansiva (más grande cuanto más rápido, hasta 2.5x el radio).
      Guardian: al rebotar en el núcleo suelta un pulso que empuja y aturde. Support: la marca se contagia a los enemigos cercanos.
  - **Reacciones entre pelotas.** Un enemigo guarda el último elemento que le
    aplicó una pelota. Si **otra** pelota con **otro** elemento lo golpea, se
    dispara una reacción (y consume el estado):
    fuego+hielo **Estallido** (explosión, x2 a congelados) · fuego+veneno
    **Combustión** (el veneno restante explota en área) · veneno+eléctrico
    **Plaga** (arco que salta a 4 y los envenena) · agua+eléctrico
    **Electrocución** (todos los que tocan alguna estela reciben descarga) ·
    agua+fuego **Vapor** (nube: aturde y daña en área) · hielo+eléctrico
    **Superconductor** (quebradizos: +daño recibido) · cualquier otro par
    **Choque** (pequeña explosión).
  - **Procs (probabilidades):** Keen eye (crítico), **Echo** (el golpe se
    repite), **Tesla** (descarga a 3), **Bomber** (al matar, explota),
    **Split shot** (al rebotar en pared crea una pelota fantasma temporal que
    copia los items). Todos pasan por la **suerte** global.
  - **Reliquias nuevas, fuertes:** **Catalyst** (reacciones x2 y más grandes) ·
    **Chain Reaction** (una reacción puede repetirse en otro enemigo afectado →
    cascadas) · **Lucky Clover** (todas las probabilidades x1.6) · **Glass
    Cannon** (todo el daño x1.6, núcleo -30% vida máx.) · **Magnetic Core**
    (todas las pelotas salen del núcleo apuntando al enemigo más cercano).
  - Combos pensados para romperse: Split shot + Cleave + Bomber (fantasmas que
    atraviesan y explotan) · Chain Reaction + Catalyst + 3 elementos distintos ·
    Echo + Executioner + Keen eye · Support maestro + Superconductor.
  - Implementación: `ItemTag` / `itemTag()` en `Offers.hpp`; `BallLoadout::role()`
    y `mastery()` salen de los tags (desempate: el tag del slot más temprano).
    `World::strike` concentra un golpe (daño, estados, procs, reacción);
    `applyElement` / `triggerReaction` (el elemento también **salpica** a los
    enemigos "limpios" a `elemSplash`, y las descargas eléctricas y la estela de
    agua también aplican su elemento); `damageEnemy` aplica "quebradizo";
    fantasmas en `World::ghosts()` (no se sincronizan con el loadout, mueren al
    cambiar de oleada). Todo el tuning en `cfg::synergy`. Los items en las
    cartas muestran su tag con color; el panel de cada pelota muestra el rol
    (+ = maestría) y el tooltip cuenta los tags.
  - Medido sin jugador (12 corridas, 4 pelotas de fuego/veneno/agua/eléctrico,
    oleadas 5-9): ~10 reacciones/min, ~0.6 por kill. Performance: 1-2 µs por
    paso incluso con cascadas. `supportDamageMul` 0.6 → 0.8 (sus items de
    Bomber/Tesla/Split shot necesitan matar).
  - Forja: cada nivel sube la probabilidad / bono del item (x1.5, x2) **y** +10%
    de daño a la pelota (`forgeDamagePerLevel`), así forjar un item de sí/no
    (Cleave, Rampart...) nunca se desperdicia.
  - Falta playtest: frecuencia de reacciones con el jugador tirando, números de
    procs, precio de las reliquias nuevas.

- **Fase J — tiers + items que cambian la pelota + reliquias legendarias. [IMPLEMENTADO 2026-09-25]**
  Pedido (usuario): "no me es muy diferente si me sale cualquiera" → pocos items
  que cambien drásticamente la partida y muchos chicos que aporten.
  - **Tiers** (`Tier`, `upgradeTier`): Common (gris) · Uncommon (verde) · Rare
    (azul) · Epic (violeta) · Legendary (dorado). Cada carta tira primero un tier
    (`cfg::tier::weightsNormal` 45/30/16/7/2, `weightsElite` 18/32/28/15/7,
    `weightsBoss` solo Epic/Legendary) y después un pick de ese tier
    (`App::rollPick`, con fallback al tier vecino). Lucky clover corre las
    probabilidades un tier para arriba. Precios de tienda por tier
    (`priceByTier` 30/45/65/95/150). Cartas con marco del color del tier; Epic
    y Legendary con un halo que late.
  - **Tesoro de boss**: al "Continue" después del miniboss, una elección de
    1 de 4 **solo Epic/Legendary**.
  - **Items que cambian la pelota** (`cfg::changer`): **Seeker** (L, curva hacia
    el enemigo más cercano) · **Railgun** (L, cada rebote en pared dispara un haz
    que pega a todo en línea, x2) · **Satellite** (L, deja de rebotar y orbita el
    núcleo moliendo lo que se acerca, x1.8; no se agarra) · **Gravity well** (L,
    arrastra enemigos hacia la pelota: arma racimos para reacciones) · **Gemini**
    (L, gemelo fantasma permanente con los mismos items) · **Piercing** (E,
    atraviesa enemigos) · **Storm** (E, descargas constantes alrededor) ·
    **Berserk** (R, +15% por golpe seguido, se resetea en pared) · **Giant** (R,
    x1.8 tamaño, x1.3 daño, más lenta) · **Midas** (R, +3 oro por kill).
  - **Reliquias**: **Prism core** (L, las pelotas sin elemento dejan uno al
    azar en cada golpe → reacciones por todos lados) · **Phoenix** (E, una vez
    por acto el núcleo vuelve con 50%) · **Time dilation** (E, enemigos -25%
    velocidad) · **Overcharge** (R, el combo sube al doble).
  - Sim sin jugador (2 pelotas, 8 corridas): Seeker 2 → 22 kills, Gemini+
    Satellite ~20, Satellite 13, Storm 9; ningún NaN, <1.5 µs por paso.

- **Fase K — web con contenido nuevo + tipografía + feedback + modo foto. [IMPLEMENTADO 2026-09-25]**
  - **Modo foto (dev):** `SB_SNAPSHOT=<carpeta> ./space_breakers` arma cada
    pantalla (menú, web, mapa, pelea, cartas, tienda, selector, TAB), guarda un
    PNG de cada una y se cierra (`App::runSnapshots`). Usa un save aparte en esa
    carpeta: el save real no se toca. Sirve para revisar la UI sin jugar (en
    este entorno el display cierra las ventanas a los segundos).
  - **Tipografía:** Lato (OFL, `assets/fonts/`, licencia incluida). Lato Bold
    para la UI; Lato Black automática para todo texto de tamaño `fsHeading` o
    más (`setTitleFont` en Widgets). Arial queda de fallback.
  - **Web (save v12, append-only):** anillos más separados (`kRingGap` 36→56),
    nodos más grandes, se ven los nombres de la frontera comprable y los
    puntitos de nivel, leyenda de ramas, Start/Back abajo a la derecha. Rama
    nueva **Arsenal** (prismas): Armory (+50% odds de Epic por nivel) y los
    desbloqueos de 4 legendarios (Satellite, Gravity well, Gemini, Prism core;
    bloqueados hasta comprarlos vía `UpgradeCtx::locked`; Seeker y Railgun
    siempre disponibles). Economía: Lucky star (odds de tier), Haggler (-10%
    precios/nivel), Starter kit (item gratis al empezar: Uncommon, luego Rare),
    Elite spoils (+50% oro de élite/nivel). Combate pasó a rosa para no
    confundirse con Power-ups.
  - **Feedback:** título del menú grande con halo que respira; cartas que se
    levantan al pasar el cursor; contador de oro que "rueda"; picks Epic /
    Legendary con destello y cartel en su color; reacciones con un pequeño
    sacudón de cámara; paneles de pelotas con fondo sólido.

- **Fase L — refresco visual + animaciones + panel de dev. [IMPLEMENTADO 2026-09-25]**
  - **Kit de dibujo** `render/Draw.*` (`draw::glow` aditivo, `disc` con gradiente
    radial, `polygon`, `ring`/arcos, `box` redondeada con gradiente vertical).
  - **Pelotas:** brillo suave, cuerpo con gradiente, borde, reflejo especular,
    cola tipo cometa, anillo extra si tienen maestría, aparecen con rebote.
    **Enemigos:** forma por tipo (runner/shard = triángulo que apunta hacia
    donde va, tank = hexágono que gira, el resto círculos), gradiente, brillo,
    arco de vida alrededor (solo si están heridos), aparecen con rebote
    (`Enemy::age`) y al morir revientan en un destello (`Effects::addPop`).
    **Núcleo:** halo que respira, vida como arco, tres segmentos que giran.
  - **Recuadros:** cartas, paneles, slots, tooltips, botones e info de la web
    con gradiente suave y **esquinas rectas** (el usuario prefirió estética recta;
    `theme::corner` = 0 controla el radio en un solo lugar). Brillo de pelotas,
    enemigos y núcleo bajado a pedido ("mucho brillo").
  - **Panel de dev (F1 en pelea, `SB_DEV`):** `ui/DevScreen.cpp`. Pausa la
    pelea. Todos los picks en columnas con color de tier (click = dárselo a la
    pelota objetivo, 1-5 para elegirla), spawnear cada tipo de enemigo,
    velocidad 0.25x-4x (`App::devTimeScale_`), abrir tienda / forja / pick
    normal / élite / tesoro de boss / recluta, saltar al boss, ganar oleada,
    matar todo, curar, invulnerable, +100 oro, sumar / vaciar pelota. Sale en el
    modo foto como `09_dev.png`.

- **Fase M — identidad visual: "consola orbital". [IMPLEMENTADO 2026-09-26]**
  - **Estilo:** un tablero de instrumentos oscuro (tinta azul marino). Paneles
    de vidrio rectos con borde de un pelo, una línea de luz arriba y
    **corchetes en las esquinas**; **etiquetas chicas en mayúscula con
    espaciado** (`makeLabel`/`drawLabel` en `ui/Widgets`) para títulos de
    sección, unidades y leyendas; texto de cuerpo sigue en Lato normal. Todo lo
    estructural queda en bajo contraste: la pelota sigue siendo lo más brillante.
  - **Dónde vive:** paleta y constantes en `core/Theme.hpp` (`bg`, `bgDeep`,
    `grid`, `glassTop/Bottom`, `bracket`, `tracking`; `corner` sigue en 0).
    Kit en `render/Draw.*`: `line`, `brackets`, `panel` (el panel de la casa),
    `vignette`, `radar`. Fondo común en `render/Backdrop.*` (grilla de puntos +
    viñeta, lo dibuja `App::render` en todas las pantallas).
  - **Arena:** un **radar** tenue (anillos + rayos) centrado en el núcleo; marco
    de la arena con corchetes y muescas al medio de cada lado (la pared golpeada
    se ilumina entera). Núcleo = reactor con la vida como dial segmentado y un
    bisel de marcas que gira lento. Enemigos = **casco oscuro con borde nítido**
    (nunca más brillantes que una pelota): grunt disco con ojo, runner/shard
    dardo que apunta a donde va, tank doble hexágono, splitter con grieta en
    zigzag, shielded con arco doble hacia el núcleo. Jefe = octógono blindado
    con anillo interno y ojo, barra de vida segmentada con corchetes. Power-ups
    = rombos (lo único cuadrado de la arena). Pelotas: brillo más bajo todavía,
    cola como estela afinada en vez de discos apilados; la pelota agarrada lleva
    corchetes de "fijado". Rayos eléctricos y railgun con núcleo brillante.
  - **HUD:** "ACT 1  STAGE 3 / 15" en etiquetas, barra del núcleo segmentada con
    topes, "N LEFT"; SCORE / GOLD como lecturas con leyenda; combo en un chip
    con corchetes; tecla [TAB] y [Q] dibujadas como teclas.
  - **Pantallas:** menú con título en mayúsculas espaciadas sobre una regla con
    marca de acento, radar y pelotitas con estela de fondo; las filas del menú
    se enmarcan con corchetes al pasar el mouse. Mapa: nodos en **rombo** (jefe
    octógono), reglas y números de fila a la izquierda (la fila actual en
    acento), caminos abiertos con **guiones que avanzan**, "YOU" con corchetes
    que respiran. Cartas (elección / tienda): vidrio teñido por el tier, franja
    del color del tier arriba, corchetes que **se cierran al aparecer**; Epic y
    Legendary con un segundo juego de corchetes que respira. Tooltips con lomo
    de color. Web, panel de dev, stats y botones con el mismo panel/etiquetas.
  - Modo foto suma `10_horde`, `11_map_late`, `12_boss`, `13_pause`, `14_stats`.

- **Fase N — items que definen la pelota + niveles por duplicado. [IMPLEMENTADO 2026-09-26]**
  Pedido (usuario): pocos items aburridos pero que mejoren si los tomás varias
  veces, y items que hagan a cada pelota **completamente distinta**.
  - **Niveles por duplicado.** Tomar un item (o elemento) que la pelota ya tiene
    **lo sube de nivel** en vez de ser inelegible: `upgradeFitsBall` lo acepta
    hasta `kMaxItemLevel` = **5** y `applyUpgradeKind` sube `gearLvl` del slot
    que ya lo tiene (cartel "Item Lv N"). La forja usa el mismo nivel y el mismo
    tope (`cfg::gold::maxItemLevel` quedó sin uso). Cada item escala a su manera
    (`App::ballSpec`: valor de nivel 1 + `...PerLevel` × (nivel − 1), constantes
    en `cfg::combat` / `synergy` / `changer`) y todo nivel extra además suma +10%
    de daño a la pelota (`itemLevelDamage`). Un elemento repetido es +30% de
    potencia (`BallMods::elemMult`). Texto: la carta dice "Rare - Lv 2 -> 3" si
    alguna pelota lo tiene; en el selector, al pasar por esa pelota, el título
    dice "Lv 2 -> 3" y abajo qué da el nivel (`upgradeLevelDesc`); el tooltip del
    slot muestra "Lv N/5" + el próximo nivel.
  - **Recortes / fusiones.** Fuera **Wall rush**, **Carom**, **Warm-up**,
    **Tempo** y **Battering**. Wall rush se fusionó en **Ricochet** (Common,
    Striker: cada rebote en pared acelera y arma un golpe fuerte); Carom lo
    reemplaza **Bumper**. **Keen eye** baja a Common (12% +7%/nivel). Los
    modificadores pasan de 6 a 3: **Heavy impact**, **Big ball** (+radio y
    +empuje; absorbe Heavy knock) y **Swift** (+crucero, +techo y el tiro dura
    más; absorbe Ceiling break y Reflexes). Bugfix: Cleave ahora sí atraviesa
    (antes el choque ya había reflejado la velocidad) y su golpe cuenta para el
    combo.
  - **Items nuevos** (`UpgradeKind` 56 → 57; los `BallMods` pasaron a números ya
    escalados por nivel, 0 = no equipado):
    - **Hunter** (Epic, Striker): se traba en la mayor amenaza (vida pesada por
      cercanía al núcleo, `Enemy::id`) y dobla fuerte hacia ella hasta matarla;
      x1.25 de daño a su presa. Dos arquitos giran sobre la presa.
    - **Comet** (Rare, Striker): tirada sale x1.5, techo x1.6, el tiro casi no
      decae, y por encima de x1.9 del crucero **atraviesa** enemigos.
    - **Mitosis** (Rare, Striker): cada kill suelta una copia chica (fantasma,
      x0.62, ~2.6 s) con sus items; 2 copias en Lv3, 3 en Lv5; las copias no se
      dividen.
    - **Boomerang** (Rare, Guardian): tras golpear vuelve curvando al núcleo;
      al rebotar ahí sale apuntada a la amenaza, más rápida y con el próximo
      golpe **cargado** (x1.4).
    - **Bumper** (Uncommon, Guardian): más grande; las pelotas que chocan contra
      ella salen x1.35 más rápido (ideal para Strikers), empuja fuerte.
    - **Glutton** (Rare, Guardian): cada kill de la oleada la agranda y le suma
      daño (tope 10 stacks; se resetea al cambiar de oleada).
    - **Tether** (Epic, Support): láser a la pelota más cercana (gemelos y
      copias cuentan) que quema lo que cruza y deja el elemento de la pelota.
      Línea fina que late.
    - **Black hole** (Epic, Support): 35% de que una kill deje un agujero negro
      que chupa ~1.5 s y revienta con el elemento de la pelota (x1.5 del golpe).
    - **Resonance** (Epic, Support): cada golpe manda un arco a **todas** las
      pelotas del mismo elemento (gemelos y copias incluidos) y cada una zapea al
      enemigo más cercano con ese elemento.
    - Todos los ítems viejos también escalan: Conductor salta a un enemigo más
      por nivel, Tesla +1 objetivo, Overkill llega a 2-3 vecinos, Gemini 2
      gemelos en Lv3 y 3 en Lv5, Midas +3 oro por nivel, etc.
  - **Sinergias pensadas:** Tether + Satellite (láser que barre alrededor del
    núcleo) · Tether + Gemini (el láser une a la pelota con sus gemelos) ·
    Resonance + 3 pelotas del mismo elemento + Gemini / Mitosis (telaraña de
    rayos) · Mitosis + Cleave + Bomber (copias que atraviesan y explotan) ·
    Black hole + dos elementos + Catalyst (junta el racimo y lo hace reaccionar) ·
    Bumper + Comet/Ricochet Striker (pinball: sale disparada y el Striker pega
    por velocidad) · Hunter + Berserk + Executioner (no toca pared, apila Berserk
    sobre una sola presa).
  - Dibujo mínimo en `WorldRenderer` (el otro agente reestiliza): línea del
    Tether, agujero negro (disco oscuro + aro que se cierra), traba del Hunter.
  - **Sim headless** (3 pelotas, oleadas 1-9, auto-tiro, 16 corridas; ±0.4 de
    ruido): base 5.3 oleadas / 43 kills · Hunter L1 6.9 / L5 8.5 (Seeker de
    referencia 7.4) · Mitosis L1 5.4 / L5 6.8 · Tether L1 5.4 / L5 6.3 · Black
    hole L1 5.4 / L5 6.1 · Boomerang 6.0 · Mitosis+Cleave+Bomber 7.1 · Resonance
    fuego x3 + Gemini 7.3 · Gemini L5 + Tether 8.3. Sin auto-tiro: base 2.3,
    Hunter L1 4.7 (Seeker 4.5), Satellite+Tether 4.2. Ningún NaN; ≤2 µs por paso
    (Gemini L5 + Tether el más caro, ~1.7 µs).
  - Falta playtest: Comet y Bumper solo brillan tirando (el sim casi no tira);
    Hunter puede sentirse "autopiloto" (bajar `hunterTurn` si pasa).

- **Fase O — pactos de boss, tiendas y web ampliada. [IMPLEMENTADO 2026-09-26]**
  Pedido (usuario): al ganarle a un boss, una mejora GLOBAL fuerte que te haga
  elegir una ruta clara (tirar mucho / mirar lo que hacen las pelotas / pocas
  pelotas muy mejoradas...), "reglas locas que sean divertidas de ver"; tiendas
  más interesantes; la web se estaba quedando chica.
  - **Pactos** (`progression/Pacts.hpp`, 12): después del miniboss, al darle
    "Continue", elegís 1 de 3 (4 con el nodo **Oath**) — cada carta es un
    arquetipo distinto — y recién después viene el tesoro de boss. Con el nodo
    **Covenant** también elegís uno al empezar la run (máx. 2 por run). Se
    pueden rechazar todos por `refuseGold` de oro. Cada pacto trae su costo:
    | Pacto | Arquetipo | Te da | Te cuesta |
    |---|---|---|---|
    | Hot Hands | Tirador | tiros x1.7, techo de velocidad x3, mantienen la velocidad | sueltas, las pelotas van 35% más lento |
    | Nova | Tirador | ESPACIO / click derecho: todas salen del núcleo en anillo a x3 y el núcleo empuja (7 s) | núcleo -20% vida máx. |
    | Hunters | Espectador | cada pelota elige su presa y la persigue hasta matarla (+20% daño) | no podés agarrar pelotas |
    | Clockwork | Espectador | auto-tiro cada 0.6 s a x2.5 (+15% daño) | no podés agarrar pelotas |
    | Pinball | Espectador | las paredes son bumpers: aceleran, suman combo y chispean | tus tiros 40% más débiles |
    | Duet | Pocas pero fuertes | tus 2 mejores pelotas absorben al resto (items → niveles de forja, modificadores pasan), 2 items de un tag = maestría, x1.5 daño, +20% tamaño | nunca más de 2 pelotas |
    | Legion | Enjambre | +2 pelotas con un item cada una; los choques entre pelotas chispean y suman combo | -25% daño |
    | Living Core | Núcleo | el núcleo electrocuta cada 0.8 s; rebotar en el núcleo sobrecarga la pelota (+60% daño 2 s) | enemigos +15% velocidad |
    | Fortress | Núcleo | núcleo x1.75 vida; lo que lo toca explota y empuja | no cura antes de pelear, los descansos curan la mitad |
    | Loaded Dice | Apostador | toda probabilidad x2, las cartas salen un tier más arriba | el oro de cada pelea: doble o nada |
    | Alchemy | Alquimista | la mitad de los golpes deja un elemento extra al azar y una pelota reacciona consigo misma | -25% daño |
    | Bloodlust | Berserker | el combo no se enfría y sube el doble de rápido | lo que llega al núcleo borra el combo y pega +50% |
    Incompatibles (`pactsConflict`): Duet/Legion, Hot Hands con los que te
    sacan las manos, Hunters/Clockwork entre sí. Hunters, Legion, Loaded Dice y
    Alchemy se desbloquean en la web.
  - Sim: `sim/PactRules.hpp` (`WorldParams::pact`) + `sim/WorldPacts.cpp` (cada
    hook es un no-op sin su pacto; World.cpp solo los llama en una línea). El
    resto (daño, crucero, suerte, cantidad de pelotas, vida del núcleo) lo
    pliega `App::foldPacts`. Medido sin jugador (6 semillas x oleadas 5/9/13/17):
    sin pacto 78 kills / 13 derrotas de 24; Hunters ~1000 / 6 y Clockwork ~830 / 6
    (reemplazan tus tiros); Living Core 377, Fortress 773 / 9, Pinball 173;
    sin NaN, <2 µs por paso (5 µs con las 5 pelotas de Legion).
  - UI: `ui/PactScreen` (cartas grandes con GANA / CUESTA y tooltip "combina
    con..."), chips de pactos en el mapa, la tienda, el TAB y abajo a la
    izquierda en pelea (con la carga de Nova). Hunters dibuja una línea tenue
    de cada pelota a su presa; Living Core un anillo en la pelota sobrecargada.
  - **Tiendas:** una oferta siempre en **oferta** (-40%, más con Merchant) ·
    **caja misteriosa** (55 de oro, sale con odds de élite y queda pagada en el
    estante) · **reroll** del stock (12 + 6 por reroll) · **vender** un item
    (45% de su precio por tier x nivel, libera el slot) · **forja paga** (45).
    Tooltips en todo.
  - **Web (save v13, append-only, 51 nodos):** rama nueva **Pacts** (carmesí,
    abajo a la derecha desde Heft): Oath, Covenant y los desbloqueos de Hunters,
    Legion, Loaded Dice y Alchemy. Más: **Merchant** (+1 oferta y oferta más
    profunda), **Treasury** (+20 de oro inicial por nivel), **Quartermaster**
    (el Starter kit se elige de 4 cartas al empezar), **Last stand** (una vez
    por run el núcleo vuelve con 50%). La web ahora se **arrastra** y hace
    **zoom** con la rueda (0 resetea); la leyenda muestra comprados/total por
    rama y al pasar el cursor por una rama la resalta.
  - Inicio de run: `App::advanceRunIntro` (pacto de Covenant → carta de
    Quartermaster → mapa).
  - Panel de dev: sección PACTS (click = dar / sacar cada pacto) y "Pact choice
    (boss / start)". Modo foto: `10_pact` … `18_intro_map`.
  - Falta playtest: números de cada pacto con el jugador tirando, sobre todo
    Hot Hands, Duet y Bloodlust.
  - Merge con la Fase N: el tope de forja es `kMaxItemLevel` (5) para todos, así que Duet perdió su "forja hasta Lv5" (`App::forgeCap` quedó fijo).

- **Parada antes del boss. [IMPLEMENTADO 2026-09-26]** (pedido del usuario:
  "antes de un boss te haga elegir siempre entre tienda, rest, upgrade o
  recruit")
  - La última fila del mapa (fila 14) es fija: **Tienda · Descanso · Mejora ·
    Recluta**, uno por carril y siempre en ese orden (`kPreBossRow` en
    `progression/RunMap.hpp`). Nunca hay pelea ni élite ahí.
  - Todo nodo de la fila 13 enlaza a los cuatro, así que las cuatro opciones
    están siempre al alcance; los cuatro van al boss. Reemplaza la vieja fila
    9 de descanso/tienda/forja al azar. Probado con 5000 mapas (entradas,
    salidas, 4 nodos sin pelea, acceso a los 4 tipos).
  - El mapa no se guarda en el save: no cambia la versión.

- **Tiro rápido. [IMPLEMENTADO 2026-09-26]**
  - Motivo (usuario): apuntar cada tiro cansa; casi siempre la pelota va al
    enemigo más cercano y el rebote fino es situacional.
  - **Dos formas de tirar, sin opciones** (pedido del usuario, corrección del
    mismo día): **click** en una pelota (soltar sin mover el puntero más de
    `quickThrowSlop` 12 px, no importa cuánto la mantengas) la tira al
    **enemigo vivo más cercano a la pelota** (o al boss), a `quickThrowPower`
    0.65 del rango de la gomera (~1090 px/s; la fuerza máxima queda para el
    tiro apuntado). Sin enemigos, la pelota sigue como venía. **Apretar y
    estirar** apunta con la gomera; la cámara lenta y la banda aparecen recién
    al mover.
  - Se sacaron el modo **flick** y la opción **Auto-throw** de la pausa (y
    `aim.slingshot` / `aim.auto` del save; un save viejo los ignora). El
    auto-tiro queda solo para el pacto Clockwork.
  - Usa el mismo `World::releaseHeld` que el tiro a mano: Striker, Comet,
    Strong arm, Hot Hands / Pinball y el sonido aplican igual. Hunters y
    Clockwork siguen sin dejar agarrar.

- **Premio por jugar limpio: Flawless + Iron core. [IMPLEMENTADO 2026-09-26]**
  Pedido (usuario): más recompensa si terminás la oleada sin golpes en el
  núcleo o pasás un mapa sin reparar el núcleo.
  - **Flawless** (reemplaza la "oleada limpia" de la Fase E): pelea sin que
    nada toque el núcleo (un golpe que absorbe Aegis también cuenta como
    golpe) → `flawlessBase + flawlessPerRow * fila` = 7..23 de oro (~75% de lo
    que paga la pelea), x2 en élite. Va aparte del doble-o-nada de Loaded
    Dice. Cartel "FLAWLESS +N gold" debajo del oro de la pelea. Boss del
    acto 1 flawless → +`flawlessBoss` (30) de oro para el acto 2; el boss final
    no paga oro (la run termina).
  - **Iron core**: ganarle al boss de un acto sin **ninguna reparación
    elegida** en ese acto → +`cfg::meta::ironCoreCores` (15) núcleos, que se
    suman a `bountyCores` (se cobran aunque después pierdas la run). Rompe la
    racha: nodo de descanso, reparar en la tienda y "reparar en vez de tomar
    el item" — solo si de verdad curan algo (descansar con el núcleo lleno no
    cuenta). No la rompen las curas automáticas: la de antes de cada pelea,
    Regen, Mender, Bastion, Phoenix / Last stand. Fortress la hace más difícil
    (sin cura gratis), es parte de su costo. `RunState::repairedThisAct`, se
    reinicia al empezar el acto 2. La run no se guarda a mitad → **sin cambio
    de save** (sigue v13).
  - UI: "iron core" chiquito bajo oro/núcleo en el mapa mientras la racha
    sigue (tooltip explica), los tooltips de descanso / reparar / saltear
    avisan que la terminan, y la carta de BossWin muestra "FLAWLESS +30 gold
    · IRON CORE +15 cores". Modo foto: `19_boss_bonus`.
  - Números: 15 núcleos ≈ 7 oleadas de núcleos o un 30% de una run ganada;
    falta playtest.

- **TAB pausa la pelea y también anda en el mapa. [IMPLEMENTADO 2026-09-26]**
  - En pelea, mientras el overlay de TAB (pelotas + items + reliquias + pactos)
    está abierto, la simulación se congela (`Screen::frozen()`: no corre el
    mundo, ni timers, ni el combo, ni el banner de etapa). Dice "paused". Si
    tenías una pelota agarrada se suelta sin tirarla; clicks, Q, espacio y Esc
    no hacen nada hasta cerrarlo (Esc lo cierra).
  - En el mapa, TAB muestra el mismo overlay (`drawLoadoutOverlay`) y bloquea
    elegir nodo mientras está abierto. Hint `[tab] loadout` bajo la leyenda.
  - Regla única (`TabPeek`): **mantener** TAB = mirar, al soltar se cierra; un
    **toque rápido** (< 0,25 s) lo deja abierto hasta otro TAB (o Esc), y ahí
    avisa "tab to close". Snapshot nuevo: `14_map_tab.png`.

- **Sonido: más feedback + pantalla de Sonido. [IMPLEMENTADO 2026-09-26]**
  - Sonidos nuevos (sintetizados, cortos y suaves, en/cerca de la pentatónica
    de los rebotes): hover y click de UI, abrir / cerrar pantallas, agarrar
    pelota y soltarla sin tirar, enemigo muerto, oro que entra, oleada empieza /
    limpia, llegada del boss, aviso de núcleo bajo (<30%, cada 4 s), cartas
    repartidas (Choice / Shop / Pact) y elegidas, viaje a un nodo del mapa,
    item sube de nivel, y un **zumbido ambiente** muy bajo durante la pelea.
    Los que pueden spamear (hover, kill, oro, grab) tienen rate-limit en `Audio`.
  - Hover / click centralizados: `ui/UiSound` (`uisound::hover(owner, id)`
    lo llaman `Menu` y el `update` de cada pantalla; `App::handleEvent` hace el
    click si lo que está bajo el puntero es clicable). Abrir / cerrar va en
    `App::push` / `App::back` (se omite si un click acaba de sonar).
  - **Pantalla Sound** (`ui/SoundScreen`, desde "Sound" en el menú principal y
    en la pausa; M sigue muteando): Master / Music / Effects, cada uno con su
    interruptor on/off (`snd.on`, música y efectos aparte del volumen), y 14
    categorías (Ball hits, Throw, Grab, Core hit, Kill, Pickup, Combo, Gold,
    Waves, Cards, Click, Hover, Screens, Ambience) con volumen y estilo
    **Soft / Bright / Retro / Off** (Soft = el sonido de antes; Bright = voz de
    campana con parciales; Retro = onda cuadrada tipo chip). Click en un estilo
    o soltar un slider = preview. "Reset progress" no borra la mezcla.
  - Save **v14**: líneas `snd.master/music/sfx` + `snd.cat i vol estilo`; los
    saves viejos cargan con los valores por defecto. Modo foto: `19_sound`.
  - Falta: escucharlo de verdad y afinar volúmenes (hecho sin poder oír).

- **Pendiente (idea del usuario, 2026-09-24):** como las mejoras ya no llegan
  en cada oleada, cada una tiene que **sentirse mucho** al conseguirla: repasar
  items / modificadores / reliquias para que sean más fuertes y más visibles
  en juego. Se ve más adelante.

- **Suerte, rarezas más escasas, Opciones y TAB en todos lados. [IMPLEMENTADO 2026-09-26]**
  Pedido (usuario): que lo raro cueste más conseguirlo, una estadística de
  suerte, abrir las opciones en cualquier momento y TAB en cualquier momento
  de la partida.
  - **Rarezas:** pesos de tier normales 45/30/16/7/2 → **53/30/12/4/1**, de
    élite 18/32/28/15/7 → **26/35/24/11/4** (el tesoro del boss no cambia).
  - **Suerte** (`App::luck()`, puntos, `cfg::luck`): cada punto hace cada
    probabilidad x(1 + 0.08) y corre 3.5% del peso de cada tier uno arriba
    (tope 60%). Fuentes: Lucky clover **+6** (antes x1.6 y 50% de corrimiento),
    Lucky star **+2 por nivel**, Loaded Dice **+12** (antes x2). Se ve en la
    línea de oro / núcleo (mapa y tienda, con tooltip) y arriba del TAB. La
    clase Bufón debería sumar más fuentes.
  - **Opciones:** la pantalla Sound pasó a llamarse **Options** (menú y pausa)
    y suma **Fullscreen**. Se abre desde **cualquier pantalla** con la tecla
    **O** o el botón [O] OPTIONS abajo a la derecha (en pelea la pausa).
  - **TAB en toda la run:** la pelea y el mapa tienen su propio peek
    (`Screen::ownsTab`); en cualquier otra pantalla de una run viva (tienda,
    cartas, selector, pactos...) el App abre el mismo overlay encima y la
    pantalla de abajo no recibe input mientras está abierto. `TabPeek` vive
    ahora en `ui/Screen.hpp`.

- **TAB: arrastrar items entre pelotas. [IMPLEMENTADO 2026-09-27]** Pedido
  (usuario): "que puedas cambiar los items entre pelotas arrastrando con el
  TAB, así podés cambiar habilidades e items entre pelotas".
  - Con el TAB abierto (mantenido o fijado con un toque; en pelea ya está en
    pausa) se agarra un slot lleno con el mouse y se suelta en otro slot de
    cualquier pelota (o de la misma, para reordenar). **Mismo tipo de slot**:
    item↔item, tipo↔tipo, habilidad↔habilidad. Slot lleno = **intercambio**,
    vacío = se mueve. Soltar sobre el panel de una pelota (no en un slot) lo
    pone en su primer slot libre de ese tipo (el tipo, que es uno solo, se
    intercambia); si no hay lugar, vuelve a su sitio. El **nivel viaja** con el
    item.
  - Se rechaza (vuelve solo): dos copias del mismo item en una pelota,
    habilidad a un slot cerrado, Conductor / Bedrock sin su elemento (misma
    regla que `upgradeFitsBall`). Si se sacan items de Mago, las habilidades de
    los slots que se cierran quedan **dormidas** (regla de siempre).
  - Todo lo derivado sale del loadout: clases, ascendida, slots de habilidad;
    `App::moveSlot` / `App::slotMoveTarget` validan, mueven y hacen
    `syncWorldBalls()`. Una habilidad movida **reinicia su cooldown** como una
    carta nueva (`World::applySpec`), así no se pasa una cargada para
    dispararla dos veces. Las copias fantasma de la oleada no cambian.
  - Feedback: el slot de origen se apaga, los destinos válidos tienen borde
    suave (el que está bajo el puntero, más fuerte), un chip con el nombre
    sigue al puntero; sonido `grab` al agarrar, `uiClick` al soltar bien,
    `letGo` si vuelve. Sin tooltips mientras arrastrás. Hint bajo los paneles:
    "drag items between balls" ("drag to reorder slots" con 1 pelota).
  - Soltar TAB a mitad de arrastre no cierra el overlay hasta que soltás el
    mouse. Funciona en la pelea, el mapa y el TAB del App (tienda, cartas...).
    Estado en `TabPeek` (`dragBall/dragSlot/closeOnDrop`), input en
    `loadoutDragEvent` (ui/Screens.cpp). Modo foto: `08_tab_drag.png`.
  - Falta playtest (arrastre real con el mouse; no se probó en pantalla).

- **Pendiente (usuario, 2026-09-26): más música.** El usuario va a sumar
  más pistas (menú, boss, etc.); ahí se retoma el sonido (qué pista suena en
  cada pantalla, transiciones, y el resto de ajustes de la pantalla Sound).

- **Fase P — marco de clases: 8 clases, doble rol, ascendidas, habilidades y
  slot de tipo. [IMPLEMENTADO 2026-09-26]** (fase 1 de 2: el marco; la fase 2
  son cinco agentes, uno por clase nueva, que llenan sus items / mecánica /
  forma ascendida).
  - **Ocho clases** (`BallRole` en `sim/Entities.hpp` = `ItemTag` en
    `progression/UpgradeKind.hpp`, mismo orden): Striker, Guardian, Support,
    **Mage** (más slots de habilidad), **Shooter** (dispara balas), **Assassin**
    (al matar se teletransporta al enemigo más cercano), **Summoner** (invoca:
    pelotitas temporales, torretas, un dragoncito...), **Jester** ("bufón",
    juega con la suerte). Color por clase (`theme::classMage`..., `tagColor`),
    nombre + descripción + forma ascendida en `Entities.cpp`.
  - **Doble rol + ascendida.** Una pelota tiene **cada** clase de la que lleva
    2+ items (con 4 slots: 0, 1 o 2 clases). 4 items del mismo tag = **forma
    ascendida** (reemplaza "maestría"): Mega Striker, Iron Guardian, Grand
    Support, Ancient Mage, Deadeye, Shadow Assassin, Archsummoner, Grand Jester.
    Las maestrías viejas pasaron a ser las ascendidas de Striker / Guardian /
    Support. Con dos clases se aplican las dos (p. ej. Striker + Support: escala
    con la velocidad **y** marca, y pega x0.8). "Duet": toda clase que tenga
    cuenta como ascendida.
  - **API.** Loadout: `BallLoadout::hasRole(tag)`, `roles(out[2])` (orden de
    slot), `roleMask()`, `ascended()`, `tagCount()`, `leadTag()`. Sim:
    `Ball::roles` / `Ball::ascended` (máscaras `RoleMask`, `roleBit()`),
    `b.hasRole(BallRole::X)`, `b.isAscended(BallRole::X)`; `BallSpec` lleva lo
    mismo. `classHasItems(tag)`, `classUnlocked(tag, unlocks)`,
    `classUnlockNode(tag)`.
  - **Elemento = slot de tipo.** Cada pelota tiene 1 slot de tipo
    (`BallLoadout::type/typeLvl`); una carta de elemento va ahí (si es otro, lo
    **cambia**; si es el mismo, sube de nivel). Los elementos ya no ocupan slot
    de item ni cuentan para ninguna clase (antes Fire/Electric contaban como
    Striker, etc.).
  - **Habilidades** (categoría nueva ABILITY, `UpgradeKind::Ability*`, `enum
    class Ability` en el sim): activas que se disparan **solas** cuando se
    cumple el cooldown y tienen sobre qué actuar (si no, esperan cargadas). 1
    slot por pelota; `abilitySlotCount(L)` (Offers.hpp) es la **única** función
    que decide cuántos (hasta `kMaxAbilitySlots` = 3; el Mago la sube). Una
    habilidad en un slot que se cierra queda "dormida". No cuentan para la
    clase. Suben de nivel por duplicado (tope `kMaxItemLevel`, -10% cooldown
    por nivel + su propio bono) y la forja también las sube. Cinco: **Dash**
    (Uncommon: sale disparada al enemigo más cercano, x3 crucero), **Nova**
    (Rare: onda alrededor de la pelota, daño x1.2 del golpe + empuje + su
    elemento), **Split** (Rare: dos copias fantasma ~2.6 s), **Bulwark**
    (Uncommon: el núcleo empuja y aturde cuando hay enemigos cerca), **Overclock**
    (Rare: 3 s más rápida y x1.5 de daño). Tuning en `cfg::ability`, lógica en
    `sim/WorldAbilities.cpp`. Indicador: un arco finito por slot justo afuera
    del borde que se llena al recargar y destella al disparar. Las copias no
    disparan habilidades.
  - **Slots unificados:** 0..3 items, `kSlotType` (4), `kSlotAbility`+i (5..7);
    `BallLoadout::kindAt/levelAt/setSlot/levelUp/slotOf`, `slotAccepts(k, slot,
    L)`, `defaultSlot`. El selector (Equip) resalta los slots que aceptan la
    carta, dice "Swap X -> Y" al pasar por uno lleno; la forja sube items, tipo
    y habilidades; vender sigue siendo solo items.
  - **Arrancás con 1 pelota.** "Squad" (+1 pelota inicial, raíz de la web) pasa
    a ser **Calling** (raíz, 8 núcleos, 1 nivel): al empezar la run elegís la
    **clase** de tu pelota (cartas, `ClassPickScreen`, vía
    `App::advanceRunIntro`: Covenant → Calling → Quartermaster → mapa) y
    arranca con 2 items de esa clase (los de tier más bajo que tenga). Solo
    clases desbloqueadas **y con items**; si hay una sola se da directo (cartel
    "Striker ball"), si no hay ninguna no pasa nada. Los niveles viejos de
    Squad se devuelven en núcleos al cargar.
  - **Desbloqueo de clases en la web:** rama nueva **Classes** (lima) colgando
    de Mass: Support (12) → Guardian (18) → abanico de Mage / Shooter / Assassin
    / Summoner / Jester (24 cada una). Striker libre desde el principio. Los
    items de una clase bloqueada nunca salen (ofertas, tienda, recluta, starter
    kit, Legion) — se bloquean en `App::buildUpgradeCtx`. Las cinco nuevas no
    tienen items todavía: sus nodos existen pero no dan ofertas hasta la fase 2.
    La rama Pacts se corrió a la derecha y Satellite un poco a la izquierda
    para hacerle lugar.
  - **Recluta:** pelota nueva + un item de **tres clases distintas**
    desbloqueadas (antes uno de cada una de las 3).
  - **Starter kit / Quartermaster:** con pocas clases desbloqueadas su tier
    puede quedarse sin items; ahora completa con los tiers vecinos
    (`App::starterPool`) y, si no llega a 4 cartas, da el item directo en vez
    de no dar nada.
  - **TAB / selector:** cada panel muestra la pelota con las marcas de sus
    clases, "Fire  Striker / Support" (cada clase en su color, la ascendida con
    su nombre y más clara), 4 slots de item, el slot de tipo (borde del color
    del elemento) y los de habilidad (borde celeste). En TAB, **reliquias y
    pactos van en una columna a la derecha** (todas las reliquias, con
    tooltip). Marcas de clase (`render/ClassRender.cpp`): Guardian borde
    grueso, Support anillo interno, Striker punto central, Mage rombito,
    Shooter cañón hacia donde va, Assassin arco fino atrás, Summoner tres motas
    que giran, Jester dos pips; ascendida = halo exterior.
  - **Enum reordenado** (no se guarda, es seguro): nueva pelota, elementos,
    habilidades, modificadores, **items** (con una sección por clase nueva al
    final), reliquias. `UpgradeCtx::locked` pasó a `std::bitset<256>` (ya no hay
    tope de 64 picks). Panel de dev: columnas para habilidades y 3 de items,
    botón "Class pick (start)".
  - **Save v15:** 7 nodos nuevos al final (51 → 58), Squad → Calling con
    reembolso. Los saves viejos cargan (Support y Guardian quedan por comprar).
  - **Balance (sim headless, 16 corridas, filas 1-13 del acto 1 = oleadas 1,2,2,
    3,4,4,5,6,6,7,8,8,9, sin items nuevos, click = tiro rápido al más cercano):**
    1 pelota sola: sin tocarla gana 1.3 peleas, clickeando cada 2 s **6.5** (las
    4 primeras 16/16), cada 1 s 11.1. Con Calling Striker (Ricochet + Keen eye)
    7.7 / 11.6; Guardian (Rampart + Mender) 12.0 / 13 (8/16 y 16/16 limpian las
    13). Dash sola sin tocar nada 4.9. Referencia vieja, 3 pelotas lisas (Squad
    2): 8.7 / 12.8. Las primeras peleas se ganan siempre tirando, así que no se
    tocó la dificultad; el Guardian inicial es el más fuerte (Mender cura).
  - **Cómo agregar una clase (fase 2).** Cada clase toca solo SU sección de
    estos archivos (las secciones están marcadas con `==== <Clase> ====`):
    1. **Items:** el valor en su sección de `enum class UpgradeKind`
       (`progression/UpgradeKind.hpp`) y una línea `ItemDef` en su tabla de
       `progression/ClassItems.hpp` (id, título, texto, texto de nivel, tier,
       tag). Con eso ya salen en cartas / tienda / recluta / tooltips /
       SB_UPGRADES y quedan detrás de su nodo de web.
    2. **Números del item → pelota:** su `foldXxx` en `core/ClassSpec.cpp`
       (llena `m.cls.<clase>` y/o cualquier campo de `BallMods`), con tuning en
       su namespace de `core/ConfigClasses.hpp` (`cfg::mage`, ...).
    3. **Datos del sim:** su sección de `sim/Classes.hpp`: `XxxMods` (lo que
       suman sus items), `XxxState` (estado por pelota, se copia a fantasmas) y
       `XxxWorld` (estado del mundo: balas, torretas, invocaciones; se limpia en
       `startRun`; el render lo lee con `World::classWorld()`).
    4. **Comportamiento y forma ascendida:** su `ClassHooks<BallRole::X>` en
       `sim/WorldClasses.cpp` (hereda de `NoClassHooks`, pisa solo lo que
       usa): `tick`, `onHit`, `onKill`, `onWallBounce`, `onCoreBounce`,
       `damageMul`, `worldTick` (una vez por paso, siempre), `waveStart`. Es
       friend de `World`: tiene `enemies_`, `ghosts_`, `damageEnemy`, `strike`,
       `nearestEnemy`, `rng_`... La ascendida se chequea con
       `b.isAscended(BallRole::X)`. Usar `chance()` para las probabilidades
       (pasa por la suerte); el Jester suma suerte en puntos vía `App::luck()`.
    5. **Visual:** su marca y su `worldXxx` en `render/ClassRender.cpp`
       (mínimo, blanco a bajo alpha; la pelota sigue siendo el foco).
    6. **Textos:** su línea en `roleDesc` / `ascendedDesc` de
       `sim/Entities.cpp` (nombre, color y nodo de web ya están).
    7. **Mago además:** `abilitySlotCount` en `progression/Offers.hpp` (2 con
       la clase, 3 ascendido).
    El nodo de web de cada clase ya existe (`MetaClassMage`...): no hace falta
    tocar la web ni el save.
  - Falta: probarlo jugando (el sim no apunta), números de las habilidades y
    si Dash / Bulwark se sienten "autopiloto"; cómo se ve el arco de cooldown
    con 3 habilidades (Mago).
  - **Clase Bufón (Jester). [IMPLEMENTADO 2026-09-26]** "El bufón juega en
    base a probabilidades."
    - **Rol (2 items):** cada golpe tira un resultado: normal, **doble** (el
      golpe pega otra vez; si ya lo mató, el resto salta al vecino), **chispa**
      (x0.8 del golpe al enemigo más cercano) o **elemento al azar** sin dueño
      (reacciona con cualquiera, incluso con el propio elemento de la pelota).
      15% cada banda, x la suerte, tope 75% entre las tres.
    - **Grand Jester (4 items):** toda tirada del Bufón (resultado, moneda,
      wild card, jackpot) se tira **dos veces y se queda con la mejor**, y los
      dobles pasan a ser **triples**.
    - **Items** (`cfg::jester`, Lv1 → +por nivel): **Lucky charm** (Common,
      +2 suerte, +1 por nivel; suma en `App::luck()` por cada charm de cada
      pelota) · **Coin flip** (Common, cada golpe tira moneda: cara x2 (+0.25),
      cruz x0.6; la suerte favorece la cara) · **Wild card** (Uncommon, 15%
      (+5%) de que un golpe dispare un proc al azar **prestado de los items de
      cualquier pelota** en juego (Keen eye, Echo, Tesla, Bomber, Black hole, con
      sus números), +10% de potencia por nivel; sin ninguno: zap / bomba / eco)
      · **Reroll** (Uncommon, una tirada fallida del Bufón tiene 35% (+15%) de
      segundo intento) · **Chaos bounce** (Rare, al rebotar en la pared sale en
      ángulo al azar (±63° de la normal) y arma el próximo golpe x1.5 (+0.15)) ·
      **Jackpot** (Epic, 4% (+1.5%) de que una kill dé 10 (+5) de oro y una
      explosión grande x3 (+0.5) del golpe, cartel "JACKPOT" dorado).
    - **Los items del Bufón funcionan sueltos** (1 solo item en cualquier
      pelota): `ClassMods::loose` (máscara de clases cuyos hooks corren sin
      tener la clase; el Bufón se anota desde su fold). El resultado del rol sí
      pide la clase. La moneda se tira después de cada golpe y la lee
      `damageMul` en el siguiente.
    - **Visual:** los dos pips de la marca; en un doble, dos pips chicos que
      suben y se apagan sobre el enemigo (espaciados 0.12 s, máx. 8). Jackpot usa
      el cartel de burst. Nada más.
    - **Balance (sim headless, 1 pelota, filas 1-13 del acto 1, 32 corridas,
      click cada 2 s / sin tocar):** lisa 7.0 / 1.5 · Striker Calling 7.9 / 1.7
      · Bufón Calling (Lucky charm + Coin flip) 7.2 / 1.8 · rol solo 7.6 ·
      Chaos + Reroll 7.7 / 2.3 · Jackpot + Wild card 7.7 (26 de oro) · Grand
      Jester Lv1 8.5 / 2.3 · Grand Jester Lv5 (Coin, Wild, Chaos, Jackpot) 9.2
      (~400 de oro) · + Loaded Dice (suerte 18) 10.0 (~1040 de oro). Sin NaN,
      ~0.5-1 µs por paso. El daño extra rinde poco en el acto 1 (los enemigos
      caen en pocos golpes); lo que más suma es más golpes (Chaos bounce).
    - Falta: jugarlo (moneda y chaos bounce se sienten solo en pantalla), ver
      si el oro del Jackpot con Loaded Dice es demasiado, y si los pips del
      doble se leen o molestan.

- **Clase Shooter (fase 2 de la Fase P). [IMPLEMENTADO 2026-09-26]** Pedido
  (usuario): "el shooter dispara balas, y después capaz estas balas rebotan
  entre los enemigos".
  - **Rol base (2 items):** cada 0.9 s dispara una bala al enemigo más cercano
    a menos de 330 px (apunta un poco adelantado; si no hay nadie espera
    cargada; al boss también, a x0.5). Dispara **más rápido cuanto más rápido
    va** (x velocidad/crucero, entre x0.75 y x2): tirarla la convierte en
    ametralladora. Bala = x0.35 del golpe de la pelota (combo, fuego, marca de
    Support incluidos), 620 px/s, vive 0.9 s; un escudo la frena. Las balas
    viven en `ShooterWorld` (tope 90), no llaman a los on-hit de items.
  - **Items** (`cfg::shooter`, valor Lv1 + por nivel):
    - **Rapid fire** (Common): dispara x1.3 más seguido (+0.15 por nivel).
    - **Scattershot** (Common): cada ráfaga es un abanico de 2 perdigones
      (x0.75 c/u); 3 en Lv3, 4 en Lv5, +6% de daño por nivel.
    - **Rebound** (Uncommon): la bala que pega **salta a otro enemigo**
      cercano (190 px) — 1 salto +1 por nivel, conserva x0.8 (+4%/nivel) por
      salto. Lo que pidió el usuario.
    - **Tracer** (Uncommon): 40% (+15%/nivel, 100% en Lv5) de las balas
      llevan el elemento de la pelota (pueden disparar reacciones) y +5% de
      daño de bala por nivel. Sin elemento no hace nada más que eso.
    - **Drill rounds** (Rare): atraviesan 2 enemigos (+1/nivel) **y escudos**,
      x1.15 de daño (+8%/nivel).
    - **Hair trigger** (Rare): cada golpe de la pelota suelta una ráfaga de 3
      balas (+1/nivel) a los enemigos de alrededor (las que sobran salen en
      anillo), x0.8 (+5%/nivel); cooldown 0.35 s para que Satellite no la
      abuse.
    - Calling Shooter arranca con los dos Common (Rapid fire + Scattershot).
  - **Deadeye (4 items):** cada 4ª ráfaga además sale un **tiro de riel**
    que atraviesa toda la línea hasta la pared (x1.6 del golpe, con el
    elemento), y toda bala salta una vez más.
  - **Visual:** marca = cañoncito hacia donde va (ya estaba); balas = punto de
    2.6 px + estela corta, blanco (o el color del elemento si lo lleva) a
    alpha ≤0.85, muy por debajo de una pelota; el riel es una línea fina que
    se apaga en 0.25 s.
  - **Sim headless** (protocolo de la Fase P: 1 pelota, filas 1-13 del acto 1,
    16 corridas; sin tocar / click cada 2 s / cada 1 s): lisa 1.6 / 7.0 /
    10.8 · Calling Striker 1.8 / 8.4 / 11.6 · Calling Guardian 2.7 / 11.6 / 13
    · **Calling Shooter (Rapid+Scatter L1) 3.4-3.6 / 9.3-9.6 / 13** · Rapid+
    Rebound 4.6 / 10.5 / 13 · Drill+Hair 4.3 / 11.4 / 13 · Tracer+Rapid (fuego)
    5.3 / 11.1 / 13 · Scatter+Rebound L5 6.3 / 12.4 / 13 · Deadeye L1 4.9-5.3
    / 11.9-12.3 / 13 · Deadeye L5 10.6 / 13 / 13. Sin NaN; ≤3 µs por paso;
    3 Deadeye L5 a la vez tienen ~30 balas vivas (lejos del tope).
  - Falta playtest: si el Shooter inicial se siente "autopiloto" (sin tocar es
    el que más aguanta de los tres iniciales, tirando queda entre Striker y
    Guardian); el riel de Deadeye no se vio en un snapshot todavía.

  - **Clase Mago. [IMPLEMENTADO 2026-09-26]** Su gracia: **más habilidades**.
    - **Slots de habilidad** (`abilitySlotCount`): 1 cualquier pelota → **2**
      con la clase (2 items de Mago) → **3** Ancient Mage (4). Al perder la
      clase (vender / cambiar un item de Mago) las habilidades de los slots que
      se cierran **no se borran**: quedan **dormidas** (no se disparan, TAB las
      muestra apagadas con "asleep") y se despiertan solas cuando vuelve la
      clase. Una habilidad nueva va a un slot abierto; repetir una dormida la
      sube de nivel igual. (Con el pacto Duet la ascendida sí da la nova, pero
      no el 3er slot: el conteo sale del loadout.)
    - **Clase (2 items):** 2do slot + cada golpe que pega acerca **0.25 s**
      todas sus habilidades. **Ancient Mage (4):** 3er slot + **cada cast suelta
      una nova arcana** alrededor (x0.6 del golpe, radio 90, empuje, con su
      elemento; el anillo solo se ve si pegó).
    - **Items** (todos actúan vía las habilidades, así que sirven en cualquier
      pelota que las tenga; tuning en `cfg::mage`):
      - **Focus** (Common): habilidades recargan 15% más rápido (+7%/nivel).
      - **Arcane missile** (Common): cada 4.5 s (−0.4 s/nivel) **y** en cada
        cast, un misil al enemigo más cercano (x0.6 del golpe +0.12/nivel, con
        su elemento); 2 objetivos en Lv3, 3 en Lv5. Anda aunque no tenga
        habilidades. Estela fina y tenue.
      - **Attunement** (Uncommon): habilidades y misiles x1.3 de daño (+0.1/
        nivel); radios / duraciones crecen la mitad.
      - **Twincast** (Rare): 30% (+8%/nivel, pasa por la suerte) de que una
        habilidad se dispare otra vez 0.35 s después (el eco no hace eco ni
        dispara Mana spring, pero sí misil y nova).
      - **Mana spring** (Epic): cada cast recarga las **otras** habilidades un
        25% de su cooldown (+7%/nivel): con 3, se encadenan.
    - **Habilidades nuevas** (genéricas, no cuentan para ninguna clase; van en
      la sección de habilidades): **Arc** (Uncommon: un rayo salta por hasta 4
      enemigos, +1 por nivel, x0.8 del golpe, con elemento) y **Meteor** (Rare:
      cae sobre el racimo más denso de enemigos, x2.0 del golpe en radio 95,
      aturde 0.5 s, con elemento). Ahora son 7.
    - **Calling Mago:** arranca con los 2 Commons (Focus + Arcane missile) **y
      una habilidad Uncommon** (Dash / Bulwark / Arc), porque sin habilidad el
      Mago no hace nada (`App::grantStartClass`).
    - **Código:** los items pegan en el sim por `World::mageCastRate` /
      `mageOnCast` / `mageTick` (definidos en la sección Mago de
      `WorldClasses.cpp`, llamados desde `updateAbilities`), no por los hooks
      de clase (esos solo corren con la clase). Visual: rombito (ya estaba),
      al castear el rombito se agranda y se apaga; estelas de misil.
    - **Sim headless** (1 pelota, filas 1-13 del acto 1, click cada 2 s, 16-24
      corridas; ±0.4): base 6.4 · Dash sola 7.3 · Arc 7.0 · Meteor 7.7 · Nova
      6.8 · **Calling Mago** (Focus+Missile+Dash) 8.7 (Arc 8.9; ref. Striker 7.7,
      Guardian 12.0) · Mago 2 slots Dash+Nova 9.4 · Attune+Twincast Nova+Arc 8.3
      · 1 item de Mago sin clase (Missile + Dash) 8.2 · **Ancient** (F+M+Att+
      Twincast, Nova+Arc+Meteor) 12.2 (10/24 limpian todo) · Ancient con Mana
      spring 13.0 (23/24). Sin clickear: base 1.3, Calling Mago 5.4, Ancient +
      Spring 9.3. Ningún NaN, ≤1.2 µs por paso.
    - Falta: jugarlo (el Mago es bastante "autopiloto" sin clickear: si molesta,
      bajar el misil por tiempo); ver si Meteor opaca a Nova; cómo se leen 3
      arcos de cooldown + el destello del rombito con mucha acción.

  - **Clase Asesino. [IMPLEMENTADO 2026-09-26]** (fase 2; pedido: "al matar un
    enemigo se teletransporta al enemigo más cercano")
    - **Rol (2 items):** una kill encola un **blink**; en su próximo tick la
      pelota aparece junto al enemigo vivo más cercano (dentro de la arena, a
      ≤480 px) y sale apuntada a él a ≥x1.15 de su crucero. Guardas: cooldown
      0.45 s entre blinks, un blink encolado que espera más de 0.2 s se
      descarta (así nunca salta al soltarla después de agarrarla), nunca
      agarrada ni en Satellite, solo con la oleada corriendo; el punto de
      llegada se busca girando alrededor del objetivo hasta que no quede en
      una pared, el núcleo u otro enemigo (si no hay lugar, no salta).
    - **Shadow Assassin (4 items):** el blink primero **corta una cadena** de
      hasta 3 enemigos (x0.6 del golpe cada uno + su elemento) y aterriza
      junto al cuarto; cooldown a la mitad.
    - **Items** (`cfg::assassin`; todos cuelgan del blink salvo Cull):
      | Item | Tier | Efecto (Lv1) | Por nivel |
      |---|---|---|---|
      | Backstab | Common | el primer golpe tras un blink (ventana 1 s) x1.5 | +0.2x |
      | Cull | Common | un golpe que deja al enemigo bajo 14% de vida lo mata (y encadena el blink) | +3% |
      | Killing spree | Uncommon | +8% de daño por blink seguido (tope 5; 2.5 s sin blink corta) | +3% y +1 de tope |
      | Shadow trail | Uncommon | el camino del blink corta lo que cruza (45% del golpe) | +15% |
      | Smoke bomb | Rare | ráfaga al aterrizar (70 px, 60% del golpe) que deja su elemento | +15%, +8 px |
      | Phantom | Epic | cada blink deja una copia sombra (1.6 s, mismos items) que se tira al siguiente enemigo | +0.4 s |
      Como todo lo de clase, los items solo actúan si la pelota **tiene** la
      clase (2+ items del tag); Calling Asesino arranca con Backstab + Cull.
    - **Visual:** la marca de siempre (arco fino atrás); cada blink deja una
      línea blanca tenue que se apaga en 0.35 s + un aro fantasma donde
      estaba; si el camino hizo daño (Shadow trail / cadena) la línea es un
      poco más gruesa y del color de la clase. `AssassinWorld::blinks`.
    - **Sim headless** (misma tabla que arriba, 16 corridas, click cada 2 s /
      1 s): Asesino Calling (Backstab + Cull) **10.6 / 12.9** (todas: 1/16 y
      15/16) contra Striker 7.5 / 11.8 y Guardian 12.2 / 12.9; los mismos 2
      items **sin** la clase 6.9 / 10.8 (= pelota lisa), o sea el blink vale
      ~+3.7 peleas. Spree + Trail 9.2 / 12.9, Smoke + Backstab 10.4 / 13,
      Phantom + Backstab 12.8 / 13, Shadow Assassin L1 12.4 / 13, L5 13 / 13,
      3 Asesinos 12.4 / 13. Sin tocarla el blink no alcanza (1.8). Sin NaN;
      0.5-2.4 µs por paso (ruido de otros procesos).
    - Falta playtest: si el blink "roba" el tiro (la pelota tirada puede
      desaparecer al matar), si Phantom + Mitosis llena la pantalla (tope de
      fantasmas `maxGhosts`), y el Asesino en la arena ancha / boss (salta a
      enemigos, nunca al boss ni a los escudos orbitales).

  - **Clase Summoner. [IMPLEMENTADO 2026-09-26]** (fase 2; tuning en
    `cfg::summoner`, lógica en `ClassHooks<BallRole::Summoner>`).
    - **Clase (2 items):** cada 4.5 s llama una **spriteling**: pelotita
      (x0.5 de radio), lisa, de su elemento, ~3 s, lanzada al enemigo más
      cercano, pega x0.6 (es un fantasma de `ghosts_`, respeta `maxGhosts`;
      máx 2 por pelota). Además **todas sus invocaciones pegan y duran x1.3**.
    - **Archsummoner (4 items):** spritelings cada 2.5 s (máx 4) que **llevan
      los items de la pelota**, invocaciones x1.6, y cada golpe de invocación
      deja el **efecto completo** del elemento (quema / veneno / congela), no
      solo el cebo de reacción.
    - **Items** (el daño es fracción del golpe de la pelota al invocar; los
      ítems andan en cualquier pelota, sin la clase también):
      - **Turret** (Common): un rebote en pared planta una torreta ahí que
        dispara al enemigo más cercano (330 px). 5 s +1/nivel, 1.2 tiros/s
        +0.25, x0.4 +0.1; 2 por pelota, +1 en Lv3 y Lv5; 1.2 s entre plantas.
      - **Wisps** (Common): cada enemigo que muere cerca de la pelota (sus
        kills, casi siempre) suelta un fuego fatuo que persigue y revienta con
        su elemento. x0.35 +0.1/nivel; 1 por kill, 2 en Lv3, 3 en Lv5. Las
        kills de invocaciones no sueltan wisps (sin cadena).
      - **Totem** (Uncommon): con enemigos cerca planta un tótem (hexágono +
        aro tenue) que frena 35% (+6%/nivel) lo que camina adentro (radio 95
        +10) y pulsa x0.3 por segundo. Cada 7 s (−0.6/nivel), dura 5 s
        (+0.75); máx 2 por pelota.
      - **Warden** (Uncommon): 2 espíritus giran alrededor del núcleo, pegan
        x0.5 (+0.12) y empujan hacia afuera; descansan 0.55 s tras pegar. +1
        en Lv3 y Lv5 (tope 4).
      - **Dragonling** (Rare): un dragoncito vuela al lado de la pelota y
        cada 1.7 s (−0.15/nivel) escupe un cono de su elemento (250 px,
        semiángulo 0.45 +0.05) al enemigo más cercano: x0.8 (+0.2), con el
        efecto completo del elemento.
    - Invocaciones contra el boss: x0.5 (sin i-frames). Solo las pelotas
      reales invocan (copias, gemelos y spritelings no). Todo corre en
      `worldTick` (el rebote en pared se detecta por la velocidad que se
      invierte junto a una pared). Topes de mundo: 48 tiros/wisps, 8
      torretas, 6 tótems. Todo se limpia al empezar la oleada.
    - **Visual:** chiquito, translúcido y del color del elemento (verde
      Summoner si es lisa): torreta = triangulito con cañón, tiro = punto,
      wisp = punto con brillo, tótem = hexágono + aro, warden = mota,
      dragón = puntita de flecha con alas que aletean + cono tenue al escupir.
    - **Balance** (sim headless, 1 pelota, filas 1-13 del acto 1, click al
      más cercano cada 2 s, 32 corridas; ±0.3): lisa 7.1 · Striker (Ricochet +
      Keen eye) 8.2 · Keen eye solo 6.9 · Mitosis 8.3 · Turret L1 7.8 / L5
      10.4 · Wisps L1 8.2 / L5 11.1 · Totem 7.6 · Warden 7.9 · Dragonling L1
      8.6 / L5 11.4 · solo la clase (spritelings) 7.9 · **Summoner inicial
      (Turret + Wisps) 10.3** · Totem + Warden 10.3 · Archsummoner 4×L1 12.8 ·
      4×L5 13/13. Sin tocar la pelota: lisa 1.4, Turret 3.4, Dragonling 3.8
      (L5 6.9), Summoner inicial 4.4 — las invocaciones pegan solas, es su
      gracia, pero ojo con el "autopiloto". Ningún NaN; ~1 µs por paso.
    - Falta playtest: que no se llene la pantalla con 3 Summoners, y si el
      dragón L5 queda muy autopiloto.

- **Idea del usuario (2026-09-26): más clases, doble rol, habilidades y
  elemento como slot.** **[IMPLEMENTADO 2026-09-27: marco en la Fase P y las
  cinco clases nuevas en sus subsecciones (Mago, Shooter, Asesino, Summoner,
  Bufón). Falta playtest de todo.]** Nota: los items de Shooter y Asesino solo
  hacen algo con la clase activa (2+ items); los de Mago, Summoner y Bufón
  andan sueltos en cualquier pelota.
  - **Clases nuevas** además de Striker / Support / Guardian: **Mago**,
    **Shooter**, **Asesino**, **Summoner**, **Bufón**, cada una con una mecánica
    propia. Se desbloquean en la web (al principio solo hay Striker, después
    Support, después Guardian, después las nuevas).
    - Mago: su gracia es llevar **más habilidades**. Slots de habilidad según
      su nivel: 1 (cualquier pelota) → 2 (mago, 2 items) → 3 (4 items de mago).
    - Asesino: al matar un enemigo se **teletransporta** al enemigo más cercano.
    - Bufón: juega con **probabilidades**.
    - Shooter: **dispara balas**; más adelante las balas pueden rebotar entre
      enemigos.
    - Summoner: **invoca cosas**. Sus items invocan: pelotitas que duran un
      tiempo, torretas, un dragoncito, cosas varias.
  - **Doble rol:** 2 items de una clase + 2 de otra = la pelota tiene **los dos
    roles** (ej. Striker + Bufón). **4 items de la misma clase** = versión
    mejorada con más cosas de su rol: **Mega Striker**, **Ancient Mage**,
    **Shadow Assassin**, etc. (reemplaza a la "maestría" actual).
  - **Arrancás con 1 sola pelota** (3 es demasiado fuerte al principio). Un nodo
    de la web te deja **elegir la clase** de esa pelota inicial.
  - **Habilidades:** activas que se disparan solas con el tiempo (cooldown), en
    **1 slot de habilidad** por pelota (el Mago tiene más), intercambiables y
    visibles en el TAB. **No cuentan para la clase** (no suman puntos de
    Mago / Asesino / etc.).
  - **Elemento como slot:** fuego / hielo / eléctrico / etc. deja de ocupar un
    slot de item y pasa a un **slot de tipo** propio, intercambiable como las
    habilidades.
  - **Pelota final:** 4 slots de items + 1 de habilidad + 1 de tipo. Los items
    pasivos globales (reliquias) aparecen a un costado en el TAB.

- **Fase Q — la clase es la identidad, el elemento es secundario. [IMPLEMENTADO 2026-09-27]**
  Pedido (usuario): "que resalte más si una pelota consigue una clase, y que
  los colores de los elementos sean menos vistosos y tomen un rol más
  secundario, entonces priorizás más conseguir una clase que un elemento".
  - **Cuerpo de la pelota = su clase.** `b.color` sale de la clase principal
    (la del primer item, `BallSpec/Ball::primary`, `Ball::leadRole()`) vía
    `World::ballTint` → `theme::hueSpeedColor` (grisácea lenta, más saturada
    rápida, como antes hacía el elemento). Sin clase = gris neutro
    (`theme::speedColor`). **Ascendida** = más saturada y clara, glow más
    grande, halo del color de la clase con 4 marcas que giran lento y su
    marca a pleno. **Doble rol** = arco del color de la segunda clase por
    dentro de la mitad de abajo del borde. Color por clase: `roleColor(BallRole)`
    (sim/Entities; `tagColor` lo usa).
  - **El elemento pasa a detalle:** un aro finito apagado justo afuera del
    borde, tiñe la estela (mezcla 55%) y sigue en estados de enemigo,
    reacciones y explosiones. Todo el dibujo de "quién es" está en
    `drawBallIdentity` (render/ClassRender), igual en la arena y en TAB.
    Puntitos del HUD y mira de Hunters usan `ballHue` (la clase).
  - **Paleta de elementos (antes → ahora):** Fire 255,148,66 → 206,136,92 ·
    Poison 150,214,96 → 142,180,108 · Water 92,152,255 → 104,138,204 · Ice
    170,224,240 → 158,192,204 · Stone 176,156,132 → 154,142,126 · Electric
    176,116,246 → 152,122,204. Los colores de clase quedan igual (los vivos).
    Lo que no era elemento y usaba esos colores pasa a `theme::ember`
    (255,148,66: pacto Thrower, rama Special de la web, borde de las monedas)
    y `theme::venom` (150,214,96: pacto Alchemist).
  - **Momento de clase nueva:** `App::syncWorldBalls` (por donde pasa todo
    cambio de loadout: elegir/equipar, tienda, forja, vender, recluta, pactos,
    Calling, starter, dev, y el drag de TAB si llama a sync) compara las
    clases de cada pelota con las de la última sync (`knownClasses_`,
    `announceClassGains`; la base se toma en `newRun`). Clase nueva o segunda
    clase → franja "BALL 2 - NEW CLASS / FIRE BALL → STRIKER" (o "STRIKER →
    STRIKER + SUPPORT") en el color de la clase, sobre cualquier pantalla
    (`Effects::classBanner`, en cola si hay varias, máx. 4); ascender →
    "BALL 1 - ASCENDED / MEGA STRIKER" con corchetes. Más un flash suave,
    un acorde que sube (`Audio::classGain`, categoría Cards; ascendida más
    largo con campana arriba) y un destello de anillos del color de la clase
    en la pelota cuando corre la pelea (`Ball::classPulse`). Perder una clase
    no dice nada. El cartel viejo "Striker ball" del Calling quedó reemplazado.
  - **Cartas:** los items con clase llevan lomo del color de la clase a la
    izquierda y un lavado tenue arriba; si tomarlo le da la clase a alguna
    pelota (2º item del tag, con slot libre) o la asciende (4º), un chip abajo
    dice "MAKES A STRIKER" / "ASCENDS: MEGA STRIKER" (`drawClassCardMark`,
    elección y tienda). Las de elemento quedan calmas (título y cabecera en el
    color apagado). No se tocó la frecuencia de ofertas de elementos.
  - Modo foto suma `05b_class_gain` y `05c_ascend`.
  - Falta: escuchar el acorde en los 3 estilos, ver en juego si el aro del
    elemento se lee sobre Guardian (borde grueso) y si el destello molesta
    cuando se ganan varias clases juntas (Legion).

- **Web por rutas de clase + primera habilidad + Magic missile. [IMPLEMENTADO 2026-09-27]**
  Pedido (usuario): "hacé que la web sea más interesante, siento que las clases
  están todas muy pegadas. Si elegís mejorar daño vas por la ruta del Striker,
  si mejorás el dinero vas por el Bufón, si desbloqueás nuevas habilidades vas
  por el Mago, y así." Y después: "cuando empieces solo elegís la primera
  habilidad de la pelota, no elegís la clase, y el mago tiene como habilidad
  principal Magic Missile, que es un proyectil pero sigue a enemigos".
  - **Rutas.** La web ya no tiene ramas Base / Combat / Eco / Special /
    Pickups / Arsenal / Classes: cada clase tiene **su ruta**, que sale del
    centro en su propia cuña y la clase queda **cerca de la punta**; pasada la
    clase hay dos nodos: **"<Clase> lore"** (los items de esa clase salen +50%
    más seguido por nivel, 2 niveles, 10 núcleos; pesa dentro del tier en
    `App::rollPick`) y un **perk propio** de la clase. Pacts sigue siendo su
    rama. Cada ruta se tiñe con el color de su clase (`tagColor`; Pacts en un
    hueso pálido), la leyenda lista las rutas en el orden del reloj con
    comprados/total y al pasar el cursor resalta la ruta. El nodo de clase se
    dibuja más grande, con aro y su nombre siempre visible. La web es una
    elipse (`kStretchX` 1.4, anillo r a (r + 0.6) separaciones) para usar el
    ancho; el zoom inicial encaja toda la web; los nombres de la frontera se
    corren hacia afuera del centro para no pisarse. Posición = ángulo + anillo
    (`MetaUnlockDef::ang/ring`).
  - **Mapa de rutas** (sentido horario desde arriba; `nuevo` = índice 58+,
    costo en núcleos salvo "pr" = prismas; → = padre):
    | Ruta | Camino principal | Ramas | Pasada la clase |
    |---|---|---|---|
    | Striker (arriba, daño / tiro) | Heft → **Sling** (nuevo, 8, 3 niv: tiro +8%) → **Momentum** (nuevo, 12, 2 niv: pelotas Striker +10% daño) → **Striker lore** | — | (Striker siempre abierto: la ruta termina en su lore) |
    | Shooter (arriba-der., velocidad / proyectiles) | **Velocity** (nuevo, 8, 2 niv: crucero +5%) → Kinetics → **Shooter** (18) | Overload (pr) | Shooter lore · **Caliber** (nuevo, 12, 2 niv: balas +15%) |
    | Jester (derecha, oro / economía / suerte) | Fortune → Foresight → Lucky star → **Jester** (18) | Salvage → Windfall → Interest → Prospector; Treasury → Haggler → Merchant → Armory (pr) | Jester lore · **Fool's luck** (nuevo, 12, 2 niv: +2 suerte por nivel y +2 más por cada pelota Bufón) |
    | Assassin (abajo-der., críticos / kills) | **Keen instinct** (nuevo, 8, 3 niv: +4% crítico x2 a toda pelota) → Elite spoils → **Assassin** (18) | — | Assassin lore · **Deathmark** (nuevo, 12, 2 niv: Asesinos rematan bajo +5% de vida) |
    | Pacts (abajo) | Oath → Covenant; Hunters → Loaded dice; Legion → Alchemy | | |
    | Summoner (abajo, copias / pelotas extra) | **Brood** (nuevo, 8, 2 niv: copias fantasma +25% de vida) → **Muster** (nuevo, 12: una pelota nueva llega con un item Common) → **Summoner** (18) | Twins (Gemini, pr); Starter kit → Quartermaster | Summoner lore · **Bond** (nuevo, 12, 2 niv: invocaciones +15% daño y duración) |
    | Support (abajo-izq., power-ups / marcas) | Uplink → Capacitor → Charged → **Support** (12) | Ledger → Stockpile → Magnet → Afterglow; Damper (pr) → Facet (pr) → Singularity (pr) | Support lore · **Rally** (nuevo, 12, 2 niv: marcados +10% de daño recibido; `WorldParams::markMul`) |
    | Mage (izquierda, habilidades + elementos) | **Magic missile** (nuevo, 6) → **Channel** (nuevo, 8, 3 niv: toda habilidad recarga +6%) → **Mage** (18) | habilidades: **Arc** (8) → **Bulwark pulse** (8) → **Split** (12) → **Overclock** (12) → **Meteor** (16); elementos (pr): Ignition → Venom → Tide → Frost → Quarry → Static (el nodo eléctrico, antes "Arc"), Ember, Prism | Mage lore · **Archive** (nuevo, 12, 2 niv: pelotas Mago recargan +15%) |
    | Guardian (arriba-izq., núcleo / defensa) | Bulwark → Mass → **Guardian** (12) | Mend → Regen → Bastion → Last stand; Aegis → Satellite (pr) | Guardian lore · **Stonewall** (nuevo, 12, 2 niv: Guardianes curan +0.5 por rebote en el núcleo) |
    Costo de una clase para un jugador nuevo (sin contar Calling, 8): Guardian 34, Support 38,
    Shooter 32, Assassin 38, Summoner 38, Mage 32, Jester 46 núcleos (antes:
    34 / 52 / 76). Números en `cfg::meta` (`lorePerLevel`, `slingPerLevel`...).
  - **Habilidades en la web.** Dash y Nova están abiertas desde el principio;
    Magic missile, Arc, Bulwark, Split, Overclock y Meteor se desbloquean en la
    ruta del Mago (`abilityUnlockNode`, se bloquean en `buildUpgradeCtx`): una
    habilidad bloqueada nunca sale (cartas, tienda, selección inicial).
  - **La run arranca eligiendo la primera habilidad, no la clase.** La pelota
    inicial arranca sin clase y sin items; en la intro (`advanceRunIntro`:
    Covenant → **primera habilidad** → Quartermaster → mapa) elegís su primera
    habilidad entre 3 cartas de las desbloqueadas (`AbilityPickScreen`,
    `ui/AbilityScreen.cpp`; con 1 sola se da directo). **Calling** (raíz, mismo
    índice) pasó a ser "abre la web + una 4ª carta". Se fueron el selector de
    clase (`ClassPickScreen`), `grantStartClass` y `cfg::classes::startItems`.
    Panel de dev: "Ability pick (start)".
  - **Magic missile** (habilidad nueva, Uncommon, `Ability::MagicMissile`,
    `cfg::ability::missile*`): cada 3.2 s suelta un misil que sale abierto
    hacia un costado, **curva y persigue** al enemigo más cercano (gira 7 rad/s,
    acelera de 260 a 560 px/s, vive 2.6 s; si su blanco muere busca otro a
    ≤420 px; al boss le pega x0.5). x0.9 del golpe (+0.15/nivel), con el
    elemento de la pelota; 2 misiles en Lv3, 3 en Lv5; -10% de cooldown por
    nivel como todas. Vuelan en `MageWorld::missiles` (el `worldTick` del
    Mago). Visual: punto chico tenue + estela curva corta, del color del
    elemento (azul Mago si es lisa). **Es la firma del Mago:** una pelota que
    llega a la clase Mago recibe Magic missile sola en un slot de habilidad
    libre si no la tiene (`App::grantMageMissiles`, en cada `syncWorldBalls`;
    cartel "+ Magic missile").
  - **Arcane missile → Barrage** (mismo `UpgradeKind::ArcaneMissile`, id
    `Barrage` para SB_UPGRADES): para no tener dos misiles casi iguales, ahora
    potencia los misiles: Magic missile suelta +1 misil (+1 más en Lv3 y Lv5),
    todos los misiles pegan +10%/nivel, y cada **otra** habilidad que castea
    suelta un misil (x0.6 del golpe, +0.12/nivel). Ya no dispara por tiempo.
  - **Save v16:** solo se agregan nodos al final (58 → 86); ningún índice
    cambió, los nodos viejos solo cambiaron de ruta / padre / lugar (puede
    quedar un nodo comprado con su padre nuevo sin comprar: se ve y anda
    igual). No se sacó nada, así que no hay reembolso. Ojo: en un save viejo
    Split / Bulwark / Overclock / Arc / Meteor quedan bloqueadas hasta comprar
    su nodo.
  - **Sim headless** (1 pelota, oleadas 1-6, sin tocar, 3 semillas; kills):
    sin habilidad 0-2 · Dash 14-17 · Magic missile L1 6-17 · L1 + Barrage
    19-25 · L5 46-48. Sin NaN, ≤5 misiles vivos.
  - Modo foto: `02_web` (web temprana: Guardian, Magic missile, Sling...),
    `15_web_grown` (varias rutas), `03_ability_pick` y `17_start_ability`
    (reemplazan a los de clase); la pelota 3 del `04_play` lleva Magic missile.
  - Falta playtest: si Magic missile L1 queda corta al lado de Dash, si 6
    núcleos por la primera habilidad extra es poco, legibilidad de la web con
    todo comprado y con zoom mínimo.

- **UI limpia + mapa con caminos (2026-09-27, rama `ui-polish`).** Regla de
  la usuaria: pocas cosas en pantalla pero importantes, espaciado, tamaños que
  se adaptan al contenido, texto explicativo solo al pasar el cursor.
  - **Base:** etiquetas chicas suben dos pasos (mínimo 12 px, en
    `makeLabel`), `theme::margin` 30, `fsSmall` 14. Atajos = tecla sola
    (`drawKeyCap`: [TAB], [O]); lo que hacen aparece al hover.
  - **HUD de combate:** arriba etapa + barra de **progreso de la etapa**
    (enemigos limpiados; antes la barra era la vida del core y confundía — la
    vida ya está en el anillo del core). Sin score (queda en Stats). Oro
    arriba a la derecha. Abajo a la izquierda: [TAB] y una ficha por pelota
    en su color de clase.
  - **TAB y equipar** se agrandan según cuántas pelotas hay
    (`panelRowZoom`, `Window::useUiZoom`: dibujan en un lienzo UI/zoom);
    columna de reliquias/pactos solo si hay alguno.
  - **Cartas (`drawPickCard`)**: tipo arriba a la izquierda, **rareza =
    puntos 1–5 + borde/banda** arriba a la derecha, **clase = insignia sólida**
    del color de la clase + lomo y tinte. El fondo ya no se tiñe por rareza.
  - **Tienda:** agrupada por tipo (items · habilidades · elementos · reliquias
    · modificadores · pelota · caja misteriosa) con encabezado de color, lo
    más raro primero; 4 ofertas (antes 5).
  - **Paleta** un punto más apagada: `theme::soften` sobre los colores vivos.
  - **Mapa:** caminos que no se cruzan (`generateMap`: 2–4 caminos salen del
    tronco, cada fila un camino se desvía un carril con `driftPct`, a veces se
    bifurca con `splitPct`, se juntan al pisarse); filas de 2–4 nodos; la fila
    previa al jefe solo conecta con carriles vecinos. Pantalla con **scroll**
    (rueda, flechas/W S, Espacio vuelve a tu fila), nodos más grandes y
    quietos, sin leyenda (tooltip al hover), cabecera fija con acto y estado.
    Botón [O] de la esquina eliminado (la tecla O abre opciones).
  - **Economía:** la **primera habilidad llega al terminar el primer combate**
    (`App::postFight`), ya no al empezar. Se probó un pick tras cada combate y
    **se descartó** (no convenció): los combates normales dan oro
    (`combatBase` 7, `perRow` 2), el élite su pick. El build crece en las
    paradas del mapa: menos tiendas (`wShop` 6), más Upgrade (`wUpgrade` 10,
    `wCombat` 46).
  - Mapa: además de rueda/flechas, se **arrastra** con el mouse (clic fuera de
    un nodo).
  - Falta playtest: si 4 ofertas dejan la tienda demasiado floja.

- **Items escasos, pelotas escasas, rutas con carácter (2026-09-27, `ui-polish`).**
  - **Items:** nivel máx. **3** (`kMaxGearLevel`), pero cada nivel vale doble:
    un item Lv L rinde como el viejo nivel `gearPower(L)` = 1 / 3 / 5 (números
    por nivel e `itemLevelDamage`). Elementos y habilidades siguen hasta Lv5.
  - **De dónde sale cada cosa** (`App::RollSource`): tras un combate normal,
    **solo modificadores** (`PostFight`); élite = **solo items**
    ("Elite spoils"); tesoro del jefe = todo; nodos Upgrade / Recruit = todo
    menos items; tienda = todo menos pelota (máx. 1 item). Los items solo vienen de élites,
    tiendas (y el jefe).
  - **Tienda:** solo se compra lo que aparece; sin reparar, sin forja, sin
    caja misteriosa. Rolear solo con **Merchant** (1 por nivel y visita);
    vender **1 item por visita** (+1 por nivel de **Haggler**). Los botones
    que no aplican no se muestran.
  - **Pelotas (máx. 5):** carta "Extra ball" con peso x0.3 dentro de su tier
    (`newBallCardWeight`), nunca en la tienda; el nodo Recruit la ofrece
    siempre (+3 picks sin items). La fila previa al jefe ya no tiene Recruit
    (Shop / Rest / Upgrade / Forge).
  - **Acto 1 más corto:** 10 filas (`mapRows`, `rowsAct1`); acto 2 sigue 14.
  - **Rutas con carácter (sin decirlo):** cada camino se inclina a *Recruit*
    (Recruit, pocos élites, sin tienda) o a *items* (muchos élites y tiendas,
    sin Recruit); se alternan, una bifurcación toma la inclinación contraria,
    nodos compartidos son neutros (`PathLean`, `cfg::map::w*Path`). El
    Recruit **no se sortea**: `generateMap` pone exactamente 1 por ruta de
    reclutar en el acto 1 (2 en el acto 2; `recruitsPerPathAct*`), repartidos
    a lo largo de la ruta. Al azar salía en ~39% de mapas del acto 1 sin
    ningún Recruit. Sim 3000 mapas acto 1: 0% sin Recruit, 1–2 por mapa; la
    ruta con más trae ~1.1, la con menos ~0.
  - **Items difíciles de conseguir (ajuste final):** el élite reparte **3
    cartas** (`eliteCards`); rarezas bajas para que un raro sea un evento:
    élite `{30,36,22,9,3}` (~3% Legendary por carta), tienda `{40,34,17,7,2}`.
    Una carta Epic / Legendary en la mesa hace un flash de su color.
  - **Tienda:** 3 ofertas, **máx. 1 item** (`shopMaxItems`); Merchant ya no
    suma oferta, solo rerolls (máx. 2 = su nivel máx.) y descuento.
  - **Élites colocados, no sorteados:** 1–2 por ruta de items en el acto 1, 2
    en el acto 2 (`elitesPerPath*`); fuera de eso un élite suelto es raro
    (`wEliteNeutral` 4, `wEliteRecruitPath` 3). Sim acto 1: la ruta con más
    élites ~1.8, la de reclutar ~0.2; todo mapa tiene ≥1 élite y ≥1 Recruit.
    Idea: pelotas e items son lo más importante de una run larga — cada
    Recruit y cada item cuestan una decisión de ruta.
  - TAB / equipar: zoom máx. 1.2 / 1.15 (con pocas pelotas quedaba enorme).
  - **Élite se distingue:** en el mapa naranja propio (`ember`), más grande
    y con doble marco; en la pelea el cartel dice "Elite - item spoils" y el
    HUD "ELITE" con barra naranja.

- **Lenguaje visual: clase / rareza / tipo / habilidad (2026-09-27).** Sin
  texto nuevo, solo color, forma y lugar:
  - **Rareza = brillo, arriba.** `tierColor` es una rampa gris → blanco y solo
    Legendary es dorado pálido (nunca choca con un color de clase). En la
    carta: banda superior que engrosa con el tier + puntos; en el casillero:
    puntos a la derecha.
  - **Clase = color sólido, a la izquierda.** Lomo grueso + tinte + insignia
    en la carta; en el panel, el item es un chip teñido de su clase con lomo.
    La cabecera del panel muestra solo las clases (ya no el elemento).
  - **Tipo (elemento) = píldora** de su color alrededor del nombre en la
    carta; **habilidad = corchetes cian**.
  - **Panel de la pelota (TAB / equipar), elegido por la usuaria:** la
    identidad arriba, el equipo abajo. Cabecera: **tipo = hexágono** de su
    color a la izquierda (con su nombre), la pelota al medio, **habilidades =
    rombos cian** a la derecha (1 grande o 2–3 chicos; nivel adentro, nombre
    debajo si hay una sola y entra), clases debajo. Abajo solo los 4 items
    como chips. Al equipar, los destinos válidos tienen un halo.
    (`slotRect`, `kPanelHeadY`, `kPanelItemsTop`, `kPanelH` 272.)
  - **Elemento en la pelota:** borde más grueso con halo; al ganar uno, flash
    de su color y dos anillos que se expanden (`Ball::elemPulse`,
    `drawElementPulse`, `KnownClasses::element`).

- **3 cartas, formas por tipo, dev solo F1 (2026-09-27).**
  - Toda elección reparte **3 cartas** (`cfg::run::choiceCards`, Recruit =
    pelota + 2); las mejoras que ya daban más (Quartermaster: kit de 4;
    Calling: 4ª carta en la primera habilidad) siguen igual.
  - La tienda solo trae item en el **40% de las visitas** (`shopItemChance`).
    Sonda de reparto (4000 tiradas por origen, SB_DEV=1): post-combate 100%
    modificadores; Upgrade/Recruit 0 items; élite 100% items; tesoro 80% items.
  - **Marca de tipo** discreta junto al nombre del tipo (carta, encabezado de
    la tienda, reliquias del TAB): item = cuadrado, habilidad = rombo,
    elemento = hexágono, reliquia = círculo, modificador = triángulo, pelota =
    anillo (`drawKindMark`). Los textos de las cartas se mantienen.
  - **Modo dev:** solo F1 (panel). Se sacaron la chuleta de teclas y los
    atajos N/H/G/B/U/C; el panel suma "Cores & prisms", "Pick: after fight",
    "Pick: elite (items)", "First ability pick", y los pactos en 2 columnas.

- **Pelota de dos clases (2026-09-27).** Se probó mezclar los colores y se
  descartó (colores barrosos, rompe "color = clase"). Ahora, de adentro a
  afuera: **cuerpo** = color de la clase principal; **segunda clase** = banda
  **sólida** de su color justo dentro del borde (~20% del radio); **elemento**
  = anillo **punteado** fino afuera, tras un hilo oscuro, con halo tenue
  (`drawBallIdentity`). Así Shooter (naranja) + fuego (naranja) se leen como
  dos cosas por forma y lugar.
  - **Clase principal (cuerpo):** la clase con más **niveles de item**
    sumados; empate → la del item más arriba en los casilleros (arrastrar en
    TAB elige el color). Una ascendida (4 items) siempre es la principal.
    (`BallLoadout::roles` / `tagLevels`.)

- **Dificultad: 5 actos, jefes duros, minijefes, modo difícil (2026-09-27, `ui-polish`).**
  Pedido: "muy fácil, niveles cortos, jefes extremadamente fáciles". Todo en
  `core/Config.hpp`.
  - **5 actos** (`cfg::run::acts`, `finalWave` = 50, `actOfWave` /
    `isBossWave`): la oleada 10 de cada acto es su jefe. Mapas: acto 1 = 11
    filas, actos 2-5 = 13. "Continue" en el cartel del jefe lleva al acto
    siguiente (pacto solo tras el jefe del acto 1; tesoro tras cada jefe). Oro
    de pelea + `perAct` por acto.
  - **Un jefe por acto** (`BossKind`, dibujo por forma): Charger (1, octógono,
    núcleo a la izquierda), **Hive** (2, hexágono: se mece hacia el núcleo y
    cada 4.2 s suelta un abanico de 4 runners), **Warden** (3, cuadrado: un
    escudo que gira bloquea las pelotas de ese lado; camina / se planta),
    **Dasher** (4, triángulo: se acerca, apunta con una línea punteada y
    embiste; cada golpe limpio lo empuja atrás), Orbital (5, final).
  - **Jefes mucho más duros:** vida medida en grunts de su oleada (12 / 10 /
    12 / 12 / 14, +15% por pelota extra), i-frames 0.1 → 0.2 s. **Dos fases:**
    bajo 50% se **enfurece** (todo x1.5 más rápido, adds más seguidos, cartel
    ENRAGED) y al 66% y 33% **llama a un Brute**. Sim: el Charger pasó de
    ~11 s a ~40 s con el mismo bot.
  - **Minijefe Brute** (pentágono con núcleo naranja): x10 vida, lento, casi
    no se empuja, x4 daño al núcleo. Todo élite trae uno (dos desde el acto 3);
    desde la oleada 12 una pelea normal termina con uno (35%).
  - **Enemigos nuevos:** **Blinker** (oleada 12+, triángulo: salta hacia el
    núcleo cada 2.6 s, parpadea antes) y **Mender** (22+, cruz verde: cura a
    los de alrededor). Desde la 12, **manadas** de 4 runners juntos.
  - **Curva:** cantidad = 8 + 2.1·x + 0.07·x² (tope 120 hacia la 29); vida
    x1.18 por oleada hasta la 20 y x1.04 después; cadencia al mínimo en la 30.
    Sim (bot que clickea): acto 1 peleas ~50% más largas y 2-3x el daño al
    núcleo; acto 2 más duro que el acto 2 viejo.
  - **Modo difícil** (botón "Mode: Normal / Hard" en la pantalla de inicio de
    run, se guarda como `hard` en el save, tooltip al hover; HUD "act N hard"):
    enemigos x1.6 vida, x1.12 velocidad, x1.3 cantidad, x1.5 daño al núcleo,
    jefes x1.8, Brutes desde la oleada 5 (50%) y dos por élite, **sin
    reparación gratis** entre peleas. Paga x1.75 núcleos. `cfg::hard`.
  - Panel F1: items agrupados por **clase** (encabezado y color de la clase;
    la rareza en el tooltip); spawns de Blinker / Mender / Brute.
  - **Onda de choque del Charger:** cada 5 s (más seguido enfurecido) avisa
    0.8 s con un anillo que se cierra y pulsa, y despide lejos a toda pelota
    dentro de su alcance (300 px del arena, más fuerte cuanto más cerca).
    `cfg::boss::shock*`, `World::chargerShock`.
  - **Arranque de cada pelea:** las pelotas se acomodan en un anillo
    alrededor del núcleo, giran acelerando 1.9 s y salen disparadas todas
    juntas (hacia afuera, inclinadas en el sentido del giro, x2.2 del
    crucero). No se pueden agarrar mientras giran. Un solo anillo para todas,
    medido con la pelota más grande (y con su tamaño real en la arena del
    jefe): despeja el núcleo y entran todas lado a lado. Si no entra junto a
    una pared, el centro del anillo se corre hacia adentro lo justo. En la
    pelea del Charger el núcleo pasó a 250 px de la pared (`coreMarginX`, antes
    110) para que el anillo quede centrado; medido con 1-5 pelotas de tamaño
    x1-x3 en las oleadas 1/10/15/20/50 (con la cámara del App): nada sale de la
    arena ni del cuadro. Modo foto `12a_boss_launch`. `cfg::ball::launch*`,
    `World::updateLaunch`; modo foto `04a_launch`.
  - **Esc siempre abre la pausa** en una run (cartas, mapa, tienda, pacto,
    selector, cartel del jefe...); "Resume" vuelve a donde estabas. Solo la
    pausa, lo que abre (stats / cómo jugar / opciones) y el panel de dev usan
    Esc como "volver"; con TAB abierto, Esc lo cierra. Salir de la tienda /
    cancelar el selector: su botón o clic derecho. (`App::onPauseMenus`.)
  - Cerrar la pausa (o volver del selector) no repite la animación de la
    pantalla de abajo (las cartas no se reparten de nuevo).
  - **Selector de pelota / slot:** botón **"Back to the cards"** abajo (o clic
    derecho): tomaste una carta pero todavía no la pusiste → volvés a las
    cartas sin gastar nada. Una vez puesta, la elección terminó. En la tienda
    dice "Back to the shop", en la forja "Leave the forge".
  - Falta playtest: todo lo anterior es calibración relativa con un bot
    mucho peor que un jugador; tocar `cfg::wave`, `cfg::boss`, `cfg::hard`.

- **Fase 2 — Jefe tras la oleada 10.** Da upgrades de pelota (viento/agua/
  piedra). Extiende la run mas alla de 10 en "modo infinito" opcional.
- **Fase 3 — Variedad.** Repulsor, bumper, rampa. Corredor, tanque, escindido.
  Afijos de élite.
- **Fase 4 — Clases y modificadores profundos.** Árbol de clases. Power-ups
  re-tematizados. Más modificadores (cadena, esquirla, órbita).
- **Fase 5 — Equipo y meta grande.** Roster de varias pelotas. Hub ampliado,
  hitos, ramas de meta-progresión, tutorial guiado por arenas.

---

## 9. Preguntas abiertas

- ¿La run es **infinita con escalado** o tiene **actos** con jefe final?
  (De momento: infinita, "hasta dónde llegas".)
- ¿Las estructuras de campo colocadas **persisten dentro de la run** al cambiar
  de arena, o se recolocan cada arena con el presupuesto?
- ¿La pelota **atraviesa** enemigos débiles y **rebota** en los duros, o rebota
  siempre? (MVP: rebota siempre.)
- ¿Las estructuras también curvan a los enemigos?
- ¿La meta-moneda se gana solo al morir, o también por hitos a mitad de run?
- ¿Cuántas ofertas por elección (3) y hay *reroll*? ¿Se puede saltar y coger
  chatarra?

---

## 10. Atrapar, Creeds, Pacts e items de estilo (decidido 2026-09-28)

Idea de fondo: agarrar la pelota es una mecánica. Una pelota rápida es difícil
de atrapar, así que una lenta que pega fuerte es un estilo válido. Tirar
(click o gomera) y dejarlas estar son los dos estilos; ninguno es obligatorio.

### 10.1 Atrapar y tirar
- Radio de agarre: se queda en 130, **escalado con `arenaScale()`** (hecho:
  después del acto 1 la cámara se aleja y antes el radio quedaba a la mitad).
- Tiro rápido y cámara lenta al apuntar: se quedan como están.
- **Premio por atrapar:** cuanto más rápido venía la pelota al agarrarla, más
  pega el primer golpe del tiro que sigue (hasta +50%). Destello al atraparla.

### 10.2 Creeds (los pactos de antes, renombrados)
- Los 12 pactos actuales pasan a llamarse **Creed**. Se quedan todos,
  incluidos Hunters y Clockwork: son para quien no quiere agarrar mucho.

### 10.3 Pacts (carta nueva: te da y te saca)
Solo gameplay, nada de plata. Lista aprobada:
- Lead: +60% daño / la pelota va 40% más lenta.
- Quick hands: premio por atrapar x2 / una pelota que nadie tira pierde 20% de daño.
- Heavy arm: tiros x1,5 / sin cámara lenta al apuntar.
- Glass edge: +30% chance de crítico / los golpes que no son crítico pegan 20%
  menos (antes era "-1 slot de item"; se cambió porque sacar un slot pide elegir
  pelota y romper equipamiento).
- Stillness: cuanto más lenta, más daño carga / el tiro rápido pierde potencia.
- Overflow: +1 pelota / todas -15% daño.
- Tiny: +80% daño / pelotas a la mitad de tamaño.
- Colossus: la mejor pelota x2 tamaño y daño / las demás -30% daño.
- Hot potato: la agarrada gana daño por segundo / pasados 3 s se cae sin fuerza.
- Juggler: atrapadas seguidas sin tocar el núcleo suman daño / tocar el núcleo
  corta la racha y le saca vida.
- Void walls: los bordes te pasan al otro lado / se apagan los items de rebote en pared.
- Anchor walls: tiros x2 / la pared frena la pelota en seco.
- Last breath: núcleo bajo 30% = todo x2 / -20% vida máxima del núcleo.
- Mirror: cada tiro larga una copia fantasma al revés / enemigos +20% vida.
- Elemental: elementos x2 / contacto -30%.
- Frenzy: el combo sube el doble / agarrar corta el combo.
- Blind: tiros +40% / sin guía de puntería.
- Horde (antes "Swarm", chocaba con el arquetipo Swarm de Creed): +1 pelota
  por jefe / +30% enemigos por oleada.

**Dónde:** nodo **Altar** (1 de 3, o ninguno). Es raro en el mapa. Camino
secreto: en cada acto, 3 combates seguidos (élite cuenta) sin daño al núcleo
abren un camino al Altar antes del jefe. Tiendas y descansos no cortan la
racha; solo la corta recibir daño. La animación del camino que se abre se
muestra recién cuando estás a 1 nodo del jefe. (Hecho: el Altar oculto
aparece arriba de donde estás, en la fila del jefe, y lleva al jefe. Los
Pacts que piden agarrar no se ofrecen con Hunters / Clockwork.)

### 10.4 Mapa
- Nunca dos peleas normales seguidas: un combate que lleva a otro combate
  convierte el segundo en una parada (Upgrade / Shop / Forge / Rest). Élites y
  Recruit no se tocan. Resultado: ~12 nodos de pelea por mapa (antes ~17).
- Animación mientras elegís la ruta (hover / avance), para que el camino
  elegido quede marcado y el avance se sienta fluido.

### 10.5 Items nuevos
Regla: todo lo "si está quieta" **escala con la velocidad** (más lenta = más
efecto), nunca es un sí/no; quedarse quieta no es obligatorio.
**Prioridad: sinergias.** Cada item nuevo tiene que combinar con items,
elementos o Pacts que ya existen; no hace falta que cada clase tenga los tres
estilos (quieta / en movimiento / en pareja). "En pareja" (seguir a otra
pelota) queda solo para Support y Summoner, que juegan solas. Las clases
donde más sentido tiene tirar (Striker, Assassin, Jester) llevan más items de
tiro/atrapada; el reparto final se equilibra en cada tanda. Guardian juega
solo pero cuidando el núcleo.

- Sin clase: **Ballast** (modificador que se apila: -15% velocidad, +20% daño).
- **Clase nueva: Slinger** (la clase de tirar y atrapar; así no se tocan las
  demás y combina con Striker / Assassin / Jester en la misma pelota). Nombre
  elegido porque "Thrower" es un arquetipo de Creed y "Juggler" es un Pact.
  Items: **Coil** (frena hasta 0; tirada por vos sale al doble), **Catch &
  release** (atraparla poco después de tirarla apila daño), **Afterburner**
  (tirada rápida deja estela de fuego que aplica el elemento Fuego de verdad:
  mismo estado, reacciones y nodos del árbol que una pelota de fuego),
  **Momentum** (más rápido = más daño), **Grip** (se curva hacia el cursor
  cerca de él), **Ambush** (tirada: blink al primer golpe + golpe de Backstab),
  **Trick shot** (chances x3 hasta el primer golpe tras un tiro), **Double
  down** (atrapar una pelota que tiraste vos = próximo golpe doble o nada),
  **Execution throw** (primer golpe de tiro a enemigo con vida llena = crítico).
  Clase (2 items): atraparla paga 50% más de premio y el primer golpe tras tu
  tiro pega +25%. Ascendida "Master Slinger" (4 items): cada atrapada recarga
  35% de sus habilidades y el golpe tirado pega +50%. Sus items funcionan con
  1 solo item en cualquier pelota (como Jester). En la web: ruta propia que
  sale del centro (entre Striker y Shooter): nodo "Slinger" (18 cores) y
  "Slinger lore". Regla por ahora: toda rama de la web sale del centro.
  Striker también es una clase a desbloquear (nodo "Striker", 12 cores, en su
  ruta tras Heft y Sling). ~~Mientras no tengas ninguna clase comprada, Striker
  queda abierta como clase de arranque.~~ Desde 2026-10-03 (pedido del
  usuario) no hay clase de arranque: los items de Striker recién aparecen al
  comprar su nodo. Sin ninguna clase, las élites ofrecen otras cartas.
  Ballast (modificador) pasa al paso 6, con los modificadores nuevos.
- Las demás clases solo suman items que escalan con la velocidad:
  Guardian **Anchor**, **Plow**; Shooter **Slug** (más lenta = más cadencia y
  rango), **Strafe**; Assassin **Lurk**, **Blur**; Jester **Sleight**, **Wild
  ride**; Mage **Meditate**, **Leyline**; Support **Beacon**, **Wake**, **Pass**,
  **Link**; Summoner **Kennel**, **Pack**, **Familiar**, **Drop turret**.
  (Landslide, Recoil, Spellsling, Mark throw, Roulette quedan en reserva:
  si hacen falta, van a Slinger.)
  **Hecho (2026-09-28):** 14 items: Anchor, Plow, Slug, Strafe, Lurk, Blur,
  Sleight, Meditate, Leyline, Beacon, Wake, Pass, Kennel, Drop turret.
  Quedaron afuera por repetir algo que ya existe: Link (= Tether), Wild ride
  (= Chaos bounce); Pack y Familiar (Summoner "en pareja") para más adelante.
  Beacon y Pass le pasan el elemento de una pelota a otra: dos dueños
  distintos, así que reaccionan (sinergia con elementos / Catalyst).
  Todo lo "quieta" combina con Coil, Lead y Stillness.

### 10.6 Orden de trabajo
1. Renombre Pact -> Creed.  2. Premio por atrapar.  3. Pacts + Altar + camino
secreto + animaciones de mapa.  4. Clase Slinger.  5. Items de velocidad en las otras clases, por tandas.
6. Después: repasar los elementos (quedaron desactualizados) y sumar muchos
modificadores pasivos nuevos (hoy son solo Heavy impact / Big ball / Swift y
siempre se eligen los mismos 3).
**Elementos rehechos (hecho 2026-09-28):**
- Fuego = incendio que se contagia: cada golpe prende y la quemadura sube
  (sin necesitar Ember); un enemigo que muere quemándose explota y prende a
  los de al lado. Ember: +35% de quemadura por nivel. Golpe +25% (antes +60%).
- Agua = mojar y empujar: el golpe empapa (camina 30% más lento, sale 50% más
  lejos al ser golpeado, se congela el doble) y cada 1,5 s la pelota lanza una
  OLA: un arco hacia donde va que avanza y crece, empuja hacia afuera y empapa
  a cada enemigo que cruza (sin daño propio). Ya no tiene estela.
  Electrocution electrifica a lo que está dentro de una ola.
- Piedra = la pesada que agrieta (support/tanque): cada golpe agrieta (+12% de
  daño recibido de TODO, hasta 5, 4 s); pelota 10% más lenta, empuja 30% más.
  Ya no tira escombros. Bedrock: grietas 50% más largas y hasta 7.
- Reacciones de piedra: Magma (charco de lava), Barro (charco que frena),
  Polvo tóxico (nube de veneno), Esquirlas (explosión que agrieta x2), Imán
  (junta a los enemigos).

**Modificadores nuevos (hecho 2026-09-28):** Ballast, Keen, Reach, Tempered,
Spin, Leech, Quick mind, Heavy throw, Bouncy (12 en total), para que la
elección post-pelea no sea siempre la misma. Arreglado: un reroll en una
elección de modificadores ya no puede dar un item.

**Clase Alchemist (hecho 2026-09-28; se llamaba "Elementalist" en el plan):**
con la clase (2 items) la pelota lleva 2 elementos que se turnan golpe a golpe
y reaccionan entre sí (una sola pelota ya genera reacciones); ascendida
"Archalchemist" (4): un 3er elemento y reacciones +30%. Agua y eléctrico
actúan solos si están entre sus elementos. Los extra se guardan al lado del
slot de tipo (hexágonos chicos) y comparten su nivel. Items: Attune, Crucible,
Aftershock, Flux, Prism, Conflux. Su ruta de la web sale del centro y tiene
los 6 elementos (más Ember y Prism core); el arquetipo de Creed "Alchemist"
pasó a decir "ALCHEMY".

7. **Clase nueva: Elementalist** (va junto con el repaso de elementos). Cada
clase se trata de algo; esta es sacarle más a los tipos. Como el Mage con los
slots de habilidad, pero con elementos: con la clase (2 items) la pelota tiene
2 elementos, ascendida (4 items) tiene 3. Sus dos elementos reaccionan entre
sí en la misma pelota. Sinergias: Catalyst, Chain reaction, Primed, el Creed
Alchemy y los nodos de elemento del árbol. Nombre elegido porque "Alchemist"
ya es un arquetipo de Creed.
8. Después de todo esto: estudiar la progresión (qué se desbloquea y cuándo)
y mejorar el árbol meta (la web). Clases cerradas en 10: las 8 de hoy +
Slinger + Elementalist; las combinaciones se hacen con los items.

## 11. Pasada de balance post-playtest (2026-10-01)

El usuario jugó una run: "está muy duro y eso me gusta". Cambios de esa charla
(todo en `ui-polish`):

- **Más pelotas tarde:** tope de 5 pelotas (`cfg::ball::baseBalls`) y +2 desde
  el acto 3, o sea después de vencer al jefe del acto 2 (`lateBalls`,
  `lateBallsAct`; `App::ballCap()`, Duet sigue en 2). Al entrar al acto 3 sale
  "+2 BALL SLOTS". `maxBalls` = 7 es el techo duro del World. El TAB con 7
  paneles se achica (zoom mínimo 0.6).
- **Tecla F = tirar una pelota al azar** (desde el acto 2, `cfg::app::autoThrowAct`):
  una pelota libre al azar va al enemigo más cercano, como un click pero sin
  premio de atrapada, con 1.6 s de recarga (`autoThrowCooldown`) para que
  clickear siga siendo mejor. Chip "F" a la derecha de las pelotas, con la
  recarga llenándose. No existe con Hunters / Clockwork (sin manos).
- **Saltear elecciones:** botón "Skip" bajo las cartas (al lado de "Repair the
  core instead" si el núcleo está herido). Prospector devuelve rerolls también
  acá.
- **Tiendas:** muchas menos. Como mucho `cfg::map::shopsPerAct` = 1 en los
  caminos de cada acto, más la fija de la fila previa al jefe; las demás pasan
  a "?" (~2.4 -> ~1.8 por mapa). En cambio, el **reroll es ilimitado** y cada
  uno sale bastante más caro que el anterior: 15 x 1.7^n (15, 26, 43, 74, 125,
  213...). Merchant ahora da el primer reroll gratis por nivel. La estantería
  trae un item el 60% de las visitas (antes 40%).
- **Eventos en los "?"** (`progression/Events.hpp`, `core/AppEvents.cpp`,
  `ui/EventScreen.*`): el 40% de los "?" (`cfg::event::chancePct`) es "A
  stranger" con 2 tratos a elegir o irse. Los precios suben por acto:
  - Drifter: una pelota nueva por 120 (+40 por acto; 200 en el acto 3).
  - Smuggler: elegir 1 de 3 items (probabilidades de élite) por 70 (+20).
  - Blood price: +60 oro (+20) a cambio de -12% de vida máxima del núcleo.
  - Tithe: +15% de vida máxima del núcleo (y la cura) por 50 (+15).
  - Coin flip: apostar 40 (+15); 50% de cobrar x2.5.
  - Wandering smith: subir de nivel un item por 40 (+10).
  Un trato que no podés pagar se ve apagado. Los que no aplican (sin lugar
  para pelotas, nada para forjar) no salen.
- **Enemigos nuevos:**
  - **Snare** (desde la oleada 13): atrapa la primera pelota que lo golpea y la
    tiene quieta (no se puede agarrar) hasta que lo matás con otra. Nunca atrapa
    tu última pelota libre. Al morir, la suelta a velocidad de crucero.
  - **Andares:** grunts, runners, splitters y shielded pueden avanzar en
    zigzag (Weave, desde la oleada 3) o en espiral alrededor del núcleo
    (Spiral, desde la 11), y se vuelven más comunes cada acto. Medido sin
    ventana: llegan al núcleo en tiempos parecidos a los que van derecho
    (~9-11 s), así que la presión es la misma pero hay que leerlos distinto.
- **Golpes de pelota más suaves:** voces propias para los golpes (8), nunca
  dos notas a menos de 55 ms, el volumen baja cuando se amontonan (1/raíz de
  la densidad reciente), nunca la misma nota dos veces seguidas, ataque más
  suave en el estilo Soft, y los acordes del combo solo si no está saturado.

### Habilidades del jugador: Q y E (2026-10-01)

Para que el jugador haga más que atrapar. Son del jugador, no de una pelota
ni de un build (`cfg::player`). Los chips están abajo a la izquierda, al lado
de las pelotas: Q, el anillo de E y F.

- **Q = Volley** (desde el arranque; reemplazó a "Marcar" el mismo día, a
  pedido del usuario): como clickear todas las pelotas a la vez pero más
  rápido. Cada pelota libre (no la que tenés en la mano, ni una atrapada por un
  Snare o un Satellite) sale por el mismo `releaseHeld` que un click, directo
  al enemigo más cercano **a esa pelota** (o al objetivo, si marcaste uno), a
  1.4x la velocidad del click y sin premio de atrapada. Recarga en 1.25 s (era 1 s; +25% a pedido del usuario, 2026-10-03).
  Medido sin ventana: con 4 pelotas, un Tank de la oleada 4 muere en 1 s. Muy
  fuerte y spameable: vigilar en el playtest.
- **El click a una pelota** ahora la tira al enemigo más cercano **al núcleo**
  (el que está por pegarle), no al más cercano a la pelota.
- **Objetivo del jugador:** click sobre un enemigo (o el jefe), sin una pelota
  debajo, lo marca con una mira; otro click encima lo desmarca, y se limpia al
  morir o en cada oleada. Mientras vive, todo lo que apunta va a él: el click,
  Q, Dash, los misiles del Mage, el Shooter, el Assassin, el Summoner y sus
  torretas, el Ambush del Slinger (los que tienen alcance, solo si está dentro
  del alcance), Seeker, Hunter, Clockwork y los rebotes apuntados
  (`World::focusAt / focusPos`; `nearestTarget` lo devuelve primero). Lo que
  solo pregunta "¿hay algo cerca?" para dispararse no cambió.
- **E = Tiempo bala** (desde el acto 2, se mantiene apretada): el combate va
  a 0.3x mientras la tenés apretada. Es un **recurso que el jugador administra**
  (pedido explícito): un anillo verde con 3 s reales de cámara lenta que se
  vacía al usarlo y se recarga solo (de vacío a lleno en 14 s, empieza 0.8 s
  después de soltar). Hace falta un 8% para volver a arrancarla. El pacto
  Heavy Arm también la quita.
- **Stockpile se retiró:** era la reserva de power-up en Q. Su nodo queda en el
  enum (no se mueve ningún índice del save) con maxLevel 0, lo que lo saca de la
  web (`metaNodeRetired`). Magnet ocupa su lugar y cuelga del mismo padre.
- **F = Repulsión** (desde el acto 2; reemplazó al "tiro de una pelota al
  azar", que al usuario no le gustó): el núcleo suelta una onda que empuja a
  todo enemigo a menos de 270 px y lo aturde 1 s (los empapados vuelan x1.5).
  Recarga 10 s. Medido: aleja ~215-230 px. En pelea F es esto; afuera sigue
  siendo pantalla completa (F11 siempre).

- **Dev (SB_DEV=1):** en el mapa podés ir a cualquier nodo, no solo a los que
  están conectados (`App::mapNodeOpen`), para probar lo que quieras.

### Guardar la run (2026-10-01)

- La run en curso se guarda aparte, en `saves/run.txt` (`saveRun / loadRun /
  clearRun` en `platform/Save.cpp`), **cada vez que se abre el mapa**
  (`App::openMap -> saveRunNow`). Guarda la vida del núcleo, el oro, el
  puntaje, las pelotas (items, elemento, habilidades, modificadores), las
  reliquias, los creeds, los pactos, el mapa entero con lo recorrido, la
  posición, la racha del Altar y los usos de Phoenix / Last stand.
- El menú principal muestra **"Continue run"** arriba de todo si hay una run
  guardada, y "Play" pasa a llamarse "New run". `App::resumeRun` la carga,
  arma el World con esas pelotas y ese núcleo, y abre el mapa.
- Si cerrás en medio de una pelea (o en una tienda o elección después de
  ella), volvés al mapa de antes: esa pelea sigue por delante. Se puede
  "reintentar" una pelea saliendo; aceptado por ahora.
- Se borra al perder o ganar (`bankRun`), al abandonar, al volver al menú
  de fin de run, al empezar una run nueva y con Reset progress. Un archivo
  con otro formato o roto no se carga: se borra y listo.
- El modo snapshot hace la prueba de ida y vuelta (guardar, leer, comparar
  campo por campo, reanudar) y saca 33_menu_continue / 34_resumed_map.

### Web de habilidades más grande (2026-10-01)

- Anillos a 124 px (antes 56), nodos de 15 / 22 (clase) / 26 (centro) px y más
  aire alrededor del centro: a zoom 1 la web es más grande que la pantalla y
  se recorre, no se ve toda de un vistazo.
- Navegación: arrastrar o WASD para moverse, rueda para zoom (suave, hacia el
  puntero), **click en una ruta de la leyenda = volar a su clase**, flechas =
  saltar al nodo vecino con la cámara siguiéndolo, 0 / Home = volver al centro.
  La cámara recuerda dónde la dejaste entre visitas.
- Cada nodo muestra su nombre (lo comprado en su color, lo que podés pagar en
  blanco, lo demás apagado); lo comprado brilla, lo que podés pagar respira con
  un anillo, las uniones de tus caminos se encienden.
- Tarjeta de info más grande a la izquierda: nombre, ruta, nivel, **"what it
  gives"** en letra legible, y abajo el costo del próximo nivel o qué te falta.
- Una franja arriba (título y cores) y fondos detrás de la leyenda y de los
  botones, para que la web pase por debajo sin ensuciarlos.
- Fondo vivo, como la pantalla de inicio: el radar con orbes que flotan
  (`MenuBackdrop`) y 5 planetas en órbitas elípticas lentas alrededor del
  centro de la web (algunos con anillo o luna), con paralaje de 0.3x respecto de
  la cámara (`kPlanets`, `LoadoutScreen::drawSky`). Tenues: la web es lo
  principal.
- **El mapa de la run tiene el mismo fondo** (orbes que flotan y planetas en
  órbita, `drawOrbitingPlanets` en `ui/MenuBackdrop.cpp`, compartido con la
  web); los planetas se deslizan un poco con el scroll del mapa.
- **Los nodos de clase son distintos a todo lo demás:** un círculo geométrico
  (disco, un círculo interior fino y una estrella de seis puntas que gira
  adentro), resplandor fuerte en su color y una órbita propia (anillo punteado
  que gira y tres lunas en sentido contrario). Más apagados mientras no se
  pueden comprar.
- **El anillo de las clases:** todas las clases están a la misma distancia del
  centro (anillo 4, `kClassRing`; las que los datos ponían en el 3 se corren
  hacia afuera junto con lo que cuelga de ellas, `webRing`). La web ya no se
  estira en horizontal (`kStretchX` = 1), así que el anillo es un círculo, y un
  aro rojo de fondo lo marca, con "CLASSES" en el hueco más grande entre dos
  clases.
- **Niebla:** un nodo bloqueado solo se ve si está a 2 pasos o menos de algo
  que ya compraste (en una web nueva, del centro; `webVisible`). Lo comprado
  siempre se ve. Lo oculto no se dibuja, ni sus uniones, ni se puede
  seleccionar.
- **Layout geométrico (árbol radial):** los ángulos y anillos de los datos ya
  no se usan para ubicar (el ángulo solo ordena a los hermanos). Cada nodo
  tiene una porción del círculo proporcional a las puntas de la web que salen
  de él (cada ruta cuenta como al menos 5), y sus hijos se reparten esa
  porción igual: todas las puntas quedan equiespaciadas (`webAngle`). El
  anillo es la profundidad en el árbol, salvo que el camino de cada ruta a su
  clase se reparte parejo entre el centro y el aro de clases, y lo que cuelga
  después va de a un anillo por paso (`webRing`).
- **Dev (SB_DEV=1) en la web:** F1 = +1000 cores y +10 prisms, F2 = volver a
  bloquear todos los nodos devolviendo lo gastado, F3 = desbloquear toda la
  web al máximo (gratis). Una línea roja bajo el
  título lo recuerda.

### 2026-10-02: bandas negras y el descanso con elección

- **Bandas negras arriba y abajo de la arena** (`cfg::app::arenaBand` = 36 px
  de UI por lado): la arena es más baja que la pantalla (1280x728), así que la
  cámara, que siempre encuadra con el aspecto de la UI, deja una franja negra
  arriba y abajo. Una pelota pegada a la pared de arriba o de abajo se agarra
  cómoda. El marco, el tinte del combo y el de bullet time siguen a la arena
  (`App::arenaRect`); las bandas se dibujan sobre el mundo y bajo el HUD
  (`App::drawArenaBands`). La arena del jefe usa el mismo aspecto, así que
  también tiene bandas.
- **La honda no se corta en el borde de la pantalla:** mientras apuntás, el
  cursor se oculta y el tiro se mide por el movimiento del mouse
  (`Window::setMouseCapture` / `pumpCapture`), y el cursor se recentra a
  escondidas cuando se acerca al borde. Una pelota pegada a cualquier pared,
  en pantalla completa, se tira a toda potencia. Al soltar, el cursor vuelve
  donde terminó el tiro (dentro de la ventana).
- **El nodo de descanso ahora es una elección:** reparar el núcleo (Rest) o
  subir de nivel un ítem (Forge), con las cartas del evento "?" en el color
  del núcleo (`App::openRestStop`, `EventKind::Rest/Temper`). Si no hay nada
  para forjar, repara directo como antes. "Move on" sale sin nada.

### 2026-10-03: el HUD vive en las bandas negras

- **La arena queda limpia:** solo pelotas, enemigos y núcleo. Todo el HUD de la
  pelea va en las bandas negras (`cfg::app::arenaBand`). Arriba, en una línea:
  el combo a la izquierda, "act N  stage x / y" con su barra y "N left" al
  centro (en un jefe, su nombre y su vida), y el oro a la derecha. Abajo: las
  pelotas, Q / E / F y TAB a la izquierda, el power-up activo al centro, y los
  credos y pactos (y la carga de Nova) a la derecha.

- **Web:** el panel de rutas y el de Start run / Mode / Back comparten el
  borde de abajo y el texto va centrado en su caja; las rutas se minimizan con
  el botón "-" de su encabezado (se recuerda mientras el juego está abierto).
- **Mapa de la run:** TAB en la banda negra de abajo y los credos al final de
  la línea de estado, arriba: nada del HUD dentro del marco.

- **Mapas cortos hasta la primera clase:** mientras no tengas ninguna clase
  comprada en la web, el acto 1 tiene la mitad de filas (`cfg::map::rowsFirstRun` = 6 en
  vez de 11), así el jugador llega antes al jefe y a la web con cores para una
  clase. El mapa guarda sus filas (`RunMap::rows`, también en `run.txt`), y las
  oleadas se reparten sobre ese largo; la fila 1 siempre es la oleada 1.
  Ganarle al jefe paga 30 cores (Slinger 18, o la ruta de Striker 30); morir
  en el jefe, ~18.

- **Pantalla de derrota (`RunOverScreen`):** cuando cae el núcleo, la run se
  cobra (`bankRun(false)`) y sobre la pelea congelada sale "The core fell":
  acto y stage (o el jefe), qué llegó al núcleo (`World::lastCoreHitBy`),
  kills / mejor combo / tiempo de esa run (contados desde el arranque o el
  resume, `App::startRunTally`), las pelotas con sus clases y los cores
  ganados. "Skill web" (Enter) o "Menu" (Esc). Mientras está arriba la run ya
  no está activa: sin pausa ni TAB, suena el menú.

### 2026-10-04: guía del primer run (hints)

- Una sola línea a la vez, centrada en la banda negra de abajo, con un pulso
  suave. Cada hint sale **una vez en la vida**: se va apenas hacés lo que dice
  (o a los 25 s si nunca lo hacés). Cuáles ya hiciste queda en el save
  (`hints`, un bit por `App::Hint`). Código en `core/AppHints.cpp`.
- **En pelea**, en orden: click en una pelota (tirar), tirar con la honda
  (apuntar), Q, E (desde que se abre), F (desde que se abre), TAB.
  **En el mapa:** elegir una parada, después TAB.
- No salen con el TAB abierto, en pausa ni fuera de la run; entre un hint y
  el siguiente hay 1.5 s de respiro.
- **En el mapa**, después de "click a stop": la primera vez que tenés
  adelante un Recruit / Elite / Shop / Forge / Boss, una línea dice qué es.
- **En la web**, después de la primera run: "your cores buy upgrades: click a
  lit node" y después "Start run: a real one this time".

### 2026-10-04: el tutorial (la primera run)

- Si nunca jugaste una run (`stats.runs == 0`; con `SB_DEV=1 SB_TUTORIAL=1`
  se fuerza), la run es el tutorial (`RunState::tutorial`, guardado en
  run.txt): un mapa recto (`tutorialMap`) combate → recruit → elite → tienda
  → forja → boss, sin creed ni starter pick. Las paradas suben suave (la
  fila N juega como la oleada N); el boss es el del acto 1 con 60% de vida
  (`cfg::tutorial::bossHpMul`). Sin "Continue" al ganarle.
- Gane o pierda termina en la web, donde salen los hints de la web. En la
  pausa, "Abandon run" pasa a ser "Skip tutorial".
- El primer "Play" del menú no abre la web: arranca directo el tutorial
  (`App::openLoadout`). La web se conoce al terminarlo.

### 2026-10-04: bosses en orden al azar

- Los bosses de los actos 1-4 (Charger, Hive, Warden, Dasher) salen en un
  orden nuevo cada run (`RunState::bosses`, guardado en run.txt; las runs
  viejas quedan con el orden fijo). El Orbital sigue siendo siempre el final
  (acto 5). El tutorial abre siempre con el Charger.
- La vida de cada boss ya escala con la oleada (`World::bossHp`), así que
  cualquiera puede caer en cualquier acto. El Charger usa la oleada real (antes
  estaba fijo en la 10).

### 2026-10-04: pirámide de rarezas y los 39 comunes

- **Regla (el usuario):** por clase, más comunes que poco comunes > raros >
  épicos > legendarios, y mínimo 2 legendarios. Meta por clase: 6 / 5 / 4 / 3 / 2
  (20 items). Support y Striker tienen la pirámide al revés: se van a bajar
  algunos de rareza (no sacar) cuando lleguemos a esas rarezas.
- **Sinergias indirectas:** cada item nuevo tiene que combinar también con
  cosas de otras clases, no solo de la suya.
- **Tanda 1 - comunes (hecha):** 39 items en `sim/WorldCommons.cpp` (tabla
  `commonItemDef`, números `cfg::common`, fold `foldCommon`). Funcionan en
  cualquier pelota, sin necesitar la clase. Van al final del enum, antes de
  las reliquias, para que las runs guardadas no cambien de items.
  - Alchemist: Volatile, Seep, Reagent, Tincture, Distill.
  - Jester: Pickpocket, Hot streak, Lucky break, Confetti.
  - Mage: Spellblade, Aether step, Insight.
  - Summoner: Lingering, Pack leader, Decoy.
  - Assassin: Mark for death, Bleed, Quickstep, Opportunist.
  - Shooter: Hollow point, Bank shot, Covering fire.
  - Slinger: Long pull, Relay, Second wind.
  - Guardian: Plating, Sentinel, Brace, Heavy landing, Bulk.
  - Striker: Edge, First blood, Combo hit, Adrenaline.
  - Support: Spotter, Anthem (era "Rally", chocaba con el nodo de la web),
    Medic, Sync (era "Uplink", ídem), Hand-off.
- **Tanda 2 - poco comunes (hecha):** Fission, Elixir (Alchemist); House edge,
  Mirror (Jester); Sigil, Arcane charge, Conduit (Mage); Martyr, Guard dog
  (Summoner); Chain kill, Venom blade (Assassin); Suppressing fire, Ammo feed
  (Shooter); Wind-up, Bank throw (Slinger); Bully, Carom (Striker). Mismo
  archivo y misma regla que los comunes.
  - Bajados de rareza por la pirámide: Mender (→ común); Cleave, Executioner,
    Overkill, Midas, Pass (→ poco común).
- **Tanda 3 - raros (hecha):** Transmute, Residue, Aurum (Alchemist); High
  roller, Lucky seven, Fortune (Jester); Projection, Spellweave, Cascade (Mage);
  Hatchery, Golem, Soulbind (Summoner); Predator (salto al doble de rango, al
  más débil), Flicker (Assassin); Frag rounds, Marksman (Shooter); Skipping
  stone, Formation (Slinger); Shield bash (Guardian). Golem es una invocación
  nueva: cuenta para Guard dog, Soulbind, Lingering, Martyr y Pack leader.
  - Storm y Tether bajan de épico a raro (pirámide de Support).
- **Tanda 4 - épicos (hecha):** Saturate, Maelstrom (Alchemist); Death
  sentence, Razor wire (Assassin); Ward ring, Juggernaut, Last line (Guardian);
  Slot machine, Wild magic (Jester); Spell chorus, Chronomancy (Mage); Minigun,
  Smart rounds, Shrapnel (Shooter); Perfect release, Homecoming (Slinger);
  Lightspeed (Striker); Spirit host, Twin call, Fortify (Summoner).
- **Shooter más fuerte de base (el usuario: el Magic missile era mucho mejor):**
  dispara cada 0.4 s (era 0.9), alcance global (era 330 px), balas más rápidas
  (950, viven 2.4 s), una pelota lenta ya no dispara más lento, daño por bala
  0.3 del golpe (era 0.35). En una prueba sin jugador pasó de ~14 kills a ~40
  en 6 peleas (Magic missile solo: ~6-12).
- **Reroll del kit inicial (Quartermaster):** ahora saca otro item del mismo
  kit; antes usaba el sorteo de mejoras y daba elementos / modificadores.
- **Panel F1:** tres pestañas - Items 1 (Striker..Shooter), Items 2
  (Assassin..Alchemist), Run - y cada clase ordenada de común a legendario.
- **Tanda 5 - legendarios (hecha, 2026-10-05):** 16 items; con esto la
  pirámide queda completa. Mismo archivo y misma regla.
  - Guardian: **Titan** (cada rebote en el núcleo lo agranda, hasta el doble;
    lleno, el siguiente rebote hace un terremoto que aturde a todos). El usuario
    descartó "Citadel".
  - Mage: **Archmage** (habilidades al doble de velocidad, cada cast +10% hasta
    el fin de la pelea), **Time stop** (cada 5 casts, los enemigos quietos 2 s;
    el jefe no se frena).
  - Shooter, **3 legendarios** (el usuario lo pidió; excepción a la regla de
    "más épicos que legendarios"): **Ion cannon** (40% más lenta, sin balas:
    carga 2 s y dispara un láser que cruza la arena por la línea con más
    enemigos, x4; reemplazó a "Sniper"), **Gunship** (las otras pelotas
    disparan una bala al pegar, con los items de Shooter de esta), **Lead
    storm** (cada 10 s, 2 s de fuego x6 en espiral).
  - Assassin: **Reaper** (el blink sale después de cada golpe, cooldown 0.3 s),
    **Contract** (el más fuerte - jefe, si no un Brute, si no el de más vida -
    recibe +10% de su vida máxima por golpe; 2% del jefe).
  - Summoner: **Necromancer** (lo que matan sus invocaciones se levanta como
    fantasma que explota; sus muertes no levantan más), **Overlord** (todas las
    invocaciones de todas las pelotas pasan a ser suyas: su elemento, sus items,
    +20%).
  - Jester: **Rigged** (toda probabilidad de la run se tira dos veces),
    **Joker** (en cada pelea copia un legendario al azar de los que se pueden
    encontrar, `App::rollJoker`).
  - Slinger: **Steady hand** (apuntando, el tiempo va al 20% y no se acaba -
    la cámara lenta normal es 30% por 2.5 s; cuanto más apuntás, más pega el
    primer golpe, hasta +60%), **Showtime** (cada atrapada rápida +20% daño y
    velocidad sin tope en la pelea; una lenta deja la mitad).
  - Alchemist: **Philosopher's stone** (cada golpe deja 2 elementos al azar de
    los 6 - siempre reacciona; 3 y 4 en Lv2/3), **Grand reaction** (cada 10ª
    reacción, x5 en toda la arena).
- **F (Repulse) se abre en el acto 4** (antes en el 2, igual que la E). La E sigue en el 2.

### 2026-10-05: 15 habilidades nuevas (de 8 a 23)

- **Pedido del usuario:** más del doble de habilidades, que cambien el juego y
  sin repetirse con los items. Eligió estas 15 de dos listas propuestas.
- **Poco comunes:** Hook (tira al enemigo más cercano al núcleo hacia la
  pelota y lo aturde), Mines (3 minas detrás, explotan al pisarlas, con su
  elemento), Empower (el próximo golpe x3 y una onda), Tailwind (todas las
  pelotas +30% de velocidad 3 s), Swap (cambia de lugar con el enemigo más
  cercano al núcleo, solo si está más adentro que la pelota), Shrink (los
  enemigos cerca a la mitad de tamaño 4 s: más lentos, reciben el doble),
  Bounty (el enemigo más fuerte paga 5 de oro al morir).
- **Raras:** Vortex (remolino 2 s que junta enemigos), Beam (rayo continuo 1 s
  al más cercano), Blades (3 cuchillas giran 4 s), Cloud (nube de su elemento
  3 s, reacciona con cualquiera), Smite (rayos sobre los 3 más fuertes, también
  el jefe), Warp (se teletransporta entre el núcleo y el enemigo más cercano a
  él y lo saca para afuera; cuenta como blink para los items del Assassin).
- **Épicas (las primeras habilidades épicas):** Portal (dos portales 4 s, uno
  junto al núcleo y otro donde está el enemigo más lejano; las pelotas pasan
  de uno al otro, +20% de velocidad), Leash (atada al núcleo 4 s, gira como un
  mangual sin poder alejarse).
- **Desbloqueo, sin nodos nuevos:** Hook, Swap y Empower vienen abiertas (con
  Dash y Nova, 5 de entrada). Cada nodo de habilidad abre 2 más: Magic
  missile -> Beam, Blades; Arc -> Cloud, Smite; Bulwark -> Vortex, Shrink;
  Split -> Portal, Mines; Overclock -> Tailwind, Leash; Meteor -> Warp, Bounty.
- **Código:** `sim/WorldAbilities.cpp` (casos de `fireAbility`, `abilityTick`
  por pelota, `abilityWorldTick` para minas / remolinos / nubes / portales),
  números en `cfg::ability`. En el enum van **después de las reliquias**
  (`AbilityHook..AbilityBounty`) para que las runs guardadas no cambien de
  números; `upgradeCat` y `abilityOf` leen ese rango.
- **Panel F1:** la pestaña Run tiene una columna propia de habilidades.
- **Personalidad visual (mismo día):** cada habilidad tiene su gesto, corto y
  fino, en los colores de la pelota. Hook = cadena que sale, muerde y recoge;
  Smite = rayo quebrado desde arriba que parpadea y deja chispas; Warp =
  fantasma donde estaba + costura punteada + anillo que se cierra al llegar;
  Swap = dos cometas curvos que se cruzan; Shrink = anillo que se cierra y el
  enemigo se encoge con rebote; Empower = aro que respira y, al pegar, estrella
  de púas; Mines = caen con rebote, anillo que se cierra al armarse, parpadean,
  metralla al explotar; Bounty = aro dorado girando, moneda flotando, lluvia de
  monedas + "+N" al cobrar. Las que duran se mueven: Vortex con motas en
  espiral y ojo oscuro, Cloud con bocanadas y pulso en cada mordida, Portal que
  se abre con dos anillos a contramano, Beam que zumba con chispas, Blades con
  estela, Leash con cuentas que corren (más brillante tensa), Tailwind con
  rayas de viento. De las viejas: Dash deja copias, Meteor cola de fuego +
  cráter con brasas, Overclock anillo de calor. Algunas sacuden la cámara
  (Smite, Meteor, Empower, Warp, minas). Los gestos viven en
  `CommonWorld::fx` (sólo dibujo, `World::addFx`), dibujados en
  `render/ClassRender.cpp` (`worldAbilities`).
- **Efectos de items únicos (mismo día):** ningún item reusa el anillo genérico;
  cada uno tiene su forma en `CommonWorld::fx` (`World::itemFx`): Death
  sentence = corte, Lucky break = trébol, Maelstrom = cuentas de sus elementos,
  Slot machine = fuente de monedas / rodillos, Homecoming = bumerán, Last line =
  muro hexagonal, Sigil = triángulos, Titan = grietas, Heavy landing = polvo,
  Time stop = reloj, Spell chorus = eco, Grand reaction = estrella, Ward =
  placa hexagonal, Fission = hilo al vecino, Mega Striker = boom sónico, Mage
  ascendido = rombo, Lead storm = cañones, Frag = perdigones, Smoke bomb = humo,
  Necromancer = almas que suben, Martyr = columna de luz, Bomber = bola de fuego,
  bomba del Jester = chispas, Jackpot del Jester = confeti, Sleight = cartas,
  Ambush = flechas, Golem = losa que sube / golpe cuadrado, gran espíritu =
  espiral, Black hole = colapso, Seeker = línea de mira.
- **Portal (rework):** cooldown 7 s (era 12), 3 s abierto, portales más cerca
  (el de salida a lo sumo 220 px más allá del del núcleo, hacia el enemigo más
  cercano al núcleo) y más grandes (34). Lo que entra sale **disparado** hacia el
  enemigo más cercano a la salida (x2.4 crucero).
- **Seeker (rework):** ya no curva en vuelo (eso era casi Hunter). Ahora cada
  enemigo que golpea lo hace salir derecho al siguiente más cercano (pinball);
  niveles: ve más lejos y sale más rápido. Hunter sigue siendo el que persigue.
- **Sin spam de click:** una pelota recién tirada no se puede volver a agarrar
  por 1 s (`cfg::app::regrabDelay`), así mantiene su camino. (Q sigue en 1.25 s.)
- **Seeker (ajuste):** busca en toda la arena, siempre un enemigo del lado del
  rebote: encadena enemigo a enemigo hasta tocar pared o núcleo.
- **Time stop:** cada 15 habilidades (era 5).
- **Panel de pelota:** las habilidades 2 y 3 del Mago van apiladas dentro del
  panel (la 3ra quedaba afuera y no se podía agarrar en el TAB).
- **Eventos "?" nuevos** (además de los 6 de oro): Barter (das un item, recibís
  2 al azar de rareza al azar), Recruiter (elegís 1 de 3 items; una pelota al
  azar se va 2 peleas - `RunState::bench`, vuelve sola a su lugar), Lucky find
  (una mejora al azar en una pelota al azar, gratis), Old tome (una habilidad al
  azar por oro), Blood forge (un item al azar sube de nivel por -10% vida máx. del
  núcleo), Strange cocoon (item Cocoon que no hace nada; su nivel cuenta las
  peleas, a las 3 sale un épico o legendario - 25% legendario). Los regalos se
  colocan con la pantalla de equipar (`EquipSource::Gift`, Back = dejarlo).
- **Marcas de épicos/legendarios:** Homecoming (flecha en U volviendo), Perfect
  release (brillo), Juggernaut (borde pesado), Minigun (cañones girando),
  Lightspeed (chispa alrededor cuando va rápido), Archmage (motas por cast),
  Showtime (foco que crece), Philosopher's stone (6 cuentas de elementos),
  Overlord (corona), Rigged (dado), Gunship (alitas), Hunter (flechas sobre la
  presa), Death sentence (X sobre los sentenciados), Smart rounds (aro en la
  bala). Al activarse: Mana spring (motas que entran), Conflux (salto en arco),
  Saturate (dos anillos), Wild magic (chispas de colores), Chronomancy (onda
  punteada lenta), Reaper (guadaña), Lightspeed (rayo recto blanco).
- **Mitosis / Gemini:** las copias de Mitosis salen "desnudas" (stats y
  elemento, sin items, clase ni habilidades); los gemelos de Gemini copian todo
  y además lanzan las habilidades, medio cooldown desfasados de su pelota.
- **Slinger, dos items nuevos en lugar de viejos (mismo lugar en el enum):**
  Beckon (épico, en lugar de Double down): click en suelo vacío = la pelota con
  Beckon aparece ahí y sigue volando; cada 1.5 s (1 s a Lv5); con varias, va la
  que tiene más niveles (items + habilidades + mejoras), empate al azar.
  Ping-pong (poco común, en lugar de Bank throw): después de tu tiro, cada pared
  lo manda derecho al enemigo más cercano (pared, enemigo, pared...), 4 paredes
  (+1 cada dos niveles) o hasta tocar el núcleo.
- **Conductor y Bedrock pasan al Alquimista** (trabajan con su elemento); los
  textos dicen que necesitan Eléctrico / Piedra.
- **Mapa (después, a pedido):** ya no hay paradas de Forja: se forja sólo en
  el Descanso, que es reparar el núcleo O forjar (2 puntos), así que mejorar
  es una decisión. Los "?" son siempre un encuentro (evento); el item gratis
  sólo si ningún evento aplica. La fila antes del jefe: Tienda, Descanso, ?,
  Descanso. (La "forja paga de la tienda" era código sin uso.)
- **Cooldowns del Mago (equilibrio):** las aceleraciones de recarga (Focus,
  Meditate, Archmage, nodos de la red) se suman con rendimiento decreciente:
  x(1 + 2B/(2 + B)), tope x3 (`cfg::ability::rateCap`). Lo que devuelve
  segundos (golpe del Mago 0.2 s, Insight 0.3 s, Arcane charge 0.2 s, Mana
  spring 15%, Sync, Flux, Master Slinger) pasa por `World::refundAbility` y
  entre dos lanzamientos no puede devolver más del 40% del cooldown
  (`refundCap`). Lucky break y Cascade siguen siendo recargas completas.
- **Modo infinito (dev):** con SB_DEV=1 el menú principal muestra "Endless
  (dev)": 3 pelotas, 999 de oro, núcleo x2, oleadas seguidas sin mapa (1.5 s
  entre una y otra, cada vez más duras); items con F1. No guarda nada: ni la
  run (la run guardada de verdad no se toca) ni el progreso (se restaura al
  salir).
- **Pantalla de derrota:** debajo de cada pelota, sus clases, sus kills (la
  que más mató, en dorado) y lo que llevaba (items en el color de su rareza,
  elementos, habilidades); abajo, reliquias, credos y pactos. Las kills se
  acreditan a la última pelota que dañó al enemigo (`Enemy::lastBy`): golpes,
  balas, habilidades, reacciones, invocaciones y efectos de campo cuentan
  para su pelota dueña (`World::ballKills`).
- **Forja con presupuesto:** cada visita (Rest -> forja, Wandering smith)
  tiene 2 puntos: común / poco común 1; raro / épico 1 pero sólo como el
  primero de la visita; legendario 2 (solo). Si arrancás con un común, ya no
  podés subir raros, épicos ni legendarios en esa visita. Te quedás en la forja mientras alcance. La forja paga de
  la tienda sigue siendo un nivel por pago (`cfg::gold::forgePoints`).
- **Combinar en el TAB:** arrastrar un item o habilidad y soltarlo sobre el
  mismo en otra pelota (o sobre su panel) los combina ahí: los niveles se
  suman (con tope) y el lugar de origen queda libre. Sólo en el TAB (la
  primera versión, "mantener 1.5 s", se sacó).
- **Invocador (reemplazos, mismo lugar en el enum):** Baby dragon (épico, era
  Fortify): un dragoncito al lado escupe un cono de su elemento cada 2 s (Twin
  call: dos). Elder dragon (legendario, era Overlord): un dragón enorme da
  vueltas a la arena, cada 4 s barre una franja x4. Swarm (legendario, era
  Necromancer; en el código `Hive`, no confundir con el jefe Hive): bichitos de su elemento, 3 por segundo, hasta 30, corren y
  muerden (dejan el elemento sin dueño: reaccionan con todo). Sinergias: el
  rugido del grande hace escupir a los chicos (y x1.5), el aliento de un
  dragoncito prende bichos (su próxima mordida explota), la franja del grande
  suelta bichos.
- **Bufón (reemplazos):** Lucky number (épico, era Slot machine): cada 8 s un
  d10 que flota sobre la pelota (7 segmentos): >5 más rápida y fuerte, <5 más
  lenta pero empuja y aturde, 10 = crítico en cada golpe, 1 = se frena 1 s y el
  próximo golpe x3. 777 (legendario, era Joker): rodillos sobre la pelota giran
  1.2 s y paran de izquierda a derecha; triple 7 = frenesí, tréboles = toda la
  suerte de la run x2 un rato, monedas (la más rara) = lluvia de monedas que
  pegan y dan oro; un par = una probadita.
- **Alquimista (legendarios, ahora 3):** Toxic trail (era Philosopher's stone):
  rastro de nubes, una de cada elemento por turno. Primal (era Grand reaction):
  cada 6 s su primer elemento: fuego lanzallamas, hielo nube de escarcha que
  congela, agua lluvia (enemigos -30%, pelotas +20%, empapa), piedra terremoto,
  veneno 3 botellas, eléctrico 6 rayos con nubecita. Black plague (nuevo, al
  final del enum): 25% por golpe, el enemigo se pone negro, enloquece, persigue
  a otros enemigos (60% más rápido), los muerde y contagia; no ataca al núcleo.
- **Cosméticos de clase** (al principio por legendario; después, sólo con los 4
  items de la clase): accesorio chico: Striker vincha roja, Guardián escudito, Mago sombrero,
  Shooter visor, Bufón gorro, Alquimista doctor de la peste, Asesino daga,
  Support aureola (después: vincha blanca con cruz roja, sin tiras), Invocador cuernitos, Slinger sombrero de vaquero
  (`BallSpec::legend`, `drawLegendCosmetics`).
- **Ajustes (2a vuelta, mismo día):**
  - Cosméticos: Striker vincha que cruza toda la pelota, Support vincha blanca
    con cruz roja, Guardián escudo gris oscuro, Mago sombrero viejo marrón con
    la punta doblada, Shooter scouter rojo de un ojo, Asesino daga ancha,
    Invocador cuernos rojos separados, Bufón gorro violeta y amarillo grande,
    Slinger sombrero de vaquero grande, Alquimista sólo máscara de la peste
    (pico largo, ojos de vidrio), negra para que se note.
  - Swarm: un bicho que mata se divide en dos (tope 32 en pantalla).
  - Dragones: antes de escupir cargan (brillo en la garganta) y giran la
    cabeza suave hacia el blanco; el aliento es una llamarada de bocanadas.
  - Spectral sword (raro, en lugar del Dragonling, que repetía al Baby dragon):
    una espada flota junto a la pelota y cada 2 s atraviesa al enemigo más
    cercano cortando todo en el camino, y vuelve girando.
  - Spirit host suelta sus propios wisps (~1 por segundo): no necesita otro item.
  - 777: cada 7 s; lo ganado dura hasta la próxima tirada (la lluvia más);
    triple 30% normal, 60% siendo Bufón, 92% Gran Bufón; frenesí x1.7 velocidad
    x2 daño y da tirones; tréboles x3 la suerte; muchas más monedas. Rodillos
    con luces, temblor y el último frena despacio.
  - Beckon: apretar suelo vacío trae la pelota a tu mano: apuntás y tirás con
    la gomera.
  - Alquimista: 2 elementos como máximo (también ascendido); click en el
    elemento chico en el TAB lo pasa a principal. Archalchemist: reacciones +50%.
  - Toxic trail: cada nube se infla rápido (x2.2) y se achica despacio; más daño.
  - Primal: con 2 elementos se disparan los dos poderes a la vez. Después,
    a pedido: cada 2.5 s (era 5) con ráfagas cortas y de menos daño
    (lanzallamas 1.1 s, escarcha 1.6 s, lluvia 2.5 s, 2 temblores + 3 rocas,
    4 botellas, 7 rayos) para dejar elementos seguido y encadenar reacciones.
  - Primal más intenso (cada 5 s): hielo nube 3 s que congela, agua lluvia 5 s
    con gotas que pegan, piedra 3 sacudidas + 5 rocas, veneno 6 botellas con
    charcos, eléctrico 14 rayos que saltan a 2 más.
  - Black plague: los infectados corren x1.9 y se mueren más rápido.
  - Ion cannon (rework): ya no frena ni deja de disparar; cada bala que pega
    salta como rayo de enemigo en enemigo: 2 saltos (+1 por nivel) +3 por cada
    +100% de cadencia, hasta 12.
  - Time stop: cada 15 habilidades.
- **Dragones (3a vuelta):** el cuerpo mira hacia donde vuela y sólo giran
  cuello y cabeza (hasta ~100°); escupen un chorro sostenido (chico 0.9 s,
  grande 1.6 s) que va mordiendo cada 0.15 s y sigue al blanco con la cabeza.
- **Spectral sword:** una espada por nivel del item (1/2/3), alcance 420 px
  (+40 por nivel), desfasadas entre sí.
- **Bufón caótico (clase):** cada 6 s (−0.7 por item de Bufón, mínimo 2 s)
  saca un truco al azar de otra clase: salto (Warp), misiles, ráfaga de balas,
  lanzamiento a toda velocidad, nova, zap encadenado, cuchillas, rayo del
  cielo; anunciado con un cartel. Gran Bufón: 2 trucos a la vez (3 con 6+
  items). Los trucos son más fuertes con más items.
- **Bufón con cartas (reemplaza la bolsa de trucos):** cada 3.5 s (−0.4 por
  item, mínimo 1.2 s) saca una carta que aparece sobre la pelota (palo y
  número dibujados) y se anuncia ("KING OF HEARTS!"). Corazones: cura el núcleo
  y Tailwind; diamantes: oro y ráfaga de balas; picas: cuchillas (baja) o rayo
  del cielo (alta); tréboles: nova (baja) o zap (alta). J salto, Q misiles,
  K lanzamiento a toda velocidad + nova, As uno de cada palo. El número sube la
  fuerza. Cartas por vez: 1, +1 con 3+ items, +1 Gran Bufón, +1 con 6+ items.
- **Cosméticos:** sólo con los 4 items de la clase (ascendida), ya no por legendario.
- **Bufón, básicos:** Coin flip ahora se ve (en cada golpe salta una moneda
  que cae dorada en cara, gris en cruz). Roulette (común, nuevo, al final del
  enum): cada 5 s gira una ruleta chiquita junto a la pelota; rojo +40% daño,
  negro +40% velocidad, verde (1 en 7) ambos y 5 de oro; dura hasta el
  próximo giro (aro del color ganador en la pelota).
- **Jefes:** Dasher embiste por todo el mapa (rebota en paredes, encadena
  embestidas; cada 3ra va al núcleo). Charger, Hive y Warden tienen segunda
  fase: al bajarlos la primera vez caen dormidos, entra una oleada
  (10 + 3 por acto) y al limpiarla reviven con 60% de vida, enfurecidos.
- **Anotado para después (pedido del usuario):** reliquias nuevas que
  aumenten más cosas; estadísticas por clase (una principal y 2-3
  secundarias, p. ej. Striker daño / tamaño, Asesino crítico / velocidad,
  Mago cooldown / daño elemental, Guardián tamaño, Slinger distancia de
  atrapado, Invocador daño de invocación / cooldown, Alquimista daño
  elemental, Support cooldown, Bufón suerte) y que la reducción de cooldown
  valga también para disparos y reacciones; pasada de dificultad (enemigos
  con patrones distintos, mejor composición, mejores decisiones con Q y F).
- **Tooltips de clase más simples:** el texto se separa en párrafos ("\n" en
  `wrapText`, una línea vacía = medio renglón). La pelota muestra qué hace su
  clase y aparte "2 items: X (done) / 4 items: Mega X (3/4)".

### 2026-10-06: cada clase se siente distinta

- **Rasgo propio por clase** (`cfg::role`, `World::roleCruise` / `roleContact`;
  dos clases se multiplican, la ascendida no cambia el rasgo):
  Guardian mucho más lento (x0.65, antes 0.9); Mage pega la mitad de contacto;
  Shooter 0.7x, Summoner 0.6x, Alchemist 0.85x de contacto; Assassin x1.25 de
  velocidad y x1.8 golpeando por la espalda (el lado del enemigo opuesto al
  núcleo); Jester tira un dado en cada golpe de contacto (x0.5 a x1.7);
  Slinger tirado sale x1.6, pero suelto (1.5 s de gracia) su crucero baja a
  x0.55 en 5 s; tirarlo de nuevo o una oleada nueva lo resetea. Striker y
  Support quedan como estaban.
- **Mini stats por clase** (además de items y rasgo, `App::ballSpec`):
  Striker +8% daño, +10% vel. máx.; Guardian +20% empuje, cura el núcleo al
  rebotar; Support +10% elemento, +5% recarga; Mage +15% recarga, +5%
  elemento; Shooter +10% balas, +5% crit; Assassin +10% crit, +25% daño crit;
  Summoner +15% invocaciones, +5% recarga; Jester +8% crit, +5% golpe doble;
  Slinger agarre más lejos, +15% primer golpe tirado; Alchemist +15%
  elemento, +5% recarga. Se ven en el tooltip de la pelota.
- **Leash (rehecho):** atada al núcleo gira 3 s, de 0.5 a 4 vueltas por
  segundo (sube en 2 s con curva, el último segundo a fondo; ~6 vueltas),
  siguiendo su camino como el Satellite (`World::advanceLeash`): pega a todo lo
  que barre, sin choques con otras pelotas ni paredes (la cuerda cabe en la
  arena). Al final se suelta por la tangente a 0.85x el tope duro de velocidad
  y la mantiene 2.5 s (+0.5 por nivel) rebotando por todos lados; primer golpe
  +50%. Combina con Striker (daño por velocidad) y Assassin. Una sola a la vez:
  un 2do Leash espera a que termine el vuelo; agarrarla corta la cuerda.
- **Magic missile gratis del Mage, una vez por pelota** (`BallLoadout::freeMissile`,
  guardado en la run): antes, si se lo pasabas o combinabas con otra pelota,
  el Mage recibía otro en la próxima sincronización, sin fin.
- **Alchemist ascendido sin elementos gana Poison** (en el slot de tipo, nivel
  1), una vez por pelota (`BallLoadout::freePoison`, `App::grantAlchemistPoison`).
- **Swarm ataca al jefe:** los bichos van al jefe si está más cerca que el
  enemigo más cercano (o si está solo) y lo muerden como cualquier invocación
  (`cfg::summoner::bossFrac`); los encendidos por un dragón explotan en él.
- **Dragones atacan al jefe:** el bebé y el anciano eligen al jefe como blanco
  si está más cerca que el enemigo más cercano (o si está solo), giran la
  cabeza hacia él y lo queman (bebé: su cono; anciano: su franja) a
  `cfg::summoner::bossFrac`. Nada de daño si el jefe está protegido.
- **Jefes sin repetir:** los actos 1-4 ya salían de un orden mezclado sin
  repetidos (el 5 es siempre el Orbital); al cargar una run, un orden
  inválido vuelve al fijo.
- **Música en pools (2026-10-06):** cada acto tiene 3 temas que se turnan al
  azar (`Audio::setPool`): cada uno suena entero y, 6 s antes de terminar,
  otro del pool (nunca el mismo) entra desde el principio con un fundido
  cruzado de ~4 s. Acto 1: map, fight2, fight3; 2: fight2, fight4, fight3; 3:
  fight3, fight4, fight5; 4: fight4, fight5, fight2; 5: fight5, fight4,
  fight3. Menú y web se turnan con "It's not Undertale 2" (calm.mp3, CC0).
  Nuevo: "Soul Ripper" de Alexandr Zhelanov (fight5.ogg, CC-BY 4.0, en
  CREDITS). El jefe sigue con sus dos movimientos.
- **Satellite -> Moons (legendario del Guardian, mismo lugar en la web y en
  el guardado):** la pelota se mueve normal y sus 3 lunas (4 al nivel máximo)
  orbitan el NÚCLEO en elipses inclinadas que giran y respiran, siguiendo su
  lugar con un resorte blando (un golpe o un rebote las saca y vuelven). Su
  tamaño es el 45% del de la pelota: un Guardian más grande (Giant, Big ball),
  lunas más grandes. Cada luna empuja y aturde lo que toca (45% del golpe de la
  pelota +12% por nivel, con su elemento; al jefe la mitad) y rompe los
  disparos que van al núcleo. Cuando la pelota rebota en el núcleo, las lunas
  salen en anillo y vuelven. Una segunda pelota con Moons tiene su anillo más
  afuera; los gemelos de Gemini traen las suyas (`World::updateMoons`).
- **Leash:** cooldown 11 -> 18 s.
- **Elder dragon:** echa fuego más tiempo del que descansa: 4.5 s de chorro
  continuo (golpes cada 0.08 s) y 2.5 s de pausa (-0.25 por nivel); ~1.6x el
  daño por segundo de antes.
- **Tu blanco manda (click):** van primero al enemigo que marcaste las
  invocaciones (también con Guard dog), los dragones y el Swarm (también al
  jefe marcado), el Beam, el primer salto del Arc, Meteor, Smite, Bounty,
  Hook, Warp, Portal, Swap (si el marcado está más cerca del núcleo que la
  pelota) y el lanzallamas del Primal. Sin blanco, eligen como siempre.
- **Click vs Q:** una pelota tirada se puede volver a agarrar a los 0.6 s
  (antes 1 s), y la Q (Volley) recarga en 2.5 s (antes 1.25): clickear bien
  rinde más que spamear la Q.

### 2026-10-06: jefes en tres fases, disparos al núcleo y Caster

Pedido (usuario): que las peleas con jefes duren más, que cada uno se sienta
distinto, que se vea a las pelotas ganándole (solas o tiradas) y que un
equipo flojo o mal apuntado pierda; cosas que disparen al núcleo y haya que
interceptar; un "mago" que drene el núcleo de lejos.

- **Tres fases** por jefe, cortadas en 66% y 33% de vida (`cfg::boss::stageAt`,
  muescas en la barra de arriba y en la del jefe). Al cruzar una el daño se
  corta ahí, el jefe **vuelve a su punto de entrada** (2.2 s, sin recibir
  daño; el Orbital vuelve a abrir su espiral) y arranca su mecánica, con un
  cartel corto. La última fase es la furia (ENRAGED, cae boss2.ogg). Reemplaza
  al "NOT YET" (dormido + oleada).
  - **Charger:** cada golpe limpio lo empuja atrás (tira y afloje). Fase 2/3:
    planta 3/4 **pilares**; mientras quede uno no recibe daño y sigue
    caminando; los pilares le disparan al núcleo.
  - **Hive:** fase 2/3: se queda atrás y pone 3/4 **nidos** que largan runners
    hasta romperlos; no recibe daño mientras haya nidos. Furioso, abanicos de 6.
  - **Warden:** fase 2+: **punto débil** en la espalda (brilla, lado opuesto al
    escudo): ahí pega x3, lo frena 1.4 s y lo empuja; en el resto del cuerpo
    x0.4. Abre abanicos de orbes al núcleo (3, furioso 5). Fase 3: el escudo
    cambia de sentido de giro cada tanto.
  - **Dasher:** fase 2+: **blindado** (x0.2 de daño) salvo **aturdido**: sus
    embestidas ya no rebotan, corren hasta una pared y se estrella (2.4 s,
    furioso 1.8 s, x1.6 de daño), soltando 3 orbes al núcleo. Las embestidas
    nunca cruzan el núcleo (antes podía atravesarlo y perder al toque).
  - **Orbital:** fase 2/3: 4 **anclas** que disparan al núcleo y reponen el
    anillo el doble de rápido; no recibe daño mientras haya anclas. Fase 3:
    cada 5 s un escudo del anillo se suelta y va al núcleo.
- **Partes** (pilares / nidos / anclas): enemigos quietos (`Enemy::part`), 12
  grunts de vida; los empujes se deshacen. El auto-apuntado no va al jefe
  mientras está protegido, así las pelotas solas van a romper las partes.
- **Disparos** (`Shot`): orbes lentos (150 px/s) al núcleo; una pelota (o
  copia, o la que tenés agarrada) que los toca los rompe; si llegan, 5 de
  daño al núcleo ("Aegis" lo absorbe). En la derrota: "a boss shot got through".
- **Caster** (enemigo nuevo, desde la oleada 7): se para a distancia del
  núcleo, carga 1.5 s y lo drena con un rayo (0.8/s) hasta que lo matás;
  congelado o aturdido no drena.
- **Duración:** jefes más lentos (Charger 22 px/s, Hive 14, Warden 20, Dasher
  acecha a 16, espiral del Orbital 4 px/s) y con más vida (120 / 100 / 110 /
  64 / 130 grunts). Sim con bot (scratchpad, mismo bot en la versión vieja):
  las peleas duran ~3x (2-4x según el jefe); un equipo flojo pierde con el
  jefe final; ganando, el núcleo suele terminar entre 20% y 70%.

### 2026-10-06: el cielo de la web crece con tus clases

La web arranca en negro. El fondo es una galaxia 2x la pantalla que se mueve
con la cámara (la mitad del paneo), así que moviéndote la explorás; "Hide web
(H)" (arriba a la derecha) saca la red para mirar sólo el cielo (arrastrar
para recorrerlo). El cartel de cada nodo aparece sólo con el cursor encima.
Cada clase desbloqueada suma su motivo, repartido por toda la galaxia salvo
los grandes, que quedan a la vista (`ui/WebSky.cpp`): Striker cometas
rápidos, Guardian un planeta chico con escudo hexagonal (derecha), Support
constelaciones, Mage un círculo mágico girando (izquierda), Shooter trazos de
balas cruzando todo el mapa a distintas alturas, Assassin ojos rojos que se
abren de a ratos y te miran, Summoner luces que suben y un dragón que pasa
volando cada tanto, Jester cartas por todos lados, Slinger arcos de tiro,
Alchemist nebulosas de colores. Por cantidad de clases: 2 pelotas que
flotan, 3 nebulosas, 4 galaxias espirales, 5 planetas, 6 estrellas gemelas,
7 un púlsar, 8 la vía láctea, 9 supernovas, 10 planetas que explotan y un
agujero negro. Al desbloquear una clase, un aro fino de su color se abre
desde el centro. Las capturas 41 / 42 muestran cada motivo y las etapas.

### 2026-10-06: Shrines, ascender (Guardian -> Atlas) y Expandir

- Se mantiene: 2 items de una clase = la clase; 4 = su forma completa de hoy.
  Las pelotas arrancan con 4 slots y como mucho 2 clases.
- **Shrine** (parada nueva, una por acto desde el acto 2 - desde el acto 4 a partir del 2026-10-07 -, en lugar de un "?" o
  un Descanso de las filas del medio; hexágono dorado "A"): elegís UNA de dos,
  para UNA pelota que no haya usado su Shrine:
  - **Ascender** (con los 4 items de Guardian; máx. 3 pelotas ascendidas por
    run): se borran sus items de Guardian y elige 1 de 3 planetas (todos
    épicos o legendarios) que va directo a ella. Pasa a ser **Atlas** (clase
    `Astronomer` por dentro): con 1 planeta ya tiene la clase. Desde ahí, en
    toda la run, dejan de salir items de Guardian y salen planetas
    (`RunState::poolOut / poolIn`). La pelota ascendida suma además 2
    planetas al azar más débiles por combate.
  - **Expandir**: +2 slots de item (6), en dos columnas en el panel: una
    clase completa + otra, o 3 clases de 2.
- **Atlas (Astronomer)**: contacto 0.75x, planetas +10%, +5 de vida del
  núcleo. Items (`sim/WorldPlanets.cpp`, `cfg::planet`): épicos Mercurio,
  Venus, Tierra, Marte, Júpiter, Saturno, Neptuno; legendarios Cinturón de
  asteroides (lugar del viejo Satellite/Moons; detrás de su nodo de la web) y
  Noche estrellada. La Luna sólo sale como planeta extra. Cada planeta orbita
  el núcleo a su distancia (elipses que entran en la arena), empuja y aturde
  lo que toca, rompe disparos y, cuando su pelota rebota en el núcleo
  (con la clase): Mercurio x2 de velocidad 3 s, Venus nube ácida, Tierra
  ráfaga de cohetes (y un cohete cada 2.5 s), Marte lanza Fobos y Deimos,
  Júpiter la Gran Mancha Roja (y deja gas que frena y daña), Saturno lanza
  sus anillos en un aro que corta (vuelven en 4 s), Neptuno anillo de
  escarcha, el Cinturón dispara la mitad de sus rocas, Noche estrellada hace
  caer una estrella por planeta y suma 3 planetas al azar por combate. Más
  grande la pelota, más grandes los planetas y más cosas tiran.
- Guardado: los datos nuevos van al final de cada línea (`run.ball`,
  `run.ascend`), las runs viejas siguen cargando.

### 2026-10-06: Atlas más fuerte, Hermes y Odín

- **Atlas:** órbitas x1.7 más rápidas; la pelota va derecha al núcleo cada 2
  rebotes (pared o enemigo: `cfg::planet::homeEvery`), así oscila y los
  planetas se activan seguido. Cada planeta con lo suyo, nada reciclado:
  Tierra cohetes con llama que explotan en área; Marte lanza Fobos y Deimos
  (rocas que vuelven a su órbita); Venus una tormenta ácida pegada al planeta
  (veneno + los enemigos reciben más daño); Júpiter la Gran Mancha Roja, una
  espiral que va hacia el grupo más denso y atrae; Saturno sus anillos; Neptuno
  una onda de hielo que congela; Mercurio acelera y deja una estela solar que
  quema; el Cinturón tira meteoritos; Noche estrellada estrellas fugaces.
- **Ascensión general:** tabla clase -> dios (`godOf`): Guardian -> Atlas,
  Striker -> Hermes, Mage -> Odín. Pocos items, todos épicos/legendarios.
- **Hermes** (clase: x1.2 velocidad, x1.6 velocidad máxima, el golpe crece con
  la velocidad): Talaria (ép.: +12% de velocidad por rebote en pared hasta el
  núcleo, el tiro casi no se frena), Caduceo (ép.: rápido, cada golpe también
  pega a los 2 más cercanos con dos serpientes de luz), Mensajero (ép.: cada
  4 s va y vuelve al instante al enemigo más lejano del núcleo), Petaso (leg.:
  rápido se vuelve dorado, atraviesa enemigos y deja estampidos sónicos),
  Quicksilver (leg.: rápido, todos los enemigos 30% más lentos).
- **Odín** (clase: 3 slots de habilidad, recarga +30%, contacto 0.6x): Hugin
  (ép.: un cuervo re-lanza su última habilidad al 50% cada 4 s desde donde
  está), Munin (ép.: cada hechizo también lanza otro que ya usó en la pelea,
  al 60%), Runas (ép.: cada hechizo deja una runa trampa), Gungnir (leg.: cada
  6 s una lanza atraviesa la fila hasta el más fuerte y vuelve), Draupnir
  (leg.: cada 3er hechizo se lanza dos veces y sus habilidades +1 nivel). Los
  cuervos no re-lanzan las que mueven la pelota (Dash, Warp, Swap, Leash).
- **Odín con cooldown real:** sus habilidades ya no se lanzan una por una:
  SÓLO en descargas (todo junto) cada 8 s / su velocidad de recarga
  (decreciente). Hugin: cada 3ª descarga sale 3 veces. Matar adelanta la
  próxima descarga 4%. Gungnir cada 10 s / su recarga.
- **Hermes casi nunca toca el núcleo:** lo esquiva con fuerza (giro 14 rad/s y,
  si igual va a chocar, se desvía del todo) hasta llevar 30 rebotes; ahí va
  derecho al núcleo a entregar el mensaje (cargado).
- **Tiamat siempre haciendo algo:** la montura (dibujada debajo de la pelota,
  que va montada) rasguña lo cercano cada 0.35 s, deja un rastro de brasas,
  tira bolas de fuego cada 1.8 s, y cada 7 s: Rugido, Picada (se lanza sobre
  el grupo más denso y choca), Infierno (un anillo de fuego la acompaña 4 s)
  o Meteoros (3 caen sobre grupos). El wyrm deja escarcha en su recorrido
  (frena y daña), el draco tiene un campo estático que electrocuta lo cercano
  cada 0.5 s, la sombra del dragón dorado quema lo que tapa y su choque deja
  un charco de oro fundido. El dragón del cielo de la web también usa el
  dibujo del Elder (otra forma en cada pasada).
- **Infinito (dev):** arranca en la arena grande (la de los actos 2+);
  botones "+20 enemies", "+20 tough (x8 HP)" y "Arena: act 1 / wide" (cambia
  la arena desde esa oleada).
- **Hide web arreglado** (la vista de la web se guardaba cada cuadro y lo
  apagaba).
- **Balance de dioses por simulación (`SB_BALANCE=<oleadas> ./space_breakers`,
  `SB_BALANCE_ONLY=<nombre>` para uno):** cada clase con sus mejores items
  contra su dios con los suyos, una sola pelota, las mismas habilidades (Nova;
  Mage/Odín además Arc y Meteor), 30 enemigos constantes 12 s por oleada, la
  Q cada vez que está lista, núcleo invulnerable; cuenta sólo las muertes
  acreditadas a la pelota. Meta del usuario: el dios mata 1.5x-2x (3x como
  mucho). Resultado (60 oleadas, promedio de 3): Hermes 2.05x, Atlas 2.2x,
  Odín 1.65x, Thanatos 1.73x, Tiamat ~1.55x. Ajustes que lo dieron: Hermes
  crucero x1.5 / tope x1.7, golpe x0.3 y sin bonus por velocidad (su vuelo
  ya es su ventaja); Odín potencia x1.0, Munin 0.5x, Gungnir 6x; Atlas toques
  algo más fuertes pero estrellas / meteoros / muralla / Mancha y crecimiento
  más débiles; Thanatos puñalada x1.7, salto cada 0.9 s, +8% por alma; Tiamat
  bola de fuego cada 1 s y más grande, garras, lagartos (hasta 24) y dragones
  más fuertes. Ojo: sin la Q simulada el Striker base queda muy abajo (vive
  de los tiros).
- **Dragones de Tiamat recorren el mapa y tiran por la boca:** Wyrm de hielo
  (serpiente: escupe esquirlas de hielo, aliento, deja escarcha), Draco de
  tormenta (siempre volando: rayo en cadena desde la boca, campo estático,
  esferas de rayo lentas que electrocutan a su paso, nube de tormenta cada
  7 s sobre el grupo más denso), Oro antiguo de TRES CABEZAS (izquierda
  lanzallamas continuo, centro oro fundido que deja charcos, derecha rugido que
  atrae a los enemigos), Devorador (nuevo, ép.: se traga un enemigo entero -
  no jefes, a esos los muerde - y lo mastica 4 s antes del próximo),
  Dragoncito (nuevo, ép.: da vueltas alrededor del grupo más cercano dejando
  una estela; cuando la cruza, todo lo que quedó adentro del lazo explota).
- **Hermes:** entrega a los 100 rebotes (2026-10-07: antes cualquier roce con
  el núcleo la disparaba antes de tiempo; ahora tocar el núcleo antes de los
  100 no entrega ni reinicia nada). El halo dorado se llena con los rebotes:
  lleno = va al núcleo. Sólo gana velocidad (nunca baja hasta que entrega).
- **Thanatos, la daga (2026-10-07):** rebota 3 veces hacia enemigos lejanos
  (hasta 800 px), eligiendo el camino que atraviesa más enemigos (después el
  más lejano); todo lo que cruza en el camino recibe un corte liviano (0.25x).
  Sim ~2x del Assassin.
- **Draupnir (visual, 2026-10-07):** un anillo de oro grueso, inclinado,
  girando alrededor de Odín con un brillo que lo recorre; en cada andanada
  suelta 8 anillos iguales que salen despedidos (el anillo que se multiplica).
- **Gungnir (2026-10-07):** tira una lanza más por cada hechizo lanzado desde
  la anterior (hasta +10), una tras otra (0.07 s), cada una al siguiente
  enemigo más fuerte (el jefe primero); las extra pegan 0.3x de la principal.
  Visual nueva: estela de luz dorada que se afina, asta oscura con 3 runas
  celestes, guarda y hoja de oro en forma de hoja con filo blanco. Sim Odín
  ~2.2–2.4x del Mage (la lanza extra suma ~10%).
- **REGLA (el usuario, 2026-10-06): ningún item de clase normal puede ser
  mejor que uno de un dios.** Al balancear o agregar items, un item de dios
  siempre rinde más que cualquiera de las clases base.
- **Tiamat, montura nueva:** lenta y pesada (x0.55 de velocidad); su dragón
  lanza una bola de fuego en arco al grupo más denso cada 1.8 s (explota en
  área) y cada 7 s hace un movimiento propio con su nombre en pantalla:
  Rugido (aturde lo cercano), Tormenta de fuego (6 bolas sobre distintos
  enemigos), Ráfaga de alas (empuja lejos todo alrededor), Coletazo (un
  barrido circular fuerte). Sus dragones usan el dibujo del Elder dragon en
  otras formas (`drawDragonForm`): Wyrm de hielo = dragón serpiente (sin
  alas, cuerpo largo y bigotes), Draco de tormenta = wyverno (alas-brazo con
  garras, dos patas, cola con púa), Oro antiguo = cuatro alas, gigante; la
  montura, el dragón clásico.
- **Thanatos:** nunca dos saltos a menos de 0.5 s (salvo Última hora).
- **Nerf a Thanatos y Odín (2026-10-06):** Thanatos: su salto tiene 1.2 s de
  cooldown (vuela libre mientras; matar lo baja a la mitad). Odín: lanza todo
  cada 6 s (no 2), Hugin cada 10 s, potencia x1.3, matar resta 8%. **Toda
  reducción de cooldown va a una sola suma con rendimiento decreciente**
  (`World::mageCastRate`: x(1 + 2B/(2+B)), tope x3): la clase Odín (+0.3),
  Valknut (+0.4), Focus, la web... el primero rinde mucho, cada uno más rinde
  menos. El lanzamiento total, Hugin y Gungnir vienen a esa velocidad
  (Gungnir escala con su recarga).
- **Colores de los dioses = los de su clase base** (sin colores nuevos): Atlas
  el del Guardian, Hermes el del Striker, Odín el del Mage, Thanatos el del
  Assassin, Tiamat el del Summoner. Para distinguirlos, los items de las
  clases base se ven un poco apagados y los de los dioses más brillantes, con
  borde encendido y un reflejo que los cruza (`tagColor`, `ui/Widgets.cpp`).
- **Que se note que son dioses:** toda pelota de un dios lleva un aura y un
  halo flotante de su color, más su cosmético grande: Atlas una esfera
  armilar (dos anillos inclinados con estrellas), Hermes alas doradas que
  baten, Odín un anillo de runas, Thanatos capucha de sombra y guadaña, Tiamat
  montada en su dragón (cabeza adelante, alas, cola). Se ve en la arena y en
  el TAB (`drawGodLook`).
- **Pasivas al matar:** Odín: cada muerte resta 15% a todos sus cooldowns y a
  su próximo lanzamiento total. Atlas: cada muerte +3% de tamaño (y sus
  planetas) en la pelea, hasta +60%. Thanatos: un alma, +6% de daño en la
  pelea (hasta +90%) y salta al siguiente al instante. Tiamat: nace un
  lagarto. Hermes: su vuelo (impulso y mensaje). Hermes ahora es rapidísimo
  de base (x2 crucero, x2.2 tope) y recupera la velocidad al instante.
- **Thanatos (dios del Assassin):** salta detrás de un enemigo y lo apuñala
  cada 0.18 s (x1.5) hasta matarlo; recién ahí salta al siguiente (el
  marcado primero). Items: Paso de sombra (ép.: cada salto corta lo que hay en
  el camino), Marca de muerte (ép.: lo que apuñala con menos de 25% muere; no
  jefes), Eclipse (ép.: cada 4º salto una nova de sombra), Guadaña (leg.:
  cada salto golpea también a los 2 más cercanos), Última hora (leg.: cada
  15 s salta por hasta 12 enemigos en un instante).
- **Tiamat (diosa del Summoner):** monta un dragón que escupe fuego hacia
  adelante cada 2.5 s; cada muerte hace nacer un lagarto (hasta 16). Items:
  Wyrm de hielo (ép.: un dragón blanco enroscado alrededor del núcleo que
  escupe hielo y congela), Draco de tormenta (ép.: un dragón azul posado en el
  campo que cambia de lugar y lanza rayos en cadena a 4), Horda de lagartos
  (ép.: cada 5 s salen 6 lagartos), Oro antiguo (leg.: un dragón dorado
  gigante que vuela alto y cada 7 s se lanza en picada sobre el más fuerte:
  4x su golpe en área), Madre de la cría (leg.: los lagartos muerden el doble
  y cada muerte suya hace nacer 2 más, hasta 30).
- **Odín rehecho (que las habilidades se noten):** la clase lanza TODAS sus
  habilidades a la vez cada 2 s (un círculo de runas se enciende bajo la
  pelota), con potencia x1.5 (daño y radio) y recarga +30%. Hugin (ép.): cada
  6 s lanza todo 3 veces seguidas. Munin (ép.): cada hechizo baja una
  explosión arcana sobre el grupo más denso (1.2x su golpe). Valknut (ép.,
  lugar de Runas): +40% de recarga y el lanzamiento total más seguido. Gungnir
  (leg.): cada 5 s la lanza pega 10x su golpe al más fuerte, ida y vuelta.
  Draupnir (leg.): habilidades +1 nivel y x1.5 de potencia y radio.
- **Hermes vuela a su manera (pasiva de la clase):** esquiva el núcleo cuando
  va hacia él (gira hasta 4 rad/s de cerca; a veces igual le pega) y se curva
  hacia el enemigo marcado o el más cercano. Cada rebote (pared o enemigo) sin
  tocar el núcleo suma +5% de velocidad (hasta +120%). Cada pared y cada
  segundo volando rápido cargan un mensaje (halo dorado que crece); al tocar
  el núcleo lo entrega: "MESSAGE DELIVERED", una onda dorada y un golpe a
  todos los enemigos de la arena (0.5x su golpe por punto de carga, tope 12) y
  vuelve a cero (`cfg::hermes`, `World::godsCoreBounce`).
- **Saturno y Neptuno, rehechos:** al rebotar en el núcleo, los anillos de
  Saturno se instalan alrededor del NÚCLEO 5 s como una muralla doble que gira:
  corta y tira hacia afuera lo que intenta cruzar y frena disparos; después
  vuelven al planeta. Neptuno lanza a Tritón, su luna, que orbita el núcleo al
  revés 6 s congelando todo lo que toca (con estela de escarcha).
- **Items de dioses, sólo para su dios:** un item de un dios sólo entra en la
  pelota que ascendió a ese dios (no se pasa a otra pelota ni a otro dios), y
  una pelota ascendida sólo acepta items de su dios (elementos y habilidades
  sí). Lo que ya tenía al ascender se queda. En dev, dar un item de dios
  asciende la pelota en el acto.
- **Ascender conserva el tamaño:** lo que la pelota perdería al borrarse los
  items de su clase (y la clase Guardian) se le devuelve
  (`BallLoadout::keptSize`), así un Guardian grande da un Atlas grande y
  planetas grandes.
- Dev (F1): pestaña "Gods" con los items de los dioses y "Shrine (ascend)".
- Pendiente: Thanatos (Assassin, se teletransporta saltando entre enemigos),
  Nergal, Tiamat, Artemisa, Loki, Apolo, Ares.

### Anotado para después (el usuario lo pidió así)

- **El juego por etapas (2026-10-06, el usuario lo quiere más adelante):** se
  desbloquea de a partes: primero las habilidades, después Q / E / F, después
  aparecen los pactos, después las ascensiones. Sin desbloquear las etapas
  casi no se puede ganar (salvo un equipo muy bueno). Los items épicos y
  legendarios también se desbloquean en la web, y las ascensiones son otro
  anillo de la web más afuera, después de los legendarios y de vencer a cierto
  jefe.
- **Ascensiones como dioses (2026-10-06):** hechos Atlas (Guardian), Hermes
  (Striker), Odín (Mage), Thanatos (Assassin), Tiamat (Summoner). Faltan,
  con lo que pidió el usuario: Nergal (Alchemist, la plaga), Artemisa
  (Slinger), **Loki** (Jester: crea clones cada tanto que tiran una ruleta
  especial - un rayo, juntar todas las pelotas en un punto, cosas así),
  **Apolo** (Support: aleja a los enemigos del núcleo, se clona un montón y
  cada tanto da a los aliados efectos mejorados al azar, todo muy visible),
  Ares (Shooter). Cada dios con su pasiva al matar y su cosmético.
- **Los dioses que faltan, diseño del usuario (2026-10-07):**
  - **Loki (Jester):** se clona; cada clon juega algo de azar (moneda,
    ruleta, cartas, dados...) y lo que sale pasa. Sus items suman juegos, por
    estilos: juegos de casino, luego cartas, luego pachinko.
  - **Nergal (Alchemist):** plagas y pestes: lluvia de sangre, la plaga como
    ratas que corren por el campo, cosas así.
  - **Apolo (Support):** 4 clones, cada uno da un buff fuerte y distinto a una
    pelota (fuerza, mucha velocidad, duplicar la pelota con todo y sus
    habilidades, ...). Los buffs van rotando entre las pelotas, y una pelota
    nunca tiene dos de estos buffs a la vez.
  - **Ares (Shooter):** un descontrol de balas, misiles y artillería pesada;
    cada item le suma un arma más.
  - **Artemisa (Slinger):** sin idea del usuario todavía (propuesta abajo).
- **Loki (dios del Jester, hecho 2026-10-07, `sim/WorldLoki.cpp`):** cada 2 s
  se desprende un clon (hasta 5, 7 s cada uno; cada muerte 25% de otro al
  instante). Los clones vuelan rondando a los enemigos (se los atraviesa, son
  ilusión) y cada ~1 s juegan algo de azar que se ve girando sobre su cabeza:
  moneda siempre (cara: un rayo del cielo 6x en área; cruz: un golpe 2.5x).
  Items: Dados cargados (ép.: tantos cuchillos como saca, 2 a 12), Ruleta
  (ép.: rojo nova de fuego, negro atrae y aturde, verde 1 en 10 JACKPOT 12x en
  área + oro), Mazo marcado (ép.: picas un tajo en línea, tréboles un golpe
  que aturde, diamantes esquirlas a 3, corazones curan el núcleo; figura x2,
  as x4), Pachinko (leg.: 14 bolitas de acero caen desde arriba y rebotan
  entre los enemigos), Mesa alta (leg.: +2 clones, uno cada 2 s, todo paga
  50% más, el verde el doble de seguido). Contacto 1x.
  Cambio del usuario (mismo día): cada clon juega UN solo juego UNA vez y
  desaparece (un dado, una ruleta, una carta...); como mucho 3 clones (4 con
  Mesa alta), uno cada 0.45 s; el pachinko cae por todo el ancho de la
  arena. Sim: ~1.5x del Jester.
- **Nergal (dios del Alchemist, hecho 2026-10-07, `sim/WorldNergal.cpp`):**
  su elemento es propio y único, la **Plaga negra** (`Element::Plague`, casi
  negro; nunca sale al azar ni se elige): su pelota sólo lleva ésa, y
  reacciona con cada elemento a su manera: fuego PYRE (quema y contagia),
  hielo COLD DEATH (congela y la podredumbre sale de golpe), veneno
  PESTILENCE (la podredumbre se duplica y se esparce), eléctrico FEVER (rayo
  negro por los infectados), agua MIASMA (una nube negra que queda), piedra
  MASS GRAVE (la tierra los traga un momento y salen ratas). Se ve en su type
  slot (hexágono negro, borde violeta) y no acepta otro elemento: el
  potencial sale cuando las OTRAS pelotas traen elementos. Cada golpe
  infecta (podredumbre = veneno, 4 s). Por defecto deja una estela de
  enjambres (uno cada 0.5 s) que persiguen a lo cercano, mastican e infectan.
  Cada infectado que muere suelta una rata (hasta 20). Contacto 0.85x.
  Items: Lluvia de sangre (ép.: cada 6 s una nube roja GRANDE llueve 4 s sobre
  el grupo más denso: daño y los marca, +35% de todo), Marea de ratas (ép.:
  cada 5 s salen 8 ratas, muerden 50% más), Enjambre de langostas (ép.: sus
  enjambres se vuelven langostas: el doble, más grandes, más fuertes, duran
  más), Peste negra (leg.: el infectado que muere estalla 0.6x e infecta
  alrededor; ya NO encadena estallidos - el usuario lo vio muy fuerte),
  Caballo pálido (leg., reemplaza a Cosecha: cada 10 s un jinete pálido
  cruza la arena al galope por el grupo más denso: 3x y les deja la plaga
  negra - que reacciona con lo que tengan). Sim: ~1.9x del Alchemist (sin
  reacciones: el sim es una sola pelota). El sim ahora da a cada build Fuego
  (y Veneno de segundo para el Alchemist).
- **Apolo (dios del Support, hecho 2026-10-07, `sim/WorldApolo.cpp`):** 4
  musas (5 con Carro del sol), cada una de su color (Fuerza roja, Velocidad
  celeste, Gemelo violeta, Gigante verde, Sol naranja; anillo, mira y flechas
  del mismo color), cada una al lado de una pelota distinta
  (nunca dos en la misma) dándole un don: Fuerza (x2 daño), Velocidad (x1.8),
  Gemelo (una copia de la pelota con sus items y habilidades - que lanza -
  mientras ella esté), Gigante (x1.5 tamaño, +50% daño). Cada musa marca un
  punto débil (la marca del Support, +35% de todo) permanente: lo mantiene
  hasta que muere (no salta según la distancia de la pelota) y recién ahí
  elige el enemigo libre más cercano a ella; varios marcados a la vez, uno por
  musa. Cada 5 s rotan a
  la pelota siguiente; con menos pelotas que musas los dones igual van
  pasando (las que sobran esperan en el núcleo). Cada muerte las retiene 0.5 s.
  Contacto 0.9x. Items: Lira (ép.: cada rotación cura el núcleo; la pelota que
  pierde un don se queda la mitad 3 s), Corona de laurel (ép.: dones +40%),
  Arco de plata (ép.: cada musa tira una flecha de sol a su punto débil cada
  0.8 s, 2x), Carro del sol (leg.: una 5ª musa: su pelota es un pequeño sol
  que quema y prende fuego alrededor), Oráculo (leg.: rotan el doble de
  rápido y el golpe al llegar es crítico x3). La musa "Luz" de la propuesta
  la cambió el usuario: quedó Gigante. Sim (una sola pelota, lo subestima):
  ~1.5x del Support.
- **Ares (dios del Shooter, 2026-10-07, sim/WorldAres.cpp):** el usuario
  pidió "un descontrol de balas, misiles y artillería, cada item un arma más",
  y que las armas se sientan de un dios (mitología, no un tanque). Base: el
  Arsenal de la guerra, 6 cañones fantasma de bronce que giran alrededor y
  disparan sin parar (una bala cada 0.1 s, 45% del golpe, con el arma del
  Shooter: sus items comunes le sirven), apuntando a los 3 más cercanos; Sed
  de sangre: cada kill +8% de cadencia a TODAS sus armas (hasta x2), se
  sostiene 2 s y se apaga; con mucha arde roja y los cañones escupen fuego.
  Contacto 40%. Items (cada uno un arma): Phobos y Deimos (ép.: sus hijos
  Miedo y Terror, las lunas de Marte, una roja y una negra, orbitan y tiran
  misiles teledirigidos 1.5x con explosión chica; los alcanzados quedan
  aterrados un instante), Dientes del dragón (ép.: cada 6 s siembra 3
  dientes en fila y brotan hoplitas de bronce, escudo y lanza, que disparan
  ráfagas de 3 balas por 8 s; máx 9), Carro de guerra (ép.: cada 7 s un carro
  con caballos que escupen fuego cruza la arena por el grupo más denso, 2x y
  Fuego, dejando un surco en llamas), Ira del Olimpo (leg.: cada 4 s un
  círculo rojo se cierra sobre el grupo más denso y cae una estrella roja,
  5x en área grande y un cráter ardiendo), Guerra de Troya (leg.: cada 10 s
  una lluvia de 14 lanzas de bronce cae de punta a punta en el carril, fila o
  columna, con más enemigos, 2.5x cada una). Visual: penacho espartano rojo
  sobre la pelota, marca de espadas cruzadas. Sim ~1.66x del Shooter
  (Deadeye: Minigun, Ion cannon, Lead storm, Shrapnel).
- **Artemisa (diosa del Slinger, 2026-10-07, sim/WorldArtemisa.cpp):** la
  cazadora, gemela de Apolo (luna y caza). Regla del usuario: todo tiene que
  salir del lanzamiento. Base: tiro x1.6; cada lanzamiento dispara una flecha
  de plata por toda la línea del tiro (0.8x a todo lo que cruza) y el primer
  enemigo que toca queda como **presa** (marca del Support 6 s, luna creciente
  encima); el primer golpe de la pelota sobre su presa x2. **Si no la tirás,
  se tira sola cada 4 s** hacia los enemigos (o tu objetivo), como si la
  tiraras con el mouse, con todos los beneficios del lanzamiento (rinde más
  si la tira el jugador, pero se puede jugar sin tirarla); el lanzamiento
  quedó en `World::launchThrow`, compartido con `releaseHeld`. Contacto 0.8.
  Items: Sabuesos de plata (ép.: cada tiro suelta 2 perros espectrales que
  corren a la presa mordiendo todo en el camino), Luna creciente (ép.: cada
  tiro salen 2 pelotas más, copias de 3 s, por la misma trayectoria, un
  instante atrás - pedido del usuario), Lluvia de flechas (ép.: al agarrarla,
  o cuando se tira sola, llueven flechas en cada lugar donde pegó el tiro
  anterior, hasta 12), Ciervo dorado (leg.: un tiro con la honda al máximo -
  y los suyos propios - invoca un ciervo de cuernos de oro que embiste por la
  misma línea hasta la pared, 3x, y marca como presa todo lo que toca), Noche
  de caza (leg.: cada 15 s cae la noche 5 s: la pantalla se oscurece un poco,
  resaltando a Artemisa; todos los enemigos son presa; y al tirarla sale a
  toda velocidad atravesando a TODOS los enemigos en pantalla, uno tras
  otro, 1.5x a cada uno). Visual: arco de plata detrás y lunita creciente
  encima. Sim: ~4900 kills (el Slinger base no se tira en la sim, así que la
  comparación con él no sirve; queda en el medio de los dioses).
- **2026-10-07, varios (pedidos del usuario):**
  - **Shrines (altares de ascensión) recién desde el acto 4**
    (`cfg::shrine::fromAct`, antes 2).
  - **Cámara más lejos:** la arena ancha (jefes y actos 2-5) pasa de
    1.95x/1.45x a 2.35x/1.75x (`cfg::boss::arenaScaleX/Y`).
  - **Los dragones de Tiamat le pegan a los jefes:** vuelan hacia el jefe;
    el Frost wyrm lo enrosca y le sopla hielo, el Storm drake le tira
    descargas, cadena y nubes de tormenta encima si no hay grupos, las
    cabezas de Ancient gold le escupen fuego y oro, el Ravager lo embiste si
    no hay grupo, el Wyrmling lo muerde y las lagartijas lo rodean y muerden.
    Al jefe le hacen la mitad, como todo lo que no es una pelota.
  - **Tiamat (montura):** lagartijas máx 18 (24 con Brood mother; antes
    24/36); sus movimientos propios (rugido, dive, inferno, meteoros) cada
    3.5 s (2.5 s adulta; antes 7/5), y entre medio escupe fuego: un cono a
    lo más cercano cada 1.2 s (1.4x, Fuego). Sim ~1.6x del Summoner.
  - **Tres clases de verdad:** antes `BallLoadout::roles()` cortaba en 2
    aunque DESIGN decía que con 6 slots se podían 3 clases de 2; ahora
    cuenta hasta 3 (la tercera también aplica). Se ve como una órbita
    chiquita de puntos en su color con una lunita (estilo Atlas).
  - **Ares - Ira del Olimpo:** va por su propio reloj (la sed de sangre no la
    acelera), cada 2.5 s, 6x, y como mucho 9 a la vez entre estrellas
    cayendo y cráteres. Balas de los cañones 55%. Sim ~1.56x del Shooter.

- **Progresión:** por ahora todo se desbloquea como está; se ve más adelante.
- **5 actos:** se queda así; el balance entre actos se ve después.
- **Música:** los tracks actuales son libres, pero el usuario quiere sumar más.
- **Opción en español:** para lo último (ver la rama WIP pausada).
- **Ideas del usuario (2026-10-04), sin empezar:**
  - Faltan items para algunas clases, sobre todo **Assassin** y **Shooter**
    (en el código cada una ya tiene ~8; ver qué se siente que falta al jugar).
  - **Bosses nuevos:**
    - *Los gemelos:* dos que entran por lados distintos, se juntan y forman un
      enemigo más fuerte que atrapa una pelota hasta que muere uno de los dos.
    - *El tirador:* le dispara a tus pelotas y las hace rebotar (las desvía).
    - *El inmortal:* no se le puede hacer daño hasta que matás 3 oleadas de
      enemigos, cada una más fuerte que la anterior.
- **Tutorial:** hecho el 2026-10-04 (ver arriba).

### 2026-10-10: la web se ensancha, puentes entre clases y épicos/legendarios más lejos
- **Sigue siendo un círculo** (el usuario lo quiere redondo: `kStretchX` 1).
  Zoom mínimo 0.2 (entra todo), arranca en 0.7; los nombres se leen desde zoom
  0.5. Pasado el anillo de clases los anillos van cada 0.8 (las cadenas de items
  son largas). Se sacó el rótulo "CLASSES" (ahí viven los puentes).
- **Cada tipo de nodo, su forma:** perk = círculo, desbloqueo (habilidad,
  elemento, power-up, credo, legendario suelto) = cuadrado, épicos de la clase =
  hexágono con borde violeta, legendarios de la clase = estrella con aro dorado,
  puente = círculo partido en los colores de sus dos clases
  (`metaNodeKind`). La leyenda tiene una fila "kinds": pasar el mouse por una
  forma ilumina todos los nodos de ese tipo y dice su nombre. La tarjeta del
  nodo dice ruta y tipo.
- **Épicos y legendarios de cada clase, más lejos:** 20 nodos nuevos
  (`MetaEpic<Clase>` después de su lore, 3 prismas; `MetaLegend<Clase>` después
  del épico, 5 prismas). Sin ellos, una tirada de esa clase no da Epic /
  Legendary (`buildUpgradeCtx`). Partidas viejas (save v17): si ya tenías la
  clase, los dos nodos vienen comprados.
- **Puentes (10 nodos, `parent2`):** uno entre cada par de clases vecinas,
  justo afuera del anillo de clases; se abre desde cualquiera de las dos. Dan
  algo a las pelotas con cualquiera de las dos clases, el doble con ambas, 2
  niveles, 16 cores: Follow-through (Striker-Slinger, primer golpe lanzado
  +12%), Trick shot (Slinger-Shooter, +3% crítico), Lucky shot (Shooter-Jester,
  crítico +0.2x), Gambit (Jester-Assassin, +3% golpe doble), Shades
  (Assassin-Summoner, copias +15%), Kinship (Summoner-Support, invocaciones
  +8%), Harmony (Support-Mage, recarga +5%), Spellbrew (Mage-Alchemist,
  elementos +8%), Bedrock (Alchemist-Guardian, empuje +10% y +0.25 al núcleo
  por rebote), Juggernaut (Guardian-Striker, +6% daño y velocidad máxima).

### 2026-10-10: pasada de animaciones de items
- **Hoja de contacto (dev):** `SB_FXSHEET=<dir> ./space_breakers` guarda
  `fx_1.png..` con cada floritura de item/habilidad en tres momentos.
- **Rehechas:** Death sentence (el tajo barre, el corte se abre y saltan
  esquirlas), Lucky break (cuatro hojas de trébol de verdad y un destello),
  par del tragamonedas (tres rodillos que giran y frenan uno por uno, dos con
  el símbolo), Homecoming (un bumerán que sale en lazo y vuelve), Sleight
  (cartitas de verdad en abanico que salen volando).
- **Mejoradas:** Heavy landing (onda plana + polvo + piedritas), Spell chorus
  (anillos con runas que giran), Fission (una gota del elemento salta en arco y
  revienta), Frag rounds (perdigones con estela), Smoke bomb (pop + bocanadas
  que ruedan), Bomber (lenguas de fuego y humo), Ambush (cheurones que
  atraviesan sobre una línea), Mana spring (estelas hacia la pelota y un aro al
  llenarse), Saturate (una gota de cada elemento entra en espiral y se juntan),
  Wild magic (chispas de formas y colores con cola), Chronomancy (segundo anillo
  al revés y marcas), Reaper (hoja con filo, lomo y estela).
- **Nuevas (antes no se veía nada):** Cull (una X que se cierra sobre el
  enemigo y un destello), Backstab (un destello de cuatro puntas donde entra la
  daga), Twincast (un fantasma de la pelota se separa y lanza su aro).
- Sin tocar (ya se leían bien): Hook, Smite, Warp, Swap, Meteor, Time stop,
  Titan, Last line, Grand reaction, Ion cannon, carta del Jester, Primal, etc.

### 2026-10-10 (b): los items se abren en la web, los dioses en un anillo de oro, enlaces cruzados
- **Pool inicial y desbloqueos de items:** el nodo de una clase abre sus items
  Common y Uncommon. Después, por clase: "<Clase> rares" (desde la clase, 14
  cores), "<Clase> epics" (después del lore, 3 prismas), el primer Legendary con
  su propio nodo que lleva su nombre (5 prismas) y el resto de los Legendary en
  otro (6 prismas; el Guardian tiene uno solo, así que no lo tiene). Los nombres
  y textos salen de las tablas de items (`metaNodeName` / `metaNodeEffect`,
  `itemUnlockNode`): dicen exactamente qué items abren.
- **Dioses en la web:** un nodo por dios (8 prismas), al final de la cadena de
  legendarios de su clase, todos en un anillo exterior con un aro dorado. Se
  dibujan como soles: rayos dorados que giran, el disco en el color de la clase y
  su nombre siempre a la vista en dorado. Sin su nodo, el Shrine no deja
  ascender a esa clase (`shrineAscends`).
- **Saves viejos (v18):** si ya tenías la clase, sus raros, el segundo
  legendario y su dios vienen comprados.
- **Enlaces cruzados (`kMetaLinks`, punteados):** una segunda entrada a un nodo
  desde una ruta vecina con el mismo tema; cualquiera de las dos puntas lo abre:
  Sling -> Kinetics y -> Slinger (tiro y velocidad), Treasury -> Elite spoils
  (oro), Keen instinct -> Hunters (caza), Covenant -> Quartermaster (elegir al
  empezar), Bulwark pulse -> Singularity (atracción), Arc -> Static (rayo),
  Ember -> Meteor (fuego del cielo), Stonewall -> Quarry (piedra).
- La leyenda de formas suma el sol de los dioses; el nombre de la forma bajo el
  mouse aparece en la cabecera del panel.

### 2026-10-10 (c): web geométrica y constelaciones
- **Grilla polar:** los anillos van cada medio paso (y de a uno pasado el anillo
  de clases); cada anillo se corta en casillas parejas, múltiplo de 12 (así todos
  comparten los rayos), y cada nodo se apoya en una (`kSlotGap`, en `webAngle`).
- **Constelaciones:** dos nodos vecinos en el mismo anillo (a menos de
  `kConstellationReach`) se unen con una línea finita: apenas visible de entrada,
  en el color de la ruta si tenés uno, y blanca con una estrellita titilando en
  el medio cuando tenés los dos.
- **8 enlaces cruzados más (17 en total):** Channel -> Charged (carga), Split ->
  Twins (copias), Regen -> Tide (agua que cura), Damper -> Frost (frenar), Heft ->
  Mass (peso), Velocity -> Sling (velocidad), Muster -> Legion (más pelotas),
  Fool's luck -> Loaded dice (suerte).
- **Menos huecos (2026-10-10 d):** el anillo de clases pasa a 4.5 (lo de adentro
  respira), el segundo Legendary cuelga de los épicos al lado del primero (ya no
  en fila) y el dios del primer Legendary: las cadenas de afuera son un paso más
  cortas y se abren en horquilla; los dioses quedan a 0.8 del último anillo;
  rutas chicas reservan menos lugar (`kRouteMinTips` 4); zoom mínimo 0.24 (la web
  entera llena la pantalla).

### 2026-10-12: la web alejada respira, nodos en las líneas largas
- **Alejado no se pisa:** pasando zoom ~0.7 los nodos se achican al ritmo del
  layout (`nodeScale`, piso 0.36 en vez de 0.6); casillas de la grilla un poco
  más anchas (`kSlotGap` 0.5); los puntitos de nivel escalan y se esconden bajo
  zoom 0.4.
- **Líneas más suaves:** ruta ganada ~0.42 de opacidad (era 0.75), abierta 0.24,
  cerrada 0.12; constelaciones y enlaces cruzados bajan igual.
- **12 nodos nuevos en las líneas más largas:**
  - Camino al Slinger (era una sola línea de 5.6 anillos): Calling -> **Grip**
    (atrapás desde +6% más lejos por nivel, 2 niveles) -> **Wind-up** (primer golpe
    lanzado +6% por nivel, 2 niveles) -> Slinger.
  - **<Clase> mastery** (una por clase, 3 niveles, 30 cores): entre su primer
    Legendary y su dios, a mitad de camino. Los dos mini stats propios de la clase
    (`cfg::role`) crecen +50% por nivel (x2.5 al máximo). El dios ahora cuelga de
    la maestría; los dioses ya comprados siguen comprados.
- **Puentes exteriores (b):** 10 nodos más, uno entre cada par de clases vecinas
  allá afuera, a la altura de sus legendarios (cuelgan del primer Legendary de
  cada una, se abren desde cualquiera de los dos). Dan lo mismo que el puente de
  adentro del mismo par y se suman: Hurl (Follow-through), Deadeye (Trick shot),
  Jackpot (Lucky shot), Double cross (Gambit), Phantoms (Shades), Fellowship
  (Kinship), Resonance (Harmony), Transmute (Spellbrew), Earthwork (Bedrock),
  Colossus (Juggernaut). 2 niveles, 36 cores. Sirven sobre todo para que el
  borde de la web no se vea vacío.
- **Enlaces cruzados más limpios:** en vez de rayitas, puntitos chicos parejos
  que frenan antes de cada nodo.

### 2026-10-12: el combo de daño es una barra
- **Sube mucho más lento y sin saltos:** cada golpe suma un poco (30 golpes por
  +0.5x, antes 6); tope x5 a los 240 golpes (Overcharge lo duplica). El
  multiplicador es continuo (`World::comboLevel`), los "tiers" quedan como marcas
  para el oro por kill, el sonido y el calor del fondo.
- **Se vacía de a poco:** 2.4 s sin golpear y empieza a drenar 18 golpes por
  segundo hasta el próximo golpe (antes caía un tier entero de golpe).
- **HUD:** arriba a la izquierda, "x2.6" y una barra que se llena (azul -> dorado
  -> brasa), una marca cada +0.5x, la punta brilla y da un pop al cruzar una
  marca, una mecha finita abajo se consume mientras no pegás; sin mecha la barra
  titila y baja; al tope un brillo la recorre.
- Ojo: el oro por kill también escala con el tier del combo, así que baja un poco.
- **Se reinicia en cada pelea** (`World::resetCombo` en `App::startWave`): todas arrancan en x1, también los jefes.

### 2026-10-12: Ascensión (niveles de desafío)
- **Cómo se abre:** ganá una run en tu nivel más alto y se abre el siguiente (hasta
  10). Una partida que ya tenía runs ganadas arranca con el nivel 1 abierto.
- **Dónde se elige:** en la web, panel de abajo a la derecha: fila "Ascension N"
  (clic sube, vuelve a 0), solo si ya abriste alguno. Funciona encima de Normal o
  Hard. El hover lista todas las reglas activas y lo que paga.
- **Paga:** cores x(1 + 0.15 x nivel); ganar en 5+ da +1 prisma.
- **Las reglas** (cada nivel suma la suya a las de abajo; números en `cfg::asc`,
  textos en `progression/Ascension.hpp`):
  1. élites +25% vida y segundo Brute desde el acto 1
  2. sin reparación gratis antes de cada pelea
  3. enemigos +10% velocidad
  4. el Descanso repara la mitad y la tienda cuesta +25%
  5. jefes cambian de fase antes (75% / 45%) y se enfurecen más (x1.75)
  6. cada pelea élite tiene un rasgo: Armored (-30% daño recibido), Swift (+30%
     velocidad), Regenerating (3% vida/s), Vengeful (+50% daño al core)
  7. el combo se vacía el doble de rápido
  8. las elecciones dan una carta menos (2)
  9. cada jefe tiene una regla: Thick hide (golpes de pelota -60%), Warded (todo
     lo que no sea golpe de pelota -60%), Hex (cada 11 s congela 3 s tu pelota más
     fuerte; nunca la única)
  10. todo +15% vida y el jefe final con dos reglas
- **HUD:** "asc N" junto al acto; el rasgo del élite y las reglas del jefe al lado
  de su nombre (hover: qué hacen). La pelota congelada por Hex lleva un aro violeta
  que se consume.
- **Pendiente:** un sexto jefe para el nivel 10 (por ahora "todo +15% y dos reglas").

### 2026-10-12: preparando la beta abierta
- **Feedback** en el menú principal: abre el navegador en `cfg::app::feedbackUrl`
  (por ahora la página de itch; se cambia por un formulario o Discord ahí).
  `platform/Browser.cpp`: ShellExecute en Windows (shell32), xdg-open en Linux.
- **Versión** abajo a la izquierda del menú (`cfg::app::version`, "0.1.0-beta"):
  subirla con cada build que se publique, así el feedback dice en cuál fue.
- El menú aprieta sus filas cuando son más de 7, para que "Quit" no pise el marco.

### 2026-10-12: curarse es algo que se elige (playtest del usuario)
- **Sin curación gratis entre peleas, de base** (antes era la regla de Ascensión
  2): el core solo se repara si lo elegís - Mend (web: 3 por nivel antes de cada
  pelea), Regen, Mender, Descanso, la reparación del Choice, tratos del "?".
  Así importa más agarrar curación y duele más pagar con vida.
- **Cada acto nuevo repara el 50% del core** (`cfg::core::actHealFrac`), con un
  cartel "core repaired 50%".
- **Ascensión 2** pasa a ser "un acto nuevo repara solo el 25%".
- **Fortress:** su contra ahora es "sin reparación al empezar un acto, Mend no
  hace nada y el Descanso repara la mitad".
- **Hard** ya no menciona la reparación (es la base).
- **Strange cocoon** ("?") solo aparece si algún Legendary desbloqueado le entra
  a alguna de tus pelotas (`App::anyItemOfTier`).
- **Mapa:** las dos primeras filas de cada acto nunca son "?": arranca con dos
  peleas (la regla de "nunca dos peleas seguidas" deja en paz la fila 2).

### 2026-10-12: tutorial nuevo (más fácil, una parada de cada cosa)
- **Mapa:** pelea -> Recruit -> élite -> "?" -> tienda -> oleada final (sin
  Descanso).
- **Élite:** una sola carta, **Seeker** (Legendary del Striker), "Elite reward -
  a Striker Legendary".
- **"?":** +50 de oro de regalo y un cartel: "a ? can hold anything: deals,
  strangers, gambles, odd things" (al costado de la columna del mapa). Hint
  nuevo `Hint::Event` para el primer "?" de cualquier run.
- **Tienda:** sus primeros ítems son Common/Uncommon, así el oro alcanza para uno.
- **Final:** no hay jefe ni arena ancha ni cámara alejada: una pelea normal de
  la oleada 5 con 1.8x enemigos ("FINAL WAVE", hint `Hint::Finale`). Ganarla cierra
  el tutorial ("Tutorial complete"). El jefe y la cámara alejada recién en la
  primera run normal.
- **Enemigos del tutorial con 60% de vida** (`cfg::tutorial::enemyHpMul`); el
  tutorial ignora Hard y Ascensión.
- **SB_TUTSIM=<n>** (dev): corre las peleas del tutorial en orden (1 pelota, luego
  2, Seeker después del élite, sin reparar) con un jugador que solo aprieta Q:
  gana 52 de 60 y llega al final con ~39% del core. Un jugador real (apunta, más la
  pick del Recruit y el ítem de la tienda) debería ganar casi siempre.

### 2026-10-12: forja - lo que no se puede elegir se ve, y lo que sobra paga
- La regla de la forja (2 puntos; raro / épico solo como primero; legendario los
  2) ya estaba; ahora en la pantalla cada ítem que esos puntos no pueden tomar
  queda velado, así se ve que después de un épico solo quedan comunes, poco
  comunes, elementos y habilidades.
- **Un punto que nada puede tomar paga 20 de oro** (`cfg::gold::forgePointGold`):
  al terminar una mejora si sobra y no hay en qué gastarlo, o al entrar si no hay
  nada para mejorar. Irse de la forja a propósito con puntos útiles no paga.

### 2026-10-12: la leyenda de la web no adelanta nada
- La fila de figuras de "routes" (perk, unlock, bridge, class items, legendary
  item, god) muestra una figura recién cuando tenés un nodo de ese tipo comprado o
  comprable ahora (`nodeKnown` / `knownKinds`); al pasar el mouse se iluminan solo
  esos, no todos los de ese tipo. Una web nueva muestra solo perk y unlock.

### 2026-10-12: Blades corta de verdad (y más grande)
- **Bug:** el corte se revisaba cada 0.25 s solo donde estaba cada cuchilla en
  ese instante; girando a 7 rad/s saltaban ~100° entre revisiones y casi nunca
  tocaban a nadie. Ahora cada corte cuenta todo el arco que barrió cada cuchilla
  (un enemigo sobre el anillo que una cuchilla cruzó, recibe el corte). Medido en
  60 s contra un campo lleno: 76 cortes (antes 10). El Bufón usa la misma Blades
  (palo de picas): 30 cortes (antes 2).
- **Más grande:** órbita 70 (era 55), radio de corte 13 (era 9), la cuchilla
  dibujada 20 de largo (era 14) y más gruesa.

### 2026-10-12: playtest - menos duro al principio, más ítems, "?" nuevos
- **Primera habilidad: 2 cartas** (era 3, +1 con Calling). Calling ahora solo
  abre la web.
- **Lanzar:** una pelota tirada se puede volver a agarrar a los 0.4 s (era 0.6).
- **Acto 1 más blando:** enemigos con 75% de vida, su jefe 80%
  (`cfg::wave::act1HpMul` / `act1BossHpMul`); el tutorial lleva además su 0.8
  (60% en total; SB_TUTSIM: 57 de 60).
- **Tiendas desde la mitad del mapa** (`detail::shopFromRow`: fila (rows+1)/2,
  mínimo 3); la del pre-jefe queda. Traen ítem en el 85% de las visitas (era 60%).
- **Legendarios primero en tienda y jefe:** tienda 38/32/16/8/6 (legendario era
  2%), tesoro del jefe 40% épico / 60% legendario (era 65/35).
- **Sin elementos desbloqueados no salen ítems que los necesitan**
  (`itemNeedsElement`: Volatile, Seep, Reagent, Hollow point, Spellweave, Primal,
  Swarm...), ni el pacto Elemental. Los que traen su propio elemento (planetas,
  wyrms, carros) sí.
- **"?" que pagan con vida ahora hacen daño, no bajan la vida máxima** (Blood
  price, Blood forge): `World::woundCore`, nunca mata. El Altar no cambia.
- **"?" nuevos (tercer lote), casi ninguno por oro:**
  - Dark bargain: 15% de daño al core -> elegir un ítem a odds de élite.
  - Field medic: repara 35% del core; un ítem al azar pierde un nivel.
  - Blood rite: una habilidad al azar sube de nivel por 10% de daño al core.
  - Scavenge: gratis, elegir 1 de 3 ítems Common/Uncommon. En los actos 1-2 uno de
    los tratos de cada "?" es siempre este (`cfg::event::scavengeUntilAct`).
  - Unbinding (raro, 35% de las veces que podría, caro: 150 + 40 por acto): un
    pacto al azar pierde su precio y se queda con su regalo
    (`RunState::pactsPure`, guardado). Last Breath devuelve la vida máxima que
    quitó; Void Walls no se puede (su precio es su regalo).
- **Bug del Beam:** el rayo quedaba encendido entre peleas (la pelea terminaba
  antes de apagarlo) y aparecía al empezar la siguiente. Ahora al terminar la
  pelea Beam y Blades se apagan, y una pelea nueva arranca sin rayo.
- **Capturas (SB_SNAPSHOT):** el core escenificado es invulnerable
  (`snapInvuln_`) y un jefe que llega al core ya no termina la run si el core es
  invulnerable (dev); las peleas de jefe de Ascensión de prueba vuelven después a
  una pelea normal. Sin esto la prueba de guardar/cargar la run fallaba al azar.
- **Velos con la forma de la casilla:** en la forja (lo que no se puede tomar) y al
  arrastrar en el TAB (el hueco de origen y los destinos), el elemento lleva un
  hexágono, una habilidad un rombo y un ítem su barra - antes todo era un
  rectángulo y elementos / habilidades quedaban como un cuadro negro.
- **"?" y Descansos por acto (medido 2026-10-12):** en un camino al azar salen
  2.3-2.5 "?" y 2.1-2.5 Descansos por acto (contando la fila de antes del jefe);
  4-7% de los caminos no pasan por ningún Descanso.

### 2026-10-12: cierre del día
- **Forja:** el tope ahora es por casilla - ítems 3, elementos y habilidades 5
  (`App::forgeCap(slot)`); antes cortaba todo en 3 (Magic missile no pasaba de 3).
- **Nombre de la habilidad bajo el rombo:** uno largo ("Magic missile") se escribe
  más chico en vez de no escribirse.
- **Todo camino tiene al menos un Descanso y un "?" por acto**
  (`detail::ensureOnEveryPath`: si algún camino al jefe no pasa por uno, una pelea
  normal de ese camino se convierte - Descanso desde la fila 4, "?" desde la 3; si
  no hay pelea, un Recruit, una tienda o un élite; los Descansos primero). Medido
  en 20.000 mapas: ninguno falla.
- **Scavenge** seguro solo en el acto 1; después, 35% de las veces que podría.
- **Saltear el tutorial** paga lo mismo que ganarlo (cores de la oleada final +
  bono de victoria, 1 prisma), con un cartel en la web.
- **Aro blanco en la web:** el anillo que aparece al desbloquear una clase se
  apagaba con el reloj del cielo, que puede quedarse quieto - quedaba fijo. Ahora
  se apaga con el reloj real (1.6 s) y solo una vez.
- **Tarjeta de los nodos de la web:** nombre, ruta, nivel y pie se acomodan (el
  nivel baja a su línea si no entra al lado de la ruta, todo se parte en líneas)
  - antes "LEGENDARY ITEM" y "LEVEL 1 / 1" se pisaban.
- **Descripciones simples o detalladas** (Opciones, abajo; se guarda y sobrevive
  a "Reset progress"): en simple cada carta, ítem y nodo de la web muestra lo
  esencial - el texto hasta su primer ";", " - ", " (" o punto, y pasado ~90
  caracteres hasta la última coma (`simplifyText`, `upgradeInfo` /
  `metaNodeEffectFor`).
- **SB_TUTSIM** simula siempre un jugador nuevo (web vacía): antes tomaba la web
  del save cargado y daba de más. Con web vacía: 51 de 60, 35% de core al final.
