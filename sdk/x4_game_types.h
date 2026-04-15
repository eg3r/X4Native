// ==========================================================================
// x4_game_types.h - X4: Foundations Game Type Definitions
// ==========================================================================
// Auto-generated from X4 v9.00-605025 FFI declarations.
// Source: reference/x4_ffi_raw.txt
//
// DO NOT EDIT - regenerate with: .\scripts\generate_headers.ps1
// ==========================================================================
#pragma once

// Game build version these types were extracted from.
// Extensions can use this to guard against struct layout mismatches.
#define X4_GAME_TYPES_BUILD 900
// Full version label - includes beta/hotfix suffix when applicable.
#define X4_GAME_VERSION_LABEL "900-605025"

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// --------------------------------------------------------------------------
// Handle Types (13)
// --------------------------------------------------------------------------

typedef uint64_t OperationID;
typedef uint64_t BuildTaskID;
typedef uint64_t UniverseID;
typedef uint64_t AIOrderID;
typedef int32_t BlacklistID;
typedef int32_t FightRuleID;
typedef uint64_t FleetUnitID;
typedef uint64_t MissionID;
typedef uint64_t NPCSeed;
typedef uint64_t TradeID;
typedef int32_t TradeRuleID;
typedef uint64_t MessageID;
typedef uint64_t TickerCacheID;

// --------------------------------------------------------------------------
// Struct Types (283 unique, dependency-ordered)
// --------------------------------------------------------------------------

typedef struct {
    double fps;
    double moveTime;
    double renderTime;
    double gpuTime;
} FPSDetails;

typedef struct {
    int major;
    int minor;
} GameVersion;

typedef struct {
    const char* id;
    const char* name;
    const char* icon;
    const char* factoryname;
    const char* factorydesc;
    const char* factorymapicon;
    const char* factoryhudicon;
    const char* tags;
    uint32_t tier;
    uint32_t priority;
} WareGroupInfo2;

typedef struct {
    const char* id;
    const char* category;
    const char* paramtype;
    const char* name;
    const char* desc;
    const char* shortdesc;
    const char* iconid;
    const char* imageid;
    const char* rewardtext;
    const char* successtext;
    const char* failuretext;
    const char* agenttype;
    const char* agentexpname;
    const char* agentrisk;
    const char* giftwaretags;
    double duration;
    double cooldown;
    int32_t agentexp;
    int32_t successchance;
    int64_t price;
    int32_t influencerequirement;
    uint32_t exclusivefactionparamidx;
    uint32_t warecostscaleparamidx;
    uint32_t targetobjectparamidx;
    uint32_t numwarerequirements;
    bool unique;
    bool hidden;
    bool eventtrigger;
} DiplomacyActionInfo;

typedef struct {
    OperationID id;
    const char* actionid;
    UniverseID agentid;
    const char* agentname;
    const char* agentimageid;
    const char* agentresultstate;
    int32_t agentexp_negotiation;
    int32_t agentexp_espionage;
    const char* giftwareid;
    double starttime;
    double endtime;
    bool read;
    bool successful;
} DiplomacyActionOperation;

typedef struct {
    const char* rankname;
    const char* rankiconid;
    const char* exp_negotiation_name;
    const char* exp_espionage_name;
} DiplomacyAgentAttributeData;

typedef struct {
    const char* id;
    const char* name;
    const char* desc;
    const char* shortdesc;
    const char* iconid;
    const char* imageid;
    double duration;
    uint32_t numoptions;
} DiplomacyEventInfo;

typedef struct {
    OperationID id;
    OperationID sourceactionoperationid;
    const char* eventid;
    UniverseID agentid;
    const char* agentname;
    const char* agentimageid;
    const char* agentresultstate;
    int32_t agentexp_negotiation;
    int32_t agentexp_espionage;
    const char* faction;
    const char* otherfaction;
    const char* option;
    const char* outcome;
    double starttime;
    bool read;
    int32_t startrelation;
} DiplomacyEventOperation;

typedef struct {
    const char* id;
    const char* name;
    const char* desc;
    const char* result;
    const char* conclusion;
    const char* agentrisk;
    int32_t successchance;
    float relationchange;
    int64_t price;
    int32_t influencerequirement;
    int32_t menuposition;
    uint32_t numwarerequirements;
} DiplomacyEventOptionInfo;

typedef struct {
    float dps;
    uint32_t quadranttextid;
} DPSData;

typedef struct {
    bool excluded;
    uint32_t numexclusionreasons;
} FactionDiplomacyExclusionInfo;

typedef struct {
    const char* text;
    const char* textcondition;
    int32_t relationcondition;
    bool positive;
} RelationImplication;

typedef struct {
    const char* name;
    const char* colorid;
} RelationRangeInfo;

typedef struct {
    const char* file;
    const char* icon;
    bool ispersonal;
} UILogo;

typedef struct {
    const char* wareid;
    uint32_t amount;
} UIWareAmount;

typedef struct {
    const char* macro;
    const char* ware;
    uint32_t amount;
    uint32_t capacity;
} AmmoData;

typedef struct {
    BuildTaskID id;
    UniverseID buildingcontainer;
    UniverseID component;
    const char* macro;
    const char* factionid;
    UniverseID buildercomponent;
    int64_t price;
    bool ismissingresources;
    uint32_t queueposition;
} BuildTaskInfo;

typedef struct {
    const char* id;
    const char* name;
    bool possible;
} DroneModeInfo;

typedef struct {
    const char* id;
    const char* name;
    const char* description;
    const char* iconid;
} ShipStanceInfo;

typedef struct {
    UniverseID softtargetID;
    const char* softtargetConnectionName;
    uint32_t messageID;
} SofttargetDetails2;

typedef struct {
    UniverseID contextid;
    const char* path;
    const char* group;
} UpgradeGroup2;

typedef struct {
    UniverseID currentcomponent;
    const char* currentmacro;
    const char* slotsize;
    uint32_t count;
    uint32_t operational;
    uint32_t total;
} UpgradeGroupInfo;

typedef struct {
    const char* id;
    const char* name;
    const char* tags;
    double productiontime;
    double productionamount;
} ProductionMethodInfo3;

typedef struct {
    const char* text;
    const char* mouseovertext;
} TextEntry;

typedef struct {
    const char* macro;
    const char* ware;
    const char* productionmethodid;
} UIBlueprint;

typedef struct {
    const char* name;
    const char* typeclass;
    const char* geology;
    const char* atmosphere;
    const char* population;
    const char* settlements;
    uint32_t nummoons;
    bool hasterraforming;
} UICelestialBodyInfo2;

