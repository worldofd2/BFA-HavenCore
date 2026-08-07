-- =====================================================================
-- BFA-HavenCore
-- Quest 37446: Lazy Peons
-- NPC 10556: Lazy Peon
--
-- Fixes:
--   1. English objective displays "Peons Awoken".
--   2. Removes the legacy npc_lazy_peon C++ ScriptName so SmartAI is the
--      sole AI implementation.
--   3. Rebuilds Lazy Peon SmartAI so peons:
--        - spawn/reset asleep and awakenable,
--        - award quest credit once when hit by spell 19938,
--        - stand up,
--        - move to nearby lumber,
--        - work for 60-180 seconds,
--        - evade/reset and return to sleep.
--   4. Removes seven clearly overlapping Lazy Peon spawns.

START TRANSACTION;

-- ---------------------------------------------------------------------
-- 1. Quest objective text
-- ---------------------------------------------------------------------
UPDATE `quest_objectives`
SET `Description` = 'Peons Awoken'
WHERE `ID` = 276229
  AND `QuestID` = 37446;

-- ---------------------------------------------------------------------
-- 2. NPC 10556 must use SmartAI only.
--
-- The existing creature template had both:
--     AIName     = SmartAI
--     ScriptName = npc_lazy_peon
--
-- The C++ script independently handled spell 19938 and periodically
-- reapplied sleep, conflicting with the DB SmartAI implementation.
-- ---------------------------------------------------------------------
UPDATE `creature_template`
SET `AIName` = 'SmartAI',
    `ScriptName` = ''
WHERE `entry` = 10556;

-- Keep the template's default sleeping stand state.
-- bytes1 = 3 is UNIT_STAND_STATE_SLEEP in this core.
UPDATE `creature_template_addon`
SET `bytes1` = 3
WHERE `entry` = 10556;

-- ---------------------------------------------------------------------
-- 3. Rebuild SmartAI for Lazy Peon (10556)
--
-- HavenCore SmartAI values used here:
--   Event  8  = SMART_EVENT_SPELLHIT
--   Event 25  = SMART_EVENT_RESET
--   Event 61  = SMART_EVENT_LINK
--
--   Action 11 = SMART_ACTION_CAST
--   Action 17 = SMART_ACTION_SET_EMOTE_STATE
--   Action 24 = SMART_ACTION_EVADE
--   Action 28 = SMART_ACTION_REMOVEAURASFROMSPELL
--   Action 33 = SMART_ACTION_CALL_KILLEDMONSTER
--   Action 66 = SMART_ACTION_SET_ORIENTATION
--   Action 69 = SMART_ACTION_MOVE_TO_POS
--   Action 80 = SMART_ACTION_CALL_TIMED_ACTIONLIST
--   Action 90 = SMART_ACTION_SET_UNIT_FIELD_BYTES_1
--   Action 91 = SMART_ACTION_REMOVE_UNIT_FIELD_BYTES_1
--
-- Action 90 with action_param1=3/action_param2=0 sets stand state SLEEP.
-- Action 91 with action_param2=0 restores stand state STAND.
-- ---------------------------------------------------------------------

DELETE FROM `smart_scripts`
WHERE (`entryorguid` = 10556 AND `source_type` = 0)
   OR (`entryorguid` = 1055600 AND `source_type` = 9);

INSERT INTO `smart_scripts`
(
    `entryorguid`, `source_type`, `id`, `link`,
    `event_type`, `event_phase_mask`, `event_chance`, `event_flags`,
    `event_param1`, `event_param2`, `event_param3`, `event_param4`,
    `event_param5`, `event_param_string`,
    `action_type`, `action_param1`, `action_param2`, `action_param3`,
    `action_param4`, `action_param5`, `action_param6`,
    `target_type`, `target_param1`, `target_param2`, `target_param3`,
    `target_x`, `target_y`, `target_z`, `target_o`,
    `comment`
)
VALUES
-- Reset/spawn: ensure the peon is sleeping and has the required aura.
(10556, 0, 0, 1,
 25, 0, 100, 0,
 0, 0, 0, 0, 0, '',
 90, 3, 0, 0, 0, 0, 0,
 1, 0, 0, 0, 0, 0, 0, 0,
 'Lazy Peon - On Reset - Set Stand State Sleep'),

