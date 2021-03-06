/* history.h - player history tracking */

#ifndef HISTORY_H
#define HISTORY_H

void history_clear(void);
size_t history_get_num(void);
bool history_add_full(u16b type, byte a_idx, u16b z_pos, u16b y_pos, 
		      u16b x_pos, s16b clev, s32b turn, const char *text);
bool history_add(const char *event, u16b type, byte a_idx);
bool history_add_artifact(byte a_idx, bool known, bool found);
void history_unmask_unknown(void);
bool history_lose_artifact(byte a_idx);
void history_display(void);
void dump_history(char_attr_line *line, int *curr_line, bool *dead);
bool history_is_artifact_known(byte a_idx);

extern history_info *history_list;

#endif /* !HISTORY_H */
