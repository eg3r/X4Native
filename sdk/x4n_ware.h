// x4n_ware.h — Ware database accessors via host Lua C-func GetWareData.
// Mod / DLC / patch aware (engine runtime values, not static wares.xml parses).
// Returns nullopt for unknown wares, unknown fields, non-numeric values, or
// unresolved Lua. Non-integer Lua numbers are truncated toward zero.
// All accessors: UI thread only (Lua state not thread-safe).
#pragma once

#include "x4n_core.h"
#include <cstdint>
#include <optional>

namespace x4n { namespace ware {

namespace detail_ware {
    inline X4nLuaKey key(const char* ware_id) {
        X4nLuaKey k{};
        k.type = X4N_KEY_STRING;
        k.v.s  = ware_id;
        return k;
    }
}

/// Engine ware-DB average price (Cr per unit). @thread UI only.
inline std::optional<int64_t> avg_price(const char* ware_id) {
    if (!ware_id || !detail::g_api->get_lua_property) return std::nullopt;
    int64_t v = 0;
    if (!detail::g_api->get_lua_property("GetWareData", detail_ware::key(ware_id),
                                         "avgprice", X4N_VAL_INT64, &v))
        return std::nullopt;
    return v;
}

/// Engine ware-DB minimum price floor. @thread UI only.
inline std::optional<int64_t> min_price(const char* ware_id) {
    if (!ware_id || !detail::g_api->get_lua_property) return std::nullopt;
    int64_t v = 0;
    if (!detail::g_api->get_lua_property("GetWareData", detail_ware::key(ware_id),
                                         "minprice", X4N_VAL_INT64, &v))
        return std::nullopt;
    return v;
}

/// Engine ware-DB maximum price ceiling. @thread UI only.
inline std::optional<int64_t> max_price(const char* ware_id) {
    if (!ware_id || !detail::g_api->get_lua_property) return std::nullopt;
    int64_t v = 0;
    if (!detail::g_api->get_lua_property("GetWareData", detail_ware::key(ware_id),
                                         "maxprice", X4N_VAL_INT64, &v))
        return std::nullopt;
    return v;
}

/// Any numeric `GetWareData` field by name. Truncates non-integer values.
/// @thread UI only.
inline std::optional<int64_t> get_field_int(const char* ware_id, const char* field) {
    if (!ware_id || !field || !detail::g_api->get_lua_property) return std::nullopt;
    int64_t v = 0;
    if (!detail::g_api->get_lua_property("GetWareData", detail_ware::key(ware_id),
                                         field, X4N_VAL_INT64, &v))
        return std::nullopt;
    return v;
}

}} // namespace x4n::ware
