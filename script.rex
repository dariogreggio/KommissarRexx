/* primo prog REXX */
/* parse arg */
numeric digits 2
i=0
do 4
say "dario" + i
i = i+1
end
say time('e')
call piciu
call rotto 22 34
exit
piciu:
say random(1,90)
return 3
/* */
rotto:
/* say "rotto!" + arg(1) */
arg c1 c2
say
say "rotto" + c1+c2
return
