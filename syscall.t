asmlinkage int sys_mysyscall(void)
{
    task_struct *tsk = &init_task;
    int system_pf = 0;

    printk(KERN_INFO "Mysyscall:");
    printk(KERN_INFO "pid\tprocess name\tpage faults\tdirty pages\n");
    do{
        printk(KERN_INFO "%d\t%s\t%d\t%d\n",tsk->pid,tsk->comm,tsk->pf,tsk->nr_dirtied);
        system_pf += tsk->pf;
    } while( (tsk = next_task(tsk)) != &init_task );

    printk(KERN_INFO "---------------------------------------------\n");
    printk(KERN_INFO "System page faults: %d\n",system_pf);
    printk(KERN_INFO "Current page faults: %d\n",current->pf);
    printk(KERN_INFO "Current dirty pages: %d\n",current->nr_dirtied);
    
    return 0;
}
