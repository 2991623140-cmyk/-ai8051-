"""Merge all MIDI tracks into timed events with eight preassigned voice slots."""
import argparse
from collections import defaultdict, deque
from pathlib import Path
import mido


def convert(path):
    source = mido.MidiFile(path)
    if source.type == 2:
        raise ValueError(f'{path.name}: asynchronous type-2 MIDI is not supported')
    elapsed = 0.0
    last_ms = 0
    events = []
    active = defaultdict(deque)
    slots = [None] * 8
    sustain = [False] * 16
    released = [[] for _ in range(16)]
    max_poly = 0
    def emit(now, note, action):
        nonlocal last_ms
        delay = now - last_ms
        while delay > 60000:
            events.append((60000, 255, 0))
            delay -= 60000
        events.append((delay, note, action))
        last_ms = now

    def release(now, slot):
        note = slots[slot][1]
        emit(now, note, 128 + slot)
        slots[slot] = None

    for msg in source:
        elapsed += msg.time
        now = round(elapsed * 1000)
        if msg.type == 'control_change' and msg.control == 64:
            sustain[msg.channel] = msg.value >= 64
            if not sustain[msg.channel]:
                for slot in released[msg.channel]:
                    release(now, slot)
                released[msg.channel].clear()
            continue
        if msg.type not in ('note_on', 'note_off'):
            continue
        if msg.channel == 9:
            raise ValueError(f'{path.name}: percussion mapping is required for drum tracks')
        key = (msg.channel, msg.note)
        if msg.type == 'note_on' and msg.velocity:
            if None not in slots:
                raise ValueError(f'{path.name}: exceeds eight voices including sustain at {now}ms')
            slot = slots.index(None)
            slots[slot] = key
            active[key].append(slot)
            emit(now, msg.note, slot + 1)
            max_poly = max(max_poly, sum(slot is not None for slot in slots))
        elif active[key]:
            slot = active[key].popleft()
            if sustain[msg.channel]:
                released[msg.channel].append(slot)
            else:
                release(now, slot)
    # A held sustain pedal at file end must not leave voices stuck on.
    for slot, key in enumerate(slots):
        if key is not None:
            release(round(elapsed * 1000), slot)
    if not events:
        raise ValueError(f'{path.name}: no notes')
    return events, last_ms, max_poly


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    names = ['静悄悄', '起风了', '九重现实', '五重空洞', '赤伶----HITA']
    text = ['#ifndef PIANO_SONG_DATA_H', '#define PIANO_SONG_DATA_H', '',
            'typedef struct { unsigned int delay; unsigned char note; unsigned char velocity; } SongEvent;']
    text.append(f'#define SONG_COUNT {len(names)}u')
    accessors = []
    for index, name in enumerate(names):
        path = args.source / (name + '.mid')
        if not path.exists():
            text.append(f'#define SONG_{index}_COUNT 0u')
            print(f'{name}: missing')
            continue
        events, length, poly = convert(path)
        text.append(f'#define SONG_{index}_COUNT {len(events)}u')
        text.append(f'static const SongEvent code song_{index}[] = {{')
        text.extend(f'    {{{delay}u,{note}u,{velocity}u}},' for delay, note, velocity in events)
        text.append('};')
        print(f'{name}: all tracks, {len(events)} events, {length / 1000:.2f}s, max voices {poly}')
        accessors.append((index, len(events)))
    for result_type, function, field in [('unsigned int', 'SongData_Count', None),
                                         ('unsigned int', 'SongData_Delay', 'delay'),
                                         ('unsigned char', 'SongData_Note', 'note'),
                                         ('unsigned char', 'SongData_Velocity', 'velocity')]:
        params = 'unsigned char selected' + (', unsigned int index' if field else '')
        text.extend([f'static {result_type} {function}({params})', '{', '    switch (selected) {'])
        for index, count in accessors:
            value = f'song_{index}[index].{field}' if field else f'{count}u'
            text.append(f'    case {index}: return {value};')
        text.extend(['    default: return 0;', '    }', '}'])
    text.extend(['', '#endif', ''])
    args.output.write_text('\n'.join(text), encoding='ascii')


if __name__ == '__main__':
    main()
