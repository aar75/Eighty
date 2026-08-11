# Eighty

A polyphonic synth plugin with **two engines — a Yamaha CS-80 model and a
Roland Jupiter-8 model** — behind one clean, flat, readable digital UI
(deliberately *not* a hardware panel clone). Built with JUCE.

The header engine chips pick what sounds: **CS-80** everywhere, **JP-8**
everywhere, **Split** — a split key (with a CS LOW toggle for which side is
which) routes each note to its engine — or **Layer**, which plays both
engines on every note (in Unison mode the stack alternates engines). A
**CS/JP mix** knob balances the two engines in Split and Layer modes. The
PANEL chips switch which engine's controls are shown — the JP-8 panel has
its own slate-violet identity color; shared sections (LFO, Mix, Touch,
Voices, Arp, FX) stay put.

The UI is the "Cream Strip" design: a light cream panel with flat strip
sections, vertical faders for primary parameters, small knobs for secondary
ones, chip-stack selectors, LED toggles, and colored section underlines.
Every control has a hover tooltip; touched values read out in the footer
status line (so tooltips are never covered).

## Formats

- **Standalone** app
- **VST3** and **AU** (built by default)
- **AAX** — enabled automatically when the licensed Avid AAX SDK is available:
  `export AAX_SDK_PATH=/path/to/AAX_SDK` before configuring. (AAX also
  requires PACE signing to load in retail Pro Tools.)

## Build

```sh
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release -j 8
```

Requires a sibling `../JUCE` checkout (or it will be fetched automatically).
Built plugins are copied to `~/Library/Audio/Plug-Ins/{VST3,Components}`.

## What's modeled from the CS-80

- **Two oscillator channels (I & II)** — each a single VCO core producing
  saw + variable-width pulse simultaneously (mixable, PolyBLEP anti-aliased),
  with 32'/16'/8'/4' ranges, detune, and per-channel PWM depth from a
  dedicated PWM LFO.
- **Filter** — the CS-80's series topology: 12 dB/oct high-pass into a
  12 dB/oct resonant low-pass (TPT state-variable filters), with a
  soft-saturating drive stage.
- **Single global LFO ("sub oscillator")** routed to pitch / filter / amp,
  with a delay/fade-in — shared by all voices, as on the original.
- **Touch section** — the CS-80 is famous for polyphonic aftertouch. Real
  channel/poly aftertouch is honored, and a **simulated pressure ramp**
  (Rise) builds while a key is held so computer-keyboard players get the
  swelling vibrato/brightness/level too.
- **Analog inconsistency (Drift)** — each of the 16 voice "cards" gets fixed,
  seeded component tolerances (pitch, cutoff, envelope speed, pulse width,
  level, pan) plus a slow random pitch walk, scaled by one knob. Voices are
  also panned per-card (odd/even left/right) via Spread.
- **Ring modulator and ribbon are intentionally omitted** (ribbon scrapped by
  design; ring mod left out to keep the panel light).

## What's modeled from the Jupiter-8

- **Two VCOs** — VCO1: tri/saw/pulse/square; VCO2: tri/saw/pulse/noise, with
  semitone/fine tune, 16'/8'/4'/2' ranges, **hard sync** (VCO2 to VCO1) and
  **cross-mod** (VCO2 FMs VCO1), mixed with one balance knob.
- **Filter** — non-resonant HPF into an IR3109-style resonant low-pass
  switchable **12/24 dB/oct** (24 dB mode resonates harder, like the original).
- Its own filter/amp ADSRs. Drift, touch, LFO, and FX apply to both engines.

Both engines also get a **sub-oscillator** (in each panel's MIX section): a
square or triangle one or two octaves below that engine's first oscillator,
mixed in ahead of the filter. Neither original had one — it is there for
weight, and it is the cheapest way to make a thin patch sound large.

## Voice modes

Each engine picks its own mode and its own portamento, so a JP-8 lead can
glide over a CS-80 pad that doesn't:

- **Poly / Mono / Legato** — as usual.
- **Unison** — a fixed number of detuned voices per note.
- **Stack** — the whole pool spreads over whatever you are holding: one note
  gets all 16 cards, two notes get 8 each, and so on, detuned and panned
  across the stack. Adding a note *releases* the surplus voices off the
  older ones rather than stealing them, so the thinning is a fade. This is
  the loudest, widest thing the synth does; pair it with 16 voices, a little
  Detune, and Width above 100%.

## Step sequencer

Two tracks x 16 steps, sharing the arp's rate / division / sync / gate (arp
and sequencer are mutually exclusive). Each track has its own loop length —
different lengths give polymeter — plus mute and an engine target, so track
A can be a CS-80 bass under a JP-8 track B. **Live playing runs on top of a
running sequence**, which is the point of it.

