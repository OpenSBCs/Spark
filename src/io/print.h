#ifndef PRINT_H
#define PRINT_H

#define printg(x) _Generic((x), \
    const char*: writeOut, \
    char*: writeOut, \
    short: writeOutNum, \
    int: writeOutNum, \
    long: writeOutNum, \
    unsigned short: writeOutNum, \
    unsigned int: writeOutNum, \
    unsigned long: writeOutNum, \
    default: writeOut \
)(x)

#define EXPAND(x) x
#define __FE1(what, x) what(x)
#define __FE2(what, x, ...) what(x); EXPAND(__FE1(what, __VA_ARGS__))
#define __FE3(what, x, ...) what(x); EXPAND(__FE2(what, __VA_ARGS__))
#define __FE4(what, x, ...) what(x); EXPAND(__FE3(what, __VA_ARGS__))
#define __FE5(what, x, ...) what(x); EXPAND(__FE4(what, __VA_ARGS__))
#define __FE6(what, x, ...) what(x); EXPAND(__FE5(what, __VA_ARGS__))

#define __FE(_1,_2,_3,_4,_5,_6,NAME,...) NAME
#define print(...) EXPAND(__FE(__VA_ARGS__, __FE6, __FE5, __FE4, __FE3, __FE2, __FE1)(printg, __VA_ARGS__))

#endif