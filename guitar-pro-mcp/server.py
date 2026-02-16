#!/usr/bin/env python3
"""Guitar Pro MCP Server - Create and edit Guitar Pro tab files (.gp5).

Provides tools to create, read, and modify Guitar Pro tablature files
that can be opened in Guitar Pro 8.
"""

import os
import json
from typing import Optional

import guitarpro
from guitarpro import models
from mcp.server.fastmcp import FastMCP

server = FastMCP(
    "guitar-pro-tabs",
    instructions="Create and edit Guitar Pro tab files (.gp5). Files can be opened in Guitar Pro 8.",
)

DEFAULT_DIR = os.path.expanduser("~/Documents/Vibe Code/guitar-tabs")

# Standard tunings: list of MIDI note values, high string first
# String 1 = highest (thinnest), last = lowest (thickest)
TUNINGS = {
    "standard":         [64, 59, 55, 50, 45, 40],  # E4 B3 G3 D3 A2 E2
    "drop_d":           [64, 59, 55, 50, 45, 38],  # E4 B3 G3 D3 A2 D2
    "open_g":           [62, 59, 55, 50, 43, 38],  # D4 B3 G3 D3 G2 D2
    "open_d":           [62, 57, 54, 50, 45, 38],  # D4 A3 F#3 D3 A2 D2
    "open_e":           [64, 59, 56, 52, 47, 40],  # E4 B3 G#3 E3 B2 E2
    "open_a":           [64, 61, 57, 52, 45, 40],  # E4 C#4 A3 E3 A2 E2
    "dadgad":           [62, 57, 55, 50, 45, 38],  # D4 A3 G3 D3 A2 D2
    "half_step_down":   [63, 58, 54, 49, 44, 39],  # Eb4 Bb3 Gb3 Db3 Ab2 Eb2
    "whole_step_down":  [62, 57, 53, 48, 43, 38],  # D4 A3 F3 C3 G2 D2
    "7string_standard": [64, 59, 55, 50, 45, 40, 35],  # + B1
    "bass_standard":    [43, 38, 33, 28],           # G2 D2 A1 E1
    "bass_drop_d":      [43, 38, 33, 26],           # G2 D2 A1 D1
    "bass_5_standard":  [43, 38, 33, 28, 23],       # G2 D2 A1 E1 B0
}

# MIDI program numbers for instruments
INSTRUMENTS = {
    "nylon": 24,
    "acoustic": 25,
    "jazz": 26,
    "clean": 27,
    "muted": 28,
    "overdrive": 29,
    "distortion": 30,
    "harmonics": 31,
    "bass_finger": 33,
    "bass_pick": 34,
    "bass_fretless": 35,
    "bass_slap": 36,
}

DURATION_MAP = {
    "whole": 1,
    "half": 2,
    "quarter": 4,
    "eighth": 8,
    "8th": 8,
    "16th": 16,
    "32nd": 32,
    "64th": 64,
}

QUARTER_TIME = 960  # ticks per quarter note


def resolve_path(filepath: str) -> str:
    """Resolve filepath, defaulting to guitar-tabs directory."""
    filepath = os.path.expanduser(filepath)
    if not os.path.isabs(filepath):
        filepath = os.path.join(DEFAULT_DIR, filepath)
    if not filepath.endswith((".gp5", ".gp4", ".gp3")):
        filepath += ".gp5"
    return filepath


def measure_length(time_sig_num: int = 4, time_sig_den: int = 4) -> int:
    """Calculate measure length in ticks."""
    return int(time_sig_num * QUARTER_TIME * 4 / time_sig_den)


