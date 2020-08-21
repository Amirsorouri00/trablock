
static ssize_t process_vm_rw(pid_t pid,
			     const struct iovec __user *lvec,
			     unsigned long liovcnt,
			     const struct iovec __user *rvec,
			     unsigned long riovcnt,
			     unsigned long flags, int vm_write)
{
	struct iovec iovstack_l[UIO_FASTIOV];
	struct iovec iovstack_r[UIO_FASTIOV];
	struct iovec *iov_l = iovstack_l;
	struct iovec *iov_r = iovstack_r;
	struct iov_iter iter;
	ssize_t rc;
	int dir = vm_write ? WRITE : READ;
	struct task_struct *target_task = NULL; // Sorouri


//	if (flags != 0)
//		return -EINVAL;

	// Sorouri
	if (current->pid == pid) {
		//current->sync_counters = (atomic_t*) kmalloc(nr_cpu_ids, GFP_KERNEL); // 2
		current->sync_cnts = (long int*) kmalloc(nr_cpu_ids, GFP_KERNEL); // 2
		int i;
		for (i = 0; i < nr_cpu_ids; i++)
			current->sync_cnts[i] = 0; // 2
			//current->sync_counters[i] = (atomic_t)ATOMIC_INIT(0); // 2

		//atomic_set(&current->ipc_sched_syncher, 1);
		current->ipc_sched_synch = 1;
		current->sync_count = liovcnt;
		while(current->ipc_sched_synch != 0) {
			int cpu;
			unsigned long sum = 0;
			for (cpu = 0; cpu < nr_cpu_ids; cpu++)
				sum += current->sync_cnts[cpu];
				//sum += current->sync_counters[cpu].counter;
			if (sum >= current->sync_count) {
				//atomic_set(&current->ipc_sched_syncher, 0);
				current->ipc_sched_synch = 0;
				break;
			}
			msleep(600);
		}
		//kfree(current->sync_counters);
		kfree(current->sync_cnts);
		return 0;
	}

	/* Check iovecs */
	rc = import_iovec(dir, lvec, liovcnt, UIO_FASTIOV, &iov_l, &iter);
	
	if (rc < 0)
		return rc;
	if (!iov_iter_count(&iter))
		goto free_iovecs;

	rc = rw_copy_check_uvector(CHECK_IOVEC_ONLY, rvec, riovcnt, UIO_FASTIOV,
				   iovstack_r, &iov_r);
	if (rc <= 0)
		goto free_iovecs;

	rc = process_vm_rw_core(pid, &iter, iov_r, riovcnt, flags, vm_write);

	// Sorouri 2
	struct timespec start, stop, result;
	//getnstimeofday(&start);
	target_task = find_task_by_vpid(pid);
	//printk("mm/psvm_accs:flags = %ld.\n", flags);
	if (flags == 100) {
		getnstimeofday(&start);
		int proc_id = smp_processor_id();
		target_task->sync_cnts[proc_id]+=riovcnt;
		//atomic_add(riovcnt, &target_task->sync_counters[proc_id]);
		getnstimeofday(&stop);
	}
	//getnstimeofday(&stop);

	if ((stop.tv_nsec - start.tv_nsec) < 0) {
        result.tv_sec = stop.tv_sec - start.tv_sec - 1;
        result.tv_nsec = stop.tv_nsec - start.tv_nsec + 1000000000;
    } else {
        result.tv_sec = stop.tv_sec - start.tv_sec;
        result.tv_nsec = stop.tv_nsec - start.tv_nsec;
    }
	printk("reader: seconds without ns: %ld\n", result.tv_sec);                                       
    printk("reader: nanoseconds: %ld\n", result.tv_nsec);
