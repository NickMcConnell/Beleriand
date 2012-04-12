/*
 * File: options.c
 * Purpose: Options table and definitions.
 *
 * Copyright (c) 1997 Ben Harrison
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
#include "angband.h"
#include "option.h"

/**
 * Option screen interface
 */
const int option_page[OPT_PAGE_MAX][OPT_PAGE_PER] =
  {
    /*** Interface ***/
    
      {
	  OPT_use_sound,
	  OPT_rogue_like_commands,
	  OPT_use_old_target,
	  OPT_pickup_always,
	  OPT_pickup_inven,
	  OPT_pickup_detail,
	  OPT_pickup_single,
	  OPT_hide_squelchable,
	  OPT_squelch_worthless,
	  OPT_easy_open,
	  OPT_easy_alter,
	  OPT_show_lists,
	  OPT_show_menus,
	  OPT_mouse_movement,
	  OPT_mouse_buttons,
	  OPT_xchars_to_file
      },

    /*** Display ***/
    
      {
	  OPT_hp_changes_colour,
	  OPT_highlight_player,
	  OPT_center_player,
	  OPT_show_piles,
	  OPT_show_flavors,
	  OPT_show_labels,
	  OPT_show_weights,
	  OPT_show_detect,
	  OPT_view_yellow_light,
	  OPT_view_perma_grids,
	  OPT_view_torch_grids,
	  OPT_animate_flicker,
	  OPT_NONE,
	  OPT_NONE,
	  OPT_NONE,
	  OPT_NONE
      },
    
    /*** Warning ***/
    
      {
	  OPT_run_ignore_stairs,
	  OPT_run_ignore_doors,
	  OPT_run_cut_corners,
	  OPT_run_use_corners,
	  OPT_disturb_move,
	  OPT_disturb_near,
	  OPT_disturb_panel,
	  OPT_disturb_detect,
	  OPT_disturb_state,
	  OPT_quick_messages,
	  OPT_verify_destroy,
	  OPT_ring_bell,
	  OPT_auto_more,
	  OPT_flush_failure,
	  OPT_flush_disturb,
	  OPT_notify_recharge
      },
    
    /*** Birth ***/
    
    {
      OPT_birth_ironman, 
      OPT_birth_thrall,	      
      OPT_birth_small_device,
      OPT_birth_dungeon,
      OPT_birth_no_artifacts,
      OPT_birth_no_stairs,
      OPT_birth_ai_cheat,
      OPT_birth_auto_scum,
      OPT_NONE,
      OPT_NONE,
      OPT_NONE,
      OPT_NONE,
      OPT_NONE,
      OPT_NONE,
      OPT_NONE,
      OPT_NONE
    },
    
     /*** Cheat ***/
    
    {
      OPT_cheat_peek,
      OPT_cheat_hear,
      OPT_cheat_room,
      OPT_cheat_xtra,
      OPT_cheat_know,
      OPT_cheat_live,
      OPT_NONE,
      OPT_NONE,
      OPT_NONE,
      OPT_NONE,
      OPT_NONE,
      OPT_NONE,
      OPT_NONE,
      OPT_NONE,
      OPT_NONE,
      OPT_NONE
    }
  };


typedef struct
{
	const char *name;
	const char *description;
	bool normal;
} option_entry;

