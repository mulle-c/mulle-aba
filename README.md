# mulle-aba

**mulle_aba** is a (pretty much) lock-free, cross-platform solution to the
[ABA problem](//en.wikipedia.org/wiki/ABA_problem) written in C.

The ABA problem appears, when you are freeing memory, that is shared by
multiple threads and is not protected by a lock. As the subject matter is
fairly complicated, please read the [Wikipedia article](//en.wikipedia.org/wiki/ABA_problem) and maybe [Preshing: An Introduction to Lock-Free Programming](http://preshing.com/20120612/an-introduction-to-lock-free-programming/) first and then checkout the following items:

* [mulle-aba: How it works 1](//www.mulle-kybernetik.com/weblog/2015/mulle_aba_how_it_works_1.html)
* [mulle-aba: How it works 2](//www.mulle-kybernetik.com/weblog/2015/mulle_aba_how_it_works_2.html)
* [mulle-aba: How it works 3](//www.mulle-kybernetik.com/weblog/2015/mulle_aba_how_it_works_3.html)
* [Example](example/main.m)


Fork      |  Build Status | Release Version
----------|---------------|-----------------------------------
[Mulle kybernetiK](//github.com/mulle-nat/mulle-aba) | [![Build Status](https://travis-ci.org/mulle-nat/mulle-aba.svg?branch=release)](https://travis-ci.org/mulle-nat/mulle-aba) | ![Mulle kybernetiK tag](https://img.shields.io/github/tag/mulle-nat/mulle-aba.svg) [![Build Status](https://travis-ci.org/mulle-nat/mulle-aba.svg?branch=release)](https://travis-ci.org/mulle-nat/mulle-aba)
[Community](https://github.com/mulle-objc/mulle-aba/tree/release) | [![Build Status](https://travis-ci.org/mulle-objc/mulle-aba.svg)](https://travis-ci.org/mulle-objc/mulle-aba) | ![Community tag](https://img.shields.io/github/tag/mulle-objc/mulle-aba.svg) [![Build Status](https://travis-ci.org/mulle-objc/mulle-aba.svg?branch=release)](https://travis-ci.org/mulle-objc/mulle-aba)


## API

* [Aba](dox/API_ABA.md)


## Install

On OS X and Linux you can use
[homebrew](//brew.sh), respectively
[linuxbrew](//linuxbrew.sh)
to install the library:

```
brew tap mulle-kybernetik/software
brew install mulle-aba
```

On other platforms you can use **mulle-install** from
[mulle-build](//www.mulle-kybernetik.com/software/git/mulle-build)
to install the library:

```
mulle-install --prefix /usr/local --branch release https://github.com/mulle-objc/mulle-aba
```


Otherwise read:

* [How to Build](dox/BUILD.md)


## Author

[Nat!](//www.mulle-kybernetik.com/weblog) for
[Mulle kybernetiK](//www.mulle-kybernetik.com) and
[Codeon GmbH](//www.codeon.de)

