// ==========================================================================
// x4_md_events.h - Typed MD Event Subscription API
// ==========================================================================
// Auto-generated from event_type_ids.csv + event_layouts.csv + common.xsd
// Game version: 900-605025
//
// Usage:
//   x4n::md::on_sector_changed_owner_before([](const x4n::md::SectorChangedOwnerData& e) {
//       // e.new_owner_faction, e.sector_changing_ownership
//   });
//
// DO NOT EDIT - regenerate with: .\scripts\generate_event_type_ids.ps1
// ==========================================================================
#pragma once
#include "x4n_events.h"
#include <cstdint>

namespace x4n::md
{

    namespace detail
    {
        template<typename T>
        void trampoline(const char*, void* data, void* ud) {
            auto* ev = static_cast<const X4MdEvent*>(data);
            auto d = T::from(ev);
            reinterpret_cast<void(*)(const T&)>(ud)(d);
        }

        // Internal subscribe helpers. Use the typed on_*_before/after functions below.
        using MdSubscribeFn = int(*)(uint32_t, X4NativeEventCallback, void*, void*);
        inline int subscribe_before(uint32_t type_id, X4NativeEventCallback cb, void* ud) {
            auto fn = reinterpret_cast<MdSubscribeFn>(x4n::detail::g_api->md_subscribe_before);
            return fn ? fn(type_id, cb, ud, x4n::detail::g_api) : -1;
        }
        inline int subscribe_after(uint32_t type_id, X4NativeEventCallback cb, void* ud) {
            auto fn = reinterpret_cast<MdSubscribeFn>(x4n::detail::g_api->md_subscribe_after);
            return fn ? fn(type_id, cb, ud, x4n::detail::g_api) : -1;
        }
    } // namespace detail


    /// Event for when the alert level of an object changes (object = object changing the alert level, pa..
    struct AlertLevelChangedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t new_alert_level_string;
        uint32_t old_alert_level_string;

