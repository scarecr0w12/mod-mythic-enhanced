DROP TABLE IF EXISTS `mythic_plus_rotation`;
CREATE TABLE `mythic_plus_rotation`(
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
