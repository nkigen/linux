/*
 * DSS Scheduling Class (SCHED_DSS)
 *
 * Dynamic Sporadic Server as described by Spuri and Buttazzo 1994, 1996
 *
 * Copyright (C) 2014 Nelson Kigen <nellyk89@gmail.com> 
 *
 */

#include "sched.h"


void init_sched_dss_class(void){
/*TODO: Implement this*/
}

void init_dss_rq(struct dss_rq *dss_rq, struct rq *rq){

	dss_rq->dss_rb_root = RB_ROOT;
#ifdef CONFIG_SMP
	/*TODO:Implement this*/
#endif
dss_nr_running = 0;
dss_sporadic_nr_running = 0;

}

static inline void dss_spin_lock_init(spilock_t *lock)
{
	raw_spin_lock_init(&lock->rlock);
}
static inline void dss_spin_lock(spinlock_t *lock)
{
	raw_spin_lock(&lock->rlock);
}

static inline void dss_spin_unlock(spinlock_t *lock)
{
	raw_spin_unlock(&lock->rlock);
}

static inline struct task_struct *dss_task_of(struct sched_dss_entity *se)
{
	return container_of(se,struct task_struct, dss);
}

static inline struct rq *rq_of_dss_rq(struct dss_rq *dss_rq)
{
	return container_of(dss_rq,struct dss_rq, dss);
}

static inline struct dss_rq *dss_rq_of_se(struct sched_dss_entity *dss_se)
{
	struct task_struct *p = dss_task_of(dss_se);
	struct rq *rq = task_rq(p);
	return &rq->dss_rq;
}

static inline int dss_entity_type(struct sched_dss_entity *se){
	return se->entity_type;
}

static inline void init_dss_entity(struct sched_dss_entity *dss_se){
/*TODO: Init all the scheduling params*/
	WARN_ON(!dss_se->new);
	if(dss_entity_type(dss_se) == DSS_SPORADIC){

	}
	else if(dss_entity_type(dss_se) == DSS_PERIODIC){

	}
	else{ /*DEFINATELY an ERROR!!..Should never happen*/
		BUG_ON(1);
	}
	dss_se->dss_new = 0;
}
static void update_dss_entity(struct sched_dss_entity *se){
/*TODO:*/
	struct dss_rq *dss_rq = dss_rq_of_se(se);
	struct rq *rq = rq_of_dss_rq(dss_rq);

	if(se->dss_new){
		/*This is a new task: Initialize its sched_dss_entity*/
		init_dss_entity(se);

	}
	else{

	}
}

/*Recalculate the deadline of the SCHED_DSS task prior to enqueueing it*/
static void replenish_dss_entity(struct sched_dss_entity *se){

	if(se->entity_type == DSS_SPORADIC){

	}
	else if(se->entity_type == DSS_PERIODIC){

	}
}

static void inc_dss_tasks(struct sched_dss_entry *dss_se, struct dss_rq *dss_rq){
	dss_rq->dss_nr_running++;
	if(dss_entity_type(dss_se) == DSS_SPORADIC)
		dss_rq->dss_sporadic_nr_running++;
}


static void __enqueue_dss_entity(struct sched_dss_entity *se){

	struct dss_rq *dss_rq = dss_rq_of_se(se);
	int isleft = 1;
	struct sched_dss_entity *node = NULL;
	struct rb_node *parent;
	struct rb_node **link = &dss_rq->dss_rb_root->rb_node;

	while(*link){
		parent = *link;
		node = rb_entry(*link, struct sched_dss_entity, rb_node);
               if(dss_deadline_near(se->abs_deadline, node->abs_deadline))
		       link = &(*link)->rb_left;
	       else{
		  link = &(*link)->rb_right;
		  isleft = 0;
	       }
	}

	if(isleft)
		dss_rq->dss_rb_node = se->rb_node;
	rb_link_node(&se->rb_node, parent, link);
	rb_insert_color(&se->rb_node, &dss_rq->dss_rb_root);
        inc_dss_tasks(se, dss_rq);
}
/*Adds a SCHED_DSS task to the dss_rq*/
static void enqueue_dss_entity(struct sched_dss_entity *se, int flags)
{

	if(se->dss_new){
		/*Update scheduling params*/
		replenish_dss_entity(se);
	}
	else{
		/*replenish the runtime*/
	update_dss_entity(se);
	}
	__enqueue_dss_entity(se);
}

