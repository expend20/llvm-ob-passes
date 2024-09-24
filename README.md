# dummy test

```
cd dummy-test
clang -S -emit-llvm test.c -o test.ll
opt -load-pass-plugin=../build/libLLVMExamplePass.so -passes="example-pass" test.ll -o test.opt.ll -debug-pass-manager
```