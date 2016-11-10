/*
 * wrapper header for calling mysema_ functions */
#define SEMA_INIT		323 
#define SEMA_DOWN		324
#define SEMA_USERPRIO	325
#define SEMA_UP			326
#define SEMA_RELEASE	327

#define FIFO	0
#define OS		1
#define	USER	2

int mysema_init(int sema_id, int start_value, int mode);
int	mysema_down(int sema_id);
int mysema_down_userprio(int sema_id, int priority);
int	mysema_up(int sema_id);
int mysema_release(int sema_id);

int mysema_init(int sema_id, int start_value, int mode)
{
	return syscall(SEMA_INIT,sema_id,start_value,mode);
}

int	mysema_down(int sema_id)
{
	return syscall(SEMA_DOWN,sema_id);
}

int mysema_down_userprio(int sema_id, int priority)
{
	return syscall(SEMA_USERPRIO,sema_id,priority);
}

int	mysema_up(int sema_id)
{
	return syscall(SEMA_UP,sema_id);
}

int mysema_release(int sema_id)
{
	return syscall(SEMA_RELEASE,sema_id);
}
