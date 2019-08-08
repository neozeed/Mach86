1,/ServPort/c\
#include "mach.h"\
#include "mach_msg.h"\
\
#define FastAssign 1\
#define TypeCheck 1\
\
 static port_t	mach_reply_port;\
 static GeneralReturn	GR;\
\
\
void InitMach (reply_port)\
	port_t		reply_port;\
{\
	port_t		tmp_reply_port;\
	kern_return_t	result;\
\
	if (reply_port != PORT_NULL)\
		mach_reply_port = reply_port;\
	 else if ((result = port_allocate(task_self(), &tmp_reply_port)) == KERN_SUCCESS)\
		mach_reply_port = tmp_reply_port;\
}\
\
kern_return_t	port_allocate ( ServPort,my_port)
s/ReplyPort/mach_reply_port/
s/TRUE/1/
s/FALSE/0/
