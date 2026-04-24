-- AzerothCore module auto-updater compatibility sync.
-- The core module updater scans modules/<name>/data/sql/world recursively.
-- This file mirrors the current Mythic Enhanced world schema in an idempotent way.

CREATE TABLE IF NOT EXISTS `mythic_plus_level`(
    `lvl` int unsigned NOT NULL,
    `timelimit` int unsigned NOT NULL,
    `random_affix_count` int unsigned NOT NULL DEFAULT '0',
    `hp_mult` float NOT NULL DEFAULT '1',
    `dmg_mult` float NOT NULL DEFAULT '1',
    PRIMARY KEY (`lvl`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

SET @mp_sql := IF(
    EXISTS(
        SELECT 1
        FROM `information_schema`.`COLUMNS`
        WHERE `TABLE_SCHEMA` = DATABASE()
          AND `TABLE_NAME` = 'mythic_plus_level'
          AND `COLUMN_NAME` = 'random_affix_count'
    ),
    'SELECT 1',
    'ALTER TABLE `mythic_plus_level` ADD COLUMN `random_affix_count` int unsigned NOT NULL DEFAULT 0 AFTER `timelimit`'
);
PREPARE `mp_stmt` FROM @mp_sql;
EXECUTE `mp_stmt`;
DEALLOCATE PREPARE `mp_stmt`;

SET @mp_sql := IF(
    EXISTS(
        SELECT 1
        FROM `information_schema`.`COLUMNS`
        WHERE `TABLE_SCHEMA` = DATABASE()
          AND `TABLE_NAME` = 'mythic_plus_level'
          AND `COLUMN_NAME` = 'hp_mult'
    ),
    'SELECT 1',
    'ALTER TABLE `mythic_plus_level` ADD COLUMN `hp_mult` float NOT NULL DEFAULT 1.0 AFTER `random_affix_count`'
);
PREPARE `mp_stmt` FROM @mp_sql;
EXECUTE `mp_stmt`;
DEALLOCATE PREPARE `mp_stmt`;

SET @mp_sql := IF(
    EXISTS(
        SELECT 1
        FROM `information_schema`.`COLUMNS`
        WHERE `TABLE_SCHEMA` = DATABASE()
          AND `TABLE_NAME` = 'mythic_plus_level'
          AND `COLUMN_NAME` = 'dmg_mult'
    ),
    'SELECT 1',
    'ALTER TABLE `mythic_plus_level` ADD COLUMN `dmg_mult` float NOT NULL DEFAULT 1.0 AFTER `hp_mult`'
);
PREPARE `mp_stmt` FROM @mp_sql;
EXECUTE `mp_stmt`;
DEALLOCATE PREPARE `mp_stmt`;

REPLACE INTO `mythic_plus_level`
    (`lvl`, `timelimit`, `random_affix_count`, `hp_mult`, `dmg_mult`)
VALUES
    (1, 60*45, 0, 1.35, 1.18),
    (2, 60*45, 0, 1.42, 1.22),
    (3, 60*40, 0, 1.50, 1.26),
    (4, 60*40, 0, 1.58, 1.30),
    (5, 60*40, 0, 1.66, 1.34),
    (7, 60*40, 1, 1.82, 1.42),
    (8, 60*40, 0, 1.90, 1.46),
    (9, 60*40, 0, 1.98, 1.50);

CREATE TABLE IF NOT EXISTS `mythic_plus_affix`(
    `lvl` int unsigned NOT NULL,
    `affixtype` smallint unsigned NOT NULL,
    `val1` float DEFAULT NULL,
    `val2` float DEFAULT NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

DELETE FROM `mythic_plus_affix` WHERE `lvl` IN (1, 2, 3, 4, 5, 7, 8, 9);
INSERT INTO `mythic_plus_affix` (`lvl`, `affixtype`, `val1`, `val2`) VALUES
    (1, 1, 0.15, NULL),
    (2, 1, 0.15, NULL),
    (2, 2, 0.10, NULL),
    (2, 4, 10.0, NULL),
    (3, 1, 0.20, NULL),
    (3, 2, 0.15, NULL),
    (3, 3, 15.0, NULL),
    (3, 4, 20.0, NULL),
    (4, 1, 0.20, NULL),
    (4, 2, 0.15, NULL),
    (4, 3, 30.0, NULL),
    (4, 4, 30.0, NULL),
    (5, 1, 0.20, NULL),
    (5, 2, 0.15, NULL),
    (5, 3, 30.0, NULL),
    (5, 4, 30.0, NULL),
    (5, 5, NULL, NULL),
    (7, 1, 0.20, NULL),
    (7, 2, 0.15, NULL),
    (7, 3, 30.0, NULL),
    (7, 4, 30.0, NULL),
    (7, 6, 25000.0, 65.0),
    (8, 9, 0.20, 15.0),
    (8, 10, 0.20, 15.0),
    (8, 11, 0.15, 10.0),
    (9, 9, 0.20, 15.0),
    (9, 10, 0.20, 15.0),
    (9, 11, 0.15, 10.0),
    (9, 12, 6.0, 12.0);

CREATE TABLE IF NOT EXISTS `mythic_plus_level_rewards`(
    `lvl` int unsigned NOT NULL,
    `rewardtype` smallint unsigned NOT NULL,
    `val1` int unsigned NOT NULL,
    `val2` int unsigned DEFAULT NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

DELETE FROM `mythic_plus_level_rewards` WHERE `lvl` IN (1, 2, 3, 4, 5, 7, 8, 9);
INSERT INTO `mythic_plus_level_rewards` (`lvl`, `rewardtype`, `val1`, `val2`) VALUES
    (1, 0, 1000000, NULL),
    (1, 1, 29434, 1),
    (2, 0, 4000000, NULL),
    (2, 1, 29434, 2),
    (3, 0, 8000000, NULL),
    (3, 1, 29434, 5),
    (4, 0, 10000000, NULL),
    (4, 1, 29434, 10),
    (5, 0, 15000000, NULL),
    (5, 1, 29434, 20),
    (7, 0, 30000000, NULL),
    (7, 1, 29434, 30),
    (8, 0, 35000000, NULL),
    (8, 1, 29434, 35),
    (9, 0, 45000000, NULL),
    (9, 1, 29434, 45);

CREATE TABLE IF NOT EXISTS `mythic_plus_capable_dungeon`(
    `map` smallint unsigned NOT NULL,
    `mapdifficulty` smallint unsigned NOT NULL,
    `final_boss_entry` int unsigned NOT NULL,
    PRIMARY KEY (`map`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

REPLACE INTO `mythic_plus_capable_dungeon`
    (`map`, `mapdifficulty`, `final_boss_entry`)
VALUES
    (658, 0, 36658),
    (632, 0, 36502),
    (619, 0, 29311),
    (601, 0, 29120),
    (600, 0, 26632),
    (604, 0, 29306),
    (602, 0, 28923),
    (599, 0, 27978),
    (576, 0, 26723),
    (578, 0, 27656),
    (574, 0, 23954),
    (575, 0, 26861),
    (389, 0, 11519),
    (47, 0, 4421);

CREATE TABLE IF NOT EXISTS `mythic_plus_map_scale`(
    `map` smallint unsigned NOT NULL,
    `mapdifficulty` smallint unsigned NOT NULL,
    `dmg_scale_trash` float NOT NULL DEFAULT '1',
    `dmg_scale_boss` float NOT NULL DEFAULT '1'
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

DELETE FROM `mythic_plus_map_scale` WHERE `map` IN (389, 47);
INSERT INTO `mythic_plus_map_scale`
    (`map`, `mapdifficulty`, `dmg_scale_trash`, `dmg_scale_boss`)
VALUES
    (389, 0, 4.0, 4.2),
    (47, 0, 4.0, 4.2);

CREATE TABLE IF NOT EXISTS `mythic_plus_spell_override`(
    `spellid` int unsigned NOT NULL,
    `map` int unsigned NOT NULL,
    `modpct` float DEFAULT '-1',
    `dotmodpct` float DEFAULT '-1',
    `comment` varchar(255) DEFAULT NULL,
    PRIMARY KEY (`spellid`, `map`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

REPLACE INTO `mythic_plus_spell_override`
    (`spellid`, `map`, `modpct`, `dotmodpct`, `comment`)
VALUES
    (9532, 389, 3.75, -1, 'Ragefire Shaman - Lightning Bolt'),
    (11968, 389, 18.0, -1, 'Molten Elemental - Fire Shield'),
    (18266, 389, -1, 8.0, 'Searing Blade Cultist - Curse of Agony'),
    (20800, 389, 5.0, 12.0, 'Jergosh the Invoker - Immolate DOT');

CREATE TABLE IF NOT EXISTS `mythic_plus_ignore_multiply_affix`(
    `entry` int unsigned NOT NULL,
    PRIMARY KEY (`entry`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

REPLACE INTO `mythic_plus_ignore_multiply_affix` (`entry`) VALUES
    (36990), (37779), (37588), (37584), (37596),
    (37583), (36595), (28070), (28149), (28824);

CREATE TABLE IF NOT EXISTS `mythic_plus_rotation`(
    `id` int unsigned NOT NULL AUTO_INCREMENT,
    `rotation_type` varchar(16) NOT NULL DEFAULT 'monthly',
    `start_unix` bigint unsigned NOT NULL,
    `end_unix` bigint unsigned NOT NULL,
    `affix_slot` tinyint unsigned NOT NULL,
    `affix_type` smallint unsigned NOT NULL,
    `val1` float DEFAULT NULL,
    `val2` float DEFAULT NULL,
    `enabled` tinyint unsigned NOT NULL DEFAULT '1',
    PRIMARY KEY (`id`),
    KEY `idx_mythic_plus_rotation_window` (`enabled`, `start_unix`, `end_unix`),
    KEY `idx_mythic_plus_rotation_slot` (`rotation_type`, `affix_slot`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS `mythic_plus_season_reward`(
    `rank_start` int unsigned NOT NULL,
    `rank_end` int unsigned NOT NULL,
    `rewardtype` smallint unsigned NOT NULL,
    `val1` int unsigned NOT NULL,
    `val2` int unsigned DEFAULT NULL,
    `mail_subject` varchar(120) DEFAULT NULL,
    `mail_body` text DEFAULT NULL,
    `enabled` tinyint unsigned NOT NULL DEFAULT '1',
    KEY `idx_mythic_plus_season_reward_rank` (`rank_start`, `rank_end`, `enabled`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

INSERT INTO `mythic_plus_season_reward`
    (`rank_start`, `rank_end`, `rewardtype`, `val1`, `val2`,
    `mail_subject`, `mail_body`)
SELECT 1, 1, 0, 50000000, NULL,
    'Mythic season rewards',
    'Congratulations on finishing rank #1 in the Mythic season. Your rewards are attached.'
WHERE NOT EXISTS (
    SELECT 1 FROM `mythic_plus_season_reward`
    WHERE `rank_start` = 1 AND `rank_end` = 1 AND `rewardtype` = 0 AND `val1` = 50000000
);

INSERT INTO `mythic_plus_season_reward`
    (`rank_start`, `rank_end`, `rewardtype`, `val1`, `val2`,
    `mail_subject`, `mail_body`)
SELECT 1, 1, 1, 29434, 50,
    'Mythic season rewards',
    'Congratulations on finishing rank #1 in the Mythic season. Your rewards are attached.'
WHERE NOT EXISTS (
    SELECT 1 FROM `mythic_plus_season_reward`
    WHERE `rank_start` = 1 AND `rank_end` = 1 AND `rewardtype` = 1 AND `val1` = 29434 AND `val2` = 50
);

INSERT INTO `mythic_plus_season_reward`
    (`rank_start`, `rank_end`, `rewardtype`, `val1`, `val2`,
    `mail_subject`, `mail_body`)
SELECT 2, 3, 0, 25000000, NULL,
    'Mythic season rewards',
    'Congratulations on finishing in the top 3 of the Mythic season. Your rewards are attached.'
WHERE NOT EXISTS (
    SELECT 1 FROM `mythic_plus_season_reward`
    WHERE `rank_start` = 2 AND `rank_end` = 3 AND `rewardtype` = 0 AND `val1` = 25000000
);

INSERT INTO `mythic_plus_season_reward`
    (`rank_start`, `rank_end`, `rewardtype`, `val1`, `val2`,
    `mail_subject`, `mail_body`)
SELECT 2, 3, 1, 29434, 30,
    'Mythic season rewards',
    'Congratulations on finishing in the top 3 of the Mythic season. Your rewards are attached.'
WHERE NOT EXISTS (
    SELECT 1 FROM `mythic_plus_season_reward`
    WHERE `rank_start` = 2 AND `rank_end` = 3 AND `rewardtype` = 1 AND `val1` = 29434 AND `val2` = 30
);

INSERT INTO `mythic_plus_season_reward`
    (`rank_start`, `rank_end`, `rewardtype`, `val1`, `val2`,
    `mail_subject`, `mail_body`)
SELECT 4, 10, 0, 10000000, NULL,
    'Mythic season rewards',
    'Congratulations on finishing in the top 10 of the Mythic season. Your rewards are attached.'
WHERE NOT EXISTS (
    SELECT 1 FROM `mythic_plus_season_reward`
    WHERE `rank_start` = 4 AND `rank_end` = 10 AND `rewardtype` = 0 AND `val1` = 10000000
);

INSERT INTO `mythic_plus_season_reward`
    (`rank_start`, `rank_end`, `rewardtype`, `val1`, `val2`,
    `mail_subject`, `mail_body`)
SELECT 4, 10, 1, 29434, 15,
    'Mythic season rewards',
    'Congratulations on finishing in the top 10 of the Mythic season. Your rewards are attached.'
WHERE NOT EXISTS (
    SELECT 1 FROM `mythic_plus_season_reward`
    WHERE `rank_start` = 4 AND `rank_end` = 10 AND `rewardtype` = 1 AND `val1` = 29434 AND `val2` = 15
);

CREATE TABLE IF NOT EXISTS `mythic_plus_dungeon_ui`(
    `map_id` smallint unsigned NOT NULL,
    `slug` varchar(64) NOT NULL,
    `display_name` varchar(80) NOT NULL,
    `short_name` varchar(32) NOT NULL DEFAULT '',
    `expansion` varchar(16) NOT NULL DEFAULT 'wrath',
    `icon_key` varchar(64) NOT NULL DEFAULT '',
    `image_key` varchar(64) NOT NULL DEFAULT '',
    `sort_order` smallint unsigned NOT NULL DEFAULT '0',
    `enabled` tinyint unsigned NOT NULL DEFAULT '1',
    PRIMARY KEY (`map_id`),
    UNIQUE KEY `uk_mythic_plus_dungeon_ui_slug` (`slug`),
    KEY `idx_mythic_plus_dungeon_ui_enabled_sort` (`enabled`, `sort_order`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

REPLACE INTO `mythic_plus_dungeon_ui`
    (`map_id`, `slug`, `display_name`, `short_name`, `expansion`,
    `icon_key`, `image_key`, `sort_order`, `enabled`)
VALUES
    (47, 'razorfen-kraul', 'Razorfen Kraul', 'RFK', 'classic', 'razorfen-kraul', 'razorfen-kraul', 10, 1),
    (389, 'ragefire-chasm', 'Ragefire Chasm', 'RFC', 'classic', 'ragefire-chasm', 'ragefire-chasm', 20, 1),
    (574, 'utgarde-keep', 'Utgarde Keep', 'UK', 'wrath', 'utgarde-keep', 'utgarde-keep', 30, 1),
    (575, 'utgarde-pinnacle', 'Utgarde Pinnacle', 'UP', 'wrath', 'utgarde-pinnacle', 'utgarde-pinnacle', 40, 1),
    (576, 'the-nexus', 'The Nexus', 'Nexus', 'wrath', 'the-nexus', 'the-nexus', 50, 1),
    (578, 'the-oculus', 'The Oculus', 'Oculus', 'wrath', 'the-oculus', 'the-oculus', 60, 1),
    (599, 'halls-of-stone', 'Halls of Stone', 'HoS', 'wrath', 'halls-of-stone', 'halls-of-stone', 70, 1),
    (600, 'draktharon-keep', 'Drak\'Tharon Keep', 'DTK', 'wrath', 'draktharon-keep', 'draktharon-keep', 80, 1),
    (601, 'azjol-nerub', 'Azjol-Nerub', 'AN', 'wrath', 'azjol-nerub', 'azjol-nerub', 90, 1),
    (602, 'halls-of-lightning', 'Halls of Lightning', 'HoL', 'wrath', 'halls-of-lightning', 'halls-of-lightning', 100, 1),
    (604, 'gundrak', 'Gundrak', 'Gundrak', 'wrath', 'gundrak', 'gundrak', 110, 1),
    (619, 'ahnkahet-old-kingdom', 'Ahn\'kahet: The Old Kingdom', 'Ahn\'kahet', 'wrath', 'ahnkahet-old-kingdom', 'ahnkahet-old-kingdom', 120, 1),
    (632, 'forge-of-souls', 'The Forge of Souls', 'FoS', 'wrath', 'forge-of-souls', 'forge-of-souls', 130, 1),
    (658, 'pit-of-saron', 'Pit of Saron', 'PoS', 'wrath', 'pit-of-saron', 'pit-of-saron', 140, 1);

DROP VIEW IF EXISTS `mythic_plus_web_dungeon_catalog`;
CREATE VIEW `mythic_plus_web_dungeon_catalog` AS
SELECT
    `ui`.`map_id`,
    `ui`.`slug`,
    `ui`.`display_name`,
    `ui`.`short_name`,
    `ui`.`expansion`,
    `ui`.`icon_key`,
    `ui`.`image_key`,
    `ui`.`sort_order`,
    `ui`.`enabled`,
    `capable`.`mapdifficulty` AS `min_difficulty`,
    CASE WHEN `capable`.`mapdifficulty` = 1 THEN 'heroic' ELSE 'normal' END AS `min_difficulty_label`,
    `capable`.`final_boss_entry`,
    CASE WHEN `capable`.`map` IS NULL THEN 0 ELSE 1 END AS `is_mythic_enabled`
FROM `mythic_plus_dungeon_ui` `ui`
LEFT JOIN `mythic_plus_capable_dungeon` `capable`
    ON `capable`.`map` = `ui`.`map_id`
WHERE `ui`.`enabled` = 1;

DELETE FROM `command`
WHERE `name` IN (
    'mythic', 'mythic reload', 'mythic info', 'mythic leaderboard',
    'mythic leaderboard map', 'mythic leaderboard maps', 'mythic rating',
    'mythic season', 'mythic season history', 'mythic season rewards'
);

INSERT INTO `command` (`name`, `security`, `help`) VALUES
    ('mythic', 0, 'Syntax: .mythic $subcommand\nType .mythic to see the list of possible subcommands.'),
    ('mythic reload', 3, 'Reloads Mythic Plus data including affixes, rotations, rewards, and leaderboard settings'),
    ('mythic info', 0, 'Prints current Mythic Plus information'),
    ('mythic leaderboard', 0, 'Syntax: .mythic leaderboard [$seasonId]\nShows the overall leaderboard for the active or selected season.'),
    ('mythic leaderboard map', 0, 'Syntax: .mythic leaderboard map $mapId [$seasonId]\nShows the map leaderboard for the active or selected season.'),
    ('mythic leaderboard maps', 0, 'Lists all Mythic Plus capable map ids and names.'),
    ('mythic rating', 0, 'Syntax: .mythic rating [$seasonId]\nShows your rating summary for the active or selected season.'),
    ('mythic season', 0, 'Syntax: .mythic season [$seasonId]\nShows active season info or details for a selected archived season.'),
    ('mythic season history', 0, 'Shows recent Mythic season ids, labels, and time windows.'),
    ('mythic season rewards', 3, 'Syntax: .mythic season rewards [$seasonId]\nDistributes configured seasonal rewards for the active or selected season if they have not already been sent.');

SET @Entry = 200005;
DELETE FROM `creature_template_model` WHERE `CreatureID` = @Entry;
DELETE FROM `creature_template` WHERE `entry` = @Entry;
INSERT INTO `creature_template`
    (`entry`, `name`, `subname`, `IconName`, `gossip_menu_id`, `minlevel`,
    `maxlevel`, `exp`, `faction`, `npcflag`, `rank`, `dmgschool`,
    `baseattacktime`, `rangeattacktime`, `unit_class`, `unit_flags`, `type`,
    `type_flags`, `lootid`, `pickpocketloot`, `skinloot`, `AIName`,
    `MovementType`, `HoverHeight`, `RacialLeader`, `movementId`, `RegenHealth`,
    `flags_extra`, `ScriptName`)
VALUES
    (@Entry, 'Mythic Plus', 'Master', NULL, 0, 80, 80, 2, 35, 1, 0, 0, 2000,
    0, 1, 2147483648, 7, 138936390, 0, 0, 0, '', 0, 1, 0, 0, 1, 0,
    'mod_mythic_enhanced_npc');
INSERT INTO `creature_template_model`
    (`CreatureID`, `Idx`, `CreatureDisplayID`, `DisplayScale`, `Probability`, `VerifiedBuild`)
VALUES
    (@Entry, 0, 19646, 1, 1, 0);

SET @Entry = 200006;
DELETE FROM `creature_template_model` WHERE `CreatureID` = @Entry;
DELETE FROM `creature_template` WHERE `entry` = @Entry;
INSERT INTO `creature_template`
    (`entry`, `name`, `subname`, `IconName`, `gossip_menu_id`, `minlevel`,
    `maxlevel`, `exp`, `faction`, `npcflag`, `rank`, `dmgschool`,
    `baseattacktime`, `rangeattacktime`, `unit_class`, `unit_flags`, `type`,
    `type_flags`, `lootid`, `pickpocketloot`, `skinloot`, `AIName`,
    `MovementType`, `HoverHeight`, `RacialLeader`, `movementId`, `RegenHealth`,
    `flags_extra`, `HealthModifier`, `ScriptName`)
VALUES
    (@Entry, 'Lightning', 'Sphere', NULL, 0, 80, 80, 2, 14, 0, 1, 0, 2000,
    0, 8, 131076, 10, 0, 0, 0, 0, '', 0, 1, 0, 0, 1, 0, 3,
    'npc_lightning_sphere');
INSERT INTO `creature_template_model`
    (`CreatureID`, `Idx`, `CreatureDisplayID`, `DisplayScale`, `Probability`, `VerifiedBuild`)
VALUES
    (@Entry, 0, 25144, 1, 1, 0);

REPLACE INTO `item_template`
    (`entry`,`class`,`subclass`,`SoundOverrideSubclass`,`name`,`displayid`,
    `Quality`,`Flags`,`FlagsExtra`,`BuyCount`,`BuyPrice`,`SellPrice`,
    `InventoryType`,`AllowableClass`,`AllowableRace`,`ItemLevel`,
    `RequiredLevel`,`RequiredSkill`,`RequiredSkillRank`,`requiredspell`,
    `requiredhonorrank`,`RequiredCityRank`,`RequiredReputationFaction`,
    `RequiredReputationRank`,`maxcount`,`stackable`,`ContainerSlots`,`stat_type1`,
    `stat_value1`,`stat_type2`,`stat_value2`,`stat_type3`,`stat_value3`,
    `stat_type4`,`stat_value4`,`stat_type5`,`stat_value5`,`stat_type6`,
    `stat_value6`,`stat_type7`,`stat_value7`,`stat_type8`,`stat_value8`,
    `stat_type9`,`stat_value9`,`stat_type10`,`stat_value10`,
    `ScalingStatDistribution`,`ScalingStatValue`,`dmg_min1`,`dmg_max1`,
    `dmg_type1`,`dmg_min2`,`dmg_max2`,`dmg_type2`,`armor`,`holy_res`,
    `fire_res`,`nature_res`,`frost_res`,`shadow_res`,`arcane_res`,`delay`,
    `ammo_type`,`RangedModRange`,`spellid_1`,`spelltrigger_1`,`spellcharges_1`,
    `spellppmRate_1`,`spellcooldown_1`,`spellcategory_1`,
    `spellcategorycooldown_1`,`spellid_2`,`spelltrigger_2`,`spellcharges_2`,
    `spellppmRate_2`,`spellcooldown_2`,`spellcategory_2`,
    `spellcategorycooldown_2`,`spellid_3`,`spelltrigger_3`,`spellcharges_3`,
    `spellppmRate_3`,`spellcooldown_3`,`spellcategory_3`,
    `spellcategorycooldown_3`,`spellid_4`,`spelltrigger_4`,`spellcharges_4`,
    `spellppmRate_4`,`spellcooldown_4`,`spellcategory_4`,
    `spellcategorycooldown_4`,`spellid_5`,`spelltrigger_5`,`spellcharges_5`,
    `spellppmRate_5`,`spellcooldown_5`,`spellcategory_5`,
    `spellcategorycooldown_5`,`bonding`,`description`,`PageText`,`LanguageID`,
    `PageMaterial`,`startquest`,`lockid`,`Material`,`sheath`,`RandomProperty`,
    `RandomSuffix`,`block`,`itemset`,`MaxDurability`,`area`,`Map`,`BagFamily`,
    `TotemCategory`,`socketColor_1`,`socketContent_1`,`socketColor_2`,
    `socketContent_2`,`socketColor_3`,`socketContent_3`,`socketBonus`,
    `GemProperties`,`RequiredDisenchantSkill`,`ArmorDamageModifier`,`duration`,
    `ItemLimitCategory`,`HolidayId`,`ScriptName`,`DisenchantID`,`FoodType`,
    `minMoneyLoot`,`maxMoneyLoot`,`flagsCustom`,`VerifiedBuild`)
VALUES
    (70001,15,0,-1,'Mythic Plus Keystone',58859,4,64,0,1,0,0,0,-1,-1,1,0,
    0,0,0,0,0,0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,46331,0,0,0,-1,0,-1,0,0,0,0,-1,0,-1,0,0,
    0,0,-1,0,-1,0,0,0,0,-1,0,-1,0,0,0,0,-1,0,-1,1,
    'Keystone used to transform a dungeon into Mythic Plus.',0,0,0,0,0,-1,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,-1,0,0,0,0,'mythic_plus_keystone',0,0,
    0,0,0,-1);

REPLACE INTO `item_dbc`
    (`ID`,`ClassID`,`SubclassID`,`Sound_Override_Subclassid`,`Material`,
    `DisplayInfoID`,`InventoryType`,`SheatheType`)
VALUES
    (70001,15,0,-1,-1,58859,0,0);

SET @TOKEN := 70002;
SET @BASE := 70001;
REPLACE INTO `item_template`
SELECT @TOKEN, `class`, `subclass`, `SoundOverrideSubclass`,
    'Mythic Upgrade Token', 58859, `Quality`, `Flags`, `FlagsExtra`, `BuyCount`,
    `BuyPrice`, `SellPrice`, `InventoryType`, `AllowableClass`,
    `AllowableRace`, `ItemLevel`, `RequiredLevel`, `RequiredSkill`,
    `RequiredSkillRank`, `requiredspell`, `requiredhonorrank`,
    `RequiredCityRank`, `RequiredReputationFaction`, `RequiredReputationRank`,
    `maxcount`, `stackable`, `ContainerSlots`, `stat_type1`, `stat_value1`,
    `stat_type2`, `stat_value2`, `stat_type3`, `stat_value3`, `stat_type4`,
    `stat_value4`, `stat_type5`, `stat_value5`, `stat_type6`, `stat_value6`,
    `stat_type7`, `stat_value7`, `stat_type8`, `stat_value8`, `stat_type9`,
    `stat_value9`, `stat_type10`, `stat_value10`, `ScalingStatDistribution`,
    `ScalingStatValue`, `dmg_min1`, `dmg_max1`, `dmg_type1`, `dmg_min2`,
    `dmg_max2`, `dmg_type2`, `armor`, `holy_res`, `fire_res`, `nature_res`,
    `frost_res`, `shadow_res`, `arcane_res`, `delay`, `ammo_type`,
    `RangedModRange`, `spellid_1`, `spelltrigger_1`, `spellcharges_1`,
    `spellppmRate_1`, `spellcooldown_1`, `spellcategory_1`,
    `spellcategorycooldown_1`, `spellid_2`, `spelltrigger_2`, `spellcharges_2`,
    `spellppmRate_2`, `spellcooldown_2`, `spellcategory_2`,
    `spellcategorycooldown_2`, `spellid_3`, `spelltrigger_3`, `spellcharges_3`,
    `spellppmRate_3`, `spellcooldown_3`, `spellcategory_3`,
    `spellcategorycooldown_3`, `spellid_4`, `spelltrigger_4`, `spellcharges_4`,
    `spellppmRate_4`, `spellcooldown_4`, `spellcategory_4`,
    `spellcategorycooldown_4`, `spellid_5`, `spelltrigger_5`, `spellcharges_5`,
    `spellppmRate_5`, `spellcooldown_5`, `spellcategory_5`,
    `spellcategorycooldown_5`, `bonding`,
    'Spend at the Item Upgrade NPC to improve your gear.', `PageText`,
    `LanguageID`, `PageMaterial`, `startquest`, `lockid`, `Material`, `sheath`,
    `RandomProperty`, `RandomSuffix`, `block`, `itemset`, `MaxDurability`,
    `area`, `Map`, `BagFamily`, `TotemCategory`, `socketColor_1`,
    `socketContent_1`, `socketColor_2`, `socketContent_2`, `socketColor_3`,
    `socketContent_3`, `socketBonus`, `GemProperties`,
    `RequiredDisenchantSkill`, `ArmorDamageModifier`, `duration`,
    `ItemLimitCategory`, `HolidayId`, '', `DisenchantID`, `FoodType`,
    `minMoneyLoot`, `maxMoneyLoot`, `flagsCustom`, `VerifiedBuild`
FROM `item_template` WHERE `entry` = @BASE LIMIT 1;

REPLACE INTO `item_dbc`
    (`ID`,`ClassID`,`SubclassID`,`Sound_Override_Subclassid`,`Material`,
    `DisplayInfoID`,`InventoryType`,`SheatheType`)
SELECT @TOKEN, `ClassID`, `SubclassID`, `Sound_Override_Subclassid`,
    `Material`, `DisplayInfoID`, `InventoryType`, `SheatheType`
FROM `item_dbc` WHERE `ID` = @BASE LIMIT 1;
