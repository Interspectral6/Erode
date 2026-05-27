# Erode Architecture

This document explains how Erode is structured for a developer who has not
worked on the project before.

## What Erode Does

Erode is a JUCE audio effect plugin inspired by Ableton's Erosion. It creates a
rough, gritty modulation effect by reading from a very short delay line while
moving the read position with a sine/noise modulator.

At a high level:

```text
input
  -> dry path
  -> short delay buffer write
       -> modulated read position
       -> high-pass filter
       -> wet path
       -> feedback path

dry + wet -> output
```

The modulator can behave more like a sine wave or more like filtered noise. The
`Width` parameter controls that morph.

## Main Files

`Source/PluginProcessor.h` and `Source/PluginProcessor.cpp`

Own the plugin parameters, state saving/loading, audio processing, delay buffer,
modulator, output high-pass filters, and FFT history buffers used by the UI.

`Source/PluginEditor.h` and `Source/PluginEditor.cpp`

Own the plugin window, five rotary sliders, labels, resizing behavior, and the
spectrum/filter display component.

`Source/NoiseFilterDisplay.h` and `Source/NoiseFilterDisplay.cpp`

Draw the live spectrum display. The dimmer trace is the input/dry signal and the
brighter trace is the output/wet signal. The blue band represents the current
modulation frequency and width. Dragging the band edits `Freq` and `Width`.

`Source/ErodeLookAndFeel.h` and `Source/ErodeLookAndFeel.cpp`

Define the dark UI palette and custom rotary slider drawing.

`CMakeLists.txt`

CMake build file. It defines JUCE location, plugin settings, source files,
formats, compile definitions, and linked JUCE modules.

## Parameters

Parameters live in `ErodeAudioProcessor::createParameterLayout()`.

`freq`

Display name: `Freq`. Range: 20 Hz to 20 kHz. This sets both the sine
modulator frequency and the center frequency of the noise band-pass filter.

`width`

Display name: `Width`. Range: 0 to 1. This controls two related things:

- Noise filter resonance/Q.
- Crossfade between sine modulation and noise modulation.

Low width means narrow/resonant/noise-light behavior, closer to sine modulation.
High width means wide/noisy behavior.

`amount`

Display name: `Amount`. Range: 0 to 1. This currently controls both wet/dry mix
and delay modulation depth. At 0, output is dry. At 1, output is fully wet and
the delay read offset moves the most.

`feedback`

Display name: `Feedback`. Range: 0 to 0.95. This returns the delayed signal to
the delay input. It is capped below 1.0 to reduce runaway gain.

`cut`

Display name: `Cut`. Range: 20 Hz to 20 kHz. This is the cutoff of the output
high-pass filter used to clean up low-frequency artifacts after the modulated
delay.

## Audio Signal Flow

The core DSP happens in `ErodeAudioProcessor::processBlock()`.

For each audio block:

1. Read current parameter values from `apvts`.
2. Smooth `amount` and `cut`.
3. Convert `width` into noise filter Q and sine/noise blend amounts.
4. Update the noise band-pass filter frequency/resonance.
5. Update output high-pass filter coefficients.
6. Process each sample.

For each sample:

1. Update smoothed `amount` and `cut`.
2. Generate white noise.
3. Run noise through the band-pass filter.
4. Generate sine from `lfoPhase`.
5. Crossfade between filtered noise and sine to create `offset`.
6. Use `offset * amount` to modulate the delay read position.
7. Interpolate between two delay samples for fractional delay reading.
8. High-pass the delayed signal.
9. Mix wet delayed signal with dry input.
10. Write input plus soft-clipped feedback back into the delay line.
11. Store mono input/output samples into FFT history buffers for the display.
12. Advance delay and FFT write positions.

The delay write includes feedback:

```text
delay write sample = input sample + tanh(delayed sample * feedback)
```

At `Feedback = 0`, this behaves like the original feed-forward design.

## Delay Line

`delayBuffer` stores recent samples for each audio channel. `writePosition`
points to the next location to write.

The current code allocates a short delay buffer:

```cpp
delayBuffer.setSize(getTotalNumOutputChannels(), 100);
```

The read position is based on:

```text
writePosition - fixedDelay + modulatedOffset
```

The fixed delay is currently:

```cpp
int delayInSamples = 30;
```

The modulated offset comes from the sine/noise blend. Because this read position
can land between samples, the code uses linear interpolation between `index0`
and `index1`.