typedef struct {
    double time;
    int64_t price;
    int amount;
    int limit;
} UITradeOfferStatData;

typedef struct {
    const char* environment;
} UISpaceInfo;

typedef struct {
    const char* name;
    const char* typeclass;
} UISunInfo;

typedef struct {
    uint32_t numsuns;
    uint32_t numplanets;
} UISystemInfoCounts;

typedef struct {
    const char* path;
    const char* group;
} UpgradeGroup;

typedef struct {
    const char* sourcetype;
    const char* sourcelocation;
} WareSource;

typedef struct {
    const char* transport;
    const char* name;
    int value;
} WareTransportInfo;

typedef struct {
    const char* ware;
    int32_t current;
    int32_t max;
} WareYield;

typedef struct {
    uint32_t current;
    uint32_t capacity;
    uint32_t optimal;
    uint32_t available;
    uint32_t maxavailable;
    double timeuntilnextupdate;
} WorkForceInfo;

typedef struct {
    int32_t id;
    const char* name;
} GameStartGroupInfo;

typedef struct {
    const char* id;
    uint32_t textid;
    uint32_t descriptionid;
    uint32_t value;
    uint32_t relevance;
    const char* ware;
} SkillInfo;

typedef struct {
    const char* id;
    const char* text;
} BoardingBehaviour;

typedef struct {
    const char* id;
    const char* text;
} BoardingPhase;

typedef struct {
    uint32_t approach;
    uint32_t insertion;
} BoardingRiskThresholds;

typedef struct {
    UniverseID controllableid;
    FleetUnitID fleetunitid;
    int32_t groupindex;
} CommanderInfo;

typedef struct {
    const char* newroleid;
    NPCSeed seed;
    uint32_t amount;
} CrewTransferContainer;

typedef struct {
    const char* id;
    const char* name;
} ControlPostInfo;

typedef struct {
    UniverseID entity;
    UniverseID personcontrollable;
    NPCSeed personseed;
} GenericActor;

typedef struct {
    const char* id;
    const char* name;
    const char* description;
} ResponseInfo;

typedef struct {
    const char* id;
    const char* name;
    const char* description;
    uint32_t numresponses;
    const char* defaultresponse;
    bool ask;
} SignalInfo;

typedef struct {
    const char* name;
    const char* transport;
    uint32_t spaceused;
    uint32_t capacity;
} StorageInfo;

typedef struct {
    int x;
    int y;
} Coord2D;

typedef struct {
    const char* factionID;
    const char* factionName;
    const char* factionIcon;
} FactionDetails;

typedef struct {
    FleetUnitID fleetunitid;
    const char* name;
    const char* idcode;
    const char* macro;
    BuildTaskID buildtaskid;
    UniverseID replacementid;
} FleetUnitInfo;

typedef struct {
    const char* icon;
    const char* caption;
} MissionBriefingIconInfo;

typedef struct {
    const char* missionName;
    const char* missionDescription;
    int difficulty;
    int upkeepalertlevel;
    const char* threadType;
    const char* mainType;
    const char* subType;
    const char* subTypeName;
    const char* faction;
    int64_t reward;
    const char* rewardText;
    size_t numBriefingObjectives;
    int activeBriefingStep;
    const char* opposingFaction;
    const char* license;
    float timeLeft;
    double duration;
    bool abortable;
    bool hasObjective;
    UniverseID associatedComponent;
    UniverseID threadMissionID;
} MissionDetails;

typedef struct {
    const char* id;
    const char* name;
    bool isstory;
} MissionGroupDetails2;

typedef struct {
    const char* text;
    const char* actiontext;
    const char* detailtext;
    int step;
    bool failed;
    bool completedoutofsequence;
} MissionObjectiveStep3;

typedef struct {
    uint32_t id;
    bool ispin;
    bool ishome;
} MultiverseMapPickInfo;

typedef struct {
    NPCSeed seed;
    const char* roleid;
    int32_t tierid;
    const char* name;
    int32_t combinedskill;
} NPCInfo;

typedef struct {
    const char* chapter;
    const char* onlineid;
} OnlineMissionInfo;

typedef struct {
    const char* id;
    const char* name;
    const char* icon;
    const char* description;
    const char* category;
    const char* categoryname;
    bool infinite;
    uint32_t requiredSkill;
} OrderDefinition;

typedef struct {
    size_t queueidx;
    const char* state;
    const char* statename;
    const char* orderdef;
    size_t actualparams;
    bool enabled;
    bool isinfinite;
    bool issyncpointreached;
    bool istemporder;
} Order;

typedef struct {
    size_t queueidx;
    const char* state;
    const char* statename;
    const char* orderdef;
    size_t actualparams;
    bool enabled;
    bool isinfinite;
    bool issyncpointreached;
    bool istemporder;
    bool isoverride;
} Order2;

typedef struct {
    size_t queueidx;
    const char* state;
    const char* statename;
    const char* orderdef;
    size_t actualparams;
    bool enabled;
    bool isinfinite;
    bool issyncpointreached;
    bool istemporder;
    bool isoverride;
    bool ispriority;
} Order3;

typedef struct {
    uint32_t id;
    AIOrderID orderid;
    const char* orderdef;
    const char* message;
    double timestamp;
    bool wasdefaultorder;
    bool wasinloop;
} OrderFailure;

typedef struct {
    const char* id;
    const char* name;
    const char* desc;
    uint32_t amount;
    uint32_t numtiers;
    bool canhire;
} PeopleInfo;

typedef struct {
    const char* id;
    const char* name;
} ProductionMethodInfo;

typedef struct {
    const char* id;
    const char* name;
    const char* shortname;
    const char* description;
    const char* icon;
} RaceInfo;

typedef struct {
    const char* name;
    int32_t skilllevel;
    uint32_t amount;
} RoleTierData;

typedef struct {
    UniverseID context;
    const char* group;
    UniverseID component;
} ShieldGroup;

typedef struct {
    uint32_t textid;
    uint32_t descriptionid;
    uint32_t value;
    uint32_t relevance;
} Skill2;

typedef struct {
    const char* max;
    const char* current;
} SoftwareSlot;

typedef struct {
    float speed;
    float boostspeed;
    float travelspeed;
} SpeedInfo;

typedef struct {
    UniverseID controllableid;
    int group;
} SubordinateGroup;

typedef struct {
    uint32_t id;
    UniverseID owningcontrollable;
    size_t owningorderidx;
    bool reached;
} SyncPointInfo2;

typedef struct {
    const char* reason;
    NPCSeed person;
    NPCSeed partnerperson;
} UICrewExchangeResult;

