
#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <synch.h>
#include <file.h>
#include <vfs.h>
#include <vnode.h>

struct file *
initfile(struct vnode* fvnode, int flags, const char* name) {
  //kprintf("hello");

  /*KASSERT(f != NULL);
  KASSERT(fvnode != NULL);*/

  struct file *f = kmalloc(sizeof(struct file));
  if(f == NULL) {
    return NULL;
  }

  f->fvnode = fvnode;
  f->flags = flags;
  f->name = kstrdup(name);
  f->readwritelock = lock_create("readwritelock");
  f->seekpsnwrite = 0;  //write position
  f->seekpsnread = 0; //read position

  return f;
}

int cleanupfile(struct file* f) {
  //kprintf("%s \n", f->name);

	KASSERT(f != NULL);
  KASSERT(f->readwritelock != NULL);
   // kfree(f->name);
    if(f->fvnode != NULL) {
	   vfs_close(f->fvnode);
    }
  //  kprintf("no problem before lockdestroy \n");
    lock_destroy(f->readwritelock);
  //  kprintf("no problem before kfree \n");
  	kfree(f->name);
  	kfree(f);
  
  	return 0;
}

