# Omni-84 - Version: [1.2]

Date: 2024-03-23  
Name: Benjamin Dehli  
Profile: https://www.pianobook.co.uk/profile/benjamindehli

## Included formats

- Decent Sampler

## Release notes

### Version 1.2 (2024-03-23)

- Added "SonicStrings (AutoStrum)" preset

### Version 1.1 (2024-02-16)

- Added "Chords (Looped)" preset

### Version 1.0 (2023-04-06)

- First version released

## The Story

At the moment, the Omnichord is no longer produced and prices on the second-hand market have skyrocketed. This is my attempt to make the sound of the Omnichord more accessible.

## Description

Every sound of the Omnichord has been sampled into individual audio files.
I've also added the effects I tend to use when recording and playing the Omnichord as an impulse response called "space".

## Technical specification

- Sample rate: 48 kHz
- Bit depth: 24 bit
- Number of samples: 277
- File size for samples: 241.3 MB
- Channels: 1 (mono), 2 (stereo) for impulse response

## Instrument presets

This sample library includes 5 (plus 1 looped) presets, one for each part of the Omnichord. Drums, Bass, Chords, Keyboard and SonicStrings.
There's 2 presets for chords, a regular preset "Chords" (best in most cases) and the looped preset "Chords (Looped)". For the regular version, each sample are approximately 11 seconds long. For the looped version, to achieve the best possible loop point, some of the loops are short and some are longer. Some of the loops will however have some minor artifacts. For best audio quality, you should always use the regular version. And only if you have notes held for more than 11 seconds, use the the looped version.

### Drums

- Velocity
  - Toggle whether the velocity should affect the volume or not
- Keys
  - C1–F#1: Kick drum
  - G1–C2: Snare drum
  - C#2–F#2: Hi-hat
  - G2–A2: Clave

### Bass

- Release
  - How long the note sustains after the key is released

### Chords

- Keys
  - C1–B1: Major chords
  - C2–B2: Minor chords
  - C3–B3: 7th chords
  - C4–B4: Minor 7th chords
  - C5–B5: Major 7th chords
  - C6–B6: Diminished chords
  - C7–B7: Augmented chords

### Keyboard

- Voices
  - Monophonic
    - Plays only one note at the time, like the original Omnichord
  - Polyphonic
    - Plays multiple notes at the time

The keyboard samples loop after about 12 seconds to achieve a continuous sound. Each loop point is carefully picked and phase aligned to avoid audible artifacts.

### SonicStrings

- Voice 1 (vibrato voice)
  - Controls the volume of SonicStrings voice 1 samples
- Voice 2 (standard voice)
  - Controls the volume of SonicStrings voice 2 samples
- Sustain
  - The tone produced on the original Omnichord has the same length whether you keep your finger on the strum plate or release it. The length of the note is set by the control labeled sustain. To achieve the same behaviour in this plugin, the sustain knob affects both the decay time and the release time of the volume envelope.

### SonicStrings (AutoStrum)

The notes are strummed in the same order as the original Omnichord, but you have the possibility to make it strum in regular ascending order as well.  
The preset contains strum patterns for all the individual chords from the Suzuki Omnichord OM-84.
It has the same notes values for the chords as the Omni-84 'Chords' preset, which means you can send MIDI to both at the same time and get strummed SonicStrings on top of the chord sample.

- AutoStrum (speed)
  - Fast strum speed will sound like a chord and slow strum speed will sound like plucked notes.
- Strum pattern
  - Select between ascending/descending notes and regular/omnichord order
- Modulation Wheel
  - Controls the AutoStrum (speed)
- Key bindings
  - C0: Ascending notes in Omnichord order
  - D0: Descending notes in Omnichord order
  - E0: Ascending notes in regular order
  - F0: Descending notes in regular order
- Other controls are the same as on the regular "SonicStrings" preset
- Other key bindings are the same as on the "Chords" preset

## Effects

- Filter
  - Controls the cutoff frequency for the lowpass filter
- Space
  - Controls the reverb amount for the impulse response / convolution reverb

## Audio files

- 22 drum samples
- 19 bass samples
- 84 chord samples
- 32 keyboard samples
- 120 SonicStrings samples

## Impulse response (Space)

- Roland PA-120 preamp
- Fulltone Tube Tape Echo
- Roland PA-120 spring reverb
- Roland Dimension D SDD-320

The impulse signal was sent through a channel on the Roland PA-120 mixer so that the built-in spring reverb as well as a tape echo in the effects loop could be used. The preamplifier in the mixer also adds some nice saturation. The mixer and effects are all mono, so I ran it through a Dimension D to make it stereo and add some extra modulation.