typedef struct {
    const char* shape;
    const char* name;
    uint32_t requiredSkill;
    float radius;
    bool rollMembers;
    bool rollFormation;
    size_t maxShipsPerLine;
} UIFormationInfo;

typedef struct {
    const char* macro;
    uint32_t amount;
    bool optional;
} UILoadoutAmmoData;

typedef struct {
    const char* macro;
    const char* path;
    const char* group;
    uint32_t count;
    bool optional;
} UILoadoutGroupData;

typedef struct {
    const char* macro;
    const char* upgradetypename;
    size_t slot;
    bool optional;
} UILoadoutMacroData;

typedef struct {
    const char* ware;
} UILoadoutSoftwareData;

typedef struct {
    const char* macro;
    bool optional;
} UILoadoutVirtualMacroData;

typedef struct {
    const char* id;
    const char* name;
} UIModuleSet;

typedef struct {
    float x;
    float y;
    float z;
    float yaw;
    float pitch;
    float roll;
} UIPosRot;

typedef struct {
    bool primary;
    uint32_t idx;
} UIWeaponGroup;

typedef struct {
    const char* id;
    const char* icon;
    const char* factoryname;
    const char* factorydesc;
    const char* factorymapicon;
    const char* factoryhudicon;
    uint32_t tier;
} WareGroupInfo;

typedef struct {
    UniverseID reserverid;
    const char* ware;
    uint32_t amount;
    bool isbuyreservation;
    double eta;
    TradeID tradedealid;
    MissionID missionid;
    bool isvirtual;
    bool issupply;
} WareReservationInfo2;

typedef struct {
    const char* id;
    const char* name;
    bool active;
} WeaponSystemInfo;

typedef struct {
    const char* wareid;
    int32_t amount;
} YieldInfo;

typedef struct {
    const char* objectiveText;
    float timeout;
    const char* progressname;
    uint32_t curProgress;
    uint32_t maxProgress;
    size_t numTargets;
} MissionObjective2;

typedef struct {
    const char* id;
    const char* name;
    int32_t state;
    const char* requiredversion;
    const char* installedversion;
} InvalidPatchInfo;

typedef struct {
    const char* fieldtype;
    const char* groupref;
} RegionField;

typedef struct {
    const char* wareid;
    const char* yield;
} RegionResource;

typedef struct {
    const char* name;
    const char* id;
    const char* source;
    bool deleteable;
} UIConstructionPlan;

typedef struct {
    const char* factionid;
    const char* civiliansetting;
    const char* militarysetting;
} UIFightRuleSetting;

typedef struct {
    uint32_t id;
    const char* type;
    const char* name;
    bool usemacrowhitelist;
    uint32_t nummacros;
    const char** macros;
    bool usefactionwhitelist;
    uint32_t numfactions;
    const char** factions;
    const char* relation;
    bool hazardous;
} BlacklistInfo2;

typedef struct {
    MessageID id;
    double time;
    const char* category;
    const char* title;
    const char* text;
    const char* source;
    UniverseID sourcecomponent;
    const char* interaction;
    UniverseID interactioncomponent;
    const char* interactiontext;
    const char* interactionshorttext;
    const char* cutscenekey;
    const char* entityname;
    const char* factionname;
    int64_t money;
    int64_t bonus;
    bool highlighted;
    bool isread;
} MessageInfo;

typedef struct {
    uint32_t numspaces;
} PlayerAlertCounts;

typedef struct {
    size_t index;
    double interval;
    bool repeats;
    bool muted;
    uint32_t numspaces;
    UniverseID* spaceids;
    const char* objectclass;
    const char* objectpurpose;
    const char* objectidcode;
    const char* objectowner;
    const char* name;
    const char* message;
    const char* soundid;
} PlayerAlertInfo2;

typedef struct {
    const char* id;
    const char* name;
} SoundInfo;

typedef struct {
    TickerCacheID id;
    double time;
    const char* category;
    const char* title;
    const char* text;
} TickerCacheEntry;

typedef struct {
    uint32_t numfactions;
} TradeRuleCounts;

typedef struct {
    uint32_t id;
    const char* name;
    uint32_t numfactions;
    const char** factions;
    bool iswhitelist;
} TradeRuleInfo;

typedef struct {
    const char* ID;
    const char* Name;
    const char* RawName;
} UIClothingTheme;

typedef struct {
    const char* Name;
    const char* RawName;
    const char* Ware;
    uint32_t Quality;
} UIEquipmentMod;

typedef struct {
    const char* macro;
    uint32_t amount;
} UIMacroCount;

typedef struct {
    const char* id;
    const char* name;
    const char* desc;
    const char* category;
    bool enabled;
    bool enabledByDefault;
} UINotificationType2;

typedef struct {
    const char* ID;
    const char* Name;
    const char* RawName;
    const char* Icon;
} UIPaintTheme;

typedef struct {
    const char* name;
    const char* desc;
    const char* value;
    float score;
    float maxscore;
    bool hasscore;
} ScenarioStat;

typedef struct {
    float HullValue;
    float ShieldValue;
    double ShieldDelay;
    float ShieldRate;
    float GroupedShieldValue;
    double GroupedShieldDelay;
    float GroupedShieldRate;
    float BurstDPS;
    float SustainedDPS;
    float TurretBurstDPS;
    float TurretSustainedDPS;
    float GroupedTurretBurstDPS;
    float GroupedTurretSustainedDPS;
    float ForwardSpeed;
    float BoostSpeed;
    float TravelSpeed;
    float YawSpeed;
    float PitchSpeed;
    float RollSpeed;
    float HorizontalStrafeSpeed;
    float VerticalStrafeSpeed;
    float ForwardAcceleration;
    float HorizontalStrafeAcceleration;
    float VerticalStrafeAcceleration;
    float BoostAcceleration;
    float BoostRechargeRate;
    float BoostMaxDuration;
    float TravelAcceleration;
    float TravelChargeTime;
    uint32_t NumDocksShipMedium;
    uint32_t NumDocksShipSmall;
    uint32_t ShipCapacityMedium;
    uint32_t ShipCapacitySmall;
    uint32_t CrewCapacity;
    uint32_t ContainerCapacity;
    uint32_t SolidCapacity;
    uint32_t LiquidCapacity;
    uint32_t CondensateCapacity;
    uint32_t UnitCapacity;
    uint32_t MissileCapacity;
    uint32_t CountermeasureCapacity;
    uint32_t DeployableCapacity;
    float RadarRange;
} UILoadoutStatistics5;

typedef struct {
    const char* ammomacroname;
    const char* weaponmode;
} UILoadoutWeaponSetting;

