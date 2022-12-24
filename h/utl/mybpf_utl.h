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
	UCHAR code;		/* opcode */
	UCHAR dst_reg:4;	/* dest register */
	UCHAR src_reg:4;	/* source register */
	USHORT off;		/* signed offset */
	UINT imm;		/* signed immediate constant */
}MYBPF_INSN_S;

#ifdef __cplusplus
}
#endif
#endif //MYBPF_UTL_H_
