# Space-Breakers — rework de combate

Documento vivo. Recoge el cambio de concepto y el plan por fases.
Rama: `combat-rework`.

---

## 1. El cambio

**Antes:** una pelota rebota, cada rebote da puntos, la agarras y la lanzas para
que vaya más rápido. El objetivo es la puntuación.

**Ahora:** las pelotas son agentes autónomos que **derrotan enemigos**. Tú no las
pilotas segundo a segundo: las **lanzas** al empezar la oleada y **das forma al
campo de batalla** colocando objetos que desvían su trayectoria (agujeros negros,
rampas, imanes…). Referencia mental: Peggle + un sandbox de físicas + defensa de
núcleo.

El núcleo emocional que se mantiene: lanzar la pelota se siente genial, verla
rebotar y encadenar cosas engancha. Lo que cambia es **para qué** rebota.

---

## 2. Decisiones ya tomadas

| Tema | Decisión |
|------|----------|
| **Derrota** | Hay un **núcleo** en el centro de la arena. Los enemigos avanzan hacia él. Si su vida llega a 0, pierdes la arena (reintentas). |
| **Control en combate** | Lanzas la pelota una vez por oleada y es autónoma. Durante el combate solo **colocas / mueves / activas objetos de campo**. |
| **Economía** | Doble capa: los enemigos sueltan **chatarra** (moneda de progreso permanente) **y** el combo actual se reconvierte en **multiplicador de daño** dentro de la oleada. |
| **Primer jugable** | MVP completo: 1 arena, daño por contacto, 1 tipo de enemigo, oleadas con victoria/derrota, 1 objeto de campo (agujero negro atractor), sin paredes móviles, pantalla de *loadout* entre oleadas. |
| **Paredes móviles** | **Se eliminan.** Todo su código sale. |

---

## 3. Bucle de juego

### Segundo a segundo (dentro de una oleada)
1. **Preparación:** la pelota está aparcada. Colocas / recolocas tus objetos de
   campo dentro del presupuesto de la arena.
2. **Lanzamiento:** agarras y lanzas la pelota (mismo feel de siempre). Empieza
   la oleada.
3. **Combate:** la pelota rebota en los bordes y su trayectoria se curva por los
   objetos de campo. Al tocar un enemigo le hace daño y rebota. Los enemigos
   caminan hacia el núcleo. Puedes seguir moviendo/activando objetos de campo.
4. **Fin de oleada:** limpias a todos los enemigos → siguiente oleada. O el
   núcleo cae → arena perdida.

### Meta (entre oleadas / arenas)
- Gastas **chatarra** en: subir daño, más ranuras de objeto de campo,
  desbloquear objetos nuevos, y (más adelante) **clases de pelola** y más pelotas.
- Superas la arena → siguiente arena (layout, enemigos y presupuesto distintos).
- Cada X arenas: un **jefe**.

---

## 4. Qué se quita / se mantiene / se añade

### Se quita
- `struct Wall` y todo el sistema de paredes móviles (grab de pared, drift,
  `configureWalls`, `syncWallCount`, `WallSnapshot`, colisión bola-pared).
- La idea de "puntos por rebote" como objetivo. El rebote en el borde ya no
  puntúa por sí solo (o puntúa simbólico); el valor está en golpear enemigos.

### Se mantiene
- Agarrar y lanzar la pelota (ahora es "lanzamiento de oleada").
- Física de rebote, sub-stepping, regulación de velocidad crucero, squash, trail.
- Orbes de power-up (se re-tematizan como buffs de combate: ver §7).
- Arena con `sf::View` letterbox, audio procedural, efectos (anillos, glow, tint).
- Guardado en texto plano (se amplía el formato).
- Combo streak: se reusa como **combo de daño** (sube al golpear enemigos sin
  fallar durante X segundos; multiplica el daño).
- Prestigio / offline / hitos: siguen en la lista de futuro (§9), encajan igual.

### Se añade
- `struct Enemy`, `struct FieldObject`, `struct Core`, (fase 2) `struct Projectile`.
- Sistema de **oleadas** (spawner + estado de arena: activa / limpiada / perdida).
- **Objetos de campo** que aplican fuerzas a la pelota cada paso.
- Pantalla de **Loadout** (sustituye a la tienda): comprar mejoras + colocar
  objetos de campo con presupuesto.
