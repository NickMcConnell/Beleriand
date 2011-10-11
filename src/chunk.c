/*
 * File: chunk.c
 * Purpose: Chunk loading and saving routines; based on savefiles
 *
 * Copyright (c) 2011 Andi Sidwell, Nick McConnell
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
#include <errno.h>
#include "angband.h"
#include "savefile.h"

/**
 * Chunks are pieces of wilderness/dungeon which are saved off to make the
 * 'semi-persistent' world of Beleriand.
 *
 * The chunk saving routines are basically cut-down versions of savefiles.
 */


/**
 * Chunk saving functions 
 */
static const struct {
        char name[16];
        void (*save)(void);
        u32b version;   
} savers[] = {
        { "dungeon", cwr_dungeon_chunk, 1 },
        { "objects", cwr_objects_chunk, 1 },
        { "monsters", cwr_monsters_chunk, 1 },
        { "traps", cwr_traps_chunk, 1 },
};

/**
 * Chunk loading functions 
 */
static const struct {
        char name[16];
        int (*load)(void);
        u32b version;
} loaders[] = {
        { "dungeon", rd_dungeon, 1 },
        { "objects", rd_objects, 1 },
        { "monsters", rd_monsters, 1 },
        { "traps", rd_traps, 1 },
};


/* Buffer bits */
static byte *buffer;
static u32b buffer_size;
static u32b buffer_pos;
static u32b buffer_check;

#define BUFFER_INITIAL_SIZE		1024
#define BUFFER_BLOCK_INCREMENT	1024

#define SAVEFILE_HEAD_SIZE		28

/* Chunk location */
static int ch_y = 0;
static int ch_x = 0;


/** Utility **/

static bool grid_is_in_chunk(int y, int x)
{
    return ((y < (ch_y + CHUNK_HGT)) && (x < (ch_x + CHUNK_WID)));
}

/** Base put/get **/

static void chunk_put(byte v)
{
	assert(buffer != NULL);
	assert(buffer_size > 0);

	if (buffer_size == buffer_pos)
	{
		buffer_size += BUFFER_BLOCK_INCREMENT;
		buffer = mem_realloc(buffer, buffer_size);
	}

	assert(buffer_pos < buffer_size);

	buffer[buffer_pos++] = v;
	buffer_check += v;
}

static byte chunk_get(void)
{
	assert(buffer != NULL);
	assert(buffer_size > 0);
	assert(buffer_pos < buffer_size);

	buffer_check += buffer[buffer_pos];

	return buffer[buffer_pos++];
}


/* accessor */

void cwr_byte(byte v)
{
	sf_put(v);
}

void cwr_u16b(u16b v)
{
	sf_put((byte)(v & 0xFF));
	sf_put((byte)((v >> 8) & 0xFF));
}

void cwr_s16b(s16b v)
{
	cwr_u16b((u16b)v);
}

void cwr_u32b(u32b v)
{
	sf_put((byte)(v & 0xFF));
	sf_put((byte)((v >> 8) & 0xFF));
	sf_put((byte)((v >> 16) & 0xFF));
	sf_put((byte)((v >> 24) & 0xFF));
}

void cwr_s32b(s32b v)
{
	cwr_u32b((u32b)v);
}

void cwr_string(const char *str)
{
	while (*str)
	{
		cwr_byte(*str);
		str++;
	}
	cwr_byte(*str);
}


void crd_byte(byte *ip)
{
	*ip = sf_get();
}

void crd_u16b(u16b *ip)
{
	(*ip) = sf_get();
	(*ip) |= ((u16b)(sf_get()) << 8);
}

void crd_s16b(s16b *ip)
{
	crd_u16b((u16b*)ip);
}

void crd_u32b(u32b *ip)
{
	(*ip) = sf_get();
	(*ip) |= ((u32b)(sf_get()) << 8);
	(*ip) |= ((u32b)(sf_get()) << 16);
	(*ip) |= ((u32b)(sf_get()) << 24);
}

void crd_s32b(s32b *ip)
{
	crd_u32b((u32b*)ip);
}

void crd_string(char *str, int max)
{
	byte tmp8u;
	int i = 0;

	do
	{
		crd_byte(&tmp8u);

		if (i < max) str[i] = tmp8u;
		if (!tmp8u) break;
	} while (++i);

	str[max - 1] = '\0';
}

