/**
 * \file list-mon-timed.h
 * \brief Monster timed flags
 *
 * Fields:
 * - name         - the index name for this timed flag
 * - save         - does this effect get a saving throw?
 * - stack        - how does this effect stack? either NO, MAX, or INCR
 * - resist_flag  - monsters with this monster race flag will resist this effect
 * - time         - maximum that the timer for this effect can reach (must be below 32767)
 * (messages)
 * - message_begin - the argument to the message code when the effect begins
 * - message_end - the argument to the message code when the effect ends
 * - message_increase - the argument to the message code when the effect increases
 */
/*     name  	save  	stack  	resist_flag  	time  	message_begin           message_end             message_increase       */
MON_TMD(STUN,	false,	MAX,	RF_NO_STUN,		200,	MON_MSG_DAZED,			MON_MSG_NOT_DAZED,		MON_MSG_MORE_DAZED)
MON_TMD(CONF,	true,	MAX,	RF_NO_CONF,		200,	MON_MSG_CONFUSED,		MON_MSG_NOT_CONFUSED,	MON_MSG_MORE_CONFUSED)
MON_TMD(SLOW,	true,	INCR,	RF_NO_SLOW,		5000,	MON_MSG_SLOWED,			MON_MSG_NOT_SLOWED,		MON_MSG_MORE_SLOWED)
MON_TMD(FAST,	false,	INCR,	0,				5000,	MON_MSG_HASTED,			MON_MSG_NOT_HASTED,		MON_MSG_MORE_HASTED)
MON_TMD(MAX,	true,	INCR,	0,				0,		0,						0,						0)