- HUD de combate: vida del núcleo, oleada, enemigos restantes, chatarra.

---

## 5. Sistemas

### 5.1 Pelotas y clases
- MVP: una sola pelota, "clase" genérica. Daño por contacto = `f(velocidad, combo)`.
  Más rápida = más daño (mantiene el sentido de lanzarla fuerte).
- Fase 3: **clases**. Empiezas con una y le "das clases" (subes de nivel una
  especialización). Ideas:
  - **Ariete** — daño de contacto alto, mucho knockback.
  - **Artillero** — dispara proyectiles en la dirección de su velocidad mientras
    se mueve.
  - **Divisor** — al matar, probabilidad de soltar una mini-pelota temporal.
  - **Guardián** — ralentiza a los enemigos cercanos.
- Fase 4: **equipo / roster**. Varias pelotas a la vez, cada una con su clase.
  Se lanzan en secuencia o todas al empezar la oleada.

### 5.2 Objetos de campo (lo que más te gusta)
Se colocan en preparación y se pueden mover/activar en combate. Presupuesto de
ranuras por arena (mejorable con chatarra).

| Objeto | Efecto sobre la pelota |
|--------|------------------------|
| **Agujero negro** (MVP) | Atrae: aceleración hacia el objeto ∝ 1/dist², con radio de influencia y tope. |
| **Agujero blanco / repulsor** | Empuja hacia fuera. |
| **Bumper** | Rebote de alta restitución: al chocar, la pelota sale más rápido. |
| **Rampa / booster** | Zona direccional que añade velocidad en un sentido. |
| **Prisma** | Al atravesarlo, divide la trayectoria / duplica la pelota un instante. |
| **Nodo torreta** (fase 2) | Objeto que dispara cuando la pelota pasa cerca. |

Nota de diseño: por ahora los objetos afectan **solo a la pelota** (mantiene el
foco y es legible). "También curvan a los enemigos" queda como opción a probar.

### 5.3 Enemigos
- MVP: **Errante** — flota hacia el núcleo despacio, poca vida.
- Después:
  - **Corredor** — rápido, va directo al núcleo.
  - **Tanque** — mucha vida, lento.
  - **Escindido** — al morir se parte en dos pequeños.
  - **Jefe** — barra de vida grande, patrón de movimiento simple.
- Los enemigos **no atacan a la pelota**: su amenaza es llegar al núcleo. (Si en
  el futuro las pelotas tienen vida, se revisa.)
- Al morir sueltan chatarra (y un anillo de impacto + sonido).

### 5.4 Núcleo
- Círculo en el centro con vida y barra/anillo. Un enemigo que lo alcanza le
  quita vida y desaparece. Vida 0 → arena perdida.
- Mejorable con chatarra (vida máx, quizá un pulso que repele) — futuro.

### 5.5 Oleadas y arena
- Una arena = layout fijo + lista de oleadas + presupuesto de objetos de campo.
- Spawner: mete `N` enemigos por oleada desde los bordes, escalonados en el
  tiempo. Oleada limpiada cuando no quedan enemigos y no hay más por aparecer.
- Arena limpiada al superar todas sus oleadas → pantalla de resultados → siguiente.

### 5.6 Economía
- **Chatarra**: moneda de progreso. Cae de enemigos. Se gasta en el Loadout.
- **Combo de daño**: `daño = base(velocidad) * (1 + tier*k)`. El tier sube al
  golpear enemigos seguidos; decae por tiempo sin golpear. Es el `comboStreak_`
  actual, realimentado desde impactos en vez de rebotes.
- **Puntuación**: se conserva como marcador/records (Stats), no como objetivo.

---

## 6. Encaje en el código actual

La arquitectura modular tras el refactor lo hace abordable:

