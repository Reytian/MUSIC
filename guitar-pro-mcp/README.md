# Guitar Pro MCP Server

A Python MCP server that lets Claude Code create and edit Guitar Pro tab files (.gp5). Files can be opened in Guitar Pro 8.

## Tools

| Tool | Description |
|------|-------------|
| `create_song` | Create a new .gp5 file with one track |
| `write_measures` | Write notes into measures (supports full notation: bends, slides, hammer-ons, etc.) |
| `add_track` | Add a new track to an existing file |
| `read_song` | Read and display file info including notes |
| `list_files` | List Guitar Pro files in a directory |
| `set_tempo` | Change the tempo of a file |
| `duplicate_measures` | Copy measures from one location to another within a track |

## Tunings (13)

`standard`, `drop_d`, `open_g`, `open_d`, `open_e`, `open_a`, `dadgad`, `half_step_down`, `whole_step_down`, `7string_standard`, `bass_standard`, `bass_drop_d`, `bass_5_standard`

## Instruments (12)

`nylon`, `acoustic`, `jazz`, `clean`, `muted`, `overdrive`, `distortion`, `harmonics`, `bass_finger`, `bass_pick`, `bass_fretless`, `bass_slap`

## Setup

### 1. Install dependencies

```bash
python -m venv venv
source venv/bin/activate
pip install -r requirements.txt
```

### 2. Configure Claude Code

Add to your Claude Code MCP settings (`~/.claude.json` or project `.mcp.json`):

```json
{
  "mcpServers": {
    "guitar-pro-tabs": {
      "command": "/path/to/guitar-pro-mcp/venv/bin/python",
      "args": ["/path/to/guitar-pro-mcp/server.py"]
    }
  }
}
```

### 3. Use it

Ask Claude Code to create a Guitar Pro tab:

> "Create a Guitar Pro file for 'Smoke on the Water' by Deep Purple at 112 BPM with a distortion guitar track"

## Note Format

Notes are passed as JSON to `write_measures`. Each beat supports:

- **String/fret**: `{"string": 6, "fret": 0}` (string 1 = highest, 6 = lowest)
- **Durations**: `whole`, `half`, `quarter`, `eighth`/`8th`, `16th`, `32nd`, `64th`
- **Dotted**: `"dotted": true`
- **Rests**: `"rest": true`
- **Effects**: `hammer`, `vibrato`, `palm_mute`, `let_ring`, `staccato`, `accent`, `ghost`, `dead`, `tie`
- **Slides**: `"slide": "shift"` / `"legato"` / `"down"` / `"up"`
- **Bends**: `"bend": 2` (semitones) or `"bend_release": 2`
