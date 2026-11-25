#!/usr/bin/env python3
# SPDX-License-Identifier: MPL-2.0
"""Prepare or embed CADENCE-1's compact, tuned Salamander Grand Piano sample bank."""
import argparse
import math
import struct
import subprocess
from pathlib import Path

RATE = 16_000
FRAMES = 23_200
ANCHORS = {
    27: "D#1", 36: "C2", 45: "A2", 51: "D#3", 54: "F#3",
    57: "A3", 60: "C4", 63: "D#4", 66: "F#4", 69: "A4",
    72: "C5", 75: "D#5", 78: "F#5", 81: "A5", 90: "F#6",
    99: "D#7", 108: "C8",
}
MAGIC = b"KSPIANO1"
ENTRY = struct.Struct("<Bh")


def measured_cents(frames, note):
    start = RATE * (30 if note >= 96 else 100) // 1000
    end = RATE * (260 if note >= 96 else 900) // 1000
    signal = frames[start:end]
    window = [
        sample * (0.5 - 0.5 * math.cos(math.tau * i / (len(signal) - 1)))
        for i, sample in enumerate(signal)
    ]
    expected = 440 * 2 ** ((note - 69) / 12)

    def power(cents):
        frequency = expected * 2 ** (cents / 1200)
        coefficient = 2 * math.cos(math.tau * frequency / RATE)
        first = second = 0.0
        for sample in window:
            first, second = sample + coefficient * first - second, first
        return first * first + second * second - coefficient * first * second

    offsets = list(range(-60, 61, 2))
    powers = [power(offset) for offset in offsets]
    peak = max(range(len(powers)), key=powers.__getitem__)
    if not 0 < peak < len(powers) - 1:
        raise ValueError(f"Could not measure a near-tuned fundamental for MIDI note {note}")
    left, center, right = powers[peak - 1 : peak + 2]
    fraction = (left - right) / (left - 2 * center + right)
    return round(100 * (offsets[peak] + fraction))


def mu_law(sample):
    magnitude = min(abs(sample), 32635) + 132
    exponent = magnitude.bit_length() - 8
    mantissa = (magnitude >> (exponent + 3)) & 15
    sign = 128 if sample < 0 else 0
    return (~(sign | (exponent << 4) | mantissa)) & 255


def sample_entry(path, note):
    raw = subprocess.check_output([
        "ffmpeg", "-hide_banner", "-loglevel", "error", "-i", str(path),
        "-af", "pan=mono|c0=0.5*c0+0.5*c1", "-ar", str(RATE), "-t", "1.8",
        "-f", "s16le", "pipe:1",
    ])
    samples = struct.unpack(f"<{len(raw) // 2}h", raw)
    peak = max(map(abs, samples))
    onset = next(i for i, sample in enumerate(samples) if abs(sample) > peak * 0.012)
    frames = list(samples[max(onset - 8, 0) : max(onset - 8, 0) + FRAMES])
    frames.extend([0] * (FRAMES - len(frames)))
    tuning = measured_cents(frames, note)
    gain = 29_000 / max(map(abs, frames))
    tail = RATE * 180 // 1000
    for i in range(FRAMES):
        attack = min(i / 20, 1)
        remaining = min((FRAMES - i - 1) / tail, 1)
        fade = 0.5 - 0.5 * math.cos(math.pi * remaining)
        frames[i] = mu_law(round(frames[i] * gain * attack * fade))
    return tuning, bytes(frames)


def prepare(source, output):
    data = bytearray(MAGIC)
    for note, name in ANCHORS.items():
        tuning, frames = sample_entry(source / f"{name}v9.flac", note)
        data.extend(ENTRY.pack(note, tuning))
        data.extend(frames)
        print(f"{name}: {tuning / 100:+.2f} cents")
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_bytes(data)


def decode(value):
    code = (~value) & 255
    magnitude = ((code & 15) * 8 + 132) << ((code >> 4) & 7)
    return 132 - magnitude if code & 128 else magnitude - 132


def c_rows(values, indent, count):
    for i in range(0, len(values), count):
        yield indent + ", ".join(map(str, values[i : i + count])) + ","


def embed(asset, output):
    raw = asset.read_bytes()
    if not raw.startswith(MAGIC) or len(raw) != len(MAGIC) + (ENTRY.size + FRAMES) * len(ANCHORS):
        raise ValueError(f"{asset}: invalid CADENCE-1 piano bank")
    lines = [
        "/* Generated from the compact piano bank. See dist/licenses/Salamander-Attribution.md. */",
        f"enum {{ PIANO_RATE = {RATE}, PIANO_FRAMES = {FRAMES}, PIANO_ANCHORS = {len(ANCHORS)} }};",
        "static const int16_t piano_decode[256] = {",
        *c_rows([decode(i) for i in range(256)], "    ", 12),
        "};",
        "static const struct {",
        "    uint8_t note;",
        "    int16_t cents_x100;",
        "    uint8_t samples[PIANO_FRAMES];",
        "} piano_anchors[PIANO_ANCHORS] = {",
    ]
    offset = len(MAGIC)
    for expected in ANCHORS:
        note, tuning = ENTRY.unpack_from(raw, offset)
        if note != expected or not -6000 <= tuning <= 6000:
            raise ValueError(f"{asset}: invalid sample metadata for MIDI note {expected}")
        offset += ENTRY.size
        lines.append(f"    {{{note}, {tuning}, {{")
        lines.extend(c_rows(raw[offset : offset + FRAMES], "        ", 24))
        lines.append("    }},")
        offset += FRAMES
    lines.extend(["};", ""])
    output.write_text("\n".join(lines), encoding="ascii")
    print(f"piano bank: {len(ANCHORS)} sampled notes, {len(raw):,} bytes")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("mode", choices=["prepare", "embed"])
    parser.add_argument("input", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    if args.mode == "prepare":
        prepare(args.input, args.output)
    else:
        embed(args.input, args.output)


if __name__ == "__main__":
    main()
