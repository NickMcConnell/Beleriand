/**
 * \file list-object-modifiers.h
 * \brief object modifiers (plusses and minuses) for all objects
 *
 * Changing modifier order will break savefiles. Modifiers below start from
 * 13 on line 12 (stats and skills count as modifiers, and are included from
 * list-stats.h and list-skills.h), so a modifier's sequence number is its line
 * number plus 1.
 *
 * Modifier properties are defined in lib/gamedata/object_property.txt
 */
OBJ_MOD(DAMAGE_SIDES)
OBJ_MOD(TUNNEL)