typedef struct {
    BlacklistID id;
    const char* type;
} BlacklistTypeID;

typedef struct {
    const char* newroleid;
    NPCSeed seed;
    uint32_t amount;
    int64_t price;
} CrewTransferContainer2;

typedef struct {
    uint32_t numremoved;
    uint32_t numadded;
    uint32_t numtransferred;
} CrewTransferInfoCounts;

typedef struct {
    const char* state;
    float defaultvalue;
} CustomGameStartFloatPropertyState;

typedef struct {
    const char* state;
} CustomGameStartLoadoutPropertyState;

typedef struct {
    const char* state;
    const char* defaultvalue;
    const char* options;
} CustomGameStartStringPropertyState;

typedef struct {
    const char* tag;
    const char* name;
} EquipmentCompatibilityInfo;

typedef struct {
    const char* type;
    const char* ware;
    const char* macro;
    int amount;
} EquipmentWareInfo;

typedef struct {
    const char* PropertyType;
    float MinValueFloat;
    float MaxValueFloat;
    uint32_t MinValueUINT;
    uint32_t MaxValueUINT;
    uint32_t BonusMax;
    float BonusChance;
} EquipmentModInfo;

typedef struct {
    FightRuleID id;
    const char* type;
} FightRuleTypeID;

typedef struct {
    const char* name;
    const char* icon;
} LicenceInfo;

typedef struct {
    MissionID missionid;
    const char* macroname;
    uint32_t amount;
} MissionShipDeliveryInfo;

typedef struct {
    const char* id;
    const char* name;
    const char* desc;
} PeopleDefinitionInfo;

typedef struct {
    const char* Name;
    const char* RawName;
    const char* Ware;
    uint32_t Quality;
    const char* PropertyType;
    float ForwardThrustFactor;
    float StrafeAccFactor;
    float StrafeThrustFactor;
    float RotationThrustFactor;
    float BoostAccFactor;
    float BoostThrustFactor;
    float BoostDurationFactor;
    float BoostAttackTimeFactor;
    float BoostReleaseTimeFactor;
    float BoostChargeTimeFactor;
    float BoostRechargeTimeFactor;
    float TravelThrustFactor;
    float TravelStartThrustFactor;
    float TravelAttackTimeFactor;
    float TravelReleaseTimeFactor;
    float TravelChargeTimeFactor;
} UIEngineMod2;

typedef struct {
    const char* roleid;
    uint32_t count;
    bool optional;
} UILoadoutCrewData;

typedef struct {
    uint32_t numweapons;
    uint32_t numturrets;
    uint32_t numshields;
    uint32_t numengines;
    uint32_t numturretgroups;
    uint32_t numshieldgroups;
    uint32_t numammo;
    uint32_t numunits;
    uint32_t numsoftware;
    uint32_t numcrew;
} UILoadoutCounts2;

typedef struct {
    const char* id;
    const char* name;
    const char* iconid;
    bool deleteable;
} UILoadoutInfo;

typedef struct {
    const char* upgradetype;
    size_t slot;
} UILoadoutSlot;

typedef struct {
    const char* Name;
    const char* RawName;
    const char* Ware;
    uint32_t Quality;
    uint32_t Amount;
} UIPaintMod;

typedef struct {
    const char* Name;
    const char* RawName;
    const char* Ware;
    uint32_t Quality;
    const char* PropertyType;
    float CapacityFactor;
    float RechargeDelayFactor;
    float RechargeRateFactor;
} UIShieldMod;

typedef struct {
    const char* ware;
    const char* macro;
    int amount;
} UIWareInfo;

typedef struct {
    const char* Name;
    const char* RawName;
    const char* Ware;
    uint32_t Quality;
    const char* PropertyType;
    float DamageFactor;
    float CoolingFactor;
    float ReloadFactor;
    float SpeedFactor;
    float LifeTimeFactor;
    float MiningFactor;
    float StickTimeFactor;
    float ChargeTimeFactor;
    float BeamLengthFactor;
    uint32_t AddedAmount;
    float RotationSpeedFactor;
    float SurfaceElementFactor;
} UIWeaponMod;

typedef struct {
    const char* macro;
    const char* category;
    uint32_t amount;
} UnitData;

typedef struct {
    const char* filename;
    const char* name;
    const char* id;
} UIConstructionPlanInfo;

typedef struct {
    uint32_t numweapons;
    uint32_t numturrets;
    uint32_t numshields;
    uint32_t numengines;
    uint32_t numturretgroups;
    uint32_t numshieldgroups;
    uint32_t numammo;
    uint32_t numunits;
    uint32_t numsoftware;
} UILoadoutCounts;

typedef struct {
    int64_t trade;
    int64_t defence;
    int64_t build;
    int64_t repair;
    int64_t missile;
} SupplyBudget;

typedef struct {
    const char* ware;
    int total;
    int current;
    const char* supplytypes;
} SupplyResourceInfo;

typedef struct {
    const char* macro;
    int amount;
} SupplyOverride;

typedef struct {
    double time;
    int64_t money;
} UIAccountStatData;

typedef struct {
    double time;
    uint64_t amount;
} UICargoStatData;

typedef struct {
    const char* id;
    const char* group;
    const char* name;
    const char* description;
    double duration;
    double repeatcooldown;
    uint32_t timescompleted;
    int32_t successchance;
    bool resilient;
    bool showalways;
    int64_t price;
    float payoutfactor;
    const char* requiredresearchid;
    const char* pricescale;
    const char* pricescaletext;
    bool anypredecessor;
    uint32_t numpredecessors;
    uint32_t numpredecessorgroups;
    uint32_t numblockingprojects;
    uint32_t numconditions;
    uint32_t numprimaryeffects;
    uint32_t numsideeffects;
    uint32_t numblockedprojects;
    uint32_t numblockedgroups;
    uint32_t numrebates;
    uint32_t numresources;
    uint32_t numremovedprojects;
} UITerraformingProject2;

typedef struct {
    const char* type;
    const char* name;
    float value;
    bool active;
} UIWorkforceInfluence;

typedef struct {
    uint32_t numcapacityinfluences;
    uint32_t numgrowthinfluences;
} WorkforceInfluenceCounts;

typedef struct {
    uint32_t red;
    uint32_t green;
    uint32_t blue;
    uint32_t alpha;
} Color;

typedef struct {
    uint32_t numbuildsinprogress;
    uint32_t numbuildsinqueue;
    uint32_t numcurrentdeliveries;
} UITerraformingDroneInfo;

