#ifndef	_Mach
#define	_Mach	       1

/* Module Mach */
/* 
 * WARNING FROM MATCHMAKER
 */
#include "AccentType.h"

 extern void InitMach () ;

#include "mach_types.h"

 extern kern_return_t	port_allocate ( );

 extern kern_return_t	port_deallocate ( );

 extern kern_return_t	port_enable ( );

 extern kern_return_t	port_disable ( );

 extern kern_return_t	port_select ( );

 extern kern_return_t	port_set_backlog ( );

 extern kern_return_t	port_status ( );

 extern kern_return_t	task_create ( );

 extern kern_return_t	task_terminate ( );

 extern kern_return_t	task_suspend ( );

 extern kern_return_t	task_resume ( );

 extern kern_return_t	task_threads ( );

 extern kern_return_t	task_ports ( );

 extern kern_return_t	task_status ( );

 extern kern_return_t	task_set_notify ( );

 extern kern_return_t	thread_create ( );

 extern kern_return_t	thread_terminate ( );

 extern kern_return_t	thread_suspend ( );

 extern kern_return_t	thread_resume ( );

 extern kern_return_t	thread_status ( );

 extern kern_return_t	thread_mutate ( );

 extern kern_return_t	vm_allocate ( );

 extern kern_return_t	vm_allocate_with_pager ( );

 extern kern_return_t	vm_deallocate ( );

 extern kern_return_t	vm_protect ( );

 extern kern_return_t	vm_inherit ( );

 extern kern_return_t	vm_read ( );

 extern kern_return_t	vm_write ( );

 extern kern_return_t	vm_copy ( );

 extern kern_return_t	vm_regions ( );

 extern kern_return_t	vm_statistics ( );

 extern kern_return_t	pager_clean ( );

 extern kern_return_t	pager_fetch ( );

#endif	_Mach
