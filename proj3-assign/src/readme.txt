I implemented btree without duplicates.
When I include key.C in my btleaf_page.h, it leads to a multiple definition problem. According to solution on stackoverflow, I created a key.h and include it.
Besides, keys' format should be unix rather than dos to get the output the same as answer.