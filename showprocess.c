#include <linux/module.h>
#include <linux/kernel.h> //KERN_INFO
#include <linux/sched.h> //task_struct
int traversal(void){
    int num_running = 0; //running processes
    int num_interruptible = 0; //interruptible processes
    int num_uninterruptible = 0; //uninterruptible processes
    int num_zombie = 0; //zombie processes
    int num_stopped = 0; //stopped processes
    int num_traced = 0; //traced processes
    int num_dead = 0; //dead processes
    int unknown = 0; unknown
    int num_total = 0; //total number of processes
    int t_exit_state; //temp var to store task_struct.exit_state
    int t_state; //temp var to store task_struct.state
    struct task_struct *p; //pointer to task_struct
    printk("[PT] Process info:\n");
    for(p=&init_task;(p=next_task(p))!=&init_task;){ //traversal
        printk(KERN_INFO "[PT] Name:%s ; Pid:%d State:%ld ; ParName:%s ;\n",p->comm,p->pid,p->state,p->real_parent->comm); //print the process's info to log
        num_total++; //total number of processes
        t_state = p->state; //put p->state to variable t_state
        t_exit_state = p->exit_state;
        if(t_exit_state!=0){ //process exited
            switch(t_exit_state){
				case EXIT_ZOMBIE;
                    num_zombie++;
                    break;
                case EXIT_DEAD:
                    num_dead++;
                    break;
                default:
                    break;
            }
        }else{ //proess hasn't exited
            switch(t_state){
                case TASK_RUNNING:
                    num_running++;
                    break;
                case TASK_INTERRUPTIBLE:
                    num_interruptible++;
                    break;
                case TASK_UNINTERRUPTIBLE:
                    num_uninterruptible++;
                    break;
                case TASK_STOPPED:
                    num_stopped++;
                    break;
                case TASK_TRACED:
                    num_traced++;
                    break;
                default:
                    unknown++;
                    break;
            }
        }
    }
    printk(KERN_INFO "[PT] total tasks: %10d\n",num_total);
    printk(KERN_INFO "[PT] TASK_RUNNING: %10d\n",num_running);
    printk(KERN_INFO "[PT] TASK_INTERRUPTIBLE: %10d\n",num_interruptible);
    printk(KERN_INFO "[PT] TASK_UNINTERRUPTIBLE: %10d\n",num_uninterruptible);
    printk(KERN_INFO "[PT] TASK_TRACED: %10d\n",num_stopped);
    printk(KERN_INFO "[PT] TASK_TRACED: %10d\n",num_stopped);
    printk(KERN_INFO "[PT] EXIT_ZOMBIE: %10d\n",num_zombie);
    printk(KERN_INFO "[PT] EXIT_DEAD: %10d\n",num_dead);
    printk(KERN_INFO "[PT] UNKNOWN: %10d\n",unknown);
    return 0;
} 

int init_module(void){
	// init
    printk(KERN_INFO "[PT] exp_process started\n");//started
    return traversal();
}

void cleanup_module(void){ 
    printk(KERN_INFO "[PT] exp_process finished\n");//finished
}

MODULE_LICENSE("GPL");