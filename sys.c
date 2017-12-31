asmlinkage int sys_mysyscall(void)
{
    struct task_struct *tsk = &init_task;
    int system_pf = 0;
    int system_dp = 0;
    
    printk(KERN_INFO "Mysyscall:");
    do{
        printk(KERN_INFO "Process name: %s   Page faults: %d   Dirty pages: %d\n",tsk->comm,tsk->pf,tsk->nr_dirtied);
        system_pf += tsk->pf;
        system_dp += tsk->nr_dirtied;
        tsk = next_task(tsk);
      }
    while( tsk != &init_task );

    printk(KERN_INFO "---------------------------------------------\n");
    printk(KERN_INFO "Total page faults: %d\n",system_pf);
    printk(KERN_INFO "Total dirty pages: %d\n",system_dp);
    printk(KERN_INFO "Current page faults: %d\n",current->pf);
    printk(KERN_INFO "Current dirty pages: %d\n",current->nr_dirtied);
    
    return 0;
}
