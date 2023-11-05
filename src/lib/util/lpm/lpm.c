#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>

#include "utl/lpm.h"

#define MAX_DEPTH_TBL24 24

enum valid_flag {
    INVALID = 0,
    VALID
};


#if defined(LPM_DEBUG)
#define VERIFY_DEPTH(depth) do {                                \
    if ((depth == 0) || (depth > LPM_MAX_DEPTH))        \
        printf("LPM: Invalid depth (%u) at line %d", \
                (unsigned)(depth), __LINE__);   \
} while (0)
#else
#define VERIFY_DEPTH(depth)
#endif


static uint32_t __attribute__((pure))
depth_to_mask(uint8_t depth)
{
    VERIFY_DEPTH(depth);

    
    return (int)0x80000000 >> (depth - 1);
}


static uint32_t __attribute__((pure))
depth_to_range(uint8_t depth)
{
    VERIFY_DEPTH(depth);

    
    if (depth <= MAX_DEPTH_TBL24)
        return 1 << (MAX_DEPTH_TBL24 - depth);

    
    return 1 << (LPM_MAX_DEPTH - depth);
}


struct lpm *
lpm_create(const char *name, const struct lpm_config *config)
{
    char mem_name[LPM_NAMESIZE];
    struct lpm *lpm = NULL;
    uint32_t mem_size, rules_size, tbl8s_size;
    
    
    if ((name == NULL) || (config->max_rules == 0)
            || config->number_tbl8s > LPM_MAX_TBL8_NUM_GROUPS) {
        errno = EINVAL;
        return NULL;
    }

    snprintf(mem_name, sizeof(mem_name), "LPM_%s", name);

    
    mem_size = sizeof(*lpm);
    rules_size = sizeof(struct lpm_rule) * config->max_rules;
    tbl8s_size = (sizeof(struct lpm_tbl_entry) * LPM_TBL8_GROUP_NUM_ENTRIES * config->number_tbl8s);

    
    lpm = malloc(mem_size);
    if (lpm == NULL) {
        printf("LPM memory allocation failed\n");
        errno = ENOMEM;
        goto exit;
    }

    lpm->rules_tbl = malloc(rules_size);
    if (lpm->rules_tbl == NULL) {
        printf("LPM rules_tbl memory allocation failed\n");
        free(lpm);
        lpm = NULL;
        errno = ENOMEM;
        goto exit;
    }
    
    lpm->tbl8 = malloc((size_t) tbl8s_size);
    if (lpm->tbl8 == NULL) {
        printf("LPM tbl8 memory allocation failed\n");
        free(lpm->rules_tbl);
        free(lpm);
        lpm = NULL;
        errno = ENOMEM;
        goto exit;
    }

    
    lpm->max_rules = config->max_rules;
    lpm->number_tbl8s = config->number_tbl8s;
    int len = sizeof(lpm->name) > LPM_NAMESIZE ? LPM_NAMESIZE -1 : sizeof(lpm->name);
    strncpy(lpm->name, name, len);

exit:
    return lpm;
}


void
lpm_free(struct lpm *lpm)
{
    
    if (lpm == NULL)
        return;

    free(lpm->tbl8);
    free(lpm->rules_tbl);
    free(lpm);
}


