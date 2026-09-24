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

- **Musica de fondo. [IMPLEMENTADO 2026-09-07]**
  - `Audio` ahora ademas streamea dos loops OGG:
    `assets/music/menu.ogg` (menus/loadout) y `assets/music/game.ogg`
    (run viva: Play, Choice, Pause y cartel del boss incluidos).
  - `Audio::setTrack(Track)` cambia de loop; `App::update` lo llama cada
    frame con `data_.run.active ? Game : Menu` (idempotente). El toggle de
    sonido pausa/reanuda la musica.
  - Archivos opcionales: si faltan, el juego suena igual que antes. Los
    `.m4a` no sirven (SFML no decodifica AAC) - hay que convertir a OGG.
    Ride junto al resto de `assets/` en el copy de CMake y el `install`.

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
    mapa del acto 2. Selector de pelota/slot sacado a **EquipScreen**
    (`EquipSource` Choice / Shop / Forge). Pantallas nuevas en `ui/RunScreens.cpp`;
    los paneles de loadout viven en `ui/Widgets`.
  - Falta: playtest (la UI no se pudo clickear acá), precios y pagos de oro,
    nodos de mapa que den roles (pedido del usuario), eventos.

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
