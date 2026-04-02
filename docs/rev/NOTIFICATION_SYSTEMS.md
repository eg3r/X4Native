# X4 Player Notification Systems

Research into how X4 displays messages to the player and how X4Strategos can use these channels.

**Sources**: SDK function list, XSD schema definitions, game MD/Lua scripts, binary analysis.

---

## Summary

7 notification channels exist. 3 are immediately usable via MD bridge for the Player Advisor:

| Channel | Best For | Trigger | Max Text |
|---------|----------|---------|----------|
| **Message Ticker** | Urgent brief alerts | MD `show_notification` via bridge | 4 rows, ~40 chars each |
| **Logbook** | Persistent advice records | MD `write_to_logbook` via bridge | Unlimited (title + body) |
| **Help Overlay** | Contextual briefings | MD `show_help` via bridge | Multi-paragraph |

2 more via SDK:

| Channel | Best For | Trigger |
|---------|----------|---------|
| **Player Alerts** | Sector monitoring | `AddPlayerAlert2` (direct SDK call) |
| **News Feed** | Static extension info | content.xml only |

2 not suitable: Interactive Notifications (wrong UX), Mission System (too heavyweight).

**Critical**: `AddUINotification` does NOT exist in the SDK. Notifications are created exclusively via MD actions through the Lua bridge.

---

## Channel 1: Message Ticker (Top-Right HUD)

Scrolling notification bar in the top-right. 1-4 line messages, configurable timeout, priority queue.

**Trigger**: MD action only (no SDK export creates these):
```xml
<show_notification text="$text" priority="1" timeout="5s" sound="notification_generic" />
```

**Attributes** (from `common.xsd:36696`):

| Attribute | Type | Required | Description |
|-----------|------|----------|-------------|
| `text` | notificationtext | Yes | Text content (string or list of left/right pairs) |
| `priority` | int (0-9) | No | Default 1. Higher interrupts lower. >1 = red text |
| `timeout` | duration | No | Default 5s |
| `sound` | string | No | Sound ID. Default `notification_generic` |

**Text format** (`common.xsd:1058`):
- Single string: `"Warning: Xenon detected"`
- Row pairs: `['Left text', 'Right text', 'Row 2 left', 'Row 2 right']`
- With color: `['Normal', ['Red text', 255, 64, 64]]`

**Custom notification types**: X4Strategos can define `extension/libraries/notificationtypes.xml` with a `strategos_advice` category so players can toggle advisor notifications independently.

**Ticker Cache**: All `show_notification` calls are automatically recorded in the Notifications tab of the logbook -- free persistence.

**SDK read-only functions**:
- `GetNumNotifications() -> size_t`
- `GetNotificationID(size_t) -> int`
- `NotifyDisplayNotification(int id) -> void` (tells UI to display)
- `AbortCurrentNotification() -> void`
- `GetNotificationTypes2(UINotificationType2*, uint32_t) -> uint32_t`
- `SetNotificationTypeEnabled(const char* id, bool) -> void`

---

## Channel 2: Logbook System

Persistent record viewable from Player Info panel. Organized by category, persists in savegame, supports map links.

**Trigger**: MD action only:
```xml
<write_to_logbook category="general" title="$title" text="$bodytext" 
    interaction="showonmap" object="$sector" highlighted="true" />
```

**Categories** (from `common.xsd:4112`):

| Category | Best For (Advisor) |
|----------|--------------------|
| `general` | Broad strategic advice |
| `news` | Galaxy-level events and analysis |
| `diplomacy` | Relation/diplomatic advice |
| `alerts` | Threat warnings |
| `upkeep` | Economic/infrastructure advice |
| `tips` | General gameplay suggestions |

**Interaction types** (`loginteractionlookup`):

| Value | Behavior |
|-------|----------|
| `guidance` | Starts autopilot to the object |
| `showonmap` | Opens map focused on object |
| `showlocationonmap` | Opens map at position |

Advisor entries like "Build defense station in Hatikvah Border" can include clickable map links.

**Full attributes** (`common.xsd:38285`): category, title, text, separator, interaction, object, position, entity, faction, money, bonus, highlighted.

**SDK read-only**: `GetNumMessages`, `GetMessages` (returns `MessageInfo` with all fields), `SetMessageRead`.

---

## Channel 3: Help Overlay

Translucent text panel overlaid on game view. Supports custom text, positioning, duration, optional logbook integration.

**Trigger**: MD action:
```xml
<show_help custom="$text" position="1" force="true" width="280" 
    duration="15s" allowclose="true" log="true" />
```

