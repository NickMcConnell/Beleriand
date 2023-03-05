/**
 * \file songs.h
 * \brief Player and monster songs
 *
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke
 *
 * This work is free software; you can redistribute it and/or modify it
 * under the terms of either:
 *
 * a) the GNU General Public License as published by the Free Software
 *    Foundation, version 2, or
 *
 * b) the "Angband licence":
 *    This software may be copied and distributed for educational, research,
 *    and not for profit purposes provided that this copyright and statement
 *    are included in all such copies.  Other copyrights may also apply.
 */
#ifndef INCLUDED_SONGS_H
#define INCLUDED_SONGS_H

struct alt_song_desc {
	char *desc;
	struct alt_song_desc *next;
};

extern struct song *songs;

struct song *song_by_idx(int idx);
struct song *lookup_song(char *name);
int song_bonus(struct player *p, int pskill, struct song *song);
void player_change_song(struct player *p, struct song *song, bool exchange);
bool player_is_singing(struct player *p, struct song *song);
int player_song_noise(struct player *p);
void player_sing(struct player *p);
int monster_sing(struct monster *mon, struct song *song);

extern struct file_parser song_parser;

#endif /* !INCLUDED_SONGS_H */