/*** Chunk saving functions ***/

/**
 * Write an "item" record
 */
static void cwr_item(object_type *o_ptr)
{
    size_t i;

    cwr_s16b(o_ptr->k_idx);
  
    /* Location */
    cwr_byte(o_ptr->iy);
    cwr_byte(o_ptr->ix);
  
    cwr_byte(o_ptr->tval);
    cwr_byte(o_ptr->sval);
    cwr_s16b(o_ptr->pval);
  
    cwr_byte(o_ptr->discount);
    cwr_byte(o_ptr->number);
    cwr_s16b(o_ptr->weight);
  
    cwr_byte(o_ptr->name1);
    cwr_byte(o_ptr->name2);
    cwr_s16b(o_ptr->timeout);
  
    cwr_s16b(o_ptr->to_h);
    cwr_s16b(o_ptr->to_d);
    cwr_s16b(o_ptr->to_a);
    cwr_s16b(o_ptr->ac);
    cwr_byte(o_ptr->dd);
    cwr_byte(o_ptr->ds);
  
    cwr_byte(o_ptr->ident);
  
    cwr_byte(o_ptr->marked);
  
    cwr_byte(o_ptr->origin);
    cwr_byte(o_ptr->origin_stage);
    cwr_u16b(o_ptr->origin_xtra);

    /* Flags */
    for (i = 0; i < OF_SIZE; i++)
	cwr_byte(o_ptr->flags_obj[i]);
    for (i = 0; i < CF_SIZE; i++)
	cwr_byte(o_ptr->flags_curse[i]);
    for (i = 0; i < CF_SIZE; i++)
	cwr_byte(o_ptr->id_curse[i]);
    for (i = 0; i < OF_SIZE; i++)
	cwr_byte(o_ptr->id_obj[i]);
    for (i = 0; i < IF_SIZE; i++)
	cwr_byte(o_ptr->id_other[i]);

    /* Resists, bonuses, multiples -NRM- */
    for (i = 0; i < MAX_P_RES; i++)
	cwr_byte(o_ptr->percent_res[i]);
    for (i = 0; i < A_MAX; i++)
	cwr_byte(o_ptr->bonus_stat[i]);
    for (i = 0; i < MAX_P_BONUS; i++)
	cwr_byte(o_ptr->bonus_other[i]);
    for (i = 0; i < MAX_P_SLAY; i++)
	cwr_byte(o_ptr->multiple_slay[i]);
    for (i = 0; i < MAX_P_BRAND; i++)
	cwr_byte(o_ptr->multiple_brand[i]);

    /* Held by monster index */
    cwr_s16b(o_ptr->held_m_idx);
  
    /* Activation */
    cwr_u16b(o_ptr->effect);
    cwr_u16b(o_ptr->time.base);
    cwr_u16b(o_ptr->time.dice);
    cwr_u16b(o_ptr->time.sides);
  
    /* Feeling */
    cwr_byte(o_ptr->feel);
  
    /* Save the inscription (if any) */
    if (o_ptr->note)
    {
	cwr_string(quark_str(o_ptr->note));
    }
    else
    {
	cwr_string("");
    }

    /* Expansion */
    cwr_u32b(0);
}
  

/**
 * Write a "monster" record
 */
