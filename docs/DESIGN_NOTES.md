# Design Notes — Next Version

Notes from a design discussion on 2026-10-27 about where The Sentry goes after
the IR-beacon build. Nothing here is built yet. The current build (beacon clip,
4× TSOP38238, MG996R servos, Honeywell HT-900) is unchanged.

Goals carried into the discussion:
- Aim airflow at the user in both pan (X) and tilt (Y).
- Keep on-device compute as small as possible.
- Work in a home workout room.

---

## 1. Sensing — thermal array recommended

A thermal array sensor sees people as warm blobs in a small temperature image.
The blob's column gives pan, its row gives tilt, so X and Y come straight out of
the sensor with no depth or height assumptions.

**Per-frame work:** subtract a slow-moving background, threshold, take the
centroid of the biggest warm blob, filter it. A few hundred operations per
frame; an ESP32 does it at 4–8 fps, which is plenty for a fan.

Why it fits a workout room:
- Exercise makes people warmer, so they stand out more.
- The warmest part is usually head and upper body, which is where the air should go.
- Works in the dark, and it is not a camera (people change clothes in that room).

### Options compared

| Option | Verdict |
|---|---|
| **Thermal array** | Recommended. X and Y directly, trivial compute, no camera. |
| PIR | Detects motion, not position. Coarse left/right, no tilt, loses a person holding a plank or riding a bike. |
| Cheap mmWave radar (HLK-LD2450, ~$15) | Position on the floor only: pan but no tilt. Spinning fan blades next to it create strong false readings. 3D radar (TI IWR6843) costs more and needs more processing. |
| Camera + person detection | X and Y, but the heaviest compute, needs light, and puts a camera in the room. |
| IR beacon (current build) | Works, and picks the right person by design, but the user has to wear and charge a clip. |

### Sensor choice

| Sensor | Resolution / view | Approx. cost (hobby) | Use when |
|---|---|---|---|
| Panasonic Grid-EYE AMG8833 | 8×8, 60° | $20–40 | Fan covers one or two exercise spots 2–4 m away |
| Melexis MLX90640, 110° lens | 32×24, 110°×75° | $50–75 | Fan sits in a corner and covers the whole room; needed for the raised-hand gesture |

### Things to design for
- **Glass and clear plastic block thermal IR.** The sensor needs an open hole, or an HDPE / germanium window.
- **Other warm objects** (treadmill motor, heater, sunny window, pets). Background subtraction handles anything static; track blobs over time so a passing dog doesn't steal the fan.
- **Hot rooms reduce contrast.** At 30 °C+ skin is only a few degrees above the room. Use an adaptive threshold, not a fixed one.
- **Smoothing.** Filter the target and add a deadband so the fan doesn't twitch with every rep.
- **Mount the sensor on the fixed base,** not the moving head. Each pixel then maps to a fixed pan/tilt angle and calibration happens once.

---

## 2. Picking one person when several are in the room

Thermal can't tell people apart, so designation is two steps: **pick once,
then follow that blob** frame to frame (nearest blob to last position).

Ways to pick, cheapest first:

1. **Zones.** "Follow whoever is on the treadmill." No wearable, almost no code. Covers most home gyms.
2. **Raise a hand to claim the fan.** A raised arm makes the blob taller; held ~2 s, the fan locks on and chirps/blinks. Needs the MLX90640; the 8×8 Grid-EYE probably can't see it reliably.
3. **Tap the person in a phone app.** Fan streams the thermal frame over Wi-Fi; no extra on-device work. Also the easiest way to fix a wrong lock.
4. **Wearable heat tag.** A chemical hand warmer (45–55 °C) in an armband shows as a hot spot inside one blob. A large shiny foil/Mylar patch reads *cold* and works too, but must be shirt-panel size at 3 m.
5. **IR LED beacon + PixArt PAJ7025 tracking camera** (the Wii remote sensor). The sensor finds IR dots in hardware and reports coordinates, so the MCU does no image work. Most reliable way to pick a specific person; it is the current build's beacon idea with a sensor that gives true X/Y.

**Crossings:** when blobs merge, keep the target's last position and velocity;
when they split, take the piece nearest the predicted position. If unsure, hold
the last good aim and wait for a re-claim. Never silently switch people.

**Recommended:** zones by default, raised hand to switch, app tap as backup.
Add the heat tag only if people often work out side by side.

---

## 3. The fan

Don't build the fan itself (blades, motor, housing). Build the aiming and the
control board.

