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

The UI is the "Eighty Dark" design: a dark blue-grey chassis with flat card
sections, vertical faders for primary parameters, small knobs for secondary
ones, chip-stack selectors, LED toggles, and colored section underlines.
Controls wear their section's accent color, and every value has an
amber-on-dark readout under it. Every control has a hover tooltip; touched
values read out in the footer status line (so tooltips are never covered).

The visualizers are drawn from live state rather than as decoration: the
header carries a zero-crossing-triggered waveform scope over a quarter-
division graticule and a lissajous vector display with phosphor persistence,
and each section's mini display plots that section's real values — the
filter's magnitude response on a log frequency axis (both corners, both
resonances), the envelopes' actual attack/decay/release times with their
exponential curves, and the LFO's selected waveform.

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
- **Two complete channels** — the real CS-80 is two 8-voice synths in one
  chassis: each channel has its own filter, VCA and envelope generators, and
  the two layers moving independently is what makes its brass and strings
  work. Rather than duplicate the whole panel, channel II runs the *same*
  settings offset by four controls in the **CH II** section (cutoff, res,
  env amount, envelope speed ratio). All-zero is a single shared filter.
- **Filter** — the CS-80's series topology: 12 dB/oct high-pass into a
  12 dB/oct low-pass (TPT state-variable filters), **resonant on both** —
  open the HPF against a closed LPF for its bandpass, chime and vocal
  patches — with a soft-saturating drive stage. Deliberately cannot
  self-oscillate: Yamaha put limiting resistors in the resonance path.
- **Filter envelope is IL/AL/A/D/R, not ADSR** — it starts from an Initial
  Level and attacks to an Attack Level. Raising IL shortens the sweep and
  dulls the tone; lowering it enriches it.
- **A sine that bypasses the filters**, joining at the VCA mixer — a sine has
  no harmonics for a filter to work on, so filtering it would only cost
  level. It is why CS-80 pads keep a fundamental under a closed filter.
- **Touch Response initial pitch bend** (V.BEND in PERFORM) — a hard key
  strike slides up into the note from just below, the way a brass player's
  embouchure does. Arguably the most recognisable thing the instrument does.
- **BRILLIANCE and RESONANCE performance macros** — one offset over every
  filter cutoff in the instrument and one over every resonance, on top of
  whatever the patch says. The two most-used sliders on a real CS-80.
- **Ring modulator** — a sine carrier with its own attack/decay envelope,
  where the envelope also drives the carrier's *speed*. Deliberately not
  keyboard-tracked: that fixed pitch beating against the note is where the
  inharmonic, metallic character comes from. Offered to both engines.
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
- **The ribbon controller is intentionally omitted** (scrapped by design —
  it does not map to a mouse or a computer keyboard).

## What's modeled from the Jupiter-8

- **Two VCOs** — VCO1: tri/saw/pulse/square; VCO2: tri/saw/pulse/noise, with
  semitone/fine tune, 16'/8'/4'/2' ranges, **hard sync** (VCO2 to VCO1) and
  **cross-mod** (VCO2 FMs VCO1), mixed with one balance knob. **LOW** drops
  VCO2 out of keyboard tracking and runs it at LF, which is how most real
  Jupiter-8 sync and cross-mod patches are built.
- **Filter** — a gentle 6 dB/oct non-resonant HPF into a **four-stage ladder
  low-pass with one global feedback path**, switched 12/24 dB by tapping
  stage 2 or stage 4 — which is what the IR3109 actually does. This
  topology, not the cutoff curve, is what separates a Jupiter-8 from the
  CS-80's state-variable filter:
  - A ladder loses DC gain as resonance rises, which is why a Minimoog thins
    out when you crank it. Roland compensated by boosting the input back into
    the chip, so the JP-8 keeps its body — the reason it gets called the
    fattest of the classic Rolands. Measured here: cranking resonance to
    maximum moves the low end **+1.0 dB** instead of the −14.6 dB an
    uncompensated ladder would lose.
  - Four stages reach 180° of phase, so **24 dB mode self-oscillates**. Two
    stages never do, so **12 dB resonates but cannot howl** — matching the
    real slope switch rather than bypassing a stage.
