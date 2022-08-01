aa=2
/*  "if aa==1 then bb=1 else bb=2 */
if aa==1 then do
bb=1
end
else do
bb=7
end
say aa
say bb
if aa==2 then do
bb=3
end
say aa
say bb
if bb==4 then say "GODO"
