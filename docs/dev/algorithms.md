# Cryptographic Algorithms

This section details the custom cryptographic processes implemented inside Enigma's core engine.

---

## 🔑 Hashing Internals

The hashing routine `internal_hash_password` mixes inputs into printable ASCII sequences using modular math and polynomial equations.

### 1. `hash1` (Character Mixing)
Processes the initial byte sequence $S = \text{salt} + \text{"$"} + \text{password}$:

$$\text{accumulator} = \text{seed}$$

For each byte $b_j$ in the sequence:

$$\text{val} = \text{accumulator} \times 113 + b_j$$

$$\text{accumulator} = \text{accumulator} + 2 \pmod{23}$$

$$\text{char\_code} = (\text{val} \pmod{90}) \ \& \ \left( \left\lfloor \frac{\text{val}}{90} \right\rfloor \pmod{90} + 37 \right)$$

The resulting `char_code` byte is emitted to the output buffer `hash1`.

### 2. Permutation Step
Removes elements from `hash1` one-by-one at cyclic offsets:

$$\text{index} = (\text{current} + n - 1) \pmod{\text{size}}$$

This produces a specific permutation of the character array rather than a standard sort.

### 3. `hash2` (Polynomial Hash, Base 71)
Computes a base-71 polynomial value:

$$hv_{next} = hv \times 71 + b_k$$

For each generated state, we emit printable ASCII codes by extracting digits:
$$\text{code} = (hv \pmod{90}) + 37$$

$$hv = \left\lfloor \frac{hv}{90} \right\rfloor$$

### 4. `hash3` (Polynomial Hash, Base 997)
Computes a base-997 polynomial hash. 

> [!IMPORTANT]
> To preserve state dependency, the final value of the accumulator $hv$ from `hash2` is **not** reset; it is carried directly into `hash3` as the starting accumulator seed.

---

## 🔒 Cipher Internals (Rotate + XOR)

The cipher divides files and buffers into **1024-byte chunks** (blocks). 

### 1. Rounds Calculation
For each block at index $i$:

$$\text{Rounds} = 10 + (i \pmod 6)$$

This ensures that consecutive blocks are processed with varying iteration depths ($10 \le \text{Rounds} \le 15$), breaking static pattern analysis.

### 2. Rotation and XOR
In each round $n$:

1.  **Shift Calculation**: The block is rotated left bitwise by:
    $$\text{shift} = 1 + ((\text{key\_byte}[n] + n) \pmod 7) \text{ bits}$$
2.  **Bitwise Left Rotation**: Bits are shifted left across the entire 1 KB block. The overflow bits of byte $j$ are carried over into byte $j-1$, and the leftmost bits of byte 0 are wrapped around to the rightmost positions of byte 1023.
3.  **XOR**: Each byte of the rotated block is XORed with the corresponding derived key byte.

---

## 💾 `ENIGMA01` Binary Format

All integers are stored in **Little-Endian** byte order.

```
+------------------+------------------+-------------------+-----------------+
| Magic Header     | Salt Length      | Salt String       | Cost Factor     |
| "ENIGMA01" (8B)  | uint16 (2B)      | ASCII (16B)       | uint16 (2B)     |
+------------------+------------------+-------------------+-----------------+
| Verify Tag Len   | Verify Tag       | Encrypted Data Chunks               |
| uint16 (2B)      | ASCII (Var)      | 1 KB Sequential Blocks (Var)        |
+------------------+------------------+-------------------+-----------------+
```
