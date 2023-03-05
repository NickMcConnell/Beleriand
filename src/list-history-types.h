/**
 * \file list-history-types.h
 * \brief History message types
 */
/* index				description */
HIST(NONE,				"")
HIST(PLAYER_BIRTH,		"Player was born")
HIST(CHALLENGE_OPTS,	"Player challenge option")
HIST(ARTIFACT_UNKNOWN,	"Player found but not IDd an artifact")
HIST(ARTIFACT_KNOWN,	"Player has IDed an artifact")
HIST(ARTIFACT_LOST,		"Player had an artifact and lost it")
HIST(VAULT_ENTERED,		"Player has entered a greater vault")
HIST(VAULT_LOST,		"Player has failed to enter a known greater vault")
HIST(FORGE_FOUND,		"Player has found the unique forge")
HIST(OBJECT_SMITHED,	"Player created an object")
HIST(FELL_DOWN_LEVEL,	"Player fell through the floor")
HIST(TRAPPED_STAIRS,	"Player fell through trapped stairs")
HIST(FELL_IN_CHASM,		"Player fell in a chasm")
HIST(PLAYER_DEATH,		"Player has been slain")
HIST(SLAY_UNIQUE,		"Player has slain a unique monster")
HIST(MEET_UNIQUE,		"Player has encountered a unique monster")
HIST(USER_INPUT,		"User-added note")
HIST(SAVEFILE_IMPORT,	"Added when an older version savefile is imported")
HIST(GAIN_LEVEL,		"Player gained a level")
HIST(SILMARIL,			"Player gained a silmaril")
HIST(ESCAPE,			"Player escaped Angband")
HIST(GENERIC,			"Anything else not covered here (unused)")