### What makes a good fan for this
- **Air circulator, not pedestal/box fan.** A tight stream that carries 3–5 m; a wide pedestal fan spreads air so much that aiming barely matters.
- **DC motor from a 12/24 V wall adapter.** PWM speed control, quiet, all electronics stay low voltage (no 120 V work, easier certification).
- **Both axes motorized with stepper motors.** Steppers can point at a set angle. Plain AC oscillation motors (2 wires, "120V 4W 5 RPM") only sweep and can't aim.
  - Tell-tale: 4–5 wire motors (often 28BYJ-48 type) = stepper; 2 wires = synchronous AC.
  - Fans that let you set the oscillation angle or recalibrate pan/tilt usually use steppers.
- Steppers need a **limit switch per axis** (or other homing) since they don't know their position at power-on.
- Optional: speed up as the person gets warmer — the thermal sensor already measures it.

### Three ways to get aiming
1. **Modify a fan that already moves both ways** — keep motors and gears, replace the control board with ours. Best for a prototype.
2. **Fixed fan on a 3D-printed stepper pan/tilt base** with belt or gear reduction (hobby servos are too weak for a 1–2 kg vibrating head). Fallback if no suitable 3D fan.
3. **Fixed fan + motorized vanes** (car-vent style louvers on servos). Cheap and light, but only ±30–40° and costs throw.

### Prototype candidates

| Fan | Why | Unknowns |
|---|---|---|
| **China Loong Blower BL-3731** (factory sample, ~$24–26, MOQ 2) | 14" circulator, 3D oscillation, brushless 28 W at 24 V DC, auto tilt, OEM/ODM service. A factory platform, so the prototype tests a fan we could actually produce on. | Listing seen in search results only (site blocked a full read). Ask: are both oscillation motors steppers, wire counts, do motors plug into the board, will they share the board wiring/schematic, which certifications? |
| **Dreo PolyFan / TurboPoly 513S** (~$115–140) | DC brushless, motorized 120° pan / 105° tilt, app sets angles in 10°/5° steps. The open-source hass-dreo integration sends direct pan/tilt position commands to at least one Dreo circulator and tells users to "recalibrate pan/tilt" — strong signs of steppers with homing. | No teardown found; open the base and count motor wires before committing. Buy with easy returns. |
| Woozoo (Iris Ohyama) Globe Fan | DC, motorized oscillation both ways. | Oscillation is on/off only, no angle control; motors are separate geared units of unknown type. Weaker bet. |

**Zero-modification test:** the Dreo can probably be aimed through its app API
(as hass-dreo does) to check that aiming at a person helps, before opening it.
Commands go through Dreo's cloud and one model needs ~8 s between pan/tilt
commands, so it's only good for testing the idea.

**Plan:** order 2 BL-3731 samples if the seller's answers check out; buy one
Dreo 513S now to start testing aiming while samples ship (1–3 weeks).

---

## Sources
- [Notebookcheck: Dreo TurboPoly Fan 513S review](https://www.notebookcheck.net/A-fresh-breeze-on-hot-days-Smart-Dreo-TurboPoly-Fan-513S-review.1087865.0.html)
- [Lanoc: Dreo PolyFan 513S review](https://lanoc.org/review/smart-home/dreo-polyfan-513s-air-circulator-fan?start=1)
- [inkl: Dreo PolyFan review (angle slider, tilt memory)](https://www.inkl.com/news/i-tried-the-dreo-polyfan-and-found-it-s-packed-with-smart-features-like-the-big-brands-but-costs-a-lot-less-here-s-why-i-love-this-standing-pedestal-fan)
- [hass-dreo: supported models (`fixedconf` pan/tilt, settle delay)](https://github.com/JeffSteinbok/hass-dreo/blob/main/SUPPORTED_MODELS.md)
- [Woozoo Globe Fan](https://irisusainc.com/products/woozoo-globe-fan-medium-multi-oscillation-powerful-quiet-dc-motor-remote)
- [kakaku.com: Iris Ohyama PCF-SDC18T oscillation repair thread](https://bbs.kakaku.com/bbs/K0001141583/SortID=24839244/)
- [China Loong Blower BL-3731 listing](https://www.china-blowers.com/product/sleep-mode-3d-oscillation-fan-14-inch-dc-motor-pedestal-air-circulator-12-speeds-7-blades-2-in-1-battery-household-hotel-plastic/)
- [Eurographics: Smart Fan thermal tracking robot (2014)](https://diglib.eg.org/handle/10.2312/ve.20141371.007-008)
- [Hackster: IntelliFan](https://www.hackster.io/de-anza-college/intellifan-smart-portable-fan-psoc-6-ai-evaluation-kit-98e096)
