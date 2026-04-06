// ---------------------------------------------------------------------------
// x4n_ship.h — Ship typed wrapper with order creation
// ---------------------------------------------------------------------------
// Part of the X4Native SDK. Included by x4native.h.
//
// Provides:
//   x4n::ship::Ship — lightweight handle for ship/controllable entities
//
// Usage:
//   x4n::ship::Ship ship(ship_id);
//   if (ship.valid()) {
//       ship.order_patrol();                      // patrol current zone
//       ship.order_attack(enemy_id);              // attack a target
//       ship.order_protect_station(station_id);   // guard a station
//       ship.order_trade_routine();               // autonomous trading
//       ship.order_mining_routine();              // autonomous mining
//       ship.order_recon(sector_id);              // military reconnaissance
//       ship.order_tactical(target_id, 75);       // fleet coordination
//   }
//
// Order creation bypasses the player-ownership check in the exported
// CreateOrder3, allowing orders on NPC faction ships.
//
// Thin handle: UniverseID + cached X4Component*. No heap allocation.
// All functions require on_game_loaded to have fired.
// ---------------------------------------------------------------------------
#pragma once

#include "x4n_core.h"
#include "x4n_entity.h"
#include "x4n_log.h"
#include <cstdint>
#include <cstring>
#include <cstdio>

namespace x4n { namespace ship {

// ---- Order param value struct ----

struct OrderParamValue {
    int32_t  type;
    int32_t  _pad = 0;
    uint64_t data;
};

// ---- Aligned string view for internal game functions ----
// Same layout as std::string_view {data, length} but with 16-byte alignment.
// Required because CreateOrderInternal reads it with movaps (SSE aligned load).

struct alignas(16) AlignedStringView {
    const char* data;
    size_t      length;
};

/// How the order is inserted into the ship's order system.
///
/// From RE: CreateOrderInternal (0x140424A90), removal logic (0x141034E00).
///
///   Default (3): stored at entity[160], not in queue. Loops when queue empty.
///   Queue   (1): pushed to order vector [150..152], is_temp=false.
///                 Survives queue manipulation. On completion (state 8): reset
///                 via 0x14102EEE0 (can re-activate). For planned sequences.
///   Immediate(0): pushed to queue, is_temp=true.
///                 Always destroyed on removal/completion. Fire-and-forget
///                 interrupt — executes once, vanishes, previous behavior resumes.
///   Mode 2: identical to 0 (game checks ==3 and ==1 only).
///
enum class OrderMode : int {
    Immediate = 0,  // Interrupt: execute now, self-destruct when done, previous resumes
    Queue     = 1,  // Sequence: FIFO, persists through queue operations, can re-activate
    Default   = 3,  // Standing: loops indefinitely when queue empty (one at a time)
};

/// Lightweight typed handle for a ship entity.
/// Constructor validates the entity is a Ship (any size class).
class Ship {
    UniverseID id_;
    X4Component* comp_;

    // ---------------------------------------------------------------
    // Order param setters — private implementation detail.
    // Callers use convenience order methods (order_patrol, etc.)
    // which encapsulate param indices verified from AI script XML.
    //
    // Param type codes from x4_manual_types.h:
    //   2 = BOOL, 3 = NUMBER, 10 = ENTITY, 15 = SECTOR
    // ---------------------------------------------------------------

    bool set_order_param_entity(void* order, uint32_t idx, UniverseID uid) {
        if (!order) return false;
        auto* g = game();
        if (!g || !g->SetOrderParamInternal) return false;
        OrderParamValue val{}; val.type = X4_ORDER_PARAM_TYPE_ENTITY; val.data = uid;
        return g->SetOrderParamInternal(order, idx, &val) != 0;
    }

    bool set_order_param_bool(void* order, uint32_t idx, bool value) {
        if (!order) return false;
        auto* g = game();
        if (!g || !g->SetOrderParamInternal) return false;
        OrderParamValue val{}; val.type = X4_ORDER_PARAM_TYPE_BOOL; val.data = value ? 1 : 0;
        return g->SetOrderParamInternal(order, idx, &val) != 0;
    }

    bool set_order_param_number(void* order, uint32_t idx, int64_t value) {
        if (!order) return false;
        auto* g = game();
        if (!g || !g->SetOrderParamInternal) return false;
        OrderParamValue val{}; val.type = X4_ORDER_PARAM_TYPE_NUMBER; val.data = static_cast<uint64_t>(value);
        return g->SetOrderParamInternal(order, idx, &val) != 0;
    }

    bool set_order_param_sector(void* order, uint32_t idx, UniverseID uid) {
        if (!order) return false;
        auto* g = game();
        if (!g || !g->SetOrderParamInternal) return false;
        OrderParamValue val{}; val.type = X4_ORDER_PARAM_TYPE_SECTOR; val.data = uid;
        return g->SetOrderParamInternal(order, idx, &val) != 0;
    }

