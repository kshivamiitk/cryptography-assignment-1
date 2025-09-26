# Short report

## 1 . Secret sharing implementations

Overview

We use additive secret sharing over the prime-like modulus MOD = 1,000,000,007. Any value x (scalar or vector element) is represented as two shares x0, x1 held by Party 0 and Party 1 respectively such that:

```x ≡ x0 + x1  (mod MOD)```

No single share leaks information about x (uniform random share), and all arithmetic is performed modulo MOD using three helper ops:

modadd(a,b) = (a + b) mod MOD

modsub(a,b) = (a - b) mod MOD

modmul(a,b) = (a * b) mod MOD (used for local multiply of share-by-share; careful where full multiplies are needed)

The code stores matrix and vector shares in party0_shares.txt and party1_shares.txt.

| Item          |                                          Meaning | Representation in file                                                                                           |
| ------------- | -----------------------------------------------: | ---------------------------------------------------------------------------------------------------------------- |
| `U`           |m×k user matrix (secret-shared) | Each party has `U_share` with m lines of k numbers                                                               |
| `V`           |n×k item matrix (secret-shared) | Each party has `V_share` with n lines of k numbers                                                               |
| `j`           |private index (one-hot) | For each query t, each party holds `j_shares[t]` length n such that sums to one-hot                              |
| `dot_triple`  |preprocessed vector-vector Beaver triple | `a (k), b (k), c (scalar = a·b)` — additively shared across parties                                              |
| `sv_triple`   |preprocessed scalar×vector triple | `A (k), B (scalar), C (k) = B * A` — additively shared                                                           |
| `sel_triples` | preprocessed scalar×vector triples for selection | For each query and for each index `idx`: `(A_sel(idx), B_sel(idx), C_sel(idx)=B_sel* A_sel)` — additively shared |

Why additive shares?

Beaver triples let us convert multiplication of secret-shared values into small interactive protocols with only one opening round.


## 2. How inner products and updates were computed securely

This section explains how each building block is computed inside MPC using Beaver-style preprocessing. I include algebra, message flow and a short proof sketch of correctness.

Notation

U[i] — the ith row (vector) of U (secret-shared).

V[j] — the jth row (vector) of V (secret-shared).

· — dot product.

MOD — modulus (field-like arithmetic).

share(x) means two additive shares x0, x1.

Goal (per query)

Compute update:

dot = ⟨U[i], V[j]⟩

delta = 1 - dot

U[i] := U[i] + delta * V[j] (elementwise, mod MOD)

All operations must be done on secret-shared values without revealing j, U[i], V[j], or intermediate secrets.

### A. Beaver triple for inner-product (vector·vector)

#### Preprocessing (offline):

Trusted or offline generator provides additive shares of random vectors a, b and scalar c = ⟨a, b⟩.

Each party receives a_share, b_share, c_share.

#### Online protocol (one dot product):

##### 1. Each party locally computes:

```
    alpha_share = U[i]_share - a_share    (vector)
    beta_share  = V_j_share - b_share     (vector)
```

where V_j_share is the party's share of V[j] (computed by selection protocol below).

##### 2. Parties open alpha = alpha0 + alpha1 and beta = beta0 + beta1 (server coordinates the opening).

##### 3. Each party computes locally:

```
local_dot_share = c_share + <alpha, b_share> + <beta, a_share>
```

##### 4. Summing two parties' local_dot_share reconstructs:

```
c + <alpha, b> + <beta, a> + <alpha, beta> = <a,b> + <U-a, b> + <V-b, a> + <U-a, V-b>
  = <U, V>
```

(all arithmetic mod MOD) — therefore correctness holds.

Security intuition: Because alpha and beta are opened, but they are one-time masked with random a and b, the openings leak nothing about U[i] or V[j]. The server sees alpha, beta but not U or V because these are masked by a and b that are random (unknown to the server if it does not collude).


### B. Computing delta * V[j] (scalar × vector) — scalar-vector Beaver triple

We must compute the product of a secret-shared scalar (delta, derived from the dot) and a secret-shared vector (V[j]). We use a scalar×vector triple (A, B, C = B * A) preprocessed and shared.

Online protocol:

1. Each party locally computes:

```
e_share = V_j_share - A_share   (vector)
f_share = delta_share - B_share  (scalar)
```

2. Parties open e = e0 + e1 and f = f0 + f1.

3. Each party computes:

```
product_share = C_share + f*A_share + e*B_share + fe_share
```

where fe_share are party shares of f * e (server computes f * e and splits it).

4. Summing product_share across parties reconstructs delta*V[j]. Algebra identical to Beaver proof.

### C. Private selection of V[j] (the crucial correction)

If V and the one-hot j are both secret-shared, a naive local computation `sum_t j_share[t] * V_share[t]` is incorrect (it misses cross-terms). We therefore compute selection inside MPC.

`Technique used (in this implementation):` For each index t (0..n-1) we precompute a scalar×vector triple `(A_sel(t), B_sel(t), C_sel(t)=B_sel*A_sel)` so we can compute each product s_t * V[t] with Beaver. Here s_t is the bit of the one-hot vector (secret-shared).

#### Per-index online computation (batched for all t):

#### 1. Each party computes locally:

```
d_share[t] = j_share[t] - B_sel_share[t]      (scalar)
e_share[t] = V_share[t] - A_sel_share[t]      (vector)
```

#### 2. Parties open `d[t] and e[t]` (server reconstructs `d[t] = d0 + d1` and `e[t] = e0 + e1`), but these are masked by the random triple components:

```
opened d[t] = s_t - B_sel,   opened e[t] = V_t - A_sel
```

Since B_sel and A_sel are random, openings leak no meaningful info.

#### 3. Server computes `f_e[t] = d[t] * e[t]` (vector scaled by scalar) and splits it into two shares `fe0[t], fe1[t]`.

#### 4. Each party computes its share of `s_t * V_t`:

```
product_share[t] = C_sel_share[t] + d[t] * A_sel_share[t] + e[t] * B_sel_share[t] + fe_share[t]
```

Summing product_share[t] across parties yields `s_t * V_t`.

#### 5. Finally compute:
```
V_j_share = Σ_t product_share[t]   (sum over all indices)
```

This yields each party’s additive share of actual V[j].

#### Correctness sketch:
Summing both parties’ product_share[t] equals (by Beaver triple algebra)
```
C + d*A + e*B + d*e = B*A + (s-B)*A + (V-A)*B + (s-B)*(V-A)
= s*V
```

All operations mod `MOD`.

### D. Overall per-query pseudocode (client side, high-level)
#### Client (party r), for each query t:
```
1. compute selection:
   for idx in 0..n-1:
     d_share[idx] = j_shares[t][idx] - sel_b[t][idx]
     e_share[idx][:] = V[idx][:] - sel_a[t][idx][:]
   send SEL_D (n scalars) and SEL_E (n*k scalars) to server
   receive OPENED_SEL_D, OPENED_SEL_E, SEL_FE_VEC from server
   for idx:
     product_share[idx] = sel_c[t][idx] + opened_d[idx]*sel_a[t][idx] + opened_e[idx]*sel_b[t][idx] + fe_share[idx]
   vj_share = Σ_idx product_share[idx]  # n*k work locally

2. dot-product (Beaver dot triple):
   alpha = U[i] - dot_a[t]
   beta  = vj_share - dot_b[t]
   send ALPHA and BETA; receive OPENED_ALPHA, OPENED_BETA, ALPHA_BETA_R
   local_dot_share = ...  (combine as described)

3. delta_share = (party 0 does 1 - local_dot_share; party 1 does 0 - local_dot_share)

4. scalar-vector multiply delta * vj (Beaver sv triple):
   e_vec = vj_share - sv_a[t]
   f_sca = delta_share - sv_b[t]
   send E_VEC and F_SCAL; receive OPENED_E, OPENED_F, FE_VEC
   product_share = sv_c[t] + opened_f*sv_a[t] + opened_e*sv_b[t] + fe_share
   U[i] += product_share
```

## 3. Communication rounds and efficiency considerations

This section lists the number of sequential rounds (latency) and communication volume (bandwidth) per query and in preprocessing, plus asymptotic complexities and a table summarizing trade-offs.

### A. Per-query online rounds (sequential steps)

We count rounds as sequential network synchronizations (an exchange that depends on previous opened values); multiple messages sent in one batch count as one round if they are sent without waiting for an immediate response.

1. Selection phase (batched for all indices) — 1 round to send masked `d` and `e` from both clients to server, 1 round for server to broadcast opened values and fe shares.
-> 2 rounds (clients → server, server → clients)

