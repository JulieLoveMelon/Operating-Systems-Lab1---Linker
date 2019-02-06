This repository includes four files, Linker.cpp, lab1-linker-fall2018.pdf, Makefile and README.txt.
The pdf is the requirements for the lab from the instructer of the course. 
Linker.cpp is the main file which contains the source code satisfying the demands in the pdf.
Makefile organizes code compilation for the Linux system. 
In the lab, I wrote a two-pass linker. Pass one parses the input and verifies the correct syntax and determines the base address for each module and the absolute address for each defined symbol, storing the latter in a symbol table. Pass two again parses the input and uses the base addresses and the symbol table entries created in pass one to generate the actual output by relocating relative addresses and resolving external references.