typedef struct {
    MissionID missionid;
    bool missioncompleted;
} UITerraformingMissionInfo;

typedef struct {
    const char* id;
    const char* name;
    const char* description;
    const char* inactivetext;
    const char* iconid;
    bool dynamic;
    uint64_t value;
    uint32_t state;
    bool useranges;
    uint32_t numranges;
} UITerraformingStat;

typedef struct {
    const char* name;
    bool ismoon;
} UITerraformingWorldInfo;

typedef struct {
    const char* id;
    const char* name;
    const char* description;
    const char* image;
    const char* video;
    const char* voice;
    float date;
    uint32_t group;
} TimelineInfo;

typedef struct {
    UniverseID component;
    const char* connection;
} UIComponentSlot;

typedef struct {
    uint32_t nummacros;
    uint32_t numfactions;
} BlacklistCounts;

typedef struct {
    const char* class1name;
    const char* class2name;
    uint32_t class1id;
    uint32_t class2id;
    bool isclass;
} ComponentClassPair;

typedef struct {
    float x;
    float y;
    float z;
} Coord3D;

typedef struct {
    const char* id;
    const char* name;
    const char* description;
    const char* propdatatype;
    float basevalue;
    bool poseffect;
} EquipmentModPropertyInfo;

typedef struct {
    uint32_t numfactions;
} FightRuleCounts;

typedef struct {
    int32_t year;
    uint32_t month;
    uint32_t day;
    bool isvalid;
} GameStartDateInfo;

typedef struct {
    uint32_t red;
    uint32_t green;
    uint32_t blue;
    uint32_t alpha;
    float glow;
} GlowColor;

typedef struct {
    double time;
    int64_t money;
    int64_t entryid;
} MoneyLogEntry;

typedef struct {
    double time;
    int64_t money;
    int64_t entryid;
    const char* eventtype;
    const char* eventtypename;
    UniverseID partnerid;
    const char* partnername;
    const char* partneridcode;
    int64_t tradeentryid;
    const char* tradeeventtype;
    const char* tradeeventtypename;
    UniverseID buyerid;
    UniverseID sellerid;
    const char* ware;
    uint32_t amount;
    int64_t price;
    bool complete;
} TransactionLogEntry;

typedef struct {
    const char* Name;
    const char* RawName;
    const char* Ware;
    uint32_t Quality;
    const char* PropertyType;
    float MassFactor;
    float DragFactor;
    float MaxHullFactor;
    float RadarRangeFactor;
    uint32_t AddedUnitCapacity;
    uint32_t AddedMissileCapacity;
    uint32_t AddedCountermeasureCapacity;
    uint32_t AddedDeployableCapacity;
    float RadarCloakFactor;
    float RegionDamageProtection;
    float HideCargoChance;
} UIShipMod2;

typedef struct {
    const char* stat;
    uint32_t min;
    uint32_t max;
    uint64_t minvalue;
    uint64_t maxvalue;
    bool issatisfied;
} UITerraformingProjectCondition;

typedef struct {
    const char* text;
    const char* stat;
    int32_t change;
    uint64_t value;
    uint64_t minvalue;
    uint64_t maxvalue;
    bool onfail;
    bool issideeffect;
    uint32_t chance;
    uint32_t setbackpercent;
    bool isbeneficial;
} UITerraformingProjectEffect;

typedef struct {
    const char* id;
    const char* name;
} UITerraformingProjectGroup;

typedef struct {
    const char* id;
    bool anyproject;
} UITerraformingProjectPredecessorGroup;

typedef struct {
    const char* ware;
    const char* waregroupname;
    uint32_t value;
} UITerraformingProjectRebate;

typedef struct {
    const char* name;
    const char* rawname;
    const char* icon;
    const char* rewardicon;
    int64_t remainingtime;
    uint32_t numships;
} UIVentureInfo;

typedef struct {
    const char* ware;
} CustomGameStartBlueprint;

typedef struct {
    const char* state;
    uint32_t numvalues;
    uint32_t numdefaultvalues;
} CustomGameStartBlueprintPropertyState;

typedef struct {
    const char* state;
    bool defaultvalue;
} CustomGameStartBoolPropertyState;

typedef struct {
    const char* id;
    int64_t value;
} CustomGameStartBudgetDetail;

typedef struct {
    const char* id;
    int64_t value;
    int64_t limit;
    uint32_t numdetails;
} CustomGameStartBudgetInfo;

typedef struct {
    const char* id;
    const char* name;
    const char* description;
    bool isresearch;
} CustomGameStartBudgetGroupInfo;

typedef struct {
    uint32_t nummacros;
    uint32_t numblueprints;
    uint32_t numconstructionplans;
    bool hasincompatibleloadout;
} CustomGameStartContentCounts;

typedef struct {
    const char* library;
    const char* item;
} CustomGameStartEncyclopediaEntry;

typedef struct {
    const char* state;
} CustomGameStartEncyclopediaPropertyState;

typedef struct {
    const char* id;
    const char* name;
    const char* filename;
} CustomGameStartInfo;

typedef struct {
    const char* ware;
    int32_t amount;
} CustomGameStartInventory;

typedef struct {
    const char* state;
    uint32_t numvalues;
    uint32_t numdefaultvalues;
} CustomGameStartInventoryPropertyState;

typedef struct {
    const char* type;
    const char* item;
    const char* classid;
    int64_t budgetvalue;
    bool unlocked;
    bool hidden;
} CustomGameStartKnownEntry2;

typedef struct {
    const char* state;
    uint32_t numvalues;
    uint32_t numdefaultvalues;
} CustomGameStartKnownPropertyState;

typedef struct {
    const char* state;
    int64_t defaultvalue;
    int64_t minvalue;
    int64_t maxvalue;
} CustomGameStartMoneyPropertyState;

typedef struct {
    uint32_t numcargo;
} CustomGameStartPlayerPropertyCounts;

typedef struct {
    const char* state;
    uint32_t numvalues;
} CustomGameStartPlayerPropertyPropertyState;

typedef struct {
    const char* factionid;
    const char* otherfactionid;
    int32_t relation;
} CustomGameStartRelationInfo;

typedef struct {
    const char* state;
} CustomGameStartRelationsPropertyState;

typedef struct {
    const char* state;
} CustomGameStartResearchPropertyState;

typedef struct {
    const char* id;
    const char* name;
    const char* description;
    const char* groupid;
    const char* wareid;
    int32_t index;
    int64_t budgetvalue;
    uint32_t numdependencylists;
} CustomGameStartStoryInfo;

typedef struct {
    const char* state;
    uint32_t numvalues;
    uint32_t numdefaultvalues;
} CustomGameStartStoryState;