        static AlertLevelChangedData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18),
                *reinterpret_cast<const uint32_t*>(p + 0x1C)
            };
        }
    };

    inline int on_alert_level_changed_before(void(*cb)(const AlertLevelChangedData&)) {
        return detail::subscribe_before(17, detail::trampoline<AlertLevelChangedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_alert_level_changed_after(void(*cb)(const AlertLevelChangedData&)) {
        return detail::subscribe_after(17, detail::trampoline<AlertLevelChangedData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when a target waypoint is approached (but not necessarily yet reached) by a flying obje..
    struct ApproachingWaypointData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t remaining_time;

        static ApproachingWaypointData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18)
            };
        }
    };

    inline int on_approaching_waypoint_before(void(*cb)(const ApproachingWaypointData&)) {
        return detail::subscribe_before(21, detail::trampoline<ApproachingWaypointData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_approaching_waypoint_after(void(*cb)(const ApproachingWaypointData&)) {
        return detail::subscribe_after(21, detail::trampoline<ApproachingWaypointData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when a flying object arrives at a target waypoint (object = arriving object)
    struct ArrivedAtWaypointData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        static ArrivedAtWaypointData from(const X4MdEvent* ev) { return { ev->source_id, ev->timestamp }; }
    };

    inline int on_arrived_at_waypoint_before(void(*cb)(const ArrivedAtWaypointData&)) {
        return detail::subscribe_before(23, detail::trampoline<ArrivedAtWaypointData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_arrived_at_waypoint_after(void(*cb)(const ArrivedAtWaypointData&)) {
        return detail::subscribe_after(23, detail::trampoline<ArrivedAtWaypointData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when a player owned object is attacked (object = attacked object, param = attacker, par..
    struct AttackedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t attacker;
        uint64_t kill_method;
        uint64_t attacked_component;

        static AttackedData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18),
                *reinterpret_cast<const uint64_t*>(p + 0x20),
                *reinterpret_cast<const uint64_t*>(p + 0x28)
            };
        }
    };

    inline int on_attacked_before(void(*cb)(const AttackedData&)) {
        return detail::subscribe_before(29, detail::trampoline<AttackedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_attacked_after(void(*cb)(const AttackedData&)) {
        return detail::subscribe_after(29, detail::trampoline<AttackedData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when an object signals it will attack a target (requires attribute attacker and/or targ..
    struct AttackStartedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t attacker;
        uint64_t target;

        static AttackStartedData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18),
                *reinterpret_cast<const uint64_t*>(p + 0x20)
            };
        }
    };

    inline int on_attack_started_before(void(*cb)(const AttackStartedData&)) {
        return detail::subscribe_before(31, detail::trampoline<AttackStartedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_attack_started_after(void(*cb)(const AttackStartedData&)) {
        return detail::subscribe_after(31, detail::trampoline<AttackStartedData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when an object signals it will stop an attack (requires attribute attacker and/or targe..
    struct AttackStoppedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t attacker;
        uint64_t target;

        static AttackStoppedData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18),
                *reinterpret_cast<const uint64_t*>(p + 0x20)
            };
        }
    };

    inline int on_attack_stopped_before(void(*cb)(const AttackStoppedData&)) {
        return detail::subscribe_before(32, detail::trampoline<AttackStoppedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_attack_stopped_after(void(*cb)(const AttackStoppedData&)) {
        return detail::subscribe_after(32, detail::trampoline<AttackStoppedData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when the player has disengaged the autopilot
    struct AutoPilotDeactivatedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        static AutoPilotDeactivatedData from(const X4MdEvent* ev) { return { ev->source_id, ev->timestamp }; }
    };

    inline int on_auto_pilot_deactivated_before(void(*cb)(const AutoPilotDeactivatedData&)) {
        return detail::subscribe_before(33, detail::trampoline<AutoPilotDeactivatedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_auto_pilot_deactivated_after(void(*cb)(const AutoPilotDeactivatedData&)) {
        return detail::subscribe_after(33, detail::trampoline<AutoPilotDeactivatedData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when the player has engaged the autopilot (param = target, param2 = next target on the ..
    struct AutoPilotTargetSetData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t target;
        uint64_t next_target_on_the_way_to_targ;

        static AutoPilotTargetSetData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18),
                *reinterpret_cast<const uint64_t*>(p + 0x20)
            };
        }
    };

    inline int on_auto_pilot_target_set_before(void(*cb)(const AutoPilotTargetSetData&)) {
        return detail::subscribe_before(34, detail::trampoline<AutoPilotTargetSetData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_auto_pilot_target_set_after(void(*cb)(const AutoPilotTargetSetData&)) {
        return detail::subscribe_after(34, detail::trampoline<AutoPilotTargetSetData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when a boarding operation has been created (param = boarding operation)
    struct BoardingOperationCreatedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t boarding_operation;

        static BoardingOperationCreatedData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18)
            };
        }
    };

    inline int on_boarding_operation_created_before(void(*cb)(const BoardingOperationCreatedData&)) {
        return detail::subscribe_before(38, detail::trampoline<BoardingOperationCreatedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_boarding_operation_created_after(void(*cb)(const BoardingOperationCreatedData&)) {
        return detail::subscribe_after(38, detail::trampoline<BoardingOperationCreatedData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when a boarding operation has been removed (param = boarding operation)
    struct BoardingOperationRemovedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t boarding_operation;

        static BoardingOperationRemovedData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18)
            };
        }
    };

    inline int on_boarding_operation_removed_before(void(*cb)(const BoardingOperationRemovedData&)) {
        return detail::subscribe_before(39, detail::trampoline<BoardingOperationRemovedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_boarding_operation_removed_after(void(*cb)(const BoardingOperationRemovedData&)) {
        return detail::subscribe_after(39, detail::trampoline<BoardingOperationRemovedData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when a boarding operation has been started (param = boarding operation)
    struct BoardingOperationStartedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t boarding_operation;

        static BoardingOperationStartedData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18)
            };
        }
    };

    inline int on_boarding_operation_started_before(void(*cb)(const BoardingOperationStartedData&)) {
        return detail::subscribe_before(40, detail::trampoline<BoardingOperationStartedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_boarding_operation_started_after(void(*cb)(const BoardingOperationStartedData&)) {
        return detail::subscribe_after(40, detail::trampoline<BoardingOperationStartedData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when a boarding operation has had its phase changed (param = boarding operation, param2..
    struct BoardingPhaseChangedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t boarding_operation;
        uint32_t new_boardingphase;
        uint32_t old_boardingphase;

        static BoardingPhaseChangedData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18),
                *reinterpret_cast<const uint32_t*>(p + 0x20),
                *reinterpret_cast<const uint32_t*>(p + 0x24)
            };
        }
    };

    inline int on_boarding_phase_changed_before(void(*cb)(const BoardingPhaseChangedData&)) {
        return detail::subscribe_before(41, detail::trampoline<BoardingPhaseChangedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_boarding_phase_changed_after(void(*cb)(const BoardingPhaseChangedData&)) {
        return detail::subscribe_after(41, detail::trampoline<BoardingPhaseChangedData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when a boarding support mission was NOT successful (object = boarding commander)
    struct BoardingSupportFailedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        static BoardingSupportFailedData from(const X4MdEvent* ev) { return { ev->source_id, ev->timestamp }; }
    };

    inline int on_boarding_support_failed_before(void(*cb)(const BoardingSupportFailedData&)) {
        return detail::subscribe_before(42, detail::trampoline<BoardingSupportFailedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_boarding_support_failed_after(void(*cb)(const BoardingSupportFailedData&)) {
        return detail::subscribe_after(42, detail::trampoline<BoardingSupportFailedData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when a boarding support mission was successful (object = boarding commander)
    struct BoardingSupportSucceededData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        static BoardingSupportSucceededData from(const X4MdEvent* ev) { return { ev->source_id, ev->timestamp }; }
    };

    inline int on_boarding_support_succeeded_before(void(*cb)(const BoardingSupportSucceededData&)) {
        return detail::subscribe_before(44, detail::trampoline<BoardingSupportSucceededData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_boarding_support_succeeded_after(void(*cb)(const BoardingSupportSucceededData&)) {
        return detail::subscribe_after(44, detail::trampoline<BoardingSupportSucceededData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event is raised when the player or a player-owned object has successfully attached a bomb (param ..
    struct BombAttachedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t bomb_that_was_attached;

        static BombAttachedData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18)
            };
        }
    };

    inline int on_bomb_attached_before(void(*cb)(const BombAttachedData&)) {
        return detail::subscribe_before(46, detail::trampoline<BombAttachedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_bomb_attached_after(void(*cb)(const BombAttachedData&)) {
        return detail::subscribe_after(46, detail::trampoline<BombAttachedData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event is raised when a bomb has been successfully defused (object = the bomb that was defused, th..
    struct BombDefusedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t bomb_that_was_defused;

        static BombDefusedData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18)
            };
        }
    };

    inline int on_bomb_defused_before(void(*cb)(const BombDefusedData&)) {
        return detail::subscribe_before(47, detail::trampoline<BombDefusedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_bomb_defused_after(void(*cb)(const BombDefusedData&)) {
        return detail::subscribe_after(47, detail::trampoline<BombDefusedData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when an object starts boosting (object = boosting object)
    struct BoostStartedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        static BoostStartedData from(const X4MdEvent* ev) { return { ev->source_id, ev->timestamp }; }
    };

    inline int on_boost_started_before(void(*cb)(const BoostStartedData&)) {
        return detail::subscribe_before(52, detail::trampoline<BoostStartedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_boost_started_after(void(*cb)(const BoostStartedData&)) {
        return detail::subscribe_after(52, detail::trampoline<BoostStartedData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when an object stops boosting (object = boosting object)
    struct BoostStoppedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        static BoostStoppedData from(const X4MdEvent* ev) { return { ev->source_id, ev->timestamp }; }
    };

    inline int on_boost_stopped_before(void(*cb)(const BoostStoppedData&)) {
        return detail::subscribe_before(53, detail::trampoline<BoostStoppedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_boost_stopped_after(void(*cb)(const BoostStoppedData&)) {
        return detail::subscribe_after(53, detail::trampoline<BoostStoppedData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when a build related to the object is cancelled (object = buildingobject, param = build..
    struct BuildCancelledData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t buildtask;

        static BuildCancelledData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18)
            };
        }
    };

    inline int on_build_cancelled_before(void(*cb)(const BuildCancelledData&)) {
        return detail::subscribe_before(54, detail::trampoline<BuildCancelledData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_build_cancelled_after(void(*cb)(const BuildCancelledData&)) {
        return detail::subscribe_after(54, detail::trampoline<BuildCancelledData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when a build related to the object finishes (object = buildprocessor/buildmodule/buildi..
    struct BuildFinishedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t buildprocessor;
        uint64_t unused;

        static BuildFinishedData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18),
                *reinterpret_cast<const uint64_t*>(p + 0x20)
            };
        }
    };

    inline int on_build_finished_before(void(*cb)(const BuildFinishedData&)) {
        return detail::subscribe_before(55, detail::trampoline<BuildFinishedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_build_finished_after(void(*cb)(const BuildFinishedData&)) {
        return detail::subscribe_after(55, detail::trampoline<BuildFinishedData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when a build related to the object partially finishes construction. List of completed c..
    struct BuildFinishedComponentsData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint32_t buildprocessor;
        uint32_t list;
        uint64_t buildtask;

        static BuildFinishedComponentsData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint32_t*>(p + 0x18),
                *reinterpret_cast<const uint32_t*>(p + 0x20),
                *reinterpret_cast<const uint64_t*>(p + 0x28)
            };
        }
    };

    inline int on_build_finished_components_before(void(*cb)(const BuildFinishedComponentsData&)) {
        return detail::subscribe_before(56, detail::trampoline<BuildFinishedComponentsData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_build_finished_components_after(void(*cb)(const BuildFinishedComponentsData&)) {
        return detail::subscribe_after(56, detail::trampoline<BuildFinishedComponentsData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when a player-owned station's build plot is resized or repositioned (param = station, p..
    struct BuildPlotChangedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t station;
        uint64_t old_build_plot_max;
        uint64_t old_build_plot_center;

        static BuildPlotChangedData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18),
                *reinterpret_cast<const uint64_t*>(p + 0x20),
                *reinterpret_cast<const uint64_t*>(p + 0x30)
            };
        }
    };

    inline int on_build_plot_changed_before(void(*cb)(const BuildPlotChangedData&)) {
        return detail::subscribe_before(58, detail::trampoline<BuildPlotChangedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_build_plot_changed_after(void(*cb)(const BuildPlotChangedData&)) {
        return detail::subscribe_after(58, detail::trampoline<BuildPlotChangedData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when a build related to the object begins (object = buildprocessor/buildmodule/building..
    struct BuildStartedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t buildprocessor;
        uint64_t unused;

        static BuildStartedData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18),
                *reinterpret_cast<const uint64_t*>(p + 0x20)
            };
        }
    };

    inline int on_build_started_before(void(*cb)(const BuildStartedData&)) {
        return detail::subscribe_before(60, detail::trampoline<BuildStartedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_build_started_after(void(*cb)(const BuildStartedData&)) {
        return detail::subscribe_after(60, detail::trampoline<BuildStartedData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when a build storage has been moved (object = build storage or the base station)
    struct BuildStorageMovedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        static BuildStorageMovedData from(const X4MdEvent* ev) { return { ev->source_id, ev->timestamp }; }
    };

    inline int on_build_storage_moved_before(void(*cb)(const BuildStorageMovedData&)) {
        return detail::subscribe_before(61, detail::trampoline<BuildStorageMovedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_build_storage_moved_after(void(*cb)(const BuildStorageMovedData&)) {
        return detail::subscribe_after(61, detail::trampoline<BuildStorageMovedData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when a subordinate changes assignments without changing commander (object = source play..
    struct ChangedAssignmentData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t subordinate;
        uint64_t previous_assignment;
        uint64_t new_assignment;

        static ChangedAssignmentData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18),
                *reinterpret_cast<const uint64_t*>(p + 0x20),
                *reinterpret_cast<const uint64_t*>(p + 0x28)
            };
        }
    };

    inline int on_changed_assignment_before(void(*cb)(const ChangedAssignmentData&)) {
        return detail::subscribe_before(68, detail::trampoline<ChangedAssignmentData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_changed_assignment_after(void(*cb)(const ChangedAssignmentData&)) {
        return detail::subscribe_after(68, detail::trampoline<ChangedAssignmentData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for the specified object changing attention level (object = attention changing object, para..
    struct ChangedAttentionData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint32_t new_attention_level;
        uint32_t previous_attention_level;

        static ChangedAttentionData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint32_t*>(p + 0x18),
                *reinterpret_cast<const uint32_t*>(p + 0x1C)
            };
        }
    };

    inline int on_changed_attention_before(void(*cb)(const ChangedAttentionData&)) {
        return detail::subscribe_before(69, detail::trampoline<ChangedAttentionData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_changed_attention_after(void(*cb)(const ChangedAttentionData&)) {
        return detail::subscribe_after(69, detail::trampoline<ChangedAttentionData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for the specified object changing cluster (object = cluster changing object, param = new cl..
    struct ChangedClusterData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t new_cluster;
        uint64_t previous_cluster;

        static ChangedClusterData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18),
                *reinterpret_cast<const uint64_t*>(p + 0x20)
            };
        }
    };

    inline int on_changed_cluster_before(void(*cb)(const ChangedClusterData&)) {
        return detail::subscribe_before(70, detail::trampoline<ChangedClusterData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_changed_cluster_after(void(*cb)(const ChangedClusterData&)) {
        return detail::subscribe_after(70, detail::trampoline<ChangedClusterData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for the specified object changing object (object = object changing object, param = new obje..
    struct ChangedObjectData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t new_object;
        uint64_t previous_object;

        static ChangedObjectData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18),
                *reinterpret_cast<const uint64_t*>(p + 0x20)
            };
        }
    };

    inline int on_changed_object_before(void(*cb)(const ChangedObjectData&)) {
        return detail::subscribe_before(76, detail::trampoline<ChangedObjectData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_changed_object_after(void(*cb)(const ChangedObjectData&)) {
        return detail::subscribe_after(76, detail::trampoline<ChangedObjectData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for any sector within the specified space changing owner (object = space which contains the..
    struct ChangedOwnerData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t sector_changing_ownership;
        uint64_t new_owner_faction;
        uint64_t previous_owner_faction;

        static ChangedOwnerData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18),
                *reinterpret_cast<const uint64_t*>(p + 0x20),
                *reinterpret_cast<const uint64_t*>(p + 0x40)
            };
        }
    };

    inline int on_changed_owner_before(void(*cb)(const ChangedOwnerData&)) {
        return detail::subscribe_before(77, detail::trampoline<ChangedOwnerData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_changed_owner_after(void(*cb)(const ChangedOwnerData&)) {
        return detail::subscribe_after(77, detail::trampoline<ChangedOwnerData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for the specified object changing room (object = room changing object, param = new room, pa..
    struct ChangedRoomData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t new_room;
        uint64_t previous_room;

        static ChangedRoomData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18),
                *reinterpret_cast<const uint64_t*>(p + 0x20)
            };
        }
    };

    inline int on_changed_room_before(void(*cb)(const ChangedRoomData&)) {
        return detail::subscribe_before(80, detail::trampoline<ChangedRoomData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_changed_room_after(void(*cb)(const ChangedRoomData&)) {
        return detail::subscribe_after(80, detail::trampoline<ChangedRoomData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for the specified object changing sector - sector can be null in case of a superhighway (ob..
    struct ChangedSectorData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t new_sector_or_null;
        uint64_t previous_sector_or_null;

        static ChangedSectorData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18),
                *reinterpret_cast<const uint64_t*>(p + 0x20)
            };
        }
    };

    inline int on_changed_sector_before(void(*cb)(const ChangedSectorData&)) {
        return detail::subscribe_before(82, detail::trampoline<ChangedSectorData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_changed_sector_after(void(*cb)(const ChangedSectorData&)) {
        return detail::subscribe_after(82, detail::trampoline<ChangedSectorData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for the specified object changing state (object = state changing object, param = new state,..
    struct ChangedStateData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t new_state;
        uint32_t previous_state;
        uint64_t perform_transition;

        static ChangedStateData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18),
                *reinterpret_cast<const uint32_t*>(p + 0x1C),
                *reinterpret_cast<const uint64_t*>(p + 0x20)
            };
        }
    };

    inline int on_changed_state_before(void(*cb)(const ChangedStateData&)) {
        return detail::subscribe_before(83, detail::trampoline<ChangedStateData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_changed_state_after(void(*cb)(const ChangedStateData&)) {
        return detail::subscribe_after(83, detail::trampoline<ChangedStateData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when a subordinate changes subordinate groups (object = subordinate/commander, param = ..
    struct ChangedSubordinateGroupData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t subordinate;
        uint64_t previous_group;
        uint32_t new_group;

        static ChangedSubordinateGroupData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18),
                *reinterpret_cast<const uint64_t*>(p + 0x20),
                *reinterpret_cast<const uint32_t*>(p + 0x28)
            };
        }
    };

    inline int on_changed_subordinate_group_before(void(*cb)(const ChangedSubordinateGroupData&)) {
        return detail::subscribe_before(84, detail::trampoline<ChangedSubordinateGroupData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_changed_subordinate_group_after(void(*cb)(const ChangedSubordinateGroupData&)) {
        return detail::subscribe_after(84, detail::trampoline<ChangedSubordinateGroupData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for any entity within the specified space changing true owner (object = space or container ..
    struct ChangedTrueOwnerData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t entity_changing_ownership;
        uint64_t new_true_owner_faction;
        uint64_t previous_true_owner_faction;

        static ChangedTrueOwnerData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18),
                *reinterpret_cast<const uint64_t*>(p + 0x20),
                *reinterpret_cast<const uint64_t*>(p + 0x40)
            };
        }
    };

    inline int on_changed_true_owner_before(void(*cb)(const ChangedTrueOwnerData&)) {
        return detail::subscribe_before(85, detail::trampoline<ChangedTrueOwnerData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_changed_true_owner_after(void(*cb)(const ChangedTrueOwnerData&)) {
        return detail::subscribe_after(85, detail::trampoline<ChangedTrueOwnerData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for the specified object changing zone (object = zone changing object, param = new zone, pa..
    struct ChangedZoneData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t new_zone;
        uint64_t previous_zone;

        static ChangedZoneData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18),
                *reinterpret_cast<const uint64_t*>(p + 0x20)
            };
        }
    };

    inline int on_changed_zone_before(void(*cb)(const ChangedZoneData&)) {
        return detail::subscribe_before(86, detail::trampoline<ChangedZoneData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_changed_zone_after(void(*cb)(const ChangedZoneData&)) {
        return detail::subscribe_after(86, detail::trampoline<ChangedZoneData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when a character finishes an animation. (object = player/npc, param = animation ID, par..
    struct CharacterAnimationFinishedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t animation_id;
        uint64_t remaining_time_for_blending_to;

        static CharacterAnimationFinishedData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18),
                *reinterpret_cast<const uint64_t*>(p + 0x48)
            };
        }
    };

    inline int on_character_animation_finished_before(void(*cb)(const CharacterAnimationFinishedData&)) {
        return detail::subscribe_before(92, detail::trampoline<CharacterAnimationFinishedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_character_animation_finished_after(void(*cb)(const CharacterAnimationFinishedData&)) {
        return detail::subscribe_after(92, detail::trampoline<CharacterAnimationFinishedData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when a character starts an animation. (object = player/npc, param = animation ID)
    struct CharacterAnimationStartedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t animation_id;

        static CharacterAnimationStartedData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18)
            };
        }
    };

    inline int on_character_animation_started_before(void(*cb)(const CharacterAnimationStartedData&)) {
        return detail::subscribe_before(93, detail::trampoline<CharacterAnimationStartedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_character_animation_started_after(void(*cb)(const CharacterAnimationStartedData&)) {
        return detail::subscribe_after(93, detail::trampoline<CharacterAnimationStartedData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for the specified object collecting ammo (object = collecting object, param = weapon macro,..
    struct CollectedAmmoData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t weapon_macro;
        uint64_t amount;

        static CollectedAmmoData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18),
                *reinterpret_cast<const uint64_t*>(p + 0x20)
            };
        }
    };

    inline int on_collected_ammo_before(void(*cb)(const CollectedAmmoData&)) {
        return detail::subscribe_before(101, detail::trampoline<CollectedAmmoData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_collected_ammo_after(void(*cb)(const CollectedAmmoData&)) {
        return detail::subscribe_after(101, detail::trampoline<CollectedAmmoData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when the player collects a powerup
    struct CollectedPowerUpData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        static CollectedPowerUpData from(const X4MdEvent* ev) { return { ev->source_id, ev->timestamp }; }
    };

    inline int on_collected_power_up_before(void(*cb)(const CollectedPowerUpData&)) {
        return detail::subscribe_before(102, detail::trampoline<CollectedPowerUpData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_collected_power_up_after(void(*cb)(const CollectedPowerUpData&)) {
        return detail::subscribe_after(102, detail::trampoline<CollectedPowerUpData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when a player owned object has collected a resource (object = collecting object, param ..
    struct CollectedResourceData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t ware;
        uint64_t amount;

        static CollectedResourceData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18),
                *reinterpret_cast<const uint64_t*>(p + 0x20)
            };
        }
    };

    inline int on_collected_resource_before(void(*cb)(const CollectedResourceData&)) {
        return detail::subscribe_before(103, detail::trampoline<CollectedResourceData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_collected_resource_after(void(*cb)(const CollectedResourceData&)) {
        return detail::subscribe_after(103, detail::trampoline<CollectedResourceData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when the player collects a ware drop (param = ware, param2 = amount, param3 = collected..
    struct CollectedWareData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t ware;
        uint64_t amount;
        uint64_t collected_object;

        static CollectedWareData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18),
                *reinterpret_cast<const uint64_t*>(p + 0x20),
                *reinterpret_cast<const uint64_t*>(p + 0x28)
            };
        }
    };

    inline int on_collected_ware_before(void(*cb)(const CollectedWareData&)) {
        return detail::subscribe_before(104, detail::trampoline<CollectedWareData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_collected_ware_after(void(*cb)(const CollectedWareData&)) {
        return detail::subscribe_after(104, detail::trampoline<CollectedWareData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when the player unsuccessfully attempts to collect a ware drop on collision (param = wa..
    struct CollectWareFailedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t ware;
        uint64_t amount;
        uint64_t crate_object_that_cannot_be_co;

        static CollectWareFailedData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18),
                *reinterpret_cast<const uint64_t*>(p + 0x20),
                *reinterpret_cast<const uint64_t*>(p + 0x28)
            };
        }
    };

    inline int on_collect_ware_failed_before(void(*cb)(const CollectWareFailedData&)) {
        return detail::subscribe_before(105, detail::trampoline<CollectWareFailedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_collect_ware_failed_after(void(*cb)(const CollectWareFailedData&)) {
        return detail::subscribe_after(105, detail::trampoline<CollectWareFailedData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when a subordinate receives a new commander (object = subordinate, param = new commande..
    struct CommanderSetData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t new_commander;
        uint64_t previous_commander;
        uint64_t new_assignment;

        static CommanderSetData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18),
                *reinterpret_cast<const uint64_t*>(p + 0x20),
                *reinterpret_cast<const uint64_t*>(p + 0x28)
            };
        }
    };

    inline int on_commander_set_before(void(*cb)(const CommanderSetData&)) {
        return detail::subscribe_before(107, detail::trampoline<CommanderSetData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_commander_set_after(void(*cb)(const CommanderSetData&)) {
        return detail::subscribe_after(107, detail::trampoline<CommanderSetData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when a component has been dismantled by a dismantling processor. (object = dismantled o..
    struct ComponentDismantledData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t dismantled_component;
        uint64_t dismantling_object;

        static ComponentDismantledData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18),
                *reinterpret_cast<const uint64_t*>(p + 0x20)
            };
        }
    };

    inline int on_component_dismantled_before(void(*cb)(const ComponentDismantledData&)) {
        return detail::subscribe_before(108, detail::trampoline<ComponentDismantledData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_component_dismantled_after(void(*cb)(const ComponentDismantledData&)) {
        return detail::subscribe_after(108, detail::trampoline<ComponentDismantledData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when a control entity is set on a controllable (object = controllable or entity, param ..
    struct ControlEntityAddedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t entity;

        static ControlEntityAddedData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18)
            };
        }
    };

    inline int on_control_entity_added_before(void(*cb)(const ControlEntityAddedData&)) {
        return detail::subscribe_before(110, detail::trampoline<ControlEntityAddedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_control_entity_added_after(void(*cb)(const ControlEntityAddedData&)) {
        return detail::subscribe_after(110, detail::trampoline<ControlEntityAddedData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when a control entity is removed from a controllable (object = controllable or entity, ..
    struct ControlEntityRemovedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t entity;
        uint32_t kill_method;

        static ControlEntityRemovedData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18),
                *reinterpret_cast<const uint32_t*>(p + 0x20)
            };
        }
    };

    inline int on_control_entity_removed_before(void(*cb)(const ControlEntityRemovedData&)) {
        return detail::subscribe_before(111, detail::trampoline<ControlEntityRemovedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_control_entity_removed_after(void(*cb)(const ControlEntityRemovedData&)) {
        return detail::subscribe_after(111, detail::trampoline<ControlEntityRemovedData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when a conversation is finished (object = actor (entity or list containing context and ..
    struct ConversationFinishedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t outcome;
        uint64_t outcome_parameter;

        static ConversationFinishedData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18),
                *reinterpret_cast<const uint64_t*>(p + 0x20)
            };
        }
    };

    inline int on_conversation_finished_before(void(*cb)(const ConversationFinishedData&)) {
        return detail::subscribe_before(114, detail::trampoline<ConversationFinishedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_conversation_finished_after(void(*cb)(const ConversationFinishedData&)) {
        return detail::subscribe_after(114, detail::trampoline<ConversationFinishedData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when the next section in a conversation is triggered (object = actor (entity or list co..
    struct ConversationNextSectionData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t section_name;
        uint64_t choice_parameter;

        static ConversationNextSectionData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18),
                *reinterpret_cast<const uint64_t*>(p + 0x20)
            };
        }
    };

    inline int on_conversation_next_section_before(void(*cb)(const ConversationNextSectionData&)) {
        return detail::subscribe_before(115, detail::trampoline<ConversationNextSectionData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_conversation_next_section_after(void(*cb)(const ConversationNextSectionData&)) {
        return detail::subscribe_after(115, detail::trampoline<ConversationNextSectionData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when a sub-conversation returns to the previous section (object = actor (entity or list..
    struct ConversationReturnedToSectionData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t section_name;
        uint64_t base_parameter;
        uint64_t return_parameter;

        static ConversationReturnedToSectionData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x38),
                *reinterpret_cast<const uint64_t*>(p + 0x40),
                *reinterpret_cast<const uint64_t*>(p + 0x48)
            };
        }
    };

    inline int on_conversation_returned_to_section_before(void(*cb)(const ConversationReturnedToSectionData&)) {
        return detail::subscribe_before(116, detail::trampoline<ConversationReturnedToSectionData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_conversation_returned_to_section_after(void(*cb)(const ConversationReturnedToSectionData&)) {
        return detail::subscribe_after(116, detail::trampoline<ConversationReturnedToSectionData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when a conversation is started (object = actor (entity or list containing context and t..
    struct ConversationStartedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t conversation_name;
        uint64_t conversation_parameter;

        static ConversationStartedData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x38),
                *reinterpret_cast<const uint64_t*>(p + 0x40)
            };
        }
    };

    inline int on_conversation_started_before(void(*cb)(const ConversationStartedData&)) {
        return detail::subscribe_before(117, detail::trampoline<ConversationStartedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_conversation_started_after(void(*cb)(const ConversationStartedData&)) {
        return detail::subscribe_after(117, detail::trampoline<ConversationStartedData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for countermeasure being launched (object = the space which the countermeasure was launched..
    struct CountermeasureLaunchedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t the_launching_defensible;
        uint64_t launched_countermeasure;

        static CountermeasureLaunchedData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18),
                *reinterpret_cast<const uint64_t*>(p + 0x48)
            };
        }
    };

    inline int on_countermeasure_launched_before(void(*cb)(const CountermeasureLaunchedData&)) {
        return detail::subscribe_before(119, detail::trampoline<CountermeasureLaunchedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_countermeasure_launched_after(void(*cb)(const CountermeasureLaunchedData&)) {
        return detail::subscribe_after(119, detail::trampoline<CountermeasureLaunchedData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when the camera of a cutscene is created (param = cutscene key string, param2 = cutscen..
    struct CutsceneCameraCreatedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t cutscene_key_string;
        uint32_t cutscene_id;

        static CutsceneCameraCreatedData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18),
                *reinterpret_cast<const uint32_t*>(p + 0x28)
            };
        }
    };

    inline int on_cutscene_camera_created_before(void(*cb)(const CutsceneCameraCreatedData&)) {
        return detail::subscribe_before(123, detail::trampoline<CutsceneCameraCreatedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_cutscene_camera_created_after(void(*cb)(const CutsceneCameraCreatedData&)) {
        return detail::subscribe_after(123, detail::trampoline<CutsceneCameraCreatedData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when a cutscene has been started and loading of required assets for playing the cutscen..
    struct CutsceneReadyData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t cutscene_key_string;
        uint64_t cutscene_id;

        static CutsceneReadyData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18),
                *reinterpret_cast<const uint64_t*>(p + 0x20)
            };
        }
    };

    inline int on_cutscene_ready_before(void(*cb)(const CutsceneReadyData&)) {
        return detail::subscribe_before(124, detail::trampoline<CutsceneReadyData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_cutscene_ready_after(void(*cb)(const CutsceneReadyData&)) {
        return detail::subscribe_after(124, detail::trampoline<CutsceneReadyData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when a cutscene sends a signal (param = cutscene key string)
    struct CutsceneSignalData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t cutscene_key_string;

        static CutsceneSignalData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x68)
            };
        }
    };

    inline int on_cutscene_signal_before(void(*cb)(const CutsceneSignalData&)) {
        return detail::subscribe_before(125, detail::trampoline<CutsceneSignalData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_cutscene_signal_after(void(*cb)(const CutsceneSignalData&)) {
        return detail::subscribe_after(125, detail::trampoline<CutsceneSignalData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when a cutscene starts (param = cutscene key string, param2 = cutscene ID, param3 = tru..
    struct CutsceneStartedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t cutscene_key_string;

        static CutsceneStartedData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18)
            };
        }
    };

    inline int on_cutscene_started_before(void(*cb)(const CutsceneStartedData&)) {
        return detail::subscribe_before(126, detail::trampoline<CutsceneStartedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_cutscene_started_after(void(*cb)(const CutsceneStartedData&)) {
        return detail::subscribe_after(126, detail::trampoline<CutsceneStartedData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when a cutscene ends (param = cutscene key string, param2 = cutscene ID, param3 = true ..
    struct CutsceneStoppedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint32_t cutscene_key_string;
        uint32_t cutscene_id;

        static CutsceneStoppedData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint32_t*>(p + 0x38),
                *reinterpret_cast<const uint32_t*>(p + 0x40)
            };
        }
    };

    inline int on_cutscene_stopped_before(void(*cb)(const CutsceneStoppedData&)) {
        return detail::subscribe_before(127, detail::trampoline<CutsceneStoppedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_cutscene_stopped_after(void(*cb)(const CutsceneStoppedData&)) {
        return detail::subscribe_after(127, detail::trampoline<CutsceneStoppedData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for the player cycling the ammunition of active weapons
    struct CycledActiveWeaponAmmoData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        static CycledActiveWeaponAmmoData from(const X4MdEvent* ev) { return { ev->source_id, ev->timestamp }; }
    };

    inline int on_cycled_active_weapon_ammo_before(void(*cb)(const CycledActiveWeaponAmmoData&)) {
        return detail::subscribe_before(128, detail::trampoline<CycledActiveWeaponAmmoData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_cycled_active_weapon_ammo_after(void(*cb)(const CycledActiveWeaponAmmoData&)) {
        return detail::subscribe_after(128, detail::trampoline<CycledActiveWeaponAmmoData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when an object has been detached from all mass traffic networks that restrict its movem..
    struct DetachedFromMassTrafficData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        static DetachedFromMassTrafficData from(const X4MdEvent* ev) { return { ev->source_id, ev->timestamp }; }
    };

    inline int on_detached_from_mass_traffic_before(void(*cb)(const DetachedFromMassTrafficData&)) {
        return detail::subscribe_before(135, detail::trampoline<DetachedFromMassTrafficData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_detached_from_mass_traffic_after(void(*cb)(const DetachedFromMassTrafficData&)) {
        return detail::subscribe_after(135, detail::trampoline<DetachedFromMassTrafficData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when a diplomacy action operation has been aborted (param = diplomacy operation)
    struct DiplomacyActionOperationAbortedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t diplomacy_operation;

        static DiplomacyActionOperationAbortedData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18)
            };
        }
    };

    inline int on_diplomacy_action_operation_aborted_before(void(*cb)(const DiplomacyActionOperationAbortedData&)) {
        return detail::subscribe_before(137, detail::trampoline<DiplomacyActionOperationAbortedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_diplomacy_action_operation_aborted_after(void(*cb)(const DiplomacyActionOperationAbortedData&)) {
        return detail::subscribe_after(137, detail::trampoline<DiplomacyActionOperationAbortedData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when a diplomacy action operation has been completed (param = diplomacy operation)
    struct DiplomacyActionOperationCompletedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t diplomacy_operation;

        static DiplomacyActionOperationCompletedData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18)
            };
        }
    };

    inline int on_diplomacy_action_operation_completed_before(void(*cb)(const DiplomacyActionOperationCompletedData&)) {
        return detail::subscribe_before(138, detail::trampoline<DiplomacyActionOperationCompletedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_diplomacy_action_operation_completed_after(void(*cb)(const DiplomacyActionOperationCompletedData&)) {
        return detail::subscribe_after(138, detail::trampoline<DiplomacyActionOperationCompletedData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when a diplomacy action operation has been created (param = diplomacy operation)
    struct DiplomacyActionOperationCreatedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t diplomacy_operation;

        static DiplomacyActionOperationCreatedData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18)
            };
        }
    };

    inline int on_diplomacy_action_operation_created_before(void(*cb)(const DiplomacyActionOperationCreatedData&)) {
        return detail::subscribe_before(139, detail::trampoline<DiplomacyActionOperationCreatedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_diplomacy_action_operation_created_after(void(*cb)(const DiplomacyActionOperationCreatedData&)) {
        return detail::subscribe_after(139, detail::trampoline<DiplomacyActionOperationCreatedData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when a diplomacy action operation has been started (param = diplomacy operation)
    struct DiplomacyActionOperationStartedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t diplomacy_operation;

        static DiplomacyActionOperationStartedData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18)
            };
        }
    };

    inline int on_diplomacy_action_operation_started_before(void(*cb)(const DiplomacyActionOperationStartedData&)) {
        return detail::subscribe_before(140, detail::trampoline<DiplomacyActionOperationStartedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_diplomacy_action_operation_started_after(void(*cb)(const DiplomacyActionOperationStartedData&)) {
        return detail::subscribe_after(140, detail::trampoline<DiplomacyActionOperationStartedData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when a diplomacy event operation has been aborted (param = diplomacy operation)
    struct DiplomacyEventOperationAbortedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t diplomacy_operation;

        static DiplomacyEventOperationAbortedData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18)
            };
        }
    };

    inline int on_diplomacy_event_operation_aborted_before(void(*cb)(const DiplomacyEventOperationAbortedData&)) {
        return detail::subscribe_before(141, detail::trampoline<DiplomacyEventOperationAbortedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_diplomacy_event_operation_aborted_after(void(*cb)(const DiplomacyEventOperationAbortedData&)) {
        return detail::subscribe_after(141, detail::trampoline<DiplomacyEventOperationAbortedData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when a diplomacy event operation has been completed (param = diplomacy operation, param..
    struct DiplomacyEventOperationCompletedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t diplomacy_operation;
        uint64_t outcome_id;

        static DiplomacyEventOperationCompletedData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18),
                *reinterpret_cast<const uint64_t*>(p + 0x20)
            };
        }
    };

    inline int on_diplomacy_event_operation_completed_before(void(*cb)(const DiplomacyEventOperationCompletedData&)) {
        return detail::subscribe_before(142, detail::trampoline<DiplomacyEventOperationCompletedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_diplomacy_event_operation_completed_after(void(*cb)(const DiplomacyEventOperationCompletedData&)) {
        return detail::subscribe_after(142, detail::trampoline<DiplomacyEventOperationCompletedData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when a diplomacy event operation has been created (param = diplomacy operation)
    struct DiplomacyEventOperationCreatedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t diplomacy_operation;

        static DiplomacyEventOperationCreatedData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18)
            };
        }
    };

    inline int on_diplomacy_event_operation_created_before(void(*cb)(const DiplomacyEventOperationCreatedData&)) {
        return detail::subscribe_before(143, detail::trampoline<DiplomacyEventOperationCreatedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_diplomacy_event_operation_created_after(void(*cb)(const DiplomacyEventOperationCreatedData&)) {
        return detail::subscribe_after(143, detail::trampoline<DiplomacyEventOperationCreatedData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when a diplomacy event operation option has been chosen as the preferred outcome (param..
    struct DiplomacyEventOperationOptionChosenData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t diplomacy_operation;
        uint64_t option_id;

        static DiplomacyEventOperationOptionChosenData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18),
                *reinterpret_cast<const uint64_t*>(p + 0x20)
            };
        }
    };

    inline int on_diplomacy_event_operation_option_chosen_before(void(*cb)(const DiplomacyEventOperationOptionChosenData&)) {
        return detail::subscribe_before(144, detail::trampoline<DiplomacyEventOperationOptionChosenData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_diplomacy_event_operation_option_chosen_after(void(*cb)(const DiplomacyEventOperationOptionChosenData&)) {
        return detail::subscribe_after(144, detail::trampoline<DiplomacyEventOperationOptionChosenData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when a diplomacy event operation has been started (param = diplomacy operation)
    struct DiplomacyEventOperationStartedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t diplomacy_operation;

        static DiplomacyEventOperationStartedData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18)
            };
        }
    };

    inline int on_diplomacy_event_operation_started_before(void(*cb)(const DiplomacyEventOperationStartedData&)) {
        return detail::subscribe_before(145, detail::trampoline<DiplomacyEventOperationStartedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_diplomacy_event_operation_started_after(void(*cb)(const DiplomacyEventOperationStartedData&)) {
        return detail::subscribe_after(145, detail::trampoline<DiplomacyEventOperationStartedData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when an object has been dismantled by a dismantling processor. (object = dismantled obj..
    struct DismantledData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t dismantling_object;

        static DismantledData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18)
            };
        }
    };

    inline int on_dismantled_before(void(*cb)(const DismantledData&)) {
        return detail::subscribe_before(146, detail::trampoline<DismantledData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_dismantled_after(void(*cb)(const DismantledData&)) {
        return detail::subscribe_after(146, detail::trampoline<DismantledData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when an object has dismantled a component. (object = object performing the dismantling,..
    struct DismantledComponentData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t dismantled_component;

        static DismantledComponentData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18)
            };
        }
    };

    inline int on_dismantled_component_before(void(*cb)(const DismantledComponentData&)) {
        return detail::subscribe_before(147, detail::trampoline<DismantledComponentData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_dismantled_component_after(void(*cb)(const DismantledComponentData&)) {
        return detail::subscribe_after(147, detail::trampoline<DismantledComponentData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when a dismantle process failed. (object = object performing the dismantling, param = d..
    struct DismantlingFailedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t dismantling_processor;

        static DismantlingFailedData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18)
            };
        }
    };

    inline int on_dismantling_failed_before(void(*cb)(const DismantlingFailedData&)) {
        return detail::subscribe_before(148, detail::trampoline<DismantlingFailedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_dismantling_failed_after(void(*cb)(const DismantlingFailedData&)) {
        return detail::subscribe_after(148, detail::trampoline<DismantlingFailedData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for a distress drone being launched (object = the space which the distress drone was launch..
    struct DistressDroneLaunchedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t the_launching_distress_drone;
        uint64_t launched_distress_drone;

        static DistressDroneLaunchedData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18),
                *reinterpret_cast<const uint64_t*>(p + 0x48)
            };
        }
    };

    inline int on_distress_drone_launched_before(void(*cb)(const DistressDroneLaunchedData&)) {
        return detail::subscribe_before(150, detail::trampoline<DistressDroneLaunchedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_distress_drone_launched_after(void(*cb)(const DistressDroneLaunchedData&)) {
        return detail::subscribe_after(150, detail::trampoline<DistressDroneLaunchedData>,
            reinterpret_cast<void*>(cb));
    }

    /// If object is a ship: Event for the specified object getting a free docking bay assigned for docki..
    struct DockAssignedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t docking_bay;
        uint64_t ship;

        static DockAssignedData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18),
                *reinterpret_cast<const uint64_t*>(p + 0x48)
            };
        }
    };

    inline int on_dock_assigned_before(void(*cb)(const DockAssignedData&)) {
        return detail::subscribe_before(151, detail::trampoline<DockAssignedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_dock_assigned_after(void(*cb)(const DockAssignedData&)) {
        return detail::subscribe_after(151, detail::trampoline<DockAssignedData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for the specified object docking (object = docking object, param = dock object, param2 = zo..
    struct DockedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t dock_object;
        uint64_t zone;
        uint32_t docking_bay;

        static DockedData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18),
                *reinterpret_cast<const uint64_t*>(p + 0x20),
                *reinterpret_cast<const uint32_t*>(p + 0x28)
            };
        }
    };

    inline int on_docked_before(void(*cb)(const DockedData&)) {
        return detail::subscribe_before(152, detail::trampoline<DockedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_docked_after(void(*cb)(const DockedData&)) {
        return detail::subscribe_after(152, detail::trampoline<DockedData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for the specified object having aborted a docking process (object = docking object, param =..
    struct DockingAbortedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t previously_assigned_docking_ba;

        static DockingAbortedData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18)
            };
        }
    };

    inline int on_docking_aborted_before(void(*cb)(const DockingAbortedData&)) {
        return detail::subscribe_before(153, detail::trampoline<DockingAbortedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_docking_aborted_after(void(*cb)(const DockingAbortedData&)) {
        return detail::subscribe_after(153, detail::trampoline<DockingAbortedData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for the specified object being denied docking (object = docking object, param = object that..
    struct DockingDeniedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t object_that_docking_was_reques;

        static DockingDeniedData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18)
            };
        }
    };

    inline int on_docking_denied_before(void(*cb)(const DockingDeniedData&)) {
        return detail::subscribe_before(155, detail::trampoline<DockingDeniedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_docking_denied_after(void(*cb)(const DockingDeniedData&)) {
        return detail::subscribe_after(155, detail::trampoline<DockingDeniedData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for the specified object being incapable of docking (object = docking object, param = objec..
    struct DockingImpossibleData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t object_that_docking_was_reques;

        static DockingImpossibleData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18)
            };
        }
    };

    inline int on_docking_impossible_before(void(*cb)(const DockingImpossibleData&)) {
        return detail::subscribe_before(156, detail::trampoline<DockingImpossibleData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_docking_impossible_after(void(*cb)(const DockingImpossibleData&)) {
        return detail::subscribe_after(156, detail::trampoline<DockingImpossibleData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for the specified object being put into the queue for a free docking bay (object = docking ..
    struct DockingQueuedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        static DockingQueuedData from(const X4MdEvent* ev) { return { ev->source_id, ev->timestamp }; }
    };

    inline int on_docking_queued_before(void(*cb)(const DockingQueuedData&)) {
        return detail::subscribe_before(157, detail::trampoline<DockingQueuedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_docking_queued_after(void(*cb)(const DockingQueuedData&)) {
        return detail::subscribe_after(157, detail::trampoline<DockingQueuedData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for the specified object starting to dock (object = docking object, param = dock, param2 = ..
    struct DockingStartedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t dock;
        uint64_t zone;
        uint32_t docking_bay;

        static DockingStartedData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18),
                *reinterpret_cast<const uint64_t*>(p + 0x20),
                *reinterpret_cast<const uint32_t*>(p + 0x28)
            };
        }
    };

    inline int on_docking_started_before(void(*cb)(const DockingStartedData&)) {
        return detail::subscribe_before(158, detail::trampoline<DockingStartedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_docking_started_after(void(*cb)(const DockingStartedData&)) {
        return detail::subscribe_after(158, detail::trampoline<DockingStartedData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for the specified object being too far from the docking target (object = docking object, pa..
    struct DockTooFarData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t object_that_docking_was_reques;

        static DockTooFarData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18)
            };
        }
    };

    inline int on_dock_too_far_before(void(*cb)(const DockTooFarData&)) {
        return detail::subscribe_before(159, detail::trampoline<DockTooFarData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_dock_too_far_after(void(*cb)(const DockTooFarData&)) {
        return detail::subscribe_after(159, detail::trampoline<DockTooFarData>,
            reinterpret_cast<void*>(cb));
    }

    /// If object is a ship: Event for the specified object getting the dock unassigned for a docking bay..
    struct DockUnassignedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t docking_bay;
        uint64_t ship;
        uint64_t docking_bay_2;

        static DockUnassignedData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18),
                *reinterpret_cast<const uint64_t*>(p + 0x20),
                *reinterpret_cast<const uint64_t*>(p + 0x28)
            };
        }
    };

    inline int on_dock_unassigned_before(void(*cb)(const DockUnassignedData&)) {
        return detail::subscribe_before(160, detail::trampoline<DockUnassignedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_dock_unassigned_after(void(*cb)(const DockUnassignedData&)) {
        return detail::subscribe_after(160, detail::trampoline<DockUnassignedData>,
            reinterpret_cast<void*>(cb));
    }

    /// Player was emergency ejected from ship.
    struct EjectedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        static EjectedData from(const X4MdEvent* ev) { return { ev->source_id, ev->timestamp }; }
    };

    inline int on_ejected_before(void(*cb)(const EjectedData&)) {
        return detail::subscribe_before(163, detail::trampoline<EjectedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_ejected_after(void(*cb)(const EjectedData&)) {
        return detail::subscribe_after(163, detail::trampoline<EjectedData>,
            reinterpret_cast<void*>(cb));
    }

    /// Player is emergency ejecting from ship.
    struct EjectingData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        static EjectingData from(const X4MdEvent* ev) { return { ev->source_id, ev->timestamp }; }
    };

    inline int on_ejecting_before(void(*cb)(const EjectingData&)) {
        return detail::subscribe_before(164, detail::trampoline<EjectingData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_ejecting_after(void(*cb)(const EjectingData&)) {
        return detail::subscribe_after(164, detail::trampoline<EjectingData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for the player entering an anomaly (param = entry anomaly, param2 = exit anomaly)
    struct EnteredAnomalyData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t entry_anomaly;
        uint64_t exit_anomaly;

        static EnteredAnomalyData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18),
                *reinterpret_cast<const uint64_t*>(p + 0x20)
            };
        }
    };

    inline int on_entered_anomaly_before(void(*cb)(const EnteredAnomalyData&)) {
        return detail::subscribe_before(169, detail::trampoline<EnteredAnomalyData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_entered_anomaly_after(void(*cb)(const EnteredAnomalyData&)) {
        return detail::subscribe_after(169, detail::trampoline<EnteredAnomalyData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for the player entering a gate (param = entry gate, param2 = exit gate)
    struct EnteredGateData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t entry_gate;
        uint64_t exit_gate;

        static EnteredGateData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18),
                *reinterpret_cast<const uint64_t*>(p + 0x20)
            };
        }
    };

    inline int on_entered_gate_before(void(*cb)(const EnteredGateData&)) {
        return detail::subscribe_before(170, detail::trampoline<EnteredGateData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_entered_gate_after(void(*cb)(const EnteredGateData&)) {
        return detail::subscribe_after(170, detail::trampoline<EnteredGateData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for any entity within the specified space changing owner (object = space or container which..
    struct EntityChangedOwnerData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t entity_changing_ownership;
        uint32_t new_owner_faction;
        uint64_t previous_owner_faction;

        static EntityChangedOwnerData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18),
                *reinterpret_cast<const uint32_t*>(p + 0x20),
                *reinterpret_cast<const uint64_t*>(p + 0x28)
            };
        }
    };

    inline int on_entity_changed_owner_before(void(*cb)(const EntityChangedOwnerData&)) {
        return detail::subscribe_before(171, detail::trampoline<EntityChangedOwnerData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_entity_changed_owner_after(void(*cb)(const EntityChangedOwnerData&)) {
        return detail::subscribe_after(171, detail::trampoline<EntityChangedOwnerData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for any entity within the specified space changing true owner (object = space or container ..
    struct EntityChangedTrueOwnerData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t entity_changing_ownership;
        uint32_t new_true_owner_faction;
        uint64_t previous_true_owner_faction;

        static EntityChangedTrueOwnerData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18),
                *reinterpret_cast<const uint32_t*>(p + 0x20),
                *reinterpret_cast<const uint64_t*>(p + 0x28)
            };
        }
    };

    inline int on_entity_changed_true_owner_before(void(*cb)(const EntityChangedTrueOwnerData&)) {
        return detail::subscribe_before(172, detail::trampoline<EntityChangedTrueOwnerData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_entity_changed_true_owner_after(void(*cb)(const EntityChangedTrueOwnerData&)) {
        return detail::subscribe_after(172, detail::trampoline<EntityChangedTrueOwnerData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event on an object or a space, e.g. zone, for when any entity enters it or is created in it (obje..
    struct EntityEnteredData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t entering_entity;
        uint64_t new_context_of_entering_entity;
        uint64_t old_context_of_entering_entity;

        static EntityEnteredData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18),
                *reinterpret_cast<const uint64_t*>(p + 0x20),
                *reinterpret_cast<const uint64_t*>(p + 0x28)
            };
        }
    };

    inline int on_entity_entered_before(void(*cb)(const EntityEnteredData&)) {
        return detail::subscribe_before(173, detail::trampoline<EntityEnteredData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_entity_entered_after(void(*cb)(const EntityEnteredData&)) {
        return detail::subscribe_after(173, detail::trampoline<EntityEnteredData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event on an object or a space, e.g. zone, for when any entity leaves it or is removed (object = l..
    struct EntityLeftData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t leaving_entity;
        uint64_t new_context_of_leaving_entity;
        uint64_t old_context_of_leaving_entity;

        static EntityLeftData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18),
                *reinterpret_cast<const uint64_t*>(p + 0x20),
                *reinterpret_cast<const uint64_t*>(p + 0x28)
            };
        }
    };

    inline int on_entity_left_before(void(*cb)(const EntityLeftData&)) {
        return detail::subscribe_before(175, detail::trampoline<EntityLeftData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_entity_left_after(void(*cb)(const EntityLeftData&)) {
        return detail::subscribe_after(175, detail::trampoline<EntityLeftData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when a faction is activated (param = faction)
    struct FactionActivatedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t faction;

        static FactionActivatedData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18)
            };
        }
    };

    inline int on_faction_activated_before(void(*cb)(const FactionActivatedData&)) {
        return detail::subscribe_before(180, detail::trampoline<FactionActivatedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_faction_activated_after(void(*cb)(const FactionActivatedData&)) {
        return detail::subscribe_after(180, detail::trampoline<FactionActivatedData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when a faction is deactivated (param = faction)
    struct FactionDeactivatedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t faction;

        static FactionDeactivatedData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18)
            };
        }
    };

    inline int on_faction_deactivated_before(void(*cb)(const FactionDeactivatedData&)) {
        return detail::subscribe_before(181, detail::trampoline<FactionDeactivatedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_faction_deactivated_after(void(*cb)(const FactionDeactivatedData&)) {
        return detail::subscribe_after(181, detail::trampoline<FactionDeactivatedData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when a faction headquarters has been changed (param = faction, param2 = new faction hea..
    struct FactionHeadquartersChangedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t faction;
        uint64_t new_faction_headquarters_stati;
        uint64_t old_faction_headquarters_stati;

        static FactionHeadquartersChangedData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18),
                *reinterpret_cast<const uint64_t*>(p + 0x20),
                *reinterpret_cast<const uint64_t*>(p + 0x28)
            };
        }
    };

    inline int on_faction_headquarters_changed_before(void(*cb)(const FactionHeadquartersChangedData&)) {
        return detail::subscribe_before(182, detail::trampoline<FactionHeadquartersChangedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_faction_headquarters_changed_after(void(*cb)(const FactionHeadquartersChangedData&)) {
        return detail::subscribe_after(182, detail::trampoline<FactionHeadquartersChangedData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when a faction's police faction has been changed (param = faction, param2 = new police ..
    struct FactionPoliceChangedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t faction;
        uint64_t new_police_faction;
        uint64_t old_police_faction;

        static FactionPoliceChangedData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18),
                *reinterpret_cast<const uint64_t*>(p + 0x20),
                *reinterpret_cast<const uint64_t*>(p + 0x28)
            };
        }
    };

    inline int on_faction_police_changed_before(void(*cb)(const FactionPoliceChangedData&)) {
        return detail::subscribe_before(183, detail::trampoline<FactionPoliceChangedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_faction_police_changed_after(void(*cb)(const FactionPoliceChangedData&)) {
        return detail::subscribe_after(183, detail::trampoline<FactionPoliceChangedData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when a faction changes the relation towards another faction (param = faction, param2 = ..
    struct FactionRelationChangedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t faction;
        uint64_t other_faction;
        uint32_t new_relation;
        uint32_t old_relation;
        uint32_t relationchangereason;

        static FactionRelationChangedData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18),
                *reinterpret_cast<const uint64_t*>(p + 0x20),
                *reinterpret_cast<const uint32_t*>(p + 0x28),
                *reinterpret_cast<const uint32_t*>(p + 0x2C),
                *reinterpret_cast<const uint32_t*>(p + 0x30)
            };
        }
    };

    inline int on_faction_relation_changed_before(void(*cb)(const FactionRelationChangedData&)) {
        return detail::subscribe_before(184, detail::trampoline<FactionRelationChangedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_faction_relation_changed_after(void(*cb)(const FactionRelationChangedData&)) {
        return detail::subscribe_after(184, detail::trampoline<FactionRelationChangedData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for faction changing relation range with another faction (param = faction, param2 = other f..
    struct FactionRelationRangeChangedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t faction;
        uint64_t other_faction;

        static FactionRelationRangeChangedData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18),
                *reinterpret_cast<const uint64_t*>(p + 0x20)
            };
        }
    };

    inline int on_faction_relation_range_changed_before(void(*cb)(const FactionRelationRangeChangedData&)) {
        return detail::subscribe_before(185, detail::trampoline<FactionRelationRangeChangedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_faction_relation_range_changed_after(void(*cb)(const FactionRelationRangeChangedData&)) {
        return detail::subscribe_after(185, detail::trampoline<FactionRelationRangeChangedData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when a faction representative has been changed (param = faction, param2 = new faction r..
    struct FactionRepresentativeChangedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t faction;
        uint64_t new_faction_representative;
        uint64_t old_faction_representative;

        static FactionRepresentativeChangedData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18),
                *reinterpret_cast<const uint64_t*>(p + 0x20),
                *reinterpret_cast<const uint64_t*>(p + 0x28)
            };
        }
    };

    inline int on_faction_representative_changed_before(void(*cb)(const FactionRepresentativeChangedData&)) {
        return detail::subscribe_before(186, detail::trampoline<FactionRepresentativeChangedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_faction_representative_changed_after(void(*cb)(const FactionRepresentativeChangedData&)) {
        return detail::subscribe_after(186, detail::trampoline<FactionRepresentativeChangedData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when a formation action for the specified object has failed to perform (object = format..
    struct FormationUpdateFailedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        static FormationUpdateFailedData from(const X4MdEvent* ev) { return { ev->source_id, ev->timestamp }; }
    };

    inline int on_formation_update_failed_before(void(*cb)(const FormationUpdateFailedData&)) {
        return detail::subscribe_before(194, detail::trampoline<FormationUpdateFailedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_formation_update_failed_after(void(*cb)(const FormationUpdateFailedData&)) {
        return detail::subscribe_after(194, detail::trampoline<FormationUpdateFailedData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when a formation action for the specified object has been successfully performed (objec..
    struct FormationUpdateSucceededData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        static FormationUpdateSucceededData from(const X4MdEvent* ev) { return { ev->source_id, ev->timestamp }; }
    };

    inline int on_formation_update_succeeded_before(void(*cb)(const FormationUpdateSucceededData&)) {
        return detail::subscribe_before(195, detail::trampoline<FormationUpdateSucceededData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_formation_update_succeeded_after(void(*cb)(const FormationUpdateSucceededData&)) {
        return detail::subscribe_after(195, detail::trampoline<FormationUpdateSucceededData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when the game has completed loading a savegame (param = gameversion, param2 = build num..
    struct GameLoadedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint32_t gameversion;
        uint32_t build_number;
        uint32_t original_gameversion_of_savega;

        static GameLoadedData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint32_t*>(p + 0x18),
                *reinterpret_cast<const uint32_t*>(p + 0x1C),
                *reinterpret_cast<const uint32_t*>(p + 0x20)
            };
        }
    };

    inline int on_game_loaded_before(void(*cb)(const GameLoadedData&)) {
        return detail::subscribe_before(196, detail::trampoline<GameLoadedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_game_loaded_after(void(*cb)(const GameLoadedData&)) {
        return detail::subscribe_after(196, detail::trampoline<GameLoadedData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when the game has completed saving a savegame (param = success)
    struct GameSavedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t success;

        static GameSavedData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x20)
            };
        }
    };

    inline int on_game_saved_before(void(*cb)(const GameSavedData&)) {
        return detail::subscribe_before(198, detail::trampoline<GameSavedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_game_saved_after(void(*cb)(const GameSavedData&)) {
        return detail::subscribe_after(198, detail::trampoline<GameSavedData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when a new game has been started and the universe populated (param = list of selected g..
    struct GameStartedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t list_of_selected_gamestart_opt;

        static GameStartedData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18)
            };
        }
    };

    inline int on_game_started_before(void(*cb)(const GameStartedData&)) {
        return detail::subscribe_before(199, detail::trampoline<GameStartedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_game_started_after(void(*cb)(const GameStartedData&)) {
        return detail::subscribe_after(199, detail::trampoline<GameStartedData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for god creating a base factory with the modules to be constructed (object = containing spa..
    struct GodCreatedFactoryData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t station;
        uint64_t list_of_module_macros;
        uint64_t base_construction_sequence;

        static GodCreatedFactoryData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18),
                *reinterpret_cast<const uint64_t*>(p + 0x20),
                *reinterpret_cast<const uint64_t*>(p + 0x28)
            };
        }
    };

    inline int on_god_created_factory_before(void(*cb)(const GodCreatedFactoryData&)) {
        return detail::subscribe_before(204, detail::trampoline<GodCreatedFactoryData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_god_created_factory_after(void(*cb)(const GodCreatedFactoryData&)) {
        return detail::subscribe_after(204, detail::trampoline<GodCreatedFactoryData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for god creating an object (object = containing space, param = object)
    struct GodCreatedObjectData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t object;

        static GodCreatedObjectData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18)
            };
        }
    };

    inline int on_god_created_object_before(void(*cb)(const GodCreatedObjectData&)) {
        return detail::subscribe_before(205, detail::trampoline<GodCreatedObjectData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_god_created_object_after(void(*cb)(const GodCreatedObjectData&)) {
        return detail::subscribe_after(205, detail::trampoline<GodCreatedObjectData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for god creating a ship (object = containing space, param = ship)
    struct GodCreatedShipData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t ship;

        static GodCreatedShipData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18)
            };
        }
    };

    inline int on_god_created_ship_before(void(*cb)(const GodCreatedShipData&)) {
        return detail::subscribe_before(206, detail::trampoline<GodCreatedShipData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_god_created_ship_after(void(*cb)(const GodCreatedShipData&)) {
        return detail::subscribe_after(206, detail::trampoline<GodCreatedShipData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for god creating a station (object = containing space, param = station)
    struct GodCreatedStationData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t station;

        static GodCreatedStationData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18)
            };
        }
    };

    inline int on_god_created_station_before(void(*cb)(const GodCreatedStationData&)) {
        return detail::subscribe_before(207, detail::trampoline<GodCreatedStationData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_god_created_station_after(void(*cb)(const GodCreatedStationData&)) {
        return detail::subscribe_after(207, detail::trampoline<GodCreatedStationData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when an object's gravidar factor has changed (object = gravidar object, param = new fac..
    struct GravidarFactorChangedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t new_factor;
        uint32_t old_factor;

        static GravidarFactorChangedData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18),
                *reinterpret_cast<const uint32_t*>(p + 0x40)
            };
        }
    };

    inline int on_gravidar_factor_changed_before(void(*cb)(const GravidarFactorChangedData&)) {
        return detail::subscribe_before(209, detail::trampoline<GravidarFactorChangedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_gravidar_factor_changed_after(void(*cb)(const GravidarFactorChangedData&)) {
        return detail::subscribe_after(209, detail::trampoline<GravidarFactorChangedData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event fired when the gravidar of an object has performed a scan (object = object of gravidar)
    struct GravidarHasScannedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        static GravidarHasScannedData from(const X4MdEvent* ev) { return { ev->source_id, ev->timestamp }; }
    };

    inline int on_gravidar_has_scanned_before(void(*cb)(const GravidarHasScannedData&)) {
        return detail::subscribe_before(212, detail::trampoline<GravidarHasScannedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_gravidar_has_scanned_after(void(*cb)(const GravidarHasScannedData&)) {
        return detail::subscribe_after(212, detail::trampoline<GravidarHasScannedData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when an object is hacked (requires attribute hacker and/or hacked) (object = hacker if ..
    struct HackedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t the_other_object;
        uint64_t true_iff_object_is_hacker;

        static HackedData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18),
                *reinterpret_cast<const uint64_t*>(p + 0x20)
            };
        }
    };

    inline int on_hacked_before(void(*cb)(const HackedData&)) {
        return detail::subscribe_before(213, detail::trampoline<HackedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_hacked_after(void(*cb)(const HackedData&)) {
        return detail::subscribe_after(213, detail::trampoline<HackedData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for the specified components's hull climbing above the function threshold, rendering the ob..
    struct HullAboveFunctionThresholdData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t affected_component;

        static HullAboveFunctionThresholdData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18)
            };
        }
    };

    inline int on_hull_above_function_threshold_before(void(*cb)(const HullAboveFunctionThresholdData&)) {
        return detail::subscribe_before(216, detail::trampoline<HullAboveFunctionThresholdData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_hull_above_function_threshold_after(void(*cb)(const HullAboveFunctionThresholdData&)) {
        return detail::subscribe_after(216, detail::trampoline<HullAboveFunctionThresholdData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for the specified components's hull falling below the function threshold, rendering the obj..
    struct HullBelowFunctionThresholdData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t affected_component;

        static HullBelowFunctionThresholdData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18)
            };
        }
    };

    inline int on_hull_below_function_threshold_before(void(*cb)(const HullBelowFunctionThresholdData&)) {
        return detail::subscribe_before(217, detail::trampoline<HullBelowFunctionThresholdData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_hull_below_function_threshold_after(void(*cb)(const HullBelowFunctionThresholdData&)) {
        return detail::subscribe_after(217, detail::trampoline<HullBelowFunctionThresholdData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for the specified object's hull being damaged - includes hulls of children (object = event ..
    struct HullDamagedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t damaged_component;

        static HullDamagedData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18)
            };
        }
    };

    inline int on_hull_damaged_before(void(*cb)(const HullDamagedData&)) {
        return detail::subscribe_before(218, detail::trampoline<HullDamagedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_hull_damaged_after(void(*cb)(const HullDamagedData&)) {
        return detail::subscribe_after(218, detail::trampoline<HullDamagedData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for the specified object's hull being repaired - includes hulls of children (object = event..
    struct HullRepairedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t repaired_component;
        uint8_t new_hull_value;

        static HullRepairedData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18),
                *reinterpret_cast<const uint8_t*>(p + 0x40)
            };
        }
    };

    inline int on_hull_repaired_before(void(*cb)(const HullRepairedData&)) {
        return detail::subscribe_before(219, detail::trampoline<HullRepairedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_hull_repaired_after(void(*cb)(const HullRepairedData&)) {
        return detail::subscribe_after(219, detail::trampoline<HullRepairedData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when an object has been caught doing an illegal activity (see &lt;report_illegal_activi..
    struct IllegalActivityDetectedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t caught_object;
        uint64_t victim_of_the_activity_optiona;

        static IllegalActivityDetectedData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18),
                *reinterpret_cast<const uint64_t*>(p + 0x20)
            };
        }
    };

    inline int on_illegal_activity_detected_before(void(*cb)(const IllegalActivityDetectedData&)) {
        return detail::subscribe_before(221, detail::trampoline<IllegalActivityDetectedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_illegal_activity_detected_after(void(*cb)(const IllegalActivityDetectedData&)) {
        return detail::subscribe_after(221, detail::trampoline<IllegalActivityDetectedData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when a missile is launched at the player (param = target component, param2 = missile, p..
    struct IncomingMissileData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t target_component;
        uint64_t missile;
        uint64_t missile_source;

        static IncomingMissileData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18),
                *reinterpret_cast<const uint64_t*>(p + 0x20),
                *reinterpret_cast<const uint64_t*>(p + 0x28)
            };
        }
    };

    inline int on_incoming_missile_before(void(*cb)(const IncomingMissileData&)) {
        return detail::subscribe_before(224, detail::trampoline<IncomingMissileData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_incoming_missile_after(void(*cb)(const IncomingMissileData&)) {
        return detail::subscribe_after(224, detail::trampoline<IncomingMissileData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when the player influence gets updated (param = oldamount, param2 = newamount)
    struct InfluenceUpdatedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint32_t oldamount;
        uint32_t newamount;

        static InfluenceUpdatedData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint32_t*>(p + 0x18),
                *reinterpret_cast<const uint32_t*>(p + 0x1C)
            };
        }
    };

    inline int on_influence_updated_before(void(*cb)(const InfluenceUpdatedData&)) {
        return detail::subscribe_before(225, detail::trampoline<InfluenceUpdatedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_influence_updated_after(void(*cb)(const InfluenceUpdatedData&)) {
        return detail::subscribe_after(225, detail::trampoline<InfluenceUpdatedData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when an object has interiors about to be despawned (object = the populated controllable)
    struct InteriorsDespawningData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        static InteriorsDespawningData from(const X4MdEvent* ev) { return { ev->source_id, ev->timestamp }; }
    };

    inline int on_interiors_despawning_before(void(*cb)(const InteriorsDespawningData&)) {
        return detail::subscribe_before(226, detail::trampoline<InteriorsDespawningData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_interiors_despawning_after(void(*cb)(const InteriorsDespawningData&)) {
        return detail::subscribe_after(226, detail::trampoline<InteriorsDespawningData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when a job ship has been activated in the galaxy, either spawned directly in space or w..
    struct JobShipActivatedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t jobship;

        static JobShipActivatedData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18)
            };
        }
    };

    inline int on_job_ship_activated_before(void(*cb)(const JobShipActivatedData&)) {
        return detail::subscribe_before(229, detail::trampoline<JobShipActivatedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_job_ship_activated_after(void(*cb)(const JobShipActivatedData&)) {
        return detail::subscribe_after(229, detail::trampoline<JobShipActivatedData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for the specified object being destroyed (object = destroyed object, param = killer, param2..
    struct KilledData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t killer;
        uint64_t kill_method;
        uint64_t was_parent_killed;

        static KilledData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18),
                *reinterpret_cast<const uint64_t*>(p + 0x20),
                *reinterpret_cast<const uint64_t*>(p + 0x28)
            };
        }
    };

    inline int on_killed_before(void(*cb)(const KilledData&)) {
        return detail::subscribe_before(233, detail::trampoline<KilledData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_killed_after(void(*cb)(const KilledData&)) {
        return detail::subscribe_after(233, detail::trampoline<KilledData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for lasertower being launched (object = the space which the laser tower was launched in, pa..
    struct LaserTowerLaunchedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t the_launching_defensible;
        uint64_t launched_lasertower;

        static LaserTowerLaunchedData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18),
                *reinterpret_cast<const uint64_t*>(p + 0x48)
            };
        }
    };

    inline int on_laser_tower_launched_before(void(*cb)(const LaserTowerLaunchedData&)) {
        return detail::subscribe_before(235, detail::trampoline<LaserTowerLaunchedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_laser_tower_launched_after(void(*cb)(const LaserTowerLaunchedData&)) {
        return detail::subscribe_after(235, detail::trampoline<LaserTowerLaunchedData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when the player is granted a licence for another faction (param = licenceref, use .type..
    struct LicenceAddedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t licenceref;

        static LicenceAddedData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18)
            };
        }
    };

    inline int on_licence_added_before(void(*cb)(const LicenceAddedData&)) {
        return detail::subscribe_before(238, detail::trampoline<LicenceAddedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_licence_added_after(void(*cb)(const LicenceAddedData&)) {
        return detail::subscribe_after(238, detail::trampoline<LicenceAddedData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when the player has lost a licence for another faction (param = licenceref, use .type o..
    struct LicenceLostData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t licenceref;

        static LicenceLostData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18)
            };
        }
    };

    inline int on_licence_lost_before(void(*cb)(const LicenceLostData&)) {
        return detail::subscribe_before(239, detail::trampoline<LicenceLostData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_licence_lost_after(void(*cb)(const LicenceLostData&)) {
        return detail::subscribe_after(239, detail::trampoline<LicenceLostData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when a target lock is acquired (requires attribute attacker and/or target) (object = at..
    struct LockAcquiredData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t the_other_object;
        uint64_t locking_weapon;

        static LockAcquiredData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18),
                *reinterpret_cast<const uint64_t*>(p + 0x20)
            };
        }
    };

    inline int on_lock_acquired_before(void(*cb)(const LockAcquiredData&)) {
        return detail::subscribe_before(240, detail::trampoline<LockAcquiredData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_lock_acquired_after(void(*cb)(const LockAcquiredData&)) {
        return detail::subscribe_after(240, detail::trampoline<LockAcquiredData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when a lockbox has been opened (object = opened lockbox, param = the object opening the..
    struct LockboxOpenedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t the_object_opening_the_lockbox;

        static LockboxOpenedData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18)
            };
        }
    };

    inline int on_lockbox_opened_before(void(*cb)(const LockboxOpenedData&)) {
        return detail::subscribe_before(241, detail::trampoline<LockboxOpenedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_lockbox_opened_after(void(*cb)(const LockboxOpenedData&)) {
        return detail::subscribe_after(241, detail::trampoline<LockboxOpenedData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when a target lock attempt is initiated (requires attribute attacker and/or target) (ob..
    struct LockInitiatedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t the_other_object;
        uint64_t locking_weapon;

        static LockInitiatedData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18),
                *reinterpret_cast<const uint64_t*>(p + 0x20)
            };
        }
    };

    inline int on_lock_initiated_before(void(*cb)(const LockInitiatedData&)) {
        return detail::subscribe_before(242, detail::trampoline<LockInitiatedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_lock_initiated_after(void(*cb)(const LockInitiatedData&)) {
        return detail::subscribe_after(242, detail::trampoline<LockInitiatedData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when a target lock is lost (requires attribute attacker and/or target) (object = attack..
    struct LockLostData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t the_other_object;
        uint64_t locking_weapon;

        static LockLostData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18),
                *reinterpret_cast<const uint64_t*>(p + 0x20)
            };
        }
    };

    inline int on_lock_lost_before(void(*cb)(const LockLostData&)) {
        return detail::subscribe_before(243, detail::trampoline<LockLostData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_lock_lost_after(void(*cb)(const LockLostData&)) {
        return detail::subscribe_after(243, detail::trampoline<LockLostData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when an object is pinged by a long range scan. (object = scanned object, param = true i..
    struct LongRangeScanPingData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t true_iff_the_result_was_identi;

        static LongRangeScanPingData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x20)
            };
        }
    };

    inline int on_long_range_scan_ping_before(void(*cb)(const LongRangeScanPingData&)) {
        return detail::subscribe_before(244, detail::trampoline<LongRangeScanPingData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_long_range_scan_ping_after(void(*cb)(const LongRangeScanPingData&)) {
        return detail::subscribe_after(244, detail::trampoline<LongRangeScanPingData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when an object sends out a long range scan wave. (object = scanning object or player, p..
    struct LongRangeScanSentData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint32_t scan_progress_between_0_and_1;

        static LongRangeScanSentData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint32_t*>(p + 0x18)
            };
        }
    };

    inline int on_long_range_scan_sent_before(void(*cb)(const LongRangeScanSentData&)) {
        return detail::subscribe_before(245, detail::trampoline<LongRangeScanSentData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_long_range_scan_sent_after(void(*cb)(const LongRangeScanSentData&)) {
        return detail::subscribe_after(245, detail::trampoline<LongRangeScanSentData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when a mass traffic network has been removed (object = the zone that owned the network,..
    struct MassTrafficNetworkRemovedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t the_removed_networks_id;

        static MassTrafficNetworkRemovedData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18)
            };
        }
    };

    inline int on_mass_traffic_network_removed_before(void(*cb)(const MassTrafficNetworkRemovedData&)) {
        return detail::subscribe_before(247, detail::trampoline<MassTrafficNetworkRemovedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_mass_traffic_network_removed_after(void(*cb)(const MassTrafficNetworkRemovedData&)) {
        return detail::subscribe_after(247, detail::trampoline<MassTrafficNetworkRemovedData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for mine being launched (object = the space which the mine was launched in, param = the lau..
    struct MineLaunchedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t the_launching_defensible;
        uint64_t launched_mine;

        static MineLaunchedData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18),
                *reinterpret_cast<const uint64_t*>(p + 0x48)
            };
        }
    };

    inline int on_mine_launched_before(void(*cb)(const MineLaunchedData&)) {
        return detail::subscribe_before(248, detail::trampoline<MineLaunchedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_mine_launched_after(void(*cb)(const MineLaunchedData&)) {
        return detail::subscribe_after(248, detail::trampoline<MineLaunchedData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when the objects' money gets updated (object = container of the account, param = oldamo..
    struct MoneyUpdatedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t oldamount;
        uint64_t newamount;

        static MoneyUpdatedData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18),
                *reinterpret_cast<const uint64_t*>(p + 0x20)
            };
        }
    };

    inline int on_money_updated_before(void(*cb)(const MoneyUpdatedData&)) {
        return detail::subscribe_before(257, detail::trampoline<MoneyUpdatedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_money_updated_after(void(*cb)(const MoneyUpdatedData&)) {
        return detail::subscribe_after(257, detail::trampoline<MoneyUpdatedData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for the specified object having been moved into internal storage (object = stored object, p..
    struct MovedIntoInternalStorageData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t old_docking_bay;
        uint64_t internal_docking_bay;

        static MovedIntoInternalStorageData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18),
                *reinterpret_cast<const uint64_t*>(p + 0x20)
            };
        }
    };

    inline int on_moved_into_internal_storage_before(void(*cb)(const MovedIntoInternalStorageData&)) {
        return detail::subscribe_before(258, detail::trampoline<MovedIntoInternalStorageData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_moved_into_internal_storage_after(void(*cb)(const MovedIntoInternalStorageData&)) {
        return detail::subscribe_after(258, detail::trampoline<MovedIntoInternalStorageData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for navbeacon being launched (object = the space which the navbeacon was launched in, param..
    struct NavBeaconLaunchedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t the_launching_defensible;
        uint64_t launched_navbeacon;

        static NavBeaconLaunchedData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18),
                *reinterpret_cast<const uint64_t*>(p + 0x48)
            };
        }
    };

    inline int on_nav_beacon_launched_before(void(*cb)(const NavBeaconLaunchedData&)) {
        return detail::subscribe_before(259, detail::trampoline<NavBeaconLaunchedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_nav_beacon_launched_after(void(*cb)(const NavBeaconLaunchedData&)) {
        return detail::subscribe_after(259, detail::trampoline<NavBeaconLaunchedData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when npc slots are validated in an interior which could have blocked slots (object = in..
    struct NPCSlotsValidatedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t interior;
        uint8_t was_this_a_recalculation_of_an;

        static NPCSlotsValidatedData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18),
                *reinterpret_cast<const uint8_t*>(p + 0x20)
            };
        }
    };

    inline int on_n_p_c_slots_validated_before(void(*cb)(const NPCSlotsValidatedData&)) {
        return detail::subscribe_before(265, detail::trampoline<NPCSlotsValidatedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_n_p_c_slots_validated_after(void(*cb)(const NPCSlotsValidatedData&)) {
        return detail::subscribe_after(265, detail::trampoline<NPCSlotsValidatedData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when a npc finishes their walk. (object = npc)
    struct NPCWalkFinishedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        static NPCWalkFinishedData from(const X4MdEvent* ev) { return { ev->source_id, ev->timestamp }; }
    };

    inline int on_n_p_c_walk_finished_before(void(*cb)(const NPCWalkFinishedData&)) {
        return detail::subscribe_before(266, detail::trampoline<NPCWalkFinishedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_n_p_c_walk_finished_after(void(*cb)(const NPCWalkFinishedData&)) {
        return detail::subscribe_after(266, detail::trampoline<NPCWalkFinishedData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for the specified object being abandoned by its NPCs (object = abandoned object)
    struct ObjectAbandonedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        static ObjectAbandonedData from(const X4MdEvent* ev) { return { ev->source_id, ev->timestamp }; }
    };

    inline int on_object_abandoned_before(void(*cb)(const ObjectAbandonedData&)) {
        return detail::subscribe_before(267, detail::trampoline<ObjectAbandonedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_object_abandoned_after(void(*cb)(const ObjectAbandonedData&)) {
        return detail::subscribe_after(267, detail::trampoline<ObjectAbandonedData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for the specified object changing owner (object = owner changing object, param = new owner ..
    struct ObjectChangedOwnerData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t new_owner_faction;
        uint32_t previous_owner_faction;

        static ObjectChangedOwnerData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18),
                *reinterpret_cast<const uint32_t*>(p + 0x20)
            };
        }
    };

    inline int on_object_changed_owner_before(void(*cb)(const ObjectChangedOwnerData&)) {
        return detail::subscribe_before(268, detail::trampoline<ObjectChangedOwnerData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_object_changed_owner_after(void(*cb)(const ObjectChangedOwnerData&)) {
        return detail::subscribe_after(268, detail::trampoline<ObjectChangedOwnerData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for the specified object changing state (object = state changing object, param = new state,..
    struct ObjectChangedStateData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t new_state;
        uint64_t previous_state;
        uint32_t perform_transition;

        static ObjectChangedStateData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18),
                *reinterpret_cast<const uint64_t*>(p + 0x20),
                *reinterpret_cast<const uint32_t*>(p + 0x24)
            };
        }
    };

    inline int on_object_changed_state_before(void(*cb)(const ObjectChangedStateData&)) {
        return detail::subscribe_before(269, detail::trampoline<ObjectChangedStateData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_object_changed_state_after(void(*cb)(const ObjectChangedStateData&)) {
        return detail::subscribe_after(269, detail::trampoline<ObjectChangedStateData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for the specified object changing true owner (object = owner changing object, param = new t..
    struct ObjectChangedTrueOwnerData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t new_true_owner_faction;
        uint32_t previous_true_owner_faction;

        static ObjectChangedTrueOwnerData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18),
                *reinterpret_cast<const uint32_t*>(p + 0x20)
            };
        }
    };

    inline int on_object_changed_true_owner_before(void(*cb)(const ObjectChangedTrueOwnerData&)) {
        return detail::subscribe_before(271, detail::trampoline<ObjectChangedTrueOwnerData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_object_changed_true_owner_after(void(*cb)(const ObjectChangedTrueOwnerData&)) {
        return detail::subscribe_after(271, detail::trampoline<ObjectChangedTrueOwnerData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when the specified object collides (object = colliding object, param = other colliding ..
    struct ObjectCollidedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t other_colliding_object;
        uint32_t impulse_of_the_colliding_objec;
        uint32_t speed_of_the_colliding_object;

        static ObjectCollidedData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18),
                *reinterpret_cast<const uint32_t*>(p + 0x20),
                *reinterpret_cast<const uint32_t*>(p + 0x24)
            };
        }
    };

    inline int on_object_collided_before(void(*cb)(const ObjectCollidedData&)) {
        return detail::subscribe_before(272, detail::trampoline<ObjectCollidedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_object_collided_after(void(*cb)(const ObjectCollidedData&)) {
        return detail::subscribe_after(272, detail::trampoline<ObjectCollidedData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when a subordinate receives a new commander (object = subordinate, param = new commande..
    struct ObjectCommanderSetData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t new_commander;
        uint64_t previous_commander;
        uint64_t new_assignment;

        static ObjectCommanderSetData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18),
                *reinterpret_cast<const uint64_t*>(p + 0x20),
                *reinterpret_cast<const uint64_t*>(p + 0x28)
            };
        }
    };

    inline int on_object_commander_set_before(void(*cb)(const ObjectCommanderSetData&)) {
        return detail::subscribe_before(273, detail::trampoline<ObjectCommanderSetData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_object_commander_set_after(void(*cb)(const ObjectCommanderSetData&)) {
        return detail::subscribe_after(273, detail::trampoline<ObjectCommanderSetData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for the specified object changing drone modes for its defence drones (object = object that ..
    struct ObjectDefenceDroneModeChangedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint32_t new_drone_mode;

        static ObjectDefenceDroneModeChangedData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint32_t*>(p + 0x18)
            };
        }
    };

    inline int on_object_defence_drone_mode_changed_before(void(*cb)(const ObjectDefenceDroneModeChangedData&)) {
        return detail::subscribe_before(274, detail::trampoline<ObjectDefenceDroneModeChangedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_object_defence_drone_mode_changed_after(void(*cb)(const ObjectDefenceDroneModeChangedData&)) {
        return detail::subscribe_after(274, detail::trampoline<ObjectDefenceDroneModeChangedData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for the specified object authorizing defence drone use (object = object that launches drones)
    struct ObjectDefenceDronesArmedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        static ObjectDefenceDronesArmedData from(const X4MdEvent* ev) { return { ev->source_id, ev->timestamp }; }
    };

    inline int on_object_defence_drones_armed_before(void(*cb)(const ObjectDefenceDronesArmedData&)) {
        return detail::subscribe_before(275, detail::trampoline<ObjectDefenceDronesArmedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_object_defence_drones_armed_after(void(*cb)(const ObjectDefenceDronesArmedData&)) {
        return detail::subscribe_after(275, detail::trampoline<ObjectDefenceDronesArmedData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for the specified object unauthorizing defence drone use (object = object that launches dro..
    struct ObjectDefenceDronesDisarmedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        static ObjectDefenceDronesDisarmedData from(const X4MdEvent* ev) { return { ev->source_id, ev->timestamp }; }
    };

    inline int on_object_defence_drones_disarmed_before(void(*cb)(const ObjectDefenceDronesDisarmedData&)) {
        return detail::subscribe_before(276, detail::trampoline<ObjectDefenceDronesDisarmedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_object_defence_drones_disarmed_after(void(*cb)(const ObjectDefenceDronesDisarmedData&)) {
        return detail::subscribe_after(276, detail::trampoline<ObjectDefenceDronesDisarmedData>,
            reinterpret_cast<void*>(cb));
    }

    /// If object is a ship: Event for the specified object getting a free docking bay assigned for docki..
    struct ObjectDockAssignedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t docking_bay;
        uint64_t ship;
        uint64_t docking_bay_2;

        static ObjectDockAssignedData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18),
                *reinterpret_cast<const uint64_t*>(p + 0x20),
                *reinterpret_cast<const uint64_t*>(p + 0x28)
            };
        }
    };

    inline int on_object_dock_assigned_before(void(*cb)(const ObjectDockAssignedData&)) {
        return detail::subscribe_before(277, detail::trampoline<ObjectDockAssignedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_object_dock_assigned_after(void(*cb)(const ObjectDockAssignedData&)) {
        return detail::subscribe_after(277, detail::trampoline<ObjectDockAssignedData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for the specified object docking (object = docking object, param = dock object, param2 = zo..
    struct ObjectDockedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t dock_object;
        uint64_t zone;
        uint32_t docking_bay;

        static ObjectDockedData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18),
                *reinterpret_cast<const uint64_t*>(p + 0x20),
                *reinterpret_cast<const uint32_t*>(p + 0x28)
            };
        }
    };

    inline int on_object_docked_before(void(*cb)(const ObjectDockedData&)) {
        return detail::subscribe_before(278, detail::trampoline<ObjectDockedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_object_docked_after(void(*cb)(const ObjectDockedData&)) {
        return detail::subscribe_after(278, detail::trampoline<ObjectDockedData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for the specified object starting to dock (object = docking object, param = dock, param2 = ..
    struct ObjectDockingStartedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t dock;
        uint64_t zone;
        uint32_t docking_bay;

        static ObjectDockingStartedData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18),
                *reinterpret_cast<const uint64_t*>(p + 0x20),
                *reinterpret_cast<const uint32_t*>(p + 0x28)
            };
        }
    };

    inline int on_object_docking_started_before(void(*cb)(const ObjectDockingStartedData&)) {
        return detail::subscribe_before(279, detail::trampoline<ObjectDockingStartedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_object_docking_started_after(void(*cb)(const ObjectDockingStartedData&)) {
        return detail::subscribe_after(279, detail::trampoline<ObjectDockingStartedData>,
            reinterpret_cast<void*>(cb));
    }

    /// If object is a ship: Event for the specified object getting the dock unassigned for a docking bay..
    struct ObjectDockUnassignedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t docking_bay;
        uint64_t ship;
        uint64_t docking_bay_2;

        static ObjectDockUnassignedData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18),
                *reinterpret_cast<const uint64_t*>(p + 0x20),
                *reinterpret_cast<const uint64_t*>(p + 0x28)
            };
        }
    };

    inline int on_object_dock_unassigned_before(void(*cb)(const ObjectDockUnassignedData&)) {
        return detail::subscribe_before(280, detail::trampoline<ObjectDockUnassignedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_object_dock_unassigned_after(void(*cb)(const ObjectDockUnassignedData&)) {
        return detail::subscribe_after(280, detail::trampoline<ObjectDockUnassignedData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event on an object or a space, e.g. zone, for when any object enters it or is created in it (obje..
    struct ObjectEnteredData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t entering_object;
        uint64_t new_context_of_entering_object;
        uint32_t old_context_of_entering_object;

        static ObjectEnteredData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18),
                *reinterpret_cast<const uint64_t*>(p + 0x20),
                *reinterpret_cast<const uint32_t*>(p + 0x24)
            };
        }
    };

    inline int on_object_entered_before(void(*cb)(const ObjectEnteredData&)) {
        return detail::subscribe_before(281, detail::trampoline<ObjectEnteredData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_object_entered_after(void(*cb)(const ObjectEnteredData&)) {
        return detail::subscribe_after(281, detail::trampoline<ObjectEnteredData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when the object becomes visible on the gravidar of any player-owned object (object = en..
    struct ObjectEnteredLiveViewData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        static ObjectEnteredLiveViewData from(const X4MdEvent* ev) { return { ev->source_id, ev->timestamp }; }
    };

    inline int on_object_entered_live_view_before(void(*cb)(const ObjectEnteredLiveViewData&)) {
        return detail::subscribe_before(282, detail::trampoline<ObjectEnteredLiveViewData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_object_entered_live_view_after(void(*cb)(const ObjectEnteredLiveViewData&)) {
        return detail::subscribe_after(282, detail::trampoline<ObjectEnteredLiveViewData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for an object launching a countermeasure (object = the launching defensible, param = launch..
    struct ObjectLaunchedCountermeasureData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t launched_countermeasure;

        static ObjectLaunchedCountermeasureData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18)
            };
        }
    };

    inline int on_object_launched_countermeasure_before(void(*cb)(const ObjectLaunchedCountermeasureData&)) {
        return detail::subscribe_before(284, detail::trampoline<ObjectLaunchedCountermeasureData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_object_launched_countermeasure_after(void(*cb)(const ObjectLaunchedCountermeasureData&)) {
        return detail::subscribe_after(284, detail::trampoline<ObjectLaunchedCountermeasureData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for an object launching a distress drone (object = the launching defensible, param = launch..
    struct ObjectLaunchedDistressDroneData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t launched_distress_drone;

        static ObjectLaunchedDistressDroneData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18)
            };
        }
    };

    inline int on_object_launched_distress_drone_before(void(*cb)(const ObjectLaunchedDistressDroneData&)) {
        return detail::subscribe_before(285, detail::trampoline<ObjectLaunchedDistressDroneData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_object_launched_distress_drone_after(void(*cb)(const ObjectLaunchedDistressDroneData&)) {
        return detail::subscribe_after(285, detail::trampoline<ObjectLaunchedDistressDroneData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for an object launching a lasertower (object = the launching defensible, param = launched l..
    struct ObjectLaunchedLaserTowerData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t launched_lasertower;

        static ObjectLaunchedLaserTowerData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18)
            };
        }
    };

    inline int on_object_launched_laser_tower_before(void(*cb)(const ObjectLaunchedLaserTowerData&)) {
        return detail::subscribe_before(286, detail::trampoline<ObjectLaunchedLaserTowerData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_object_launched_laser_tower_after(void(*cb)(const ObjectLaunchedLaserTowerData&)) {
        return detail::subscribe_after(286, detail::trampoline<ObjectLaunchedLaserTowerData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for an object launching a mine (object = the launching defensible, param = launched mine)
    struct ObjectLaunchedMineData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t launched_mine;

        static ObjectLaunchedMineData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18)
            };
        }
    };

    inline int on_object_launched_mine_before(void(*cb)(const ObjectLaunchedMineData&)) {
        return detail::subscribe_before(287, detail::trampoline<ObjectLaunchedMineData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_object_launched_mine_after(void(*cb)(const ObjectLaunchedMineData&)) {
        return detail::subscribe_after(287, detail::trampoline<ObjectLaunchedMineData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for an object launching a navigation beacon (object = the launching defensible, param = lau..
    struct ObjectLaunchedNavBeaconData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t launched_navigation_beacon;

        static ObjectLaunchedNavBeaconData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18)
            };
        }
    };

    inline int on_object_launched_nav_beacon_before(void(*cb)(const ObjectLaunchedNavBeaconData&)) {
        return detail::subscribe_before(288, detail::trampoline<ObjectLaunchedNavBeaconData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_object_launched_nav_beacon_after(void(*cb)(const ObjectLaunchedNavBeaconData&)) {
        return detail::subscribe_after(288, detail::trampoline<ObjectLaunchedNavBeaconData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for an object launching a resourceprobe (object = the launching defensible, param = launche..
    struct ObjectLaunchedResourceProbeData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t launched_resourceprobe;

        static ObjectLaunchedResourceProbeData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18)
            };
        }
    };

    inline int on_object_launched_resource_probe_before(void(*cb)(const ObjectLaunchedResourceProbeData&)) {
        return detail::subscribe_before(289, detail::trampoline<ObjectLaunchedResourceProbeData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_object_launched_resource_probe_after(void(*cb)(const ObjectLaunchedResourceProbeData&)) {
        return detail::subscribe_after(289, detail::trampoline<ObjectLaunchedResourceProbeData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for an object launching a satellite (object = the launching defensible, param = launched sa..
    struct ObjectLaunchedSatelliteData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t launched_satellite;

        static ObjectLaunchedSatelliteData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18)
            };
        }
    };

    inline int on_object_launched_satellite_before(void(*cb)(const ObjectLaunchedSatelliteData&)) {
        return detail::subscribe_before(290, detail::trampoline<ObjectLaunchedSatelliteData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_object_launched_satellite_after(void(*cb)(const ObjectLaunchedSatelliteData&)) {
        return detail::subscribe_after(290, detail::trampoline<ObjectLaunchedSatelliteData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event on an object or a space, e.g. zone, for when any object leaves it or is removed (object = l..
    struct ObjectLeftData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t leaving_object;
        uint64_t new_context_of_leaving_object;
        uint32_t old_context_of_leaving_object;

        static ObjectLeftData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18),
                *reinterpret_cast<const uint64_t*>(p + 0x20),
                *reinterpret_cast<const uint32_t*>(p + 0x24)
            };
        }
    };

    inline int on_object_left_before(void(*cb)(const ObjectLeftData&)) {
        return detail::subscribe_before(291, detail::trampoline<ObjectLeftData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_object_left_after(void(*cb)(const ObjectLeftData&)) {
        return detail::subscribe_after(291, detail::trampoline<ObjectLeftData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when the object is no longer visible on the gravidar of any player-owned object (object..
    struct ObjectLeftLiveViewData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        static ObjectLeftLiveViewData from(const X4MdEvent* ev) { return { ev->source_id, ev->timestamp }; }
    };

    inline int on_object_left_live_view_before(void(*cb)(const ObjectLeftLiveViewData&)) {
        return detail::subscribe_before(292, detail::trampoline<ObjectLeftLiveViewData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_object_left_live_view_after(void(*cb)(const ObjectLeftLiveViewData&)) {
        return detail::subscribe_after(292, detail::trampoline<ObjectLeftLiveViewData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for the specified object changing drone modes for its mining drones (object = object that l..
    struct ObjectMiningDroneModeChangedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint32_t new_drone_mode;

        static ObjectMiningDroneModeChangedData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint32_t*>(p + 0x18)
            };
        }
    };

    inline int on_object_mining_drone_mode_changed_before(void(*cb)(const ObjectMiningDroneModeChangedData&)) {
        return detail::subscribe_before(294, detail::trampoline<ObjectMiningDroneModeChangedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_object_mining_drone_mode_changed_after(void(*cb)(const ObjectMiningDroneModeChangedData&)) {
        return detail::subscribe_after(294, detail::trampoline<ObjectMiningDroneModeChangedData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for the specified object authorizing mining drone use (object = object that launches drones)
    struct ObjectMiningDronesArmedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        static ObjectMiningDronesArmedData from(const X4MdEvent* ev) { return { ev->source_id, ev->timestamp }; }
    };

    inline int on_object_mining_drones_armed_before(void(*cb)(const ObjectMiningDronesArmedData&)) {
        return detail::subscribe_before(295, detail::trampoline<ObjectMiningDronesArmedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_object_mining_drones_armed_after(void(*cb)(const ObjectMiningDronesArmedData&)) {
        return detail::subscribe_after(295, detail::trampoline<ObjectMiningDronesArmedData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for the specified object unauthorizing mining drone use (object = object that launches drones)
    struct ObjectMiningDronesDisarmedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        static ObjectMiningDronesDisarmedData from(const X4MdEvent* ev) { return { ev->source_id, ev->timestamp }; }
    };

    inline int on_object_mining_drones_disarmed_before(void(*cb)(const ObjectMiningDronesDisarmedData&)) {
        return detail::subscribe_before(296, detail::trampoline<ObjectMiningDronesDisarmedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_object_mining_drones_disarmed_after(void(*cb)(const ObjectMiningDronesDisarmedData&)) {
        return detail::subscribe_after(296, detail::trampoline<ObjectMiningDronesDisarmedData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for the specified object having been moved into internal storage (object = stored object, p..
    struct ObjectMovedIntoInternalStorageData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t old_docking_bay;
        uint64_t internal_docking_bay;

        static ObjectMovedIntoInternalStorageData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18),
                *reinterpret_cast<const uint64_t*>(p + 0x20)
            };
        }
    };

    inline int on_object_moved_into_internal_storage_before(void(*cb)(const ObjectMovedIntoInternalStorageData&)) {
        return detail::subscribe_before(297, detail::trampoline<ObjectMovedIntoInternalStorageData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_object_moved_into_internal_storage_after(void(*cb)(const ObjectMovedIntoInternalStorageData&)) {
        return detail::subscribe_after(297, detail::trampoline<ObjectMovedIntoInternalStorageData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when an object has opened a lockbox (object = object opening the lockbox, param = lockb..
    struct ObjectOpenedLockboxData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t lockbox_which_was_opened;

        static ObjectOpenedLockboxData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18)
            };
        }
    };

    inline int on_object_opened_lockbox_before(void(*cb)(const ObjectOpenedLockboxData&)) {
        return detail::subscribe_before(298, detail::trampoline<ObjectOpenedLockboxData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_object_opened_lockbox_after(void(*cb)(const ObjectOpenedLockboxData&)) {
        return detail::subscribe_after(298, detail::trampoline<ObjectOpenedLockboxData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when the specified object is picked up by another object (object = pickup, param = coll..
    struct ObjectPickedUpData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t collector;

        static ObjectPickedUpData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18)
            };
        }
    };

    inline int on_object_picked_up_before(void(*cb)(const ObjectPickedUpData&)) {
        return detail::subscribe_before(299, detail::trampoline<ObjectPickedUpData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_object_picked_up_after(void(*cb)(const ObjectPickedUpData&)) {
        return detail::subscribe_after(299, detail::trampoline<ObjectPickedUpData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for the specified object changing relation range (object = the object changing relation, pa..
    struct ObjectRelationRangeChangedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t faction_with_which_relation_ha;

        static ObjectRelationRangeChangedData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18)
            };
        }
    };

    inline int on_object_relation_range_changed_before(void(*cb)(const ObjectRelationRangeChangedData&)) {
        return detail::subscribe_before(300, detail::trampoline<ObjectRelationRangeChangedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_object_relation_range_changed_after(void(*cb)(const ObjectRelationRangeChangedData&)) {
        return detail::subscribe_after(300, detail::trampoline<ObjectRelationRangeChangedData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for the specified object having been retrieved from internal storage (object = retrieved ob..
    struct ObjectRetrievedFromInternalStorageData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t new_docking_bay;
        uint64_t internal_docking_bay;

        static ObjectRetrievedFromInternalStorageData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18),
                *reinterpret_cast<const uint64_t*>(p + 0x20)
            };
        }
    };

    inline int on_object_retrieved_from_internal_storage_before(void(*cb)(const ObjectRetrievedFromInternalStorageData&)) {
        return detail::subscribe_before(301, detail::trampoline<ObjectRetrievedFromInternalStorageData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_object_retrieved_from_internal_storage_after(void(*cb)(const ObjectRetrievedFromInternalStorageData&)) {
        return detail::subscribe_after(301, detail::trampoline<ObjectRetrievedFromInternalStorageData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for the specified object being signalled (object = signalled object, param = custom, usuall..
    struct ObjectSignalledData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t custom;
        uint64_t custom_2;
        uint64_t custom_3;

        static ObjectSignalledData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18),
                *reinterpret_cast<const uint64_t*>(p + 0x20),
                *reinterpret_cast<const uint64_t*>(p + 0x28)
            };
        }
    };

    inline int on_object_signalled_before(void(*cb)(const ObjectSignalledData&)) {
        return detail::subscribe_before(302, detail::trampoline<ObjectSignalledData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_object_signalled_after(void(*cb)(const ObjectSignalledData&)) {
        return detail::subscribe_after(302, detail::trampoline<ObjectSignalledData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for the specified object changing drone modes for its transport drones (object = object whe..
    struct ObjectTransportDroneModeChangedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint32_t new_drone_mode;

        static ObjectTransportDroneModeChangedData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint32_t*>(p + 0x18)
            };
        }
    };

    inline int on_object_transport_drone_mode_changed_before(void(*cb)(const ObjectTransportDroneModeChangedData&)) {
        return detail::subscribe_before(303, detail::trampoline<ObjectTransportDroneModeChangedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_object_transport_drone_mode_changed_after(void(*cb)(const ObjectTransportDroneModeChangedData&)) {
        return detail::subscribe_after(303, detail::trampoline<ObjectTransportDroneModeChangedData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for the specified object authorizing transport drone use (object = object that launches dro..
    struct ObjectTransportDronesArmedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        static ObjectTransportDronesArmedData from(const X4MdEvent* ev) { return { ev->source_id, ev->timestamp }; }
    };

    inline int on_object_transport_drones_armed_before(void(*cb)(const ObjectTransportDronesArmedData&)) {
        return detail::subscribe_before(304, detail::trampoline<ObjectTransportDronesArmedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_object_transport_drones_armed_after(void(*cb)(const ObjectTransportDronesArmedData&)) {
        return detail::subscribe_after(304, detail::trampoline<ObjectTransportDronesArmedData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for the specified object unauthorizing transport drone use (object = object that launches d..
    struct ObjectTransportDronesDisarmedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        static ObjectTransportDronesDisarmedData from(const X4MdEvent* ev) { return { ev->source_id, ev->timestamp }; }
    };

    inline int on_object_transport_drones_disarmed_before(void(*cb)(const ObjectTransportDronesDisarmedData&)) {
        return detail::subscribe_before(305, detail::trampoline<ObjectTransportDronesDisarmedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_object_transport_drones_disarmed_after(void(*cb)(const ObjectTransportDronesDisarmedData&)) {
        return detail::subscribe_after(305, detail::trampoline<ObjectTransportDronesDisarmedData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when an object is triggered (object = trigger object, param = triggerer)
    struct ObjectTriggeredData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t triggerer;

        static ObjectTriggeredData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18)
            };
        }
    };

    inline int on_object_triggered_before(void(*cb)(const ObjectTriggeredData&)) {
        return detail::subscribe_before(306, detail::trampoline<ObjectTriggeredData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_object_triggered_after(void(*cb)(const ObjectTriggeredData&)) {
        return detail::subscribe_after(306, detail::trampoline<ObjectTriggeredData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for the specified object undocking (object = undocking object, param = dock, param2 = zone,..
    struct ObjectUndockedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t dock;
        uint64_t zone;
        uint64_t docking_bay;

        static ObjectUndockedData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18),
                *reinterpret_cast<const uint64_t*>(p + 0x20),
                *reinterpret_cast<const uint64_t*>(p + 0x28)
            };
        }
    };

    inline int on_object_undocked_before(void(*cb)(const ObjectUndockedData&)) {
        return detail::subscribe_before(307, detail::trampoline<ObjectUndockedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_object_undocked_after(void(*cb)(const ObjectUndockedData&)) {
        return detail::subscribe_after(307, detail::trampoline<ObjectUndockedData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for the specified object starting to undock (object = undocking object, param = dock, param..
    struct ObjectUndockingStartedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t dock;
        uint64_t zone;
        uint64_t docking_bay;

        static ObjectUndockingStartedData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18),
                *reinterpret_cast<const uint64_t*>(p + 0x20),
                *reinterpret_cast<const uint64_t*>(p + 0x28)
            };
        }
    };

    inline int on_object_undocking_started_before(void(*cb)(const ObjectUndockingStartedData&)) {
        return detail::subscribe_before(308, detail::trampoline<ObjectUndockingStartedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_object_undocking_started_after(void(*cb)(const ObjectUndockingStartedData&)) {
        return detail::subscribe_after(308, detail::trampoline<ObjectUndockingStartedData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for the specified object changing a weapon mode (object = object changing the weapon mode, ..
    struct ObjectWeaponModeChangedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t weapon;
        uint64_t new_weapon_mode;

        static ObjectWeaponModeChangedData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18),
                *reinterpret_cast<const uint64_t*>(p + 0x20)
            };
        }
    };

    inline int on_object_weapon_mode_changed_before(void(*cb)(const ObjectWeaponModeChangedData&)) {
        return detail::subscribe_before(309, detail::trampoline<ObjectWeaponModeChangedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_object_weapon_mode_changed_after(void(*cb)(const ObjectWeaponModeChangedData&)) {
        return detail::subscribe_after(309, detail::trampoline<ObjectWeaponModeChangedData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when an object starts an observation (requires attribute observer and/or observed) (obj..
    struct ObservationStartedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t observer;
        uint64_t observed;
        uint32_t range_length;

        static ObservationStartedData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18),
                *reinterpret_cast<const uint64_t*>(p + 0x20),
                *reinterpret_cast<const uint32_t*>(p + 0x28)
            };
        }
    };

    inline int on_observation_started_before(void(*cb)(const ObservationStartedData&)) {
        return detail::subscribe_before(310, detail::trampoline<ObservationStartedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_observation_started_after(void(*cb)(const ObservationStartedData&)) {
        return detail::subscribe_after(310, detail::trampoline<ObservationStartedData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when an observation is stopped (requires attribute observer and/or observed) (object = ..
    struct ObservationStoppedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t observer;
        uint64_t observed;

        static ObservationStoppedData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18),
                *reinterpret_cast<const uint64_t*>(p + 0x20)
            };
        }
    };

    inline int on_observation_stopped_before(void(*cb)(const ObservationStoppedData&)) {
        return detail::subscribe_before(311, detail::trampoline<ObservationStoppedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_observation_stopped_after(void(*cb)(const ObservationStoppedData&)) {
        return detail::subscribe_after(311, detail::trampoline<ObservationStoppedData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when the player's current weapon runs out of ammo (param = weapon macro, param2 = ranou..
    struct OutOfAmmoData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t weapon_macro;
        uint64_t ranout_of_ammo_due_to_the_shot;

        static OutOfAmmoData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18),
                *reinterpret_cast<const uint64_t*>(p + 0x20)
            };
        }
    };

    inline int on_out_of_ammo_before(void(*cb)(const OutOfAmmoData&)) {
        return detail::subscribe_before(316, detail::trampoline<OutOfAmmoData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_out_of_ammo_after(void(*cb)(const OutOfAmmoData&)) {
        return detail::subscribe_after(316, detail::trampoline<OutOfAmmoData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when a player-owned station's paid build plot is resized or repositioned (param = stati..
    struct PaidBuildPlotChangedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t station;
        uint64_t old_paid_build_plot_size;

        static PaidBuildPlotChangedData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18),
                *reinterpret_cast<const uint64_t*>(p + 0x20)
            };
        }
    };

    inline int on_paid_build_plot_changed_before(void(*cb)(const PaidBuildPlotChangedData&)) {
        return detail::subscribe_before(319, detail::trampoline<PaidBuildPlotChangedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_paid_build_plot_changed_after(void(*cb)(const PaidBuildPlotChangedData&)) {
        return detail::subscribe_after(319, detail::trampoline<PaidBuildPlotChangedData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when an animation phase has started (param = phase string, param2 = object that the pha..
    struct PhaseStartedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t phase_string;

        static PhaseStartedData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x38)
            };
        }
    };

    inline int on_phase_started_before(void(*cb)(const PhaseStartedData&)) {
        return detail::subscribe_before(322, detail::trampoline<PhaseStartedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_phase_started_after(void(*cb)(const PhaseStartedData&)) {
        return detail::subscribe_after(322, detail::trampoline<PhaseStartedData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when the specified object picks up another object (object = collector, param = pickup)
    struct PickedUpObjectData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t pickup;

        static PickedUpObjectData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18)
            };
        }
    };

    inline int on_picked_up_object_before(void(*cb)(const PickedUpObjectData&)) {
        return detail::subscribe_before(326, detail::trampoline<PickedUpObjectData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_picked_up_object_after(void(*cb)(const PickedUpObjectData&)) {
        return detail::subscribe_after(326, detail::trampoline<PickedUpObjectData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when a player alert is triggered (object = location (sector), param = alert message, pa..
    struct PlayerAlertData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t alert_message;
        uint64_t sound_id;
        uint32_t object_list;

        static PlayerAlertData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x28),
                *reinterpret_cast<const uint64_t*>(p + 0x30),
                *reinterpret_cast<const uint32_t*>(p + 0x58)
            };
        }
    };

    inline int on_player_alert_before(void(*cb)(const PlayerAlertData&)) {
        return detail::subscribe_before(332, detail::trampoline<PlayerAlertData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_player_alert_after(void(*cb)(const PlayerAlertData&)) {
        return detail::subscribe_after(332, detail::trampoline<PlayerAlertData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event is raised when the player has unlocked a blueprint (param = blueprint's module macro, param..
    struct PlayerBlueprintAddedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t blueprints_module_macro;
        uint64_t blueprints_ware;

        static PlayerBlueprintAddedData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18),
                *reinterpret_cast<const uint64_t*>(p + 0x20)
            };
        }
    };

    inline int on_player_blueprint_added_before(void(*cb)(const PlayerBlueprintAddedData&)) {
        return detail::subscribe_before(333, detail::trampoline<PlayerBlueprintAddedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_player_blueprint_added_after(void(*cb)(const PlayerBlueprintAddedData&)) {
        return detail::subscribe_after(333, detail::trampoline<PlayerBlueprintAddedData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when the player changes target (param = new target. null if player deselects target.)
    struct PlayerChangedTargetData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t new_target_null_if_player_dese;

        static PlayerChangedTargetData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18)
            };
        }
    };

    inline int on_player_changed_target_before(void(*cb)(const PlayerChangedTargetData&)) {
        return detail::subscribe_before(336, detail::trampoline<PlayerChangedTargetData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_player_changed_target_after(void(*cb)(const PlayerChangedTargetData&)) {
        return detail::subscribe_after(336, detail::trampoline<PlayerChangedTargetData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when the player crafts ammo from inventory wares (param = weapon macro, param2 = amount)
    struct PlayerCraftedAmmoData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t weapon_macro;
        uint32_t amount;

        static PlayerCraftedAmmoData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18),
                *reinterpret_cast<const uint32_t*>(p + 0x20)
            };
        }
    };

    inline int on_player_crafted_ammo_before(void(*cb)(const PlayerCraftedAmmoData&)) {
        return detail::subscribe_before(337, detail::trampoline<PlayerCraftedAmmoData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_player_crafted_ammo_after(void(*cb)(const PlayerCraftedAmmoData&)) {
        return detail::subscribe_after(337, detail::trampoline<PlayerCraftedAmmoData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when the player is created (no parameters)
    struct PlayerCreatedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        static PlayerCreatedData from(const X4MdEvent* ev) { return { ev->source_id, ev->timestamp }; }
    };

    inline int on_player_created_before(void(*cb)(const PlayerCreatedData&)) {
        return detail::subscribe_before(338, detail::trampoline<PlayerCreatedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_player_created_after(void(*cb)(const PlayerCreatedData&)) {
        return detail::subscribe_after(338, detail::trampoline<PlayerCreatedData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when an object is detected by the player and added to the long range scan memory (param..
    struct PlayerDetectedObjectData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t object;

        static PlayerDetectedObjectData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18)
            };
        }
    };

    inline int on_player_detected_object_before(void(*cb)(const PlayerDetectedObjectData&)) {
        return detail::subscribe_before(339, detail::trampoline<PlayerDetectedObjectData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_player_detected_object_after(void(*cb)(const PlayerDetectedObjectData&)) {
        return detail::subscribe_after(339, detail::trampoline<PlayerDetectedObjectData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when a mission offer is discovered by the player and added to the mission offer memory ..
    struct PlayerDiscoveredMissionOfferData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t mission_offer_cue;

        static PlayerDiscoveredMissionOfferData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18)
            };
        }
    };

    inline int on_player_discovered_mission_offer_before(void(*cb)(const PlayerDiscoveredMissionOfferData&)) {
        return detail::subscribe_before(341, detail::trampoline<PlayerDiscoveredMissionOfferData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_player_discovered_mission_offer_after(void(*cb)(const PlayerDiscoveredMissionOfferData&)) {
        return detail::subscribe_after(341, detail::trampoline<PlayerDiscoveredMissionOfferData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when an object is hacked by the player (param = hacked, param2 = hacker, param3 = contr..
    struct PlayerHackedObjectData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t hacked;
        uint64_t hacker;
        uint64_t control_panel_type;

        static PlayerHackedObjectData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18),
                *reinterpret_cast<const uint64_t*>(p + 0x20),
                *reinterpret_cast<const uint64_t*>(p + 0x28)
            };
        }
    };

    inline int on_player_hacked_object_before(void(*cb)(const PlayerHackedObjectData&)) {
        return detail::subscribe_before(342, detail::trampoline<PlayerHackedObjectData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_player_hacked_object_after(void(*cb)(const PlayerHackedObjectData&)) {
        return detail::subscribe_after(342, detail::trampoline<PlayerHackedObjectData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when a player responds to an interactive notification (param = interaction param, param..
    struct PlayerInteractionData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint32_t interaction_param;
        uint64_t secondary_interaction_param;
        uint64_t id_of_cutscene_or_notification;

        static PlayerInteractionData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint32_t*>(p + 0x28),
                *reinterpret_cast<const uint64_t*>(p + 0x30),
                *reinterpret_cast<const uint64_t*>(p + 0x78)
            };
        }
    };

    inline int on_player_interaction_before(void(*cb)(const PlayerInteractionData&)) {
        return detail::subscribe_before(343, detail::trampoline<PlayerInteractionData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_player_interaction_after(void(*cb)(const PlayerInteractionData&)) {
        return detail::subscribe_after(343, detail::trampoline<PlayerInteractionData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when a player owned object was killed (param = killed object, param2 = killer object, p..
    struct PlayerOwnedObjectKilledData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t killed_object;
        uint32_t killer_object;
        uint64_t kill_method;

        static PlayerOwnedObjectKilledData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18),
                *reinterpret_cast<const uint32_t*>(p + 0x20),
                *reinterpret_cast<const uint64_t*>(p + 0x28)
            };
        }
    };

    inline int on_player_owned_object_killed_before(void(*cb)(const PlayerOwnedObjectKilledData&)) {
        return detail::subscribe_before(349, detail::trampoline<PlayerOwnedObjectKilledData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_player_owned_object_killed_after(void(*cb)(const PlayerOwnedObjectKilledData&)) {
        return detail::subscribe_after(349, detail::trampoline<PlayerOwnedObjectKilledData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when the player controlled ship or ship context is hit by a projectile (param = attacke..
    struct PlayerShipHitData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t attacked_object;
        uint64_t attacker;
        uint64_t projectile;

        static PlayerShipHitData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18),
                *reinterpret_cast<const uint64_t*>(p + 0x20),
                *reinterpret_cast<const uint64_t*>(p + 0x28)
            };
        }
    };

    inline int on_player_ship_hit_before(void(*cb)(const PlayerShipHitData&)) {
        return detail::subscribe_before(353, detail::trampoline<PlayerShipHitData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_player_ship_hit_after(void(*cb)(const PlayerShipHitData&)) {
        return detail::subscribe_after(353, detail::trampoline<PlayerShipHitData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when a trade is discovered by the player and added to the trade memory (param = tradeof..
    struct PlayerTradeDiscoveredData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t tradeoffer;

        static PlayerTradeDiscoveredData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18)
            };
        }
    };

    inline int on_player_trade_discovered_before(void(*cb)(const PlayerTradeDiscoveredData&)) {
        return detail::subscribe_before(354, detail::trampoline<PlayerTradeDiscoveredData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_player_trade_discovered_after(void(*cb)(const PlayerTradeDiscoveredData&)) {
        return detail::subscribe_after(354, detail::trampoline<PlayerTradeDiscoveredData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when the player should be warned, e.g. on station explosion (param = component, param2 ..
    struct PlayerWarningData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t component;
        uint32_t text_line;

        static PlayerWarningData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18),
                *reinterpret_cast<const uint32_t*>(p + 0x1C)
            };
        }
    };

    inline int on_player_warning_before(void(*cb)(const PlayerWarningData&)) {
        return detail::subscribe_before(357, detail::trampoline<PlayerWarningData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_player_warning_after(void(*cb)(const PlayerWarningData&)) {
        return detail::subscribe_after(357, detail::trampoline<PlayerWarningData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when a processing module is ready to accept new material (object = station the processi..
    struct ProcessingModuleAvailableData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t processing_module;

        static ProcessingModuleAvailableData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18)
            };
        }
    };

    inline int on_processing_module_available_before(void(*cb)(const ProcessingModuleAvailableData&)) {
        return detail::subscribe_before(364, detail::trampoline<ProcessingModuleAvailableData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_processing_module_available_after(void(*cb)(const ProcessingModuleAvailableData&)) {
        return detail::subscribe_after(364, detail::trampoline<ProcessingModuleAvailableData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when a player-owned productionmodule has cancelled its production queue (param = produc..
    struct ProductionCancelledData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t productionmodule;
        uint32_t whether_a_research_project_was;
        uint64_t list_of_cancelled_product_ware;

        static ProductionCancelledData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18),
                *reinterpret_cast<const uint32_t*>(p + 0x20),
                *reinterpret_cast<const uint64_t*>(p + 0x28)
            };
        }
    };

    inline int on_production_cancelled_before(void(*cb)(const ProductionCancelledData&)) {
        return detail::subscribe_before(365, detail::trampoline<ProductionCancelledData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_production_cancelled_after(void(*cb)(const ProductionCancelledData&)) {
        return detail::subscribe_after(365, detail::trampoline<ProductionCancelledData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when a player-owned productionmodule has finished production of a ware (param = product..
    struct ProductionFinishedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t productionmodule;
        uint64_t product_ware;
        uint64_t whether_this_was_a_research_pr;

        static ProductionFinishedData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18),
                *reinterpret_cast<const uint64_t*>(p + 0x20),
                *reinterpret_cast<const uint64_t*>(p + 0x48)
            };
        }
    };

    inline int on_production_finished_before(void(*cb)(const ProductionFinishedData&)) {
        return detail::subscribe_before(366, detail::trampoline<ProductionFinishedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_production_finished_after(void(*cb)(const ProductionFinishedData&)) {
        return detail::subscribe_after(366, detail::trampoline<ProductionFinishedData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when an object has started production of a ware (object = container or productionmodule..
    struct ProductionStartedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t productionmodule;
        uint64_t product_ware;
        uint32_t whether_this_is_a_research_pro;

        static ProductionStartedData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18),
                *reinterpret_cast<const uint64_t*>(p + 0x48),
                *reinterpret_cast<const uint32_t*>(p + 0x58)
            };
        }
    };

    inline int on_production_started_before(void(*cb)(const ProductionStartedData&)) {
        return detail::subscribe_before(369, detail::trampoline<ProductionStartedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_production_started_after(void(*cb)(const ProductionStartedData&)) {
        return detail::subscribe_after(369, detail::trampoline<ProductionStartedData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when an object has props about to be despawned (object = the populated component or its..
    struct PropsDespawningData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t component_which_was_populated;

        static PropsDespawningData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18)
            };
        }
    };

    inline int on_props_despawning_before(void(*cb)(const PropsDespawningData&)) {
        return detail::subscribe_before(370, detail::trampoline<PropsDespawningData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_props_despawning_after(void(*cb)(const PropsDespawningData&)) {
        return detail::subscribe_after(370, detail::trampoline<PropsDespawningData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when an object has had props generated (object = the populated component or its contain..
    struct PropsGeneratedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t component_which_was_populated;

        static PropsGeneratedData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18)
            };
        }
    };

    inline int on_props_generated_before(void(*cb)(const PropsGeneratedData&)) {
        return detail::subscribe_before(371, detail::trampoline<PropsGeneratedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_props_generated_after(void(*cb)(const PropsGeneratedData&)) {
        return detail::subscribe_after(371, detail::trampoline<PropsGeneratedData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when a region related asteroid has been mined in the specified sector, usually on destr..
    struct RegionAsteroidMinedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t mined_asteroid;
        uint64_t mining_object;

        static RegionAsteroidMinedData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18),
                *reinterpret_cast<const uint64_t*>(p + 0x20)
            };
        }
    };

    inline int on_region_asteroid_mined_before(void(*cb)(const RegionAsteroidMinedData&)) {
        return detail::subscribe_before(381, detail::trampoline<RegionAsteroidMinedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_region_asteroid_mined_after(void(*cb)(const RegionAsteroidMinedData&)) {
        return detail::subscribe_after(381, detail::trampoline<RegionAsteroidMinedData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when a faction or an object's control entity changes the relation towards the player (o..
    struct RelationChangedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t faction_or_null;
        uint64_t new_relation;
        uint32_t old_relation;
        uint32_t relationchangereason;

        static RelationChangedData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18),
                *reinterpret_cast<const uint64_t*>(p + 0x20),
                *reinterpret_cast<const uint32_t*>(p + 0x28),
                *reinterpret_cast<const uint32_t*>(p + 0x2C)
            };
        }
    };

    inline int on_relation_changed_before(void(*cb)(const RelationChangedData&)) {
        return detail::subscribe_before(382, detail::trampoline<RelationChangedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_relation_changed_after(void(*cb)(const RelationChangedData&)) {
        return detail::subscribe_after(382, detail::trampoline<RelationChangedData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for the specified object changing relation range (object = the object changing relation, pa..
    struct RelationRangeChangedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t faction_with_which_relation_ha;

        static RelationRangeChangedData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18)
            };
        }
    };

    inline int on_relation_range_changed_before(void(*cb)(const RelationRangeChangedData&)) {
        return detail::subscribe_before(383, detail::trampoline<RelationRangeChangedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_relation_range_changed_after(void(*cb)(const RelationRangeChangedData&)) {
        return detail::subscribe_after(383, detail::trampoline<RelationRangeChangedData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for the specified object having been removed from a formation (object = removed object)
    struct RemovedFromFormationData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        static RemovedFromFormationData from(const X4MdEvent* ev) { return { ev->source_id, ev->timestamp }; }
    };

    inline int on_removed_from_formation_before(void(*cb)(const RemovedFromFormationData&)) {
        return detail::subscribe_before(385, detail::trampoline<RemovedFromFormationData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_removed_from_formation_after(void(*cb)(const RemovedFromFormationData&)) {
        return detail::subscribe_after(385, detail::trampoline<RemovedFromFormationData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when repair of a component has been requested (object = the object containing the damag..
    struct RepairRequestedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t the_damaged_component;
        uint64_t whether_to_make_the_damaged_co;

        static RepairRequestedData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18),
                *reinterpret_cast<const uint64_t*>(p + 0x20)
            };
        }
    };

    inline int on_repair_requested_before(void(*cb)(const RepairRequestedData&)) {
        return detail::subscribe_before(389, detail::trampoline<RepairRequestedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_repair_requested_after(void(*cb)(const RepairRequestedData&)) {
        return detail::subscribe_after(389, detail::trampoline<RepairRequestedData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for resourceprobe being launched (object = the space which the resource probe was launched ..
    struct ResourceProbeLaunchedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t the_launching_defensible;
        uint64_t launched_resourceprobe;

        static ResourceProbeLaunchedData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18),
                *reinterpret_cast<const uint64_t*>(p + 0x48)
            };
        }
    };

    inline int on_resource_probe_launched_before(void(*cb)(const ResourceProbeLaunchedData&)) {
        return detail::subscribe_before(397, detail::trampoline<ResourceProbeLaunchedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_resource_probe_launched_after(void(*cb)(const ResourceProbeLaunchedData&)) {
        return detail::subscribe_after(397, detail::trampoline<ResourceProbeLaunchedData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for the specified object having been retrieved from internal storage (object = retrieved ob..
    struct RetrievedFromInternalStorageData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t new_docking_bay;
        uint64_t internal_docking_bay;

        static RetrievedFromInternalStorageData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18),
                *reinterpret_cast<const uint64_t*>(p + 0x20)
            };
        }
    };

    inline int on_retrieved_from_internal_storage_before(void(*cb)(const RetrievedFromInternalStorageData&)) {
        return detail::subscribe_before(401, detail::trampoline<RetrievedFromInternalStorageData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_retrieved_from_internal_storage_after(void(*cb)(const RetrievedFromInternalStorageData&)) {
        return detail::subscribe_after(401, detail::trampoline<RetrievedFromInternalStorageData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when an object has lost its salvage claim on an object (either for dismantling or towin..
    struct SalvageClaimLostData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t the_claimed_object;
        uint64_t the_successful_claiming_object;

        static SalvageClaimLostData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18),
                *reinterpret_cast<const uint64_t*>(p + 0x20)
            };
        }
    };

    inline int on_salvage_claim_lost_before(void(*cb)(const SalvageClaimLostData&)) {
        return detail::subscribe_before(403, detail::trampoline<SalvageClaimLostData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_salvage_claim_lost_after(void(*cb)(const SalvageClaimLostData&)) {
        return detail::subscribe_after(403, detail::trampoline<SalvageClaimLostData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for satellite being launched (object = the space which the satellite was launched in, param..
    struct SatelliteLaunchedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t the_launching_defensible;
        uint64_t launched_satellite;

        static SatelliteLaunchedData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18),
                *reinterpret_cast<const uint64_t*>(p + 0x48)
            };
        }
    };

    inline int on_satellite_launched_before(void(*cb)(const SatelliteLaunchedData&)) {
        return detail::subscribe_before(404, detail::trampoline<SatelliteLaunchedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_satellite_launched_after(void(*cb)(const SatelliteLaunchedData&)) {
        return detail::subscribe_after(404, detail::trampoline<SatelliteLaunchedData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when a (deep)scan is aborted (requires attribute scanner and/or scanned) (object = scan..
    struct ScanAbortedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t the_other_object;
        uint64_t true_iff_object_is_scanner;

        static ScanAbortedData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18),
                *reinterpret_cast<const uint64_t*>(p + 0x20)
            };
        }
    };

    inline int on_scan_aborted_before(void(*cb)(const ScanAbortedData&)) {
        return detail::subscribe_before(405, detail::trampoline<ScanAbortedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_scan_aborted_after(void(*cb)(const ScanAbortedData&)) {
        return detail::subscribe_after(405, detail::trampoline<ScanAbortedData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when a (deep)scan is finished (requires attribute scanner and/or scanned) (object = sca..
    struct ScanFinishedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t the_other_object;
        uint32_t true_iff_object_is_scanner;

        static ScanFinishedData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18),
                *reinterpret_cast<const uint32_t*>(p + 0x1C)
            };
        }
    };

    inline int on_scan_finished_before(void(*cb)(const ScanFinishedData&)) {
        return detail::subscribe_before(406, detail::trampoline<ScanFinishedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_scan_finished_after(void(*cb)(const ScanFinishedData&)) {
        return detail::subscribe_after(406, detail::trampoline<ScanFinishedData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when a (deep)scan is started (requires attribute scanner and/or scanned) (object = scan..
    struct ScanStartedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t the_other_object;
        uint64_t true_iff_object_is_scanner;

        static ScanStartedData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18),
                *reinterpret_cast<const uint64_t*>(p + 0x20)
            };
        }
    };

    inline int on_scan_started_before(void(*cb)(const ScanStartedData&)) {
        return detail::subscribe_before(407, detail::trampoline<ScanStartedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_scan_started_after(void(*cb)(const ScanStartedData&)) {
        return detail::subscribe_after(407, detail::trampoline<ScanStartedData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for any sector within the specified space changing owner (object = space which contains the..
    struct SectorChangedOwnerData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t sector_changing_ownership;
        uint64_t new_owner_faction;
        uint64_t previous_owner_faction;

        static SectorChangedOwnerData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18),
                *reinterpret_cast<const uint64_t*>(p + 0x20),
                *reinterpret_cast<const uint64_t*>(p + 0x28)
            };
        }
    };

    inline int on_sector_changed_owner_before(void(*cb)(const SectorChangedOwnerData&)) {
        return detail::subscribe_before(410, detail::trampoline<SectorChangedOwnerData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_sector_changed_owner_after(void(*cb)(const SectorChangedOwnerData&)) {
        return detail::subscribe_after(410, detail::trampoline<SectorChangedOwnerData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for any sector within the specified space changing true owner (object = space which contain..
    struct SectorChangedTrueOwnerData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t sector_changing_ownership;
        uint64_t new_true_owner_faction;
        uint64_t previous_true_owner_faction;

        static SectorChangedTrueOwnerData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18),
                *reinterpret_cast<const uint64_t*>(p + 0x20),
                *reinterpret_cast<const uint64_t*>(p + 0x28)
            };
        }
    };

    inline int on_sector_changed_true_owner_before(void(*cb)(const SectorChangedTrueOwnerData&)) {
        return detail::subscribe_before(411, detail::trampoline<SectorChangedTrueOwnerData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_sector_changed_true_owner_after(void(*cb)(const SectorChangedTrueOwnerData&)) {
        return detail::subscribe_after(411, detail::trampoline<SectorChangedTrueOwnerData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for a resource in sector being depleted by a certain amount (object = sector, param = resou..
    struct SectorResourceDepletedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t resource_ware;
        uint32_t depletion_amount;
        uint64_t sector_position;
        uint32_t killed_object;

        static SectorResourceDepletedData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18),
                *reinterpret_cast<const uint32_t*>(p + 0x20),
                *reinterpret_cast<const uint64_t*>(p + 0x40),
                *reinterpret_cast<const uint32_t*>(p + 0x58)
            };
        }
    };

    inline int on_sector_resource_depleted_before(void(*cb)(const SectorResourceDepletedData&)) {
        return detail::subscribe_before(413, detail::trampoline<SectorResourceDepletedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_sector_resource_depleted_after(void(*cb)(const SectorResourceDepletedData&)) {
        return detail::subscribe_after(413, detail::trampoline<SectorResourceDepletedData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for the specified object's shield being damaged (object = event source, param = damaged com..
    struct ShieldDamagedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t damaged_component;
        uint64_t new_shield_value;
        uint64_t damage;

        static ShieldDamagedData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18),
                *reinterpret_cast<const uint64_t*>(p + 0x20),
                *reinterpret_cast<const uint64_t*>(p + 0x30)
            };
        }
    };

    inline int on_shield_damaged_before(void(*cb)(const ShieldDamagedData&)) {
        return detail::subscribe_before(420, detail::trampoline<ShieldDamagedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_shield_damaged_after(void(*cb)(const ShieldDamagedData&)) {
        return detail::subscribe_after(420, detail::trampoline<ShieldDamagedData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event is raised when the player fails to unlock a signal leak (param = signal leak being unlocked)
    struct SignalUnlockFailedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t signal_leak_being_unlocked;

        static SignalUnlockFailedData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18)
            };
        }
    };

    inline int on_signal_unlock_failed_before(void(*cb)(const SignalUnlockFailedData&)) {
        return detail::subscribe_before(428, detail::trampoline<SignalUnlockFailedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_signal_unlock_failed_after(void(*cb)(const SignalUnlockFailedData&)) {
        return detail::subscribe_after(428, detail::trampoline<SignalUnlockFailedData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event is raised when the player has successfully unlock a signal leak (param = signal leak being ..
    struct SignalUnlockFinishedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t signal_leak_being_unlocked;
        uint64_t object_that_the_signal_leak_wa;

        static SignalUnlockFinishedData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18),
                *reinterpret_cast<const uint64_t*>(p + 0x20)
            };
        }
    };

    inline int on_signal_unlock_finished_before(void(*cb)(const SignalUnlockFinishedData&)) {
        return detail::subscribe_before(429, detail::trampoline<SignalUnlockFinishedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_signal_unlock_finished_after(void(*cb)(const SignalUnlockFinishedData&)) {
        return detail::subscribe_after(429, detail::trampoline<SignalUnlockFinishedData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event is raised when the player is not currently able to unlock a signal leak (param = signal lea..
    struct SignalUnlockImpossibleData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t signal_leak_being_unlocked;

        static SignalUnlockImpossibleData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18)
            };
        }
    };

    inline int on_signal_unlock_impossible_before(void(*cb)(const SignalUnlockImpossibleData&)) {
        return detail::subscribe_before(430, detail::trampoline<SignalUnlockImpossibleData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_signal_unlock_impossible_after(void(*cb)(const SignalUnlockImpossibleData&)) {
        return detail::subscribe_after(430, detail::trampoline<SignalUnlockImpossibleData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event is raised when the player begins unlocking a signal leak (param = signal leak being unlocked)
    struct SignalUnlockStartedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t signal_leak_being_unlocked;

        static SignalUnlockStartedData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18)
            };
        }
    };

    inline int on_signal_unlock_started_before(void(*cb)(const SignalUnlockStartedData&)) {
        return detail::subscribe_before(431, detail::trampoline<SignalUnlockStartedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_signal_unlock_started_after(void(*cb)(const SignalUnlockStartedData&)) {
        return detail::subscribe_after(431, detail::trampoline<SignalUnlockStartedData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when a space reservation is expired (object = reserving component, param = zone that th..
    struct SpaceReservationExpiredData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t zone_that_the_reservation_was;
        uint64_t reservation_index;

        static SpaceReservationExpiredData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18),
                *reinterpret_cast<const uint64_t*>(p + 0x20)
            };
        }
    };

    inline int on_space_reservation_expired_before(void(*cb)(const SpaceReservationExpiredData&)) {
        return detail::subscribe_before(432, detail::trampoline<SpaceReservationExpiredData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_space_reservation_expired_after(void(*cb)(const SpaceReservationExpiredData&)) {
        return detail::subscribe_after(432, detail::trampoline<SpaceReservationExpiredData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when an actor has finished speaking (object = actor, param = page, param2 = line, param..
    struct SpeakFinishedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t page;
        uint64_t line;
        uint32_t interrupted_0_if_fully_finishe;

        static SpeakFinishedData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18),
                *reinterpret_cast<const uint64_t*>(p + 0x20),
                *reinterpret_cast<const uint32_t*>(p + 0x24)
            };
        }
    };

    inline int on_speak_finished_before(void(*cb)(const SpeakFinishedData&)) {
        return detail::subscribe_before(434, detail::trampoline<SpeakFinishedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_speak_finished_after(void(*cb)(const SpeakFinishedData&)) {
        return detail::subscribe_after(434, detail::trampoline<SpeakFinishedData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when an actor has finished speaking a single text line, possibly a part of a speech wit..
    struct SpeakLineFinishedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t page;
        uint32_t line;

        static SpeakLineFinishedData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18),
                *reinterpret_cast<const uint32_t*>(p + 0x1C)
            };
        }
    };

    inline int on_speak_line_finished_before(void(*cb)(const SpeakLineFinishedData&)) {
        return detail::subscribe_before(435, detail::trampoline<SpeakLineFinishedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_speak_line_finished_after(void(*cb)(const SpeakLineFinishedData&)) {
        return detail::subscribe_after(435, detail::trampoline<SpeakLineFinishedData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when a speak timer was triggered in a &lt;speak&gt; or &lt;add_npc_line&gt; via the tim..
    struct SpeakTimerTriggeredData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t page;
        uint64_t line;
        uint32_t interrupted_0_if_speak_is_cont;

        static SpeakTimerTriggeredData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18),
                *reinterpret_cast<const uint64_t*>(p + 0x20),
                *reinterpret_cast<const uint32_t*>(p + 0x24)
            };
        }
    };

    inline int on_speak_timer_triggered_before(void(*cb)(const SpeakTimerTriggeredData&)) {
        return detail::subscribe_before(436, detail::trampoline<SpeakTimerTriggeredData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_speak_timer_triggered_after(void(*cb)(const SpeakTimerTriggeredData&)) {
        return detail::subscribe_after(436, detail::trampoline<SpeakTimerTriggeredData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when an object has activated a stance (object = object activating the stance, param = s..
    struct StanceActivatedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t stance_id;

        static StanceActivatedData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18)
            };
        }
    };

    inline int on_stance_activated_before(void(*cb)(const StanceActivatedData&)) {
        return detail::subscribe_before(437, detail::trampoline<StanceActivatedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_stance_activated_after(void(*cb)(const StanceActivatedData&)) {
        return detail::subscribe_after(437, detail::trampoline<StanceActivatedData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when the player adds a new subordinate to squad (param = subordinate, param2 = new assi..
    struct SubordinateAddedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t subordinate;
        uint64_t new_assignment;

        static SubordinateAddedData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18),
                *reinterpret_cast<const uint64_t*>(p + 0x20)
            };
        }
    };

    inline int on_subordinate_added_before(void(*cb)(const SubordinateAddedData&)) {
        return detail::subscribe_before(456, detail::trampoline<SubordinateAddedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_subordinate_added_after(void(*cb)(const SubordinateAddedData&)) {
        return detail::subscribe_after(456, detail::trampoline<SubordinateAddedData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when a new subordinate is promoted in place of the old commander (object = old commande..
    struct SubordinatePromotedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t promoted_subordinate;

        static SubordinatePromotedData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18)
            };
        }
    };

    inline int on_subordinate_promoted_before(void(*cb)(const SubordinatePromotedData&)) {
        return detail::subscribe_before(457, detail::trampoline<SubordinatePromotedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_subordinate_promoted_after(void(*cb)(const SubordinatePromotedData&)) {
        return detail::subscribe_after(457, detail::trampoline<SubordinatePromotedData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when a subordinate is removed from a commander (object = old commander, param = subordi..
    struct SubordinateRemovedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t subordinate;
        uint64_t old_commander;

        static SubordinateRemovedData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18),
                *reinterpret_cast<const uint64_t*>(p + 0x20)
            };
        }
    };

    inline int on_subordinate_removed_before(void(*cb)(const SubordinateRemovedData&)) {
        return detail::subscribe_before(458, detail::trampoline<SubordinateRemovedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_subordinate_removed_after(void(*cb)(const SubordinateRemovedData&)) {
        return detail::subscribe_after(458, detail::trampoline<SubordinateRemovedData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when an object's target is invalid, e.g. went through a jump gate, does not exist any m..
    struct TargetInvalidData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t invalid_target;
        uint64_t true_iff_the_target_is_actuall;

        static TargetInvalidData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18),
                *reinterpret_cast<const uint64_t*>(p + 0x20)
            };
        }
    };

    inline int on_target_invalid_before(void(*cb)(const TargetInvalidData&)) {
        return detail::subscribe_before(464, detail::trampoline<TargetInvalidData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_target_invalid_after(void(*cb)(const TargetInvalidData&)) {
        return detail::subscribe_after(464, detail::trampoline<TargetInvalidData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when the player failed to teleport. (param = destination room, param2 = is shortcut ins..
    struct TeleportFailedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t destination_room;
        uint64_t is_shortcut_instead_of_real_te;

        static TeleportFailedData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18),
                *reinterpret_cast<const uint64_t*>(p + 0x20)
            };
        }
    };

    inline int on_teleport_failed_before(void(*cb)(const TeleportFailedData&)) {
        return detail::subscribe_before(467, detail::trampoline<TeleportFailedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_teleport_failed_after(void(*cb)(const TeleportFailedData&)) {
        return detail::subscribe_after(467, detail::trampoline<TeleportFailedData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when the player successfully teleports. (param = new room, param2 = old room, param3 = ..
    struct TeleportSuccessfulData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t new_room;
        uint64_t old_room;
        uint64_t is_shortcut_instead_of_real_te;

        static TeleportSuccessfulData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18),
                *reinterpret_cast<const uint64_t*>(p + 0x20),
                *reinterpret_cast<const uint64_t*>(p + 0x47)
            };
        }
    };

    inline int on_teleport_successful_before(void(*cb)(const TeleportSuccessfulData&)) {
        return detail::subscribe_before(468, detail::trampoline<TeleportSuccessfulData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_teleport_successful_after(void(*cb)(const TeleportSuccessfulData&)) {
        return detail::subscribe_after(468, detail::trampoline<TeleportSuccessfulData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when a terraforming event has been completed (object = cluster in which terraforming is..
    struct TerraformingEventCompletedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t project_id;

        static TerraformingEventCompletedData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18)
            };
        }
    };

    inline int on_terraforming_event_completed_before(void(*cb)(const TerraformingEventCompletedData&)) {
        return detail::subscribe_before(474, detail::trampoline<TerraformingEventCompletedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_terraforming_event_completed_after(void(*cb)(const TerraformingEventCompletedData&)) {
        return detail::subscribe_after(474, detail::trampoline<TerraformingEventCompletedData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when a terraforming event has started (object = cluster in which terraforming is taking..
    struct TerraformingEventStartedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t project_id;

        static TerraformingEventStartedData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x48)
            };
        }
    };

    inline int on_terraforming_event_started_before(void(*cb)(const TerraformingEventStartedData&)) {
        return detail::subscribe_before(475, detail::trampoline<TerraformingEventStartedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_terraforming_event_started_after(void(*cb)(const TerraformingEventStartedData&)) {
        return detail::subscribe_after(475, detail::trampoline<TerraformingEventStartedData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when a terraforming world's habitability has changed (object = cluster in which terrafo..
    struct TerraformingHabitabilityChangedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        static TerraformingHabitabilityChangedData from(const X4MdEvent* ev) { return { ev->source_id, ev->timestamp }; }
    };

    inline int on_terraforming_habitability_changed_before(void(*cb)(const TerraformingHabitabilityChangedData&)) {
        return detail::subscribe_before(476, detail::trampoline<TerraformingHabitabilityChangedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_terraforming_habitability_changed_after(void(*cb)(const TerraformingHabitabilityChangedData&)) {
        return detail::subscribe_after(476, detail::trampoline<TerraformingHabitabilityChangedData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when a repeatable terraforming project has become available again (object = cluster in ..
    struct TerraformingProjectAvailableData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t project_id;

        static TerraformingProjectAvailableData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18)
            };
        }
    };

    inline int on_terraforming_project_available_before(void(*cb)(const TerraformingProjectAvailableData&)) {
        return detail::subscribe_before(477, detail::trampoline<TerraformingProjectAvailableData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_terraforming_project_available_after(void(*cb)(const TerraformingProjectAvailableData&)) {
        return detail::subscribe_after(477, detail::trampoline<TerraformingProjectAvailableData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when a terraforming project has been completed (object = cluster in which terraforming ..
    struct TerraformingProjectCompletedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t project_id;

        static TerraformingProjectCompletedData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18)
            };
        }
    };

    inline int on_terraforming_project_completed_before(void(*cb)(const TerraformingProjectCompletedData&)) {
        return detail::subscribe_before(478, detail::trampoline<TerraformingProjectCompletedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_terraforming_project_completed_after(void(*cb)(const TerraformingProjectCompletedData&)) {
        return detail::subscribe_after(478, detail::trampoline<TerraformingProjectCompletedData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when a terraforming project has been completed unsuccessfully (object = cluster in whic..
    struct TerraformingProjectFailedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t project_id;
        uint64_t were_there_positive_sideeffect;

        static TerraformingProjectFailedData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18),
                *reinterpret_cast<const uint64_t*>(p + 0x48)
            };
        }
    };

    inline int on_terraforming_project_failed_before(void(*cb)(const TerraformingProjectFailedData&)) {
        return detail::subscribe_before(479, detail::trampoline<TerraformingProjectFailedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_terraforming_project_failed_after(void(*cb)(const TerraformingProjectFailedData&)) {
        return detail::subscribe_after(479, detail::trampoline<TerraformingProjectFailedData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when a terraforming project has started after finishing production - also triggered if ..
    struct TerraformingProjectStartedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t project_id;

        static TerraformingProjectStartedData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18)
            };
        }
    };

    inline int on_terraforming_project_started_before(void(*cb)(const TerraformingProjectStartedData&)) {
        return detail::subscribe_before(481, detail::trampoline<TerraformingProjectStartedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_terraforming_project_started_after(void(*cb)(const TerraformingProjectStartedData&)) {
        return detail::subscribe_after(481, detail::trampoline<TerraformingProjectStartedData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when a terraforming project has been completed successfully (object = cluster in which ..
    struct TerraformingProjectSucceededData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t project_id;
        uint64_t were_there_positive_sideeffect;
        uint64_t were_there_negative_sideeffect;
        uint64_t payout_in_credits;

        static TerraformingProjectSucceededData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18),
                *reinterpret_cast<const uint64_t*>(p + 0x20),
                *reinterpret_cast<const uint64_t*>(p + 0x28),
                *reinterpret_cast<const uint64_t*>(p + 0x30)
            };
        }
    };

    inline int on_terraforming_project_succeeded_before(void(*cb)(const TerraformingProjectSucceededData&)) {
        return detail::subscribe_before(482, detail::trampoline<TerraformingProjectSucceededData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_terraforming_project_succeeded_after(void(*cb)(const TerraformingProjectSucceededData&)) {
        return detail::subscribe_after(482, detail::trampoline<TerraformingProjectSucceededData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when a terraforming stat has been added to the list of relevant stats (object = cluster..
    struct TerraformingStatAddedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t stat_id;

        static TerraformingStatAddedData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18)
            };
        }
    };

    inline int on_terraforming_stat_added_before(void(*cb)(const TerraformingStatAddedData&)) {
        return detail::subscribe_before(483, detail::trampoline<TerraformingStatAddedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_terraforming_stat_added_after(void(*cb)(const TerraformingStatAddedData&)) {
        return detail::subscribe_after(483, detail::trampoline<TerraformingStatAddedData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when a terraforming stat has changed (object = cluster in which terraforming is taking ..
    struct TerraformingStatChangedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t stat_id;

        static TerraformingStatChangedData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18)
            };
        }
    };

    inline int on_terraforming_stat_changed_before(void(*cb)(const TerraformingStatChangedData&)) {
        return detail::subscribe_before(484, detail::trampoline<TerraformingStatChangedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_terraforming_stat_changed_after(void(*cb)(const TerraformingStatChangedData&)) {
        return detail::subscribe_after(484, detail::trampoline<TerraformingStatChangedData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when a terraforming stat has been removed from the list of relevant stats (object = clu..
    struct TerraformingStatRemovedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t stat_id;

        static TerraformingStatRemovedData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18)
            };
        }
    };

    inline int on_terraforming_stat_removed_before(void(*cb)(const TerraformingStatRemovedData&)) {
        return detail::subscribe_before(485, detail::trampoline<TerraformingStatRemovedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_terraforming_stat_removed_after(void(*cb)(const TerraformingStatRemovedData&)) {
        return detail::subscribe_after(485, detail::trampoline<TerraformingStatRemovedData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when a trade is cancelled, (object = either buyer, seller or space - see following docu..
    struct TradeCancelledData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t tradeoffer;

        static TradeCancelledData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18)
            };
        }
    };

    inline int on_trade_cancelled_before(void(*cb)(const TradeCancelledData&)) {
        return detail::subscribe_before(490, detail::trampoline<TradeCancelledData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_trade_cancelled_after(void(*cb)(const TradeCancelledData&)) {
        return detail::subscribe_after(490, detail::trampoline<TradeCancelledData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when a trade is completed (object = either buyer, seller or space - see following docum..
    struct TradeCompletedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t tradeoffer;
        uint64_t tradeorder;

        static TradeCompletedData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18),
                *reinterpret_cast<const uint64_t*>(p + 0x20)
            };
        }
    };

    inline int on_trade_completed_before(void(*cb)(const TradeCompletedData&)) {
        return detail::subscribe_before(494, detail::trampoline<TradeCompletedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_trade_completed_after(void(*cb)(const TradeCompletedData&)) {
        return detail::subscribe_after(494, detail::trampoline<TradeCompletedData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when a trade is started (object = either buyer or seller - see following documentation,..
    struct TradeStartedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t tradeoffer;
        uint64_t tradeorder;

        static TradeStartedData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18),
                *reinterpret_cast<const uint64_t*>(p + 0x20)
            };
        }
    };

    inline int on_trade_started_before(void(*cb)(const TradeStartedData&)) {
        return detail::subscribe_before(495, detail::trampoline<TradeStartedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_trade_started_after(void(*cb)(const TradeStartedData&)) {
        return detail::subscribe_after(495, detail::trampoline<TradeStartedData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when the traffic level of an object changes (object = object changing the traffic level..
    struct TrafficLevelChangedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t new_traffic_level_string;
        uint32_t old_traffic_level_string;

        static TrafficLevelChangedData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18),
                *reinterpret_cast<const uint32_t*>(p + 0x1C)
            };
        }
    };

    inline int on_traffic_level_changed_before(void(*cb)(const TrafficLevelChangedData&)) {
        return detail::subscribe_before(496, detail::trampoline<TrafficLevelChangedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_traffic_level_changed_after(void(*cb)(const TrafficLevelChangedData&)) {
        return detail::subscribe_after(496, detail::trampoline<TrafficLevelChangedData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event is raised when the player ship travel mode aborts an ongoing charging process
    struct TravelModeChargeAbortedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        static TravelModeChargeAbortedData from(const X4MdEvent* ev) { return { ev->source_id, ev->timestamp }; }
    };

    inline int on_travel_mode_charge_aborted_before(void(*cb)(const TravelModeChargeAbortedData&)) {
        return detail::subscribe_before(500, detail::trampoline<TravelModeChargeAbortedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_travel_mode_charge_aborted_after(void(*cb)(const TravelModeChargeAbortedData&)) {
        return detail::subscribe_after(500, detail::trampoline<TravelModeChargeAbortedData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event is raised when the player ship travel mode begins charging (param = time when the mode is f..
    struct TravelModeChargeStartedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t time_when_the_mode_is_fully_ch;

        static TravelModeChargeStartedData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x48)
            };
        }
    };

    inline int on_travel_mode_charge_started_before(void(*cb)(const TravelModeChargeStartedData&)) {
        return detail::subscribe_before(501, detail::trampoline<TravelModeChargeStartedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_travel_mode_charge_started_after(void(*cb)(const TravelModeChargeStartedData&)) {
        return detail::subscribe_after(501, detail::trampoline<TravelModeChargeStartedData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event is raised when the player ship travel mode is done charging
    struct TravelModeStartedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        static TravelModeStartedData from(const X4MdEvent* ev) { return { ev->source_id, ev->timestamp }; }
    };

    inline int on_travel_mode_started_before(void(*cb)(const TravelModeStartedData&)) {
        return detail::subscribe_before(502, detail::trampoline<TravelModeStartedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_travel_mode_started_after(void(*cb)(const TravelModeStartedData&)) {
        return detail::subscribe_after(502, detail::trampoline<TravelModeStartedData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event is raised when the player ship travel mode has stopped
    struct TravelModeStoppedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        static TravelModeStoppedData from(const X4MdEvent* ev) { return { ev->source_id, ev->timestamp }; }
    };

    inline int on_travel_mode_stopped_before(void(*cb)(const TravelModeStoppedData&)) {
        return detail::subscribe_before(503, detail::trampoline<TravelModeStoppedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_travel_mode_stopped_after(void(*cb)(const TravelModeStoppedData&)) {
        return detail::subscribe_after(503, detail::trampoline<TravelModeStoppedData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when a UI control is triggered (param = screen ID, param2 = control ID, param3 = screen..
    struct UITriggeredData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t screen_id;
        uint64_t control_id;
        uint64_t screen_parameters;

        static UITriggeredData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18),
                *reinterpret_cast<const uint64_t*>(p + 0x20),
                *reinterpret_cast<const uint64_t*>(p + 0x28)
            };
        }
    };

    inline int on_u_i_triggered_before(void(*cb)(const UITriggeredData&)) {
        return detail::subscribe_before(509, detail::trampoline<UITriggeredData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_u_i_triggered_after(void(*cb)(const UITriggeredData&)) {
        return detail::subscribe_after(509, detail::trampoline<UITriggeredData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for the specified object undocking (object = undocking object, param = dock, param2 = zone,..
    struct UndockedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t dock;
        uint64_t zone;
        uint64_t docking_bay;

        static UndockedData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18),
                *reinterpret_cast<const uint64_t*>(p + 0x20),
                *reinterpret_cast<const uint64_t*>(p + 0x28)
            };
        }
    };

    inline int on_undocked_before(void(*cb)(const UndockedData&)) {
        return detail::subscribe_before(510, detail::trampoline<UndockedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_undocked_after(void(*cb)(const UndockedData&)) {
        return detail::subscribe_after(510, detail::trampoline<UndockedData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for the specified object being cleared to undock (object = docked object)
    struct UndockingClearedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        static UndockingClearedData from(const X4MdEvent* ev) { return { ev->source_id, ev->timestamp }; }
    };

    inline int on_undocking_cleared_before(void(*cb)(const UndockingClearedData&)) {
        return detail::subscribe_before(511, detail::trampoline<UndockingClearedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_undocking_cleared_after(void(*cb)(const UndockingClearedData&)) {
        return detail::subscribe_after(511, detail::trampoline<UndockingClearedData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for the specified object starting to undock (object = undocking object, param = dock, param..
    struct UndockingStartedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t dock;
        uint64_t zone;
        uint64_t docking_bay;

        static UndockingStartedData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18),
                *reinterpret_cast<const uint64_t*>(p + 0x20),
                *reinterpret_cast<const uint64_t*>(p + 0x28)
            };
        }
    };

    inline int on_undocking_started_before(void(*cb)(const UndockingStartedData&)) {
        return detail::subscribe_before(513, detail::trampoline<UndockingStartedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_undocking_started_after(void(*cb)(const UndockingStartedData&)) {
        return detail::subscribe_after(513, detail::trampoline<UndockingStartedData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when a unit has been destroyed (object = the object owning the unit, param = the unit's..
    struct UnitDestroyedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t the_units_category;
        uint32_t the_units_mk;

        static UnitDestroyedData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18),
                *reinterpret_cast<const uint32_t*>(p + 0x1C)
            };
        }
    };

    inline int on_unit_destroyed_before(void(*cb)(const UnitDestroyedData&)) {
        return detail::subscribe_before(514, detail::trampoline<UnitDestroyedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_unit_destroyed_after(void(*cb)(const UnitDestroyedData&)) {
        return detail::subscribe_after(514, detail::trampoline<UnitDestroyedData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when a unit has started building/welding (object = the object on which the unit operate..
    struct UnitStartedBuildingData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint32_t the_units_mk;

        static UnitStartedBuildingData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint32_t*>(p + 0x18)
            };
        }
    };

    inline int on_unit_started_building_before(void(*cb)(const UnitStartedBuildingData&)) {
        return detail::subscribe_before(515, detail::trampoline<UnitStartedBuildingData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_unit_started_building_after(void(*cb)(const UnitStartedBuildingData&)) {
        return detail::subscribe_after(515, detail::trampoline<UnitStartedBuildingData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when a unit has started repairing/welding (object = the object on which the unit operat..
    struct UnitStartedRepairingData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint32_t the_units_mk;

        static UnitStartedRepairingData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint32_t*>(p + 0x18)
            };
        }
    };

    inline int on_unit_started_repairing_before(void(*cb)(const UnitStartedRepairingData&)) {
        return detail::subscribe_before(516, detail::trampoline<UnitStartedRepairingData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_unit_started_repairing_after(void(*cb)(const UnitStartedRepairingData&)) {
        return detail::subscribe_after(516, detail::trampoline<UnitStartedRepairingData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when a unit has stopped building/welding (object = the object on which the unit operate..
    struct UnitStoppedBuildingData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint32_t the_units_mk;

        static UnitStoppedBuildingData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint32_t*>(p + 0x18)
            };
        }
    };

    inline int on_unit_stopped_building_before(void(*cb)(const UnitStoppedBuildingData&)) {
        return detail::subscribe_before(517, detail::trampoline<UnitStoppedBuildingData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_unit_stopped_building_after(void(*cb)(const UnitStoppedBuildingData&)) {
        return detail::subscribe_after(517, detail::trampoline<UnitStoppedBuildingData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when a unit has stopped repairing/welding (object = the object on which the unit operat..
    struct UnitStoppedRepairingData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint32_t the_units_mk;

        static UnitStoppedRepairingData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint32_t*>(p + 0x18)
            };
        }
    };

    inline int on_unit_stopped_repairing_before(void(*cb)(const UnitStoppedRepairingData&)) {
        return detail::subscribe_before(518, detail::trampoline<UnitStoppedRepairingData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_unit_stopped_repairing_after(void(*cb)(const UnitStoppedRepairingData&)) {
        return detail::subscribe_after(518, detail::trampoline<UnitStoppedRepairingData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when the universe generation process has been completed (all stations have been built) ..
    struct UniverseGeneratedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        static UniverseGeneratedData from(const X4MdEvent* ev) { return { ev->source_id, ev->timestamp }; }
    };

    inline int on_universe_generated_before(void(*cb)(const UniverseGeneratedData&)) {
        return detail::subscribe_before(519, detail::trampoline<UniverseGeneratedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_universe_generated_after(void(*cb)(const UniverseGeneratedData&)) {
        return detail::subscribe_after(519, detail::trampoline<UniverseGeneratedData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for when a weapon has changed its ammo (object = weapon, param = new ammo macro, can be null)
    struct WeaponAmmoChangedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t new_ammo_macro;

        static WeaponAmmoChangedData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18)
            };
        }
    };

    inline int on_weapon_ammo_changed_before(void(*cb)(const WeaponAmmoChangedData&)) {
        return detail::subscribe_before(541, detail::trampoline<WeaponAmmoChangedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_weapon_ammo_changed_after(void(*cb)(const WeaponAmmoChangedData&)) {
        return detail::subscribe_after(541, detail::trampoline<WeaponAmmoChangedData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for the specified weapon firing (object = the weapon, param = fired bullet/missile/bomb)
    struct WeaponFiredData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        uint64_t fired_bulletmissilebomb;

        static WeaponFiredData from(const X4MdEvent* ev) {
            auto* p = static_cast<const uint8_t*>(ev->raw_event);
            return {
                ev->source_id,
                ev->timestamp,
                *reinterpret_cast<const uint64_t*>(p + 0x18)
            };
        }
    };

    inline int on_weapon_fired_before(void(*cb)(const WeaponFiredData&)) {
        return detail::subscribe_before(543, detail::trampoline<WeaponFiredData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_weapon_fired_after(void(*cb)(const WeaponFiredData&)) {
        return detail::subscribe_after(543, detail::trampoline<WeaponFiredData>,
            reinterpret_cast<void*>(cb));
    }

    /// Event for the specified weapon overheating (object = the weapon)
    struct WeaponOverheatedData {
        uint64_t source_id;        // Event source entity (X4MdEvent)
        double   timestamp;         // Game time (X4MdEvent)
        static WeaponOverheatedData from(const X4MdEvent* ev) { return { ev->source_id, ev->timestamp }; }
    };

    inline int on_weapon_overheated_before(void(*cb)(const WeaponOverheatedData&)) {
        return detail::subscribe_before(548, detail::trampoline<WeaponOverheatedData>,
            reinterpret_cast<void*>(cb));
    }

    inline int on_weapon_overheated_after(void(*cb)(const WeaponOverheatedData&)) {
        return detail::subscribe_after(548, detail::trampoline<WeaponOverheatedData>,
            reinterpret_cast<void*>(cb));
    }


} // namespace x4n::md
