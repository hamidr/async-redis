# async-redis
An async redis library with sentinel support based on (libevpp|ASIO).

- LibEVpp is a simple c++1y interface for making it easier to adapt to higher level libraries such as "Asio".
- Supports BootAsio in a branch called `adapt-asio` or you can simply use it like as it is.
- Also introduces a sentinel interface example (which needs cleaning and more testing)
- Has it's own redis command parsing library.

## Dependencies
- LibEvPP: It's a library written by the same author. It's A library based on libEv.

### Road map
- Write more examples.
- Write a sentinel interface(adapt the example).
- Write BDD tests.


## INSTALL
```
bash ./build.sh
```
You can use event_loop and parser as link library for your project and pass `/usr/local/include` as include directory.


## State of library
Need a pal to review and revise my codes.

Contributions are welcome!
