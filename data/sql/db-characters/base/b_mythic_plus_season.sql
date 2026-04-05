DROP TABLE IF EXISTS `mythic_plus_season`;
CREATE TABLE `mythic_plus_season`(
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
