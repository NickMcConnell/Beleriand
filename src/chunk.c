/*
 * File: chunk.c
 * Purpose: Chunk loading and saving main routines
 *
 * Copyright (c) 2009 Andi Sidwell <andi@takkaria.org>
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
        { "stores", wr_stores, 1 },
        { "dungeon", wr_dungeon, 1 },
        { "objects", wr_objects, 1 },
        { "monsters", wr_monsters, 1 },
        { "traps", wr_traps, 1 },
};

/**
 * Savefile loading functions 
 */
static const struct {
        char name[16];
        int (*load)(void);
        u32b version;
} loaders[] = {
        { "stores", rd_stores, 1 },
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


/** Utility **/


/** Base put/get **/

static void sf_put(byte v)
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

static byte sf_get(void)
{
	assert(buffer != NULL);
	assert(buffer_size > 0);
	assert(buffer_pos < buffer_size);

	buffer_check += buffer[buffer_pos];

	return buffer[buffer_pos++];
}


/*** Savefile saving functions ***/

static bool try_save(ang_file *file)
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
