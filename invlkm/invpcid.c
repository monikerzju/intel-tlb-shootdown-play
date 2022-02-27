#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/export.h>
#include <linux/fs.h>
#include <asm/tlbflush.h>
#include <asm/mmu_context.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("monikerzju");
MODULE_DESCRIPTION("KO for invpcid.");
MODULE_VERSION("0.01");

// extern void flush_tlb_local(void);

static ssize_t invpcid_local(struct file *fp, char *c, size_t len, loff_t *off) {
    // flush_tlb_local();
    __flush_tlb_all();
    return 0;
}

static struct file_operations invpcid_fops = 
{
    .owner   = THIS_MODULE,
    .read    = invpcid_local,
};
static int invpcid_dev_number;

static int __init invpcid_init(void) {
    int result;
    printk(KERN_INFO "Invpcid Init\n");
    result = register_chrdev(0, "invpcidfs", &invpcid_fops);
    invpcid_dev_number = result;
    if (result < 0) {
        printk(KERN_INFO "Invpcid Badly registered\n");
        return result;
    }
    return 0;
}

static void __exit invpcid_exit(void) {
    if (invpcid_dev_number > 0) {
        unregister_chrdev(invpcid_dev_number, "invpcidfs");
    }
    printk(KERN_INFO "INVPCID KO EXIT\n");
}

module_init(invpcid_init);
module_exit(invpcid_exit);
