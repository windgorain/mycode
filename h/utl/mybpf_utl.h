/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#ifndef _MYBPF_UTL_H
#define _MYBPF_UTL_H
#ifdef __cplusplus
extern "C"
{
#endif

typedef struct {
    int sec_id;
    int map_def_size;
    int map_count;
    void *maps;
}MYBPF_MAPS_SEC_S;

typedef struct {
	UCHAR opcode;		/* opcode */
	UCHAR dst_reg:4;	/* dest register */
	UCHAR src_reg:4;	/* source register */
	short off;		/* signed offset */
	int imm;		/* signed immediate constant */
}MYBPF_INSN_S;

#ifdef __cplusplus
}
#endif
#endif //MYBPF_UTL_H_