static void enqueue_task_dss(struct rq *rq, struct task_struct *p, int flags)
{
	struct sched_dss_entity *se = p->dss;
	/*TODO: Account for PI */
   	
	enqueue_dss_entity(se, flags);/*add to rb-tree of SCHED_DSS tasks*/
	inc_nr_running(rq); /*inc running tasks in the main rq*/
}


static void dss_update_task(struct rq *rq){
/*TODO:*/
}


static void dec_dss_task(struct dss_rq *dss_rq, struct sched_dss_entity *dss_se){
dss_rq->dss_nr_running--;
if(dss_entity_type(dss_se) == DSS_SPORADIC)
	dss_rq->dss_sporadic_nr_running--;
}

static void __dequeue_dss_entity(struct sched_dss_entity *se, int flags){
	struct dss_rq *dss_rq= dss_rq_of_se(se);
	struct rb_node *next_node;

	/*Check if rb_node is empty*/
	if(RB_EMPTY_NODE(se->rb_node))
		return; 
	if(dss_rq->dss_rb_node == &se->rb_node){
		next_node = rb_next(se->rb_next);
		dss_rq->dss_rb_next = next_node;
	}

	/*Erase node*/
	rb_erase(&se->rb_node,&dss_rq->dss_rb_root);
	RB_CLEAR_NODE(&se->rb_node);
	dec_dss_task(dss_rq, se);

}

static void dequeue_dss_entity(struct sched_dss_entity *se){
	__dequeue_dss_entity(se);
}

static void dequeue_task_dss(struct rq *rq, struct task_struct *p, int flags)
{

	/*TODO: update dss rq*/
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
/*
 * Check if the current SCHED_DSS task can be preempted by this new task*/
static bool dss_entity_preempt(struct sched_dss_entity *se, struct sched_dss_entity *curr)
{
	return ((s64) (se->deadline - curr->deadline) ) < 0 ;
}

/*All SCHED_DSS Tasks( both DSS_SPORADIC and DSS_PERIODIC priorities are based on their absolute
 * deadlines. 
 * To preempt, just check if the task's abs_deadline is nearer.
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
	struct rb_node *next = dss_rq->dss_rb_node;
	
	/*If next == NULL >> No SCHED_DSS task available*/
	if(!next)
		return NULL;
	/*else >> return a SCHED_DSS_ENTITY*/

	return rb_entry(next, struct sched_dss_entity, rb_node);
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
    
	t = dss_task_of(se);
/*TODO:exec_start??*/
return t;
}

static void put_prev_task_dss(struct rq *rq, struct task_struct *p)
{
	/*TODO: implement this!!*/
}
static void set_curr_task_dss(struct rq *rq)
{

}

static void prio_changed_dss(struct rq *rq, struct task_struct *t, int oldp)
{
	if(t->on_rq || rq->curr == t) {
#ifdef CONFIG_SMP
/*TODO: Implement this!!*/
#else
		resched_task(t);
#endif
	}
	else
		/*TODO: Implement this!!*/
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
/*TODO: Define these hooks*/
#endif
/*TODO: Define task_tick_dss, task_fork_dss and task_dead_dss*/
	.set_curr_task		= set_curr_task_dss,
	.task_tick		= task_tick_dss,
	.task_fork		= task_fork_dss,
	.task_dead		= task_dead_dss,

	.prio_changed		= prio_changed_dss,
	.switched_from		= switched_from_dss,
	.switched_to		= switched_to_dss,


};
