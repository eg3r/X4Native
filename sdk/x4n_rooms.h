// ---------------------------------------------------------------------------
// x4n_rooms.h — Walkable Interior Utilities
// ---------------------------------------------------------------------------
// Part of the X4Native SDK. Included by x4native.h.
//
// Provides:
//   x4n::rooms::roomtype_name()  — convert X4RoomType enum to string
//
// Pure function — no game state dependency (safe to call anytime).
// ---------------------------------------------------------------------------
#pragma once

#include "x4n_core.h"

namespace x4n { namespace rooms {

/// Convert an X4RoomType enum value to its lowercase string name.
/// Returns nullptr for out-of-range or sentinel values.
/// @stability STABLE — enum mapping, verified against game data.
/// @verified v9.00 build 600626
inline const char* roomtype_name(X4RoomType type) {
    #define X4N_RT(e, s) case e: return s;
    switch (type) {
        X4N_RT(X4_ROOMTYPE_BAR,               "bar")
        X4N_RT(X4_ROOMTYPE_CASINO,            "casino")
        X4N_RT(X4_ROOMTYPE_CORRIDOR,          "corridor")
        X4N_RT(X4_ROOMTYPE_CREWQUARTERS,      "crewquarters")
        X4N_RT(X4_ROOMTYPE_EMBASSY,           "embassy")
        X4N_RT(X4_ROOMTYPE_FACTIONREP,        "factionrep")
        X4N_RT(X4_ROOMTYPE_GENERATORROOM,     "generatorroom")
        X4N_RT(X4_ROOMTYPE_INFRASTRUCTURE,    "infrastructure")
        X4N_RT(X4_ROOMTYPE_INTELLIGENCEOFFICE,"intelligenceoffice")
        X4N_RT(X4_ROOMTYPE_LIVINGROOM,        "livingroom")
        X4N_RT(X4_ROOMTYPE_MANAGER,           "manager")
        X4N_RT(X4_ROOMTYPE_OFFICE,            "office")
        X4N_RT(X4_ROOMTYPE_PLAYEROFFICE,      "playeroffice")
        X4N_RT(X4_ROOMTYPE_PRISON,            "prison")
        X4N_RT(X4_ROOMTYPE_SECURITY,          "security")
        X4N_RT(X4_ROOMTYPE_SERVERROOM,        "serverroom")
        X4N_RT(X4_ROOMTYPE_SERVICEROOM,       "serviceroom")
        X4N_RT(X4_ROOMTYPE_SHIPTRADERCORNER,  "shiptradercorner")
        X4N_RT(X4_ROOMTYPE_TRADERCORNER,      "tradercorner")
        X4N_RT(X4_ROOMTYPE_TRAFFICCONTROL,    "trafficcontrol")
        X4N_RT(X4_ROOMTYPE_WARROOM,           "warroom")
        default: return nullptr;
    }
    #undef X4N_RT
}

}} // namespace x4n::rooms
