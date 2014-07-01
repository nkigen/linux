/*
 * DSS Scheduling Class (SCHED_DSS)
 *
 * Dynamic Sporadic Server as described by Spuri and Buttazzo 1994, 1996
 *
 * Copyright (C) 2014 Nelson Kigen <nellyk89@gmail.com>
 *
 */

#include "sched.h"

static void update_dss_curr(struct rq *rq);

void init_sched_dss_class(void) {
    /*TODO:*/
}

void init_dss_rq(struct dss_rq *dss_rq, struct rq *rq) {

    struct dss_sporadic_bw *bw ;
    dss_rq->dss_rb_root = RB_ROOT;

    bw = &dss_rq->dss_sporadic_bw;
#ifdef CONFIG_SMP
    /*TODO:*/
#endif
    /*set DSS_SPORADIC bandwidth management params needed by the DSS algorithm*/
    dss_rq->dss_nr_running = 0;
    bw->dss_sporadic_nr_running = 0;
    bw->dss_sporadic_runtime = 0;
    bw->dss_replenish_time = rq_clock(rq);
    bw->dss_replenish_amt = 0;
    bw->prev_runtime = 0;

}

static inline struct task_struct *dss_task_of(struct sched_dss_entity *se)
{
    return container_of(se,struct task_struct, dss);
}

static inline struct rq *rq_of_dss_rq(struct dss_rq *dss_rq)
{
    return container_of(dss_rq,struct rq, dss);
}

static inline struct dss_rq *dss_rq_of_se(struct sched_dss_entity *dss_se)
{
    struct task_struct *p = dss_task_of(dss_se);
    struct rq *rq = task_rq(p);
    return &rq->dss;
}

static inline unsigned int dss_entity_type(struct sched_dss_entity *se) {
    return se->dss_entity_type;
}

/*
 * DSS_SPORADIC tasks share a runtime and the replemnishment times
 * stored in struct dss_rq. HOWEVER, each has individual deadlines
 *
 * Here we update the scheduling parameters based on the DSS Algorithm
 */
static void dss_sporadic_update(struct sched_dss_entity *dss_se) {
    struct dss_rq *dss_rq = dss_rq_of_se(dss_se);
    struct rq *rq = rq_of_dss_rq(dss_rq);
    u64 rtime;

    struct dss_sporadic_bw *bw;
    rtime = rq_clock_task(rq) - rq->curr->se.exec_start;
    bw  = &dss_rq->dss_sporadic_bw;
    /*Set the replenishment time*/
    if(bw->dss_sporadic_nr_running &&
            bw->dss_sporadic_runtime) {
        bw->dss_replenish_time = rq_clock(rq) + dss_se->dss_period;
        bw->is_replenish = true;
        bw->prev_runtime = bw->dss_sporadic_runtime;

    }
    if(bw->is_replenish) {
        bw->dss_replenish_amt += bw->prev_runtime - bw->dss_sporadic_runtime;
        bw->prev_runtime = bw->dss_sporadic_runtime;
    }
    if(!bw->dss_sporadic_nr_running ||
            !bw->dss_sporadic_runtime) {
        bw->dss_sporadic_runtime += bw->dss_replenish_amt;
        bw->prev_runtime = 0;
        bw->is_replenish = false;
    }

    /*Absolute deadline is set to the next replenishment time
     * if is_replenish is true it means that the deadline so we have to resched the task*/

    if(bw->is_replenish) {
        dss_se->abs_deadline = bw->dss_replenish_time;
        resched_task(rq->curr);
    }
    bw->dss_sporadic_runtime -= rtime;
}

/*
 * Update runtime of a DSS_PERIODIC task*/
static void dss_periodic_update(struct sched_dss_entity *dss_se) {

    struct dss_rq *dss_rq = dss_rq_of_se(dss_se);
    struct rq *rq = rq_of_dss_rq(dss_rq);
    s64 rtime;
    dss_se->runtime -= rq_clock_task(rq) - rq->curr->se.exec_start;

    /*rtime = max_rtime - curr_rtime*/
    rtime = dss_se->dss_runtime - dss_se->runtime;
    if(rtime <= 0) {
        dss_se->runtime = 0;
        dss_se->abs_deadline = rq_clock(rq) + dss_se->dss_deadline;
    }
}

static inline void init_dss_entity(struct sched_dss_entity *dss_se) {
    /*TODO: Init all the scheduling params*/
//    WARN_ON(!dss_se->new);
    struct dss_rq *dss_rq = dss_rq_of_se(dss_se);
    struct rq *rq = rq_of_dss_rq(dss_rq);

    if(dss_entity_type(dss_se) == DSS_SPORADIC) {
        dss_sporadic_update(dss_se);
    }
    else if(dss_entity_type(dss_se) == DSS_PERIODIC) {
        dss_se->runtime = dss_se->dss_runtime;
        dss_se->abs_deadline = rq_clock(rq) + dss_se->dss_deadline;
    }
    else { /*DEFINATELY an ERROR!!..Should never happen*/
        BUG_ON(1);
    }
    dss_se->dss_new = 0;
}