- **ENV-1 polarity invert** for reverse sweeps and plucks, and its own
  filter/amp ADSRs. Drift, touch, LFO, ring mod and FX apply to both engines.

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

Two tracks x 16 steps, sharing the step clock with the arp. Each track has
its own loop length — different lengths give polymeter — plus mute and an
engine target, so track A can be a CS-80 bass under a JP-8 track B. **Live
playing runs on top of a running sequence**, which is the point of it.

### The arpeggiator is per engine

**CS** and **JP** in the ARP section arm the arpeggiator on each card
separately, so the CS-80 can walk a chord while the JP-8 holds it underneath —
or the other way round. In Layer mode that is one instrument arpeggiating and
one sustaining from the same keys; in Split, each side follows its own
engine's switch. Both on is the ordinary arpeggiator, both off is off, and
`N` toggles the pair together.

Everything else about the arp — mode, division, octaves, rate, gate — is
shared, so the two run the same figure on their own cards rather than
becoming two unrelated sequencers. Each keeps its own position and its own
sounding note, which is what lets them come apart at all.

A held note may now be sounding on one card and being arpeggiated on the
other, so the note stream fed to a hosted VST3 synth layer is
reference-counted: it gets one note-on for a pitch however many cards are
playing it, and its note-off only when the last of them lets go. Otherwise
one engine's arpeggio would keep cutting a note the other engine was still
holding.

### Arpeggiating the sequence

The arp and the sequencer are **not** alternatives. Switch the arp on over a
running pattern and each track's chord becomes the figure that track
arpeggiates — so you can sequence chords and turn them into arpeggios with
one switch, which is the fastest way to get a moving line out of a static
progression. Every track walks its own figure on its own engine, and your own
playing still arpeggiates over the top of all of it.

Two rules keep the switch from wrecking what is already there:

- **A figure needs more than one note.** A single-note step — a bass line, a
  kick pattern — plays exactly as it does with the arp off, held steps and
  all, so breaking up the chords does not also start chopping the bass.
  Octaves count, so OCT above 1 does give a one-note step a figure.
- **The figure walks on across steps** rather than restarting on each one.
  At x1 there is only one note per step, so restarting would play the same
  note of the chord every time and never actually arpeggiate anything.

A sequenced step splits by engine exactly as a live note does: the cards
whose arp is on walk the chord, the others hold it as a block, and both
halves can be sounding at once. So a Layer patch with only **CS** armed has
the JP-8 holding the sequenced chord while the CS-80 arpeggiates it. A track
set to a specific engine can only be reached by that engine's switch — the
JP-8's arp leaves a CS-80 track alone.

**ARP RATE** (`x1`–`x8`) is how many arp notes fit in one DIV step. At `x1` —
the default — the arp *is* the step clock, exactly as it always was. Above
that it subdivides, which is what lets a figure run *inside* a sequenced
chord instead of only moving when the chord does: hold a chord across four
steps at `x4` and you get sixteen notes over it. It applies to the
arpeggiator whether or not the sequencer is running.

**Tempo** is one BPM control next to the transport, against the DIV note
division in the ARP section: 1/16 at 124 BPM, not a rate in hertz. It drives
the arpeggiator, the sequencer and the synced delay alike. ARP **SYNC**
hands tempo over to the host, but only when the host actually reports one —
a standalone build has no host tempo, so the knob keeps working there.

A readout strip under the grid names the **scale** the pattern is in: every
root and scale family is scored against the pitch classes played, preferring
the tightest fit (a pentatonic beats the major scale that also contains it)
and taking the root from the note the pattern leans on, which is what
settles C major against A minor. A `~` prefix means the pattern spills
outside the closest scale, and the strip says how many notes did fit. The
right-hand side shows the step clock actually in force.

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
the list, the name button opens it, **SAVE** snapshots the current
sound. A preset is every parameter plus the per-key engine map and the
sequencer pattern — not the MIDI-learn map (that belongs to your hardware)
and not the hosted VST3 chain (reloading plugins on every switch would
stall). Files are `.eighty` XML in
`~/Library/Eighty/Presets`, and the name menu has a
"Reveal folder" item. An asterisk after the name means you have edited it
since it was loaded.

