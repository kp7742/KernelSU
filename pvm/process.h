#include <linux/sched.h>
#include <linux/module.h>
#include <linux/tty.h>
#include <linux/mm.h>
#include <linux/fs.h>
#include <linux/version.h>

#if(LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 1))
#include <linux/maple_tree.h>
#endif

#define ARC_PATH_MAX 256

extern struct mm_struct *get_task_mm(struct task_struct *task);
extern char *file_path(struct file *, char *, int);

#if(LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 61))
extern void mmput(struct mm_struct *);
#endif

// Ref: https://elixir.bootlin.com/linux/v6.1.57/source/mm/mmap.c#L325
uintptr_t traverse_vma(struct mm_struct* mm, char* name) {
    struct vm_area_struct *vma;
#if(LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 1))
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
            path_nm = file_path(vma->vm_file, buf, ARC_PATH_MAX-1);
            if (!strcmp(kbasename(path_nm), name)) {
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

    pid_struct = find_get_pid(pid);
    if (!pid_struct) {
        return false;
    }
    task = get_pid_task(pid_struct, PIDTYPE_PID);
    if (!task) {
        return false;
    }
    mm = get_task_mm(task);
    if (!mm) {
        return false;
    }
    mmput(mm);
    return traverse_vma(mm, name);
}