2. Dot-product (vector·vector) — clients send `ALPHA` and `BETA` to server (1 round), server broadcasts `OPENED_ALPHA`, `OPENED_BETA` and sends `ALPHA_BETA_R` (1 round).
-> 2 rounds

3. Scalar-vector multiplication `(delta * vj)` — clients send E_VEC and F_SCAL (1 round), server broadcasts OPENED_E, OPENED_F and sends FE_VEC (1 round).
-> 2 rounds

Total sequential rounds per query:
`2 + 2 + 2 = 6` rounds (client→server / server→clients exchanges). Note: some optimizations let you pipeline or combine messages; below we discuss trade-offs.

B. Communication volume (words = elements mod MOD)

Per query, assume k = vector length, n = number of rows in V.

#### Selection phase:

* Clients → server:

   * SEL_D from each client: n scalars → 2n

   * SEL_E from each client: n*k scalars → 2n*k

* Server → clients:

    * `OPENED_SEL_D`: n scalars (broadcast) → n (sent twice folded into two directions but counted once for broadcast)

    * `OPENED_SEL_E`: n*k scalars (broadcast)

    * `SEL_FE_VEC`: each party gets n*k shares (server sends n*k to each party) → 2n*k

So selection total (count each network transfer once):

* Client→server: 2n + 2n*k scalars

* Server→clients: n + n*k + 2n*k = n + 3n*k scalars

#### Dot-product phase:

* Clients→server: ALPHA & BETA: each client sends k scalars twice (ALPHA and BETA) → 2k * 2 = 4k? Careful breakdown:

* Client 0 sends ALPHA(k) and BETA(k) -> 2k

* Client 1 sends ALPHA(k) and BETA(k) -> 2k
    => Clients→server total = 4k

* Server→clients:

    * `OPENED_ALPHA (k)` and `OPENED_BETA (k)` broadcast -> 2k

`ALPHA_BETA_R` scalar per party -> 2 scalars
=> Server→clients total = `2k + 2`

#### Scalar-vector multiplication phase:

* Clients→server:

    * E_VEC from each client: k each -> 2k

    * F_SCAL from each client: 1 each -> 2
        => total = 2k + 2

* Server→clients:

    * OPENED_E (k) broadcast -> k

    * OPENED_F (1) broadcast -> 1

    * FE_VEC vector of k sent to each party -> 2k
        => total = k + 1 + 2k = 3k + 
        
#### Aggregate per-query communication (scalars):

| Phase         |                                    Client→Server |                                     Server→Clients |
| ------------- | -----------------------------------------------: | -------------------------------------------------: |
| Selection     |                                      `2n + 2n*k` |                                         `n + 3n*k` |
| Dot-product   |                                             `4k` |                                           `2k + 2` |
| Scalar×vector |                                         `2k + 2` |                                           `3k + 1` |
| **Total**     | `2n + 2n*k + 4k + 2k + 2` = `2n + 2n*k + 6k + 2` | `n + 3n*k + 2k + 2 + 3k + 1` = `n + 3n*k + 5k + 3` |

(Communication measured in field elements; multiply by element size to get bytes — e.g., 4 or 8 bytes if stored in 32/64-bit integers, but real wire format in our code is ASCII so actual bytes higher).

``Asymptotic dominant term``: O(n*k) per query (selection phase), i.e., communication and local work scale with n*k. This is the expected cost because our simple selection uses per-index triples.

#### C. Preprocessing cost (offline generator)

The generator produces:

For each query (q) :

one dot triple (vectors length k and scalar) => O(k) storage per query

one scalar-vector triple (A length k) => O(k) per query

n selection triples: for each index idx a scalar×vector triple length k => O(n*k) per query

So total storage for preprocessing across q queries:

```

O(q * (n*k + k)) = O(q * n * k)

```

This is the major cost; memory/disk footprint grows linearly with q * n * k.

#### D. Round & bandwidth trade-offs (table)

| Metric                   |                    Value (per query) | Comments                                   |
| ------------------------ | -----------------------------------: | ------------------------------------------ |
| Rounds (sequential)      |                                    6 | selection(2) + dot(2) + sv(2)              |
| Communication (dominant) |                Θ(n·k) field elements | selection step dominates                   |
| Online local computation |           Θ(n·k + k) multiplications | building product_shares and dot/vector ops |
| Preprocessing storage    |              Θ(q·n·k) field elements | selection triples dominate                 |
| Best improvement         | use DPF/PIR or binary tree selection | reduces online cost drastically            |
