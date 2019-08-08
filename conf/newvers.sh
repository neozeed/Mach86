#/*	newvers.sh	6.1	84/05/25	*/
if [ ! -r version ]; then echo 0 > version; fi
touch version
awk '	{	version = $1 + 1; }\
END	{
		printf "char version[] = \"Mach/4.3/2/1 WB#%d: ", version > "vers.c";\
		printf "%d\n", version > "version"; }' < version;
echo "`date`"'\n";' >> vers.c
