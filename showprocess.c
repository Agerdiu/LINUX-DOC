#include <linux/module.h>
#include <linux/kernel.h> //KERN_INFO
#include <linux/sched.h> //task_struct
int traversal(void){
    int running = 0; //running processes
    int interruptible = 0; //interruptible processes
    int uninterruptible = 0; //uninterruptible processes
    int zombie = 0; //zombie processes
    int stopped = 0; //stopped processes
    int traced = 0; //traced processes
    int dead = 0; //dead processes
    int unknown = 0; 
    int total = 0; //total number of processes
    int exit; 
    int taskstate; //save the states of process
    struct task_struct *p; //pointer to task_struct
    printk("[PT] Process info:\n");
    for(p=&init_task; (p=next_task(p))!=&init_task;){ //traversal
        printk("[PT] Name:%s ; Pid:%d State:%ld ; ParName:%s ;\n",p->comm,p->pid,p->state,p->real_parent->comm);
        taskstate = p->state; //put p->state to variable t_state
        exit = p->exit_state;
        if(exit !=0){ //process exited
            switch(exit){
			case EXIT_ZOMBIE:
                    zombie++;
                    break;
                case EXIT_DEAD:
                    dead++;
                    break;
                default:
                    break;
            }
        }else{ //proess hasn't exited
            switch(taskstate){
                case TASK_RUNNING:
                    running++;
                    break;
                case TASK_INTERRUPTIBLE:
                    interruptible++;
                    break;
                case TASK_UNINTERRUPTIBLE:
                    uninterruptible++;
                    break;
                case TASK_STOPPED:
                    stopped++;
                    break;
                case TASK_TRACED:
                    traced++;
                    break;
                default:
                    unknown++;
                    break;
            }
        }
    }
    printk("[PT] total tasks: %10d\n",total);
    printk("[PT] RUNNING: %10d\n",running);
    printk("[PT] INTERRUPTIBLE: %10d\n",interruptible);
    printk("[PT] UNINTERRUPTIBLE: %10d\n",uninterruptible);
    printk("[PT] STOPPED: %10d\n",stopped);
    printk("[PT] TRACED: %10d\n",traced);
    printk("[PT] ZOMBIE: %10d\n",zombie);
    printk("[PT] DEAD: %10d\n",dead);
    printk("[PT] Unknown: %10d\n",unknown);
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
