#ifndef __LPM__H__
#define __LPM__H__
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif


#define LPM_NAMESIZE                32


#define LPM_MAX_DEPTH               32


#define LPM_TBL24_NUM_ENTRIES       (1 << 24)


#define LPM_TBL8_GROUP_NUM_ENTRIES  256


#define LPM_MAX_TBL8_NUM_GROUPS         (1 << 24)


#define LPM_TBL8_NUM_GROUPS         256


#define LPM_TBL8_NUM_ENTRIES        (LPM_TBL8_NUM_GROUPS *  LPM_TBL8_GROUP_NUM_ENTRIES)


#define LPM_RETURN_IF_TRUE(cond, retval) do { \
    if (cond) return (retval);                \
} while (0)


#define LPM_VALID_EXT_ENTRY_BITMASK 0x03000000


#define LPM_LOOKUP_SUCCESS          0x01000000

struct lpm_tbl_entry {
    
    uint32_t next_hop    :24;
    
    uint32_t valid       :1;   
    
    uint32_t valid_group :1;
    uint32_t depth       :6; 
};


struct lpm_config {
    uint32_t max_rules;      
    uint32_t number_tbl8s;   
    int flags;               
};


struct lpm_rule {
    uint32_t ip; 
    uint32_t next_hop; 
};


struct lpm_rule_info {
    uint32_t used_rules; 
    uint32_t first_rule; 
};


struct lpm {
    
    char name[LPM_NAMESIZE];        
    uint32_t max_rules; 
    uint32_t number_tbl8s; 
    struct lpm_rule_info rule_info[LPM_MAX_DEPTH]; 

    
    struct lpm_tbl_entry tbl24[LPM_TBL24_NUM_ENTRIES];
    struct lpm_tbl_entry *tbl8; 
    struct lpm_rule *rules_tbl; 
};


struct lpm *
lpm_create(const char *name, const struct lpm_config *config);


struct lpm *
lpm_find_existing(const char *name);


void
lpm_free(struct lpm *lpm);


int
lpm_add(struct lpm *lpm, uint32_t ip, uint8_t depth, uint32_t next_hop);


int
lpm_is_rule_present(struct lpm *lpm, uint32_t ip, uint8_t depth,
uint32_t *next_hop);


int
lpm_delete(struct lpm *lpm, uint32_t ip, uint8_t depth);


void
lpm_delete_all(struct lpm *lpm);


static inline int
lpm_lookup(struct lpm *lpm, uint32_t ip, uint32_t *next_hop)
{
    unsigned tbl24_index = (ip >> 8);
    uint32_t tbl_entry;
    const uint32_t *ptbl;

    
    LPM_RETURN_IF_TRUE(((lpm == NULL) || (next_hop == NULL)), -EINVAL);

    
    ptbl = (const uint32_t *)(&lpm->tbl24[tbl24_index]);
    tbl_entry = *ptbl;

    
    
    if (unlikely((tbl_entry & LPM_VALID_EXT_ENTRY_BITMASK) ==
            LPM_VALID_EXT_ENTRY_BITMASK)) {

        unsigned tbl8_index = (uint8_t)ip +
                (((uint32_t)tbl_entry & 0x00FFFFFF) * LPM_TBL8_GROUP_NUM_ENTRIES);
        ptbl = (const uint32_t *)&lpm->tbl8[tbl8_index];
        tbl_entry = *ptbl;
    }

    *next_hop = ((uint32_t)tbl_entry & 0x00FFFFFF);
    return (tbl_entry & LPM_LOOKUP_SUCCESS) ? 0 : -ENOENT;
}


#define lpm_lookup_bulk(lpm, ips, next_hops, n) \
        lpm_lookup_bulk_func(lpm, ips, next_hops, n)

static inline int
lpm_lookup_bulk_func(const struct lpm *lpm, const uint32_t *ips,
        uint32_t *next_hops, const unsigned n)
{
    unsigned i;
    unsigned tbl24_indexes[n];
    const uint32_t *ptbl;

    
    LPM_RETURN_IF_TRUE(((lpm == NULL) || (ips == NULL) ||
            (next_hops == NULL)), -EINVAL);

    for (i = 0; i < n; i++) {
        tbl24_indexes[i] = ips[i] >> 8;
    }

    for (i = 0; i < n; i++) {
        
        ptbl = (const uint32_t *)&lpm->tbl24[tbl24_indexes[i]];
        next_hops[i] = *ptbl;

        
        if (unlikely((next_hops[i] & LPM_VALID_EXT_ENTRY_BITMASK) ==
                LPM_VALID_EXT_ENTRY_BITMASK)) {

            unsigned tbl8_index = (uint8_t)ips[i] +
                    (((uint32_t)next_hops[i] & 0x00FFFFFF) *
                     LPM_TBL8_GROUP_NUM_ENTRIES);

            ptbl = (const uint32_t *)&lpm->tbl8[tbl8_index];
            next_hops[i] = *ptbl;
        }
    }
    return 0;
}


#define     LPM_MASKX4_RES    UINT64_C(0x00ffffff00ffffff)

#ifdef __cplusplus
}
#endif

#endif 
