$1 ~ /^set_op_types$/{
	for (i = 6; i < 7; i++)
	    arg[i] = F

	n = split($2,arg,",")
	if ( n == 6 )
	    printf "movw $(typ_%s+(typ_%s<<8)),op_types4(fp)\n",arg[5],arg[6]
	
	if ( n == 5 )
	    printf "movb $(typ_%s),op_types4(fp)\n",arg[5]

	if ( n > 2)
	    {
	    printf "movl $(typ_%s+(typ_%s<<8)+(typ_%s<<16)+(typ_%s<<24))",arg[1],arg[2],arg[3],arg[4]
	    printf ",op_types(fp)\n"
	    }

	if ( n == 2 )
	    printf "movw $(typ_%s+(typ_%s<<8)),op_types(fp)\n",arg[1],arg[2]

	if ( n == 1)
	    printf "movb $typ_%s,op_types(fp)\n",arg[1]
	}

$1 ~ /^inst_type$/ {

	print "# define xxx 0"
	split($2,opcode,",")
	for ( i =1 ; i <= length(opcode[2]); i++ )
	    {
	    printf "# define xxx xxx+it_%s\n",substr(opcode[2],i,1)
	    }

	print "# define xxx xxx<<4"

	for ( i =1 ; i <= length(opcode[1]); i++ )
	    {
	    printf "# define xxx xxx+it_%s\n",substr(opcode[1],i,1)
	    }

	print ".byte xxx"

	}

$1 !~ /^set_op_types$|^inst_type$/