**Starting points** sit above your own presets in the list — four built-in
sounds that are a bare waveform and nothing else, for when you want to build
a sound rather than play one:

| | Engine | |
|---|---|---|
| **Plain Saw** | CS-80 | one oscillator, saw only |
| **Plain Pulse** | CS-80 | 25% duty — a notch on every fourth harmonic |
| **Plain Square** | JP-8 | VCO 1 only |
| **Plain Triangle** | JP-8 | VCO 1 only |

Saw and pulse come off the CS-80 card, which mixes its saw and pulse outputs
continuously; square and triangle come off the JP-8, whose VCO has them as
discrete waveform positions — which is also why they open on different
panels. On each one the filter is wide open and out of circuit, the envelope
is a flat gate, and velocity, aftertouch, drift, stereo spread and the
chorus are all turned *off* rather than merely left alone. They are defined
in `Source/FactoryPresets.h` as overrides on the parameter defaults, so
loading one resets everything else rather than leaving the last sound's
settings lying around, and being built in they cannot be overwritten or
deleted.

## Recording

**RECORD** in the header captures the output to a stereo 24-bit WAV; the
button becomes STOP and counts the take up beside it. Files land in
`~/Library/Eighty/Recordings` — alongside the presets — named for the patch
and the moment (`Eighty Brass Stack 2026-08-16 20-31-04.wav`), so a folder of
takes is still navigable a week later and nothing ever overwrites anything.
Right-click the button to open the folder, or the last take.

The tap is at the very end of the chain — post FX, post the VST3 inserts,
post master volume and limiter — the same point the scopes read, so what
lands in the file is what you were hearing and watching rather than an
earlier, tidier version of it.

Writing to disk cannot happen on the audio thread, so a background thread
drains a lock-free queue to the file and the audio thread only ever takes a
lock it is allowed to lose. If the disk ever falls far enough behind that the
queue backs up, the missing samples are counted and reported when you stop —
a recording with a silent hole in it and nothing said about it would be worse
than one that admits the hole.

Deliberately **not** a parameter: it writes files, and nothing that writes
files should be reachable from host automation or restored by loading a
patch. A sample-rate change ends the take rather than corrupting it, and a
take still open when the plugin closes is finalised on the way out.

## Render quality (oversampling)

The **QUALITY** selector runs the whole voice pool at 2x, 4x or 8x the
sample rate and decimates back down through cascaded halfband filters. It is
there because filter drive, hard sync and cross-mod all generate harmonics
above Nyquist that otherwise fold back into the audible band as fizz. Both
filters' drive stages additionally use antiderivative anti-aliasing, which
costs one log-cosh and a divide instead of a filter bank.

Measured as energy below the fundamental — a bandlimited sawtooth has none,
so anything down there is folded:

| | CS-80, drive at max | JP-8, hard sync |
| --- | --- | --- |
| Off | −45.6 dB | −42.0 dB |
| 2x | −51.8 dB | −46.3 dB |
| 4x | −55.1 dB | −52.6 dB |
| 8x | −61.2 dB | −60.2 dB |

CPU scales roughly with the factor. 2x is the default. The decimation
filters are linear phase, so the latency they cost (0 / 16 / 23 / 27
samples) is reported to the host.

## Everything else

Hold latch, arpeggiator with an "As Played" mode, pitch wheel (sprung), full
MIDI learn (right-click any knob), delay / tremolo FX, output width control,
velocity sensitivity, master tune/volume.

**Output mixer** — a strip per sound source beside the plugin loader: the
CS-80 card, the JP-8 card and the hosted VST3 synth layer, each with a level
fader, mute and solo. Solo anywhere mutes everything not soloed, as on a
desk. The engine levels fold into the per-voice gains, which are smoothed at
control rate, so a mute is a fast fade rather than a click.

