This is a *very rough* proof of concept which allows for compiling / running C code within a web browser. An x86 emulator is used to run TCC ([Tiny C Compiler](https://bellard.org/tcc/)) in a web browser. The resulting exe can then be emulated in the same way.

Try it out here https://pixeltris.github.io/webc86

TODO:
- Create a WebAssembly version by rewriting the emulator in C and compiling with emscripten. This should hopefully help out with performance (which currently sucks) and fix the various issues related to using JavaScript.
