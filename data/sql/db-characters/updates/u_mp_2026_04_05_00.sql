CREATE TABLE IF NOT EXISTS `mythic_plus_season`(
    `id` int unsigned NOT NULL AUTO_INCREMENT,
    `year` smallint unsigned NOT NULL,
    `month` tinyint unsigned NOT NULL,
    `start_unix` bigint unsigned NOT NULL,
    `end_unix` bigint unsigned NOT NULL,
    `is_active` tinyint unsigned NOT NULL DEFAULT '0',
    `label` varchar(16) NOT NULL,
    PRIMARY KEY (`id`),
    UNIQUE KEY `uk_mythic_plus_season_year_month` (`year`, `month`),
    KEY `idx_mythic_plus_season_active` (`is_active`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS `mythic_plus_leaderboard`(
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
