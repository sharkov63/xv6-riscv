#include "param.h"
#include "types.h"

#include "riscv.h"
#include "spinlock.h"
#include "stat.h"

#include "defs.h"
#include "fs.h"
#include "sleeplock.h"

#include "file.h"

#define NULL 0

/// Create a symlink file at \p linkpath.
///
/// \return 0 on success
///
/// \param out on success \p *out contains referenced inode to the symlink file.
static int create_symlink_file(char *linkpath, struct inode **out) {
  int ret = 0;

  char link_name[DIRSIZ];
  struct inode *link_parent = nameiparent(linkpath, link_name);
  if (!link_parent)
    return -1;
  ilock(link_parent);

  struct inode *link = dirlookup(link_parent, link_name, NULL);
  if (link) {
    // Link file exists
    ret = -1;
    iput(link);
    goto unlock_parent;
  }

  // Link file does not exist, we must create it.
  link = ialloc(link_parent->dev, T_SYMLINK);
  if (!link) {
    // Failed to allocate inode for the link
    ret = -1;
    goto unlock_parent;
  }
  if ((ret = dirlink(link_parent, link_name, link->inum))) {
    // Failed to create a directory entry
    iput(link);
    goto unlock_parent;
  }
  ilock(link);
  ++link->nlink;
  iupdate(link);
  iunlock(link);
  *out = link;

unlock_parent:
  iunlockput(link_parent);
  return ret;
}

/// Implementation of 'symlink' system call.
uint64 sys_symlink() {
  uint64 ret = 0;

  char target[MAXPATH], linkpath[MAXPATH];
  int target_length = argstr(0, target, MAXPATH);
  if (target_length < 0 || target_length >= MAXPATH ||
      argstr(1, linkpath, MAXPATH) < 0)
    return -1;

  begin_op(); // begin FS transaction

  struct inode *link;
  if ((ret = create_symlink_file(linkpath, &link)))
    goto end_fs_transaction;
  ilock(link);

  if (link->type != T_SYMLINK) {
    link->type = T_SYMLINK;
    iupdate(link);
  }
  itrunc(link);
  ret = writei(link, 0 /* kernel src */, /* src = */ (uint64)(&target),
               /* off = */ 0, target_length + 1) < target_length + 1;
  iunlockput(link);
end_fs_transaction:
  end_op();
  return ret;
}

/// Implementation of 'readlink' system call.
uint64 sys_readlink() {
  uint64 ret = 0;

  char linkpath[MAXPATH];
  if (argstr(0, linkpath, MAXPATH) < 0)
    return -1;
  uint64 user_buffer;
  argaddr(1, &user_buffer);

  begin_op(); // begin FS transaction

  struct inode *link = namei(linkpath);
  if (!link) {
    ret = -1;
    goto end_fs_transaction;
  }
  if (link->type != T_SYMLINK) {
    ret = -1;
    goto iput_link;
  }

  ilock(link);
  ret = readi(link, 1 /* user dest */, user_buffer, 0, link->size) <= 0;
  iunlock(link);

iput_link:
  iput(link);
end_fs_transaction:
  end_op();
  return ret;
}
