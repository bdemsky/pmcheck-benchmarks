#/bin/bash

/scratch/nvm/llvm-project/build/bin/clang++ -Xclang -load -Xclang /scratch/nvm/llvm-project/build/lib/libPMCPass.so -L/scratch/nvm/pmcheck/bin -lpmcheck -Wno-unused-command-line-argument -Wno-address-of-packed-member -Wno-mismatched-tags -Wno-unused-private-field -Wno-constant-conversion -Wno-null-dereference -fheinous-gnu-extensions -O0 $@
