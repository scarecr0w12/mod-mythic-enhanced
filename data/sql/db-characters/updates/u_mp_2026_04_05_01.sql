CREATE TABLE IF NOT EXISTS `mythic_plus_season_reward_log`(
    `season_id` int unsigned NOT NULL,
    `char_guid` int unsigned NOT NULL,
    `char_name` varchar(12) NOT NULL,
    `reward_rank` int unsigned NOT NULL,
    `total_money` int unsigned NOT NULL DEFAULT '0',
    `items_summary` varchar(255) NOT NULL DEFAULT '',
    `sent_at` bigint unsigned NOT NULL DEFAULT '0',
    PRIMARY KEY (`season_id`, `char_guid`),
    KEY `idx_mythic_plus_season_reward_log_rank` (`season_id`, `reward_rank`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
