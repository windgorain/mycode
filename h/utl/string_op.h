#ifndef _STRING_OP_H
#define _STRING_OP_H

#include <stdbool.h>
#include <string.h>
#include <strings.h>
#include "types.h"

#define TMPV(name) name##__func__##__LINE__

#define SENSE_VALUE 0
#define SP_VALUE    1
#define CRLF_VALUE  2

extern unsigned char __check_ascii[0x100];

#define IS_SPACE(ch) (__check_ascii[(uint8_t)(ch)] == SP_VALUE)
#define IS_CRLF(ch) (__check_ascii[(uint8_t)(ch)] == CRLF_VALUE)
#define IS_SENSE(ch) (__check_ascii[(uint8_t)(ch)] == SENSE_VALUE)
#define IS_NOSENSE(ch)  (!IS_SENSE(ch))

#define FOR_INC(a, b)  int a; for(a = 0; a < b; a++)
#define FOR_DEC(a, b)  int a; for(a = b; a >= 0; a--)

#define SEARCH_SENSE_FUNC(__SENSE_MACRO)               \
	__extension__ ({                                   \
		char *TMPV(sense) = NULL;                      \
		if (likely(begin && num_search > 0)) {            \
			if (is_forward) {                          \
				FOR_INC(_jdx_, num_search) {              \
					if (__SENSE_MACRO(begin[_jdx_])) { \
						TMPV(sense) = begin + _jdx_;   \
						break;                         \
					}                                  \
				}                                      \
			} else {                                   \
				FOR_DEC(_jdx_, num_search) {              \
					if (__SENSE_MACRO(begin[_jdx_])) { \
						TMPV(sense) = begin + _jdx_;   \
						break;                         \
					}                                  \
				}                                      \
			}                                          \
		}                                              \
		TMPV(sense);                                   \
	})


static inline char *_search_nosense(char *begin, int num_search, bool is_forward)
{
	return SEARCH_SENSE_FUNC(IS_NOSENSE);
}
#define search_nosense(begin, num_search)      \
	_search_nosense((char *)(begin), (int)(num_search), true)
#define search_nosense_r(begin, num_search)    \
	_search_nosense((char *)(begin), (int)(num_search), false)


static inline char *_search_sense(char *begin, int num_search, bool is_forward)
{
    return SEARCH_SENSE_FUNC(IS_SENSE);
}
#define search_sense(begin, num_search)  \
	_search_sense((char *)(begin), (int)(num_search), true)

#define search_sense_r(begin, num_search)    \
	_search_sense((char *)(begin), (int)(num_search), false)

#define SKIP_SPACE(__data_begin, __data_end)    \
	__data_begin = search_sense(__data_begin, __data_end - __data_begin)

static inline char *lstrim(char *str, int *str_len)
{
	char *end = str + *str_len;
	str = search_sense(str, *str_len);
	if (str) {
		*str_len = (int)(end - str);
	} else {
		*str_len = 0;
	}

	return str;
}

static inline char *strim(char *str, int *str_len)
{
	char *pline = search_sense(str, *str_len);
	if (pline == NULL) {
		*str_len = 0;
		return NULL;
	}

	char *p = search_sense_r(pline, str + *str_len - pline);
	if (p != NULL) {
		*str_len = (int)(p + 1 - pline);
	} else {
		*str_len = 0;
	}

	return pline;
}

static inline void rstrim(char *str, int *str_len)
{
	char *p = search_sense_r(str, *str_len);
	if (p != NULL) {
		*str_len = (int)(p + 1 - str);
	} else {
		*str_len = 0;
	}

	return;
}

static inline void rstrim_str(char *str)
{
	

	int str_len = strlen(str);
	char *p = search_sense_r(str, str_len);
	if (p != NULL) {
		if (p + 1 < str + str_len) {
			p[1] = '\0';
		}
	} else {
		str[0] = '\0';
	}

	return;
}

typedef struct _split_field_t {
	char *part_ptr;
	int part_len;
#if __WORDSIZE == 64
	int resv_32;
#endif
} split_field_t;   


int split_pos(char *subject, int len, char ch, split_field_t *split_list, int max_split_num, bool need_strim, bool extend_last_node);

#define SPLIT_POS(__subj, __len, __ch, __list) \
    split_pos(__subj, __len, __ch, ARRAY_LIST(__list), true, false)

#define SPLIT_POS_EX(__subj, __len, __ch, __list) \
    split_pos(__subj, __len, __ch, ARRAY_LIST(__list), true, true)

#define SPLIT_POS_STR(__subj, __ch, __list) \
    SPLIT_POS(__subj, ustrlen(__subj), __ch, __list)

#define SPLIT_POS_EX_STR(__subj, __ch, __list) \
    SPLIT_POS_EX(__subj, ustrlen(__subj), __ch, __list)

int split_sp_pos(char *subject, int len, split_field_t *split_list, int max_split_num, bool extend_last_node);

#define SPLIT_SP_POS(__subj, __len, __list) \
    split_sp_pos(__subj, __len, ARRAY_LIST(__list), false)

#define SPLIT_SP_POS_EX(__subj, __len, __list) \
    split_sp_pos(__subj, __len, ARRAY_LIST(__list), true)

#define SPLIT_SP_POS_STR(__subj, __list) \
    SPLIT_SP_POS(__subj, ustrlen(__subj), __list)

#define SPLIT_SP_POS_EX_STR(__subj, __list) \
    SPLIT_SP_POS_EX(__subj, ustrlen(__subj), __list)

#define fix_case_prefix(a, a_len, b) (a_len >= strlen(b) && (strncmp(a, b, strlen(b)))) 

char *memstr_r(char *haystack, uint32_t haystack_len, char *str, uint32_t str_len);
char *memstr(char *haystack, uint32_t haystack_len, char *str, uint32_t str_len);


#define find_mem_str(buf, buf_sz, str)   memstr(buf, buf_sz, str, strlen(str))
#define find_mem_r_str(buf, buf_sz, str) memstr_r(buf, buf_sz, str, strlen(str))
#endif
