#include <types.h>
#include <kern/errno.h>
#include <kern/unistd.h>
#include <kern/fcntl.h>
#include <copyinout.h>
#include <lib.h>
#include <uio.h>
#include <syscall.h>
#include <vnode.h>
#include <vfs.h>
#include <current.h>
#include <proc.h>
#include <synch.h>
#include <array.h>
#include <file.h>
#include "opt-A2.h"


/* handler for write() system call                  */
/*
 * n.b.
 * This implementation handles only writes to standard output 
 * and standard error, both of which go to the console.
 * Also, it does not provide any synchronization, so writes
 * are not atomic.
 *
 * You will need to improve this implementation
 */

int
sys_write(int fdesc,userptr_t ubuf,unsigned int nbytes,int *retval)
{
  #if OPT_A2

  struct iovec iov;
  struct uio u;
  int res;
  struct file* f;
  struct lock* rwlock;

  //check for invalid file descriptor:
  //anything above 127 or below 0 is a bad file descriptor
  if(fdesc > 127 || fdesc < 0) {
    return EBADF;
  }
  f = array_get(curproc->filetable, fdesc);
  //the file table index is null, thus it has not been opened.
  if(f == NULL) {
    return EBADF;
  }
  //read only
  if((f->flags & O_ACCMODE) == O_RDONLY) {
    return EBADF;
  }
  rwlock = f->readwritelock;
  lock_acquire(rwlock);
   //kprintf("LOCK NAME %d: %s  ACQUIRED! \n", fdesc, rwlock->lk_name);

  //get the file again to get any updated changes.
  f = array_get(curproc->filetable, fdesc);
  if(f == NULL) {

    lock_release(rwlock);
    //    kprintf("LOCK NAME %d: %s  RELEASED! \n", fdesc, rwlock->lk_name);
    return EBADF;
  }
  if((f->flags & O_ACCMODE) == O_RDONLY) {

    lock_release(rwlock);
    //    kprintf("LOCK NAME %d: %s  RELEASED! \n", fdesc, rwlock->lk_name);
    return EBADF;
  }
  //kprintf("WRITING");
  DEBUG(DB_SYSCALL,"Syscall: write(%d,%x,%d)\n",fdesc,(unsigned int)ubuf,nbytes);

  /* set up a uio structure to refer to the user program's buffer (ubuf) */
  iov.iov_ubase = ubuf;
  iov.iov_len = nbytes;
  u.uio_iov = &iov;
  u.uio_iovcnt = 1;
  u.uio_offset = f->seekpsnwrite;  
  u.uio_resid = nbytes;
  u.uio_segflg = UIO_USERSPACE;
  u.uio_rw = UIO_WRITE;
  u.uio_space = curproc->p_addrspace;

  KASSERT(f != NULL);

  res = VOP_WRITE(f->fvnode, &u);
  
  if (res) {
    //lock_release(f->readwritelock);

    lock_release(rwlock);
   // kprintf("LOCK NAME %d: %s  RELEASED! \n", fdesc, rwlock->lk_name);
    return res;
  }
  /* pass back the number of bytes actually written */
  *retval = nbytes - u.uio_resid;
  //don't need to increment seek positions for console
  if(fdesc > 2)
  f->seekpsnwrite = f->seekpsnwrite + *retval;
  KASSERT(*retval >= 0);

  lock_release(rwlock);
  //kprintf("LOCK NAME %d: %s  RELEASED! \n", fdesc, rwlock->lk_name);
  return 0;
  /*
  *
  *
  *END OF MY IMPLEMENTATION
  *BELOW IS THE ORIGINAL IMPLEMENTATION
  */
  #else

  struct iovec iov;
  struct uio u;
  int res;

  DEBUG(DB_SYSCALL,"Syscall: write(%d,%x,%d)\n",fdesc,(unsigned int)ubuf,nbytes);
  
  /* only stdout and stderr writes are currently implemented */
  if (!((fdesc==STDOUT_FILENO)||(fdesc==STDERR_FILENO))) {
    return EUNIMP;
  }
  KASSERT(curproc != NULL);
  KASSERT(curproc->console != NULL);
  KASSERT(curproc->p_addrspace != NULL);

  /* set up a uio structure to refer to the user program's buffer (ubuf) */
  iov.iov_ubase = ubuf;
  iov.iov_len = nbytes;
  u.uio_iov = &iov;
  u.uio_iovcnt = 1;
  u.uio_offset = 0;  /* not needed for the console */
  u.uio_resid = nbytes;
  u.uio_segflg = UIO_USERSPACE;
  u.uio_rw = UIO_WRITE;
  u.uio_space = curproc->p_addrspace;

  res = VOP_WRITE(curproc->console,&u);
  if (res) {
    return res;
  }

  /* pass back the number of bytes actually written */
  *retval = nbytes - u.uio_resid;
  KASSERT(*retval >= 0);
  return 0;

  #endif

}