**Chorus** has three modes: **I** (wide and slow), **II** (faster and
deeper) and **ENS**. The wet path is darker than the dry, because a BBD is
a chain of sample-and-holds with a limited clock, and that roll-off is most
of why an analog chorus sits behind the sound instead of on top of it.

ENS is a string-machine ensemble rather than a chorus, and it is built not
to be polite about it: a slow, deep sweep with a ~5 Hz shimmer riding on top
(the fast modulator is the one you actually hear), one dominant tap per side
plus a quieter companion a third of a cycle behind it for the comb, and the
right channel running both modulators inverted so the image swings side to
side. The taps are deliberately lopsided — two equally weighted taps average
each other's movement away, which is how this mode used to end up sounding
like a slightly wider chorus. MIX also pulls the dry down harder here and
puts a louder wet in its place. Measured against mode II on a mono sine at
the panel defaults: 1.8x the stereo side energy and 1.6x the departure from
the dry signal, at the same output level.

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
across all keys, with its own strip in the mixer next door. It's
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
| `N` | toggle the Arp on both engines |
| `M` | sequencer play / stop |
| `R` | sequencer record |
| `←` / `→` | select previous / next control (click also selects) |
| `↑` / `↓` | adjust the selected control — hold to sweep it smoothly |
| `Shift+↑` / `Shift+↓` | pitch bend (springs back) |
| `Esc` | deselect |

Right-click any knob → **MIDI Learn**, then move a hardware control.
Mappings are saved with the plugin state.

Two more keys are held rather than pressed — `Q` and `I` — and belong to the
trackpad below.

## Trackpad

The trackpad is treated as a playing surface rather than a pointer. JUCE only
ever hands a plugin a mouse, so this reads AppKit's indirect-touch API
directly (`Source/Trackpad.mm`) to get every finger's position on the pad;
`Source/Gesture.h` turns those into gestures, and they reach the audio thread
through a lock-free queue like any other note. **macOS only** — everywhere
else the layer reports itself unavailable and nothing changes.

Everything lives behind a **held key**, so the trackpad is a mouse again the
moment you let go. While a gesture key is down the pointer is parked and
decoupled (a strum would otherwise fling it across the screen and off the
window, which would cut the touch stream off at its source) and stray clicks
are swallowed, so a stroke cannot land on a fader.

### Strum — hold `Q`

Sweep a finger across the pad and it strums the notes you are holding: one
string per note, spread over **SPAN** octaves. Arming damps whatever was
ringing, so the keyboard's job becomes saying *what* the chord is and the
hand on the pad decides when any of it is heard. A fast sweep sounds every
string it crossed rather than only where the finger landed, and stroke speed
drives velocity as far as **FORCE** asks it to.

**Turning around re-strikes the string you stopped on**, the way a pick does
when it changes direction — so you can run up the scale, park on the top
note, and come straight back down through it without that note being skipped.
Turning the *pattern* clock does not, because there one crossing has to stay
worth exactly one step.

- **Hold** (`B`) is the natural partner: latch a chord, let the keys go, and
  strum it with both hands free. Anything the strum leaves ringing when you
  come off `Q` stays up exactly as the Hold button would have kept it.
- **The arpeggiator** stands down while the pad is armed — they are both
  "play what is held", so they would only collide — and picks straight back
  up when you release. **The sequencer keeps running**, because playing over
  a pattern is the point of it.
- **STROKES PLAY → Pattern** hands the step clock to your hand instead: each
  crossing advances the arp or the sequencer by one step, in the direction
  you moved, and the note holds until the next stroke. Strum back down and
  the figure runs backwards.

**ARTICULATION** is what a stroke leaves behind. On `Hold` the strings ring
until something damps them. On `Pluck` each one is struck and let go as the
finger passes, so the patch's own release shapes the decay and nothing is
still holding the notes you already went past — which is what makes a run
work in both directions, and the reason the mode exists.

