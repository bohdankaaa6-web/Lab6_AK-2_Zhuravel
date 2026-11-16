// SPDX-License-Identifier: Dual BSD/GPL
/*
 * Copyright (c) 2017, GlobalLogic Ukraine LLC
 * All rights reserved.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/printk.h>
#include <linux/moduleparam.h>
#include <linux/ktime.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/bug.h>

MODULE_AUTHOR("Serhii Popovych <serhii.popovych@globallogic.com>");
MODULE_AUTHOR("Bohdana Zhuravel <zhuravel.bohdana_io42@edu.kpi.ua>");
MODULE_DESCRIPTION("Lab 6 Task 1 - Kernel Debugging: BUG_ON and Oops Analysis");
MODULE_LICENSE("Dual BSD/GPL");

/* Module parameter definition */
static uint print_count = 1;
module_param(print_count, uint, 0444);
MODULE_PARM_DESC(print_count, "Number of times to print 'Hello, world!'");

/*
 * Structure for storing event timestamps in a linked list.
 * time is at offset 0, list_node is at offset 8 on 32-bit ARM.
 */
struct hello_time_node {
	ktime_t time;
	struct list_head list_node;
};

/* Statically initialize the linked list head */
static LIST_HEAD(hello_time_list);

/*
 * Dedicated non-inlined helper to isolate the crash site in the disassembly
 * exactly as shown in appendix1.
 */
__attribute__((__noinline__))
static void add_to_list(struct list_head *node)
{
	/*
	 * BUG_ON checks for a NULL pointer.
	 * When node is &((struct hello_time_node *)0)->list_node,
	 * node evaluates to 0x00000008, so BUG_ON does NOT trigger here.
	 */
	BUG_ON(!node);
	list_add_tail(node, &hello_time_list);
}

static int __init hello_init(void)
{
	uint i;

	/* Warning for print_count == 0 or within [5; 10] */
	if (print_count == 0 || (print_count >= 5 && print_count <= 10))
		pr_warn("Warning: print_count is %u (0 or 5-10)\n", print_count);

	/*
	 * Task 1.I:
	 * Use BUG_ON() instead of printing error and returning -EINVAL
	 * when print_count exceeds 10.
	 */
	BUG_ON(print_count > 10);

	/* Event creation loop */
	for (i = 0; i < print_count; i++) {
		struct hello_time_node *node;

		/*
		 * Task 1.II:
		 * Inject artificial fault: simulate kmalloc returning NULL
		 * on the 5th message (index 4).
		 */
		if (i == 4) {
			pr_warn("Artificially simulating kmalloc failure on event %u\n",
				i + 1);
			node = 0;
		} else {
			node = kmalloc(sizeof(*node), GFP_KERNEL);
			if (!node) {
				struct hello_time_node *cursor, *tmp;

				pr_err("Real memory allocation failed\n");
				list_for_each_entry_safe(cursor, tmp,
							 &hello_time_list,
							 list_node) {
					list_del(&cursor->list_node);
					kfree(cursor);
				}
				return -ENOMEM;
			}
			node->time = ktime_get();
		}

		pr_emerg("Hello, world!\n");

		/*
		 * If node is NULL (0x0), &node->list_node evaluates to address 0x8.
		 * Passing this pointer causes an invalid memory dereference inside
		 * add_to_list().
		 */
		add_to_list(&node->list_node);
	}

	return 0;
}

static void __exit hello_exit(void)
{
	struct hello_time_node *cursor, *tmp;

	/* Traverse list, print timestamps and free allocated memory */
	list_for_each_entry_safe(cursor, tmp, &hello_time_list, list_node) {
		pr_info("Event time: %lld ns\n", ktime_to_ns(cursor->time));
		list_del(&cursor->list_node);
		kfree(cursor);
	}
	pr_info("Module unloaded.\n");
}

module_init(hello_init);
module_exit(hello_exit);