static void update_dss_entity(struct sched_dss_entity *se) {
    if(se->dss_new) {
        /*This is a new task: Initialize its sched_dss_entity*/
        init_dss_entity(se);

    }
    else {

        if(dss_entity_type(se) == DSS_SPORADIC) {
            dss_sporadic_update(se);
        }
        else if(dss_entity_type(se) == DSS_PERIODIC) {
            dss_periodic_update(se);
        }
    }
}

/*Recalculate the deadline of the SCHED_DSS task prior to enqueueing it*/
static void replenish_dss_entity(struct sched_dss_entity *se) {

    if(dss_entity_type(se) == DSS_SPORADIC) {
        dss_sporadic_update(se);
    }
    else if(dss_entity_type(se) == DSS_PERIODIC) {
        dss_periodic_update(se);
    }
}

static void inc_dss_tasks(struct sched_dss_entity *dss_se, struct dss_rq *dss_rq) {
    struct dss_sporadic_bw *bw;
    bw = &dss_rq->dss_sporadic_bw;
    dss_rq->dss_nr_running++;
    if(dss_entity_type(dss_se) == DSS_SPORADIC)
        bw->dss_sporadic_nr_running++;
}

static void __enqueue_dss_entity(struct sched_dss_entity *se) {

    struct dss_rq *dss_rq = dss_rq_of_se(se);
    int isleft = 1;
    struct sched_dss_entity *node = NULL;
    struct rb_node *parent = NULL;
    struct rb_node **link = &dss_rq->dss_rb_root.rb_node;

    while(*link) {
        parent = *link;
        node = rb_entry(*link, struct sched_dss_entity, rb_node);
        if(dss_deadline_near(se->abs_deadline, node->abs_deadline))
            link = &(*link)->rb_left;
        else {
            link = &(*link)->rb_right;
            isleft = 0;
        }
    }

    if(isleft)
        dss_rq->dss_rb_node = &se->rb_node;
    rb_link_node(&se->rb_node, parent, link);
    rb_insert_color(&se->rb_node, &dss_rq->dss_rb_root);
    inc_dss_tasks(se, dss_rq);
}
/*Adds a SCHED_DSS task to the dss_rq*/
static void enqueue_dss_entity(struct sched_dss_entity *se, int flags)
{
    /*Calculate the Task deadline before enqueueing it*/
    if(se->dss_new) {
        /*Update scheduling params*/
        update_dss_entity(se);
    }
    else {
        /*replenish the runtime*/
        replenish_dss_entity(se);
    }
    __enqueue_dss_entity(se);
}

static void enqueue_task_dss(struct rq *rq, struct task_struct *p, int flags)
{
    struct sched_dss_entity *se = &p->dss;
    /*TODO: Account for PI */

    enqueue_dss_entity(se, flags);/*add to rb-tree of SCHED_DSS tasks*/
    inc_nr_running(rq); /*inc running tasks in the main rq*/
}


static void dss_update_task(struct rq *rq) {
    /*TODO:*/
    struct dss_rq *dss_rq = &rq->dss;
    struct rb_node *node = dss_rq->dss_rb_node;
    struct sched_dss_entity *dss_se = rb_entry(node,struct sched_dss_entity,rb_node);
    update_dss_entity(dss_se);

}


static void dec_dss_task(struct dss_rq *dss_rq, struct sched_dss_entity *dss_se) {
    struct dss_sporadic_bw *bw = &dss_rq->dss_sporadic_bw;
    dss_rq->dss_nr_running--;
    if(dss_entity_type(dss_se) == DSS_SPORADIC)
        bw->dss_sporadic_nr_running--;
}

static void __dequeue_dss_entity(struct sched_dss_entity *se, int flags) {
    struct dss_rq *dss_rq= dss_rq_of_se(se);
    struct rb_node *next_node;

    /*Check if rb_node is empty*/
    if(RB_EMPTY_NODE(&se->rb_node))
        return;
    if(dss_rq->dss_rb_node == &se->rb_node) {
        next_node = rb_next(&se->rb_node);
        dss_rq->dss_rb_node = next_node;
    }

    /*Erase node*/
    rb_erase(&se->rb_node,&dss_rq->dss_rb_root);
    RB_CLEAR_NODE(&se->rb_node);
    dec_dss_task(dss_rq, se);

}

static void dequeue_dss_entity(struct sched_dss_entity *se) {
    __dequeue_dss_entity(se, 0);
}

static void dequeue_task_dss(struct rq *rq, struct task_struct *p, int flags)
{
    dequeue_dss_entity(&p->dss);
    dec_nr_running(rq);
}