(10556, 0, 1, 0,
 61, 0, 100, 0,
 0, 0, 0, 0, 0, '',
 75, 17743, 0, 0, 0, 0, 0,
 1, 0, 0, 0, 0, 0, 0, 0,
 'Lazy Peon - On Reset - Add Sleep Aura 17743'),

-- Foreman's Blackjack (19938): give credit and begin wake sequence.
(10556, 0, 2, 3,
 8, 0, 100, 0,
 19938, 0, 0, 0, 0, '',
 33, 10556, 0, 0, 0, 0, 0,
 7, 0, 0, 0, 0, 0, 0, 0,
 'Lazy Peon - Spell 19938 Hit - Give Peon Credit'),

-- Remove the aura required by the spell condition so the same peon
-- cannot immediately be credited again.
(10556, 0, 3, 4,
 61, 0, 100, 0,
 0, 0, 0, 0, 0, '',
 28, 17743, 0, 0, 0, 0, 0,
 1, 0, 0, 0, 0, 0, 0, 0,
 'Lazy Peon - Wake - Remove Sleep Aura 17743'),

-- HavenCore action 91/type 0 explicitly restores UNIT_STAND_STATE_STAND.
(10556, 0, 4, 5,
 61, 0, 100, 0,
 0, 0, 0, 0, 0, '',
 91, 0, 0, 0, 0, 0, 0,
 1, 0, 0, 0, 0, 0, 0, 0,
 'Lazy Peon - Wake - Stand Up'),

-- Existing creature_text group 0 wake-up line.
(10556, 0, 5, 6,
 61, 0, 100, 0,
 0, 0, 0, 0, 0, '',
 1, 0, 0, 0, 0, 0, 0,
 7, 0, 0, 0, 0, 0, 0, 0,
 'Lazy Peon - Wake - Speak to Invoker'),

-- Start work sequence immediately after waking.
(10556, 0, 6, 0,
 61, 0, 100, 0,
 0, 0, 0, 0, 0, '',
 80, 1055600, 0, 2, 0, 0, 0,
 1, 0, 0, 0, 0, 0, 0, 0,
 'Lazy Peon - Wake - Start Work Action List'),

-- ---------------------------------------------------------------------
-- Timed action list 1055600
-- ---------------------------------------------------------------------

-- Face the nearest Lumber gameobject (175784) within 20 yards.
(1055600, 9, 0, 0,
 0, 0, 100, 0,
 2000, 2000, 0, 0, 0, '',
 66, 0, 0, 0, 0, 0, 0,
 20, 175784, 20, 0, 0, 0, 0, 0,
 'Lazy Peon - Work - Face Nearest Lumber'),

-- Move to the nearest Lumber gameobject.
(1055600, 9, 1, 0,
 0, 0, 100, 0,
 1000, 1000, 0, 0, 0, '',
 69, 0, 0, 0, 0, 0, 0,
 20, 175784, 20, 0, 0, 0, 0, 0,
 'Lazy Peon - Work - Go to Nearest Lumber'),

-- Begin chopping.
(1055600, 9, 2, 0,
 0, 0, 100, 0,
 5000, 5000, 0, 0, 0, '',
 17, 234, 0, 0, 0, 0, 0,
 1, 0, 0, 0, 0, 0, 0, 0,
 'Lazy Peon - Work - Chop Wood'),

-- Work for 60-180 seconds, then evade/reset.
(1055600, 9, 3, 0,
 0, 0, 100, 0,
 60000, 180000, 0, 0, 0, '',
 24, 0, 0, 0, 0, 0, 0,
 1, 0, 0, 0, 0, 0, 0, 0,
 'Lazy Peon - Work - Evade and Reset');

-- ---------------------------------------------------------------------
-- 4. Remove clearly overlapping Lazy Peon spawns
--
-- These seven GUIDs overlap lumber locations already served by another
-- Lazy Peon. We are deliberately NOT inventing replacement coordinates.
--
-- Remaining population: 13 Lazy Peons for a quest requiring 4 awakenings.
-- ---------------------------------------------------------------------

-- Harmless cleanup in case addon rows are added later/elsewhere.
DELETE FROM `creature_addon`
WHERE `guid` IN
(
    251482,
    251483,
    210115157,
    210115159,
    210115161,
    210115164,
    210115166
);

DELETE FROM `creature`
WHERE `id` = 10556
  AND `guid` IN
(
    251482,
    251483,
    210115157,
    210115159,
    210115161,
    210115164,
    210115166
);

COMMIT;