- **REC** step-records into the armed track from the cursor: play a note or
  chord, and the cursor advances when you let go. Stops after one lap.
- **Click** a step to move the cursor there, and to drop in any keys you are
  currently holding. **Drag** up/down transposes a step, **double-click**
  clears it, **right-click** opens the step/track menu (hold, clear, set
  loop length here, copy A/B, transpose an octave).
- **Held steps** — a step can occupy more than one step, so a bass note can
  ring under a busier line. Drag a step left/right, or pick "Hold for" from
  its menu. The steps it covers are greyed and skipped, and a tie line runs
  through them; Gate still shortens the note, measured against the whole
  held length rather than one step. Holds wrap around the loop end.

Live notes are tagged with a different voice owner from each sequencer
track, so a step ending never releases a note you are holding, and a busy
track recycles its own voice cards before touching yours.

## Presets

The bar in the tab strip is the whole preset system: `<` / `>` step through
the folder, the name button opens the list, **SAVE** snapshots the current
sound. A preset is every parameter plus the per-key engine map and the
sequencer pattern — not the MIDI-learn map (that belongs to your hardware)
and not the hosted VST3 chain (reloading plugins on every switch would
stall). Files are `.eighty` XML in
`~/Library/Application Support/Eighty/Presets`, and the name menu has a
"Reveal folder" item. An asterisk after the name means you have edited it
since it was loaded.

## Everything else

Hold latch, arpeggiator with an "As Played" mode (sync or free-rate), pitch
wheel (sprung), full MIDI learn (right-click any knob), chorus / delay /
tremolo FX, output width control, velocity sensitivity, master tune/volume.

Two displays in the header: a triggered waveform trace, and a vector
(lissajous) display rotated so mono draws a vertical line and stereo width
opens it sideways — a loud, wide patch fills the circle.

Parameters are smoothed at control rate inside each voice, so sweeping a
cutoff — by mouse, arrow key or MIDI CC — glides instead of stepping.

**VST3 insert chain** — the panel next to the keyboard loads up to four
external VST3 effects at the end of the chain (post-FX, pre master volume).
"+ FX" opens a type-to-search list of scanned plugins (filter on name or
manufacturer, up/down to move, Return to load); each loaded plugin opens
its own editor window, and plugin states are saved with the host session.
Reported latency is forwarded to the host.

**VST3 synth layer** — "+ SET SYNTH" in the same panel opens the same
searchable list, filtered to instruments, and loads one as a third layer
across all keys with its own level knob. It's
fed the engine's *effective* note stream — so it follows the arpeggiator,
hold latch, pitch wheel, mod wheel and sustain pedal — and its audio runs
through Eighty's FX chain.

**Crash-safe plugin scanning** — scanning probes each plugin binary; if one
kills the process, it's remembered via a dead-man's-pedal file, blacklisted
on the next launch, and skipped forever after. Scan results are cached
incrementally, so a rescan resumes where the crash happened.

## Computer keyboard

| Keys | Action |
| --- | --- |
| `A W S E D F T G Y H U J K O L P ;` | play notes (piano layout) |
| `Z` / `X` | octave down / up |
| `C` / `V` | velocity down / up |
| `B` | toggle Hold |
| `N` | toggle Arp |
| `M` | sequencer play / stop |
| `R` | sequencer record |
| `←` / `→` | select previous / next control (click also selects) |
| `↑` / `↓` | adjust the selected control — hold to sweep it smoothly |
| `Shift+↑` / `Shift+↓` | pitch bend (springs back) |
| `Esc` | deselect |

Right-click any knob → **MIDI Learn**, then move a hardware control.
Mappings are saved with the plugin state.

## Code map

- `Source/DSP/Voice.h` — one voice card, playable as CS-80 or JP-8 per note
- `Source/DSP/SynthEngine.h` — voice allocation, modes, hold, arp, step sequencer, split, global LFOs
- `Source/DSP/CS80Filter.h`, `JP8Filter.h`, `Oscillator.h`, `Envelope.h`, `LFO.h`, `Effects.h`
- `Source/PluginProcessor.*` — parameters, MIDI handling, MIDI learn, FX chain
- `Source/PluginEditor.*` — flat UI, scope, pitch wheel, computer-key handling
