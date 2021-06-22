/*
 * Description:         A module for super simple file system
 * Version:             1.2
 * Author:              Ayush Sharma
*/

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <asm/atomic.h>
#include <asm/uaccess.h>
#include <linux/sched.h>

/*
 * Description:         A function for creating inode
 * Parameters:          superblock and mode(for type and permission)
 * Return:              inode pointer
*/
static struct inode *s2fs_make_inode(struct super_block *sb, int mode)
{
        struct inode *inode;
        inode = new_inode(sb);
        if (!inode)
                return NULL;
        inode->i_mode = mode;
        inode->i_uid = current_uid();
        inode->i_blocks = 0;
        inode->i_atime = inode->i_mtime = inode->i_ctime = current_time(inode);
        inode->i_ino = get_next_ino();
        return inode;
}

/*
 * Description:         A function for reading file
 * Parameters:          file pointer, read buffer, size for buffer, offset for file
 * Return:              size of buffer
*/
#define TMPSIZE 20
static ssize_t s2fs_read_file(struct file *filp, char *buf, size_t count, loff_t *offset)
{
        //atomic_t *counter = (atomic_t *) filp->private_data;
        /*uint8_t *data = "Hello World!\n";
        size_t datalen = strlen(data);
        //msg = (char *)kmalloc(32, GFP_KERNEL);
        //char msg[12] = "Hello World!";

        //strcpy(msg,"Hello World!");
        //pid = filp->private_data;

        if (count > datalen) {
                count = datalen;
        }
        if (copy_to_user(buf, data + *offset, count)) 
        {
                return -EFAULT;
        }*/
        char data[TMPSIZE];
        int datalen = snprintf(data, TMPSIZE, "%s\n", "Hello World!");
        if (*offset > datalen)
                return 0;
        if (count > datalen - *offset)
               count = datalen - *offset;
        if (copy_to_user(buf, data + *offset, count))
                return -EFAULT;
        *offset += count;
        return count;
}

//create dummy functions
//write
//open
static ssize_t s2fs_write_file(struct file *filp, const char *buf, size_t count, loff_t *offset)
{
        return 0;
}
static int s2fs_open(struct inode *inode, struct file *filp)
{
        return 0;
}
//structure for file "bar"
static struct file_operations s2fs_fops = {
        .open   = s2fs_open,
        .read   = s2fs_read_file,
        .write  = s2fs_write_file,
};

/*
 * Description:         A function for creating file
 * Parameters:          superblock, directory pointer, file name
 * Return:              pointer to dentry for file "bar"
*/
static struct dentry *s2fs_create_file (struct super_block *sb, struct dentry *dir, const char *file_name)
{
        struct dentry *dentry;
        struct inode *inode;
        struct qstr qname;
        qname.name = file_name;
        qname.len = strlen(file_name);
        qname.hash = full_name_hash(dir, file_name, qname.len);
        dentry = d_alloc(dir, &qname);
        if (!dentry)
                goto out;
        inode = s2fs_make_inode(sb, S_IFREG | 0644);
        if (!inode)
                goto out_dput;
        inode->i_fop = &s2fs_fops;
        //inode->i_private = counter;   
        d_add(dentry, inode);
        return dentry;
        out_dput:
                dput(dentry);
        out:
                return 0;
}

/*
 * Description:         A function for creating directory
 * Parameters:          superblock, root pointer, directory name
 * Return:              pointer to dentry for directory "foo"
*/
static struct dentry *s2fs_create_dir (struct super_block *sb, struct dentry *parent, const char *dir_name)
{
        struct dentry *dentry = parent;
        struct inode *inode;
        struct qstr qname;
        qname.name = dir_name;
        qname.len = strlen (dir_name);
        qname.hash = full_name_hash(parent, dir_name, qname.len);
        dentry = d_alloc(parent, &qname);
        if (!dentry)
                goto out;
        inode = s2fs_make_inode(sb, S_IFDIR | 0755);
        if (!inode)
                goto out_dput;
        inode->i_op = &simple_dir_inode_operations;
        inode->i_fop = &simple_dir_operations;
        d_add(dentry, inode);
        return dentry;
        out_dput:
                dput(dentry);
        out:
                return 0;
}

//structure for super
static struct super_operations s2fs_s_ops = {
        .statfs         = simple_statfs,
        .drop_inode     = generic_delete_inode,
};

/*
 * Description:         A function for filling superblock
 * Parameters:          superblock, data and silent flag
 * Return:              0 or location
*/
static int s2fs_fill_super (struct super_block *sb, void *data, int silent)
{
        char *directory_name = "foo";
        char *file_name = "bar";
        struct inode *root;
        struct dentry *root_dentry, *dir;
        //sb->s_blocksize = PAGE_CACHE_SIZE;
        //sb->s_blocksize_bits = PAGE_CACHE_SHIFT;
        sb->s_blocksize = VMACACHE_SIZE;
        sb->s_blocksize_bits = VMACACHE_SIZE;
        sb->s_magic = 0x19980122;
        sb->s_op = &s2fs_s_ops;

        root = s2fs_make_inode (sb, S_IFDIR | 0755);
        if (!root)
                goto out;
        root->i_op = &simple_dir_inode_operations;
        root->i_fop = &simple_dir_operations;

        root_dentry = d_make_root(root);
        if (!root_dentry)
                goto out_iput;
        sb->s_root = root_dentry;

        dir = s2fs_create_dir(sb, root_dentry, directory_name);
        if(!dir)
        {
                return -ENOMEM;
        }
        s2fs_create_file(sb, dir, file_name);
        return 0;

        out_iput:
                iput(root);
        out:
                return -ENOMEM;
}

static struct dentry *s2fs_get_super(struct file_system_type *fst, int flags, const char *devname, void *data)
{
        return mount_nodev(fst, flags, data, s2fs_fill_super);
}
static struct file_system_type s2fs_type = {
        .owner          = THIS_MODULE,
        .name           = "s2fs",
        .mount          = s2fs_get_super,
        .kill_sb        = kill_litter_super,
};
static int __init s2fs_init(void) {
        return register_filesystem(&s2fs_type);
}
static void __exit s2fs_exit(void) {
        unregister_filesystem(&s2fs_type);
}
module_init(s2fs_init);
module_exit(s2fs_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("ayu");