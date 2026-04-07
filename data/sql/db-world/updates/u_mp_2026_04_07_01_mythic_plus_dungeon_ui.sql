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

INSERT IGNORE INTO `mythic_plus_dungeon_ui`
    (`map_id`, `slug`, `display_name`, `short_name`, `expansion`,
    `icon_key`, `image_key`, `sort_order`, `enabled`)
VALUES
    (47, 'razorfen-kraul', 'Razorfen Kraul', 'RFK', 'classic',
        'razorfen-kraul', 'razorfen-kraul', 10, 1),
    (389, 'ragefire-chasm', 'Ragefire Chasm', 'RFC', 'classic',
        'ragefire-chasm', 'ragefire-chasm', 20, 1),
    (574, 'utgarde-keep', 'Utgarde Keep', 'UK', 'wrath',
        'utgarde-keep', 'utgarde-keep', 30, 1),
    (575, 'utgarde-pinnacle', 'Utgarde Pinnacle', 'UP', 'wrath',
        'utgarde-pinnacle', 'utgarde-pinnacle', 40, 1),
    (576, 'the-nexus', 'The Nexus', 'Nexus', 'wrath',
        'the-nexus', 'the-nexus', 50, 1),
    (578, 'the-oculus', 'The Oculus', 'Oculus', 'wrath',
        'the-oculus', 'the-oculus', 60, 1),
    (599, 'halls-of-stone', 'Halls of Stone', 'HoS', 'wrath',
        'halls-of-stone', 'halls-of-stone', 70, 1),
    (600, 'draktharon-keep', 'Drak\'Tharon Keep', 'DTK', 'wrath',
        'draktharon-keep', 'draktharon-keep', 80, 1),
    (601, 'azjol-nerub', 'Azjol-Nerub', 'AN', 'wrath',
        'azjol-nerub', 'azjol-nerub', 90, 1),
    (602, 'halls-of-lightning', 'Halls of Lightning', 'HoL', 'wrath',
        'halls-of-lightning', 'halls-of-lightning', 100, 1),
    (604, 'gundrak', 'Gundrak', 'Gundrak', 'wrath',
        'gundrak', 'gundrak', 110, 1),
    (619, 'ahnkahet-old-kingdom', 'Ahn\'kahet: The Old Kingdom',
        'Ahn\'kahet', 'wrath', 'ahnkahet-old-kingdom',
        'ahnkahet-old-kingdom', 120, 1),
    (632, 'forge-of-souls', 'The Forge of Souls', 'FoS', 'wrath',
        'forge-of-souls', 'forge-of-souls', 130, 1),
    (658, 'pit-of-saron', 'Pit of Saron', 'PoS', 'wrath',
        'pit-of-saron', 'pit-of-saron', 140, 1);

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
    CASE
        WHEN `capable`.`mapdifficulty` = 1 THEN 'heroic'
        ELSE 'normal'
    END AS `min_difficulty_label`,
    `capable`.`final_boss_entry`,
    CASE
        WHEN `capable`.`map` IS NULL THEN 0
        ELSE 1
    END AS `is_mythic_enabled`
FROM `mythic_plus_dungeon_ui` `ui`
LEFT JOIN `mythic_plus_capable_dungeon` `capable`
    ON `capable`.`map` = `ui`.`map_id`
WHERE `ui`.`enabled` = 1;