def build_beat(voice, beat_data: dict, start: int) -> models.Beat:
    """Create a Beat from a dict specification."""
    dur_name = beat_data.get("duration", "quarter")
    dur_value = DURATION_MAP.get(dur_name, 4)
    dotted = beat_data.get("dotted", False)

    duration = models.Duration(value=dur_value, isDotted=dotted)
    beat = models.Beat(voice, status=models.BeatStatus.normal, start=start)
    beat.duration = duration

    if beat_data.get("rest", False):
        beat.status = models.BeatStatus.rest
        return beat

    for note_data in beat_data.get("notes", []):
        note = models.Note(
            beat,
            value=note_data["fret"],
            string=note_data["string"],
            type=models.NoteType.normal,
            velocity=note_data.get("velocity", 95),
        )

        # Note effects
        if note_data.get("hammer"):
            note.effect.hammer = True
        if note_data.get("vibrato"):
            note.effect.vibrato = True
        if note_data.get("palm_mute"):
            note.effect.palmMute = True
        if note_data.get("let_ring"):
            note.effect.letRing = True
        if note_data.get("staccato"):
            note.effect.staccato = True
        if note_data.get("accent"):
            note.effect.accentuatedNote = True
        if note_data.get("ghost"):
            note.effect.ghostNote = True
        if note_data.get("dead"):
            note.type = models.NoteType.dead
        if note_data.get("tie"):
            note.type = models.NoteType.tie

        # Slide
        slide = note_data.get("slide")
        if slide:
            slide_map = {
                "shift": models.SlideType.shiftSlideTo,
                "legato": models.SlideType.legatoSlideTo,
                "down": models.SlideType.outDownwards,
                "up": models.SlideType.outUpwards,
                "into_above": models.SlideType.intoFromAbove,
                "into_below": models.SlideType.intoFromBelow,
            }
            note.effect.slides = [slide_map.get(slide, models.SlideType.shiftSlideTo)]

        # Bend (value in semitones: 1=half step, 2=whole step, etc.)
        bend_val = note_data.get("bend")
        if bend_val is not None:
            bend = models.BendEffect()
            bend.type = models.BendType.bend
            bend.value = bend_val
            bend.points = [
                models.BendPoint(position=0, value=0),
                models.BendPoint(position=6, value=bend_val),
                models.BendPoint(position=12, value=bend_val),
            ]
            note.effect.bend = bend

        # Bend-release
        bend_release = note_data.get("bend_release")
        if bend_release is not None:
            bend = models.BendEffect()
            bend.type = models.BendType.bendRelease
            bend.value = bend_release
            bend.points = [
                models.BendPoint(position=0, value=0),
                models.BendPoint(position=4, value=bend_release),
                models.BendPoint(position=8, value=bend_release),
                models.BendPoint(position=12, value=0),
            ]
            note.effect.bend = bend

        beat.notes.append(note)

    return beat


@server.tool()
def create_song(
    filepath: str,
    title: str = "Untitled",
    artist: str = "",
    tempo: int = 120,
    time_sig_num: int = 4,
    time_sig_den: int = 4,
    num_measures: int = 16,
    track_name: str = "Guitar",
    instrument: str = "clean",
    tuning: str = "standard",
) -> str:
    """Create a new Guitar Pro (.gp5) tab file with one track.

    Args:
        filepath: File path (relative to ~/Documents/Vibe Code/guitar-tabs/ or absolute)
        title: Song title
        artist: Artist name
        tempo: BPM (beats per minute)
        time_sig_num: Time signature numerator (e.g. 4 for 4/4)
        time_sig_den: Time signature denominator (e.g. 4 for 4/4)
        num_measures: Number of empty measures
        track_name: Name for the first track
        instrument: nylon, acoustic, jazz, clean, muted, overdrive, distortion, harmonics, bass_finger, bass_pick, bass_fretless, bass_slap
        tuning: standard, drop_d, open_g, open_d, open_e, open_a, dadgad, half_step_down, whole_step_down, 7string_standard, bass_standard, bass_drop_d, bass_5_standard
    """
    filepath = resolve_path(filepath)
    os.makedirs(os.path.dirname(filepath), exist_ok=True)

    song = models.Song()
    song.title = title
    song.artist = artist
    song.tempo = tempo

    # Configure the default first header
    h0 = song.measureHeaders[0]
    h0.timeSignature.numerator = time_sig_num
    h0.timeSignature.denominator = models.Duration(value=time_sig_den)

    # Add more measure headers
    ml = measure_length(time_sig_num, time_sig_den)
    for i in range(1, num_measures):
        header = models.MeasureHeader(
            number=i + 1,
            start=QUARTER_TIME + i * ml,
        )
        header.timeSignature.numerator = time_sig_num
        header.timeSignature.denominator = models.Duration(value=time_sig_den)
        song.measureHeaders.append(header)

    # Configure the default track
    track = song.tracks[0]
    track.name = track_name
    track.channel.instrument = INSTRUMENTS.get(instrument, 27)

    tuning_values = TUNINGS.get(tuning, TUNINGS["standard"])
    track.strings = [
        models.GuitarString(i + 1, v) for i, v in enumerate(tuning_values)
    ]

    # Add measures to match headers
    for i in range(1, num_measures):
        measure = models.Measure(track, song.measureHeaders[i])
        track.measures.append(measure)

    guitarpro.write(song, filepath)

    return (
        f"Created: {filepath}\n"
        f"{title} by {artist}\n"
        f"{num_measures} measures, {tempo} BPM, {time_sig_num}/{time_sig_den}\n"
        f"Track 1: {track_name} ({instrument}, {tuning} tuning, {len(tuning_values)} strings)"
    )