static int32_t
rule_add(struct lpm *lpm, uint32_t ip_masked, uint8_t depth,
    uint32_t next_hop)
{
    uint32_t rule_gindex, rule_index, last_rule;
    int i;

    VERIFY_DEPTH(depth);

    
    if (lpm->rule_info[depth - 1].used_rules > 0) {

        
        rule_gindex = lpm->rule_info[depth - 1].first_rule;
        
        rule_index = rule_gindex;
        
        last_rule = rule_gindex + lpm->rule_info[depth - 1].used_rules;

        for (; rule_index < last_rule; rule_index++) {

            
            if (lpm->rules_tbl[rule_index].ip == ip_masked) {
                lpm->rules_tbl[rule_index].next_hop = next_hop;

                return rule_index;
            }
        }

        if (rule_index == lpm->max_rules)
            return -ENOSPC;
    } else {
        
        rule_index = 0;

        for (i = depth - 1; i > 0; i--) {
            if (lpm->rule_info[i - 1].used_rules > 0) {
                rule_index = lpm->rule_info[i - 1].first_rule
                        + lpm->rule_info[i - 1].used_rules;
                break;
            }
        }
        if (rule_index == lpm->max_rules)
            return -ENOSPC;

        lpm->rule_info[depth - 1].first_rule = rule_index;
    }

    
    for (i = LPM_MAX_DEPTH; i > depth; i--) {
        if (lpm->rule_info[i - 1].first_rule
                + lpm->rule_info[i - 1].used_rules == lpm->max_rules)
            return -ENOSPC;

        if (lpm->rule_info[i - 1].used_rules > 0) {
            lpm->rules_tbl[lpm->rule_info[i - 1].first_rule
                + lpm->rule_info[i - 1].used_rules]
                    = lpm->rules_tbl[lpm->rule_info[i - 1].first_rule];
            lpm->rule_info[i - 1].first_rule++;
        }
    }

    
    lpm->rules_tbl[rule_index].ip = ip_masked;
    lpm->rules_tbl[rule_index].next_hop = next_hop;

    
    lpm->rule_info[depth - 1].used_rules++;

    return rule_index;
}


static void
rule_delete(struct lpm *lpm, int32_t rule_index, uint8_t depth)
{
    int i;

    VERIFY_DEPTH(depth);

    lpm->rules_tbl[rule_index] =
            lpm->rules_tbl[lpm->rule_info[depth - 1].first_rule
            + lpm->rule_info[depth - 1].used_rules - 1];

    for (i = depth; i < LPM_MAX_DEPTH; i++) {
        if (lpm->rule_info[i].used_rules > 0) {
            lpm->rules_tbl[lpm->rule_info[i].first_rule - 1] =
                    lpm->rules_tbl[lpm->rule_info[i].first_rule
                        + lpm->rule_info[i].used_rules - 1];
            lpm->rule_info[i].first_rule--;
        }
    }

    lpm->rule_info[depth - 1].used_rules--;
}


static int32_t
rule_find(struct lpm *lpm, uint32_t ip_masked, uint8_t depth)
{
    uint32_t rule_gindex, last_rule, rule_index;

    VERIFY_DEPTH(depth);

    rule_gindex = lpm->rule_info[depth - 1].first_rule;
    last_rule = rule_gindex + lpm->rule_info[depth - 1].used_rules;

    
    for (rule_index = rule_gindex; rule_index < last_rule; rule_index++) {
        
        if (lpm->rules_tbl[rule_index].ip == ip_masked)
            return rule_index;
    }

    
    return -EINVAL;
}


static int32_t
tbl8_alloc(struct lpm_tbl_entry *tbl8, uint32_t number_tbl8s)
{
    uint32_t group_idx; 
    struct lpm_tbl_entry *tbl8_entry;

    
    for (group_idx = 0; group_idx < number_tbl8s; group_idx++) {
        tbl8_entry = &tbl8[group_idx * LPM_TBL8_GROUP_NUM_ENTRIES];
        
        if (!tbl8_entry->valid_group) {
            struct lpm_tbl_entry new_tbl8_entry = {
                .next_hop = 0,
                .valid = INVALID,
                .depth = 0,
                .valid_group = VALID,
            };

            memset(&tbl8_entry[0], 0, LPM_TBL8_GROUP_NUM_ENTRIES * sizeof(tbl8_entry[0]));
            __atomic_store(tbl8_entry, &new_tbl8_entry, __ATOMIC_RELAXED);

            
            return group_idx;
        }
    }

    
    return -ENOSPC;
}

static void
tbl8_free(struct lpm_tbl_entry *tbl8, uint32_t tbl8_group_start)
{
    
    struct lpm_tbl_entry zero_tbl8_entry = {0};
    __atomic_store(&tbl8[tbl8_group_start], &zero_tbl8_entry, __ATOMIC_RELAXED);
}

