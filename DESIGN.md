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
| **Movimiento** (2026-09-01) | Las pelotas **orbitan el núcleo** (no van en línea recta) y se **curvan hacia el enemigo más cercano** para interceptarlo. Flingear una pelota la saca de la órbita y vuelve en espiral. |
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

## 7. Power-ups re-tematizados (cuando toque)

Doble Puntos → **Doble Daño** · Cámara lenta → igual (para leer el campo) ·
Oleada de velocidad → **Sobrecarga** · Rebote dorado → **Perforante** ·
Fase → **Fantasma** (atraviesa estructuras, a revisar) · Frenesí x3 → **Frenesí**.

---

## 8. Roadmap por fases

- **Fase 0 — MVP roguelite. [IMPLEMENTADO]**
  Bucle completo jugable: Hub -> Play (oleadas) -> Choice -> ... -> RunSummary ->
  Hub. Nucleo central con vida, 1 enemigo (Errante) que va al nucleo, spawner de
  oleadas con escalado, muerte -> resumen -> nucleos -> hub con 2 desbloqueos.
  Guardado v4 (meta siempre, run reanudable con `ball i <elem>`).

- **Fase 0b — orbita + pelotas elementales. [IMPLEMENTADO 2026-09-01]**
  - Las pelotas **orbitan el nucleo** (`cfg::orbit`) y se **curvan hacia el
    enemigo mas cercano** (`interceptAccel`). Ya no van en linea recta.
  - **Agujero negro / estructuras: fuera.** `FieldObject` eliminado del codigo.
  - **Pelotas elementales** (`enum class Element`): Fire (quemadura DoT), Wind
    (dispara `Projectile` al mas cercano), Water (suelta `Puddle` que daña),
    Stone (suelta `Obstacle` que bloquea enemigos). Se compran en la Choice con
    chatarra; el precio sube con el nº de pelotas.
  - Oferta "ganar chatarra": eliminada. Run empieza con 2 pelotas Plain + 25
    chatarra. Nucleo se cura `cfg::core::waveHeal` al limpiar una oleada.
  - Verificado headless: ~15 oleadas, sin NaN, elementos activos, curva de
    dificultad decreciente (afinar escalado de enemigos mas adelante).

  Detalle original:
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
- **Fase 1 — Disparos.** Clase Artillero / nodo torreta. `Projectile`.
- **Fase 2 — Variedad.** Repulsor, bumper, rampa. Corredor, tanque, escindido.
  Primer jefe. Afijos de élite.
- **Fase 3 — Clases y modificadores profundos.** Árbol de clases. Power-ups
  re-tematizados. Más modificadores (cadena, esquirla, órbita).
- **Fase 4 — Equipo y meta grande.** Roster de varias pelotas. Hub ampliado,
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
