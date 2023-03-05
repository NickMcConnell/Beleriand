/**
 * \file src/list-skills.h
 * \brief player skills
 *
 * Changing skill order or making new ones will break savefiles. Skills
 * below start from 0 on line 11, so a skill's sequence number is its line
 * number minus 11.
 *
 * Skill properties are not defined in lib/gamedata/object_property.txt
 */
SKILL(MELEE,		"Melee")
SKILL(ARCHERY,		"Archery")
SKILL(EVASION,		"Evasion")
SKILL(STEALTH,		"Stealth")
SKILL(PERCEPTION,	"Perception")
SKILL(WILL,			"Will")
SKILL(SMITHING,		"Smithing")
SKILL(SONG,			"Song")