| Módulo | Cambios |
|--------|---------|
| `sim/Entities.hpp` | quitar `Wall`; añadir `Enemy`, `FieldObject`, `Core`, `FieldKind`; ampliar `FrameEvents` (chatarra, muertes, golpe al núcleo, oleada limpiada, arena perdida/limpiada). |
| `sim/World.*` | quitar todo lo de paredes; añadir `enemies_`, `field_`, `core_`, estado de oleada; en `step()`: integrar fuerzas de campo sobre la pelota, IA de enemigos, colisión bola↔enemigo con daño, spawner, condición de derrota. `grabAt` reusa la lógica de agarre para **objetos de campo** (prioridad) o la pelota. `toggleDriftAt` → `toggleFieldAt`. |
| `sim/Collision.*` | `circleVsWall` se recicla para bola↔enemigo y bola↔núcleo (misma resolución círculo-círculo/AABB). Nueva `applyFieldForce`. |
| `progression/Upgrades.hpp` | re-scope: `UpgDamage` nuevo; el hueco de "Walls" pasa a "Ranuras de campo"; mantener Speed y Multiball; Combo/Luck se revisan. |
| `progression/GameData.hpp` | `walls` → `field` (vector de `FieldSnapshot{x,y,kind,strength}`); `scrap`; `arena` (índice). |
| `platform/Save.*` | versión 3: líneas `field i x y kind strength`; `scrap`; `arena`. Parser sigue tolerante. |
| `render/WorldRenderer.*` | quitar dibujo de paredes; dibujar núcleo (anillo de vida), enemigos (círculo + arco de vida), objetos de campo (agujero negro: disco oscuro + remolino + radio de influencia tenue). |
| `ui/` | `ShopScreen` → `LoadoutScreen` (comprar + colocar objetos de campo). `Hud` de combate (vida núcleo, oleada, enemigos, chatarra). `Screens` gana un sub-estado de "preparación" antes de lanzar. Reescribir `HowToScreen`. |
| `core/Config.hpp` | nueva sección `cfg::combat` (daño base, knockback, radios), `cfg::field` (fuerza, radio, tope), `cfg::wave` (nº enemigos, cadencia), `cfg::core` (vida). |

---

## 7. Power-ups re-tematizados (cuando toque)

- Doble Puntos → **Doble Daño**.
- Cámara lenta → igual (útil para "leer" el campo).
- Oleada de velocidad → **Sobrecarga** (daño y velocidad).
- Rebote dorado → **Perforante** (atraviesa un enemigo sin frenar).
- Fase → **Fantasma** (atraviesa objetos de campo — se re-evalúa).
- Frenesí x3 → **Frenesí** (x3 daño, corto).

---

## 8. Roadmap por fases

- **Fase 0 — MVP (esta rama, ahora).**
  Quitar paredes. Núcleo + 1 enemigo + oleadas + derrota. Daño por contacto
  `f(velocidad, combo)`. 1 objeto de campo: agujero negro (colocar hasta 2,
  mover/activar en combate). Loadout mínimo: subir daño, +1 ranura. HUD de
  combate. Guardado v3. Debe compilar y ser jugable de principio a fin de una
  arena.
- **Fase 1 — Disparos.**
  Clase Artillero y/o nodo torreta. `struct Projectile`. Proyectil↔enemigo.
- **Fase 2 — Variedad de campo y enemigos.**
  Repulsor, bumper, rampa. Corredor, tanque, escindido. Primer jefe.
- **Fase 3 — Clases de pelota.**
  Árbol de clase con niveles. Power-ups re-tematizados.
- **Fase 4 — Equipo y arenas.**
  Roster de varias pelotas. Secuencia de arenas con tutorial guiado. Progresión
  meta (prestigio, offline, hitos).

---

## 9. Preguntas abiertas (para más adelante)

- ¿La pelota **atraviesa** enemigos débiles y **rebota** en los duros, o rebota
  siempre? (MVP: rebota siempre, es lo más legible.)
- ¿Los objetos de campo también curvan a los enemigos?
- ¿Se lanza una pelota por oleada o se puede relanzar si se "muere"/pierde
  energía? (De momento: una por oleada, autónoma.)
- ¿Cómo entra el equipo: todas las pelotas a la vez o por turnos?
- ¿La puntuación clásica se enseña o se guarda solo como record interno?
