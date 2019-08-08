BEGIN {
    U["a"] = "A"; U["b"] = "B"; U["c"] = "C"; U["d"] = "D"; U["e"] = "E";
    U["f"] = "F"; U["g"] = "G"; U["h"] = "H"; U["i"] = "I"; U["j"] = "J";
    U["k"] = "K"; U["l"] = "L"; U["m"] = "M"; U["n"] = "N"; U["o"] = "O";
    U["p"] = "P"; U["q"] = "Q"; U["r"] = "R"; U["s"] = "S"; U["t"] = "T";
    U["u"] = "U"; U["v"] = "V"; U["w"] = "W"; U["x"] = "X"; U["y"] = "Y";
    U["z"] = "Z";
    sp = 0;
    #
    #  Symbol definition table
    #
    #  P = pre-defined
    #  O = makefile option
    #  D = device count
    #  F = feature
    #  d = #define/#undef symbol
    #
    #  S = standard 4.2
    #  C = CMU
    #
    #  N = allowed in #ifdef/#ifndef
    #
    #  I = allowed in #if 
    #
    symbols["CMU"]	= "PCxI";
    symbols["lint"]	= "PSNx";
    symbols["notdef"]	= "PSNx";
    symbols["vax"]	= "PSNx";
    symbols["COMPAT"]	= "OSNx";
    symbols["GPROF"]	= "OSNx";
    symbols["KERNEL"]	= "OSNx";
    symbols["MRSP"]	= "OSNx";
    symbols["INET"]	= "OSNx";
    symbols["NS"]	= "OSNx";
    symbols["PGINPROF"]	= "OSNx";
    symbols["TRENDATA"]	= "OSNx";
    symbols["QUOTA"]	= "OSNx";
    symbols["TRACE"]	= "OSNx";
    symbols["UUDMA"]	= "OSNx";
    symbols["REMDEBUG"]	= "OCNx";
    symbols["NKG"]	= "DSxI";
    symbols["NPS"]	= "DSxI";
    symbols["NDZ"]	= "DSxI";
    symbols["NDH"]	= "DSxI";
    symbols["NUU"]	= "DSxI";
    symbols["NLOOP"]	= "DSxI";
    symbols["VAX780"]	= "OSNI";
    symbols["VAX750"]	= "OSNI";
    symbols["VAX730"]	= "OSNI";
    symbols["VAXM"]	= "OSNI";
    symbols["plock"]	= "dSNx";
    symbols["prele"]	= "dSNx";
}

length($1) <= 0 || substr($0, 1, 1) != "#" {
	next;
}

{
    if ($1 != "#")
    {
	word = substr($1, 2, length($1)-1);
	idx = 2;
    }
    else
    {
	word = $2;
	idx = 3;
    }
    args = "";
    for (i=idx; i<=NF; i++)
    {
	if (i == idx)
	    args = $i;
	else
	    args = args " " $i;
    }
}

word == "include" {
    l = length($idx);
    if (substr($idx, 1, 1) != "\"" || substr($idx, l-2, 3) != ".h\"")
    {
	printf "[ bad %s at line %d ]\n", $0, NR;
    }
    if ($idx ~ /"..\//)
	next;
    temp = substr($idx, 2, l-4);
    l = length(temp);
    option = "";
    for (i=1; i<length; i++)
    {
	c = substr(temp, i, 1);
	if (c >= "a" && c <= "z")
	    c = U[c];
	option = option c;
    }
    temp = "FCxI";
    if (option !~ /_/)
    {
	option = "N" option;
	temp = "DCxI";
    }
    if (symbols[option] == "")
	symbols[option] = temp;
    included[option] = "'" $0 "' at line " NR;
    used[option] = 0;
    uses++;
    next;
}

word == "ifdef" || word == "ifndef" {
    if (symbols[$idx] !~ /N/ || NF != idx)
	printf "[ improper \"%s\" at line %d ]\n", $0, NR;
    uses++;
    used[$idx]++;
    cstack[++sp] = "";
    next;
}

word == "if" {
    targs = "";
    xargs = "";
    for (i=idx; i<=NF; i++)
    {
	if ($i != "||" && $i != "&&" && $i != ">" && $i != "<" && $i != ">=" && $i != "<=" && $i != "==" && $i !~ /^[0-9]*$/)
	{
	    symbol = $i;
	    def = "I";
	    if (symbol ~ /^!/)
		symbol = substr(symbol, 2, length(symbol)-1);
	    if (symbol ~ /^defined/)
	    {
		symbol = substr(symbol, 8, length(symbol)-7);
		def = "N";
	    }
	    while (symbol ~ /^\(/)
		symbol = substr(symbol, 2, length(symbol)-1);
	    while (symbol ~ /\)$/)
		symbol = substr(symbol, 1, length(symbol)-1);
	    temp = symbols[symbol];
	    if (temp == "")
		printf "[ symbol '%s' used before defined at line %d ]\n", $i, NR;
	    if ((def == "I" && temp !~ /I/) || (def == "N" && temp !~ /N/))
		printf "[ improper \"%s\" at line %d ]\n", $0, NR;
	    if (temp ~ /C/)
	    {
		targs = args;
		xargs = args;
	    }
	    if (temp ~ /D/)
		xargs = $idx;
	    used[symbol]++;
	    uses++;
	}
    }
    cstack[++sp] = targs;
    xstack[sp] = xargs;
    next;
}

word == "else" || word == "endif" {
    if (cstack[sp] != "" && cstack[sp] != args && xstack[sp] != args)
    {
	printf("[ incorrectly matched '%s' (with '%s') at line %d ]\n", $0, cstack[sp], NR);
    }
    if (word == "endif")
    {
	if (sp <= 0)
	    printf("[ extra '%s' at line %d ]\n", $0, NR);
	else
	    sp--;
    }
    next;
}

word == "define" || word == "undef" {
    next;
}

{
    printf("[ unknown control '%s' at line %d ]\n", $0, NR);
}

END {
    while (sp > 0)
	printf("[ missing \"#endif %s\" at EOF ]\n", cstack[sp--]);
    if (uses)
    {
	for (symbol in used)
	    if (used[symbol] == 0)
		printf("[ unused %s ]\n", included[symbol]);
	max = 0;
	for (symbol in used)
	{
	    if (used[symbol] && length(symbol) > max)
		max = length(symbol);
	}
	format = "%-" (max+2) "s%d\n";
	for (symbol in used)
	    if (used[symbol])
		printf(format, symbol ":", used[symbol]);
    }
}
