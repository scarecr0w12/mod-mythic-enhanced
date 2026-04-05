#!/usr/bin/env bash

## GETS THE CURRENT MODULE ROOT DIRECTORY
MOD_MYTHIC_ENHANCED_ROOT="$( cd "$( dirname "${BASH_SOURCE[0]}" )/" && pwd )"

## CUSTOM SQL - Important file used by the db assembler.
## Keep only the required variables (base sql files or updates, depending on the DB).

## BASE SQL

DB_CHARACTERS_CUSTOM_PATHS+=(
	"$MOD_MYTHIC_ENHANCED_ROOT/data/sql/db-characters/base/"
)

DB_WORLD_CUSTOM_PATHS+=(
	"$MOD_MYTHIC_ENHANCED_ROOT/data/sql/db-world/base/"
)

## UPDATES

DB_CHARACTERS_UPDATE_PATHS+=(
	"$MOD_MYTHIC_ENHANCED_ROOT/data/sql/db-characters/updates/"
)

DB_WORLD_UPDATE_PATHS+=(
	"$MOD_MYTHIC_ENHANCED_ROOT/data/sql/db-world/updates/"
)
