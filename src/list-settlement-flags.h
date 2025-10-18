/**
 * \file list-settlement-flags.h
 * \brief List flags for settlement types
 *
 * Changing these flags would not affect savefiles but would affect the parsing
 * of settlement.txt.
 *
 * Fields:
 * name
 * help string
 */
SETTF(TOWN, "Settlement can be generated in towns")
SETTF(PLAIN, "Settlement can be generated in plains")
SETTF(FOREST, "Settlement can be generated in forests")
SETTF(DESERT, "Settlement can be generated in desert")
SETTF(LIGHT, "Settlement is always lit")
SETTF(MAX, "")
