// ==========================================================================
// x4_game_class_ids.inc - X4 Entity Class IDs
// ==========================================================================
// Auto-generated from class_ids.csv (game version: 900-607977).
// 121 classes, sentinel = 121.
//
// Usage:
//   x4n::GameClass cls = x4n::entity::get_class_id(comp);
//   if (cls == x4n::GameClass::Station) { ... }
//   if (x4n::entity::is_derived_from(comp, x4n::GameClass::Container)) { ... }
//
// WARNING: Class IDs shift between game builds when new classes are inserted.
// DO NOT EDIT - regenerate with: .\scripts\generate_class_ids.ps1
// ==========================================================================

#ifdef __cplusplus
namespace x4n {

/// Entity class ID enum. Values are auto-generated per game build.
/// Use with x4n::entity::get_class_id() and x4n::entity::is_derived_from().
enum class GameClass : uint32_t {
    Accessory                 = 0,
    Adsign                    = 1,
    Adsignobject              = 2,
    Anomaly                   = 3,
    Asteroid                  = 4,
    Attachment                = 5,
    Bomb                      = 6,
    Bomblauncher              = 7,
    Buildstorage              = 8,
    Buildmodule               = 9,
    Buildprocessor            = 10,
    Bullet                    = 11,
    Cargobay                  = 12,
    Celestialbody             = 13,
    Checkpoint                = 14,
    Cluster                   = 15,
    Cockpit                   = 16,
    Collectableshieldrestore  = 17,
    Collectableammo           = 18,
    Collectabledata           = 19,
    Collectableblueprints     = 20,
    Collectablewares          = 21,
    Component                 = 22,
    Computer                  = 23,
    Connectionmodule          = 24,
    Controlroom               = 25,
    Countermeasure            = 26,
    Crate                     = 27,
    CrateL                    = 28,
    CrateM                    = 29,
    CrateS                    = 30,
    Crystal                   = 31,
    Cutsceneanchor            = 32,
    Datavault                 = 33,
    Defencemodule             = 34,
    Defensible                = 35,
    Destructible              = 36,
    Detector                  = 37,
    Dismantleprocessor        = 38,
    Dockarea                  = 39,
    Dockingbay                = 40,
    Drop                      = 41,
    Effectobject              = 42,
    Engine                    = 43,
    Entity                    = 44,
    Forceemitter              = 45,
    Fogvolume                 = 46,
    Galaxy                    = 47,
    Gate                      = 48,
    Habitation                = 49,
    Hackerprobe               = 50,
    Highway                   = 51,
    Highwayblocker            = 52,
    Highwaybooster            = 53,
    Highwayentrygate          = 54,
    Highwayexitgate           = 55,
    Highwayscene              = 56,
    Highwaytrigger            = 57,
    Holomap                   = 58,
    Influenceobject           = 59,
    Landmarkobject            = 60,
    Lensflare                 = 61,
    Lock                      = 62,
    Lockbox                   = 63,
    Mine                      = 64,
    Miningnode                = 65,
    Missile                   = 66,
    Missilelauncher           = 67,
    Missileturret             = 68,
    Moon                      = 69,
    Navbeacon                 = 70,
    Navcontext                = 71,
    Npc                       = 72,
    Object                    = 73,
    Pier                      = 74,
    Planet                    = 75,
    Player                    = 76,
    Positional                = 77,
    Processingmodule          = 78,
    Production                = 79,
    Radar                     = 80,
    Recyclable                = 81,
    Region                    = 82,
    Resourceprobe             = 83,
    Room                      = 84,
    Satellite                 = 85,
    Scanner                   = 86,
    Scene                     = 87,
    Sector                    = 88,
    Shieldgenerator           = 89,
    ShipXs                    = 90,
    ShipS                     = 91,
    ShipM                     = 92,
    ShipL                     = 93,
    ShipXl                    = 94,
    Signalleak                = 95,
    Spacesuit                 = 96,
    Stardust                  = 97,
    Station                   = 98,
    Storage                   = 99,
    Sun                       = 100,
    Switchable                = 101,
    Targetpoint               = 102,
    Textdisplay               = 103,
    Turret                    = 104,
    Uielement                 = 105,
    Ventureplatform           = 106,
    Weapon                    = 107,
    Welfaremodule             = 108,
    Zone                      = 109,
    Collectable               = 110,
    Container                 = 111,
    Controllable              = 112,
    Explosive                 = 113,
    Launcher                  = 114,
    Module                    = 115,
    Nonplayer                 = 116,
    Ship                      = 117,
    Space                     = 118,
    Triggerobject             = 119,
    Walkablemodule            = 120
};

/// Internal sentinel value -- not a real class. Returned by get_class() on failure.
/// Extension code should check for nullptr/null component instead of comparing to this.
inline constexpr uint32_t GAMECLASS_SENTINEL = 121;

/// Engine-facing class name (raw form from class_ids.csv).
/// Used by x4n::entity::find_ancestor() to pass GameClass into GetContextByClass,
/// and by typed wrappers around IsComponentClass / IsRealComponentClass.
/// Returns nullptr for out-of-range values (graceful fallback: caller gets null result).
inline const char* class_name(GameClass c) {
    static constexpr const char* NAMES[] = {
        "accessory"                 , // 0 Accessory
        "adsign"                    , // 1 Adsign
        "adsignobject"              , // 2 Adsignobject
        "anomaly"                   , // 3 Anomaly
        "asteroid"                  , // 4 Asteroid
        "attachment"                , // 5 Attachment
        "bomb"                      , // 6 Bomb
        "bomblauncher"              , // 7 Bomblauncher
        "buildstorage"              , // 8 Buildstorage
        "buildmodule"               , // 9 Buildmodule
        "buildprocessor"            , // 10 Buildprocessor
        "bullet"                    , // 11 Bullet
        "cargobay"                  , // 12 Cargobay
        "celestialbody"             , // 13 Celestialbody
        "checkpoint"                , // 14 Checkpoint
        "cluster"                   , // 15 Cluster
        "cockpit"                   , // 16 Cockpit
        "collectableshieldrestore"  , // 17 Collectableshieldrestore
        "collectableammo"           , // 18 Collectableammo
        "collectabledata"           , // 19 Collectabledata
        "collectableblueprints"     , // 20 Collectableblueprints
        "collectablewares"          , // 21 Collectablewares
        "component"                 , // 22 Component
        "computer"                  , // 23 Computer
        "connectionmodule"          , // 24 Connectionmodule
        "controlroom"               , // 25 Controlroom
        "countermeasure"            , // 26 Countermeasure
        "crate"                     , // 27 Crate
        "crate_l"                   , // 28 CrateL
        "crate_m"                   , // 29 CrateM
        "crate_s"                   , // 30 CrateS
        "crystal"                   , // 31 Crystal
        "cutsceneanchor"            , // 32 Cutsceneanchor
        "datavault"                 , // 33 Datavault
        "defencemodule"             , // 34 Defencemodule
        "defensible"                , // 35 Defensible
        "destructible"              , // 36 Destructible
        "detector"                  , // 37 Detector
        "dismantleprocessor"        , // 38 Dismantleprocessor
        "dockarea"                  , // 39 Dockarea
        "dockingbay"                , // 40 Dockingbay
        "drop"                      , // 41 Drop
        "effectobject"              , // 42 Effectobject
        "engine"                    , // 43 Engine
        "entity"                    , // 44 Entity
        "forceemitter"              , // 45 Forceemitter
        "fogvolume"                 , // 46 Fogvolume
        "galaxy"                    , // 47 Galaxy
        "gate"                      , // 48 Gate
        "habitation"                , // 49 Habitation
        "hackerprobe"               , // 50 Hackerprobe
        "highway"                   , // 51 Highway
        "highwayblocker"            , // 52 Highwayblocker
        "highwaybooster"            , // 53 Highwaybooster
        "highwayentrygate"          , // 54 Highwayentrygate
        "highwayexitgate"           , // 55 Highwayexitgate
        "highwayscene"              , // 56 Highwayscene
        "highwaytrigger"            , // 57 Highwaytrigger
        "holomap"                   , // 58 Holomap
        "influenceobject"           , // 59 Influenceobject
        "landmarkobject"            , // 60 Landmarkobject
        "lensflare"                 , // 61 Lensflare
        "lock"                      , // 62 Lock
        "lockbox"                   , // 63 Lockbox
        "mine"                      , // 64 Mine
        "miningnode"                , // 65 Miningnode
        "missile"                   , // 66 Missile
        "missilelauncher"           , // 67 Missilelauncher
        "missileturret"             , // 68 Missileturret
        "moon"                      , // 69 Moon
        "navbeacon"                 , // 70 Navbeacon
        "navcontext"                , // 71 Navcontext
        "npc"                       , // 72 Npc
        "object"                    , // 73 Object
        "pier"                      , // 74 Pier
        "planet"                    , // 75 Planet
        "player"                    , // 76 Player
        "positional"                , // 77 Positional
        "processingmodule"          , // 78 Processingmodule
        "production"                , // 79 Production
        "radar"                     , // 80 Radar
        "recyclable"                , // 81 Recyclable
        "region"                    , // 82 Region
        "resourceprobe"             , // 83 Resourceprobe
        "room"                      , // 84 Room
        "satellite"                 , // 85 Satellite
        "scanner"                   , // 86 Scanner
        "scene"                     , // 87 Scene
        "sector"                    , // 88 Sector
        "shieldgenerator"           , // 89 Shieldgenerator
        "ship_xs"                   , // 90 ShipXs
        "ship_s"                    , // 91 ShipS
        "ship_m"                    , // 92 ShipM
        "ship_l"                    , // 93 ShipL
        "ship_xl"                   , // 94 ShipXl
        "signalleak"                , // 95 Signalleak
        "spacesuit"                 , // 96 Spacesuit
        "stardust"                  , // 97 Stardust
        "station"                   , // 98 Station
        "storage"                   , // 99 Storage
        "sun"                       , // 100 Sun
        "switchable"                , // 101 Switchable
        "targetpoint"               , // 102 Targetpoint
        "textdisplay"               , // 103 Textdisplay
        "turret"                    , // 104 Turret
        "uielement"                 , // 105 Uielement
        "ventureplatform"           , // 106 Ventureplatform
        "weapon"                    , // 107 Weapon
        "welfaremodule"             , // 108 Welfaremodule
        "zone"                      , // 109 Zone
        "collectable"               , // 110 Collectable
        "container"                 , // 111 Container
        "controllable"              , // 112 Controllable
        "explosive"                 , // 113 Explosive
        "launcher"                  , // 114 Launcher
        "module"                    , // 115 Module
        "nonplayer"                 , // 116 Nonplayer
        "ship"                      , // 117 Ship
        "space"                     , // 118 Space
        "triggerobject"             , // 119 Triggerobject
        "walkablemodule"             // 120 Walkablemodule
    };
    auto idx = static_cast<size_t>(c);
    return idx < (sizeof(NAMES) / sizeof(NAMES[0])) ? NAMES[idx] : nullptr;
}

} // namespace x4n
#endif
