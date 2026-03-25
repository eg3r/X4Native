// ---------------------------------------------------------------------------
// x4n_math.h — Game Math & Hash Utilities
// ---------------------------------------------------------------------------
// Part of the X4Native SDK. Included by x4native.h.
//
// Provides:
//   x4n::math::RAD_TO_DEG       — radian-to-degree conversion factor
//   x4n::math::advance_seed()   — game LCG seed advancement
//   x4n::math::fnv1a_lower()    — FNV-1a hash with lowercase normalization
//
// Pure functions — no game state dependency (safe to call anytime).
// ---------------------------------------------------------------------------
#pragma once

#include "x4n_core.h"

namespace x4n { namespace math {

/// Radian-to-degree conversion factor.
/// GetObjectPositionInSector returns radians, SetObjectSectorPos expects degrees.
constexpr float RAD_TO_DEG = 180.0f / 3.14159265f;

/// Advance a seed using the game's LCG formula (same as MD autoadvanceseed).
/// Formula: next = ROR64(seed * multiplier + addend, 30)
/// @stability STABLE — algorithm constants, not data offsets. Unlikely to change.
/// @verified v9.00 build 600626
inline uint64_t advance_seed(uint64_t seed) {
    uint64_t lcg = seed * X4_SEED_LCG_MULTIPLIER + X4_SEED_LCG_ADDEND;
    return (lcg >> X4_SEED_LCG_ROTATE) | (lcg << (64 - X4_SEED_LCG_ROTATE));
}

/// Compute FNV-1a hash of a lowercased string (engine convention).
/// Used by MacroRegistry, ConstructionPlanDB, and connection lookups.
/// @stability STABLE — standard hash algorithm, not data offsets.
inline uint64_t fnv1a_lower(const char* str) {
    uint64_t hash = 2166136261ULL;
    for (const char* p = str; *p; p++) {
        char c = (*p >= 'A' && *p <= 'Z') ? (*p + 32) : *p;
        hash = static_cast<int8_t>(c) ^ (16777619ULL * hash);
    }
    return hash;
}

}} // namespace x4n::math