**Key attributes** (`common.xsd:7629`):

| Attribute | Default | Description |
|-----------|---------|-------------|
| `custom` | - | Custom text (always shows, no "show once" restriction) |
| `position` | 0 | Screen position (14+ defined in parameters.xml) |
| `duration` | auto | Display duration |
| `width` | 200 | Width factor % (min 50) |
| `allowclose` | false | Show close button |
| `log` | false | Also write to Tips logbook tab |
| `color` | white | `[r,g,b,a]` text color |

Best for multi-paragraph strategic briefings. Use `log="true"` for persistence.

**Remove**: `<remove_help />` or `<remove_help all="true" />`

---

## Channel 4: Player Alert System

Configurable monitoring rules. Audio alerts + map markers when matching objects appear in sectors.

**Trigger**: Direct SDK call (no MD bridge needed):

```c
void AddPlayerAlert2(PlayerAlertInfo2 alert);
void UpdatePlayerAlert2(PlayerAlertInfo2 alert);
void RemovePlayerAlert(size_t index);
void MutePlayerAlert(size_t index);
void UnmutePlayerAlert(size_t index, bool silent);
```

```c
typedef struct {
    size_t index;
    double interval;           // Check interval (seconds)
    bool repeats;
    bool muted;
    uint32_t numspaces;        // Number of monitored sectors
    UniverseID* spaceids;      // Sector IDs array
    const char* objectclass;   // Object class filter
    const char* objectpurpose;
    const char* objectidcode;
    const char* objectowner;   // Faction filter
    const char* name;          // Alert name (UI label)
    const char* message;       // Alert message
    const char* soundid;       // Sound ID
} PlayerAlertInfo2;
```

**Native**: `AddPlayerAlert2` at `0x14013E1C0`. Appends to alert manager vector.

Could create sector watch alerts for the advisor ("Monitor Hatikvah Border for Xenon"). Advantage: native SDK, no bridge. Disadvantage: designed for object detection, not arbitrary text.

---

## Channel 5: News Feed (Limited)

Static extension announcements from content.xml. `GetNextNewsItem2` iterates, `SetNewsItemHidden` hides. Not runtime-usable -- only for "X4Strategos is active" type messages.

---

## Channel 6: Interactive Notifications (Not Recommended)

Target monitor notifications with NPC interaction callbacks (`show_interactive_notification`). 8 text rows, faction notoriety ladder, response handling. Wrong UX for passive advisor -- feels like NPC hails.

---

## Channel 7: Mission System (Not Recommended)

Full mission lifecycle. Too heavyweight for advisory messages. Possible Phase 3+ for "Strategic Objectives."

---

## MD Bridge Pattern for Advisor

`AddUITriggeredEvent` passes only 3 string params. For logbook entries needing category + title + text + interaction + object:

**Recommended**: Sequential variable-setting pattern:
```lua
AddUITriggeredEvent("x4strategos", "set_var", "category|general")
AddUITriggeredEvent("x4strategos", "set_var", "title|Economic Advisory")
AddUITriggeredEvent("x4strategos", "set_var", "text|Your hydrogen supply is critical...")
AddUITriggeredEvent("x4strategos", "write_logbook", "true") -- triggers MD cue
```

MD side receives `set_var` events, stores in MD variables, then `write_logbook` triggers the actual `write_to_logbook` action with the accumulated variables.

---

## Native References

| Function | Address | Notes |
|----------|---------|-------|
| `AddPlayerAlert2` | 0x14013E1C0 | Alert manager at `qword_143CAEE68`, vector at offset 566856 |
| `GetNumNotifications` | 0x140AB9950 | Array at `qword_146C86900 + 56/64` |
| `NotifyDisplayNotification` | 0x140ABBDE0 | Writes ID to `qword_146C86900 + 80` |
| `GetNotificationDetails` | 0x140ACC6E0 | Full notification struct layout decoded |
| `GetNumTickerCache` | 0x140166800 | Ticker cache = automatic notification history |

---

## XSD Source References

| Schema Location | Content |
|----------------|---------|
| `common.xsd:36696` | `show_notification` action definition |
| `common.xsd:38285` | `write_to_logbook` action definition |
| `common.xsd:36193` | `show_help` action group |
| `common.xsd:1058` | `notificationtext` type (text format) |
| `common.xsd:4112` | `logcategorylookup` (logbook categories) |
| `common.xsd:4129` | `loginteractionlookup` (map link types) |
| `common.xsd:7629` | `showhelp` attribute group |
