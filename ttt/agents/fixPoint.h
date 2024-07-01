#pragma once
// An simple fix point calc lib of unsigned number
#include <math.h>
#include <stdint.h>

#define PRECISION 12
// In mcts, there has 100000 ITERATION for simulation
// For 4x4 with 3 in line to be winner, first ai always
// win if it is smart enough. The score will be plused by 1
// when a move cause win.
// In the most extreme situation, the score can be 100000
// and log2(100000) = 16.6 -> We need at least 17 bits for
// int part, PRECISION cannot > 15

// extern inline double _fix_to_float(uint32_t val, int precision);
// extern inline uint32_t _float_to_fix(double f, int precision);
// extern inline uint32_t _fixMult(uint32_t a, uint32_t b, int precision);
// extern inline uint32_t _fixDiv(uint32_t a, uint32_t b, int precision);
// extern inline uint32_t _fixSqrt(uint32_t val, int precision);
// extern inline uint32_t _fixLog2(uint32_t x, int precision);

// extern inline double fix_to_float(uint32_t val);
// extern inline uint32_t float_to_fix(double f);
// extern inline uint32_t fixMult(uint32_t a, uint32_t b);
// extern inline uint32_t fixDiv(uint32_t a, uint32_t b);
// extern inline uint32_t fixSqrt(uint32_t val);
// extern inline uint32_t fixLog2(uint32_t x);

static inline double _fix_to_float(uint32_t val, int precision)
{
    double res = 0.0;
    for (int i = 0; i < precision; i++) {
        if (val & 1)
            res += pow(2, -(precision - i));

        val >>= 1;
    }

    res += (unsigned) val;
    return res;
}

static inline uint32_t _float_to_fix(double f, int precision)
{
    return (uint32_t) (f * (1 << precision));
}

static inline uint32_t _fixMult(uint32_t a, uint32_t b, int precision)
{
    uint64_t res = a * b;
    return (uint32_t) (res >> precision);
}

static inline uint32_t _fixDiv(uint32_t a, uint32_t b, int precision)
{
    uint64_t t_a = (uint64_t) a << precision;
    // or 1 to prevent div by zero
    uint32_t res = (t_a / (b | 1));
    return res;
}

static inline uint32_t _fixSqrt(uint32_t val, int precision)
{
    /**
     * This function computes the square root of a fixed-point number using
     * Newton's method.
     *
     * Explanation of terms:
     * - 'x' is the initial guess for the square root.
     * - CLZ (Count Leading Zeros) determines the number of leading zeros in
     * 'val', helping choose an approximate value for the square root
     * calculation, speeding it up.
     * - If CLZ(k) = 10, it means the MSB (Most Significant Bit) is at index 32
     * - 10 = 22. Since we're calculating the square root, sqrt(2^22) = 2^11, so
     * the initial 'x' is set as (1 << 11). We can get 11 by (32 - CLZ) >> 1.
     * - However, 'val' passed has already been left-shifted by precision bits,
     *   meaning the true integer length width is only (32 - precision).
     *   So the initial 'x' is boosted with left shift (32 - precision - CLZ)
     * >> 1.
     * - Since 'val' is a fixed-point number with precision bits, to represent
     * 1, 'x' is set to be 1 << precision.
     * - Combining 'x' with (32 - precision - CLZ) >> 1, we get:
     *     x = (1 << precision) << ((32 - precision - CLZ) >> 1)
     *       = 1 << (precision + (32 - precision - CLZ) >> 1)
     *       = 1 << ((precision >> 1) + 16 - (CLZ >> 1))
     * The above method is come up with conversion with TWstinkytofu, huge
     * thanks to him.
     */
    uint32_t clz_res = __builtin_clz(val);
    uint64_t x = (uint64_t) 1 << ((precision >> 1) + 16 - (clz_res >> 1));
    for (int i = 0; i < 4; i++) {
        x = (x + _fixDiv(val, x, precision)) >> 1;
    }
    return (uint32_t) x;
}

static inline uint32_t _fixLog2(uint32_t x, int precision)
{
    // This implementation is based on Clay. S. Turner's fast binary logarithm
    // algorithm (http://www.claysturner.com/dsp/binarylogarithm.pdf).

    int32_t b = 1U << (precision - 1);
    int32_t y = 0;

    if (x == 0) {
        return 0;  // represents negative infinity
    }

    while (x < 1U << precision) {
        x <<= 1;
        y -= 1U << precision;
    }

    while (x >= 2U << precision) {
        x >>= 1;
        y += 1U << precision;
    }

    uint64_t z = x;

    for (int i = 0; i < precision; i++) {
        z = z * z >> precision;
        if (z >= 2U << precision) {
            z >>= 1;
            y += b;
        }
        b >>= 1;
    }
    return y;
}

static inline double fix_to_float(uint32_t val)
{
    return _fix_to_float(val, PRECISION);
}
static inline uint32_t float_to_fix(double f)
{
    return _float_to_fix(f, PRECISION);
}
static inline uint32_t fixMult(uint32_t a, uint32_t b)
{
    return _fixMult(a, b, PRECISION);
}
static inline uint32_t fixDiv(uint32_t a, uint32_t b)
{
    return _fixDiv(a, b, PRECISION);
}
static inline uint32_t fixSqrt(uint32_t val)
{
    return _fixSqrt(val, PRECISION);
}
static inline uint32_t fixLog2(uint32_t x)
{
    return _fixLog2(x, PRECISION);
}