The interpolated delayed sample does two jobs:

- It becomes the wet signal after output high-pass filtering.
- It feeds back into the delay input through `tanh(delayedSample * feedback)`.

## Modulator

The modulator is the heart of the erosion sound.

Noise side:

```text
white noise -> band-pass filter -> tanh soft clipping
```

Sine side:

```text
sin(lfoPhase)
```

Blend:

```text
offset = noiseAmount * noise + sineAmount * sine
```

`Width` decides the blend. Low width favors sine. High width favors noise.

## Wet/Dry Mixing

The dry input is scaled by:

```text
1 - amount
```

The wet delayed signal is scaled by:

```text
amount
```

Important: `amount` also scales modulation depth:

```text
delay read modulation depth = amount * 20
```

This means changing `Amount` affects both how much wet signal is heard and how
wildly the delay read position moves.

## Spectrum Display

The audio processor owns two mono circular buffers:

```cpp
inputBuffer
outputBuffer
```

During audio processing, the processor writes recent input and output samples
into those buffers. `NoiseFilterDisplay` reads them on a timer, runs FFTs, and
draws the spectrum.

This avoids using the live host audio buffer from the UI thread.

Display behavior:

- Input spectrum is drawn dimmer.
- Output spectrum is drawn brighter.
- Frequency axis is logarithmic from 20 Hz to 20 kHz.
- Magnitudes are converted to dB and clamped into a visible range.
- A small peak-hold decay keeps the trace readable.

## Dragging the Blue Band

The blue band in `NoiseFilterDisplay` is both visual feedback and an editor.

On mouse down:

1. The component checks if the click is inside the blue band.
2. If yes, it stores the starting mouse position, `freq`, and `width`.

On mouse drag:

1. Horizontal movement changes `freq` on a log scale.
2. Vertical movement changes `width`.
3. Parameter changes are sent through `setValueNotifyingHost()` so the host can
   see automation/gesture changes.

## UI Layout

The editor has a fixed 2.5:1 aspect ratio.

Top 40 percent:

```text
NoiseFilterDisplay
```

Bottom 60 percent:

```text
Freq | Width | Amount | Feedback | Cut
```

Each control is a rotary slider with a text box below it. Label and text box
sizes scale with window height.

## State And Automation

All parameters are owned by `juce::AudioProcessorValueTreeState`.

Slider attachments connect UI controls to APVTS parameters. This keeps UI,
automation, and saved state synchronized.

Preset/state save:

```cpp
getStateInformation()
```

Preset/state load:

```cpp
setStateInformation()
```

## Threading Notes

Audio thread:

- Runs `processBlock()`.
- Writes audio output.
- Writes FFT history buffers.

UI/message thread:

- Runs editor painting and mouse handling.
- Runs `NoiseFilterDisplay::timerCallback()`.
- Reads FFT history buffers.

The FFT write positions are atomic integers. The audio sample buffers themselves
are shared without explicit locking, which is common for simple visualizers but
means the display should be treated as approximate/diagnostic, not sample-exact.

## Important Gotchas

`amount` has two jobs.

It controls wet/dry mix and modulation depth. This is musically simple, but it
means mix changes also change the character/intensity of the delay modulation.

`width` has two jobs.

It controls noise filter Q and sine/noise blend. This keeps the UI simple, but
future changes should remember that `Width` is not only visual band width.

The delay now has feedback.

The original design was feed-forward only. The current implementation has
feedback:

```text
delay write sample = input sample + tanh(delayed sample * feedback)
```

`Feedback = 0` restores original behavior.

High-pass coefficients are updated very often.

The code updates high-pass coefficients inside the sample loop. This is simple
and tracks automation smoothly, but it may be more CPU-heavy than necessary.

The delay buffer is intentionally very short.

The allocation uses `100` samples. This keeps the effect in phase/pitch
modulation territory instead of echo territory.

## Feedback Design

Feedback is implemented in the per-channel sample loop inside `processBlock()`.
It uses the pre-mix delayed sample:

```text
delay write sample = input sample + delayed sample * feedback
```

The actual code soft-clips the feedback path:

```text
delay write sample = input sample + tanh(delayed sample * feedback)
```

Feedback is independent from `amount`, so changing wet/dry mix does not change
loop stability.

Expected sound:

- Low feedback: thicker erosion and longer smear.
- Medium feedback: metallic ringing, comb motion, flanger-like resonance.
- High feedback: unstable, harsh, self-oscillating textures.