#if OPT_A2
int
sys_read(int fd, void *buf, size_t buflen, int *retval) {
  
  struct file* f;
  struct iovec iov;
  struct uio u;
  int result = 0;
  struct lock* rwlock; 
  //KASSERT(array_get(curproc->filetable, fd) != NULL);

  if(fd > 127 || fd < 0) {
    return EBADF;
  }
  f = array_get(curproc->filetable, fd);
  if(f == NULL) {
    return EBADF;
  }
  //write only
  if((f->flags & O_ACCMODE) == O_WRONLY) {
    return EBADF;
  }
  rwlock = f->readwritelock;
  lock_acquire(rwlock);
 // kprintf("LOCK NAME %d: %s  ACQUIRED! \n", fd, rwlock->lk_name);
  //get the file again to get updated information
  f = array_get(curproc->filetable, fd);
  if(f == NULL) {
    
    lock_release(rwlock);
  //  kprintf("LOCK NAME %d: %s  RELEASED! \n", fd, rwlock->lk_name);
    return EBADF;
  }
  if((f->flags & O_ACCMODE) == O_WRONLY) {
   
    lock_release(rwlock);
  //   kprintf("LOCK NAME %d: %s  RELEASED! \n", fd, rwlock->lk_name);
    return EBADF;
  }


  iov.iov_ubase = (userptr_t)buf;
  iov.iov_len = buflen;
  u.uio_iov = &iov;
  u.uio_iovcnt = 1;
  u.uio_offset = f->seekpsnread;  
 // u.uio_offset = 0;  
  u.uio_resid = buflen;
  u.uio_segflg = UIO_USERSPACE;
  u.uio_rw = UIO_READ;
  u.uio_space = curproc->p_addrspace;

  //if read ran into an error, we return it.
  KASSERT(f->fvnode != NULL);
  result = VOP_READ(f->fvnode, &u);
  if (result) {
    //lock_release(f->readwritelock);

    lock_release(rwlock);
 //   kprintf("LOCK NAME %d: %s  RELEASED! \n", fd, rwlock->lk_name);
    return result;
  }

  /*
    u.uio_resid is the amount that still needs to be read.
    this value should be 0 normally. 
  */
  *retval = buflen - u.uio_resid;
  f->seekpsnread = f->seekpsnread + *retval;
  lock_release(rwlock);
 // kprintf("LOCK NAME %d: %s  RELEASED! \n", fd, rwlock->lk_name);
  return 0;

}
int
sys_open(const char *filename, int flags, int *fd) {
  lock_acquire(curproc->opencloselock);
  struct vnode* filenode;
  int filedescriptor = -1;

  //vfs_open returns the vnode in filenode, and returns a integer value denoting it's success.
  char *temp = NULL;
  temp = kstrdup(filename);
  int result = vfs_open(temp, flags, 0, &(filenode));
  if(result != 0) {
    lock_release(curproc->opencloselock);
    return result;
  }
  kfree(temp);
  KASSERT(filenode != NULL);

  //make our file, a wrapper for vnode. Stores vnode, lock, ect.. May be useful later.
  struct file* f;
  f = initfile(filenode, flags, filename);
  KASSERT(f != NULL);
  KASSERT(f->fvnode != NULL);

  //go through the file table, find an unused filedescriptor.
  for(int i = 0; i < 128; i++) {
    if(array_get(curproc->filetable, i) == NULL) {
      filedescriptor = i;
      break;
    }
  }

  //if fd>=0 then a file descriptor was found. Else file table is probably full.
  if(filedescriptor >= 0) {
    array_set(curproc->filetable, filedescriptor, f);
    *fd = filedescriptor;
  } else {
    //kprintf("RAN OUT OF FD FD %d \n", *fd);
    lock_release(curproc->opencloselock);
    cleanupfile(f);
    return EMFILE;
  }
 // kprintf("OPENED FD %d \n", *fd);
  lock_release(curproc->opencloselock);
  return 0;
}

int
sys_close(int fd){

struct file* f;
struct lock* rwlock;
  //fd 0,1,2 reserved for console, cannot close.
  if(fd > 127 || fd < 3) {
    return EBADF;
  }
  f = array_get(curproc->filetable, fd);
  if(f == NULL) {
    return EBADF;
  }
  rwlock = f->readwritelock;
  lock_acquire(rwlock);
  //kprintf("CLOSED FD %d \n", fd);
  //kprintf("LOCK NAME %d: %s  ACQUIRED! \n", fd, rwlock->lk_name);
  KASSERT(f->fvnode != NULL);

  array_set(curproc->filetable, fd, NULL);
  lock_release(rwlock);
  //kprintf("LOCK NAME %d: %s  RELEASED! \n", fd, rwlock->lk_name);
  //cleanupfile closes the vnode, destroys the lock, frees everything else
  cleanupfile(f);
  return 0;
}

#endif
