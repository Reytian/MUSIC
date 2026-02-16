---
name: guitar-tab-transcription
description: Guidelines for transcribing guitar/bass tabs from video into Guitar Pro files. Load when creating GP5 tabs from video footage or instructional videos with scrolling tablature.
---

# Guitar Tab Transcription from Video

## When to Use

Load this skill when:
- Transcribing guitar or bass tabs from video footage into Guitar Pro (.gp5) files
- Reading scrolling tablature from instructional or cover videos
- Using the guitar-pro-tabs MCP tool to write measures

## Core Principles

### 1. All Strings Must Be Checked

Never assume notes only appear on the lowest strings. Bass has 4 strings (E-A-D-G), guitar has 6. The chorus or bridge sections often use higher strings that are harder to read from video. When a section sounds melodic or scalar, expect notes across ALL strings, not just the root strings.

**Common mistake**: Writing a chorus using only E and A strings when it actually uses all 4 bass strings in ascending/descending scalar patterns.

### 2. Count Measures from the Source, Don't Pad

Each section has a specific measure count. Never add extra measures to "fill space." When unsure of the measure count:
- Count beats in the video carefully
- A repeating section (like B pedal) is usually 1 bar, not 2 or 3
- Verse sections tend to have a consistent length throughout the song (e.g., always 6 measures)
- Do not pad with extra pedal tone measures

**Common mistake**: Adding 3 bars of B pedal when the song only has 1, inflating the measure count.

### 3. Identify Repeating Riffs Across the Song

Riffs and motifs often appear multiple times in a song. Before writing dead notes or filler, check whether a previously identified riff repeats in that section.

**Common mistake**: Writing dead notes for a section that is actually a second occurrence of a slide-7 riff heard earlier in the song.

### 4. Verse Chord Progression

In rock songs, verse measures typically follow a chord progression where EACH measure is based on a DIFFERENT chord. Do not repeat the same chord arpeggio for every measure.

- Identify the chord progression first (e.g., E-B-A-E)
- Each measure should use notes from the corresponding chord
- The root note, 3rd, 5th, and chromatic approach notes change per chord

**Common mistake**: Using the same E-chord arpeggio (E0-E4-A2) for every verse measure instead of following the chord progression.

### 5. Rhythm Pattern Accuracy

Pay close attention to where rests fall within a measure. A rest on beat 1 vs beat 3 completely changes the feel.

- Note whether the measure STARTS with a note or a rest
- The "and" of beat 3 (position 5 of 8 eighth notes) is a common rest position in rock
- Do not default to starting every measure with a rest

**Common mistake**: Starting every verse measure with `r(8th)` when the correct pattern starts with a note and has the rest at position 5.

### 6. Transitions Between Sections

Do not invent transition measures that don't exist. Sections in rock songs often connect directly without filler. Only write a transition measure if clearly visible in the source.

**Common mistake**: Adding dead note measures or sparse transition bars between sections where none exist, inflating the total measure count.

## Verification Checklist

Before finalizing a transcription:

1. **Total measure count**: Compare against the video duration. At a known BPM and time signature, calculate the expected measure count: `measures = (duration_seconds * BPM) / (beats_per_measure * 60)`
2. **Section consistency**: All verse sections should have the same measure count
3. **String coverage**: Verify chorus/bridge sections use the correct strings (not just the lowest two)
4. **Riff occurrences**: Check if any riff appears more than once in the song
5. **Chord progression**: Verify each verse measure follows a different chord, not the same pattern repeated
6. **No padding**: Ensure no extra pedal tone or dead note measures were added as filler

## Reference

See `references/common-mistakes.md` for detailed examples from real transcription sessions.
