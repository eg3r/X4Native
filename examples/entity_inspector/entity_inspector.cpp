// ---------------------------------------------------------------------------
// x4native_entity_inspector — Deep Entity Inspection Example
//
// Demonstrates the X4Native SDK's low-level entity access APIs:
//   - x4n::entity        — find_component, get_component_macro, get_spawntime
//   - x4n::visibility    — radar, forced radar, known-to-all, faction counts
//   - x4n::math          — advance_seed (LCG), fnv1a_lower (hash)
//   - x4n::rooms         — roomtype_name enum conversion
//   - x4n::plans         — resolve_macro, resolve_connection
//   - x4n::memory        — game_alloc (SMem pool)
//   - X4Component struct — parent, combined_seed, exists flag (Object-hierarchy only)
//   - Vtable-based GetClassID + IsOrDerivedFromClassID (universal, all entity types)
//   - X4RadarChangedEvent — radar visibility change subscription
//   - X4NativeFrameUpdate — native frame tick with timing data
//
// All of these use the RVA-resolved internal functions and the hand-authored
// types from x4_manual_types.h. This example shows how extension authors can
// inspect game entities far beyond what the public C FFI exposes.
// ---------------------------------------------------------------------------
#include <x4native.h>

// Subscription handles
static int g_sub_loaded      = 0;
static int g_sub_radar       = 0;
static int g_sub_native_tick = 0;

// Periodic inspection state
static double g_accum = 0.0;
static constexpr double INSPECT_INTERVAL = 10.0;  // seconds


// ---------------------------------------------------------------------------
// Inspect a single entity — reads raw component memory + SDK helpers
// ---------------------------------------------------------------------------
static void inspect_entity(uint64_t id, const char* label) {
    auto* comp = x4n::entity::find_component(id);
    if (!comp) {
        x4n::log::warn("inspector: [%s] id=%llu — could not resolve component", label, id);
        return;
    }

    // --- Class ID via vtable (universal — works for ALL component types) ---
    auto cls = x4n::entity::game_class(comp);

    // --- Macro name via definition interface vtable (universal) ---
    const char* macro = x4n::entity::get_component_macro(comp);

    x4n::log::info("inspector: [%s] id=%llu class=%u(%s) macro=%s",
                   label, id, cls, x4n::entity::get_class_name(id),
                   macro ? macro : "(null)");

    // --- Object-hierarchy fields (only valid if IS-A "object") ---
    bool is_object = x4n::entity::is_a(comp, x4n::GameClass::Object);
    if (is_object) {
        bool    alive  = comp->exists != 0;
        void*   parent = comp->parent;
        uint64_t parent_id = parent
            ? reinterpret_cast<X4Component*>(parent)->id
            : 0;
        int64_t combined_seed = *reinterpret_cast<int64_t*>(
            reinterpret_cast<uintptr_t>(comp) + X4_COMPONENT_OFFSET_COMBINED_SEED);

        x4n::log::info("inspector: [%s]   object: alive=%s parent=%llu seed=%lld",
                       label, alive ? "yes" : "no", parent_id, combined_seed);

        if (combined_seed != 0) {
            uint64_t next = x4n::math::advance_seed(static_cast<uint64_t>(combined_seed));
            x4n::log::debug("inspector: [%s]   advance_seed(%lld) = %llu",
                            label, combined_seed, next);
        }

        // Visibility reads (radar, forced, map, known-to)
        bool radar       = x4n::visibility::get_radar_visible(id);
        bool forced      = x4n::visibility::get_forced_radar_visible(id);
        bool map_vis     = x4n::visibility::is_map_visible(id);
        bool known_all   = x4n::visibility::get_known_to_all(id);
        size_t fac_count = x4n::visibility::get_known_factions_count(id);

        x4n::log::info("inspector: [%s]   visibility: radar=%d forced=%d "
                       "map=%d known_all=%d factions=%zu",
                       label, radar, forced, map_vis, known_all, fac_count);
    }

    // --- Space-class visibility (clusters, sectors, zones — NOT Object-derived) ---
    bool is_space = x4n::entity::is_a(comp, x4n::GameClass::Space);
    if (is_space) {
        bool known_all   = x4n::visibility::get_space_known_to_all(id);
        size_t fac_count = x4n::visibility::get_space_known_factions_count(id);

        x4n::log::info("inspector: [%s]   space visibility: known_all=%d factions=%zu",
                       label, known_all, fac_count);
    }

    // --- Spawntime (Container-class: stations, ships) ---
    if (x4n::entity::is_a(comp, x4n::GameClass::Container)) {
        double spawn = x4n::entity::get_spawntime(id);
        if (spawn >= 0.0)
            x4n::log::info("inspector: [%s]   spawntime=%.1f", label, spawn);
    }
}

// ---------------------------------------------------------------------------
// Demonstrate macro/connection resolution and hash utilities
// ---------------------------------------------------------------------------
static void demo_plan_resolution() {
    // Resolve a well-known station macro
    const char* macro_name = "station_gen_factory_base_01_macro";
    void* macro_ptr = x4n::plans::resolve_macro(macro_name);

    if (macro_ptr) {
        x4n::log::info("inspector: resolve_macro('%s') = %p", macro_name, macro_ptr);

        // Attempt to resolve a connection on this macro
        void* conn = x4n::plans::resolve_connection(macro_ptr, "connection_build01");
        x4n::log::info("inspector:   resolve_connection('connection_build01') = %p",
                       conn);
    } else {
        x4n::log::debug("inspector: resolve_macro('%s') = nullptr (macro not loaded)",
                        macro_name);
    }

    // FNV-1a hash demo
    uint64_t hash = x4n::math::fnv1a_lower("connection_build01");
    x4n::log::info("inspector: fnv1a_lower('connection_build01') = 0x%llX", hash);
}