@server.tool()
def write_measures(
    filepath: str,
    track_number: int,
    start_measure: int,
    measures: str,
) -> str:
    """Write notes into measures of a Guitar Pro file.

    Args:
        filepath: Path to the .gp5 file
        track_number: Track number (1-based)
        start_measure: Starting measure number (1-based)
        measures: JSON array of measures. Each measure has a "beats" array.
            Each beat: {"notes": [{"string": 1-7, "fret": 0-24, ...}], "duration": "quarter", "dotted": false, "rest": false}
            Durations: whole, half, quarter, eighth/8th, 16th, 32nd, 64th
            String 1 = highest/thinnest, 6 = lowest/thickest. Fret 0 = open.
            Note effects (optional): hammer (bool), vibrato (bool), palm_mute (bool),
            let_ring (bool), staccato (bool), accent (bool), ghost (bool), dead (bool), tie (bool),
            slide ("shift"/"legato"/"down"/"up"), bend (semitones: 1=half, 2=whole), bend_release (semitones).
            Example: [{"beats":[{"notes":[{"string":6,"fret":0},{"string":5,"fret":2}],"duration":"quarter"}]}]
    """
    filepath = resolve_path(filepath)
    song = guitarpro.parse(filepath)

    if track_number < 1 or track_number > len(song.tracks):
        return f"Error: Track {track_number} not found. Song has {len(song.tracks)} track(s)."

    track = song.tracks[track_number - 1]
    measures_data = json.loads(measures)

    # Extend song if we need more measures
    while start_measure + len(measures_data) - 1 > len(song.measureHeaders):
        last_header = song.measureHeaders[-1]
        ts = last_header.timeSignature
        ml = measure_length(ts.numerator, ts.denominator.value)
        new_header = models.MeasureHeader(
            number=len(song.measureHeaders) + 1,
            start=last_header.start + ml,
        )
        new_header.timeSignature.numerator = ts.numerator
        new_header.timeSignature.denominator = models.Duration(value=ts.denominator.value)
        song.measureHeaders.append(new_header)

        for t in song.tracks:
            m = models.Measure(t, new_header)
            t.measures.append(m)

    # Write notes into each measure
    for i, measure_data in enumerate(measures_data):
        measure_idx = start_measure - 1 + i
        measure = track.measures[measure_idx]

        voice = measure.voices[0]
        voice.beats = []

        current_start = measure.start
        for beat_data in measure_data.get("beats", []):
            beat = build_beat(voice, beat_data, current_start)
            voice.beats.append(beat)
            current_start += beat.duration.time

    guitarpro.write(song, filepath)

    return (
        f"Wrote {len(measures_data)} measure(s) starting at measure {start_measure} "
        f"on track {track_number} in {filepath}"
    )


