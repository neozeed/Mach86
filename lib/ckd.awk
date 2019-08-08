BEGIN {
    unknown = 0;
    line = 0;
}

NR == 1 {
    for (i=1; i<=NF; i++)
    {
	switch = $i;
	if (switch ~ /^-.*=/)
	{
	    n = split(switch, x, "=");
	    if (n == 2)
	    {
		switch = x[1];
		param = x[2];
	    }
	}
	if (switch == "-output")
	{
	    output = param;
	}
	else if (switch == "-error")
	{
	    error = param;
	}
	else
	{
	    unknown++;
	    if (unknown != i)
	    {
		emesg = sprintf("ckd.awk: unknown switch %s\n", switch);
	        enumb = 11;
		exit;
	    }
	}
    }
    if (unknown == 0)
	next;
}

{
    if (output == "")
	print $0;
    else
	print $0 >> output;
}

$1 !~ /[<>]/ {
    n = split($1, x, "a");
    if (n != 2)
    {
	n = split($1, x, "c");
	enumb = 1;
    }
    if (n != 2)
    {
	n = split($1, x, "d");
	enumb = 2;
    }
    if (n != 2)
	enumb = 3;
    temp = x[2];
    n = split(temp, x, ",")
    lline = line;
    line = x[1];
    if (enumb)
    {
	emesg = sprintf("[ bad range ]\n");
	exit;
    }
    if (nlast && last[2] != "#else" && last[2] != "#endif")
    {
	line = lline-1;
	emesg = sprintf("[ missing last '#else' or '#endif' ]\n");
	enumb = 4;
	exit;
    }
    nlast = 0;
    state = "first";
    next;
}

$1 == "<" {
    emesg = sprintf("[ missing original contents ]\n");
    enumb = 5;
    exit;
}

state == "first" {
    if ($2 != "#if" && $2 != "#endif")
    {
	emesg = sprintf("[ missing first '#if' or '#endif' ]\n");
	enumb = 6;
	exit;
    }
    if ($3 !~ /^CMU$/ && $3 !~ /^CMU_/ && $3 !~ /^CS_/ && $3 !~ /^N/)
    {
	emesg = sprintf("[ missing local symbol in first '#if' or '#endif' ]\n");
	enumb = 12;
	exit;
    }
    state = body;
}

$1 == ">" {
    if ($2 == "#if" || $2 == "#ifdef" || $2 == "#ifndef")
    {
	stack[++sp] = $0;
    }
    else if ($2 == "#else" || $2 == "#endif")
    {
	n = split(stack[sp], cond);
	if (n < NF)
	    n = NF;
	for (i=3; i<=n; i++)
	    if ($i != cond[i])
	    {
		if (i == 4 && cond[i] == ">" && cond[3] ~ /N*/) continue;
		if (i == 5 && cond[i] == "0" && cond[3] ~ /N*/) continue;
		emesg = sprintf("[ mismatched '%s' (with '%s') ]\n", $0, stack[sp]);
		enumb = 7;
		exit;
	    }
	if ($2 == "#endif" && sp-- <= 0)
	{
	    emesg = sprintf("[ extra '%s' ]\n", $0);
	    enumb = 8;
	    exit;
	}
    }
    nlast = split($0, last);
    line++;
}

END {
    if (enumb == 0)
    {
	if (sp != 0)
	{
	    emesg = sprintf("[ missing '#endif' at EOF ]\n");
	    enumb = 9;
	}
	else if (nlast && last[2] != "#else" && last[2] != "#endif")
	{
	    emesg = sprintf("[ missing last '#else' or '#endif' ]\n");
	    enumb = 10;
	}
    }
    if (emesg != "")
    {
	if (output == "")
	    printf("%s", emesg);
	else
	    printf("%s", emesg) >> output;
    }
    if (enumb && error != "")
	print enumb, line >error;
}
