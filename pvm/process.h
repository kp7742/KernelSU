#include <linux/sched.h>
#include <linux/module.h>
#include <linux/tty.h>
#include <linux/mm.h>
#include <linux/fs.h>
#include <linux/version.h>
#include <linux/rcupdate.h>

#if(LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0))
#include <linux/dcache.h>
#include <linux/maple_tree.h>

extern char *d_path(const struct path *, char *, int);
#endif

#define ARC_PATH_MAX 256

extern struct mm_struct *get_task_mm(struct task_struct *task);
extern void mmput(struct mm_struct *);

// Ref: https://elixir.bootlin.com/linux/v6.1.57/source/mm/mmap.c#L325
//      https://elixir.bootlin.com/linux/v6.1.57/source/fs/open.c#L998
uintptr_t traverse_vma(struct mm_struct* mm, char* name) {
    struct vm_area_struct *vma;
#if(LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0))
    struct ma_state mas = {
		.tree = &mm->mm_mt,
		.index = 0,
		.last = 0,
		.node = MAS_START,
		.min = 0,
		.max = ULONG_MAX,
		.alloc = NULL,
	};

    while ((vma = mas_find(&mas, ULONG_MAX)) != NULL)
#else
    for (vma = mm->mmap; vma; vma = vma->vm_next)
#endif
    {
        char buf[ARC_PATH_MAX];
        char *path_nm = "";

        if (vma->vm_file) {
#if(LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0))
            path_nm = d_path(&(vma->vm_file)->f_path, buf, ARC_PATH_MAX-1);
#else
            path_nm = file_path(vma->vm_file, buf, ARC_PATH_MAX-1);
#endif
            pr_info("traverse_vma - vm_file: %s, path_nm: %s\n", &(vma->vm_file)->f_path, path_nm);
            if (!strcmp(kbasename(path_nm), name)) {
                pr_info("traverse_vma - found: %p\n", vma->vm_start);
                return vma->vm_start;
            }
        }
    }
    return 0;
}

uintptr_t get_module_base(pid_t pid, char* name) {
    struct pid* pid_struct;
    struct task_struct* task;
    struct mm_struct* mm;

#if(LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0))
    rcu_read_lock();
    pid_struct = find_vpid(pid);
    if (!pid_struct) {
        return false;
    }
    pr_info("get_module_base - pid_struct: %p\n", pid_struct);

    task = pid_task(pid_struct, PIDTYPE_PID);
    if (!task) {
        return false;
    }
    pr_info("get_module_base - task: %p\n", task);
    rcu_read_unlock();
#else
    pid_struct = find_get_pid(pid);
    if (!pid_struct) {
        return false;
    }
    pr_info("get_module_base - pid_struct: %p\n", pid_struct);

    task = get_pid_task(pid_struct, PIDTYPE_PID);
    if (!task) {
        return false;
    }
    pr_info("get_module_base - task: %p\n", task);
#endif

    mm = get_task_mm(task);
    if (!mm) {
        return false;
    }
    pr_info("get_module_base - mm: %p\n", mm);
    mmput(mm);

    return traverse_vma(mm, name);
}
