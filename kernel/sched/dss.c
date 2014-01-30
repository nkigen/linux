/*
 * DSS Scheduling Class (SCHED_DSS)
 *
 * Dynamic Sporadic Server as described by Spuri and Buttazzo 1994, 1996
 *
 * Copyright (C) 2014 Nelson Kigen <nellyk89@gmail.com> 
 *
 */


static void enqueue_task_dss(struct rq *rq, struct task_struct *p, int flags)
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

	.set_curr-task		= set_curr_task_dss,
	.task_tick		=task_tick_dss,


};
