/**
 * \file list-square-flags.h
 * \brief special grid flags
 *
 * Adding flags to the end will not break savefiles (the added flags will be
 * read but not used when a savefile is loaded into an older version);
 * inserting into, deleting, or rearranging the existing flags will break
 * savefiles.  Flags below start from 1 on line 14, so a flag's sequence
 * number is its line number minus 13.
 */

/*  symbol          descr */
SQUARE(NONE,		"")
SQUARE(MARK,		"memorized feature")
SQUARE(GLOW,		"self-illuminating")
SQUARE(VAULT,		"part of a vault")
SQUARE(G_VAULT,		"part of a greater vault")
SQUARE(ROOM,		"part of a room")
SQUARE(SEEN,		"seen flag")
SQUARE(VIEW,		"view flag")
SQUARE(WASSEEN,		"previously seen (during update)")
SQUARE(FEEL,		"hidden points to trigger feelings")
SQUARE(TRAP,		"square containing a known trap")
SQUARE(INVIS,		"square containing an unknown trap")
SQUARE(WALL_INNER,	"inner wall generation flag")
SQUARE(WALL_OUTER,	"outer wall generation flag")
SQUARE(WALL_SOLID,	"solid wall generation flag")
SQUARE(CHASM,		"chasm generation flag")
SQUARE(PROJECT,		"marked for projection processing")
SQUARE(HIDDEN,		"a trap that looks like the normal floor")
SQUARE(TEMP,		"temporary flag")
SQUARE(WALL,		"wall flag")
SQUARE(FIRE,		"is in line of fire")
SQUARE(CLOSE_PLAYER,"square is seen and in player's light radius")
SQUARE(RIVER_N,		"square is part of a river flowing north")
SQUARE(RIVER_NE,	"square is part of a river flowing north east")
SQUARE(RIVER_E,		"square is part of a river flowing east")
SQUARE(RIVER_SE,	"square is part of a river flowing south east")
SQUARE(RIVER_S,		"square is part of a river flowing south")
SQUARE(RIVER_SW,	"square is part of a river flowing south west")
SQUARE(RIVER_W,		"square is part of a river flowing west")
SQUARE(RIVER_NW,	"square is part of a river flowing north west")
SQUARE(ROAD,		"square is part of a road")