static int32_t
add_depth_small(struct lpm *lpm, uint32_t ip, uint8_t depth,
        uint32_t next_hop)
{
#define group_idx next_hop
    uint32_t tbl24_index, tbl24_range, tbl8_index, tbl8_group_end, i, j;

    
    tbl24_index = ip >> 8;
    tbl24_range = depth_to_range(depth);

    for (i = tbl24_index; i < (tbl24_index + tbl24_range); i++) {
        
        if (!lpm->tbl24[i].valid || (lpm->tbl24[i].valid_group == 0 &&
                lpm->tbl24[i].depth <= depth)) {

            struct lpm_tbl_entry new_tbl24_entry = {
                .next_hop = next_hop,
                .valid = VALID,
                .valid_group = 0,
                .depth = depth,
            };

            
            __atomic_store(&lpm->tbl24[i], &new_tbl24_entry, __ATOMIC_RELEASE);

            continue;
        }

        if (lpm->tbl24[i].valid_group == 1) {
            
            tbl8_index = lpm->tbl24[i].group_idx * LPM_TBL8_GROUP_NUM_ENTRIES;
            tbl8_group_end = tbl8_index + LPM_TBL8_GROUP_NUM_ENTRIES;
            for (j = tbl8_index; j < tbl8_group_end; j++) {
                if (!lpm->tbl8[j].valid ||
                        lpm->tbl8[j].depth <= depth) {
                    struct lpm_tbl_entry
                        new_tbl8_entry = {
                        .valid = VALID,
                        .valid_group = VALID,
                        .depth = depth,
                        .next_hop = next_hop,
                    };

                    
                    __atomic_store(&lpm->tbl8[j], &new_tbl8_entry, __ATOMIC_RELAXED);
                    continue;
                }
            }
        }
    }
#undef group_idx
    return 0;
}

static int32_t
add_depth_big(struct lpm *lpm, uint32_t ip_masked, uint8_t depth,
        uint32_t next_hop)
{
#define group_idx next_hop
    uint32_t tbl24_index;
    int32_t tbl8_group_index, tbl8_group_start, tbl8_group_end, tbl8_index,
        tbl8_range, i;

    tbl24_index = (ip_masked >> 8);
    tbl8_range = depth_to_range(depth);

    if (!lpm->tbl24[tbl24_index].valid) {
        
        tbl8_group_index = tbl8_alloc(lpm->tbl8, lpm->number_tbl8s);

        
        if (tbl8_group_index < 0) {
            return tbl8_group_index;
        }

        
        tbl8_index = (tbl8_group_index * LPM_TBL8_GROUP_NUM_ENTRIES) + (ip_masked & 0xFF);

        
        for (i = tbl8_index; i < (tbl8_index + tbl8_range); i++) {
            struct lpm_tbl_entry new_tbl8_entry = {
                .valid = VALID,
                .depth = depth,
                .valid_group = lpm->tbl8[i].valid_group,
                .next_hop = next_hop,
            };
            __atomic_store(&lpm->tbl8[i], &new_tbl8_entry, __ATOMIC_RELAXED);
        }

        

        struct lpm_tbl_entry new_tbl24_entry = {
            .group_idx = tbl8_group_index,
            .valid = VALID,
            .valid_group = 1,
            .depth = 0,
        };

        
        __atomic_store(&lpm->tbl24[tbl24_index], &new_tbl24_entry, __ATOMIC_RELEASE);

    } 
    else if (lpm->tbl24[tbl24_index].valid_group == 0) {
        
        tbl8_group_index = tbl8_alloc(lpm->tbl8, lpm->number_tbl8s);

        if (tbl8_group_index < 0) {
            return tbl8_group_index;
        }

        tbl8_group_start = tbl8_group_index * LPM_TBL8_GROUP_NUM_ENTRIES;
        tbl8_group_end = tbl8_group_start + LPM_TBL8_GROUP_NUM_ENTRIES;

        
        for (i = tbl8_group_start; i < tbl8_group_end; i++) {
            struct lpm_tbl_entry new_tbl8_entry = {
                .valid = VALID,
                .depth = lpm->tbl24[tbl24_index].depth,
                .valid_group = lpm->tbl8[i].valid_group,
                .next_hop = lpm->tbl24[tbl24_index].next_hop,
            };
            __atomic_store(&lpm->tbl8[i], &new_tbl8_entry, __ATOMIC_RELAXED);
        }

        tbl8_index = tbl8_group_start + (ip_masked & 0xFF);

        
        for (i = tbl8_index; i < tbl8_index + tbl8_range; i++) {
            struct lpm_tbl_entry new_tbl8_entry = {
                .valid = VALID,
                .depth = depth,
                .valid_group = lpm->tbl8[i].valid_group,
                .next_hop = next_hop,
            };
            __atomic_store(&lpm->tbl8[i], &new_tbl8_entry, __ATOMIC_RELAXED);
        }

        

        struct lpm_tbl_entry new_tbl24_entry = {
                .group_idx = tbl8_group_index,
                .valid = VALID,
                .valid_group = 1,
                .depth = 0,
        };

        
        __atomic_store(&lpm->tbl24[tbl24_index], &new_tbl24_entry,
                __ATOMIC_RELEASE);

    } else { 
        tbl8_group_index = lpm->tbl24[tbl24_index].group_idx;
        tbl8_group_start = tbl8_group_index *
                LPM_TBL8_GROUP_NUM_ENTRIES;
        tbl8_index = tbl8_group_start + (ip_masked & 0xFF);

        for (i = tbl8_index; i < (tbl8_index + tbl8_range); i++) {

            if (!lpm->tbl8[i].valid ||
                    lpm->tbl8[i].depth <= depth) {
                struct lpm_tbl_entry new_tbl8_entry = {
                    .valid = VALID,
                    .depth = depth,
                    .next_hop = next_hop,
                    .valid_group = lpm->tbl8[i].valid_group,
                };

                
                __atomic_store(&lpm->tbl8[i], &new_tbl8_entry, __ATOMIC_RELAXED);

                continue;
            }
        }
    }
#undef group_idx
    return 0;
}