/*Force task to sleep until next deadline*/
static void yield_task_dss(struct rq *rq)
{
    struct task_struct *t = rq->curr;
    if(likely(t->dss.runtime > 0))
        t->dss.runtime = 0;
    /*TODO:update dss rq*/
}

/*All SCHED_DSS Tasks( both DSS_SPORADIC and DSS_PERIODIC priorities are based on their absolute
 * deadlines.
 * To preempt, just check if the task's abs_deadline is nearer.
 * */
static void check_preempt_curr_dss(struct rq *rq, struct task_struct *p, int flags)
{
    struct task_struct *curr_task = rq->curr;
    struct sched_dss_entity *curr_se = &curr_task->dss;
    struct sched_dss_entity *new_se = &p->dss;

    /*Check if p is of SCHED_DSS*/
    if(!dss_task(p))
        return;

    if(dss_entity_preempt(new_se, curr_se)) {
        resched_task(rq->curr);
        return;
    }

    /*TODO: cases where the deadlines are the same though unlikely*/
}

static struct sched_dss_entity *pick_next_dss_entity(struct dss_rq *dss_rq)
{
    /*TODO: Pick a dss task from the rb-tree*/
    struct rb_node *next = dss_rq->dss_rb_node;

    /*If next == NULL >> No SCHED_DSS task available*/
    if(!next)
        return NULL;
    /*else >> return a SCHED_DSS_ENTITY*/

    return rb_entry(next, struct sched_dss_entity, rb_node);
}

static struct task_struct *pick_next_task_dss(struct rq *rq)
{
    struct task_struct *next = NULL;
    struct sched_dss_entity *se;
    struct dss_rq *dss_rq = &rq->dss;

    se = pick_next_dss_entity(dss_rq);
    next = dss_task_of(se);

    /*TODO:exec_start??*/
    return next;
}

static void put_prev_task_dss(struct rq *rq, struct task_struct *p)
{
    dss_update_task(rq);
}
static void set_curr_task_dss(struct rq *rq)
{

}

static void prio_changed_dss(struct rq *rq, struct task_struct *t, int oldp)
{
    if(t->on_rq || rq->curr == t) {
#ifdef CONFIG_SMP
        /*TODO: */
#else
        resched_task(t);
#endif
    }
    else
        ;
    /*TODO:*/
}

static bool dss_runtime_exceeded(struct rq *rq, struct sched_dss_entity *dss_se) {
    /*TODO:*/
    int miss = dss_time_before(dss_se->abs_deadline, rq_clock(rq));
    /*Update the deadline and reschedule the task*/

    if(!miss)
        return 0;
    dss_update_task(rq);
    return 1;
}
static void update_dss_curr(struct rq *rq) {
    struct sched_dss_entity *curr = &rq->curr->dss;
    u64 rtime;
    if(!dss_task(rq->curr))
        return;
    rtime = rq_clock_task(rq) - rq->curr->se.exec_start;
    if(rtime < 0)
        return;

    /*TODO: */
    /*If runtime has been exceeded, enqueue it and reschedule it*/
    if(dss_runtime_exceeded(rq, curr)) {
        enqueue_task_dss(rq, rq->curr,0);
        resched_task(rq->curr);
    }
    else
        update_dss_entity(curr);

}
static void task_tick_dss(struct rq *rq, struct task_struct *t, int queued) {

    update_dss_curr(rq);
}

static void task_fork_dss(struct task_struct *p) {}

static void switched_to_dss(struct rq *rq, struct task_struct *p) {

    /*Check if policy is SCHED_DSS*/
    if(!task_has_dss_policy(rq->curr))
        return;
    check_preempt_curr_dss(rq, p, 0);
}


static void switched_from_dss(struct rq *rq, struct task_struct *p) {
    if(dss_policy(p->policy))
        return;
    /*TODO:*/
#ifdef CONFIG_SMP
#endif
}


static void task_dead_dss(struct task_struct *p) {
}
const struct sched_class dss_sched_class = {
    .next 			= &dl_sched_class,
    .enqueue_task		= enqueue_task_dss,
    .dequeue_task		= dequeue_task_dss,
    .yield_task			= yield_task_dss,

    .check_preempt_curr		= check_preempt_curr_dss,

    .pick_next_task		= pick_next_task_dss,
    .put_prev_task		= put_prev_task_dss,

#ifdef CONFIG_SMP
    /*TODO: Define these hooks*/
#endif
    /*TODO: Define task_tick_dss, task_fork_dss and task_dead_dss*/
    .set_curr_task		= set_curr_task_dss,
    .task_tick			= task_tick_dss,
    .task_fork			= task_fork_dss,
    .task_dead			= task_dead_dss,

    .prio_changed		= prio_changed_dss,
    .switched_from		= switched_from_dss,
    .switched_to		= switched_to_dss,


};
