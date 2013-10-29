/*
 * elevator sstf
 */
#include <linux/blkdev.h>
#include <linux/elevator.h>
#include <linux/bio.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/init.h>

struct sstf_data {
        struct list_head queue;
		sector_t head_pos;
		int direction; 
};

static void sstf_merged_requests(struct request_queue *q, struct request *rq,
                                 struct request *next)
{
        list_del_init(&next->queuelist);
}

static int sstf_dispatch(struct request_queue *q, int force)
{
        struct sstf_data *nd = q->elevator->elevator_data;
		
		if (q==NULL) 
			return 0;
		
        if (!list_empty(&nd->queue)) {
                struct request *rq, *nextrq, *prevrq;
				sector_t next, prev;
				
                nextrq = list_entry(nd->queue.next, struct request, queuelist);
                next = blk_rq_pos(nextrq);
				
				prevrq = list_entry(nd->queue.prev, struct request, queuelist);
				prev = blk_rq_pos(prevrq);
								
				if(nd->direction > 0 && next > prev) {
					/* MOVE UP */
					rq = nextrq;
				
				} else if(nd->direction < 0 && next < prev) {
					/* UPPER BOUND */
					rq = prevrq;
					nd->direction = -1;
				} 
				else if(nd->direction < 0 && next < prev) {
					/* Move DOWN */
					rq = prevrq;
				} else {
					/* LOWER BOUND */
					rq = nextrq;
					nd->direction = 1;
				}
				
				/* remove request */ 
                list_del_init(&rq->queuelist);
                nd->head_pos = blk_rq_pos(rq)  + blk_rq_sectors(rq) - 1;
                elv_dispatch_sort(q, rq);
                /* update queue head position
                sstf_balance(nd); */
				
                if(rq_data_dir(rq) == 0)
                        printk(KERN_INFO "[SSTF] dsp READ %ld\n",(long)blk_rq_sectors(rq));
                else
                        printk(KERN_INFO "[SSTF] dsp WRITE %ld\n",(long)blk_rq_sectors(rq));
                return 1;
        }
        return 0;
}

static void sstf_add_request(struct request_queue *q, struct request *rq)
{
        struct sstf_data *sd = q->elevator->elevator_data;
		struct request * rnext, * rprev;
        sector_t next, prev, pos;
		/* BASE CASE */
		if(list_empty(&sd->queue))  {
                list_add(&rq->queuelist,&sd->queue);
                return;
        }
		
		rnext = list_entry(sd->queue.next, struct request, queuelist);
		next = blk_rq_pos(rnext);
		rprev = list_entry(sd->queue.prev, struct request, queuelist);
		prev = blk_rq_pos(rprev);
        
		pos = blk_rq_pos(rq);
		/* Get correct position to add request */
        while(1) {
                /* Upper edge */
                if(pos > prev && next < prev)
                        break;
                /* Lower edge */
                if(pos < next && prev > next)
                        break;
                /* correct position */
                if(pos < next && pos > prev)
                        break;
                if(pos > next) {
						/* Moving on Up (in the morning) */
                        rprev = rnext;
                        prev = next;
                        rnext = list_entry(sd->queue.next, struct request,
                                           queuelist);
                        next = blk_rq_pos(rnext);
                } else {
						/* down, down, down and the flames went higher */
                        rnext = rprev;
                        next = prev;
                        rprev = list_entry(sd->queue.prev, struct request,
                                          queuelist);
                        prev = blk_rq_pos(rprev);
                }
        }
		/* now add at the correct position */
        __list_add(&rq->queuelist, &rprev->queuelist, &rnext->queuelist);
        printk(KERN_INFO "[SSTF] add %ld",(long) blk_rq_sectors(rq));
}

static struct request *
sstf_former_request(struct request_queue *q, struct request *rq)
{
        struct sstf_data *nd = q->elevator->elevator_data;

        if (rq->queuelist.prev == &nd->queue)
                return NULL;
        return list_entry(rq->queuelist.prev, struct request, queuelist);
}

static struct request *
sstf_latter_request(struct request_queue *q, struct request *rq)
{
        struct sstf_data *nd = q->elevator->elevator_data;

        if (rq->queuelist.next == &nd->queue)
                return NULL;
        return list_entry(rq->queuelist.next, struct request, queuelist);
}

static void *sstf_init_queue(struct request_queue *q)
{
        struct sstf_data *nd;

        nd = kmalloc_node(sizeof(*nd), GFP_KERNEL, q->node);
        if (!nd)
                return NULL;
        INIT_LIST_HEAD(&nd->queue);
        return nd;
}

static void sstf_exit_queue(struct elevator_queue *e)
{
        struct sstf_data *nd = e->elevator_data;

        BUG_ON(!list_empty(&nd->queue));
        kfree(nd);
}

static struct elevator_type elevator_sstf = {
        .ops = {
                .elevator_merge_req_fn		= sstf_merged_requests,
                .elevator_dispatch_fn		= sstf_dispatch,
                .elevator_add_req_fn		= sstf_add_request,
                .elevator_former_req_fn     = sstf_former_request,
                .elevator_latter_req_fn     = sstf_latter_request,
                .elevator_init_fn           = sstf_init_queue,
                .elevator_exit_fn           = sstf_exit_queue,
        },
        .elevator_name = "sstf",
        .elevator_owner = THIS_MODULE,
};

static int __init sstf_init(void)
{
        elv_register(&elevator_sstf);

        return 0;
}

static void __exit sstf_exit(void)
{
        elv_unregister(&elevator_sstf);
}

module_init(sstf_init);
module_exit(sstf_exit);


MODULE_AUTHOR("CS411 - GROUP14");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("SSTF IO scheduler");
