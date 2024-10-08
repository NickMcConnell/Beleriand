/**
 * \file list-terrain-flags.h
 * \brief monster race blow effects
 *
 * Adjusting these flags does not break savefiles. Flags below start from 1
 * on line 13, so a flag's sequence number is its line number minus 12.
 *
 *
 */

/*  symbol     descr */
TF(NONE,        "")
TF(LOS,        "Allows line of sight")
TF(PROJECT,    "Allows projections to pass through")
TF(PASSABLE,   "Can be passed through by all creatures")
TF(INTERESTING,"Is noticed on looking around")
TF(PERMANENT,  "Is permanent")
TF(EASY,       "Is easily passed through")
TF(TRAP,       "Can hold a trap")
TF(NO_SCENT,   "Cannot store scent")
TF(NO_FLOW,    "No flow through")
TF(OBJECT,     "Can hold objects")
TF(TORCH,      "Becomes bright when torch-lit")
TF(HIDDEN,     "Can be found by searching")
TF(GOLD,       "Contains treasure")
TF(CLOSABLE,   "Can be closed")
TF(FLOOR,      "Is a clear floor")
TF(WALL,       "Is a solid wall")
TF(ROCK,       "Is rocky")
TF(GRANITE,    "Is a granite rock wall")
TF(DOOR_ANY,   "Is any door")
TF(DOOR_CLOSED,"Is a closed door")
TF(FORGE,      "Is a forge")
TF(DOOR_JAMMED,"Is a jammed door")
TF(DOOR_LOCKED,"Is a locked door")
TF(QUARTZ,     "Is a quartz seam")
TF(STAIR,      "Is a stair")
TF(SHAFT,      "Is a shaft")
TF(UPSTAIR,    "Is an up staircase")
TF(DOWNSTAIR,  "Is a down staircase")
TF(PIT,        "Is a pit")
TF(CHASM,      "Is a chasm")
TF(SMOOTH,     "Should have smooth boundaries")
TF(TREE,       "Is a tree")
TF(HIDE_OBJ,   "Hides objects")
TF(ORGANIC,    "Is growing")
TF(FREEZE,     "Can freeze")
TF(WATERY,     "Is water based")
TF(SWIM,       "Can only be crossed by swimming")
TF(ICY,        "Is ice based")
TF(PROTECT,    "Makes the occupant harder to hit")
TF(EXPOSE,     "Makes the occupant easier to hit")
TF(RUN1,       "First choice for running")
TF(RUN2,       "Second choice for running")
TF(MTRAP,      "Can hold a monster trap")
