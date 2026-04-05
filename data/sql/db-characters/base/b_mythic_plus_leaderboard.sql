DROP TABLE IF EXISTS `mythic_plus_leaderboard`;
CREATE TABLE `mythic_plus_leaderboard`(
    `season_id` int unsigned NOT NULL,
    `char_guid` int unsigned NOT NULL,
    `char_name` varchar(12) NOT NULL,
    `map_id` smallint unsigned NOT NULL,
    `difficulty` smallint unsigned NOT NULL,
    `mythic_level` int unsigned NOT NULL DEFAULT '0',
    `best_time` int unsigned NOT NULL DEFAULT '0',
    `deaths` int unsigned NOT NULL DEFAULT '0',
    `penalty_seconds` int unsigned NOT NULL DEFAULT '0',
    `completed_in_time` tinyint unsigned NOT NULL DEFAULT '0',
    `score` int unsigned NOT NULL DEFAULT '0',
    `group_members` varchar(255) NOT NULL DEFAULT '',
    `last_update` bigint unsigned NOT NULL DEFAULT '0',
    PRIMARY KEY (`season_id`, `char_guid`, `map_id`, `difficulty`),
    KEY `idx_mythic_plus_leaderboard_map` (`season_id`, `map_id`, `difficulty`, `score`),
    KEY `idx_mythic_plus_leaderboard_score` (`season_id`, `score`, `mythic_level`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
