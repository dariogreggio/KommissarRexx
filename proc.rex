/*trace A*/
z=0
say "chiamo1"
call piciu
say "chiamo2"
call merde 12 34
say "z alla fine: "+z+" "+z2
exit
piciu: procedure
z=3
z2=1
say "piciu"+random(1,90)+" z:"+z+" "+z2
return 3
/* */
merde:
arg c1 c2
say
say "merde" + c1+c2
return