int
lpm_add(struct lpm *lpm, uint32_t ip, uint8_t depth, uint32_t next_hop)
{
    int32_t rule_index, status = 0;
    uint32_t ip_masked;

    
    if ((lpm == NULL) || (depth < 1) || (depth > LPM_MAX_DEPTH))
        return -EINVAL;

    ip_masked = ip & depth_to_mask(depth);

    
    rule_index = rule_add(lpm, ip_masked, depth, next_hop);

    
    if (rule_index < 0) {
        return rule_index;
    }

    if (depth <= MAX_DEPTH_TBL24) {
        status = add_depth_small(lpm, ip_masked, depth, next_hop);
    } else { 
        status = add_depth_big(lpm, ip_masked, depth, next_hop);

        
        if (status < 0) {
            rule_delete(lpm, rule_index, depth);

            return status;
        }
    }

    return 0;
}


int
lpm_is_rule_present(struct lpm *lpm, uint32_t ip, uint8_t depth, uint32_t *next_hop)
{
    uint32_t ip_masked;
    int32_t rule_index;

    
    if ((lpm == NULL) || (next_hop == NULL) || (depth < 1) || (depth > LPM_MAX_DEPTH))
        return -EINVAL;

    
    ip_masked = ip & depth_to_mask(depth);
    rule_index = rule_find(lpm, ip_masked, depth);
    if (rule_index >= 0) {
        *next_hop = lpm->rules_tbl[rule_index].next_hop;
        return 1;
    }

    
    return 0;
}

static int32_t
find_previous_rule(struct lpm *lpm, uint32_t ip, uint8_t depth, uint8_t *sub_rule_depth)
{
    int32_t rule_index;
    uint32_t ip_masked;
    uint8_t prev_depth;

    for (prev_depth = (uint8_t)(depth - 1); prev_depth > 0; prev_depth--) {
        ip_masked = ip & depth_to_mask(prev_depth);

        rule_index = rule_find(lpm, ip_masked, prev_depth);

        if (rule_index >= 0) {
            *sub_rule_depth = prev_depth;
            return rule_index;
        }
    }

    return -1;
}

