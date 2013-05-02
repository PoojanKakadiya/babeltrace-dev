#ifndef _BABELTRACE_INCLUDE_COMPAT_DIRENT_H
#define _BABELTRACE_INCLUDE_COMPAT_DIRENT_H

#include <dirent.h>

#ifdef __MINGW32__
static inline
int readdir_r (DIR *dirp, struct dirent *entry, struct dirent **result)
{
	errno = 0;
	entry = readdir (dirp);
	*result = entry;
	if (entry == NULL && errno != 0) {
		return -1;
	}
	return 0;
}
#endif

static inline
int compat_dirfd(DIR * dirp)
{
#ifdef BABELTRACE_HAVE_OPENAT
	return dirfd(dirp);
#else
	/* ignore dirfd - see compat_openat() */
	return 0;
#endif
}

#endif
