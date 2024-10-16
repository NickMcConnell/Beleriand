/**
 * \file list-rooms.h
 * \brief matches dungeon room names to their building functions
 *
 * Fields:
 * name: name as appears in edit files
 * rows: Maximum number of rows (for vaults)
 * cols: Maximum number of columns (for vaults)
 * builder: name of room building function (with build_ prepended)
 */

/* name						rows	cols	builder */
ROOM("staircase room",		0,		0,		staircase)
ROOM("simple room",			0,		0,		simple)
ROOM("crossed room",		0,		0,		crossed)
ROOM("circular room",		0,		0,		circular)
ROOM("overlap room",		0,		0,		overlap)
ROOM("Interesting room",	22,		33,		interesting)
ROOM("Lesser vault",		22,		33,		lesser_vault)
ROOM("Greater vault",		44,		66,		greater_vault)
ROOM("Throne room",			30,		35,		throne)

