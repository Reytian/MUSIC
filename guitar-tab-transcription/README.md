# Guitar Tab Transcription Skill

A Claude Code skill with guidelines for transcribing guitar/bass tabs from video footage into Guitar Pro (.gp5) files.

## What It Does

Provides 6 core rules that prevent common transcription errors when reading scrolling tablature from instructional or cover videos:

1. **All strings must be checked** - Don't assume notes only appear on the lowest strings
2. **Count measures from the source** - Don't pad with extra measures
3. **Identify repeating riffs** - Riffs often recur across sections
4. **Verse chord progression** - Each measure follows a different chord
5. **Rhythm pattern accuracy** - Pay attention to where rests fall
6. **Transitions between sections** - Don't invent filler measures

Includes a verification checklist and a [reference of common mistakes](references/common-mistakes.md) from real transcription sessions.

## Installation

Copy the skill into your Claude Code skills directory:

```bash
mkdir -p ~/.claude/skills/guitar-tab-transcription/references
cp SKILL.md ~/.claude/skills/guitar-tab-transcription/
cp references/common-mistakes.md ~/.claude/skills/guitar-tab-transcription/references/
```

The skill will be available as `/guitar-tab-transcription` in Claude Code.

## Usage

Works best with the [Guitar Pro MCP server](../guitar-pro-mcp/) for writing the actual .gp5 files. Invoke the skill when transcribing tabs from video:

> "/guitar-tab-transcription - Transcribe the bass tab from this video of MCR's Teenagers"