// ---------------------------------------------------------------------------
// Demonstrate room type enum
// ---------------------------------------------------------------------------
static void demo_room_types() {
    x4n::log::info("inspector: room type enum samples:");
    const X4RoomType samples[] = {
        X4_ROOMTYPE_BAR, X4_ROOMTYPE_MANAGER, X4_ROOMTYPE_PLAYEROFFICE,
        X4_ROOMTYPE_TRADERCORNER, X4_ROOMTYPE_WARROOM, X4_ROOMTYPE_NONE
    };
    for (auto rt : samples) {
        const char* name = x4n::rooms::roomtype_name(rt);
        x4n::log::info("inspector:   roomtype %d = '%s'", (int)rt,
                       name ? name : "(none/sentinel)");
    }
}

// ---------------------------------------------------------------------------
// Demonstrate game memory allocation
// ---------------------------------------------------------------------------
static void demo_game_alloc() {
    // Allocate a PlanEntry via the game's SMem pool — this is how X4Online
    // injects station modules. We allocate and immediately discard (leak is
    // intentional for demo — the game's pool will reclaim on shutdown).
    auto* entry = x4n::memory::game_alloc<X4PlanEntry>();
    if (entry) {
        x4n::log::info("inspector: game_alloc<X4PlanEntry>() = %p "
                       "(sizeof=%zu, alignof=%zu)",
                       entry, sizeof(X4PlanEntry), alignof(X4PlanEntry));
    } else {
        x4n::log::warn("inspector: game_alloc<X4PlanEntry>() = nullptr "
                       "(GameAlloc not available)");
    }
}

// ---------------------------------------------------------------------------
// Radar visibility change event
// ---------------------------------------------------------------------------
static void on_radar_changed(const X4RadarChangedEvent* e) {
    if (!e) return;

    const char* macro = x4n::entity::get_component_macro(e->entity_id);
    x4n::log::info("inspector: radar changed — entity=%llu visible=%d macro=%s",
                   e->entity_id, (int)e->visible,
                   macro ? macro : "(null)");
}

// ---------------------------------------------------------------------------
// Periodic inspection on native frame tick
// ---------------------------------------------------------------------------
static void on_native_tick(const X4NativeFrameUpdate* frame) {
    g_accum += frame->delta;
    if (g_accum < INSPECT_INTERVAL) return;
    g_accum = 0.0;

    auto* gf = x4n::game();
    if (!gf) return;

    // Inspect the player entity
    UniverseID player_id = gf->GetPlayerID ? gf->GetPlayerID() : 0;
    if (player_id)
        inspect_entity(player_id, "player");

    // Inspect the player's current ship
    UniverseID ship_id = gf->GetPlayerControlledShipID
                             ? gf->GetPlayerControlledShipID() : 0;
    if (ship_id && ship_id != player_id)
        inspect_entity(ship_id, "ship");

    // Inspect the player's current sector
    UniverseID sector_id = gf->GetContextByClass
                               ? gf->GetContextByClass(ship_id ? ship_id : player_id,
                                                        "sector", false)
                               : 0;
    if (sector_id)
        inspect_entity(sector_id, "sector");
}

// ---------------------------------------------------------------------------
// Game loaded — run one-time demos and start periodic inspection
// ---------------------------------------------------------------------------
static void on_game_loaded() {
    x4n::log::info("inspector: === game loaded — running demos ===");

    auto* gf = x4n::game();
    if (!gf) {
        x4n::log::error("inspector: game function table not available");
        return;
    }

    // One-shot entity inspection
    UniverseID player_id = gf->GetPlayerID ? gf->GetPlayerID() : 0;
    if (player_id) {
        inspect_entity(player_id, "player");

        UniverseID ship_id = gf->GetPlayerControlledShipID
                                 ? gf->GetPlayerControlledShipID() : 0;
        if (ship_id)
            inspect_entity(ship_id, "ship");
    }

    // Plan resolution demo
    demo_plan_resolution();

    // Room type enum demo
    demo_room_types();

    // Game allocator demo
    demo_game_alloc();

    x4n::log::info("inspector: === demos complete — periodic inspection active "
                   "(every %.0fs) ===", INSPECT_INTERVAL);
    g_accum = 0.0;
}

// ---------------------------------------------------------------------------
// Extension lifecycle
// ---------------------------------------------------------------------------

X4N_EXTENSION {
    x4n::log::info("entity_inspector: initializing");

    g_sub_loaded      = x4n::on("on_game_loaded",          on_game_loaded);
    g_sub_radar       = x4n::on("on_radar_changed",         on_radar_changed);
    g_sub_native_tick = x4n::on("on_native_frame_update",   on_native_tick);

    x4n::log::info("entity_inspector: subscribed (loaded=%d, radar=%d, tick=%d)",
                   g_sub_loaded, g_sub_radar, g_sub_native_tick);
}

X4N_SHUTDOWN {
    x4n::log::info("entity_inspector: shutting down");
    x4n::off(g_sub_loaded);
    x4n::off(g_sub_radar);
    x4n::off(g_sub_native_tick);
}
