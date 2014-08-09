# hxcpp

[![Build Status](https://travis-ci.org/HaxeFoundation/hxcpp.png?branch=master)](https://travis-ci.org/HaxeFoundation/hxcpp)

hxcpp is the runtime support for the c++ backend of the [haxe](http://haxe.org/) compiler. This contains the headers, libraries and support code required to generate a fully compiled executable from haxe code.


# rebuilding

```
cd project
haxelib run hxcpp build.xml clean
haxelib run hxcpp build.xml
```

In the same folder, you can cross build to other platforms using the run.n with the said platform name.

For example : 

```
neko run.n android
```

For experts, you can configure the compilation scripts that will be used for executables and library production in the 'toolchain' folder.
