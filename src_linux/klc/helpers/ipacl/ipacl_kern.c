#include "klc/klc_base.h"
#include "helpers/ipacl_klc.h"

#define KLC_MODULE_NAME IPACL_KLC_MODULE_NAME
KLC_DEF_MODULE();

static inline BOOL_T _ipack_klc_match_rule(IPACLKLC_RULE_S *rule, IP_5TUPS_S *info)
{
    if (rule->ip_proto) {
        if (info->ip_proto != rule->ip_proto) {
            return FALSE;
        }
    }

    if (rule->sip) {
        if ((info->sip & rule->sip_mask) != rule->sip) {
            return FALSE;
        }
    }

    if (rule->dip) {
        if ((info->dip & rule->dip_mask) != rule->dip) {
            return FALSE;
        }
    }

    if (rule->sport_min || rule->sport_max) {
        if (info->sport < rule->sport_min) {
            return FALSE;
        }
        if (info->sport > rule->sport_max) {
            return FALSE;
        }
    }

    if (rule->dport_min || rule->dport_max) {
        if (info->dport < rule->dport_min) {
            return FALSE;
        }
        if (info->dport > rule->dport_max) {
            return FALSE;
        }
    }

    return TRUE;
}

SEC_NAME_FUNC(IPACL_KLC_MATCH_NAME)
int ipacl_klc_match_acl(IPACLKLC_CTRL_S *ctrl, IP_5TUPS_S *tup)
{
    int i;
    IPACLKLC_RULE_S *rule;

    if ((! ctrl) || (! tup)) {
        return -1;
    }

    for (i=0; i<ctrl->rule_count; i++) {
        rule = &ctrl->rules[i];
        if (_ipack_klc_match_rule(rule, tup)) {
            return ctrl->rules[i].action;
        }
    }

    return -1;
}


