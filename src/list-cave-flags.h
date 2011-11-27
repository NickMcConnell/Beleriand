/* list-cave-flags.h - special grid flags
 *
 * Adjusting these flags does not break savefiles. Flags below start from 1
 * on line 11, so a flag's sequence number is its line number minus 10.
 *
 *
 */

/*  symbol     descr */
CAVE(NONE,     "")
CAVE(MARK,     "memorized feature")
CAVE(GLOW,     "self-illuminating")
CAVE(ICKY,     "part of a vault")
CAVE(ROOM,     "part of a room")
CAVE(SEEN,     "seen flag")
CAVE(VIEW,     "view flag")
CAVE(TEMP,     "temp flag")
CAVE(WALL,     "wall flag")
CAVE(DTRAP,    "trap detected grid")
CAVE(TRAP,     "grid containing a known trap")
CAVE(INVIS,    "grid containing an unknown trap")
CAVE(RIVER,    "part of a river")
CAVE(RANGE,    "part of a mountain range")
CAVE(LAKE,     "part of a lake")
CAVE(OCEAN,    "part of the ocean")
CAVE(MELIAN,   "contained within the Girdle of Melian")
CAVE(IMPASS,   "cannot contain anything")
CAVE(ROAD,     "part of a road")
