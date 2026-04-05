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

INSERT INTO `mythic_plus_season_reward` (`rank_start`, `rank_end`, `rewardtype`, `val1`, `val2`, `mail_subject`, `mail_body`)
SELECT 1, 1, 0, 50000000, NULL, 'Mythic season rewards', 'Congratulations on finishing rank #1 in the Mythic season. Your rewards are attached.'
WHERE NOT EXISTS (SELECT 1 FROM `mythic_plus_season_reward`);
INSERT INTO `mythic_plus_season_reward` (`rank_start`, `rank_end`, `rewardtype`, `val1`, `val2`, `mail_subject`, `mail_body`)
SELECT 1, 1, 1, 29434, 50, 'Mythic season rewards', 'Congratulations on finishing rank #1 in the Mythic season. Your rewards are attached.'
WHERE NOT EXISTS (SELECT 1 FROM `mythic_plus_season_reward` WHERE `rank_start` = 1 AND `rank_end` = 1 AND `rewardtype` = 1 AND `val1` = 29434 AND `val2` = 50);
INSERT INTO `mythic_plus_season_reward` (`rank_start`, `rank_end`, `rewardtype`, `val1`, `val2`, `mail_subject`, `mail_body`)
SELECT 2, 3, 0, 25000000, NULL, 'Mythic season rewards', 'Congratulations on finishing in the top 3 of the Mythic season. Your rewards are attached.'
WHERE NOT EXISTS (SELECT 1 FROM `mythic_plus_season_reward` WHERE `rank_start` = 2 AND `rank_end` = 3 AND `rewardtype` = 0 AND `val1` = 25000000);
INSERT INTO `mythic_plus_season_reward` (`rank_start`, `rank_end`, `rewardtype`, `val1`, `val2`, `mail_subject`, `mail_body`)
SELECT 2, 3, 1, 29434, 30, 'Mythic season rewards', 'Congratulations on finishing in the top 3 of the Mythic season. Your rewards are attached.'
WHERE NOT EXISTS (SELECT 1 FROM `mythic_plus_season_reward` WHERE `rank_start` = 2 AND `rank_end` = 3 AND `rewardtype` = 1 AND `val1` = 29434 AND `val2` = 30);
INSERT INTO `mythic_plus_season_reward` (`rank_start`, `rank_end`, `rewardtype`, `val1`, `val2`, `mail_subject`, `mail_body`)
SELECT 4, 10, 0, 10000000, NULL, 'Mythic season rewards', 'Congratulations on finishing in the top 10 of the Mythic season. Your rewards are attached.'
WHERE NOT EXISTS (SELECT 1 FROM `mythic_plus_season_reward` WHERE `rank_start` = 4 AND `rank_end` = 10 AND `rewardtype` = 0 AND `val1` = 10000000);
INSERT INTO `mythic_plus_season_reward` (`rank_start`, `rank_end`, `rewardtype`, `val1`, `val2`, `mail_subject`, `mail_body`)
SELECT 4, 10, 1, 29434, 15, 'Mythic season rewards', 'Congratulations on finishing in the top 10 of the Mythic season. Your rewards are attached.'
WHERE NOT EXISTS (SELECT 1 FROM `mythic_plus_season_reward` WHERE `rank_start` = 4 AND `rank_end` = 10 AND `rewardtype` = 1 AND `val1` = 29434 AND `val2` = 15);
