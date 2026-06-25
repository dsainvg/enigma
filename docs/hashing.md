# Password Hashing

Enigma features a custom password hashing algorithm designed to stretch password credentials and resist dictionary attacks through iterative strengthening and memory-hard memoization.

---

## ⚙️ The Hashing Algorithm

Each hashing operation computes a series of intermediate rounds over the concatenated credential sequence `salt + "$" + password`.

### 1. One Hashing Round
A single round of hashing computes four distinct sub-hashes sequentially:

1. **`hash1` (Character Mixing)**: An accumulator mixes each byte of the string using prime multiplication (`* 113`) and maps the output values to printable ASCII ranges (`37`–`126`).
2. **Permutation**: Elements from `hash1` are removed one-by-one using a stateful index offset to shuffle the byte ordering.
3. **`hash2` (Polynomial Hash, multiplier 71)**: Processes the shuffled bytes using a polynomial expansion equation with a base of 71.
4. **`hash3` (Polynomial Hash, multiplier 997)**: Processes the shuffled bytes using a base of 997. The accumulator from `hash2` is carried over into `hash3` to maintain dependency integrity.
5. **`hash4` (Memo Expansion)**: Expands the output blocks by referencing values in a lookup matrix.

### 2. Work Factor & Memory Hardness
To increase the computational cost of cracking passwords, the hashing function runs iteratively:

$$\text{Iterations} = 2^{\text{cost}}$$

* **Default Cost**: `10` ($1024$ rounds).
* **Recommended Range**: `8` to `14` ($256$ to $16,384$ rounds).
* **State Table**: During iteration, a lookup table (`memo`) of size $16 \times 2^{\text{cost}}$ elements tracks previous round variables. Each new iteration references historical state indices, enforcing memory utilization and stopping simple hardware pipelining.

---

## 🐍 Python Usage

```python
import enigma

# 1. Generate a new password hash (automatically generates a random salt)
hashed = enigma.hash_password("mypassword", cost=10)
print(hashed)  
# Output format: "$salt$/$hash"
# e.g., "$J&%Z1n_N/boCGJef$/$aBcD..."

# 2. Extract the salt from an existing hash to verify a password
salt = enigma.extract_salt(hashed)

# 3. Verify a password by re-hashing with the same salt and cost
is_valid = enigma.hash_password("mypassword", cost=10, salt=salt) == hashed
assert is_valid is True
```

---

## 💻 CLI Usage

Use the `hash` command to hash passwords directly from the terminal.

```bash
# Hash a password using default cost 10 and a random salt:
enigma hash "mySecurePassword"

# Hash a password with custom cost 12:
enigma hash "mySecurePassword" 12

# Hash a password with a custom cost and a fixed salt:
enigma hash "mySecurePassword" 12 "myFixedSalt12345"
```
