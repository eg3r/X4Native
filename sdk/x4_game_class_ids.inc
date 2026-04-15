// ==========================================================================
// x4_game_class_ids.inc - X4 Entity Class IDs
// ==========================================================================
// Auto-generated from class_ids.csv (game version: 900-605025).
// 120 classes, sentinel = 120.
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
    Collectableblueprints     = 19,
    Collectablewares          = 20,
    Component                 = 21,
    Computer                  = 22,
    Connectionmodule          = 23,
    Controlroom               = 24,
    Countermeasure            = 25,
    Crate                     = 26,
    CrateL                    = 27,
    CrateM                    = 28,
    CrateS                    = 29,
    Crystal                   = 30,
    Cutsceneanchor            = 31,
    Datavault                 = 32,
    Defencemodule             = 33,
    Defensible                = 34,
    Destructible              = 35,
    Detector                  = 36,
    Dismantleprocessor        = 37,
    Dockarea                  = 38,
    Dockingbay                = 39,
    Drop                      = 40,
    Effectobject              = 41,
    Engine                    = 42,
    Entity                    = 43,
    Forceemitter              = 44,
    Fogvolume                 = 45,
    Galaxy                    = 46,
    Gate                      = 47,
    Habitation                = 48,
    Hackerprobe               = 49,
    Highway                   = 50,
    Highwayblocker            = 51,
    Highwaybooster            = 52,
    Highwayentrygate          = 53,
    Highwayexitgate           = 54,
    Highwayscene              = 55,
    Highwaytrigger            = 56,
    Holomap                   = 57,
    Influenceobject           = 58,
    Landmarkobject            = 59,
    Lensflare                 = 60,
    Lock                      = 61,
    Lockbox                   = 62,
    Mine                      = 63,
    Miningnode                = 64,
    Missile                   = 65,
    Missilelauncher           = 66,
    Missileturret             = 67,
    Moon                      = 68,
    Navbeacon                 = 69,
    Navcontext                = 70,
    Npc                       = 71,
    Object                    = 72,
    Pier                      = 73,
    Planet                    = 74,
    Player                    = 75,
    Positional                = 76,
    Processingmodule          = 77,
    Production                = 78,
    Radar                     = 79,
    Recyclable                = 80,
    Region                    = 81,
    Resourceprobe             = 82,
    Room                      = 83,
    Satellite                 = 84,
    Scanner                   = 85,
    Scene                     = 86,
    Sector                    = 87,
    Shieldgenerator           = 88,
    ShipXs                    = 89,
    ShipS                     = 90,
    ShipM                     = 91,
    ShipL                     = 92,
    ShipXl                    = 93,
    Signalleak                = 94,
    Spacesuit                 = 95,
    Stardust                  = 96,
    Station                   = 97,
    Storage                   = 98,
    Sun                       = 99,
    Switchable                = 100,
    Targetpoint               = 101,
    Textdisplay               = 102,
    Turret                    = 103,
    Uielement                 = 104,
    Ventureplatform           = 105,
    Weapon                    = 106,
    Welfaremodule             = 107,
    Zone                      = 108,
    Collectable               = 109,
    Container                 = 110,
    Controllable              = 111,
    Explosive                 = 112,
    Launcher                  = 113,
    Module                    = 114,
    Nonplayer                 = 115,
    Ship                      = 116,
    Space                     = 117,
    Triggerobject             = 118,
    Walkablemodule            = 119
};

/// Internal sentinel value -- not a real class. Returned by get_class() on failure.
/// Extension code should check for nullptr/null component instead of comparing to this.
inline constexpr uint32_t GAMECLASS_SENTINEL = 120;

} // namespace x4n
#endif
