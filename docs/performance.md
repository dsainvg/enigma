# Performance & Tuning Guide

Enigma streams files in configurable chunks and distributes cipher block passes
across multiple threads. This page tells you exactly what to expect and how to
tune the knobs for your workload.

---

## How Performance Scales

### Key derivation dominates small files

Every encrypt/decrypt call runs `hash_password` **once** to derive the key.
At `cost=10` this takes ~55–150 ms regardless of file size.
For files under ~10 MB, key derivation is the dominant cost.

| Cost | Approx. KDF time | Resistance vs cost=10 |
|------|------------------|-----------------------|
| 8    | ~15–40 ms        | 4× easier to brute-force |
| 10   | ~55–150 ms       | **Default — good balance** |
| 12   | ~240–580 ms      | 4× harder |
| 14   | ~1 200–3 000 ms  | 16× harder — use for long-term archives |

### Threading accelerates large files

Each 1 KB cipher block is independent and can be processed in parallel.
On large files the cipher work dominates and additional threads help:

| File size | Threads | Approx. throughput (cost=8) |
|-----------|---------|---------------------------|
| 64 MB     | 1       | ~10–13 MB/s               |
| 64 MB     | 4       | ~30–45 MB/s (estimated)   |
| 64 MB     | all cores | **~60–70 MB/s (estimated)** |
| 1 MB      | 1       | ~8–9 MB/s                 |
| 1 MB      | 4       | ~18–19 MB/s               |
| 1 MB      | auto(22) | **~22 MB/s**             |

### Buffer size controls memory, not throughput

`--buffer-mb` controls how much data is held in RAM at once.
**File size has zero effect on peak memory.** A 500 MB file with `--buffer-mb=4`
uses only ~4–8 MB peak RSS.

| `--buffer-mb` | Peak RSS |
|---------------|----------|
| 1             | ~1–10 MB |
| 4             | ~4–8 MB  |
| 16            | ~16–25 MB |
| 64            | ~60–85 MB |

---

## Benchmark Results

> **Environment:** Python 3.13.9 / 22-core CPU / Windows
> Run with: `python benchmarks/bench.py`

### Hash Benchmarks — Full Matrix

| Password len | Cost | Avg time (ms) | Min / Max (ms)      | Throughput (h/s) | Peak RSS delta (MB) |
|---|---|---|---|---|---|
| 8   | 8  | 17.0  | 15.8 / 19.6   | 58.8  | 0.3  |
| 8   | 10 | 68.9  | 68.0 / 70.1   | 14.5  | 0.6  |
| 8   | 12 | 278.5 | 274.5 / 283.3 | 3.6   | 0.4  |
| 8   | 14 | 2146  | 1524 / 2440   | 0.5   | 0.9  |
| 16  | 8  | 38.2  | 30.8 / 45.4   | 26.2  | 1.2  |
| 16  | 10 | 150.7 | 132.8 / 177.1 | 6.6   | 5.6  |
| 16  | 12 | 581.2 | 514 / 636     | 1.7   | 38.6 |
| 16  | 14 | 1931  | 1856 / 2008   | 0.5   | 46.7 |
| 32  | 10 | 104.5 | 84 / 122      | 9.6   | 4.6  |
| 64  | 10 | 123.6 | 99 / 151      | 8.1   | 4.6  |
| 128 | 10 | 142.3 | 110 / 169     | 7.0   | 4.6  |
| 256 | 10 | 144.1 | 118 / 182     | 6.9   | 0.1  |
| 512 | 10 | 65.8  | 63.8 / 69.9   | 15.2  | 4.6  |
| 128 | 12 | 559.4 | 459 / 651     | 1.8   | 17.5 |
| 256 | 14 | 1229  | 1076 / 1774   | 0.8   | 28.2 |
| 512 | 14 | 2482  | 1903 / 2712   | 0.4   | 30.5 |

!!! note
    **Password length has minimal impact** — cost factor dominates completely.
    A 512-char password at cost=10 takes ~66 ms; an 8-char password at cost=10 takes ~69 ms.

### File Encryption — 1 MB (cost=8)

| Threads   | Buffer (MB) | Encrypt MB/s | Decrypt MB/s | Peak RSS (MB) |
|-----------|-------------|--------------|--------------|---------------|
| 1         | 1           | 8.5          | 8.0          | 1.0           |
| 1         | 4           | 9.2          | 8.6          | 4.0           |
| 2         | 4           | **14.2**     | 11.3         | 4.1           |
| 4         | 4           | **18.7**     | 13.8         | 4.1           |
| auto (22) | 1           | **22.3**     | 15.1         | 1.4           |

### File Encryption — 64 MB (cost=8, single-thread)

| Threads | Buffer (MB) | Encrypt (s) | Encrypt MB/s | Peak RSS (MB) |
|---------|-------------|-------------|--------------|---------------|
| 1       | 1           | 6.5         | 9.9          | 9.9           |
| 1       | 4           | 5.4         | 11.8         | 5.1           |
| 1       | 32          | 5.1         | **12.6**     | 32.0          |

---

## Recommendations

### For most files (default)
```bash
enigma encrypt file.bin "password"
# cost=10, threads=1, buffer=4 MB — safe and simple
```

### For large files (> 50 MB) — maximum throughput
```bash
enigma encrypt huge.iso "password" --threads=0 --buffer-mb=4
# threads=0 = all CPU cores, buffer=4 MB keeps memory low
```

### For memory-constrained systems (< 512 MB RAM)
```bash
enigma encrypt huge.iso "password" --threads=4 --buffer-mb=1
# Only ~1–10 MB RAM regardless of file size
```

### For long-term archive security (cost=14)
```bash
enigma encrypt secrets.zip "password" --cost=14
# ~2 seconds per encrypt/decrypt — deliberately slow for brute-force resistance
```

### Parallel batch encryption (Python)
```python
import enigma, threading

files = [("a.mp4","a.enc"), ("b.mp4","b.enc"), ("c.mp4","c.enc")]
threads = [
    threading.Thread(target=enigma.encrypt_file,
                     args=(s, d, "pass"),
                     kwargs={"threads": 4, "io_buf_mb": 4})
    for s, d in files
]
for t in threads: t.start()
for t in threads: t.join()
# GIL is released during file ops — true parallel execution
```