static option_entry options[OPT_MAX] =
{
    {"use_sound",             "Play sounds in game",
     FALSE /* 0 */},
    {"rogue_like_commands",   "Rogue-like commands",
     FALSE /* 1 */},
    {"use_old_target",        "Use old target by default",
     FALSE /* 2 */},
    {"pickup_always",         "Pick things up by default",
     TRUE  /* 3 */},
    {"pickup_inven",          "Always pickup items matching inventory",
     TRUE  /* 4 */},
    {"pickup_detail",         "Be verbose when picking things up",           
     TRUE  /* 5 */},
    {"pickup_single",         "Automatically pickup single items",
     TRUE  /* 6 */},
    {"hide_squelchable",      "Hide items set as squelchable",
     TRUE  /* 7 */},
    {"squelch_worthless",     "Squelch worthless items automatically",
     FALSE /* 8 */},
    {"easy_open",             "Open/close/disarm without direction",
     TRUE  /* 9 */},
    {"easy_alter",            "Open/close/disarm on movement",
     FALSE /* 10 */},
    {"show_lists",            "Automatically show lists for commands",
     TRUE  /* 11 */},
    {"show_menus",            "Enter key brings up command menu",
     TRUE  /* 12 */},
    {"mouse_movement",        "Allow mouse clicks to move the player",       
     FALSE /* 13 */},
    {"mouse_buttons",         "Mouse commands are enabled",
     TRUE  /* 14 */},
    {"xchars_to_file",        "Allow accents in output files",
     FALSE /* 15 */},
    {"hp_changes_colour",     "Player colour indicates low hit points",
     TRUE  /* 16 */},
    {"highlight_player",      "Highlight the player with the cursor",
     FALSE /* 17 */},
    {"center_player",         "Keep the player centered (slow)",
     FALSE /* 18 */},
    {"show_piles",            "Show stacks using special attr/char",
     FALSE /* 19 */},
    {"show_flavors",          "Show flavors in object descriptions",
     TRUE  /* 20 */},
    {"show_labels",           "Show labels in equipment listings",
     TRUE  /* 21 */},
    {"show_weights",          "Show weights in all object listings",
     TRUE  /* 22 */},
    {"show_detect",           "Show detection region",
     TRUE  /* 23 */},
    {"view_yellow_light",     "Use special colors for torch light",
     FALSE /* 24 */},
    {"view_bright_light",     "Use special colors for field of view",
     TRUE  /* 25 */},
    {"view_granite_light",    "Use special colors for wall grids",
     FALSE /* 26 */},
    {"view_special_light",    "Use special colors for floor grids",
     TRUE  /* 27 */},
    {"view_perma_grids",      "Map remembers all perma-lit grids",
     TRUE  /* 28 */},
    {"view_torch_grids",      "Map remembers all torch-lit grids",
     TRUE  /* 29 */},
    {"animate_flicker",       "Animate multi-colored monsters and items",    
     FALSE /* 30 */},
    {NULL,                     NULL,
     FALSE /* 31 */},
    {"run_ignore_stairs",     "When running, ignore stairs",
     TRUE  /* 32 */},
    {"run_ignore_doors",      "When running, ignore doors",
     TRUE  /* 33 */},
    {"run_cut_corners",       "When running, cut corners",
     TRUE  /* 34 */},
    {"run_use_corners",       "When running, use corners",
     TRUE  /* 35 */},
    {"disturb_move",          "Disturb whenever any monster moves",
     TRUE  /* 36 */},
    {"disturb_near",          "Disturb whenever viewable monster moves",
     TRUE  /* 37 */},
    {"disturb_panel",         "Disturb whenever map panel changes",
     TRUE  /* 38 */},
    {"disturb_trap_detect",   "Disturb when leaving last trap detect area",
     TRUE  /* 39 */},
    {"disturb_state",         "Disturb whenever player state changes",
     TRUE  /* 40 */},
    {"quick_messages",        "Activate quick messages",
     TRUE  /* 41 */},
    {"verify_destroy",        "Verify destruction of objects",
     TRUE  /* 42 */},
    {"ring_bell",             "Audible bell (on errors, etc)",
     TRUE  /* 43 */},
    {"auto_more",             "Automatically clear '-more-' prompts",
     FALSE /* 44 */},
    {"flush_failure",         "Flush input on various failures",
     TRUE  /* 45 */},
    {"flush_disturb",         "Flush input whenever disturbed",
     FALSE /* 46 */},
    {"notify_recharge",       "Notify on object recharge",
     FALSE /* 47 */},
    {NULL,                     NULL,
     FALSE /* 48 */},
    {NULL,                     NULL,
     FALSE /* 49 */},
    {NULL,                     NULL,
     FALSE /* 50 */},
    {NULL,                     NULL,
     FALSE /* 51 */},
    {NULL,                     NULL,
     FALSE /* 52 */},
    {NULL,                     NULL,
     FALSE /* 53 */},
    {NULL,                     NULL,
     FALSE /* 54 */},
    {NULL,                     NULL,
     FALSE /* 55 */},
    {NULL,                     NULL,
     FALSE /* 56 */},
    {NULL,                     NULL,
     FALSE /* 57 */},
    {NULL,                     NULL,
     FALSE /* 58 */},
    {NULL,                     NULL,
     FALSE /* 59 */},
    {NULL,                     NULL,
     FALSE /* 60 */},
    {NULL,                     NULL,
     FALSE /* 61 */},
    {NULL,                     NULL,
     FALSE /* 62 */},
    {NULL,                     NULL,
     FALSE /* 63 */},
    {NULL,                     NULL,
     FALSE /* 64 */},
    {NULL,                     NULL,
     FALSE /* 65 */},
    {NULL,                     NULL,
     FALSE /* 66 */},
    {NULL,                     NULL,
     FALSE /* 67 */},
    {NULL,                     NULL,
     FALSE /* 68 */},
    {NULL,                     NULL,
     FALSE /* 69 */},
    {NULL,                     NULL,
     FALSE /* 70 */},
    {NULL,                     NULL,
     FALSE /* 71 */},
    {NULL,                     NULL,
     FALSE /* 72 */},
    {NULL,                     NULL,
     FALSE /* 73 */},
    {NULL,                     NULL,
     FALSE /* 74 */},
    {NULL,                     NULL,
     FALSE /* 75 */},
    {NULL,                     NULL,
     FALSE /* 76 */},
    {NULL,                     NULL,
     FALSE /* 77 */},
    {NULL,                     NULL,
     FALSE /* 78 */},
    {NULL,                     NULL,
     FALSE /* 79 */},
    {NULL,                     NULL,
     FALSE /* 80 */},
    {NULL,                     NULL,
     FALSE /* 81 */},
    {NULL,                     NULL,
     FALSE /* 82 */},
    {NULL,                     NULL,
     FALSE /* 83 */},
    {NULL,                     NULL,
     FALSE /* 84 */},
    {NULL,                     NULL,
     FALSE /* 85 */},
    {NULL,                     NULL,
     FALSE /* 86 */},
    {NULL,                     NULL,
     FALSE /* 87 */},
    {NULL,                     NULL,
     FALSE /* 88 */},
    {NULL,                     NULL,
     FALSE /* 89 */},
    {NULL,                     NULL,
     FALSE /* 90 */},
    {NULL,                     NULL,
     FALSE /* 91 */},
    {NULL,                     NULL,
     FALSE /* 92 */},
    {NULL,                     NULL,
     FALSE /* 93 */},
    {NULL,                     NULL,
     FALSE /* 94 */},
    {NULL,                     NULL,
     FALSE /* 95 */},
    {NULL,                     NULL,
     FALSE /* 96 */},
    {NULL,                     NULL,
     FALSE /* 97 */},
    {NULL,                     NULL,
     FALSE /* 98 */},
    {NULL,                     NULL,
     FALSE /* 99 */},
    {NULL,                     NULL,
     FALSE /* 100 */},
    {NULL,                     NULL,
     FALSE /* 101 */},
    {NULL,                     NULL,
     FALSE /* 102 */},
    {NULL,                     NULL,
     FALSE /* 103 */},
    {NULL,                     NULL,
     FALSE /* 104 */},
    {NULL,                     NULL,
     FALSE /* 105 */},
    {NULL,                     NULL,
     FALSE /* 106 */},
    {NULL,                     NULL,
     FALSE /* 107 */},
    {NULL,                     NULL,
     FALSE /* 108 */},
    {NULL,                     NULL,
     FALSE /* 109 */},
    {NULL,                     NULL,
     FALSE /* 110 */},
    {NULL,                     NULL,
     FALSE /* 111 */},
    {NULL,                     NULL,
     FALSE /* 112 */},
    {NULL,                     NULL,
     FALSE /* 113 */},
    {NULL,                     NULL,
     FALSE /* 114 */},
    {NULL,                     NULL,
     FALSE /* 115 */},
    {NULL,                     NULL,
     FALSE /* 116 */},
    {NULL,                     NULL,
     FALSE /* 117 */},
    {NULL,                     NULL,
     FALSE /* 118 */},
    {NULL,                     NULL,
     FALSE /* 119 */},
    {NULL,                     NULL,
     FALSE /* 120 */},
    {NULL,                     NULL,
     FALSE /* 121 */},
    {NULL,                     NULL,
     FALSE /* 122 */},
    {NULL,                     NULL,
     FALSE /* 123 */},
    {NULL,                     NULL,
     FALSE /* 124 */},
    {NULL,                     NULL,
     FALSE /* 125 */},
    {NULL,                     NULL,
     FALSE /* 126 */},
    {NULL,                     NULL,
     FALSE /* 127 */},
    {"birth_point_based",     "Birth: Use point based character generation",
     TRUE /* 128 */},
    {"birth_auto_roller",     "Birth: Use Autoroller if rolling for stats",
     FALSE /* 129 */},
    {"birth_take_notes",      "Birth: Have notes written to a file",
     TRUE  /* 130 */},
    {"birth_preserve",        "Birth: No special feelings/artifacts preserved",
     TRUE  /* 131 */},
    {"birth_no_sell",         "Birth: No selling to stores",
     FALSE /* 132 */},
    {"birth_ironman",         "Birth: Never return to less danger",
     FALSE /* 133 */},
    {"birth_thrall",          "Birth: Start as a thrall at the gate of Angband",
     FALSE /* 134 */},
    {"birth_small_device",    "Birth: View and spell distances halved",
     FALSE /* 135 */},
    {"birth_dungeon",         "Birth: Play with no wilderness",
     FALSE /* 136 */},
    {"birth_no_artifacts",    "Birth: Restrict creation of artifacts",
     FALSE /* 137 */},
    {"birth_no_stairs",       "Birth: Generate levels with disconnected stairs",
     FALSE  /* 138 */},
    {"birth_ai_cheat",        "Birth: Monsters exploit players weaknesses",
     FALSE /* 139 */},
    {"birth_auto_scum",       "Birth: Auto-scum for good levels",
     FALSE /* 140 */},
    {NULL,                     NULL,
     FALSE /* 141 */},
    {NULL,                     NULL,
     FALSE /* 142 */},
    {NULL,                     NULL,
     FALSE /* 143 */},
    {NULL,                     NULL,
     FALSE /* 144 */},
    {NULL,                     NULL,
     FALSE /* 145 */},
    {NULL,                     NULL,
     FALSE /* 146 */},
    {NULL,                     NULL,
     FALSE /* 147 */},
    {NULL,                     NULL,
     FALSE /* 148 */},
    {NULL,                     NULL,
     FALSE /* 149 */},
    {NULL,                     NULL,
     FALSE /* 150 */},
    {NULL,                     NULL,
     FALSE /* 151 */},
    {NULL,                     NULL,
     FALSE /* 152 */},
    {NULL,                     NULL,
     FALSE /* 153 */},
    {NULL,                     NULL,
     FALSE /* 154 */},
    {NULL,                     NULL,
     FALSE /* 155 */},
    {NULL,                     NULL,		
     FALSE /* 156 */},
    {NULL,                     NULL,		
     FALSE /* 157 */},
    {NULL,                     NULL,		
     FALSE /* 158 */},
    {NULL,                     NULL,		
     FALSE /* 159 */},
    {"cheat_peak",            "Cheat: Peek into object creation",
     FALSE /* 160 */},
    {"cheat_hear",            "Cheat: Peek into monster creation",
     FALSE /* 161 */},
    {"cheat_room",            "Cheat: Peek into dungeon creation",
     FALSE /* 162 */},
    {"cheat_xtra",            "Cheat: Peek into something else",
     FALSE /* 163 */},
    {"cheat_know",            "Cheat: Know complete monster info",
     FALSE /* 164 */},
    {"cheat_live",            "Cheat: Allow player to avoid death",
     FALSE /* 165 */},
    {NULL,                     NULL,
     FALSE /* 166 */},
    {NULL,                     NULL,
     FALSE /* 167 */},
    {NULL,                     NULL,
     FALSE /* 168 */},
    {NULL,                     NULL,
     FALSE /* 169 */},
    {NULL,                     NULL,
     FALSE /* 170 */},
    {NULL,                     NULL,
     FALSE /* 171 */},
    {NULL,                     NULL,
     FALSE /* 172 */},
    {NULL,                     NULL,
     FALSE /* 173 */},
    {NULL,                     NULL,
     FALSE /* 174 */},
    {NULL,                     NULL,
     FALSE /* 175 */},
    {NULL,                     NULL,
     FALSE /* 176 */},
    {NULL,                     NULL,
     FALSE /* 177 */},
    {NULL,                     NULL,
     FALSE /* 178 */},
    {NULL,                     NULL,
     FALSE /* 179 */},
    {NULL,                     NULL,
     FALSE /* 180 */},
    {NULL,                     NULL,
     FALSE /* 181 */},
    {NULL,                     NULL,
     FALSE /* 182 */},
    {NULL,                     NULL,
     FALSE /* 183 */},
    {NULL,                     NULL,
     FALSE /* 184 */},
    {NULL,                     NULL,
     FALSE /* 185 */},
    {NULL,                     NULL,
     FALSE /* 186 */},
    {NULL,                     NULL,
     FALSE /* 187 */},
    {NULL,                     NULL,
     FALSE /* 188 */},
    {NULL,                     NULL,
     FALSE /* 189 */},
    {NULL,                     NULL,
     FALSE /* 190 */},
    {NULL,                     NULL,
     FALSE /* 191 */},
    {"adult_point_based",     "Adult: Use point based character generation",
     TRUE  /* 192 */},
    {"adult_auto_roller",     "Adult: Use Autoroller if rolling for stats",
     TRUE  /* 193 */},
    {"adult_take_notes",      "Adult: Have notes written to a file",
     TRUE  /* 194 */},
    {"adult_preserve",        "Adult: Artifacts preserved, no special feelings",
     TRUE  /* 195 */},
    {"adult_no_sell",         "Adult: No selling to stores",
     FALSE /* 196 */},
    {"adult_ironman",         "Adult: Never return to less danger",
     FALSE /* 197 */},
    {"adult_thrall",          "Adult: Start as a thrall at the gate of Angband",
     FALSE /* 198 */},
    {"adult_small_device",    "Adult: View and spell distances halved",
     FALSE /* 199 */},
    {"adult_dungeon",         "Adult: Play with no wilderness",
     FALSE /* 200 */},
    {"adult_no_artifacts",    "Adult: Restrict creation of artifacts",
     FALSE /* 201 */},
    {"adult_no_stairs",       "Adult: Generate levels with disconnected stairs",
     TRUE  /* 202 */},
    {"adult_ai_cheat",        "Adult: Monsters exploit players weaknesses",
     FALSE /* 203 */},
    {"adult_auto_scum",       "Adult: Auto-scum for good levels",
     FALSE /* 204 */},
    {NULL,                     NULL,
     FALSE /* 205 */},
    {NULL,                     NULL,
     FALSE /* 206 */},
    {NULL,                     NULL,
     FALSE /* 207 */},
    {NULL,                     NULL,
     FALSE /* 208 */},
    {NULL,                     NULL,
     FALSE /* 209 */},
    {NULL,                     NULL,
     FALSE /* 210 */},
    {NULL,                     NULL,
     FALSE /* 211 */},
    {NULL,                     NULL,
     FALSE /* 212 */},
    {NULL,                     NULL,
     FALSE /* 213 */},
    {NULL,                     NULL,
     FALSE /* 214 */},
    {NULL,                     NULL,
     FALSE /* 215 */},
    {NULL,                     NULL,
     FALSE /* 216 */},
    {NULL,                     NULL,
     FALSE /* 217 */},
    {NULL,                     NULL,
     FALSE /* 218 */},
    {NULL,                     NULL,
     FALSE /* 219 */},
    {NULL,                     NULL,
     FALSE /* 220 */},
    {NULL,                     NULL,	
     FALSE /* 221 */},
    {NULL,                     NULL,	
     FALSE /* 222 */},
    {NULL,                     NULL,	
     FALSE /* 223 */},
    {"score_peek",            "Score: Peek into object creation",
     FALSE /* 224 */},
    {"score_hear",            "Score: Peek into monster creation",
     FALSE /* 225 */},
    {"score_room",            "Score: Peek into dungeon creation",
     FALSE /* 226 */},
    {"score_xtra",            "Score: Peek into something else",
     FALSE /* 227 */},
    {"score_know",            "Score: Know complete monster info",
     FALSE /* 228 */},
    {"score_live",            "Score: Allow player to avoid death",
     FALSE /* 229 */},
    {NULL,                     NULL,	
     FALSE /* 230 */},
    {NULL,                     NULL,
     FALSE /* 231 */},
    {NULL,                     NULL,
     FALSE /* 232 */},
    {NULL,                     NULL,
     FALSE /* 233 */},
    {NULL,                     NULL,
     FALSE /* 234 */},
    {NULL,                     NULL,
     FALSE /* 235 */},
    {NULL,                     NULL,
     FALSE /* 236 */},
    {NULL,                     NULL,
     FALSE /* 237 */},
    {NULL,                     NULL,
     FALSE /* 238 */},
    {NULL,                     NULL,
     FALSE /* 239 */},
    {NULL,                     NULL,
     FALSE /* 240 */},
    {NULL,                     NULL,
     FALSE /* 241 */},
    {NULL,                     NULL,
     FALSE /* 242 */},
    {NULL,                     NULL,
     FALSE /* 243 */},
    {NULL,                     NULL,
     FALSE /* 244 */},
    {NULL,                     NULL,
     FALSE /* 245 */},
    {NULL,                     NULL,
     FALSE /* 246 */},
    {NULL,                     NULL,
     FALSE /* 247 */},
    {NULL,                     NULL,
     FALSE /* 248 */},
    {NULL,                     NULL,
     FALSE /* 249 */},
    {NULL,                     NULL,
     FALSE /* 250 */},
    {NULL,                     NULL,
     FALSE /* 251 */},
    {NULL,                     NULL,
     FALSE /* 252 */},
    {NULL,                     NULL,
     FALSE /* 253 */},
    {NULL,                     NULL,
     FALSE /* 254 */},
    {NULL,                     NULL,
     FALSE /* 255 */}
};


/* Accessor functions */
const char *option_name(int opt)
{
	if (opt >= OPT_MAX) return NULL;
	return options[opt].name;
}

const char *option_desc(int opt)
{
	if (opt >= OPT_MAX) return NULL;
	return options[opt].description;
}

/* Setup functions */
bool option_set(const char *name, bool on)
{
	size_t opt;
	for (opt = 0; opt < OPT_ADULT; opt++)
	{
		if (!options[opt].name || !streq(options[opt].name, name))
			continue;

		op_ptr->opt[opt] = on;
		if (on && opt > OPT_CHEAT && opt < OPT_ADULT)
			op_ptr->opt[opt + (OPT_SCORE - OPT_CHEAT)] = TRUE;

		return TRUE;
	}

	return FALSE;
}

void option_set_defaults(void)
{
	size_t opt;
	for (opt = 0; opt < OPT_MAX; opt++)
		op_ptr->opt[opt] = options[opt].normal;
}
