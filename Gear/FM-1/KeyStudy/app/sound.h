/* SPDX-License-Identifier: MPL-2.0 */
#include "keystudy_piano.h"

/* The audio interrupt owns the voices; the foreground publishes the desired notes. */
enum { VOICE_COUNT = 12, SAMPLE_FRACTION_BITS = 16 };
typedef struct {
    int note;
    int on;
    int tone;
    float phase;
    float level;
    uint32_t sample_position;
} StudyVoice;

static StudyVoice study_voices[VOICE_COUNT];
static float phase_step[128];
static uint32_t piano_step[128];
static uint8_t piano_zone[128];
static float study_gain;

static void sound_init(void)
{
    for (int note = 0, zone = 0; note < 128; ++note) {
        while (zone + 1 < PIANO_ANCHORS && 2 * note > piano_anchors[zone].note + piano_anchors[zone + 1].note)
            ++zone;
        float stretch = note - piano_anchors[zone].note - piano_anchors[zone].cents_x100 * 0.0001f;
        float step = fm_exp2f(stretch * (1.0f / 12.0f)) * (PIANO_RATE * (65536.0f / 44100.0f));
        piano_zone[note] = (uint8_t)zone;
        piano_step[note] = (uint32_t)(step + 0.5f);
        phase_step[note] = 440.0f * fm_exp2f((note - 69) * (1.0f / 12.0f)) * (1.0f / 44100.0f);
    }
    for (int i = 0; i < VOICE_COUNT; ++i)
        study_voices[i] = (StudyVoice){.note = -1};
}

static void assign_voices(void)
{
    NoteSet pending = sound_notes;
    int tone = settings.open && settings.item == FILTER_TONE ? settings.draft.tone : prefs.tone;
    for (int i = 0; i < VOICE_COUNT; ++i) {
        StudyVoice *voice = &study_voices[i];
        voice->on = voice->on && voice->tone == tone && key_contains(&pending, voice->note);
        if (voice->on)
            pending.word[voice->note / 32] &= ~(1u << (voice->note % 32));
    }
    for (int group = 0; group < 4; ++group) {
        uint32_t word = pending.word[group];
        for (int bit = 0; word; ++bit, word >>= 1) {
            if (!(word & 1u))
                continue;
            StudyVoice *quietest = 0;
            for (int i = 0; i < VOICE_COUNT; ++i) {
                StudyVoice *voice = &study_voices[i];
                if (!voice->on && (!quietest || voice->level < quietest->level))
                    quietest = voice;
            }
            if (!quietest)
                return;
            *quietest = (StudyVoice){.note = group * 32 + bit, .on = 1, .tone = tone};
        }
    }
}

static float sample_piano(StudyVoice *voice)
{
    uint32_t position = voice->sample_position;
    uint32_t offset = position >> SAMPLE_FRACTION_BITS;
    if (offset >= PIANO_FRAMES - 1u)
        return 0.0f;

    const uint8_t *samples = piano_anchors[piano_zone[voice->note]].samples;
    int first = piano_decode[samples[offset]];
    int second = piano_decode[samples[offset + 1]];
    int fraction = (position >> 8) & 255u;
    int mixed = first + ((second - first) * fraction >> 8);
    voice->sample_position += piano_step[voice->note];
    return mixed * (1.0f / 32768.0f);
}

static float sample_pure_tone(StudyVoice *voice)
{
    voice->phase += phase_step[voice->note];
    if (voice->phase >= 1.0f)
        voice->phase -= 1.0f;
    float saw = 2.0f * voice->phase - 1.0f;
    float pulse = 2.0f * (voice->phase < 0.5f ? saw + 0.5f : saw - 0.5f);
    float fundamental = 4.0f * saw * (1.0f - fm_fabsf(saw));
    float harmonic = 4.0f * pulse * (1.0f - fm_fabsf(pulse));
    return 0.86f * fundamental + 0.14f * harmonic;
}

void app_render(int32_t *lr, uint32_t n, uint32_t master_q12)
{
    assign_voices();
    float master = master_q12 * (8200000.0f / 4096.0f);
    for (uint32_t i = 0; i < n; ++i) {
        float sum = 0.0f;
        study_gain += (master - study_gain) * 0.004f;
        for (int j = 0; j < VOICE_COUNT; ++j) {
            StudyVoice *voice = &study_voices[j];
            if (voice->note < 0)
                continue;
            int piano = voice->tone == TONE_GRAND_PIANO;
            float target = voice->on ? (piano ? 0.118f : 0.078f) : 0.0f;
            float speed = voice->on ? 0.014f : (piano ? 0.00095f : 0.0020f);
            voice->level += (target - voice->level) * speed;
            if (!voice->on && voice->level < 0.00007f) {
                *voice = (StudyVoice){.note = -1};
                continue;
            }
            float sample = piano ? sample_piano(voice) : sample_pure_tone(voice);
            sum += sample * voice->level;
        }
        /* Leave room for full chords without flattening ordinary piano attacks. */
        if (sum > 0.86f)
            sum = 0.86f + (sum - 0.86f) * 0.25f;
        else if (sum < -0.86f)
            sum = -0.86f + (sum + 0.86f) * 0.25f;
        lr[2 * i] = lr[2 * i + 1] = (int32_t)(sum * study_gain);
    }
}

void app_panic(void)
{
    uint32_t lock = plat_lock();
    sound_notes = (NoteSet){{0}};
    for (int i = 0; i < VOICE_COUNT; ++i)
        study_voices[i] = (StudyVoice){.note = -1};
    plat_unlock(lock);
}
