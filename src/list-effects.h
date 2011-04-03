/*
 * File: list-effects.h
 * Purpose: List of effect types
 */

/*
 * "rating" is the power rating for an item activation, as a damage-per-blow
 * equivalent (x2). These ratings are used in the calculation of the power (and
 * therefore cost) of an item which has the effect as an activation, but NOT
 * for other items (e.g. potions, scrolls). Hence the use of INIHIBIT_POWER.
 */

/*     name            aim?   rating	short description	*/
EFFECT(XXX,            FALSE,  0,	NULL, 0, 0)
EFFECT(POISON1,        FALSE,  0,	"poisons you for 12-20 turns", 0, 0)
EFFECT(POISON2,        FALSE,  0,	"poisons you for 20-30 turns", 0, 0)
EFFECT(BLIND1,         FALSE,  0,	"blinds you for 1d200+200 turns", 0, 0)
EFFECT(BLIND2,         FALSE,  0,	"blinds you for 1d100+100 turns", 0, 0)
EFFECT(SCARE,          FALSE,  0,	"induces fear in you for 1d10+10 turns", 0, 0)
EFFECT(CONFUSE1,       FALSE,  0,	"confuses you for 1d10+10 turns", 0, 0)
EFFECT(CONFUSE2,       FALSE,  0,	"confuses you for 1d20+15 turns", 0, 0)
EFFECT(HALLUC,         FALSE,  0,	"causes you to hallucinate for 1d250 + 250 turns", 0, 0)
EFFECT(PARALYSE,       FALSE,  0,	"induces paralysis for 1d10+10 turns", 0, 0)
EFFECT(WEAKNESS,       FALSE,  0,	"reduces your strength with damage 6d6", 0, 0)
EFFECT(SICKNESS,       FALSE,  0,	"reduces your constitution with damage 6d6", 0, 0)
EFFECT(STUPIDITY,      FALSE,  0,	"reduces your intelligence with damage 8d8", 0, 0)
EFFECT(NAIVETY,        FALSE,  0,	"reduces your wisdom with damage 8d8", 0, 0)
EFFECT(UNHEALTH,       FALSE,  0,	"reduces your constitution with damage 10d10", 0, 0)
EFFECT(DISEASE,        FALSE,  0,	"reduces your strength with damage 10d10", 0, 0)
EFFECT(DETONATIONS,    FALSE,  0,	"inflicts 50d20 points of damage, severe cuts, and stunning", 0, 0)
EFFECT(DEATH,          FALSE,  0,	"reduces your hitpoints to zero", 0, 0)
EFFECT(SLOWNESS1,      FALSE,  0,	"slows you for 1d25+15 turns", 0, 0)
EFFECT(SALT_WATER,     FALSE,  0,	"induces vomiting and paralysis for 4 turns, resulting in severe hunger but also curing poison", 0, 0)
EFFECT(SLEEP,          FALSE,  0,	"induces paralysis for 1d4+4 turns", 0, 0)
EFFECT(LOSE_MEMORIES,  FALSE,  0,	"drains a quarter of your experience", 0, 0)
EFFECT(RUINATION,      FALSE,  0,	"inflicts 5d10 points of damage and decreases all your stats", 0, 0)
EFFECT(DEC_STR,        FALSE,  0,	"reduces your strength", 0, 0)
EFFECT(DEC_INT,        FALSE,  0,	"reduces your intelligence", 0, 0)
EFFECT(DEC_WIS,        FALSE,  0,	"reduces your wisdom", 0, 0)
EFFECT(DEC_DEX,        FALSE,  0,	"reduces your dexterity", 0, 0)
EFFECT(DEC_CON,        FALSE,  0,	"reduces your constitution", 0, 0)
EFFECT(DEC_CHR,        FALSE,  0,	"reduces your charisma", 0, 0)
EFFECT(AGGRAVATE,      FALSE,  0,	"awakens all nearby sleeping monsters", 0, 0)
EFFECT(CURSE_ARMOR,    FALSE,  0,	"curses your currently worn body armor", 0, 0)
EFFECT(CURSE_WEAPON,   FALSE,  0,	"curses your currently wielded melee weapon", 0, 0)
EFFECT(SUMMON3,        FALSE,  0,	"summons monsters at the current dungeon level", 0, 0)
EFFECT(SUMMON_UNDEAD,  FALSE,  0,	"summons undead monsters at the current dungeon level", 0, 0)
EFFECT(TRAP_CREATION,  FALSE,  0,	"creates traps surrounding you", 0, 0)
EFFECT(DARKNESS,       FALSE,  0,	"darkens the nearby area and blinds you for 1d5+3 turns", 0, 0) 
EFFECT(SLOWNESS2,      FALSE,  0,	"slows you for 1d30+15 turns", 0, 0)
EFFECT(HASTE_MONSTERS, FALSE,  0,	"hastes all monsters within line of sight", 0, 0)
EFFECT(SUMMON4,        FALSE,  0,	"summons monsters at the current dungeon level", 0, 0)
EFFECT(HEAL_MONSTER,   TRUE,   0,	"heals a single monster 4d6 hit points", 0, 0)
EFFECT(HASTE_MONSTER,  TRUE,   0,	"hastes a single monster", 0, 0)
EFFECT(CLONE_MONSTER,  TRUE,   0,	"hastes, heals, and magically duplicates a single monster", 0, 0)
EFFECT(ROUSE_LEVEL,    FALSE,  0,	"awakens and hastes every monster on the level", 0, 0)
EFFECT(CURE_POISON,    FALSE,  1,	"neutralizes poison", 0, 0)
EFFECT(CURE_BLINDNESS, FALSE,  4,	"cures blindness", 0, 0)
EFFECT(CURE_PARANOIA,  FALSE,  2,	"removes your fear", 0, 0)
EFFECT(CURE_CONFUSION, FALSE,  4,	"cures confusion", 0, 0)
EFFECT(CURE_SMALL,     FALSE,  3,	"restores 4d8 hitpoints", 0, 0)
EFFECT(RES_STR,        FALSE,  10,	"restores your strength", 0, 0)
EFFECT(RES_INT,        FALSE,  8,	"restores your intelligence", 0, 0)
EFFECT(RES_WIS,        FALSE,  8,	"restores your wisdom", 0, 0)
EFFECT(RES_DEX,        FALSE,  9,	"restores your dexterity", 0, 0)
EFFECT(RES_CON,        FALSE,  10,	"restores your constitution", 0, 0)
EFFECT(RES_CHR,        FALSE,  4,	"restores your charisma", 0, 0)
EFFECT(RESTORING,      FALSE,  15,	"restores all your stats", 0, 0)
EFFECT(ATHELAS,        FALSE,  15,	"heals you of poison, stunning, cuts and the Black Breath", 0, 0)
EFFECT(BEORNING,       FALSE,  3,	"restores 5d8 hitpoints", 0, 0)
EFFECT(FOOD_GOOD,      FALSE,  0,	NULL, 0, 0)
EFFECT(WAYBREAD,       FALSE,  5,	"restores 5d10 hit points and halves poison", 0, 0)
EFFECT(DRINK_GOOD,     FALSE,  0,	NULL, 0, 0) 
EFFECT(CURE_LIGHT,     FALSE,  3,	"restores 2d10 hitpoints, heals a little cut damage and cures blindness", 0, 0)
EFFECT(CURE_SERIOUS,   FALSE,  6,	"restores 4d10 hitpoints, heals some cut damage, and cures blindness and confusion", 0, 0)
EFFECT(CURE_CRITICAL,  FALSE,  10,	"restores 6d10 hitpoints, heals considerable cut damage, reduces poisoning by over a half, and cures stunning, blindness, and confusion", 0, 0)
EFFECT(HEALING1,       FALSE,  11,	"restores 300 hitpoints and cures cuts and stunning", 0, 0)
EFFECT(HEALING2,       FALSE,  12,	"restores 500 hitpoints and cures cuts and stunning", 0, 0)
EFFECT(HEALING3,       FALSE,  12,	"restores 300 hitpoints, considerably reduces poison and heals cut damage, stunning, blindness, and confusion", 0, 0)
EFFECT(HEALING4,       FALSE,  18,	"restores 600 hit points, heals cut damage, and cures stunning, poisoning, blindness, confusion and the Black Breath", 0, 0)
EFFECT(LIFE,           FALSE,  22,	"restores 2000 hit points, restores experience and stats, heals cut damage, and cures stunning, poison, blindness, confusion, hallucination and the Black Breath", 0, 0)
EFFECT(INFRAVISION,    FALSE,  4,	"extends your infravision by 50 feet for 1d100+100 turns", 0, 0)
EFFECT(SEE_INVIS,      FALSE,  5,	"allows you to see invisible things for 1d12+12 turns", 0, 0)
EFFECT(SLOW_POISON,    FALSE,  1,	"halves poisoning", 0, 0)
EFFECT(SPEED1,         FALSE,  9,	"hastens you for 1d20+15 turns", 0, 0)
EFFECT(RES_HEAT_COLD,  FALSE,  7,	"grants temporary resistance to fire and cold for 1d30+20 turns", 0, 0)
EFFECT(RES_ACID_ELEC,  FALSE,  7,	"grants temporary resistance to acid and electricity for 1d30+20 turns", 0, 0)
EFFECT(RESIST_ALL,     FALSE,  10,	"grants temporary resistance to acid, electricity, fire, cold and poison for 1d25+15 turns", 0, 0)
EFFECT(HEROISM,        FALSE,  7,	"restores 10 hit points, removes fear and grants you resistance to fear, +10 to-skill and +5 to-deadliness for 1d25+25 turns", 0, 0)
EFFECT(BERSERK_STR,    FALSE,  9,	"puts you in a berserker rage for d25+25 turns", 0, 0)
EFFECT(RESTORE_MANA,   FALSE,  20,	"restores your mana points to maximum", 0, 0)
EFFECT(RESTORE_EXP,    FALSE,  8,	"restores your experience", 0, 0)
EFFECT(INC_STR,        FALSE,  INHIBIT_POWER,	"restores and increases your strength up to a point", 0, 0)
EFFECT(INC_INT,        FALSE,  INHIBIT_POWER,	"restores and increases your intelligence up to a point", 0, 0)
EFFECT(INC_WIS,        FALSE,  INHIBIT_POWER,	"restores and increases your wisdom up to a point", 0, 0)
EFFECT(INC_DEX,        FALSE,  INHIBIT_POWER,	"restores and increases your dexterity up to a point", 0, 0)
EFFECT(INC_CON,        FALSE,  INHIBIT_POWER,	"restores and increases your constitution up to a point", 0, 0)
EFFECT(INC_CHR,        FALSE,  INHIBIT_POWER,	"restores and increases your charisma up to a point", 0, 0)
EFFECT(STAR_INC_STR,   FALSE,  INHIBIT_POWER,	"restores and increases your strength", 0, 0)
EFFECT(STAR_INC_INT,   FALSE,  INHIBIT_POWER,	"restores and increases your intelligence", 0, 0)
EFFECT(STAR_INC_WIS,   FALSE,  INHIBIT_POWER,	"restores and increases your wisdom", 0, 0)
EFFECT(STAR_INC_DEX,   FALSE,  INHIBIT_POWER,	"restores and increases your dexterity", 0, 0)
EFFECT(STAR_INC_CON,   FALSE,  INHIBIT_POWER,	"restores and increases your constitution", 0, 0)
EFFECT(STAR_INC_CHR,   FALSE,  INHIBIT_POWER,	"restores and increases your charisma", 0, 0)
EFFECT(AUGMENTATION,   FALSE,  INHIBIT_POWER,	"restores and increases all your stats", 0, 0)
EFFECT(ENLIGHTENMENT1, FALSE,  22,	"completely lights up and magically maps the level", 0, 0)
EFFECT(ENLIGHTENMENT2, FALSE,  INHIBIT_POWER,	"increases your intelligence and wisdom, detects and maps everything in the surrounding area, and identifies all items in your pack", 0, 0)
EFFECT(EXPERIENCE,     FALSE,  INHIBIT_POWER,	"grants half your current experience points, to a maximum of 100,000", 0, 0)
EFFECT(VAMPIRE,        FALSE,  5,	"changes you into a vampire, dealing 3d6 damage; holy and natural casters do not change and take 10d6 damage", 0, 0)
EFFECT(PHASE_DOOR,     FALSE,  5,	"teleports you randomly up to 10 squares away", 0, 0)
EFFECT(TELEPORT100,    FALSE,  6,	"teleports you randomly up to 100 squares away", 0, 0)
EFFECT(TELEPORT_LEVEL, FALSE,  15,	"teleports you one level up or down", 0, 0)
EFFECT(RECALL,         FALSE,  15,	"returns you to a recall point or takes you to a recall point after a short delay", 0, 0)
EFFECT(IDENTIFY,       FALSE,  9,	"reveals to you the extent of an item's magical powers", 0, 0)
EFFECT(REVEAL_CURSES,  FALSE,  15,	"reveals to you the extent of an item's magical powers and curses", 0, 0)
EFFECT(BRANDING,       FALSE,  20,	"brands a stack of missiles", 0, 0)
EFFECT(FRIGHTENING,    TRUE,   3,	"attempts to induce magical fear in a single monster", 0, 0)
EFFECT(REMOVE_CURSE,   FALSE,  10,	"attempts to remove curses from an item", 0, 0)
EFFECT(REM_CURSE_GOOD, FALSE,  15,	"makes multiple attempts to remove curses from an item", 0, 0)
EFFECT(ENCHANT_ARMOR1, FALSE,  12,	"attempts to magically enhance a piece of armour", 0, 0)
EFFECT(ENCHANT_ARMOR2, FALSE,  15,	"attempts to magically enhance a piece of armour with high chance of success", 0, 0)
EFFECT(ENCHANT_TO_HIT, FALSE,  20,	"attempts to magically enhance a weapon's to-skill bonus", 0, 0)
EFFECT(ENCHANT_TO_DAM, FALSE,  20,	"attempts to magically enhance a weapon's to-deadliness bonus", 0, 0)
EFFECT(ENCHANT_WEAPON, FALSE,  25,	"attempts to magically enhance a weapon both to-skill and to-deadliness", 0, 0)
EFFECT(RECHARGING1,    FALSE,  12,	"tries to recharge a wand or staff, destroying the wand or staff on failure", 0, 0)
EFFECT(RECHARGING2,    FALSE,  15,	"tries to recharge a wand or staff, destroying the wand or staff on failure", 0, 0)
EFFECT(LIGHT,          FALSE,  4,	"lights up an area and inflicts 2d8 damage on light-sensitive creatures", 0, 0)
EFFECT(MAPPING,        FALSE,  10,	"maps the area around you", 0, 0)
EFFECT(DETECT_GOLD,    FALSE,  3,	"detects gold and precious minerals nearby", 0, 0)
EFFECT(DETECT_ITEM,    FALSE,  6,	"detects objects nearby", 0, 0)
EFFECT(DETECT_TRAP,    FALSE,  6,	"detects traps nearby", 0, 0)
EFFECT(DETECT_DOOR,    FALSE,  6,	"detects doors and stairs nearby", 0, 0)
EFFECT(DETECT_INVIS,   FALSE,  6,	"detects invisible creatures nearby", 0, 0)
EFFECT(SATISFY_HUNGER, FALSE,  7,	"magically renders you well-fed", 0, 0)
EFFECT(BLESSING1,      FALSE,  6,	"increases your AC and to-hit bonus for 1d12+6 turns", 0, 0)
EFFECT(BLESSING2,      FALSE,  7,	"increases your AC and to-hit bonus for 1d24+12 turns", 0, 0)
EFFECT(BLESSING3,      FALSE,  8,	"increases your AC and to-hit bonus for 1d48+24 turns", 0, 0)
EFFECT(MONSTER_CONFU,  FALSE,  8,	"causes your next attack upon a monster to confuse it", 0, 0)
EFFECT(PROT_FROM_EVIL, FALSE,  6,	"grants you protection from evil for 1d25 plus 3 times your character level turns", 0, 0)
EFFECT(RUNE_PROTECT,   FALSE,  20,	"inscribes a glyph of warding beneath you, which monsters cannot move onto", 0, 0)
EFFECT(DOOR_DESTRUCT,  FALSE,  4,	"destroys adjacent doors", 0, 0)
EFFECT(DESTRUCTION,    FALSE,  20,	"destroys an area around you in the shape of a circle radius 15, and blinds you for 1d10+10 turns", 0, 0)
EFFECT(DISPEL_UNDEAD,  FALSE,  4,	"deals 60 damage to all undead creatures that you can see", 0, 0)
EFFECT(GENOCIDE,       FALSE,  20,	"removes all non-unique monsters represented by a chosen symbol from the level, dealing you damage in the process", 0, 0)
EFFECT(MASS_GENOCIDE,  FALSE,  25,	"removes all non-unique monsters within 20 squares, dealing you damage in the process", 0, 0)
EFFECT(ACQUIREMENT1,   FALSE,  INHIBIT_POWER,	"creates a good object nearby", 0, 0)
EFFECT(ACQUIREMENT2,   FALSE,  INHIBIT_POWER,	"creates a few good items nearby", 0, 0)
EFFECT(ELE_ATTACKS,    FALSE,  20,	"temporarily brands your weapon", 0, 0)
EFFECT(ACID_PROOF,     FALSE,  20,	"renders an item invulnerable to acid", 0, 0)
EFFECT(ELEC_PROOF,     FALSE,  20,	"renders an item invulnerable to electricity", 0, 0)
EFFECT(FIRE_PROOF,     FALSE,  20,	"renders an item invulnerable to fire", 0, 0)
EFFECT(COLD_PROOF,     FALSE,  20,	"renders an item invulnerable to cold", 0, 0)
EFFECT(STARLIGHT,      FALSE,  6,	"fires a line of light in all directions, each one hurting light-sensitive creatures (damage: 20 - 60+)", 0, 0)
EFFECT(DETECT_EVIL,    FALSE,  6,	"detects evil creatures nearby", 0, 0)
EFFECT(CURE_MEDIUM,    FALSE,  5,	"restores 10 - 30 hitpoints", 0, 0)
EFFECT(CURING,         FALSE,  9,	"heals cut damage, and cures all stunning, poison, blindness and confusion", 0, 0)
EFFECT(BANISHMENT,     FALSE,  20,	"teleports all evil monsters in your line of sight 80+ squares", 0, 0)
EFFECT(SLEEP_MONSTERS, FALSE,  8,	"tries to sleep all creatures within line of sight", 0, 0)
EFFECT(SLOW_MONSTERS,  FALSE,  7,	"tries to slow all monsters within line of sight", 0, 0)
EFFECT(SPEED2,         FALSE,  12,	"hastens you for 1d30+15 turns", 0, 0)
EFFECT(PROBING,        FALSE,  8,	"gives you information on the health and abilities of monsters you can see", 0, 0)
EFFECT(DISPEL_EVIL,    FALSE,  6,	"deals 60 damage to all evil creatures that you can see", 0, 0)
EFFECT(POWER,          FALSE,  10,	"deals 100 damage to all creatures that you can see", 0, 0)
EFFECT(HOLINESS,       FALSE,  12,	"inflicts damage on evil creatures you can see, cures 50 hit points, heals all temporary effects and grants you protection from evil (damage: 120)", 0, 0)
EFFECT(EARTHQUAKES,    FALSE,  5,	"causes an earthquake around you", 0, 0)
EFFECT(DETECTION,      FALSE,  8,	"detects treasure, traps, doors, stairs, and all creatures nearby", 0, 0)
EFFECT(MSTORM,         FALSE,  15,	"unleashes a blast of pure damage (125 - 200) around you", 0, 0)
EFFECT(STARBURST,      FALSE,  13,	"unleashes a blast of light (damage: 100 - 167) around you", 0, 0)
EFFECT(MASS_CONFU,     FALSE,  12,	"powerfully attempts to confuse all creatures within line of sight", 0, 0)
EFFECT(STARFIRE,       FALSE,  INHIBIT_POWER,	"fires several balls of confusing light (damage 30 - 90+ each) nearby", 0, 0)
EFFECT(WINDS,          FALSE,  INHIBIT_POWER,	"catches all nearby monsters in a whirlwind (damage: 100 - 200)", 0, 0)
EFFECT(HOLDING,     FALSE,  INHIBIT_POWER,	"attempts to put all undead in line of sight into stasis", 0, 0)
EFFECT(KELVAR,         FALSE,  INHIBIT_POWER,	"if any natural creatures are on the level, maps around them and detects all traps", 0, 0)
EFFECT(TELEPORT_AWAY2, TRUE,   12,	"teleports a target monster away (distance: 55-80)", 0, 0)
EFFECT(DISARMING,      TRUE,   6,	"destroys an adjacent trap, failure sets off the trap (success chance: 95%); adjacent doors are found and unlocked", 0, 0)
EFFECT(DOOR_DEST,      TRUE,   4,	"destroys an adjacent door", 0, 0)
EFFECT(STONE_TO_MUD,   TRUE,   6,	"turns rock into mud", 0, 0)
EFFECT(LIGHT_LINE,     TRUE,   6,	"lights up part of the dungeon in a straight line (damage: 4 - 20)", 0, 0)
EFFECT(SLEEP_MONSTER2, TRUE,   3,	"attempts to induce magical sleep in a single monster", 0, 0)
EFFECT(SLOW_MONSTER2,  TRUE,   3,	"attempts to magically slow a single monster", 0, 0)
EFFECT(CONFUSE_MONSTER, TRUE,  3,	"confuses a target monster", 0, 0)
EFFECT(FEAR_MONSTER,   TRUE,   3,	"attempts to induce magical fear in a single monster", 0, 0)
EFFECT(DRAIN_LIFE1,    TRUE,   10,	"drains life from a target creature (damage: 50 - 100)", 0, 0)
EFFECT(POLYMORPH,      TRUE,   7,	"polymorphs a monster into another kind of creature", 0, 0)
EFFECT(STINKING_CLOUD, TRUE,   3,	"fires a stinking cloud (damage: 12)", 0, 0)
EFFECT(MAGIC_MISSILE,  TRUE,   3,	"fires a magic missile (damage: 2 - 12)", 0, 0)
EFFECT(ACID_BOLT1,     TRUE,   4,	"creates an acid bolt (damage: 5 - 80)", 0, 0)
EFFECT(ELEC_BOLT1,     TRUE,   4,	"creates a lightning bolt (damage: 3 - 48)", 0, 0)
EFFECT(FIRE_BOLT1,     TRUE,   4,	"creates a fire bolt (damage: 6 - 98)", 0, 0)
EFFECT(COLD_BOLT1,     TRUE,   4,	"creates a frost bolt (damage: 4 - 64)", 0, 0)
EFFECT(ACID_BALL1,     TRUE,   9,	"creates an acid ball (damage: 60 - 90)", 0, 0)
EFFECT(ELEC_BALL1,     TRUE,   9,	"creates a lightning ball (damage: 40 - 70)", 0, 0)
EFFECT(FIRE_BALL1,     TRUE,   9,	"creates a fire ball (damage: 70 - 100)", 0, 0)
EFFECT(COLD_BALL1,     TRUE,   9,	"creates a frost ball (damage: 50 - 80)", 0, 0)
EFFECT(WONDER,         TRUE,   9,	"creates random and unpredictable effects", 0, 0)
EFFECT(DRAGON_FIRE,    TRUE,   15,	"creates an arc of fire (damage: 160)", 0, 0)
EFFECT(DRAGON_COLD,    TRUE,   15,	"creates an arc of cold (damage: 160)", 0, 0)
EFFECT(DRAGON_BREATH,  TRUE,   20,	"creates an arc of a random basic element or poison (damage: 180 - 210)", 0, 0)
EFFECT(ANNIHILATION,   TRUE,   12,	"drains life from a target creature (damage: 100 - 300)", 0, 0)
EFFECT(STRIKING,       TRUE,   15,	"fires a meteor (damage: 10 - 234)", 0, 0)
EFFECT(STORMS,         TRUE,   13,	"fires a bolt of storm lightning (damage: 35 - 140)", 0, 0)
EFFECT(SHARD_BOLT,     TRUE,   8,	"fires a bolt of shards (damage: 4 - 56)", 0, 0)
EFFECT(ILKORIN,        TRUE,   INHIBIT_POWER,	"fires a bolt of poison and then produces a large cloud of poison (damage ~30 - 310.  hurt 30.)", 0, 0)
EFFECT(BEGUILING,      TRUE,   INHIBIT_POWER,	"powerfully attempts to slow, confuse and sleep a target creature", 0, 0)
EFFECT(UNMAKING,       TRUE,   INHIBIT_POWER,	"creates various chaotic effects, some of them dangerous to the player", 0, 0)
EFFECT(OSSE,           TRUE,   INHIBIT_POWER,	"creates a massive tidal wave (damage: 3 * plev + d100)", 0, 0)
EFFECT(RESTORATION,    FALSE,  18,	"restores all your stats and your experience points", 0, 0)
EFFECT(TELEPORT_AWAY1, TRUE,   10,	"teleports a target monster away (distance: 45-60+)", 0, 0)
EFFECT(SLEEP_MONSTER1, TRUE,   3,	"attempts to induce magical sleep in a single monster", 0, 0)
EFFECT(SLOW_MONSTER1,  TRUE,   3,	"attempts to magically slow a single monster", 0, 0)
EFFECT(DRAIN_LIFE2,    TRUE,   8,	"drains life from a target creature (damage: 45 - 120)", 0, 0)
EFFECT(ACID_BOLT2,     TRUE,   5,	"creates an acid bolt (damage: 6 - 88)", 0, 0)
EFFECT(ELEC_BOLT2,     TRUE,   5,	"creates a lightning bolt (damage: 4 - 56)", 0, 0)
EFFECT(FIRE_BOLT2,     TRUE,   5,	"creates a fire bolt (damage: 7 - 104)", 0, 0)
EFFECT(COLD_BOLT2,     TRUE,   5,	"creates a frost bolt (damage: 5 - 72)", 0, 0)
EFFECT(ACID_BALL2,     TRUE,   10,	"creates an acid ball (damage: 60 - 100)", 0, 0)
EFFECT(ELEC_BALL2,     TRUE,   10,	"creates a lightning ball (damage: 40 - 80)", 0, 0)
EFFECT(FIRE_BALL2,     TRUE,   10,	"creates a fire ball (damage: 70 - 110)", 0, 0)
EFFECT(COLD_BALL2,     TRUE,   10,	"creates a frost ball (damage: 50 - 90)", 0, 0)
EFFECT(LIGHTINGSTRIKE, TRUE,   20,	"fires a powerful lightning bolt (damage: 18 - 272)", 0, 0)
EFFECT(NORTHWINDS,     TRUE,   20,	"creates a breath of cold north wind (damage: 21 - 296)", 0, 0)
EFFECT(DRAGONFIRE,     TRUE,   20,	"creates a powerful fire bolt (damage: 24 - 320)", 0, 0)
EFFECT(GLAURUNGS,      TRUE,   20,	"creates a sizzling bolt of acid (damage: 27 - 344)", 0, 0)
EFFECT(DELVING,        TRUE,   INHIBIT_POWER,	"creates a room if fired straight down, or else powerful stone to mud (damage 160 - 400)", 0, 0)
EFFECT(SHADOW,         FALSE,  INHIBIT_POWER,	"allows you to slip into the shadows", 0, 0)
EFFECT(AIR,            FALSE,  INHIBIT_POWER,	"bombards the area with multiple light, gravity and lightning balls", 0, 0)
EFFECT(PORTALS,        FALSE,  INHIBIT_POWER,	"allows directed teleportation", 0, 0)
EFFECT(GWINDOR,        FALSE,  0,	"illumination (2d15 damage)", 0, 0)
EFFECT(DWARVES,        FALSE,  0,	"level clairvoyance", 0, 0)
EFFECT(ELESSAR,        FALSE,  0,	"heal (500) and restore life levels", 0, 0)
EFFECT(RAZORBACK,      FALSE,  0,	"Assume Dragonform; Activation in Dragonform: star ball (150)", 0, 0)
EFFECT(BLADETURNER,    FALSE,  0,	"Assume Dragonform; Activation in Dragonform: heroism, bless, and resistance", 0, 0)
EFFECT(SOULKEEPER,     FALSE,  0,	"heal (1000)", 0, 0)
EFFECT(ELEMENTS,       FALSE,  0,	"protection from the elements", 0, 0)
EFFECT(GIL_GALAD,      FALSE,  0,	"blinding light (75)", 0, 0)
EFFECT(NARGOTHROND,    FALSE,  0,	"heal (500)", 0, 0)
EFFECT(VALINOR,        FALSE,  0,	"resistance (20+d20 turns)", 0, 0)
EFFECT(HOLCOLLETH,     FALSE,  0,	"Sleep II", 0, 0)
EFFECT(THINGOL,        FALSE,  0,	"recharge magical device", 0, 0)
EFFECT(MAEGLIN,        TRUE,   0,	"mana bolt (9d8)", 0, 0)
EFFECT(PAURNIMMEN,     FALSE,  0,	"add cold damage to your melee attacks (50 turns)", 0, 0)
EFFECT(PAURNEN,        FALSE,  0,	"add acid damage to your melee attacks (30 turns)", 0, 0)
EFFECT(DAL,            FALSE,  0,	"remove fear and cure poison", 0, 0)
EFFECT(NARTHANC,       TRUE,   0,	"fire bolt (6d8)", 0, 0)
EFFECT(NIMTHANC,       TRUE,   0,	"frost bolt (5d8)", 0, 0)
EFFECT(DETHANC,        TRUE,   0,	"lightning bolt (4d8)", 0, 0)
EFFECT(RILIA,          TRUE,   0,	"stinking cloud (12)", 0, 0)
EFFECT(BELANGIL,       TRUE,   0,	"frost ball (3 * level / 2)", 0, 0)
EFFECT(ARANRUTH,       TRUE,   0,	"frost bolt (12d8)", 0, 0)
EFFECT(RINGIL,         TRUE,   0,	"frost cone (250)", 0, 0)
EFFECT(NARSIL,         TRUE,   0,	"fire ball (150)", 0, 0)
EFFECT(MANWE,          TRUE,   0,	"wide arc of force (300)", 0, 0)
EFFECT(AEGLOS,         TRUE,   0,	"frost ball (100)", 0, 0)
EFFECT(LOTHARANG,      FALSE,  0,	"cure wounds (4d12)", 0, 0)
EFFECT(ULMO,           TRUE,   0,	"teleport away (distance: 45-60+)", 0, 0)
EFFECT(AVAVIR,         FALSE,  0,	"word of recall", 0, 0)
EFFECT(TOTILA,         TRUE,   0,	"confuse monster", 0, 0)
EFFECT(FIRESTAR,       TRUE,   0,	"large fire ball (125)", 0, 0)
EFFECT(TURMIL,         TRUE,   0,	"drain life (90)", 0, 0)
EFFECT(DRAGON_BLACK,   FALSE,  9,	"assume dragon form; Activation in dragon form: breathe acid for 45-270 damage", 0, 0)
EFFECT(DRAGON_BLUE,    FALSE,  20,	"assume dragon form; Activation in dragon form: breathe lightning for 40-260 damage", 0, 0)
EFFECT(DRAGON_WHITE,   FALSE,  20,	"assume dragon form; Activation in dragon form: breathe frost for 45-270 damage", 0, 0)
EFFECT(DRAGON_RED,     FALSE,  20,	"assume dragon form; Activation in dragon form: breathe fire for 50-300 damage", 0, 0)
EFFECT(DRAGON_GREEN,   FALSE,  20,	"assume dragon form; Activation in dragon form: breathe poison gas for 45-270 damage", 0, 0)
EFFECT(DRAGON_MULTIHUED, FALSE,  20,	"assume dragon form; Activation in dragon form: breathe an element or poison for 60-360 damage", 0, 0)
EFFECT(DRAGON_SHINING, FALSE,  22,	"assume dragon form; Activation in dragon form: breathe light/darkness for 50-300 damage", 0, 0)
EFFECT(DRAGON_LAW,     FALSE,  22,	"assume dragon form; Activation in dragon form: breathe sound or shards for 60-360 damage", 0, 0)
EFFECT(DRAGON_BRONZE,  FALSE,  20,	"assume dragon form; Activation in dragon form: breathe confusionfor 40-260 damage", 0, 0)
EFFECT(DRAGON_GOLD,    FALSE,  20,	"assume dragon form; Activation in dragon form: breathe sound for 40-260 damage", 0, 0)
EFFECT(DRAGON_CHAOS,   FALSE,  22,	"assume dragon form; Activation in dragon form: breathe chaos or disenchantment for 55-330 damage", 0, 0)
EFFECT(DRAGON_BALANCE, FALSE,  23,	"assume dragon form; Activation in dragon form: breathe sound, shards, chaos, or disenchantment for 60-360 damage", 0, 0)
EFFECT(DRAGON_POWER,   FALSE,  25,	"assume dragon form; Activation in dragon form: breathe the elements for 75-450 damage", 0, 0)
EFFECT(RING_ACID,      TRUE,   11,	"cast an acid ball(80) and oppose acid", 0, 0)
EFFECT(RING_ELEC,      TRUE,   11,	"cast a cold ball(80) and oppose cold", 0, 0)
EFFECT(RING_FIRE,      TRUE,   11,	"cast a fire ball(80) and oppose fire", 0, 0)
EFFECT(RING_COLD,      TRUE,   11,	"cast an electricity ball(80) and oppose electricity", 0, 0)
EFFECT(RING_POIS,      TRUE,   11,	"cast a poison ball(80) and oppose poison", 0, 0)
EFFECT(AMULET_ESCAPING, FALSE,  10,	"teleport(40)", 0, 0)
EFFECT(AMULET_LION,    FALSE,  15,	"become lion", 0, 0)
EFFECT(AMULET_METAMORPH, FALSE,  15,	"Make a random shapechange", 0, 0)
EFFECT(RAND_FIRE1,         TRUE,   4,	"fire bolt (3 + level / 8)d8", 7, 7)
EFFECT(RAND_FIRE2,         TRUE,   5,	"sphere of fire (90)", 250, 0)
EFFECT(RAND_FIRE3,         FALSE,  6,	"fire storm (150)", 600, 0)
EFFECT(RAND_COLD1,         TRUE,   4,	"frost bolt (3 + level / 8)d8", 7, 7)
EFFECT(RAND_COLD2,         TRUE,   5,	"sphere of frost (90)", 250, 0)
EFFECT(RAND_COLD3,         FALSE,  6,	"frost storm (150)", 600, 0)
EFFECT(RAND_ACID1,         TRUE,   4,	"acid bolt (3 + level / 8)d8", 7, 7)
EFFECT(RAND_ACID2,         TRUE,   5,	"sphere of acid (90)", 250, 0)
EFFECT(RAND_ACID3,         FALSE,  6,	"acid storm (160)", 600, 0)
EFFECT(RAND_ELEC1,         TRUE,   4,	"electricity bolt (3 + level / 8)d8", 7, 7)
EFFECT(RAND_ELEC2,         TRUE,   5,	"ball lightning (100)", 250, 0)
EFFECT(RAND_ELEC3,         FALSE,  6,	"lightning strike (130+25)", 600, 0)
EFFECT(RAND_POIS1,         TRUE,   4,	"poison dart (3 + level / 10)d8", 22, 22)
EFFECT(RAND_POIS2,         TRUE,   5,	"poison cloud (110)", 300, 0)
EFFECT(RAND_LIGHT1,        TRUE,   6,	"blinding ball of light (50+10)", 250, 0)
EFFECT(RAND_LIGHT2,        FALSE,  6,	"dispel light-hating (175)", 600, 0)
EFFECT(RAND_DISPEL_UNDEAD, FALSE,  6,	"dispel undead (100)", 300, 0)
EFFECT(RAND_DISPEL_EVIL,   FALSE,  9,	"dispel evil (100)", 400, 0)
EFFECT(RAND_SMITE_UNDEAD,  TRUE,   8,	"dispel an undead (level / 4)d33", 200, 0)
EFFECT(RAND_SMITE_DEMON,   TRUE,   8,	"dispel a demon (level / 4)d33", 200, 0)
EFFECT(RAND_SMITE_DRAGON,  TRUE,   8,	"dispel a dragon (level / 4)d33", 200, 0)
EFFECT(RAND_HOLY_ORB,      TRUE,   5,	"holy orb (60)", 175, 0)
EFFECT(RAND_BLESS,         FALSE,  4,	"blessing (24+d24)", 200, 0)
EFFECT(RAND_FRIGHTEN_ALL,  FALSE,  5,	"frighten adversaries", 120, 120)
EFFECT(RAND_HEAL1,         FALSE,  5,	"heal (5d20)", 85, 0)
EFFECT(RAND_HEAL2,         FALSE,  8,	"heal (7d40)", 225, 0)
EFFECT(RAND_HEAL3,         FALSE,  10,	"heal (10d60)", 500, 0)
EFFECT(RAND_CURE,          FALSE,  8,	"cure ailments", 500, 0)
EFFECT(RAND_PROT_FROM_EVIL, FALSE,  6,	"protection from evil (24+d24)", 250, 0)
EFFECT(RAND_CHAOS,         TRUE,   9,	"chaos ball (d300)", 600, 0)
EFFECT(RAND_SHARD_SOUND,   TRUE,   9,	"shard or sound ball (150)", 600, 0)
EFFECT(RAND_NETHR,         TRUE,   8,	"nether orb (100)", 400, 0)
EFFECT(RAND_LINE_LIGHT,    TRUE,   4,	"ray of light (4d5)", 6, 6)
EFFECT(RAND_STARLIGHT,     FALSE,  5,	"starlight (4d5)", 8, 8)
EFFECT(RAND_EARTHQUAKE,    FALSE,  5,	"earthquake (radius 10)", 40, 40)
EFFECT(RAND_IDENTIFY,      FALSE,  6,	"identify", 30, 30)
EFFECT(RAND_SPEED,         FALSE,  10,	"haste self (20+d20)", 120, 120)
EFFECT(RAND_TELEPORT_AWAY, TRUE,   9,	"teleport away (distance: 45-60+)", 110, 0)
EFFECT(RAND_HEROISM,       FALSE,  6,	"heroism", 200, 0)
EFFECT(RAND_STORM_DANCE,   FALSE,  11,	"storm dance", 300, 0)
EFFECT(RAND_RESIST_ELEMENTS, FALSE,  8,	"resistance to the elements", 400, 0)
EFFECT(RAND_RESIST_ALL,    FALSE,  10,	"resistance", 400, 0)
EFFECT(RAND_TELEPORT1,     FALSE,  6,	"teleport self (30)", 10, 10)
EFFECT(RAND_TELEPORT2,     FALSE,  9,	"major displacement (200)", 80, 0)
EFFECT(RAND_RECALL,        FALSE,  10,	"recall", 350, 0)
EFFECT(RAND_REGAIN,        FALSE,  8,	"restore level", 800, 0)
EFFECT(RAND_RESTORE,       FALSE,  10,	"restore stats", 800, 0)
EFFECT(RAND_SHIELD,        FALSE,  8,	"magic shield", 400, 0)
EFFECT(RAND_BRAND_MISSILE, FALSE,  20,	"brand missiles", 0, 0)
EFFECT(RAND_SUPER_SHOOTING, FALSE,  10,	"an especially deadly shot", 200, 200)
EFFECT(RAND_DETECT_MONSTERS, FALSE,  4,	"detect monsters", 4, 4)
EFFECT(RAND_DETECT_EVIL,   FALSE,  3,	"detect evil", 4, 4)
EFFECT(RAND_DETECT_ALL,    FALSE,  8,	"detection", 30, 30)
EFFECT(RAND_MAGIC_MAP,     FALSE,  8,	"sense surroundings", 30, 30)
EFFECT(RAND_DETECT_D_S_T,  FALSE,  5,	"detect traps, doors, and stairs", 10, 10)
EFFECT(RAND_CONFU_FOE,     TRUE,   4,	"strong confuse monster", 250, 0)
EFFECT(RAND_SLEEP_FOE,     TRUE,   4,	"strong sleep monster", 250, 0)
EFFECT(RAND_TURN_FOE,      TRUE,   4,	"strong frighten monster", 250, 0)
EFFECT(RAND_SLOW_FOE,      TRUE,   4,	"strong slow monster", 250, 0)
EFFECT(RAND_BANISH_EVIL,   FALSE,  6,	"banish evil", 400, 0)
EFFECT(RAND_DISARM,        TRUE,   4,	"disarming", 7, 7)
EFFECT(RAND_CONFU_FOES,    FALSE,  5,	"confuse monsters", 300, 0)
EFFECT(RAND_SLEEP_FOES,    FALSE,  5,	"sleep monsters", 300, 0)
EFFECT(RAND_TURN_FOES,     FALSE,  5,	"frighten monsters", 300, 0)
EFFECT(RAND_SLOW_FOES,     FALSE,  5,	"slow monsters", 300, 0)
EFFECT(ACID_BLAST,     FALSE,  18,	"acid blast", 1500, 0)
EFFECT(CHAIN_LIGHTNING, FALSE,  19,	"chain lightning", 2000, 0)
EFFECT(LAVA_POOL,      FALSE,  22,	"lava pool", 2000, 0)
EFFECT(ICE_WHIRLPOOL,  FALSE,  22,	"whirlpool of ice", 1000, 0)
EFFECT(GROW_FOREST,    FALSE,  22,	"grow forest", 2000, 0)
EFFECT(RESTORE_AND_ENHANCE, FALSE,  23,	"restore and enhance you", 3000, 0)
EFFECT(ZONE_OF_CHAOS,  FALSE,  23,	"create a zone of chaos", 3000, 0)
EFFECT(PRESSURE_WAVE,  FALSE,  21,	"pressure wave", 1500, 0)
EFFECT(ENERGY_DRAIN,   FALSE,  25,	"drain energy", 3000, 0)
EFFECT(MASS_STASIS,    FALSE,  20,	"mass stasis", 1000, 0)
EFFECT(LIGHT_FROM_ABOVE, FALSE,  22,	"light from above", 1500, 0)
EFFECT(MASS_POLYMORPH, FALSE,  18,	"mass polymorph", 2500, 0)
EFFECT(GRAVITY_WAVE,   FALSE,  25,	"gravity wave", 2000, 0)
EFFECT(ENLIST_EARTH,   TRUE,   20,	"summon allies from the Earth", 3000, 0)
EFFECT(TELEPORT_ALL,   FALSE,  25,	"mass teleport", 2000, 0)
EFFECT(BALROG_WHIP,    TRUE,   0,	"lash out at at range 2 for base weapon damage (unless target resists fire)", 0, 0)
EFFECT(MAGESTAFF,      FALSE,  0,	"restore 10 spell points (mages and necromancers)", 15, 10)
