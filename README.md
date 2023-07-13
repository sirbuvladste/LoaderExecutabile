**﻿Loader de Executabile**

 Sa se implementeze sub forma unei biblioteci partajate/dinamice un loader de fisiere executabile in format ELF pentru Linux. Loader-ul va incarca fisierul executabil in memorie pagina cu pagina, folosind un mecanism de tipul demand paging - o pagina va fi încarcata doar in momentul in care este nevoie de ea. Pentru simplitate, loader-ul va rula doar executabile statice - care nu sunt link-ate cu biblioteci partajate/dinamice.
 
 In procesul de rezolvare a acestei cerinte a trebuit sa imi insusesc cunostinte legate de ELF files, mmap, memset, mprotect si cum funtioneaza un loader.
Ca sursa principala de documentatie am folosit:
https://man7.org/linux/man-pages/

 Etapele pe care programul le realizeaza sunt:
 - verificarea semnalului si informatiei primite
 - cautrea segmentului unde s-a produs page fault
 - aloc un vector in "data" pentru a memorarea paginilor mapate
 - mapez pagina daca nu este deja cu permisiuni de read si write
 - verific corner case-ul:
 -> daca file_size < mem_size sunt doua cazuri posibile:
 	1) daca file_size nu cuprinde ultima pagina complet, este un caz special 
de mapare, asa ca la flag-uri se adauga MAP_ANONYMOUS
 	2) daca file_size intra si pe pagina urmatoare decat celei pe care trebuie
mapata, se scrie mai mult spatiu decat trebuie, astfel dupa ce se realizeaza
maparea se reseteaza la 0 toti bitii extra
 - setez permisiunile memoriei de pagina corespunzator
