// ==========================================================================
// x4_game_func_table.h - X4 Game Function Pointer Table
// ==========================================================================
// Auto-generated from X4 v9.00-607977 FFI declarations.
//
// The X4GameFunctions struct provides compile-time type-safe access to
// resolved game function pointers. Populated at runtime via GetProcAddress.
//
// Usage in extensions (cache api->game during init):
//   if (game->GetPlayerID)
//       UniverseID player = game->GetPlayerID();
//
// DO NOT EDIT - regenerate with: .\scripts\generate_headers.ps1
// ==========================================================================
#pragma once

#include "x4_game_types.h"

#ifdef __cplusplus
extern "C" {
#endif

// --------------------------------------------------------------------------
// Function Pointer Table (2059 entries)
// --------------------------------------------------------------------------

#define X4_FUNC(ret, name, params) ret (*name) params;

typedef struct X4GameFunctions {
#include "x4_game_func_list.inc"
#include "x4_internal_func_list.inc"
} X4GameFunctions;

#undef X4_FUNC

// Named lookup for functions not in the table (untyped exports, etc.)
// Returns NULL if the function is not found in the game executable.
typedef void* (*X4GetGameFunctionFn)(const char* name);

// --------------------------------------------------------------------------
// Untyped Exports (312)
// --------------------------------------------------------------------------
// These functions exist in X4.exe's export table but have no known
// C signature from FFI data. Resolve with X4GetGameFunctionFn and cast.
//
// --- With known FFI sibling (similar signature likely) ---
//   AddBuildTask                                              (see AddBuildTask6)
//   AddBuildTask2                                             (see AddBuildTask6)
//   AddBuildTask3                                             (see AddBuildTask6)
//   AddBuildTask4                                             (see AddBuildTask6)
//   AddBuildTask5                                             (see AddBuildTask6)
//   AddPlayerAlert                                            (see AddPlayerAlert2)
//   CreateBlacklist                                           (see CreateBlacklist2)
//   CreateOrder2                                              (see CreateOrder)
//   FadeScreen                                                (see FadeScreen2)
//   GenerateFactionRelationTextFromRelation                   (see GenerateFactionRelationTextFromRelation2)
//   GenerateModuleLoadout2                                    (see GenerateModuleLoadout)
//   GenerateModuleLoadoutCounts2                              (see GenerateModuleLoadoutCounts)
//   GenerateShipKnownLoadout                                  (see GenerateShipKnownLoadout2)
//   GenerateShipKnownLoadout3                                 (see GenerateShipKnownLoadout2)
//   GenerateShipKnownLoadoutCounts                            (see GenerateShipKnownLoadoutCounts2)
//   GenerateShipKnownLoadoutCounts3                           (see GenerateShipKnownLoadoutCounts2)
//   GenerateShipLoadout                                       (see GenerateShipLoadout2)
//   GenerateShipLoadout3                                      (see GenerateShipLoadout2)
//   GenerateShipLoadoutCounts                                 (see GenerateShipLoadoutCounts2)
//   GenerateShipLoadoutCounts3                                (see GenerateShipLoadoutCounts2)
//   GetAllEquipment                                           (see GetAllEquipment2)
//   GetAllShipMacros                                          (see GetAllShipMacros2)
//   GetBlacklistInfo                                          (see GetBlacklistInfo2)
//   GetBuildMapStationLocation                                (see GetBuildMapStationLocation2)
//   GetBuildTaskCrewTransferInfo                              (see GetBuildTaskCrewTransferInfo2)
//   GetChatAuthorColor                                        (see GetChatAuthorColor2)
//   GetConstructionMapItemLoadout                             (see GetConstructionMapItemLoadout2)
//   GetConstructionMapItemLoadoutCounts                       (see GetConstructionMapItemLoadoutCounts2)
//   GetContainerWareReservations                              (see GetContainerWareReservations2)
//   GetCurrentLoadoutStatistics                               (see GetCurrentLoadoutStatistics5)
//   GetCurrentLoadoutStatistics2                              (see GetCurrentLoadoutStatistics5)
//   GetCurrentLoadoutStatistics3                              (see GetCurrentLoadoutStatistics5)
//   GetCurrentLoadoutStatistics4                              (see GetCurrentLoadoutStatistics5)
//   GetCustomGameStartContent                                 (see GetCustomGameStartContent2)
//   GetCustomGameStartContentCounts                           (see GetCustomGameStartContentCounts2)
//   GetCustomGameStartKnownDefaultProperty                    (see GetCustomGameStartKnownDefaultProperty2)
//   GetCustomGameStartKnownProperty                           (see GetCustomGameStartKnownProperty2)
//   GetCustomGameStartKnownPropertyBudgetValue                (see GetCustomGameStartKnownPropertyBudgetValue2)
//   GetCustomGameStartLoadoutProperty                         (see GetCustomGameStartLoadoutProperty2)
//   GetCustomGameStartLoadoutPropertyCounts                   (see GetCustomGameStartLoadoutPropertyCounts2)
//   GetCustomGameStartPlayerPropertyProperty                  (see GetCustomGameStartPlayerPropertyProperty3)
//   GetCustomGameStartPlayerPropertyProperty2                 (see GetCustomGameStartPlayerPropertyProperty3)
//   GetCustomGameStartShipPeopleValue                         (see GetCustomGameStartShipPeopleValue2)
//   GetDefaultResponseToSignalForFaction                      (see GetDefaultResponseToSignalForFaction2)
//   GetDropDownOptionOverlayInfo                              (see GetDropDownOptionOverlayInfo2)
//   GetDropDownOptions                                        (see GetDropDownOptions3)
//   GetDropDownOptions2                                       (see GetDropDownOptions3)
//   GetFactionRelationStatus                                  (see GetFactionRelationStatus2)
//   GetGraphData                                              (see GetGraphData2)
//   GetGraphXAxis                                             (see GetGraphXAxis2)
//   GetGraphYAxis                                             (see GetGraphYAxis2)
//   GetHelpOverlayInfo                                        (see GetHelpOverlayInfo3)
//   GetHelpOverlayInfo2                                       (see GetHelpOverlayInfo3)
//   GetInstalledEngineMod                                     (see GetInstalledEngineMod2)
//   GetInstalledShipMod                                       (see GetInstalledShipMod2)
//   GetLoadoutStatistics                                      (see GetLoadoutStatistics5)
//   GetLoadoutStatistics2                                     (see GetLoadoutStatistics5)
//   GetLoadoutStatistics3                                     (see GetLoadoutStatistics5)
//   GetLoadoutStatistics4                                     (see GetLoadoutStatistics5)
//   GetMapPositionOnEcliptic                                  (see GetMapPositionOnEcliptic2)
//   GetMaxLoadoutStatistics                                   (see GetMaxLoadoutStatistics5)
//   GetMaxLoadoutStatistics2                                  (see GetMaxLoadoutStatistics5)
//   GetMaxLoadoutStatistics3                                  (see GetMaxLoadoutStatistics5)
//   GetMaxLoadoutStatistics4                                  (see GetMaxLoadoutStatistics5)
//   GetMessageDetails                                         (see GetMessageDetails3)
//   GetMessageDetails2                                        (see GetMessageDetails3)
//   GetMissingConstructionPlanBlueprints                      (see GetMissingConstructionPlanBlueprints3)
//   GetMissingConstructionPlanBlueprints2                     (see GetMissingConstructionPlanBlueprints3)
//   GetMissionGroupDetails                                    (see GetMissionGroupDetails2)
//   GetMissionIDObjective                                     (see GetMissionIDObjective2)
//   GetMissionInfo                                            (see GetMissionInfo3)
//   GetMissionInfo2                                           (see GetMissionInfo3)
//   GetMissionObjectiveStep                                   (see GetMissionObjectiveStep3)
//   GetMissionObjectiveStep2                                  (see GetMissionObjectiveStep3)
//   GetMoonInfo                                               (see GetMoonInfo2)
//   GetNextNewsItem                                           (see GetNextNewsItem2)
//   GetNotificationTypes                                      (see GetNotificationTypes2)
//   GetNumAllEquipment                                        (see GetNumAllEquipment2)
//   GetNumAllShipMacros                                       (see GetNumAllShipMacros2)
//   GetNumContainerWareReservations                           (see GetNumContainerWareReservations2)
//   GetNumMissingBuildResources                               (see GetNumMissingBuildResources2)
//   GetNumMissingLoadoutResources                             (see GetNumMissingLoadoutResources2)
//   GetNumPlayerAlertSounds                                   (see GetNumPlayerAlertSounds2)
//   GetNumRemovedConstructionPlanModules                      (see GetNumRemovedConstructionPlanModules2)
//   GetNumRemovedStationModules                               (see GetNumRemovedStationModules2)
//   GetNumRepairResources                                     (see GetNumRepairResources2)
//   GetNumValidTransporterTargets                             (see GetNumValidTransporterTargets2)
//   GetOverlayCols                                            (see GetOverlayCols3)
//   GetOverlayCols2                                           (see GetOverlayCols3)
//   GetPeople                                                 (see GetPeople2)
//   GetPersonSkills                                           (see GetPersonSkills3)
//   GetPersonSkills2                                          (see GetPersonSkills3)
//   GetPickedBuildMapEntry                                    (see GetPickedBuildMapEntry2)
//   GetPlayerAlerts                                           (see GetPlayerAlerts2)
//   GetPlayerAlertSounds                                      (see GetPlayerAlertSounds2)
//   GetPresentModeOption                                      (see GetPresentModeOption2)
//   GetProductionMethodInfo                                   (see GetProductionMethodInfo2)
//   GetRelationStatus                                         (see GetRelationStatus3)
//   GetRelationStatus2                                        (see GetRelationStatus3)
//   GetRemovedConstructionPlanModules                         (see GetRemovedConstructionPlanModules2)
//   GetRemovedStationModules                                  (see GetRemovedStationModules2)
//   GetRepairResources                                        (see GetRepairResources2)
//   GetSofttarget                                             (see GetSofttarget2)
//   GetSSROption                                              (see GetSSROption2)
//   GetStandardButtonHelpOverlayInfo                          (see GetStandardButtonHelpOverlayInfo3)
//   GetStandardButtonHelpOverlayInfo2                         (see GetStandardButtonHelpOverlayInfo3)
//   GetSyncPointInfo                                          (see GetSyncPointInfo2)
//   GetTableHighlightMode                                     (see GetTableHighlightMode2)
//   GetTerraformingProjects                                   (see GetTerraformingProjects2)
//   GetTurretGroupMode                                        (see GetTurretGroupMode2)
//   GetUISystemInfo                                           (see GetUISystemInfo2)
//   GetValidTransporterTargets                                (see GetValidTransporterTargets2)
//   GetWeaponDetails                                          (see GetWeaponDetails3)
//   GetWeaponDetails2                                         (see GetWeaponDetails3)
//   HasDefaultLoadout                                         (see HasDefaultLoadout2)
//   IsDestructible                                            (see IsDestructible2)
//   PerformCrewExchange                                       (see PerformCrewExchange2)
//   PrepareBuildSequenceResources                             (see PrepareBuildSequenceResources2)
//   RemoveBuildPlot                                           (see RemoveBuildPlot2)
//   RemoveCommander                                           (see RemoveCommander2)
//   RemoveItemFromConstructionMap                             (see RemoveItemFromConstructionMap2)
//   RequestShipFromInternalStorage                            (see RequestShipFromInternalStorage2)
//   SetCheckBoxChecked                                        (see SetCheckBoxChecked2)
//   SetCustomGameStartKnownProperty                           (see SetCustomGameStartKnownProperty2)
//   SetCustomGameStartPlayerPropertyMacroAndConstructionPlan  (see SetCustomGameStartPlayerPropertyMacroAndConstructionPlan2)
//   SetCustomGameStartPlayerPropertyMacroAndLoadout           (see SetCustomGameStartPlayerPropertyMacroAndLoadout2)
//   SetCustomGameStartPlayerPropertyPeopleFillPercentage      (see SetCustomGameStartPlayerPropertyPeopleFillPercentage2)
//   SetCustomGameStartShipAndLoadoutProperty                  (see SetCustomGameStartShipAndLoadoutProperty2)
//   SetDefaultResponseToSignalForFaction                      (see SetDefaultResponseToSignalForFaction2)
//   SetDropDownOptionTexts                                    (see SetDropDownOptionTexts3)
//   SetDropDownOptionTexts2                                   (see SetDropDownOptionTexts3)
//   SetMultipleGfxModes                                       (see SetMultipleGfxModes3)
//   SetMultipleGfxModes2                                      (see SetMultipleGfxModes3)
//   SetPresentModeOption                                      (see SetPresentModeOption2)
//   SetRadarRenderTarget                                      (see SetRadarRenderTarget2)
//   SetRadarRenderTargetOnTarget                              (see SetRadarRenderTargetOnTarget2)
//   SetSSROption                                              (see SetSSROption2)
//   SetTurretGroupMode                                        (see SetTurretGroupMode2)
//   ShowObjectConfigurationMap                                (see ShowObjectConfigurationMap2)
//   ShowUniverseMacroMap                                      (see ShowUniverseMacroMap2)
//   ShowUniverseMap                                           (see ShowUniverseMap2)
//   ShuffleMapConstructionPlan                                (see ShuffleMapConstructionPlan2)
//   StartVoiceSequence                                        (see StartVoiceSequence2)
//   UpdateBlacklist                                           (see UpdateBlacklist2)
//   UpdatePlayerAlert                                         (see UpdatePlayerAlert2)
//
// --- No known sibling ---
//   ActivateMouseEmulation
//   ActivatePlayerControls
//   AddCrewExchangeOrder
//   AmdPowerXpressRequestHighPerformance
//   CanCancelCurrentOrder
//   CanWarpShipToVentureDock
//   CheckGroupedWeaponModCompatibility
//   CheckModuleDeconstructionPossible
//   CheckShieldModCompatibility
//   ClearPersonRole
//   CreatePlugin
//   DeactivatePlayerControls
//   DoesMapConstructionSequenceRequireBuilder
//   GetActiveMissionComponentID
//   GetAudioLogs
//   GetAvailableClothingMods
//   GetCargoSpaceUsedAfterTradeOrders
//   GetConstructionMapVenturePlatformDocks
//   GetContainerAllowedBuildFactions
//   GetControlPanelHackExpireTime
//   GetControlPanelNumRequiredWares
//   GetCurrentGPUNiceName
//   GetCustomGameStartAccessProperty
//   GetCustomGameStartAccessPropertyCounts
//   GetCustomGameStartAccessPropertyState
//   GetCustomGameStartIntegerProperty
//   GetCustomGameStartPoliceProperty
//   GetCustomGameStartPolicePropertyCounts
//   GetCustomGameStartPolicePropertyState
//   GetDropDownOptionHeight
//   GetDropDownOptionWidth
//   GetDropDownText2Details
//   GetDropDownTextDetails
//   GetFactionBuildMethod
//   GetFrameBackgroundColor
//   GetFrameOverlayColor
//   GetInstalledClothingMod
//   GetMainMissiontargetPOSID
//   GetMaxConstructionMapUndoSteps
//   GetMouseHUDModeOption
//   GetMouseVRSensitivityPitch
//   GetMouseVRSensitivityYaw
//   GetNumAvailableClothingMods
//   GetNumAvailablePaintMods
//   GetNumContainerAllowedBuildFactions
//   GetNumTradeWares
//   GetPlayerGlobalLoadoutQuality
//   GetPlayerGlobalLoadoutQuantity
//   GetPlayerOccupiedShipNumFreeActorSlots
//   GetPluginCount
//   GetPluginType
//   GetRealComponentClass
//   GetRelationRangeUIMinValue
//   GetResourceBoxSize
//   GetSaveInquiryVentureText
//   GetSavesCompressedOption
//   GetShipPurpose
//   GetShipSize
//   GetShowRoomModules
//   GetStationModuleLoadoutQuality
//   GetStationModuleLoadoutQuantity
//   GetTobiiAngleFactor
//   GetTobiiDeadzoneAngle
//   GetTobiiDeadzonePosition
//   GetTobiiGazeAngleFactor
//   GetTobiiGazeDeadzone
//   GetTobiiGazeFilterStrength
//   GetTobiiHeadFilterStrength
//   GetTobiiHeadPositionFactor
//   GetTobiiMode
//   GetTradeWares
//   GetUIRelation
//   GetVRControllerAutoHide
//   GetVRVivePointerHand
//   GetVRViveTouchpadLockTime
//   GetVRWindowMode
//   GetWareReservationVolume
//   GetWeaponLaunchedMacro
//   GetWingName
//   HasCustomConversation
//   HasOrderQueuePriorityOrder
//   HasPopulatedSubordinateGroup
//   InstallClothingMod
//   IsAutoMouseEmulationActive
//   IsContainerFactionBuildRescricted
//   IsContainerFactionTradeRescricted
//   IsControlPanelHacked
//   IsDockingBay
//   IsDropDownMouseOverInteractionAllowed
//   IsEncryptedDirectInputModeActive
//   IsFactionHQ
//   IsFleetCommander
//   IsGestureSteeringActive
//   IsHeadTrackingActive
//   IsLeftMouseButtonDown
//   IsMiddleMouseButtonDown
//   IsMissionBoard
//   IsMouseDoubleClickMode
//   IsMouseVRPointerAllowed
//   IsPlayerControllingShip
//   IsPointingWithinAimingRange
//   IsRightMouseButtonDown
//   IsTobiiAvailable
//   IsVRMode
//   IsVROculusTouchActive
//   IsVRPointerActive
//   IsVRVersion
//   IsVRViveControllerActive
//   IsWingCommander
//   MapModifierKey
//   NvOptimusEnablement
//   ReassignFleetUnitToFleetUnit
//   RemoveClothingMod
//   RemovePaintMod
//   RemoveReadMessages
//   SetAssignment
//   SetCommander
//   SetContainerBuildAllowedFactions
//   SetContainerFactionBuildRescricted
//   SetCustomGameStartAccessProperty
//   SetCustomGameStartIntegerProperty
//   SetCustomGameStartPoliceProperty
//   SetLootMagnetActive
//   SetMacroMapPlayerSectorOffset
//   SetMapRenderCargoContents
//   SetMapRenderCrewInfo
//   SetMapRenderDockedShipInfos
//   SetMapRenderWorkForceInfo
//   SetMapSelectedVenture
//   SetMapSelectedVentureShip
//   SetMapToInitialCameraDistance
//   SetMapToMaxCameraDistance
//   SetModifierKeyPosition
//   SetMouseHUDModeOption
//   SetMouseVRPointerAllowed
//   SetMouseVRSensitivityPitch
//   SetMouseVRSensitivityYaw
//   SetPlayerCameraExternalView
//   SetPlayerCameraFloatingView
//   SetPlayerGlobalLoadoutQuality
//   SetPlayerGlobalLoadoutQuantity
//   SetSavesCompressedOption
//   SetSofttargetByMissionOffer
//   SetStationModuleLoadoutQuality
//   SetStationModuleLoadoutQuantity
//   SetTobiiAngleFactor
//   SetTobiiDeadzoneAngle
//   SetTobiiDeadzonePosition
//   SetTobiiGazeAngleFactor
//   SetTobiiGazeDeadzone
//   SetTobiiGazeFilterStrength
//   SetTobiiHeadFilterStrength
//   SetTobiiHeadPositionFactor
//   SetTobiiMode
//   SetVRControllerAutoHide
//   SetVRVivePointerHand
//   SetVRViveTouchpadLockTime
//   SetVRWindowMode
//   SetWeaponLaunchedMacro
//   SetWingName
//   StopEncryptedDirectInputHandling
//   TargetRadarComponent
//   ToggleLootMagnet
//   ToggleMouseSteeringMode
//   ToggleRadarMode
//   TransferPerson
//   UnmapModifierKey

#ifdef __cplusplus
}
#endif
