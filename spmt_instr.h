#ifndef ST_SPMT_INSTR_H
#define ST_SPMT_INSTR_H

tnle* insert_spawn_instr(cfg_block *block, tnle *spawn_pos, label_sym *cqip_pos_num);
tnle* insert_cqip_instr(cfg_block *, bool before_label = false);
//tnle* insert_cqip_instr_for_loop(cfg_block *, bool before_label = false);
tnle* insert_cqip_instr_for_loop(cfg_block *, tnle *pos, bool before_label = false);
tnle* insert_cancel_instr(cfg_node *, label_sym *);

label_sym* peek_cqip_pos(tnle *item);
label_sym* peek_pslice_entry_pos(tnle *item);
label_sym* peek_pslice_exit_pos(tnle *item);

//tnle* insert_loopbegin_instr(loop_block *lpblock, tnle *loopbegin_pos, label_sym *loopbegin_pos_lab);


bool is_cmp_op(instruction *i);

#endif