typedef struct {
    uint32_t mintime;
    uint32_t maxtime;
    float factor;
} AutosaveIntervalInfo;

typedef struct {
    float min;
    float max;
} FloatRange;

typedef struct {
    int32_t source;
    int32_t code;
    int32_t signum;
    bool istoggle;
} InputData;

typedef struct {
    const char* type;
    uint32_t id;
    const char* idname;
    const char* textoption;
    const char* voiceoption;
} InputFeedbackConfig;

typedef struct {
    const char* key;
    const char* value;
} NewGameParameter;

typedef struct {
    int x;
    int y;
} ResolutionInfo;

typedef struct {
    const char* cutsceneid;
    uint32_t extensionidx;
    bool isdefault;
} StartmenuBackgroundInfo;

typedef struct {
    const char* filename;
    const char* name;
} UIColorProfileInfo;

typedef struct {
    const char* filename;
    const char* name;
    const char* description;
    const char* version;
    uint32_t rawversion;
    const char* time;
    int64_t rawtime;
    double playtime;
    const char* playername;
    const char* location;
    int64_t money;
    bool error;
    bool invalidgameid;
    bool invalidversion;
    uint32_t numinvalidpatches;
} UISaveInfo;

typedef struct {
    UniverseID attacker;
    double time;
    const char* method;
} LastAttackerInfo;

typedef struct {
    UniverseID target;
    uint32_t numwares;
    MissionID missionid;
} MissionWareDeliveryCounts;

typedef struct {
    float x;
    float y;
    float width;
    float height;
} MonitorExtents;

typedef struct {
    float x;
    float y;
} Position2D;

typedef struct {
    float x;
    float y;
    float z;
    float yaw;
    float pitch;
    float roll;
} PosRot;

typedef struct {
    uint32_t id;
    const char* text;
    const char* type;
    bool ispossible;
    bool istobedisplayed;
} UIAction;

typedef struct {
    const char* name;
    float hull;
    float shield;
    int speed;
    bool hasShield;
} ComponentDetails;

typedef struct {
    const char* type;
    const char* name;
    const char* desc;
    const char* hackeddesc;
    int64_t price;
    int32_t numrequiredwares;
    bool ishack;
    bool hacked;
    double hackduration;
    double hackexpiretime;
} ControlPanelInfo;

typedef struct {
    uint64_t poiID;
    UniverseID componentID;
    const char* messageType;
    const char* connectionName;
    bool isAssociative;
    bool isMissionTarget;
    bool isPriorityMissionTarget;
    bool showIndicator;
    bool hasAdditionalOffset;
} MessageDetails3;

typedef struct {
    const char* missionName;
    const char* missionDescription;
    const char* missionIcon;
    int difficulty;
    int upkeepalertlevel;
    const char* threadType;
    const char* mainType;
    const char* subType;
    const char* subTypeName;
    const char* faction;
    int64_t reward;
    const char* rewardText;
    size_t numBriefingObjectives;
    int activeBriefingStep;
    const char* opposingFaction;
    const char* license;
    float timeLeft;
    double duration;
    bool abortable;
    bool hasObjective;
    UniverseID associatedComponent;
    UniverseID threadMissionID;
} MissionDetails2;

typedef struct {
    const char* type;
    const char* control;
    uint32_t controlid;
    uint32_t contextid;
    bool active;
} CompassMenuEntry;

typedef struct {
    float angle;
    bool inside;
    bool valid;
} ArrowDetails;

typedef struct {
    bool armed;
    bool blocked;
    const char* modeIcon;
    bool pending;
    bool possible;
    uint32_t total;
    uint32_t undocked;
} DroneState;

typedef struct {
    int relationStatus;
    int relationValue;
    int relationLEDValue;
    bool isBoostedValue;
} RelationDetails;

typedef struct {
    const char* icon;
    const char* mode;
    uint32_t damageState;
    bool active;
    bool usesAmmo;
    uint32_t ammo;
} TurretDetails;

typedef struct {
    const char* icon;
    uint32_t damageState;
    bool active;
    bool usesAmmo;
    uint32_t ammo;
    int32_t currentclip;
    int32_t maxclip;
    uint32_t mode;
    float reloadPercent;
    bool isAutoReloading;
    double clipReloadTime;
    uint32_t heatState;
    float heatPercent;
    float nextShotHeatPercent;
    float lockPercent;
    uint64_t counterMeasureTarget;
} WeaponDetails4;

typedef struct {
    uint32_t width;
    uint32_t height;
    uint32_t xHotspot;
    uint32_t yHotspot;
} CursorInfo;

typedef struct {
    const char* HintText;
    uint32_t HintID;
} LoadingHint;

typedef struct {
    bool active;
    bool reverse;
    uint32_t numtexts;
    uint32_t numinfotexts;
} ScenarioLoadingData;

typedef struct {
    size_t numImportantMails;
    size_t numNormalMails;
} MailCount;

typedef struct {
    uint64_t poiID;
    UniverseID componentID;
    const char* messageType;
    const char* connectionName;
    bool isAssociative;
    bool isMissionObjective;
    bool showIndicator;
} MessageDetails;

typedef struct {
    bool active;
    bool callbackMode;
    uint32_t barLine;
    float barPercent;
    const char* line1Left;
    const char* line1Right;
    const char* line2Left;
    const char* line2Right;
    const char* line3Left;
    const char* line3Right;
    const char* line4Left;
    const char* line4Right;
    const char* line5Left;
    const char* line5Right;
    const char* missionBarText;
} MissionInfo3;

typedef struct {
    uint32_t numrows;
    float fadein;
    float fadeout;
    float relwidth;
    float reloffsetx;
    float reloffsety;
    bool fromright;
    bool fromtop;
} OverlayInfo;

typedef struct {
    uint32_t numcols;
    float reloffsetx;
    float reloffsety;
    bool fromright;
    bool fromtop;
} OverlayRowInfo;

typedef struct {
    const char* id;
    const char* extensionid;
    const char* icon;
    const char* text;
    const char* link;
    bool islinkappid;
} NewsInfo2;

typedef struct {
    uint32_t messageID;
    bool obstructed;
} CrosshairMessage;

typedef struct {
    const char* POIName;
    const char* POIType;
} POIDetails;

typedef struct {
    int relationStatus;
    int relationValue;
    int relationLEDValue;
    bool isBoostedValue;
    const char* owningFactionID;
} RelationDetails2;

typedef struct {
    float yaw;
    float pitch;
    float roll;
} Rotation;

