#include <vnode.h>
#include <synch.h>

/*
The structure that represents a file, has a vnode which can be
thought of as the actual file, and the flags that this file was
opened with.
*/
struct file {
	struct vnode* fvnode;
	int flags;
	char* name;
	struct lock* readwritelock;
	int seekpsnwrite;
	int seekpsnread;
};

struct file * initfile(struct vnode* fvnode, int flags, const char* name);
int cleanupfile(struct file* f);

