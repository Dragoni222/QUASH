On the cycle servers, the output operators (> and >>) seem to make multiple files, for example:
cat a.txt > b.txt

will make
b
b.t
b.tx
b.txt 
each with identical correct output. I cannot reproduce this issue on my home computer, and cannot even find any line
 in my code capable of even creating a file in the first place. If possible, I would really appreciate if this weird bug is ignored if it appears,
 as b.txt should be correct.

 Additionally, my shell sometimes reprints the starter text [QUASH]$ several times when running commands. This seems like basically whitespace and does not impede 
 functionality other than visuals (though I have tried to fix the issue with little success).

 As far as I can tell, this should be capable of everything in tiers 1-3 and tier 4 except for pipes and redirects with built in commands. 

 Thank you!

 Joel Harrison
 3114086