static int32_t
delete_depth_small(struct lpm *lpm, uint32_t ip_masked,
    uint8_t depth, int32_t sub_rule_index, uint8_t sub_rule_depth)
{
#define group_idx next_hop
    uint32_t tbl24_range, tbl24_index, tbl8_group_index, tbl8_index, i, j;

    
    tbl24_range = depth_to_range(depth);
    tbl24_index = (ip_masked >> 8);
    struct lpm_tbl_entry zero_tbl24_entry = {0};

    
    if (sub_rule_index < 0) {
        
        for (i = tbl24_index; i < (tbl24_index + tbl24_range); i++) {

            if (lpm->tbl24[i].valid_group == 0 &&
                    lpm->tbl24[i].depth <= depth) {
                __atomic_store(&lpm->tbl24[i],
                    &zero_tbl24_entry, __ATOMIC_RELEASE);
            } else if (lpm->tbl24[i].valid_group == 1) {
                

                tbl8_group_index = lpm->tbl24[i].group_idx;
                tbl8_index = tbl8_group_index * LPM_TBL8_GROUP_NUM_ENTRIES;

                for (j = tbl8_index; j < (tbl8_index +
                    LPM_TBL8_GROUP_NUM_ENTRIES); j++) {

                    if (lpm->tbl8[j].depth <= depth)
                        lpm->tbl8[j].valid = INVALID;
                }
            }
        }
    } else {
        

        struct lpm_tbl_entry new_tbl24_entry = {
            .next_hop = lpm->rules_tbl[sub_rule_index].next_hop,
            .valid = VALID,
            .valid_group = 0,
            .depth = sub_rule_depth,
        };

        struct lpm_tbl_entry new_tbl8_entry = {
            .valid = VALID,
            .valid_group = VALID,
            .depth = sub_rule_depth,
            .next_hop = lpm->rules_tbl
            [sub_rule_index].next_hop,
        };

        for (i = tbl24_index; i < (tbl24_index + tbl24_range); i++) {

            if (lpm->tbl24[i].valid_group == 0 &&
                    lpm->tbl24[i].depth <= depth) {
                __atomic_store(&lpm->tbl24[i], &new_tbl24_entry,
                        __ATOMIC_RELEASE);
            } else  if (lpm->tbl24[i].valid_group == 1) {
                

                tbl8_group_index = lpm->tbl24[i].group_idx;
                tbl8_index = tbl8_group_index * LPM_TBL8_GROUP_NUM_ENTRIES;

                for (j = tbl8_index; j < (tbl8_index +
                    LPM_TBL8_GROUP_NUM_ENTRIES); j++) {
                    if (lpm->tbl8[j].depth <= depth)
                        __atomic_store(&lpm->tbl8[j], &new_tbl8_entry, __ATOMIC_RELAXED);
                }
            }
        }
    }
#undef group_idx
    return 0;
}


static int32_t
tbl8_recycle_check(struct lpm_tbl_entry *tbl8,
        uint32_t tbl8_group_start)
{
    uint32_t tbl8_group_end, i;
    tbl8_group_end = tbl8_group_start + LPM_TBL8_GROUP_NUM_ENTRIES;

    
    if (tbl8[tbl8_group_start].valid) {
        
        if (tbl8[tbl8_group_start].depth <= MAX_DEPTH_TBL24) {
            for (i = (tbl8_group_start + 1); i < tbl8_group_end;
                    i++) {

                if (tbl8[i].depth !=
                        tbl8[tbl8_group_start].depth) {

                    return -EEXIST;
                }
            }
            
            return tbl8_group_start;
        }

        return -EEXIST;
    }
    
    for (i = (tbl8_group_start + 1); i < tbl8_group_end; i++) {
        if (tbl8[i].valid)
            return -EEXIST;
    }
    
    return -EINVAL;
}

