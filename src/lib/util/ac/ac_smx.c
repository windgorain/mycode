/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/que_utl.h"
#include "utl/case_convert.h"
#include "utl/ac_smx.h"


static ACSMX_PATTERN_S * acsmx_CopyMatchListEntry(ACSMX_PATTERN_S * px)
{
    ACSMX_PATTERN_S * p;
    p = (ACSMX_PATTERN_S *) MEM_Malloc (sizeof (ACSMX_PATTERN_S));
    if (! p) {
        return NULL;
    }
    memcpy (p, px, sizeof (ACSMX_PATTERN_S));
    px->udata->ref_count++;
    p->next = 0;
    return p;
}

static int acsmx_AddMatchListEntry(ACSMX_S * acsm,
        int state, ACSMX_PATTERN_S * px)
{
    ACSMX_PATTERN_S * p;
    p =  MEM_Malloc (sizeof (ACSMX_PATTERN_S));
    if (! p) {
        RETURN(BS_NO_MEMORY);
    }

    memcpy (p, px, sizeof (ACSMX_PATTERN_S));
    p->next = acsm->acsmStateTable[state].MatchList;
    acsm->acsmStateTable[state].MatchList = p;

    return 0;
}

static int acsmx_AddPatternStates(ACSMX_S * acsm, ACSMX_PATTERN_S * p)
{
    unsigned char *pattern;
    int state=0, next, n;
    n = p->n;
    pattern = p->patrn;

    /*  Match up pattern with existing states */
    for (; n > 0; pattern++, n--) {
        next = acsm->acsmStateTable[state].NextState[*pattern];
        //printf("next=%d,state=%d\r\n", next, state);
        if (next == ACSM_FAIL_STATE)
            break;
        state = next;
    }

    /* Add new states for the rest of the pattern bytes, 1 state per byte */
    for (; n > 0; pattern++, n--) {
        acsm->acsmNumStates++;
        acsm->acsmStateTable[state].NextState[*pattern] = acsm->acsmNumStates;
        //printf("state=%d, pattern=%c, numStates=%d\r\n",state, *pattern, acsm->acsmNumStates );
        state = acsm->acsmNumStates;
    }

    return acsmx_AddMatchListEntry(acsm, state, p);
}

static void acsmx_FreePattern(ACSMX_PATTERN_S *plist)
{
    if (! plist) {
        return;
    }

    if (plist->patrn) {
        MEM_Free(plist->patrn);
    }

    if (plist->udata) {
        MEM_Free(plist->udata);
    }

    MEM_Free(plist);
}

static void acsmx_BuildNfa(ACSMX_S * acsm)
{
    int r, s;
    int i;
    SQUE_S q, *queue = &q;
    ACSMX_PATTERN_S * mlist=0;
    ACSMX_PATTERN_S * px=0;

    /* Init a Queue */
    SQUE_Init (queue);

    /* Add the state 0 transitions 1st */
    for (i = 0; i < ALPHABET_SIZE; i++) {
        s = acsm->acsmStateTable[0].NextState[i];
        if (s) {
            SQUE_Push (queue, UINT_HANDLE(s));
            acsm->acsmStateTable[s].FailState = 0;
            //printf("table[%d].failstate=0\r\n", s);
        }
    }

    /* Build the fail state transitions for each valid state */
    while (SQUE_Count (queue) > 0) {
        r = HANDLE_UINT(SQUE_Pop(queue));

        /* Find Final States for any Failure */
        for (i = 0; i < ALPHABET_SIZE; i++) {
            int fs, next;
            if ((s = acsm->acsmStateTable[r].NextState[i]) != ACSM_FAIL_STATE) {
                SQUE_Push(queue, UINT_HANDLE(s));
                fs = acsm->acsmStateTable[r].FailState;

                /*  Locate the next valid state for 'i' starting at s */
                while ((next=acsm->acsmStateTable[fs].NextState[i]) ==
                        ACSM_FAIL_STATE) {
                    fs = acsm->acsmStateTable[fs].FailState;
                }

                 /* Update 's' state failure state
                    to point to the next valid state */
                acsm->acsmStateTable[s].FailState = next;
                //printf("table[%d].failstate = %d\r\n", s, next);
                /*
                 *  Copy 'next'states MatchList to 's' states MatchList,
                 *  we copy them so each list can be MEM_Free'd later,
                 *  else we could just manipulate pointers to fake the copy.
                 */
                for (mlist  = acsm->acsmStateTable[next].MatchList;
                        mlist != NULL ;
                        mlist  = mlist->next) {
                    px = acsmx_CopyMatchListEntry(mlist);

                    if( !px ) {
                        ErrCode_FatalError("*** Out of memory Initializing Aho Corasick in acsmx.c ****");
                    }

                    /* Insert at front of MatchList */
                    px->next = acsm->acsmStateTable[s].MatchList;
                    acsm->acsmStateTable[s].MatchList = px;
                }
            }
        }
    }

    /* Clean up the queue */
    SQUE_Final (queue);
}


