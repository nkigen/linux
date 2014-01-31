#ifndef _SCHED_DSS_H
#define _SCHED_DSS_H


/*
 * SCHED_DSS has the same max priority as SCHED_DEADLINE
 */

#define MAX_DSS_PRIO 		0

static inline int dss_prio(int prio)
{
    if(unlikely(prio < MAX_DSS_PRIO))
        return 1;
    return 0;
}

static inline dss_task(struct task_struct *p)
{
    return dss_prio(p->prio);
}



#endif
