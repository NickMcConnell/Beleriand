/**
 * \file src/list-languages.h
 * \brief languages
 *
 * Changing language order or making new ones will break savefiles. Languages
 * below start from 0 on line 9, so a language's sequence number is its line
 * number minus 9.
 *
 * Note that the language name must match the name of the corresponding ability
 * in lib/gamedata/ability.txt.
 */
LANG(NONE,		"None")
LANG(ANIMAL,	"Tame Creature")
LANG(TALISKAN,	"Taliskan")
LANG(SINDARIN,	"Sindarin")
LANG(QUENYA,	"Quenya")
LANG(NANDORIN,	"Nandorin")
LANG(HALETHIAN,	"Speech of the Haladin")
LANG(KHUZDUL,	"Khuzdul")
LANG(AVARIN,	"Avarin")