static void cwr_monster(monster_type *m_ptr)
{
    monster_race *r_ptr = &r_info[m_ptr->r_idx];

    /* Special treatment for player ghosts */
    if (rf_has(r_ptr->flags, RF_PLAYER_GHOST))
	cwr_s16b(r_ghost);
    else
	cwr_s16b(m_ptr->r_idx);

    cwr_byte(m_ptr->fy);
    cwr_byte(m_ptr->fx);
    cwr_s16b(m_ptr->hp);
    cwr_s16b(m_ptr->maxhp);
    cwr_s16b(m_ptr->csleep);
    cwr_byte(m_ptr->mspeed);
    cwr_byte(m_ptr->energy);
    cwr_byte(m_ptr->stunned);
    cwr_byte(m_ptr->confused);
    cwr_byte(m_ptr->monfear);
    cwr_byte(m_ptr->stasis);
  
    cwr_byte(m_ptr->black_breath);
  
    cwr_u32b(m_ptr->smart); /* Flags for 'smart-learn' */
  
    /* Dummy writes for features soon to be implemented */
    cwr_byte(0);   

    /* Shapechange counter */
    cwr_byte(m_ptr->schange);

    /* Original form of shapechanger */      
    cwr_s16b(m_ptr->orig_idx);	 
  
    /* Extra desire to cast harassment spells */
    cwr_byte(m_ptr->harass);
  
    /* Current Mana */   
    cwr_byte(m_ptr->mana);    
  
    /* Racial type */
    cwr_byte(m_ptr->p_race);
    cwr_byte(m_ptr->old_p_race);

    /* AI info */
    cwr_s16b(m_ptr->hostile);
    cwr_u16b(m_ptr->group);
    cwr_u16b(m_ptr->group_leader);

    /* Territorial info */
    cwr_u16b(m_ptr->y_terr);
    cwr_u16b(m_ptr->x_terr);
          
    /* Spare */
    cwr_s16b(0); 

    /* Expansion */
    cwr_u32b(0);
}

/**
 * Write a trap record
 */
static void cwr_trap(trap_type *t_ptr)
{
    size_t i;

    cwr_byte(t_ptr->t_idx);
    cwr_byte(t_ptr->fy);
    cwr_byte(t_ptr->fx);
    cwr_byte(t_ptr->xtra);

    for (i = 0; i < TRF_SIZE; i++)
	cwr_byte(t_ptr->flags[i]);
}

/**
 * Write the current dungeon
 */
void cwr_dungeon(void)
{
    int y, x;
    size_t i;
  
    byte tmp8u;
  
    byte count;
    byte prev_char;
  
  
    if (p_ptr->is_dead)
	return;

    /*** Basic info ***/
  
    /* Dungeon specific info follows */
    cwr_u16b(p_ptr->stage);
    cwr_u16b(p_ptr->last_stage);
    cwr_u16b(p_ptr->py);
    cwr_u16b(p_ptr->px);
    cwr_u16b(DUNGEON_HGT);
    cwr_u16b(DUNGEON_WID);
    cwr_u16b(CAVE_SIZE);
    cwr_u16b(0);
  
  
    /*** Simple "Run-Length-Encoding" of cave ***/
  
    /* Loop across bytes of cave_info */
    for (i = 0; i < CAVE_SIZE; i++)
    {
	/* Note that this will induce two wasted bytes */
	count = 0;
	prev_char = 0;
	
	/* Dump the cave */
	for (y = ch_y; y < ch_y + CHUNK_HGT; y++)
	{
	    for (x = ch_x; x < ch_x + CHUNK_WID; x++)
	    {
		/* Extract the important cave_info flags */
		tmp8u = cave_info[y][x][i];
	  
		/* If the run is broken, or too full, flush it */
		if ((tmp8u != prev_char) || (count == MAX_UCHAR))
		{
		    cwr_byte((byte)count);
		    cwr_byte((byte)prev_char);
		    prev_char = tmp8u;
		    count = 1;
		}
		
		/* Continue the run */
		else
		{
		    count++;
		}
	    }
	}
	
	/* Flush the data (if any) */
	if (count)
	{
	    cwr_byte((byte)count);
	    cwr_byte((byte)prev_char);
	}
    }
  
    /*** Simple "Run-Length-Encoding" of cave ***/
  
    /* Note that this will induce two wasted bytes */
    count = 0;
    prev_char = 0;
  
    /* Dump the cave */
    for (y = ch_y; y < ch_y + CHUNK_HGT; y++)
    {
	for (x = ch_x; x < ch_x + CHUNK_WID; x++)
	{
	    /* Extract a byte */
	    tmp8u = cave_feat[y][x];
	  
	    /* If the run is broken, or too full, flush it */
	    if ((tmp8u != prev_char) || (count == MAX_UCHAR))
	    {
		cwr_byte((byte)count);
		cwr_byte((byte)prev_char);
		prev_char = tmp8u;
		count = 1;
	    }
	  
	    /* Continue the run */
	    else
	    {
		count++;
	    }
	}
    }
  
    /* Flush the data (if any) */
    if (count)
    {
	cwr_byte((byte)count);
	cwr_byte((byte)prev_char);
    }
}
  
