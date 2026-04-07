DROP VIEW IF EXISTS `mythic_plus_web_current_season`;
DROP VIEW IF EXISTS `mythic_plus_web_current_run_history`;
DROP VIEW IF EXISTS `mythic_plus_web_leaderboard_current_map`;
DROP VIEW IF EXISTS `mythic_plus_web_leaderboard_current_overall`;
DROP VIEW IF EXISTS `mythic_plus_web_leaderboard_map`;
DROP VIEW IF EXISTS `mythic_plus_web_leaderboard_overall`;
DROP VIEW IF EXISTS `mythic_plus_web_run_history`;
DROP VIEW IF EXISTS `mythic_plus_web_seasons`;

CREATE VIEW `mythic_plus_web_current_season` AS
SELECT
    `id` AS `season_id`,
    `year`,
    `month`,
    `start_unix`,
    `end_unix`,
    `label`,
    `is_active`,
    IF(`end_unix` > UNIX_TIMESTAMP(), `end_unix` - UNIX_TIMESTAMP(), 0)
        AS `seconds_until_end`
FROM `mythic_plus_season`
WHERE `is_active` = 1;

CREATE VIEW `mythic_plus_web_seasons` AS
SELECT
    `season`.`id` AS `season_id`,
    `season`.`year`,
    `season`.`month`,
    `season`.`start_unix`,
    `season`.`end_unix`,
    `season`.`label`,
    `season`.`is_active`,
    COUNT(`lb`.`char_guid`) AS `leaderboard_entries`,
    COUNT(DISTINCT `lb`.`char_guid`) AS `player_count`,
    COALESCE(SUM(`lb`.`score`), 0) AS `total_score`,
    COALESCE(MAX(`lb`.`last_update`), 0) AS `last_update`
FROM `mythic_plus_season` `season`
LEFT JOIN `mythic_plus_leaderboard` `lb`
    ON `lb`.`season_id` = `season`.`id`
GROUP BY
    `season`.`id`,
    `season`.`year`,
    `season`.`month`,
    `season`.`start_unix`,
    `season`.`end_unix`,
    `season`.`label`,
    `season`.`is_active`;

CREATE VIEW `mythic_plus_web_leaderboard_overall` AS
SELECT
    `lb`.`season_id`,
    `season`.`label` AS `season_label`,
    `season`.`year` AS `season_year`,
    `season`.`month` AS `season_month`,
    `lb`.`char_guid`,
    COALESCE(
        MAX(`characters`.`name`),
        SUBSTRING_INDEX(
            GROUP_CONCAT(
                `lb`.`char_name`
                ORDER BY `lb`.`last_update` DESC SEPARATOR ','
            ),
            ',',
            1
        )
    ) AS `char_name`,
    COALESCE(MAX(`characters`.`race`), 0) AS `char_race`,
    COALESCE(MAX(`characters`.`class`), 0) AS `char_class`,
    COALESCE(MAX(`characters`.`gender`), 0) AS `char_gender`,
    COALESCE(MAX(`characters`.`level`), 0) AS `char_level`,
    COALESCE(MAX(`characters`.`online`), 0) AS `char_online`,
    SUM(`lb`.`score`) AS `total_score`,
    MAX(`lb`.`mythic_level`) AS `best_level`,
    COUNT(*) AS `runs`,
    MAX(`lb`.`last_update`) AS `last_update`
FROM `mythic_plus_leaderboard` `lb`
INNER JOIN `mythic_plus_season` `season` ON `season`.`id` = `lb`.`season_id`
LEFT JOIN `characters` ON `characters`.`guid` = `lb`.`char_guid`
GROUP BY
    `lb`.`season_id`,
    `season`.`label`,
    `season`.`year`,
    `season`.`month`,
    `lb`.`char_guid`;

CREATE VIEW `mythic_plus_web_leaderboard_map` AS
SELECT
    `lb`.`season_id`,
    `season`.`label` AS `season_label`,
    `season`.`year` AS `season_year`,
    `season`.`month` AS `season_month`,
    `lb`.`char_guid`,
    COALESCE(`characters`.`name`, `lb`.`char_name`) AS `char_name`,
    COALESCE(`characters`.`race`, 0) AS `char_race`,
    COALESCE(`characters`.`class`, 0) AS `char_class`,
    COALESCE(`characters`.`gender`, 0) AS `char_gender`,
    COALESCE(`characters`.`level`, 0) AS `char_level`,
    COALESCE(`characters`.`online`, 0) AS `char_online`,
    `lb`.`map_id`,
    `lb`.`difficulty`,
    `lb`.`mythic_level`,
    `lb`.`best_time`,
    `lb`.`deaths`,
    `lb`.`penalty_seconds`,
    `lb`.`completed_in_time`,
    `lb`.`score`,
    `lb`.`group_members`,
    `lb`.`last_update`