    /// Create an order on this ship. Returns order object pointer.
    /// Bypasses the player-ownership check in exported CreateOrder3.
    /// @param order_id  Order type name (e.g., "Patrol", "Attack", "TradeRoutine")
    /// @param mode      Queue (one-shot) or Default (standing/looping)
    void* create_order(const char* order_id, OrderMode mode = OrderMode::Default) {
        if (!valid()) return nullptr;
        auto* g = game();
        if (!g || !g->CreateOrderInternal) return nullptr;

        AlignedStringView sv{ order_id, std::strlen(order_id) };
        void* result = g->CreateOrderInternal(comp_, &sv, static_cast<int>(mode), 0);
        if (!result) {
            char buf[128];
            std::snprintf(buf, sizeof(buf),
                "x4n::ship: create_order('%s') returned null. entity=%p",
                order_id, (void*)comp_);
            x4n::log::info(buf);
        }
        return result;
    }

public:
    explicit Ship(UniverseID id)
        : id_(id), comp_(entity::find_component(id)) {
        if (comp_ && !entity::is_derived_from(comp_, GameClass::Ship))
            comp_ = nullptr;
    }

    bool valid() const { return comp_ != nullptr; }
    UniverseID id() const { return id_; }

    // ---------------------------------------------------------------
    // Order methods — public API.
    // Param indices verified via IDA + AI script XML declarations.
    // ---------------------------------------------------------------

    /// Patrol a sector. Engages hostiles on sight. Loops indefinitely.
    /// @param sector  0 = current zone
    bool order_patrol(UniverseID sector = 0, OrderMode mode = OrderMode::Default) {
        void* order = create_order("Patrol", mode);
        if (!order) return false;
        if (sector != 0) set_order_param_entity(order, 0, sector);
        return true;
    }

    /// Attack a specific target (ship or station).
    bool order_attack(UniverseID target, OrderMode mode = OrderMode::Default) {
        void* order = create_order("Attack", mode);
        if (!order) return false;
        return set_order_param_entity(order, 0, target);
    }

    /// Guard a station. Patrols station zone, responds to threats.
    bool order_protect_station(UniverseID station, OrderMode mode = OrderMode::Default) {
        void* order = create_order("ProtectStation", mode);
        if (!order) return false;
        return set_order_param_entity(order, 0, station);
    }

    /// Trade autonomously. Uses ship's ware basket from commander.
    bool order_trade_routine(OrderMode mode = OrderMode::Default) {
        return create_order("TradeRoutine", mode) != nullptr;
    }

    /// Mine autonomously. Uses ship's ware basket from commander.
    /// @param sector  0 = current area
    bool order_mining_routine(UniverseID sector = 0, OrderMode mode = OrderMode::Default) {
        void* order = create_order("MiningRoutine", mode);
        if (!order) return false;
        if (sector != 0) set_order_param_entity(order, 1, sector);
        return true;
    }

    /// Military reconnaissance. Threat assessment, intel reporting,
    /// fires 'recon update' signal. Deploys satellites by default.
    /// @param sector       Target sector to scout
    /// @param deploy_sats  Deploy satellites at points of interest
    bool order_recon(UniverseID sector, bool deploy_sats = true, OrderMode mode = OrderMode::Default) {
        void* order = create_order("Recon", mode);
        if (!order) return false;
        if (sector != 0) set_order_param_entity(order, 0, sector);
        set_order_param_bool(order, 10, deploy_sats);
        return true;
    }

    /// Escort a ship in formation. Breaks formation on combat.
    bool order_escort(UniverseID target, OrderMode mode = OrderMode::Default) {
        void* order = create_order("Escort", mode);
        if (!order) return false;
        return set_order_param_entity(order, 0, target);
    }

    /// Autonomous salvage collection.
    /// @param sector  0 = use job main sector / commander sector
    bool order_salvage(UniverseID sector = 0, OrderMode mode = OrderMode::Default) {
        void* order = create_order("SalvageRoutine", mode);
        if (!order) return false;
        if (sector != 0) set_order_param_sector(order, 0, sector);
        return true;
    }

    /// Fleet coordination from a flagship (L/XL). Coordinates subordinates.
    /// @param target         Ship or station to attack
    /// @param aggressiveness 0-100 (default 50)
    bool order_tactical(UniverseID target, int32_t aggressiveness = 50, OrderMode mode = OrderMode::Default) {
        void* order = create_order("TacticalOrder", mode);
        if (!order) return false;
        set_order_param_entity(order, 0, target);
        set_order_param_number(order, 1, aggressiveness);
        return true;
    }

    /// Piracy operations. Requires high pilot skill.
    /// @param sector  Operating area (0 = ship's cluster)
    bool order_plunder(UniverseID sector = 0, OrderMode mode = OrderMode::Default) {
        void* order = create_order("Plunder", mode);
        if (!order) return false;
        if (sector != 0) set_order_param_entity(order, 0, sector);
        return true;
    }

    /// Guard a specific ship (VIP escort).
    bool order_protect_ship(UniverseID target, OrderMode mode = OrderMode::Default) {
        void* order = create_order("ProtectShip", mode);
        if (!order) return false;
        return set_order_param_entity(order, 0, target);
    }

    /// Construction vessel seeks build tasks.
    bool order_supply_fleet(OrderMode mode = OrderMode::Default) {
        return create_order("SupplyFleet", mode) != nullptr;
    }
};

}} // namespace x4n::ship
