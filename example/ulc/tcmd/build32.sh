clang -O2 -I ../../../h -target bpf -c *.c  -D IN_ULC_USER --target=armv7a-linux-gnueabihf -emit-llvm
llc -march=bpf -filetype=obj *.bc

