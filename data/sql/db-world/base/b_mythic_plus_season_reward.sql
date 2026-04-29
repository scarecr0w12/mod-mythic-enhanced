DROP TABLE IF EXISTS `mythic_plus_season_reward`;
CREATE TABLE `mythic_plus_season_reward`(
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

INSERT INTO `mythic_plus_season_reward` (`rank_start`, `rank_end`, `rewardtype`, `val1`, `val2`, `mail_subject`, `mail_body`) VALUES
(1, 1, 0, 250000000, NULL, 'Mythic season rewards', 'Congratulations on finishing rank #1 in the Mythic season. Your rewards are attached.'),
(1, 1, 1, 29434, 150, 'Mythic season rewards', 'Congratulations on finishing rank #1 in the Mythic season. Your rewards are attached.'),
(2, 3, 0, 150000000, NULL, 'Mythic season rewards', 'Congratulations on finishing in the top 3 of the Mythic season. Your rewards are attached.'),
(2, 3, 1, 29434, 100, 'Mythic season rewards', 'Congratulations on finishing in the top 3 of the Mythic season. Your rewards are attached.'),
(4, 10, 0, 90000000, NULL, 'Mythic season rewards', 'Congratulations on finishing in the top 10 of the Mythic season. Your rewards are attached.'),
(4, 10, 1, 29434, 60, 'Mythic season rewards', 'Congratulations on finishing in the top 10 of the Mythic season. Your rewards are attached.'),
(11, 25, 0, 45000000, NULL, 'Mythic season rewards', 'Congratulations on finishing in the top 25 of the Mythic season. Your rewards are attached.'),
(11, 25, 1, 29434, 30, 'Mythic season rewards', 'Congratulations on finishing in the top 25 of the Mythic season. Your rewards are attached.'),
(26, 50, 0, 20000000, NULL, 'Mythic season rewards', 'Congratulations on finishing in the top 50 of the Mythic season. Your rewards are attached.'),
(26, 50, 1, 29434, 15, 'Mythic season rewards', 'Congratulations on finishing in the top 50 of the Mythic season. Your rewards are attached.');
