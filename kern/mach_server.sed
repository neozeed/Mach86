s/task_t	Arg/port_t	Arg/
s/thread_t	Arg/port_t	Arg/
s/convert_port\(.*\)LocalPort/convert_port\1RemotePort/
s/AccentType/..\/accent\/accenttype/
s/mach_if\.h/..\/kern\/mach_if.h/
s/accent\.h/..\/accent\/accent.h/
s/MachServer/mach_server/
//d