static int32_t
delete_depth_big(struct lpm *lpm, uint32_t ip_masked,
    uint8_t depth, int32_t sub_rule_index, uint8_t sub_rule_depth)
{
#define group_idx next_hop
    uint32_t tbl24_index, tbl8_group_index, tbl8_group_start, tbl8_index,
            tbl8_range, i;
    int32_t tbl8_recycle_index;

    
    tbl24_index = ip_masked >> 8;

    
    tbl8_group_index = lpm->tbl24[tbl24_index].group_idx;
    tbl8_group_start = tbl8_group_index * LPM_TBL8_GROUP_NUM_ENTRIES;
    tbl8_index = tbl8_group_start + (ip_masked & 0xFF);
    tbl8_range = depth_to_range(depth);

    if (sub_rule_index < 0) {
        
        for (i = tbl8_index; i < (tbl8_index + tbl8_range); i++) {
            if (lpm->tbl8[i].depth <= depth)
                lpm->tbl8[i].valid = INVALID;
        }
    } else {
        
        struct lpm_tbl_entry new_tbl8_entry = {
            .valid = VALID,
            .depth = sub_rule_depth,
            .valid_group = lpm->tbl8[tbl8_group_start].valid_group,
            .next_hop = lpm->rules_tbl[sub_rule_index].next_hop,
        };

        
        for (i = tbl8_index; i < (tbl8_index + tbl8_range); i++) {
            if (lpm->tbl8[i].depth <= depth)
                __atomic_store(&lpm->tbl8[i], &new_tbl8_entry,
                        __ATOMIC_RELAXED);
        }
    }

    

    tbl8_recycle_index = tbl8_recycle_check(lpm->tbl8, tbl8_group_start);

    if (tbl8_recycle_index == -EINVAL) {
        
        lpm->tbl24[tbl24_index].valid = 0;
        tbl8_free(lpm->tbl8, tbl8_group_start);
    } else if (tbl8_recycle_index > -1) {
        
        struct lpm_tbl_entry new_tbl24_entry = {
            .next_hop = lpm->tbl8[tbl8_recycle_index].next_hop,
            .valid = VALID,
            .valid_group = 0,
            .depth = lpm->tbl8[tbl8_recycle_index].depth,
        };

        
        __atomic_store(&lpm->tbl24[tbl24_index], &new_tbl24_entry,
                __ATOMIC_RELAXED);
        tbl8_free(lpm->tbl8, tbl8_group_start);
    }
#undef group_idx
    return 0;
}


int
lpm_delete(struct lpm *lpm, uint32_t ip, uint8_t depth)
{
    int32_t rule_to_delete_index, sub_rule_index;
    uint32_t ip_masked;
    uint8_t sub_rule_depth;
    
    if ((lpm == NULL) || (depth < 1) || (depth > LPM_MAX_DEPTH)) {
        return -EINVAL;
    }

    ip_masked = ip & depth_to_mask(depth);

    
    rule_to_delete_index = rule_find(lpm, ip_masked, depth);

    
    if (rule_to_delete_index < 0)
        return -EINVAL;

    
    rule_delete(lpm, rule_to_delete_index, depth);

    
    sub_rule_depth = 0;
    sub_rule_index = find_previous_rule(lpm, ip, depth, &sub_rule_depth);

    
    if (depth <= MAX_DEPTH_TBL24) {
        return delete_depth_small(lpm, ip_masked, depth,
                sub_rule_index, sub_rule_depth);
    } else { 
        return delete_depth_big(lpm, ip_masked, depth, sub_rule_index,
                sub_rule_depth);
    }
}


void
lpm_delete_all(struct lpm *lpm)
{
    
    memset(lpm->rule_info, 0, sizeof(lpm->rule_info));

    
    memset(lpm->tbl24, 0, sizeof(lpm->tbl24));

    
    memset(lpm->tbl8, 0, sizeof(lpm->tbl8[0])
            * LPM_TBL8_GROUP_NUM_ENTRIES * lpm->number_tbl8s);

    
    memset(lpm->rules_tbl, 0, sizeof(lpm->rules_tbl[0]) * lpm->max_rules);
}
