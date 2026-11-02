#!/usr/bin/env python3
"""Generate Shawky Call's sound files (original, synthesized tones; no third-party audio).

MicroSIP looks for these next to the exe:
  ringtone.wav  incoming call ring (looped)
  ringing.wav   local ringback for outgoing calls when the PBX sends 180 without early media
                (replayed every 4.5 s by MicroSIP)
  ringing2.wav  call-waiting beep (second incoming call during a call)
  hangup.wav    call ended
  msgin.wav     message received
  msgout.wav    message sent

Usage: python3 tools/make-sounds.py [output_dir]   (default: sounds/)
Output: 16 kHz, 16-bit PCM mono, deterministic (same bytes on every run).
"""
import math
import os
import struct
import sys
import wave

RATE = 16000


def tone(freqs, seconds, volume=0.5, fade=0.01, am_hz=0.0):
    n = int(RATE * seconds)
    out = []
    f = int(RATE * fade)
    for i in range(n):
        t = i / RATE
        v = sum(math.sin(2 * math.pi * fr * t) for fr in freqs) / len(freqs)
        if am_hz:
            v *= 0.6 + 0.4 * math.sin(2 * math.pi * am_hz * t)
        env = 1.0
        if f:
            env = min(1.0, i / f, (n - 1 - i) / f)
        out.append(v * volume * env)
    return out


def silence(seconds):
    return [0.0] * int(RATE * seconds)


def write(path, samples):
    with wave.open(path, "wb") as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(RATE)
        w.writeframes(b"".join(struct.pack("<h", int(max(-1, min(1, s)) * 32767)) for s in samples))


def main():
    out = sys.argv[1] if len(sys.argv) > 1 else os.path.join(os.path.dirname(__file__), "..", "sounds")
    os.makedirs(out, exist_ok=True)
    ring_burst = tone([700, 900], 0.4, 0.45, am_hz=20)
    sounds = {
        # Two short trills, then a pause (about 3 s per loop).
        "ringtone.wav": ring_burst + silence(0.2) + ring_burst + silence(2.0),
        # Classic 425 Hz ringback burst, 1 s (MicroSIP repeats it every 4.5 s).
        "ringing.wav": tone([425], 1.0, 0.35),
        "ringing2.wav": tone([440], 0.2, 0.35) + silence(0.15) + tone([440], 0.2, 0.35),
        "hangup.wav": tone([425], 0.15, 0.35) + silence(0.1) + tone([425], 0.15, 0.35)
        + silence(0.1) + tone([425], 0.15, 0.35),
        "msgin.wav": tone([880], 0.08, 0.3) + tone([1320], 0.12, 0.3),
        "msgout.wav": tone([1320], 0.08, 0.3) + tone([880], 0.12, 0.3),
    }
    for name, samples in sounds.items():
        path = os.path.join(out, name)
        write(path, samples)
        print(f"{path}: {len(samples) / RATE:.2f} s")


if __name__ == "__main__":
    main()