@server.tool()
def add_track(
    filepath: str,
    name: str = "Guitar",
    instrument: str = "clean",
    tuning: str = "standard",
) -> str:
    """Add a new track to an existing Guitar Pro file.

    Args:
        filepath: Path to the .gp5 file
        name: Track name
        instrument: nylon, acoustic, jazz, clean, muted, overdrive, distortion, harmonics, bass_finger, bass_pick, bass_fretless, bass_slap
        tuning: standard, drop_d, open_g, open_d, open_e, open_a, dadgad, half_step_down, whole_step_down, 7string_standard, bass_standard, bass_drop_d, bass_5_standard
    """
    filepath = resolve_path(filepath)
    song = guitarpro.parse(filepath)

    # Find unused MIDI channels (skip 9 = drums)
    used = set()
    for t in song.tracks:
        used.add(t.channel.channel)
        used.add(t.channel.effectChannel)

    ch = None
    ech = None
    for c in range(16):
        if c == 9:
            continue
        if c not in used:
            if ch is None:
                ch = c
            elif ech is None:
                ech = c
                break
    if ch is None:
        ch = 0
    if ech is None:
        ech = min(ch + 1, 15)

    tuning_values = TUNINGS.get(tuning, TUNINGS["standard"])

    track = models.Track(
        song,
        number=len(song.tracks) + 1,
        name=name,
        strings=[models.GuitarString(i + 1, v) for i, v in enumerate(tuning_values)],
        channel=models.MidiChannel(
            channel=ch,
            effectChannel=ech,
            instrument=INSTRUMENTS.get(instrument, 27),
        ),
    )

    for header in song.measureHeaders:
        measure = models.Measure(track, header)
        track.measures.append(measure)

    song.tracks.append(track)
    guitarpro.write(song, filepath)

    track_num = len(song.tracks)
    return (
        f"Added track {track_num}: {name} ({instrument}, {tuning} tuning, "
        f"{len(tuning_values)} strings) to {filepath}"
    )


@server.tool()
def read_song(filepath: str) -> str:
    """Read and display info about a Guitar Pro file, including notes.

    Args:
        filepath: Path to the .gp5 file
    """
    filepath = resolve_path(filepath)
    song = guitarpro.parse(filepath)

    note_names = ["C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"]

    lines = [
        f"File: {filepath}",
        f"Title: {song.title}",
        f"Artist: {song.artist}",
        f"Tempo: {song.tempo} BPM",
        f"Measures: {len(song.measureHeaders)}",
        f"Tracks: {len(song.tracks)}",
        "",
    ]

    for ti, track in enumerate(song.tracks):
        tuning_str = " ".join(
            f"{note_names[s.value % 12]}{s.value // 12 - 1}"
            for s in track.strings
        )
        lines.append(f"Track {ti + 1}: {track.name}")
        lines.append(f"  Instrument: {track.channel.instrument}")
        lines.append(f"  Strings: {len(track.strings)} ({tuning_str})")

        non_empty = 0
        for measure in track.measures:
            voice = measure.voices[0]
            if any(b.notes for b in voice.beats):
                non_empty += 1
        lines.append(f"  Measures with notes: {non_empty}/{len(track.measures)}")

        # Show first few measures with notes
        shown = 0
        for mi, measure in enumerate(track.measures):
            voice = measure.voices[0]
            if not voice.beats or not any(b.notes for b in voice.beats):
                continue
            if shown >= 8:
                lines.append(f"  ... ({non_empty - shown} more measures with notes)")
                break

            beat_strs = []
            for beat in voice.beats:
                if beat.status == models.BeatStatus.rest:
                    beat_strs.append(f"rest({beat.duration.value})")
                elif beat.notes:
                    notes = ",".join(
                        f"s{n.string}f{n.value}" for n in beat.notes
                    )
                    beat_strs.append(f"{notes}({beat.duration.value})")
            lines.append(f"  M{mi + 1}: {' | '.join(beat_strs)}")
            shown += 1

        lines.append("")

    return "\n".join(lines)


