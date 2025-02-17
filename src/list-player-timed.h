/**
 * \file list-player-timed.h
 * \brief timed player properties
 *
 * Fields:
 * symbol - the effect name
 * flag_redraw - the things to be redrawn when the effect is set
 * flag_update - the things to be updated when the effect is set
 */

/* symbol		flag_redraw						flag_update */
TMD(FAST,		PR_STATUS,						PU_BONUS)
TMD(SLOW,		PR_STATUS,						PU_BONUS)
TMD(BLIND,		PR_MAP,							PU_UPDATE_VIEW | PU_MONSTERS) 
TMD(ENTRANCED,	PR_STATUS,						PU_BONUS)
TMD(CONFUSED,	PR_STATUS,						PU_BONUS)
TMD(AFRAID,		PR_STATUS,						PU_BONUS)
TMD(IMAGE,		PR_MAP | PR_MONLIST | PR_ITEMLIST,	PU_BONUS)
TMD(POISONED,	PR_STATUS,						PU_BONUS)
TMD(SICK,		PR_STATUS,						PU_BONUS)
TMD(CUT,		PR_STATUS,						PU_BONUS)
TMD(STUN,		PR_STATUS,						PU_BONUS)
TMD(FOOD,		PR_STATUS,						PU_BONUS)
TMD(DARKENED,	PR_STATUS,						PU_BONUS)
TMD(RAGE,		PR_STATUS,						PU_BONUS)
TMD(STR,		PR_STATUS,						PU_BONUS)
TMD(DEX,		PR_STATUS,						PU_BONUS)
TMD(CON,		PR_STATUS,						PU_BONUS)
TMD(GRA,		PR_STATUS,						PU_BONUS)
TMD(SINVIS,		PR_STATUS,						PU_BONUS | PU_MONSTERS)
TMD(OPP_FIRE,	PR_STATUS,						PU_BONUS)
TMD(OPP_COLD,	PR_STATUS,						PU_BONUS)
TMD(OPP_POIS,	PR_STATUS,						PU_BONUS)
