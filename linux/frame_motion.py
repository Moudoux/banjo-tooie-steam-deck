#!/usr/bin/env python3
"""Frame-to-frame change of a raw 8-bit gray video, to compare interpolation runs.

Smooth motion gives an even series; frames that RT64 interpolates wrongly show up as
spikes (large max/p95 against the median) and as alternation between consecutive
frames. Record the game window first (see linux/INTERPOLATION.md):

    ffmpeg -f x11grab -window_id ID -framerate 60 -i :0 -t 4 \\
        -vf "scale=480:-2,format=gray" -f rawvideo run.gray
    python3 linux/frame_motion.py run.gray 480 270
"""
import sys

import numpy as np


def main() -> int:
    path, width, height = sys.argv[1], int(sys.argv[2]), int(sys.argv[3])
    data = np.fromfile(path, dtype=np.uint8)
    count = data.size // (width * height)
    frames = data[:count * width * height].reshape(count, height, width).astype(np.int16)
    # The upper middle of the image, where menus and most objects are.
    region = frames[:, height // 8:height // 2, width // 6:width * 5 // 6]
    diff = np.abs(np.diff(region, axis=0)).mean(axis=(1, 2))
    moving = diff[diff > 0.05]
    print(f"frames={count} changed={len(moving)} identical={int((diff <= 0.05).sum())}")
    if len(moving):
        median = np.median(moving)
        print(f"mean={moving.mean():.3f} median={median:.3f} p95={np.percentile(moving, 95):.3f} "
              f"max={moving.max():.3f} spikes(>3x median)={int((moving > 3 * median).sum())}")
        print(f"alternation={np.abs(np.diff(moving, 2)).mean() / max(moving.mean(), 1e-6):.3f}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
