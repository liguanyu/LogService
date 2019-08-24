#ifndef WRAP_H
#define WRAP_H

#include <sys/sem.h>
#include <cassert>
#include <semaphore.h>
#include <fcntl.h>
#include	<sys/types.h>	/* basic system data types */
#include	<sys/time.h>	/* timeval{} for select() */
#include	<time.h>		/* timespec{} for pselect() */
#include	<errno.h>
#include	<fcntl.h>		/* for nonblocking */
#include	<limits.h>		/* PIPE_BUF */
#include	<signal.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<sys/stat.h>	/* for S_xxx file mode constants */
#include	<unistd.h>
#include	<sys/wait.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <cstring>
#include <sys/socket.h>

#include	<stdarg.h>		/* for variable arg lists */



/* Miscellaneous constants */
#ifndef	PATH_MAX			/* should be in <limits.h> */
#define	PATH_MAX	1024	/* max # of characters in a pathname */
#endif

#define	MAX_PATH	1024
/* $$.ix [MAX_PATH]~constant,~definition~of$$ */
#define	MAXLINE		4096	/* max text line length */
/* $$.ix [MAXLINE]~constant,~definition~of$$ */
/* $$.ix [BUFFSIZE]~constant,~definition~of$$ */
#define	BUFFSIZE	8192	/* buffer size for reads and writes */

#define	FILE_MODE	(S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)
					/* default permissions for new files */
/* $$.ix [FILE_MODE]~constant,~definition~of$$ */
#define	DIR_MODE	(FILE_MODE | S_IXUSR | S_IXGRP | S_IXOTH)
					/* default permissions for new directories */
/* $$.ix [DIR_MODE]~constant,~definition~of$$ */

#define	SVMSG_MODE	(MSG_R | MSG_W | MSG_R>>3 | MSG_R>>6)
					/* default permissions for new SV message queues */
/* $$.ix [SVMSG_MODE]~constant,~definition~of$$ */
#define	SVSEM_MODE	(SEM_R | SEM_A | SEM_R>>3 | SEM_R>>6)
					/* default permissions for new SV semaphores */
/* $$.ix [SVSEM_MODE]~constant,~definition~of$$ */
#define	SVSHM_MODE	(SHM_R | SHM_W | SHM_R>>3 | SHM_R>>6)
					/* default permissions for new SV shared memory */
/* $$.ix [SVSHM_MODE]~constant,~definition~of$$ */

static void
err_sys(const char *fmt, ...)
{
	va_list		ap;

	va_start(ap, fmt);
	vprintf(fmt, ap);
	printf(" errno: %d, %s\n", errno, strerror(errno));
	va_end(ap);
	exit(1);
}

static void
err_printf(const char *fmt, ...)
{
	va_list		ap;

	va_start(ap, fmt);
	vprintf(fmt, ap);
	printf(" errno: %d, %s\n", errno, strerror(errno));
	va_end(ap);
}


static inline void
Sem_init(sem_t *sem, int pshared, unsigned int value)
{
	if (sem_init(sem, pshared, value) == -1)
		err_sys("sem_init error");
}

static inline int
Shmget(key_t key, size_t size, int flags)
{
	int		rc;

	if ( (rc = shmget(key, size, flags)) == -1)
		err_sys("shmget error, key is %lx\n", key);
	return(rc);
}

static inline key_t
Ftok(const char *pathname, int id)
{
	key_t	key;

	if ( (key = ftok(pathname, id)) == -1)
		err_sys("ftok error for pathname \"%s\" and id %d\n", pathname, id);
	return(key);
}

static inline void *
Shmat(int id, const void *shmaddr, int flags)
{
	void	*ptr;

	if ( (ptr = shmat(id, shmaddr, flags)) == (void *) -1)
		err_sys("shmat error");
	return(ptr);
}

static inline void
Shmctl(int id, int cmd, struct shmid_ds *buff)
{
	if (shmctl(id, cmd, buff) == -1)
		err_sys("shmctl error");
}

static inline
void Close_same_name_shm_before_get(const char* pathname){
	int shm_id = shmget(Ftok(pathname, 0), 0, SVSHM_MODE);
	if(shm_id == -1){
		return;
	}

	Shmctl(shm_id, IPC_RMID, nullptr);
}

static inline
void* Init_shm(int &shm_id, int len, const char* pathname, int shm_num = 0){
	Close_same_name_shm_before_get(pathname);
    void    *shm_ptr;
    shm_id = Shmget(Ftok(pathname, shm_num), len, SVSHM_MODE | IPC_CREAT | O_EXCL);
    shm_ptr = Shmat(shm_id, NULL, 0);
    return shm_ptr;
}

static inline
void* Get_shm(int &shm_id, int len, const char* pathname, int shm_num = 0){
    shm_id = Shmget(Ftok(pathname, shm_num), len, SVSHM_MODE );
	void* shm_ptr = Shmat(shm_id, NULL, 0);
    return shm_ptr;
}

static inline
void Close_shm(int shm_id){
    Shmctl(shm_id, IPC_RMID, NULL);
}


// copy num characters, and add a '\0', so change num+1 characters
static inline
char* lgy_strncpy(char* dest_, const char* src_, size_t num){
	char *ret = strncpy(dest_, src_, num);
	dest_[num] = '\0';
	return ret;
}


static inline 
int Connect (int __fd, __CONST_SOCKADDR_ARG __addr, socklen_t __len)
{
	int ret = connect(__fd, __addr, __len);
	// if(ret == 1){
	// 	if(errno == EINPROGRESS || errno == EWOULDBLOCK){
	// 		return ret;
	// 	}
	// 	err_sys("Connect error\n");
	// }
}


#endif