FROM `mythic_plus_leaderboard` `lb`
INNER JOIN `mythic_plus_season` `season` ON `season`.`id` = `lb`.`season_id`
LEFT JOIN `characters` ON `characters`.`guid` = `lb`.`char_guid`;

CREATE VIEW `mythic_plus_web_leaderboard_current_overall` AS
SELECT `overall`.*
FROM `mythic_plus_web_leaderboard_overall` `overall`
INNER JOIN `mythic_plus_season` `season` ON `season`.`id` = `overall`.`season_id`
WHERE `season`.`is_active` = 1;

CREATE VIEW `mythic_plus_web_leaderboard_current_map` AS
SELECT `map_entries`.*
FROM `mythic_plus_web_leaderboard_map` `map_entries`
INNER JOIN `mythic_plus_season` `season` ON `season`.`id` = `map_entries`.`season_id`
WHERE `season`.`is_active` = 1;

CREATE VIEW `mythic_plus_web_run_history` AS
SELECT
    `runs`.`instance_id`,
    `runs`.`map_id`,
    `runs`.`difficulty`,
    `runs`.`start_time`,
    `runs`.`end_time`,
    `runs`.`mythic_level`,
    `runs`.`time_limit`,
    `runs`.`total_time`,
    `runs`.`deaths`,
    `runs`.`penalty_seconds`,
    `runs`.`completed_in_time`,
    `runs`.`rewarded`,
    `runs`.`random_affix_count`,
    `runs`.`group_members`,
    `season`.`id` AS `season_id`,
    `season`.`label` AS `season_label`,
    `season`.`year` AS `season_year`,
    `season`.`month` AS `season_month`
FROM
(
    SELECT
        `snapshot`.`id` AS `instance_id`,
        `snapshot`.`map` AS `map_id`,
        `snapshot`.`mapdifficulty` AS `difficulty`,
        `snapshot`.`starttime` AS `start_time`,
        MAX(
            CASE
                WHEN `snapshot`.`creature_final_boss` = 1
                    THEN `snapshot`.`snaptime`
                ELSE 0
            END
        ) AS `end_time`,
        MAX(`snapshot`.`mythiclevel`) AS `mythic_level`,
        MAX(`snapshot`.`timelimit`) AS `time_limit`,
        GREATEST(
            MAX(
                CASE
                    WHEN `snapshot`.`creature_final_boss` = 1
                        THEN `snapshot`.`snaptime`
                    ELSE 0
                END
            ) - `snapshot`.`starttime`,
            0
        ) AS `total_time`,
        MAX(`snapshot`.`deaths`) AS `deaths`,
        MAX(`snapshot`.`penalty_on_death`) * MAX(`snapshot`.`deaths`)
            AS `penalty_seconds`,
        CASE
            WHEN MAX(
                CASE
                    WHEN `snapshot`.`creature_final_boss` = 1
                        THEN `snapshot`.`snaptime`
                    ELSE 0
                END
            ) > 0
            AND GREATEST(
                MAX(
                    CASE
                        WHEN `snapshot`.`creature_final_boss` = 1
                            THEN `snapshot`.`snaptime`
                        ELSE 0
                    END
                ) - `snapshot`.`starttime`,
                0
            ) <= MAX(`snapshot`.`timelimit`)
                THEN 1
            ELSE 0
        END AS `completed_in_time`,
        MAX(
            CASE
                WHEN `snapshot`.`creature_final_boss` = 1
                    THEN `snapshot`.`rewarded`
                ELSE 0
            END
        ) AS `rewarded`,
        MAX(`snapshot`.`random_affix_count`) AS `random_affix_count`,
        GROUP_CONCAT(DISTINCT `snapshot`.`char_name`
            ORDER BY `snapshot`.`char_name` SEPARATOR ', ') AS `group_members`
    FROM `mythic_plus_dungeon_snapshot` `snapshot`
    GROUP BY
        `snapshot`.`id`,
        `snapshot`.`map`,
        `snapshot`.`mapdifficulty`,
        `snapshot`.`starttime`
    HAVING `end_time` > 0
) `runs`
LEFT JOIN `mythic_plus_season` `season`
    ON `runs`.`end_time` >= `season`.`start_unix`
    AND `runs`.`end_time` < `season`.`end_unix`;

CREATE VIEW `mythic_plus_web_current_run_history` AS
SELECT `runs`.*
FROM `mythic_plus_web_run_history` `runs`
INNER JOIN `mythic_plus_season` `season` ON `season`.`id` = `runs`.`season_id`
WHERE `season`.`is_active` = 1;
