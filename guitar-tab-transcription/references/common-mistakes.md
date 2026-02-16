# Common Mistakes in Guitar Tab Transcription

Real examples from transcribing MCR "Teenagers" bass tab from video.

## Mistake 1: Missing Riff Repetitions

**Scenario**: The slide-7 riff (`r|s3f7[slide]|r|r|r|r|r(q)`) appeared at M9-M14 before verse 1. The same riff appeared again at M24-M28 before verse 2.

**Error**: Wrote dead notes (muted whole notes) for M24-M28 instead of the slide-7 riff.

**Root cause**: Did not recognize from the video frames that the same riff pattern was repeating. Defaulted to dead notes to fill the gap.

**Fix**: Always check if a riff heard earlier in the song recurs. In verse-chorus-verse structures, intro riffs commonly repeat before verse 2.

## Mistake 2: Chorus Used Only 2 of 4 Strings

**Scenario**: The chorus had ascending/descending scalar movement across all 4 bass strings:
- Ascending: A0-A4 (A string) → D2-D4 (D string)
- Descending: G2 (G string) → D4-D2 (D string) → A4 (A string)

**Error**: Wrote the chorus using only E and A strings with a simple alternating pattern like `A2|A2|E4|E4`.

**Root cause**: Could not read D string (s2) and G string (s1) fret numbers from small cropped video frames. Defaulted to the two lowest strings.

**Fix**: When a section sounds melodic or moves through a wide pitch range, expect notes across all strings. Bass choruses in rock songs often feature scalar runs that span the full range of the instrument.

## Mistake 3: Over-Padding with Pedal Tones

**Scenario**: Every verse section should have been exactly 6 measures:
- 4 chord measures (E-B-A-E)
- 1 B pedal measure
- 1 ending/transition measure

**Error**: Wrote 7 measures per verse by adding 2-3 bars of B pedal instead of 1, resulting in 6 extra measures across the whole song (70 instead of 64).

**Root cause**: When unsure about the exact measure count, added extra pedal tone bars as "safe" filler. This inflated every verse section by 1 measure.

**Fix**: Pedal tones in verse endings are typically 1 bar, not multiple. Verify verse section length is consistent throughout the song. If verse 1 is 6 measures, all verses should be 6 measures.

## Mistake 4: Same Chord Arpeggio for Every Verse Measure

**Scenario**: The verse follows an E-B-A-E chord progression:
- M1 (E): E0|E0|E4|E0|r|E0|E4|A2
- M2 (B): E7|E7|A6|E7|r|E7[slide]|A6|A9
- M3 (A): A0|A0|A4|A0|r|A0|A4|A7
- M4 (E): E0|E0|E4|E0|r|E0|E4|E0

**Error**: Used the same E-chord pattern (`r|E0|E4|A2|E0|E0|E4|E0`) for every verse measure.

**Root cause**: Identified the first verse measure's notes and assumed all measures followed the same chord. Did not recognize the chord changes.

**Fix**: Identify the chord progression first, then write each measure using the corresponding chord's tones. The root note, string positions, and fret numbers change with each chord.

## Mistake 5: Wrong Rest Position in Rhythm

**Scenario**: Correct verse rhythm has 8 eighth notes per measure with the rest at position 5:
`note|note|note|note|REST|note|note|note`

**Error**: Put the rest at position 1:
`REST|note|note|note|note|note|note|note`

**Root cause**: Misread the rhythm from the scrolling video tab. The visual scrolling made it hard to determine whether the first beat was a note or a rest.

**Fix**: Pay careful attention to the downbeat. In rock bass, the downbeat is almost always played. A rest on beat 1 is unusual and should be double-checked.

## Mistake 6: Invented Transition Measures

**Scenario**: Between verse 3 and the final verse, the song goes directly from a verse ending (`E0(q)|r(half.)`) into the next verse's E chord.

**Error**: Added an extra transition measure (`r(4.)|E0(8)|r(2)`) and a dead note measure between the sections.

**Root cause**: Assumed every section change needed a transition bar. In reality, rock songs often have tight, direct section changes.

**Fix**: Only write transition measures when clearly visible in the source material. Do not invent filler.

## Summary Table

| Mistake | Impact | Root Cause |
|---------|--------|------------|
| Missing riff repeat | Wrong notes for 5 bars | Didn't check for recurring patterns |
| Only 2 of 4 strings | Entire chorus wrong | Couldn't read higher string tab numbers |
| Extra pedal tones | +6 measures total | Padded with filler when unsure |
| Same chord every bar | Wrong notes for all verse bars | Didn't identify chord progression |
| Rest on beat 1 | Wrong rhythm feel | Misread scrolling tab timing |
| Invented transitions | +2 extra measures | Assumed sections need filler between them |
