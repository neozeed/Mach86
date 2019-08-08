#! /bin/sh
#
# 5799-CGZ (C) COPYRIGHT IBM CORPORATION  1986
# LICENSED MATERIALS - PROPERTY OF IBM
# REFER TO COPYRIGHT INSTRUCTIONS FORM NUMBER G120-2083
#
#$Header: adunload.sh,v 5.0 86/01/31 18:05:36 ibmacis ibm42a $
#$Source: /ibm/acis/usr/sys_ca/caio/RCS/adunload.sh,v $
# produced an optimized (with hand ex script)
# of adunload.c
#
# this is done basically by replacing 12 get/puts with a lm and 12 puts
# or 12 gets and a stm
#
umask 2
ASSOURCE=${1-adunload.s}
ex $ASSOURCE <<'+'
1
" following removes unneeded prolog
/cal/+1ka
'a,/r10,r4/s;^;|;
g/r11/s//r2/g
" following puts profile code back in!
g/^|.*profil/s/|//
g/r9/s//r3/g
g/r10/s//r0/g
/cas.r0,r4/s;|;;
" following replaces get/put's with ls ... stm
1
/get r3,0(r3)/ka
/put.*,44(r2)/kb
'a,'bv/^|/s;^;|;
'ba
ls r4,0(r3)
ls r5,0(r3)
ls r6,0(r3)
ls r7,0(r3)
ls r8,0(r3)
ls r9,0(r3)
ls r10,0(r3)
ls r11,0(r3)
ls r12,0(r3)
ls r13,0(r3)
ls r14,0(r3)
ls r15,0(r3)
stm r4,0(r2)
.
/ai r2,r2,48/ka
'a
/ b /kb
'bs; b ; bx ;
'am'b
/get r.,0(r2)/ka
'a
/get r.,44(r2)/+1kb
'b
'a,'bv/^|/s;^;|;
'ba
lm r4,0(r2)
sts r4,0(r3)
sts r5,0(r3)
sts r6,0(r3)
sts r7,0(r3)
sts r8,0(r3)
sts r9,0(r3)
sts r10,0(r3)
sts r11,0(r3)
sts r12,0(r3)
sts r13,0(r3)
sts r14,0(r3)
sts r15,0(r3)
.
/ai r2,r2,48/ka
'a
/ b /kb
'bs; b ; bx ;
'am'b
wq
+
