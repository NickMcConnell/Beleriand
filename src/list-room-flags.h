/**
 * \file list-room-flags.h
 * \brief List flags for room types
 *
 * Changing these flags would not affect savefiles but would affect the parsing
 * of vault.txt.
 *
 * Fields:
 * name
 * help string
 */
ROOMF(NO_ROTATION, "Vault cannot be rotated")
ROOMF(TRAPS, "Vault has more traps than usual")
ROOMF(WEBS, "Vault has spider webs")
ROOMF(LIGHT, "Vault is always generated with light")
ROOMF(TEST, "Vault must be generated (for debugging)")
ROOMF(MAX, "")