@server.tool()
def list_files(directory: str = "") -> str:
    """List Guitar Pro files in a directory.

    Args:
        directory: Directory path (defaults to ~/Documents/Vibe Code/guitar-tabs/)
    """
    if not directory:
        directory = DEFAULT_DIR
    else:
        directory = os.path.expanduser(directory)

    if not os.path.isdir(directory):
        return f"No Guitar Pro files yet. Directory will be created when you create a song."

    files = []
    for f in sorted(os.listdir(directory)):
        if f.endswith((".gp5", ".gp4", ".gp3", ".gp", ".gpx")):
            full_path = os.path.join(directory, f)
            size_kb = os.path.getsize(full_path) / 1024
            files.append(f"  {f} ({size_kb:.1f} KB)")

    if not files:
        return f"No Guitar Pro files found in {directory}"

    return f"Guitar Pro files in {directory}:\n" + "\n".join(files)


@server.tool()
def set_tempo(
    filepath: str,
    tempo: int,
) -> str:
    """Change the tempo of a Guitar Pro file.

    Args:
        filepath: Path to the .gp5 file
        tempo: New BPM value
    """
    filepath = resolve_path(filepath)
    song = guitarpro.parse(filepath)
    song.tempo = tempo
    guitarpro.write(song, filepath)
    return f"Set tempo to {tempo} BPM in {filepath}"


@server.tool()
def duplicate_measures(
    filepath: str,
    track_number: int,
    source_start: int,
    source_end: int,
    dest_start: int,
) -> str:
    """Copy measures from one location to another within a track.

    Args:
        filepath: Path to the .gp5 file
        track_number: Track number (1-based)
        source_start: First measure to copy (1-based)
        source_end: Last measure to copy (1-based, inclusive)
        dest_start: Destination measure number (1-based)
    """
    filepath = resolve_path(filepath)
    song = guitarpro.parse(filepath)

    if track_number < 1 or track_number > len(song.tracks):
        return f"Error: Track {track_number} not found."

    track = song.tracks[track_number - 1]
    count = source_end - source_start + 1

    # Extend if needed
    while dest_start + count - 1 > len(song.measureHeaders):
        last_header = song.measureHeaders[-1]
        ts = last_header.timeSignature
        ml = measure_length(ts.numerator, ts.denominator.value)
        new_header = models.MeasureHeader(
            number=len(song.measureHeaders) + 1,
            start=last_header.start + ml,
        )
        new_header.timeSignature.numerator = ts.numerator
        new_header.timeSignature.denominator = models.Duration(value=ts.denominator.value)
        song.measureHeaders.append(new_header)
        for t in song.tracks:
            m = models.Measure(t, new_header)
            t.measures.append(m)

    # Copy beats from source to dest
    for i in range(count):
        src_measure = track.measures[source_start - 1 + i]
        dst_measure = track.measures[dest_start - 1 + i]

        src_voice = src_measure.voices[0]
        dst_voice = dst_measure.voices[0]

        dst_voice.beats = []
        offset = dst_measure.start - src_measure.start

        for src_beat in src_voice.beats:
            beat = models.Beat(
                dst_voice,
                status=src_beat.status,
                start=src_beat.start + offset,
            )
            beat.duration = models.Duration(
                value=src_beat.duration.value,
                isDotted=src_beat.duration.isDotted,
            )
            for src_note in src_beat.notes:
                note = models.Note(
                    beat,
                    value=src_note.value,
                    string=src_note.string,
                    type=src_note.type,
                    velocity=src_note.velocity,
                )
                note.effect = src_note.effect
                beat.notes.append(note)
            dst_voice.beats.append(beat)

    guitarpro.write(song, filepath)
    return (
        f"Copied measures {source_start}-{source_end} to {dest_start}-{dest_start + count - 1} "
        f"on track {track_number}"
    )


if __name__ == "__main__":
    server.run()