typedef struct {
    bool factionNPC;
    bool missionActor;
    bool shadyGuy;
} SpecialNPCSet;

typedef struct {
    float speed;
    float screenx;
    float screeny;
    bool onscreen;
} VelocityInfo;

typedef struct {
    const char* icon;
    uint32_t damageState;
    bool active;
    bool usesAmmo;
    uint32_t ammo;
    uint32_t mode;
    float reloadPercent;
    uint32_t heatState;
    float heatPercent;
    float lockPercent;
    uint64_t counterMeasureTarget;
} WeaponDetails;

typedef struct {
    const char* active;
    const char* inactive;
    const char* select;
} IconSet;

typedef struct {
    float x;
    float y;
    bool onScreen;
} ScreenPos;

typedef struct {
    const char* name;
    uint32_t size;
} Font;

typedef struct {
    const char* startoption;
    bool active;
    bool rightsidearrow;
    bool mouseoverinteraction;
    uint32_t optionwidth;
    uint32_t optionheight;
    uint32_t numoptions;
} DropDownInfo;

typedef struct {
    double x;
    double y;
    bool inactive;
} GraphDataPoint2;

typedef struct {
    size_t DataRecordIdx;
    size_t DataIdx;
    const char* IconID;
    const char* MouseOverText;
} GraphIcon;

typedef struct {
    size_t InitialSelectedRecordIdx;
    size_t InitialSelectedDataIdx;
} GraphInitialSelectionInfo;

typedef struct {
    const char* iconid;
    uint32_t x;
    uint32_t y;
    bool display;
} HotkeyInfo;

typedef struct {
    int32_t top;
    int32_t bottom;
    int32_t left;
    int32_t right;
} PaddingInfo;

typedef struct {
    double min;
    double minSelect;
    double max;
    double maxSelect;
    double start;
    double step;
    double infinitevalue;
    uint32_t maxfactor;
    bool exceedmax;
    bool hidemaxvalue;
    bool righttoleft;
    bool fromcenter;
    bool readonly;
    bool useinfinitevalue;
    bool usetimeformat;
} SliderCellDetails;

typedef struct {
    uint32_t toprow;
    uint32_t selectedrow;
    uint32_t selectedcol;
    uint32_t shiftstart;
    uint32_t shiftend;
} TableSelectionInfo;

typedef struct {
    const char* id;
    const char* text;
    int32_t x;
    int32_t y;
    uint32_t width;
    uint32_t height;
    bool highlightonly;
    bool usebackgroundspan;
} UIOverlayInfo3;

typedef struct {
    UniverseID target;
    UIWareAmount* wares;
    uint32_t numwares;
} MissionWareDeliveryInfo;

typedef struct {
    const char* wareid;
    bool isSellOffer;
    UITradeOfferStatData* data;
    uint32_t numdata;
} UITradeOfferStat;

typedef struct {
    UISpaceInfo space;
    uint32_t numsuns;
    UISunInfo* suns;
    uint32_t numplanets;
    UICelestialBodyInfo2* planets;
} UISystemInfo2;

typedef struct {
    MissionID missionid;
    uint32_t amount;
    uint32_t numskills;
    SkillInfo* skills;
} MissionNPCInfo;

typedef struct {
    const char* race;
    const char* tags;
    uint32_t numskills;
    SkillInfo* skills;
} CustomGameStartPersonEntry;

typedef struct {
    UILoadoutMacroData* weapons;
    uint32_t numweapons;
    UILoadoutMacroData* turrets;
    uint32_t numturrets;
    UILoadoutMacroData* shields;
    uint32_t numshields;
    UILoadoutMacroData* engines;
    uint32_t numengines;
    UILoadoutGroupData* turretgroups;
    uint32_t numturretgroups;
    UILoadoutGroupData* shieldgroups;
    uint32_t numshieldgroups;
    UILoadoutAmmoData* ammo;
    uint32_t numammo;
    UILoadoutAmmoData* units;
    uint32_t numunits;
    UILoadoutSoftwareData* software;
    uint32_t numsoftware;
    UILoadoutVirtualMacroData thruster;
} UILoadout;

typedef struct {
    UIPosRot offset;
    float cameradistance;
} HoloMapState;

typedef struct {
    size_t idx;
    const char* macroid;
    UniverseID componentid;
    UIPosRot offset;
    const char* connectionid;
    size_t predecessoridx;
    const char* predecessorconnectionid;
    bool isfixed;
} UIConstructionPlanEntry;

typedef struct {
    size_t idx;
    const char* macroid;
    UniverseID componentid;
    UIPosRot offset;
    const char* connectionid;
    size_t predecessoridx;
    const char* predecessorconnectionid;
    bool isfixed;
    uint32_t bookmarknum;
} UIConstructionPlanEntry2;

typedef struct {
    const char* state;
    UIPosRot defaultvalue;
} CustomGameStartPosRotPropertyState;

typedef struct {
    FightRuleID id;
    const char* name;
    uint32_t numfactions;
    UIFightRuleSetting* factions;
} FightRuleInfo;

typedef struct {
    UIMacroCount* macros;
    uint32_t nummacros;
    const char** blueprints;
    uint32_t numblueprints;
    const char** constructionplanids;
    uint32_t numconstructionplans;
} CustomGameStartContentData2;

typedef struct {
    const char* macro;
    const char* path;
    const char* group;
    uint32_t count;
    bool optional;
    UILoadoutWeaponSetting weaponsetting;
} UILoadoutGroupData2;

typedef struct {
    const char* macro;
    const char* upgradetypename;
    size_t slot;
    bool optional;
    UILoadoutWeaponSetting weaponsetting;
} UILoadoutMacroData2;

typedef struct {
    CrewTransferContainer2* removed;
    uint32_t numremoved;
    CrewTransferContainer2* added;
    uint32_t numadded;
    CrewTransferContainer2* transferred;
    uint32_t numtransferred;
} CrewTransferInfo2;

typedef struct {
    BlacklistTypeID* blacklists;
    uint32_t numblacklists;
    FightRuleTypeID* fightrules;
    uint32_t numfightrules;
    const char* paintmodwareid;
} AddBuildTask6Container;

typedef struct {
    const char* type;
    const char* id;
    const char* sector;
    UIPosRot offset;
    const char* dockedatid;
    const char* commanderid;
    const char* macroname;
    const char* name;
    const char* constructionplanid;
    const char* paintmod;
    const char* peopledefid;
    float peoplefillpercentage;
    uint32_t numcargo;
    UIWareInfo* cargo;
    uint32_t count;
} CustomGameStartPlayerProperty3;