A note-off cannot simply follow the note-on, because the release segment
falls from wherever the envelope has got to and at that instant it has got
nowhere — the note would be silent. So a plucked string is gated just long
enough for its amp envelope to reach the top, then released there. The gate
is taken from the patch's own attack rather than fixed, so a slow-attacking
sound still gets to open before it is let go and a fast one is let go almost
at once. Nothing is held in `Pluck`, so **ON LIFT** has nothing to act on and
is greyed out.

**Lifting decides whether it rings or stops** (in `Hold`). On the default the
gesture does the deciding — lift mid-stroke and the strings ring on; rest
your finger on them first and they damp, which is what a hand on a real
instrument does. `Ring` and `Damp` pin it either way.

**Two fingers play the two engines.** The first finger down drives the CS-80
card and the second the JP-8, each with its own strings, its own ringing
notes and its own voice owner — so damping one hand does not cut the other's
strings, and a JP-8 lead can be strummed across a CS-80 pad. A single finger
follows the engine mode as usual, so one-handed strumming works everywhere.

The **cross axis** — the one you are *not* strumming along — rides velocity,
BRILLIANCE, or Force Touch pressure into channel aftertouch, which is the
thing the CS-80 is most famous for.

### Ribbon — hold `I`

The CS-80's ribbon controller was left out of this synth on purpose: it does
not map to a mouse or a computer keyboard. But a trackpad *is* a ribbon.
Holding `I` makes the pad one — slide for continuous pitch, ±**RANGE**
semitones, relative to wherever you put the finger down (or absolute, with
the middle of the pad at centre pitch). It springs back when you lift.

It is kept apart from the pitch wheel and it is **per engine**, so with two
fingers down one can slide the JP-8 layer against a CS-80 one that stays put
— the thing two layers can do that one cannot.

### Touch monitor

Off by default; **SETTINGS > Show touch monitor** turns it on, and it then
appears only while a gesture key is held — there is nothing to watch
otherwise, and it sits over the panel while it is up.

It draws the pad as the surface you are playing: the string grid with its
note names, where each finger is, which string it is standing on, and the
cross axis it is riding. Each finger takes its engine's colour — amber for
the CS-80 card, violet for the JP-8, teal when it is following the engine
mode — so with two fingers down, *which of us is on this string* is answered
at a glance. Beside it, a card per finger: the note it last struck and at
what velocity, its position in the run, and a dot per string filled where
that hand is still holding one, which is the picture of what Ring versus
Damp is actually doing. In Pattern mode the strings become numbered steps; on
the ribbon they become the reference the bend is measured from.

Everything in it is read back from the gesture engine and the voice pool
rather than recomputed, so it shows what was played rather than a second
opinion about it.

### Settings

**SETTINGS**, in the preset bar. The arm key (`Q`, Space or `` ` ``) and
whether it holds or latches; what strokes play and over how many octaves;
FORCE, articulation, the strum and cross axes, what lifting means, how two
fingers split;
the ribbon's range and origin; the pointer freeze and the touch monitor. They
are ordinary
parameters, so they save with the session and travel in a preset — a patch
you strum wants its strum set up the way you left it.

## Code map

- `Source/DSP/Voice.h` — one voice card, playable as CS-80 or JP-8 per note
- `Source/DSP/SynthEngine.h` — voice allocation, modes, hold, arp, step sequencer, split, global LFOs
- `Source/DSP/CS80Filter.h`, `JP8Filter.h`, `Oscillator.h`, `Envelope.h`, `LFO.h`, `Effects.h`
- `Source/DSP/Oversampling.h` — halfband decimator (coefficients designed at
  runtime from a windowed sinc) and the ADAA saturator
- `Source/PluginProcessor.*` — parameters, MIDI handling, MIDI learn, FX chain
- `Source/PluginEditor.*` — flat UI, scope, pitch wheel, computer-key handling
- `Source/Recorder.h` — threaded WAV capture of the final output
- `Source/Trackpad.h`, `Trackpad.mm`/`.cpp` — indirect (trackpad) touch capture
- `Source/Gesture.h` — touches to strum / ribbon, free of both JUCE and AppKit
