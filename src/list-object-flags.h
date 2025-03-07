/**
 * \file list-object-flags.h
 * \brief object flags for all objects
 *
 * Changing flag order will break savefiles. Flags
 * below start from 1 on line 17, so a flag's sequence number is its line
 * number minus 16.
 *
 * Each sustain flag (SUST_*) has a matching stat in src/list-stats.h,
 * which should be at the same index in that file as the sustain in this file.
 *
 * The second argument to OF is the label used in the debugging commands
 * object flag display.  At most the first five characters are used.
 *
 * Flag properties are defined in lib/gamedata/object_property.txt
 */
OF(SUST_STR, " sStr")
OF(SUST_DEX, " sDex")
OF(SUST_CON, " sCon")
OF(SUST_GRA, " sGra")
OF(PROT_FEAR, "pFear")
OF(PROT_BLIND, "pBlnd")
OF(PROT_CONF, "pConf")
OF(PROT_STUN, "pStun")
OF(PROT_HALLU, "pHall")
OF(SLOW_DIGEST, "S.Dig")
OF(REGEN, "Regen")
OF(SEE_INVIS, "S.Inv")
OF(FREE_ACT, "FrAct")
OF(RADIANCE, "Rad")
OF(LIGHT, "Light")
OF(SPEED, "Speed")
OF(SHARPNESS, "Sharp")
OF(SHARPNESS2, "SHARP")
OF(VAMPIRIC, "Vamp")
OF(BURNS_OUT, "BuOut")
OF(TAKES_FUEL, "TFuel")
OF(NO_FUEL, "NFuel")
OF(COWARDICE, "Cowrd")
OF(HUNGER, "Hungr")
OF(DARKNESS, "Darkn")
OF(DANGER, "Dangr")
OF(HAUNTED, "Haunt")
OF(AGGRAVATE, "Aggrv")
OF(CURSED, "Curse")
OF(DIG_1, " Dig1")
OF(DIG_2, " Dig2")
OF(THROWING, "Throw")
OF(INDESTRUCTIBLE, "")
OF(CRAFT, "")
OF(WOODCRAFT, "")
OF(NO_SMITHING, "")
OF(NO_RANDOM, "")
OF(MITHRIL, "")
OF(AXE, "")
OF(POLEARM, "")
OF(ENCHANTABLE, "")
OF(HAND_AND_A_HALF, "")
OF(TWO_HANDED, "")
OF(MAX, "")