/*** Dump objects ***/
  
void cwr_objects(void)
{
    int i, num = 0;

    /* Dump the objects */
    for (i = 1; i < o_max; i++)
    {
	object_type *o_ptr = &o_list[i];
      
	/* Dump it */
	if (grid_is_in_chunk(o_ptr->iy, o_ptr->ix))
	{
	    cwr_item(o_ptr);
	    num++;
	}
    }

    /* Total objects */
    cwr_u16b(num);
  
    /* Expansion */
    cwr_u32b(0);
}
  
  
/*** Dump the monsters ***/
  
void cwr_monsters(void)
{
    int i, num = 0;

    /* Dump the monsters */
    for (i = 1; i < m_max; i++)
    {
	monster_type *m_ptr = &m_list[i];
      
	/* Dump it */
	if (grid_is_in_chunk(m_ptr->fy, m_ptr->fx))
	{
	    cwr_monster(m_ptr);
	    num++;
	}
    }

    /* Total monsters */
    cwr_u16b(num);
  
    /* Expansion */
    cwr_u32b(0);
}


void cwr_traps(void)
{
    int i, num = 0;

    cwr_byte(TRF_SIZE);

    for (i = 0; i < trap_max; i++)
    {
	trap_type *t_ptr = &trap_list[i];

	/* Dump it */
	if (grid_is_in_chunk(t_ptr->fy, t_ptr->fx))
	{
	    cwr_trap(t_ptr);
	    num++;
	}
    }

    /* Total traps */
    cwr_u16b(num);
  
    /* Expansion */
    cwr_u32b(0);
}


static bool chunk_save(byte *chunk)
{
	byte savefile_head[SAVEFILE_HEAD_SIZE];
	size_t i, pos;

	/* Start off the buffer */
        buffer = mem_alloc(BUFFER_INITIAL_SIZE);
        buffer_size = BUFFER_INITIAL_SIZE;

        for (i = 0; i < N_ELEMENTS(savers); i++)
        {
                buffer_pos = 0;
                buffer_check = 0;

                savers[i].save();

                /* 16-byte block name */
                pos = my_strcpy((char *)savefile_head,
                                savers[i].name,
                                sizeof savefile_head);
                while (pos < 16)
                        savefile_head[pos++] = 0;

#define SAVE_U32B(v)    \
                savefile_head[pos++] = (v & 0xFF); \
                savefile_head[pos++] = ((v >> 8) & 0xFF); \
                savefile_head[pos++] = ((v >> 16) & 0xFF); \
                savefile_head[pos++] = ((v >> 24) & 0xFF);

                SAVE_U32B(savers[i].version);
                SAVE_U32B(buffer_pos);
                SAVE_U32B(buffer_check);

                assert(pos == SAVEFILE_HEAD_SIZE);

		file_write(file, (char *)savefile_head, SAVEFILE_HEAD_SIZE);
		file_write(file, (char *)buffer, buffer_pos);

		/* pad to 4 byte multiples */
		if (buffer_pos % 4)
			file_write(file, "xxx", 4 - (buffer_pos % 4));
	}

	mem_free(buffer);

	return TRUE;
}


/*
 * Attempt to save the player in a savefile
 */
