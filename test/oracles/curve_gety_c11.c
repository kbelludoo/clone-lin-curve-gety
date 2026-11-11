/*
 * Independent C11 oracle for Curve 3pool get_D / get_y (uint64 scale, N=3).
 *
 * Two implementations of (a*b+add)/d:
 *   1. GCC/Clang unsigned __int128
 *   2. Portable 32-bit limbs matching src/lin_curve_gety.lin
 *
 * Consensus between (1) and (2) is required before any LIN comparison.
 * This file is not linked into the LIN compiler TCB.
 *
 * Algorithm: published stableswap Newton solver (Curve 3pool get_D/get_y).
 * Upstream bytes are pinned by sha256/git blob in the proof harness; the
 * Curve.Fi repository states all-rights-reserved / informational-only, so
 * this oracle is original C11 and does not vendor the Vyper source.
 */
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NCOINS 3
#define NEWTON 255

static int u_lt(uint64_t a, uint64_t b) { return a < b; }

static uint64_t muldiv_add_i128(uint64_t a, uint64_t b, uint64_t add, uint64_t d) {
    __uint128_t q;
    if (d == 0) return 0;
    q = ((__uint128_t)a * (__uint128_t)b + (__uint128_t)add) / (__uint128_t)d;
    if (q > UINT64_MAX) return 0;
    return (uint64_t)q;
}

static void muldiv_add_limbs(uint64_t a, uint64_t b, uint64_t add, uint64_t d,
                             uint64_t *q_out, uint64_t *r_out, int *ov_out) {
    uint64_t a0, a1, b0, b1, p0, p1, p2, p3, mid, t, lo, hi;
    uint64_t mid_carry, lo_carry, r_hi, r_lo, q, new_hi, new_lo, bit, ge, need_borrow;
    int i, ov;
    if (d == 0) {
        *q_out = 0;
        *r_out = 0;
        *ov_out = 1;
        return;
    }
    a0 = a & 0xffffffffULL;
    a1 = a >> 32;
    b0 = b & 0xffffffffULL;
    b1 = b >> 32;
    p0 = a0 * b0;
    p1 = a0 * b1;
    p2 = a1 * b0;
    p3 = a1 * b1;
    mid = p1 + p2;
    mid_carry = (mid < p1) ? 1 : 0;
    t = (mid & 0xffffffffULL) << 32;
    lo = p0 + t;
    lo_carry = (lo < p0) ? 1 : 0;
    hi = p3 + (mid >> 32);
    hi = hi + (mid_carry << 32);
    hi = hi + lo_carry;
    lo = lo + add;
    if (lo < add) hi = hi + 1;
    r_hi = 0;
    r_lo = 0;
    q = 0;
    ov = 0;
    for (i = 127; i >= 0; i--) {
        bit = (i >= 64) ? ((hi >> (i - 64)) & 1ULL) : ((lo >> i) & 1ULL);
        new_hi = (r_hi << 1) | (r_lo >> 63);
        new_lo = (r_lo << 1) | bit;
        r_hi = new_hi;
        r_lo = new_lo;
        ge = (r_hi != 0) || (r_lo >= d);
        if (ge) {
            need_borrow = u_lt(r_lo, d);
            r_lo = r_lo - d;
            if (need_borrow) r_hi = r_hi - 1;
            if (i >= 64) ov = 1;
            if (i < 64) q = q | (1ULL << i);
        }
    }
    if ((int64_t)q < 0) ov = 1;
    *q_out = ov ? 0 : q;
    *r_out = r_lo;
    *ov_out = ov;
}

static uint64_t muldiv_add_limbs_q(uint64_t a, uint64_t b, uint64_t add, uint64_t d) {
    uint64_t q, r;
    int ov;
    muldiv_add_limbs(a, b, add, d, &q, &r, &ov);
    return q;
}

typedef uint64_t (*mdadd_fn)(uint64_t, uint64_t, uint64_t, uint64_t);

static uint64_t pick(int i, uint64_t x0, uint64_t x1, uint64_t x2) {
    if (i == 0) return x0;
    if (i == 1) return x1;
    if (i == 2) return x2;
    return 0;
}

static uint64_t get_d(uint64_t x0, uint64_t x1, uint64_t x2, uint64_t amp, mdadd_fn md) {
    uint64_t s, ann, d, dp, dprev, dp_n, sum, left, right, den;
    int iter;
    if (amp == 0 || x0 == 0 || x1 == 0 || x2 == 0) return 0;
    if (x0 > UINT64_MAX - x1) return 0;
    s = x0 + x1;
    if (s > UINT64_MAX - x2) return 0;
    s = s + x2;
    if (s == 0) return 0;
    if (amp > UINT64_MAX / NCOINS) return 0;
    ann = amp * NCOINS;
    d = s;
    for (iter = 0; iter < NEWTON; iter++) {
        dp = d;
        dp = md(dp, d, 0, x0 * (uint64_t)NCOINS);
        dp = md(dp, d, 0, x1 * (uint64_t)NCOINS);
        dp = md(dp, d, 0, x2 * (uint64_t)NCOINS);
        dprev = d;
        dp_n = md(dp, (uint64_t)NCOINS, 0, 1);
        sum = md(ann, s, dp_n, 1);
        left = md(ann - 1, d, 0, 1);
        right = md((uint64_t)(NCOINS + 1), dp, 0, 1);
        if (left > UINT64_MAX - right) return 0;
        den = left + right;
        d = md(sum, d, 0, den);
        if (d >= dprev) {
            if (d - dprev <= 1) break;
        } else if (dprev - d <= 1) {
            break;
        }
    }
    return d;
}
