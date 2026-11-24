# CE 2600

A semi-modular synthesizer for VCV Rack 2, in a black-and-orange 2600-style panel. It comes as three 112 HP modules, **2600 Orange — Top**, **Middle** and **Bottom**. Each is one standard Rack row high, and stacked together they make one three-row instrument.

Every control sits where it does on the Behringer 2600 front panel. The print is original vector artwork: orange section titles, a labelled box under each normalled jack, LED-coloured slider caps, scales and signal lines. The Cloverdale Engineering logo replaces the maker's. The top band is compressed vertically to fit one Rack row; the two lower bands are close to true proportion. Rack's **View → CPU meter** draws over the bottom 21 px of every module, so that strip carries only signal lines.

This is not an official ARP, Korg or Behringer product, and the DSP is not a component-level circuit model.

## Download

Get the latest `.vcvplugin` for your computer from the repository's **Releases** page: `win-x64` for Windows, `mac-arm64` for Apple Silicon Macs, `mac-x64` for Intel Macs, `lin-x64` for Linux. In Rack, choose **Help > Open user folder**, put the file in the `plugins-<platform>` folder and restart Rack.

## Insert the instrument

To insert all three panels at once, choose Rack's **File > Import selection** and select `CE2600 Orange - Three panels.vcvs`. It is in the plugin's `res` folder, and `make install` also copies it to your Rack selections folder. You can also open `res/CE2600 Orange - Three panels.vcv` as a separate patch.

Alternatively, search the module browser for **2600 Orange — Top**, **Middle** and **Bottom**, then stack them with their left edges aligned.

Connect pitch to **KBD IN** at the far left of the bottom row and gate to **GATE IN** beside **TRIG IN** on the middle row. Take audio from **L OUTPUT** and **R OUTPUT** at top right, or from the 2-channel headphone jack on the bottom row. Hold the red **MANUAL** button in the AR section to audition. Start at a low listening level.

## Instrument links

The panels share a private signal bus inside the current Rack patch. No link cables are needed, and the panels may be on different Rack rows. All three default to **Instrument link 1**. A panel only shows a link message when it is waiting for the others or is a duplicate.

Top runs the DSP. Middle and Bottom supply their controls and patch inputs and receive their output signals. Remote signals can arrive with a small scheduling-dependent sample delay.

For a second instrument, right-click each of its three panels and choose **Instrument link > 2**. Up to 16 groups are supported, and every group needs exactly one Top, one Middle and one Bottom. A duplicate panel reports DUPLICATE PANEL and stays silent instead of taking over another panel's controls. Removing or bypassing Top silences the remote outputs. A missing lower panel falls back to default controls and disconnected inputs. Link assignments save with the patch. Separate Rack instances never share buses.

## Controls and patching

Hover any control or jack for its full name and units. Patching an input replaces its printed internal source, even if the cable carries zero volts.

- **VCOs:** three oscillators with coarse and fine tuning, AUDIO/LF range (LF turns keyboard tracking off), pulse width on all three, and VCO 2 and 3 hard sync. The FM and PWM sliders work as attenuators: no modulation at the bottom, more as the slider rises. Pitch follows 1 V/oct.
- **Filter:** five-input audio mixer, coarse and fine cutoff, resonance, keyboard tracking, ADSR and VCO 2 modulation, and a mode switch. The filter modes share one digital filter core with different cutoff responses.
- **Envelopes:** 0–10 V ADSR and AR, with separate time factors. The gate source switch selects the keyboard gate, the S/H gate (normalled to the internal clock) or the trigger. The ADSR's own **GATE IN** overrides the gate for the ADSR only.
- **VCA:** VCF and ring audio inputs, initial gain, AR and ADSR control.
- **Mixer and outputs:** mixer with two direct inputs above its sliders, pan, stereo reverb, left and right inputs, **L OUTPUT** and **R OUTPUT**, and 3.5 mm jacks beside them that carry the same signals. The POWER rocker mutes the audio outputs.
- **Keyboard section:** portamento, vibrato with delay, trigger repeat and Duo mode. Set **VOICE MODE** to Duo and patch **UPPER IN** for a second pitch on VCO 3; the two pitches share the filter and envelopes. The hardware takes pitch over MIDI, so **KBD IN** and **UPPER IN** are jacks added to the panel.
- **Utilities:** preamp with range switch, envelope follower, AC/DC ring modulator, noise with level, LFO (the first output is a triangle), two inverting voltage processors, lag, a four-jack multiple, sample and hold, and the electronic switch. Its LEDs show which input is selected. A and B are inputs and C is the output, because Rack ports are directional.
- **Headphones:** level knob and a 2-channel jack carrying left and right on one cable.

Sync, the filter modes and the reverb are software approximations. Rack CV replaces the hardware keyboard and the rear MIDI/USB, pedal and power connections. The instrument is monophonic (Duo aside) and reads the first channel of polyphonic cables.

## Build and verify

Requires a Rack 2 SDK, C++ compiler, Make, jq and zstd.

```sh
make RACK_DIR=/path/to/Rack-SDK -j4
make RACK_DIR=/path/to/Rack-SDK test
make RACK_DIR=/path/to/Rack-SDK install
```

Panel artwork and generated layouts are checked in. `tools/make_three_panel.py` draws the panel (`tools/orange_layout.py` holds the layout) and writes the three panel SVGs, widget layouts, ownership map and control artwork. It needs Python with fontTools, and Inter Bold, Inter Bold Italic and Archivo Narrow Bold (all SIL OFL) in `~/Library/Fonts`. Text is converted to outlines, so the plugin does not depend on the fonts. `tools/make_presets.py` writes the ready-stacked selection and patch.

The tests run real module process callbacks: tuning, envelopes, filter stability, patch overrides, S&H and reverb at 44.1, 48 and 96 kHz, three-panel signal and control transport, removal and reconnection, duplicate protection, link persistence, bypass, isolation between Rack contexts, and the Orange features. They also render `build/ce-2600-demo.wav`.

## License

Copyright (c) 2026 Cloverdale Engineering. This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version. See `LICENSE`.

The Cloverdale Engineering name and logo are trademarks of Cloverdale Engineering and are not licensed under the GPL. ARP is a trademark of Korg. Behringer is a trademark of its respective owner. CE 2600 is an independent work and is not affiliated with or endorsed by Korg or Behringer.
