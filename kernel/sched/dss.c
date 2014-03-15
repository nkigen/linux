/*
 * DSS Scheduling Class (SCHED_DSS)
 *
 * Dynamic Sporadic Server as described by Spuri and Buttazzo 1994, 1996
 *
 * Copyright (C) 2014 Nelson Kigen <nellyk89@gmail.com> 
 *
 */

#include "sched.h"



static inline struct task_struct *dss_task_of(struct sched_dss_entity *se)
{
	return container_of(se,struct task_struct, dss);
}
static void enqueue_dss_entity(struct sched_dss_entity *se, int flags)
{

}

static void enqueue_task_dss(struct rq *rq, struct task_struct *p, int flags)
{
	struct sched_dss_entity *se = p->dss;
	/*TODO: Account for PI */
   	
	enqueue_dss_entity(se, flags);/*add to rb-tree of SCHED_DSS tasks*/
	inc_nr_running(rq); /*inc running tasks*/

}

static void dequeue_task_dss(struct rq *rq, struct task_struct *p, int flags)
{

	/*TODO: dequeue the task*/
	/*TODO: update dss rq*/
	dec_nr_running(rq);
}

/*Force task to sleep untill next deadline*/
static void yield_task_dss(struct rg *rq)
{
	struct task_struct *t = rq->curr;
	if(likely(t->dss.runtime > 0))
		t->dss.runtime = 0;
	/*TODO:update dss rq*/
}
/*
 * Check if the current SCHED_DSS task can be preempted by this new task*/
static bool dss_entity_preempt(struct sched_dss_entity *se, struct sched_dss_entity *curr)
{
	return ((s64) (se->deadline - curr->deadline) ) < 0 ;
}

/*
 * */
static void check_preempt_curr_dss(struct rq *rq, struct task_struct *p, int flags)
{
	if(dss_entity_preempt(p->dss, &rq->current.dss)){
		resched_task(rq->current);
		return;
	}
	/*TODO: cases where the deadlines are the same though unlikely*/
}

static struct sched_dss_entity *pick_next_dss_entity(struct dss_rq *dss_rq)
{
/*TODO: Pick a dss task from the rb-tree*/
}

static struct task_struct *pick_next_task_dss(struct rq *rq)
{
     struct task_struct *t;
     struct sched_dss_entity *se;
     struct dss_rq *dss_rq = &rq->dss;

    /*Leave if no dss task is available*/
    if(unlikely(!dss_rq->dss_nr_running))
    	return NULL;
    
    se = pick_next_dss_entity(dss_rq);
    
    BUG_ON(!se); /*dss_rq CANNOT be empty at this point!!*/

	t = dss_task_of(se);
/*TODO:exec_start??*/
return t;
}

static void put_prev_task_dss(struct rq *rq, struct task_struct *p)
{
	/*TODO: implement this!!*/
}
static void set_curr_task_dss(struct rg *rq)
{

}



const struct sched_class dss_sched_class = {
	.next 			= &dl_sched_class,
	.enqueue_task		= enqueue_task_dss,
	.dequeue_task		= dequeue_task_dss,
	.yield_task		= yield_task_dss,

	.check_preempt_curr	= check_preempt_curr_dss,

	.pick_next_task		= pick_next_task_dss,
	.put_prev_task		= put_prev_task_dss,

#ifdef CONFIG_SMP

#endif

	.set_curr_task		= set_curr_task_dss,
	.task_tick		=task_tick_dss,


};
