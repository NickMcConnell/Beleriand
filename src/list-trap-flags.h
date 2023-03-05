/**
 * \file list-trap-flags.h
 * \brief trap properties
 *
 * Adjusting these flags does not break savefiles. Flags below start from 1
 * on line 13, so a flag's sequence number is its line number minus 12.
 *
 *
 */

/*  symbol		descr */
TRF(NONE,		"")
TRF(GLYPH,		"Is a glyph")
TRF(TRAP,		"Is a player trap")
TRF(VISIBLE,	"Is visible")
TRF(INVISIBLE,	"Is invisible")
TRF(FLOOR,		"Can be set on a floor")
TRF(SURFACE,	"Can be set outside the dungeon")
TRF(DOWN,		"Takes the player down a level")
TRF(PIT,		"Moves the player onto the trap")
TRF(ONETIME,	"Disappears after being activated")
TRF(SAVE_SKILL,	"Allows a save from all effects by a skill check")
TRF(LOCK,		"Is a door lock")
TRF(JAM,		"Tells how jammed a door is")
TRF(FORGE_USE,	"Tells how many uses a forge has")
TRF(DELAY,      "Has a delayed effect")