bool savefile_save(const char *path)
{
        ang_file *file;
        int count = 0;
        char new_savefile[1024];
        char old_savefile[1024];

        /* New savefile */
        strnfmt(old_savefile, sizeof(old_savefile), "%s%u.old", path,Rand_simple(1000000));
        while (file_exists(old_savefile) && (count++ < 100)) {
                strnfmt(old_savefile, sizeof(old_savefile), "%s%u%u.old", path,Rand_simple(1000000),count);
        }
        count = 0;
        /* Make sure that the savefile doesn't already exist */
        /*safe_setuid_grab();
        file_delete(new_savefile);
        file_delete(old_savefile);
        safe_setuid_drop();*/

        /* Open the savefile */
        safe_setuid_grab();
        strnfmt(new_savefile, sizeof(new_savefile), "%s%u.new", path,Rand_simple(1000000));
        while (file_exists(new_savefile) && (count++ < 100)) {
                strnfmt(new_savefile, sizeof(new_savefile), "%s%u%u.new", path,Rand_simple(1000000),count);
        }
        file = file_open(new_savefile, MODE_WRITE, FTYPE_SAVE);
        safe_setuid_drop();

	if (file)
	{
		file_write(file, (char *) &savefile_magic, 4);
		file_write(file, (char *) &savefile_name, 4);

		character_saved = try_save(file);
		file_close(file);
	}

	if (character_saved)
	{
		bool err = FALSE;

		safe_setuid_grab();

		if (file_exists(savefile) && !file_move(savefile, old_savefile))
			err = TRUE;

		if (!err)
		{
			if (!file_move(new_savefile, savefile))
				err = TRUE;

			if (err)
				file_move(old_savefile, savefile);
			else
				file_delete(old_savefile);
		} 

		safe_setuid_drop();

                return err ? FALSE : TRUE;
        }

        /* Delete temp file if the save failed */
        if (file)
        {
                /* file is no longer valid, but it still points to a non zero
                 * value if the file was created above */
                safe_setuid_grab();
                file_delete(new_savefile);
                safe_setuid_drop();
        }
        return FALSE;
}



/*** Savefiel loading functions ***/

static bool try_load(ang_file *f)
{
        byte savefile_head[SAVEFILE_HEAD_SIZE];
        u32b block_version, block_size;
        char *block_name;

        while (TRUE)
        {
                size_t i;
                int (*loader)(void) = NULL;

                /* Load in the next header */
                size_t size = file_read(f, (char *)savefile_head, SAVEFILE_HEAD_SIZE);
                if (!size)
                        break;

                if (size != SAVEFILE_HEAD_SIZE || savefile_head[15] != 0) {
                        note("Savefile is corrupted -- block header mangled.");
                        return FALSE;
                }

#define RECONSTRUCT_U32B(from) \
                ((u32b) savefile_head[from]) | \
                ((u32b) savefile_head[from+1] << 8) | \
                ((u32b) savefile_head[from+2] << 16) | \
                ((u32b) savefile_head[from+3] << 24);

                block_name = (char *) savefile_head;
                block_version = RECONSTRUCT_U32B(16);
                block_size = RECONSTRUCT_U32B(20);

                /* pad to 4 bytes */
                if (block_size % 4)
                        block_size += 4 - (block_size % 4);

                /* Find the right loader */
                for (i = 0; i < N_ELEMENTS(loaders); i++) {
                        if (streq(block_name, loaders[i].name) &&
                                        block_version == loaders[i].version) {
                                loader = loaders[i].load;
                        }
                }

                if (!loader) {
                        /* No loader found */
                        note("Savefile too old.  Try importing it into an older Angband first.");
                        return FALSE;
                }

                /* Allocate space for the buffer */
                buffer = mem_alloc(block_size);
                buffer_pos = 0;
                buffer_check = 0;

                buffer_size = file_read(f, (char *) buffer, block_size);
                if (buffer_size != block_size) {
                        note("Savefile is corrupted -- not enough bytes.");
                        mem_free(buffer);
                        return FALSE;
                }

                /* Try loading */
                if (loader() != 0) {
                        note("Savefile is corrupted.");
                        mem_free(buffer);
                        return FALSE;
                }

                mem_free(buffer);
        }

        /* Still alive */
        if (p_ptr->chp >= 0)
        {
		/* Reset cause of death */
                my_strcpy(p_ptr->died_from, "(alive and well)", sizeof(p_ptr->died_from));
        }

        return TRUE;
}


/**
 * Load a savefile.
 */
bool savefile_load(const char *path)
{
	byte head[8];
	bool ok = TRUE;

	ang_file *f = file_open(path, MODE_READ, -1);
	if (f) {
		if (file_read(f, (char *) &head, 8) == 8 &&
				memcmp(&head[0], savefile_magic, 4) == 0 &&
				memcmp(&head[4], savefile_name, 4) == 0) {
			if (!try_load(f)) {
				ok = FALSE;
				note("Failed loading savefile.");
			}
		} else {
			ok = FALSE;
			note("Savefile is corrupted -- incorrect file header.");
		}

		file_close(f);
	} else {
		ok = FALSE;
		note("Couldn't open savefile.");
	}

	return ok;
}