typedef struct {
    const char* wareid;
    UICargoStatData* data;
    uint32_t numdata;
} UICargoStat;

typedef struct {
    uint32_t numcapacityinfluences;
    UIWorkforceInfluence* capacityinfluences;
    uint32_t numgrowthinfluences;
    UIWorkforceInfluence* growthinfluences;
    float basegrowth;
    uint32_t capacity;
    uint32_t current;
    uint32_t sustainable;
    uint32_t target;
    int32_t change;
} WorkforceInfluenceInfo;

typedef struct {
    const char* icon;
    Color color;
    uint32_t volume_s;
    uint32_t volume_m;
    uint32_t volume_l;
} UIMapTradeVolumeParameter;

typedef struct {
    uint64_t endvalue;
    uint32_t state;
    Color color;
    const char* description;
} UITerraformingStatRange;

typedef struct {
    const char* id;
    const char* referenceid;
    Color color;
    float glowfactor;
    bool ispersonal;
    bool isdeletable;
} EditableColorMapEntry;

typedef struct {
    const char* id;
    Color color;
    float glowfactor;
} ColorMapEntry;

typedef struct {
    uint32_t colidx;
    uint32_t colspan;
    const char* text;
    Color textcolor;
    float textglowfactor;
    uint32_t fontsize;
    bool wordwrap;
    bool typewritereffect;
    const char* icon;
    Color iconcolor;
    float iconglowfactor;
    uint32_t iconwidth;
    uint32_t iconheight;
    int32_t offsetx;
    int32_t offsety;
    uint32_t cellheight;
    const char* halign;
    const char* valign;
    Color backgroundcolor;
    float backgroundglowfactor;
    double flashduration;
    double flashinterval;
    Color flashcolor;
    float flashglowfactor;
} OverlayCellInfo3;

typedef struct {
    const char* factionID;
    const char* factionName;
    const char* factionIcon;
    Color factionColor;
    float glowfactor;
} FactionDetails2;

typedef struct {
    Color color;
    uint32_t width;
    uint32_t height;
    uint32_t x;
    uint32_t y;
    float glowfactor;
} DropDownIconInfo;

typedef struct {
    uint32_t MarkerType;
    uint32_t MarkerSize;
    Color MarkerColor;
    float MarkerGlowFactor;
    uint32_t LineType;
    uint32_t LineWidth;
    Color LineColor;
    float LineGlowFactor;
    size_t NumData;
    bool Highlighted;
    const char* MouseOverText;
} GraphDataRecord;

typedef struct {
    const char* iconid;
    Color color;
    uint32_t width;
    uint32_t height;
    int32_t rotationrate;
    uint32_t rotstart;
    float rotduration;
    float rotinterval;
    float initscale;
    float scaleduration;
    float glowfactor;
} UIFrameTextureInfo;

typedef struct {
    const char* classid;
    Coord3D size;
    bool inverted;
} RegionBoundary;

typedef struct {
    const char* id;
    uint32_t numfields;
    uint32_t numboundaries;
    uint32_t numresources;
    Coord3D size;
    float density;
    float speed;
    float rotationSpeed;
    float defaultNoiseScale;
    float defaultMinNoiseValue;
    float defaultMaxNoiseValue;
} RegionDefinition;

typedef struct {
    Coord3D offset;
    Coord3D tangent;
    float weight;
    float inlength;
    float outlength;
} SplineData;

typedef struct
    {
    GlowColor color;
    const char* iconid;
    const char* swapiconid;
    const char* anchorid;
    uint32_t width;
    uint32_t height;
    int32_t x;
    int32_t y;
} IconInfo;

typedef struct {
    GlowColor color;
    uint32_t width;
    uint32_t height;
    int32_t x;
    int32_t y;
} DropDownBackgroundInfo;

typedef struct {
    GlowColor color;
    uint32_t groupid;
    uint32_t level;
    uint32_t firstrow;
    uint32_t numrows;
} RowGroupInfo;

typedef struct {
    int32_t id;
    const char* name;
    const char* warning;
    const char* font;
    bool voice;
} LanguageInfo;

typedef struct {
    const char* id;
    const char* iconid;
    const char* text1;
    const char* text2;
    const char* text3;
    const char* text4;
    const char* mouseovertext;
    const char* font;
    Color overrideColor;
    bool displayRemoveOption;
    bool active;
    bool hasOverrideColor;
} DropDownOption3;

typedef struct {
    Color color;
    Font font;
    const char* alignment;
    uint32_t x;
    uint32_t y;
    const char* textOverride;
    float glowfactor;
} DropDownTextInfo;

typedef struct {
    const char* text;
    Font font;
    Color color;
} GraphTextInfo;

typedef struct {
    const char* text;
    int32_t x;
    int32_t y;
    const char* alignment;
    Color color;
    Font font;
    float glowfactor;
} TextInfo;

typedef struct {
    PaddingInfo padding;
    uint32_t group;
    bool selectable;
    bool interactive;
    bool borderbelow;
} TableRowInfo;

typedef struct {
    UniverseID shipid;
    const char* macroname;
    UILoadout loadout;
    uint32_t amount;
} UIBuildOrderList;

typedef struct {
    UILoadoutMacroData2* weapons;
    uint32_t numweapons;
    UILoadoutMacroData2* turrets;
    uint32_t numturrets;
    UILoadoutMacroData2* shields;
    uint32_t numshields;
    UILoadoutMacroData2* engines;
    uint32_t numengines;
    UILoadoutGroupData2* turretgroups;
    uint32_t numturretgroups;
    UILoadoutGroupData2* shieldgroups;
    uint32_t numshieldgroups;
    UILoadoutAmmoData* ammo;
    uint32_t numammo;
    UILoadoutAmmoData* units;
    uint32_t numunits;
    UILoadoutSoftwareData* software;
    uint32_t numsoftware;
    UILoadoutVirtualMacroData thruster;
    uint32_t numcrew;
    UILoadoutCrewData* crew;
    bool hascrewexperience;
} UILoadout2;

typedef struct {
    IconInfo icon;
    IconInfo icon2;
    GlowColor color;
    const char* id;
    uint32_t linewidth;
    int32_t offsetTop;
    int32_t offsetBottom;
    int32_t offsetLeft;
    int32_t offsetRight;
    bool active;
} UIFrameBorderInfo;

typedef struct {
    GraphTextInfo label;
    double startvalue;
    double endvalue;
    double granularity;
    double offset;
    bool grid;
    Color color;
    Color gridcolor;
    float glowfactor;
    const char* unittext;
} GraphAxisInfo2;

#ifdef __cplusplus
}
#endif