/*
*   Build Deterministic Finite Automata from NFA
*/
static void
acsmx_ConvertNfa2Dfa (ACSMX_S * acsm)
{
    int r, s;
    int i;
    SQUE_S q, *queue = &q;

    /* Init a Queue */
    SQUE_Init(queue);

    /* Add the state 0 transitions 1st */
    for (i = 0; i < ALPHABET_SIZE; i++)
    {
        s = acsm->acsmStateTable[0].NextState[i];
        if (s)
        {
            SQUE_Push(queue, UINT_HANDLE(s));
        }
    }

    /* Start building the next layer of transitions */
    while (SQUE_Count(queue) > 0)
    {
        r = HANDLE_UINT(SQUE_Pop(queue));

        /* State is a branch state */
        for (i = 0; i < ALPHABET_SIZE; i++)
        {
            if ((s = acsm->acsmStateTable[r].NextState[i]) != ACSM_FAIL_STATE)
            {
                SQUE_Push (queue, UINT_HANDLE(s));
            }
            else
            {
                acsm->acsmStateTable[r].NextState[i] =
                    acsm->acsmStateTable[acsm->acsmStateTable[r].FailState].
                    NextState[i];
            }
        }
    }

    /* Clean up the queue */
    SQUE_Final (queue);
}

ACSMX_S * ACSMX_New()
{
    ACSMX_S * p;
    p = (void *) MEM_ZMalloc(sizeof (ACSMX_S));
    return p;
}

int ACSMX_AddPattern(ACSMX_S * p, UCHAR *pat, int n, void *id)
{
    ACSMX_PATTERN_S * plist;
    plist = (ACSMX_PATTERN_S *) MEM_ZMalloc (sizeof (ACSMX_PATTERN_S));
    if (! plist) {
        RETURN(BS_NO_MEMORY);
    }

    plist->patrn = (unsigned char *) MEM_ZMalloc (n);
    plist->udata = MEM_ZMalloc(sizeof(ACSMX_USERDATA_S));
    if ((! plist->patrn) || (! plist->udata)) {
        acsmx_FreePattern(plist);
        RETURN(BS_NO_MEMORY);
    }

    memcpy(plist->patrn, pat, n);
    plist->udata->ref_count = 1;
    plist->udata->id = id;
    plist->n = n;
    plist->next = p->acsmPatterns;
    p->acsmPatterns = plist;
    p->numPatterns++;

    return 0;
}

#if 0
static void acsmx_PrintDFA( ACSMX_S * acsm )
{
    int k;
    int i;
    int next;

    for (k = 0; k < acsm->acsmMaxStates; k++)
    {
        for (i = 0; i < ALPHABET_SIZE; i++)
        {
            next = acsm->acsmStateTable[k].NextState[i];

            if( next == 0 || next ==  ACSM_FAIL_STATE )
            {
                if( isprint(i) )
                    printf("%3c->%-5d\t",i,next);
                else
                    printf("%3d->%-5d\t",i,next);
            }
        }
        printf("\n");
    }
}
#endif

int ACSMX_Compile(ACSMX_S * acsm)
{
    int i, k;
    ACSMX_PATTERN_S * plist;

    /* 计算最大可能的状态个数 */
    acsm->acsmMaxStates = 1;
    for (plist = acsm->acsmPatterns; plist != NULL; plist = plist->next) {
        acsm->acsmMaxStates += plist->n;
    }

    acsm->acsmStateTable = MEM_ZMalloc(sizeof(ACSMX_STATE_TABLE_S) *
                acsm->acsmMaxStates);
    if (! acsm->acsmStateTable) {
        RETURN(BS_NO_MEMORY);
    }

    acsm->acsmNumStates = 0;
    /* Initialize all States NextStates to FAILED */
    for (k = 0; k < acsm->acsmMaxStates; k++) {
        for (i = 0; i < ALPHABET_SIZE; i++) {
            acsm->acsmStateTable[k].NextState[i] = ACSM_FAIL_STATE;
        }
    }

    /* Add each Pattern to the State Table */
    for (plist = acsm->acsmPatterns; plist != NULL; plist = plist->next) {
        int ret = acsmx_AddPatternStates(acsm, plist);
        if (ret != 0) {
            return ret;
        }
    }

    /* Set all failed state transitions to return to the 0'th state */
    for (i = 0; i < ALPHABET_SIZE; i++) {
        if (acsm->acsmStateTable[0].NextState[i] == ACSM_FAIL_STATE) {
            acsm->acsmStateTable[0].NextState[i] = 0;
        }
    }

    acsmx_BuildNfa(acsm);
    acsmx_ConvertNfa2Dfa(acsm);

    //acsmx_PrintDFA(acsm);

    return 0;
}

int ACSMX_Search(ACSMX_S * acsm, unsigned char *Tx, int n,
            PF_AC_Match match_func,     /*匹配上单个pattern的处理函数 */
            void *user_data, INOUT int * current_state )
{
    int state = 0;
    ACSMX_PATTERN_S * mlist;
    unsigned char *Tend;
    ACSMX_STATE_TABLE_S * StateTable = acsm->acsmStateTable;
    int nfound = 0;
    unsigned char *T;
    UCHAR c;
    int offset;

    BS_DBGASSERT(NULL != current_state);

    if (! StateTable) {
        return 0;
    }

    T = Tx;
    Tend = T + n;

    state = *current_state;

    for (; T < Tend; T++) {
        c = *T;
        state = StateTable[state].NextState[c];

        if( StateTable[state].MatchList != NULL ) {
            mlist = StateTable[state].MatchList;
            offset = T - mlist->n + 1 - Tx;
            nfound++;
            if (match_func(mlist, offset, user_data) > 0) {
                break;
            }
        }
    }

    *current_state = state;

    return nfound;
}

void ACSMX_Free(ACSMX_S * acsm, PF_AC_USER_FREE userfree)
{
    int i;
    ACSMX_PATTERN_S * mlist, *ilist;
    for (i = 0; i < acsm->acsmMaxStates; i++)
    {
        mlist = acsm->acsmStateTable[i].MatchList;
        while (mlist)
        {
            ilist = mlist;
            mlist = mlist->next;

            ilist->udata->ref_count--;
            if (ilist->udata->ref_count == 0)
            {
                if (userfree) {
                    userfree(ilist->udata->id);
                }

                MEM_Free(ilist->udata);
            }

            MEM_Free (ilist);
        }
    }
    MEM_Free (acsm->acsmStateTable);
    mlist = acsm->acsmPatterns;
    while(mlist)
    {
        ilist = mlist;
        mlist = mlist->next;
        MEM_Free(ilist->patrn);
        MEM_Free(ilist);
    }
    MEM_Free (acsm);
}


