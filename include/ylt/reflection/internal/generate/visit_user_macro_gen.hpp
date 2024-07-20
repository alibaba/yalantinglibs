/*The following boilerplate code was generated using a Python script:
macro = "#define YLT_CALL_ARGS"
with open("generated_visit_code.txt", "w", encoding="utf-8") as codefile:
    codefile.write(
        "\n".join(
            [
                f"{macro}{i-1}(f,o{''.join([f',_{j}' for j in range(1,i)])})
f({', '.join([f'CONCAT_MEMBER(o,_{j})' for j in range(1,i)])})" for i in
range(1, 127)
            ]
        )
    )
*/
#define WRAP_ARGS0(w, o)
#define WRAP_ARGS1(w, o, _1) w(o, _1)
#define WRAP_ARGS2(w, o, _1, ...) w(o, _1), WRAP_ARGS1(w, o, __VA_ARGS__)
#define WRAP_ARGS3(w, o, _1, ...) w(o, _1), WRAP_ARGS2(w, o, __VA_ARGS__)
#define WRAP_ARGS4(w, o, _1, ...) w(o, _1), WRAP_ARGS3(w, o, __VA_ARGS__)
#define WRAP_ARGS5(w, o, _1, ...) w(o, _1), WRAP_ARGS4(w, o, __VA_ARGS__)
#define WRAP_ARGS6(w, o, _1, ...) w(o, _1), WRAP_ARGS5(w, o, __VA_ARGS__)

#define YLT_CALL_ARGS0(f, o) f()
#define YLT_CALL_ARGS1(f, o, _1) f(CONCAT_MEMBER(o, _1))
#define YLT_CALL_ARGS2(f, o, _1, _2) \
  f(CONCAT_MEMBER(o, _1), CONCAT_MEMBER(o, _2))
#define YLT_CALL_ARGS3(f, o, _1, _2, _3) \
  f(CONCAT_MEMBER(o, _1), CONCAT_MEMBER(o, _2), CONCAT_MEMBER(o, _3))
#define YLT_CALL_ARGS4(f, o, _1, _2, _3, _4)                          \
  f(CONCAT_MEMBER(o, _1), CONCAT_MEMBER(o, _2), CONCAT_MEMBER(o, _3), \
    CONCAT_MEMBER(o, _4))
#define YLT_CALL_ARGS5(f, o, _1, _2, _3, _4, _5)                      \
  f(CONCAT_MEMBER(o, _1), CONCAT_MEMBER(o, _2), CONCAT_MEMBER(o, _3), \
    CONCAT_MEMBER(o, _4), CONCAT_MEMBER(o, _5))
#define YLT_CALL_ARGS6(f, o, _1, _2, _3, _4, _5, _6)                  \
  f(CONCAT_MEMBER(o, _1), CONCAT_MEMBER(o, _2), CONCAT_MEMBER(o, _3), \
    CONCAT_MEMBER(o, _4), CONCAT_MEMBER(o, _5), CONCAT_MEMBER(o, _6))
#define YLT_CALL_ARGS7(f, o, _1, _2, _3, _4, _5, _6, _7)              \
  f(CONCAT_MEMBER(o, _1), CONCAT_MEMBER(o, _2), CONCAT_MEMBER(o, _3), \
    CONCAT_MEMBER(o, _4), CONCAT_MEMBER(o, _5), CONCAT_MEMBER(o, _6), \
    CONCAT_MEMBER(o, _7))
#define YLT_CALL_ARGS8(f, o, _1, _2, _3, _4, _5, _6, _7, _8)          \
  f(CONCAT_MEMBER(o, _1), CONCAT_MEMBER(o, _2), CONCAT_MEMBER(o, _3), \
    CONCAT_MEMBER(o, _4), CONCAT_MEMBER(o, _5), CONCAT_MEMBER(o, _6), \
    CONCAT_MEMBER(o, _7), CONCAT_MEMBER(o, _8))
#define YLT_CALL_ARGS9(f, o, _1, _2, _3, _4, _5, _6, _7, _8, _9)      \
  f(CONCAT_MEMBER(o, _1), CONCAT_MEMBER(o, _2), CONCAT_MEMBER(o, _3), \
    CONCAT_MEMBER(o, _4), CONCAT_MEMBER(o, _5), CONCAT_MEMBER(o, _6), \
    CONCAT_MEMBER(o, _7), CONCAT_MEMBER(o, _8), CONCAT_MEMBER(o, _9))
#define YLT_CALL_ARGS10(f, o, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10) \
  f(CONCAT_MEMBER(o, _1), CONCAT_MEMBER(o, _2), CONCAT_MEMBER(o, _3),  \
    CONCAT_MEMBER(o, _4), CONCAT_MEMBER(o, _5), CONCAT_MEMBER(o, _6),  \
    CONCAT_MEMBER(o, _7), CONCAT_MEMBER(o, _8), CONCAT_MEMBER(o, _9),  \
    CONCAT_MEMBER(o, _10))
#define YLT_CALL_ARGS11(f, o, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11) \
  f(CONCAT_MEMBER(o, _1), CONCAT_MEMBER(o, _2), CONCAT_MEMBER(o, _3),       \
    CONCAT_MEMBER(o, _4), CONCAT_MEMBER(o, _5), CONCAT_MEMBER(o, _6),       \
    CONCAT_MEMBER(o, _7), CONCAT_MEMBER(o, _8), CONCAT_MEMBER(o, _9),       \
    CONCAT_MEMBER(o, _10), CONCAT_MEMBER(o, _11))
#define YLT_CALL_ARGS12(f, o, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, \
                        _12)                                                \
  f(CONCAT_MEMBER(o, _1), CONCAT_MEMBER(o, _2), CONCAT_MEMBER(o, _3),       \
    CONCAT_MEMBER(o, _4), CONCAT_MEMBER(o, _5), CONCAT_MEMBER(o, _6),       \
    CONCAT_MEMBER(o, _7), CONCAT_MEMBER(o, _8), CONCAT_MEMBER(o, _9),       \
    CONCAT_MEMBER(o, _10), CONCAT_MEMBER(o, _11), CONCAT_MEMBER(o, _12))
#define YLT_CALL_ARGS13(f, o, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, \
                        _12, _13)                                           \
  f(CONCAT_MEMBER(o, _1), CONCAT_MEMBER(o, _2), CONCAT_MEMBER(o, _3),       \
    CONCAT_MEMBER(o, _4), CONCAT_MEMBER(o, _5), CONCAT_MEMBER(o, _6),       \
    CONCAT_MEMBER(o, _7), CONCAT_MEMBER(o, _8), CONCAT_MEMBER(o, _9),       \
    CONCAT_MEMBER(o, _10), CONCAT_MEMBER(o, _11), CONCAT_MEMBER(o, _12),    \
    CONCAT_MEMBER(o, _13))
#define YLT_CALL_ARGS14(f, o, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, \
                        _12, _13, _14)                                      \
  f(CONCAT_MEMBER(o, _1), CONCAT_MEMBER(o, _2), CONCAT_MEMBER(o, _3),       \
    CONCAT_MEMBER(o, _4), CONCAT_MEMBER(o, _5), CONCAT_MEMBER(o, _6),       \
    CONCAT_MEMBER(o, _7), CONCAT_MEMBER(o, _8), CONCAT_MEMBER(o, _9),       \
    CONCAT_MEMBER(o, _10), CONCAT_MEMBER(o, _11), CONCAT_MEMBER(o, _12),    \
    CONCAT_MEMBER(o, _13), CONCAT_MEMBER(o, _14))
#define YLT_CALL_ARGS15(f, o, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, \
                        _12, _13, _14, _15)                                 \
  f(CONCAT_MEMBER(o, _1), CONCAT_MEMBER(o, _2), CONCAT_MEMBER(o, _3),       \
    CONCAT_MEMBER(o, _4), CONCAT_MEMBER(o, _5), CONCAT_MEMBER(o, _6),       \
    CONCAT_MEMBER(o, _7), CONCAT_MEMBER(o, _8), CONCAT_MEMBER(o, _9),       \
    CONCAT_MEMBER(o, _10), CONCAT_MEMBER(o, _11), CONCAT_MEMBER(o, _12),    \
    CONCAT_MEMBER(o, _13), CONCAT_MEMBER(o, _14), CONCAT_MEMBER(o, _15))
#define YLT_CALL_ARGS16(f, o, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, \
                        _12, _13, _14, _15, _16)                            \
  f(CONCAT_MEMBER(o, _1), CONCAT_MEMBER(o, _2), CONCAT_MEMBER(o, _3),       \
    CONCAT_MEMBER(o, _4), CONCAT_MEMBER(o, _5), CONCAT_MEMBER(o, _6),       \
    CONCAT_MEMBER(o, _7), CONCAT_MEMBER(o, _8), CONCAT_MEMBER(o, _9),       \
    CONCAT_MEMBER(o, _10), CONCAT_MEMBER(o, _11), CONCAT_MEMBER(o, _12),    \
    CONCAT_MEMBER(o, _13), CONCAT_MEMBER(o, _14), CONCAT_MEMBER(o, _15),    \
    CONCAT_MEMBER(o, _16))
#define YLT_CALL_ARGS17(f, o, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, \
                        _12, _13, _14, _15, _16, _17)                       \
  f(CONCAT_MEMBER(o, _1), CONCAT_MEMBER(o, _2), CONCAT_MEMBER(o, _3),       \
    CONCAT_MEMBER(o, _4), CONCAT_MEMBER(o, _5), CONCAT_MEMBER(o, _6),       \
    CONCAT_MEMBER(o, _7), CONCAT_MEMBER(o, _8), CONCAT_MEMBER(o, _9),       \
    CONCAT_MEMBER(o, _10), CONCAT_MEMBER(o, _11), CONCAT_MEMBER(o, _12),    \
    CONCAT_MEMBER(o, _13), CONCAT_MEMBER(o, _14), CONCAT_MEMBER(o, _15),    \
    CONCAT_MEMBER(o, _16), CONCAT_MEMBER(o, _17))
#define YLT_CALL_ARGS18(f, o, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, \
                        _12, _13, _14, _15, _16, _17, _18)                  \
  f(CONCAT_MEMBER(o, _1), CONCAT_MEMBER(o, _2), CONCAT_MEMBER(o, _3),       \
    CONCAT_MEMBER(o, _4), CONCAT_MEMBER(o, _5), CONCAT_MEMBER(o, _6),       \
    CONCAT_MEMBER(o, _7), CONCAT_MEMBER(o, _8), CONCAT_MEMBER(o, _9),       \
    CONCAT_MEMBER(o, _10), CONCAT_MEMBER(o, _11), CONCAT_MEMBER(o, _12),    \
    CONCAT_MEMBER(o, _13), CONCAT_MEMBER(o, _14), CONCAT_MEMBER(o, _15),    \
    CONCAT_MEMBER(o, _16), CONCAT_MEMBER(o, _17), CONCAT_MEMBER(o, _18))
#define YLT_CALL_ARGS19(f, o, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, \
                        _12, _13, _14, _15, _16, _17, _18, _19)             \
  f(CONCAT_MEMBER(o, _1), CONCAT_MEMBER(o, _2), CONCAT_MEMBER(o, _3),       \
    CONCAT_MEMBER(o, _4), CONCAT_MEMBER(o, _5), CONCAT_MEMBER(o, _6),       \
    CONCAT_MEMBER(o, _7), CONCAT_MEMBER(o, _8), CONCAT_MEMBER(o, _9),       \
    CONCAT_MEMBER(o, _10), CONCAT_MEMBER(o, _11), CONCAT_MEMBER(o, _12),    \
    CONCAT_MEMBER(o, _13), CONCAT_MEMBER(o, _14), CONCAT_MEMBER(o, _15),    \
    CONCAT_MEMBER(o, _16), CONCAT_MEMBER(o, _17), CONCAT_MEMBER(o, _18),    \
    CONCAT_MEMBER(o, _19))
#define YLT_CALL_ARGS20(f, o, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, \
                        _12, _13, _14, _15, _16, _17, _18, _19, _20)        \
  f(CONCAT_MEMBER(o, _1), CONCAT_MEMBER(o, _2), CONCAT_MEMBER(o, _3),       \
    CONCAT_MEMBER(o, _4), CONCAT_MEMBER(o, _5), CONCAT_MEMBER(o, _6),       \
    CONCAT_MEMBER(o, _7), CONCAT_MEMBER(o, _8), CONCAT_MEMBER(o, _9),       \
    CONCAT_MEMBER(o, _10), CONCAT_MEMBER(o, _11), CONCAT_MEMBER(o, _12),    \
    CONCAT_MEMBER(o, _13), CONCAT_MEMBER(o, _14), CONCAT_MEMBER(o, _15),    \
    CONCAT_MEMBER(o, _16), CONCAT_MEMBER(o, _17), CONCAT_MEMBER(o, _18),    \
    CONCAT_MEMBER(o, _19), CONCAT_MEMBER(o, _20))
#define YLT_CALL_ARGS21(f, o, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, \
                        _12, _13, _14, _15, _16, _17, _18, _19, _20, _21)   \
  f(CONCAT_MEMBER(o, _1), CONCAT_MEMBER(o, _2), CONCAT_MEMBER(o, _3),       \
    CONCAT_MEMBER(o, _4), CONCAT_MEMBER(o, _5), CONCAT_MEMBER(o, _6),       \
    CONCAT_MEMBER(o, _7), CONCAT_MEMBER(o, _8), CONCAT_MEMBER(o, _9),       \
    CONCAT_MEMBER(o, _10), CONCAT_MEMBER(o, _11), CONCAT_MEMBER(o, _12),    \
    CONCAT_MEMBER(o, _13), CONCAT_MEMBER(o, _14), CONCAT_MEMBER(o, _15),    \
    CONCAT_MEMBER(o, _16), CONCAT_MEMBER(o, _17), CONCAT_MEMBER(o, _18),    \
    CONCAT_MEMBER(o, _19), CONCAT_MEMBER(o, _20), CONCAT_MEMBER(o, _21))
#define YLT_CALL_ARGS22(f, o, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11,    \
                        _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22) \
  f(CONCAT_MEMBER(o, _1), CONCAT_MEMBER(o, _2), CONCAT_MEMBER(o, _3),          \
    CONCAT_MEMBER(o, _4), CONCAT_MEMBER(o, _5), CONCAT_MEMBER(o, _6),          \
    CONCAT_MEMBER(o, _7), CONCAT_MEMBER(o, _8), CONCAT_MEMBER(o, _9),          \
    CONCAT_MEMBER(o, _10), CONCAT_MEMBER(o, _11), CONCAT_MEMBER(o, _12),       \
    CONCAT_MEMBER(o, _13), CONCAT_MEMBER(o, _14), CONCAT_MEMBER(o, _15),       \
    CONCAT_MEMBER(o, _16), CONCAT_MEMBER(o, _17), CONCAT_MEMBER(o, _18),       \
    CONCAT_MEMBER(o, _19), CONCAT_MEMBER(o, _20), CONCAT_MEMBER(o, _21),       \
    CONCAT_MEMBER(o, _22))
#define YLT_CALL_ARGS23(f, o, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11,    \
                        _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, \
                        _23)                                                   \
  f(CONCAT_MEMBER(o, _1), CONCAT_MEMBER(o, _2), CONCAT_MEMBER(o, _3),          \
    CONCAT_MEMBER(o, _4), CONCAT_MEMBER(o, _5), CONCAT_MEMBER(o, _6),          \
    CONCAT_MEMBER(o, _7), CONCAT_MEMBER(o, _8), CONCAT_MEMBER(o, _9),          \
    CONCAT_MEMBER(o, _10), CONCAT_MEMBER(o, _11), CONCAT_MEMBER(o, _12),       \
    CONCAT_MEMBER(o, _13), CONCAT_MEMBER(o, _14), CONCAT_MEMBER(o, _15),       \
    CONCAT_MEMBER(o, _16), CONCAT_MEMBER(o, _17), CONCAT_MEMBER(o, _18),       \
    CONCAT_MEMBER(o, _19), CONCAT_MEMBER(o, _20), CONCAT_MEMBER(o, _21),       \
    CONCAT_MEMBER(o, _22), CONCAT_MEMBER(o, _23))
#define YLT_CALL_ARGS24(f, o, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11,    \
                        _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, \
                        _23, _24)                                              \
  f(CONCAT_MEMBER(o, _1), CONCAT_MEMBER(o, _2), CONCAT_MEMBER(o, _3),          \
    CONCAT_MEMBER(o, _4), CONCAT_MEMBER(o, _5), CONCAT_MEMBER(o, _6),          \
    CONCAT_MEMBER(o, _7), CONCAT_MEMBER(o, _8), CONCAT_MEMBER(o, _9),          \
    CONCAT_MEMBER(o, _10), CONCAT_MEMBER(o, _11), CONCAT_MEMBER(o, _12),       \
    CONCAT_MEMBER(o, _13), CONCAT_MEMBER(o, _14), CONCAT_MEMBER(o, _15),       \
    CONCAT_MEMBER(o, _16), CONCAT_MEMBER(o, _17), CONCAT_MEMBER(o, _18),       \
    CONCAT_MEMBER(o, _19), CONCAT_MEMBER(o, _20), CONCAT_MEMBER(o, _21),       \
    CONCAT_MEMBER(o, _22), CONCAT_MEMBER(o, _23), CONCAT_MEMBER(o, _24))
#define YLT_CALL_ARGS25(f, o, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11,    \
                        _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, \
                        _23, _24, _25)                                         \
  f(CONCAT_MEMBER(o, _1), CONCAT_MEMBER(o, _2), CONCAT_MEMBER(o, _3),          \
    CONCAT_MEMBER(o, _4), CONCAT_MEMBER(o, _5), CONCAT_MEMBER(o, _6),          \
    CONCAT_MEMBER(o, _7), CONCAT_MEMBER(o, _8), CONCAT_MEMBER(o, _9),          \
    CONCAT_MEMBER(o, _10), CONCAT_MEMBER(o, _11), CONCAT_MEMBER(o, _12),       \
    CONCAT_MEMBER(o, _13), CONCAT_MEMBER(o, _14), CONCAT_MEMBER(o, _15),       \
    CONCAT_MEMBER(o, _16), CONCAT_MEMBER(o, _17), CONCAT_MEMBER(o, _18),       \
    CONCAT_MEMBER(o, _19), CONCAT_MEMBER(o, _20), CONCAT_MEMBER(o, _21),       \
    CONCAT_MEMBER(o, _22), CONCAT_MEMBER(o, _23), CONCAT_MEMBER(o, _24),       \
    CONCAT_MEMBER(o, _25))
#define YLT_CALL_ARGS26(f, o, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11,    \
                        _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, \
                        _23, _24, _25, _26)                                    \
  f(CONCAT_MEMBER(o, _1), CONCAT_MEMBER(o, _2), CONCAT_MEMBER(o, _3),          \
    CONCAT_MEMBER(o, _4), CONCAT_MEMBER(o, _5), CONCAT_MEMBER(o, _6),          \
    CONCAT_MEMBER(o, _7), CONCAT_MEMBER(o, _8), CONCAT_MEMBER(o, _9),          \
    CONCAT_MEMBER(o, _10), CONCAT_MEMBER(o, _11), CONCAT_MEMBER(o, _12),       \
    CONCAT_MEMBER(o, _13), CONCAT_MEMBER(o, _14), CONCAT_MEMBER(o, _15),       \
    CONCAT_MEMBER(o, _16), CONCAT_MEMBER(o, _17), CONCAT_MEMBER(o, _18),       \
    CONCAT_MEMBER(o, _19), CONCAT_MEMBER(o, _20), CONCAT_MEMBER(o, _21),       \
    CONCAT_MEMBER(o, _22), CONCAT_MEMBER(o, _23), CONCAT_MEMBER(o, _24),       \
    CONCAT_MEMBER(o, _25), CONCAT_MEMBER(o, _26))
#define YLT_CALL_ARGS27(f, o, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11,    \
                        _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, \
                        _23, _24, _25, _26, _27)                               \
  f(CONCAT_MEMBER(o, _1), CONCAT_MEMBER(o, _2), CONCAT_MEMBER(o, _3),          \
    CONCAT_MEMBER(o, _4), CONCAT_MEMBER(o, _5), CONCAT_MEMBER(o, _6),          \
    CONCAT_MEMBER(o, _7), CONCAT_MEMBER(o, _8), CONCAT_MEMBER(o, _9),          \
    CONCAT_MEMBER(o, _10), CONCAT_MEMBER(o, _11), CONCAT_MEMBER(o, _12),       \
    CONCAT_MEMBER(o, _13), CONCAT_MEMBER(o, _14), CONCAT_MEMBER(o, _15),       \
    CONCAT_MEMBER(o, _16), CONCAT_MEMBER(o, _17), CONCAT_MEMBER(o, _18),       \
    CONCAT_MEMBER(o, _19), CONCAT_MEMBER(o, _20), CONCAT_MEMBER(o, _21),       \
    CONCAT_MEMBER(o, _22), CONCAT_MEMBER(o, _23), CONCAT_MEMBER(o, _24),       \
    CONCAT_MEMBER(o, _25), CONCAT_MEMBER(o, _26), CONCAT_MEMBER(o, _27))
#define YLT_CALL_ARGS28(f, o, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11,    \
                        _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, \
                        _23, _24, _25, _26, _27, _28)                          \
  f(CONCAT_MEMBER(o, _1), CONCAT_MEMBER(o, _2), CONCAT_MEMBER(o, _3),          \
    CONCAT_MEMBER(o, _4), CONCAT_MEMBER(o, _5), CONCAT_MEMBER(o, _6),          \
    CONCAT_MEMBER(o, _7), CONCAT_MEMBER(o, _8), CONCAT_MEMBER(o, _9),          \
    CONCAT_MEMBER(o, _10), CONCAT_MEMBER(o, _11), CONCAT_MEMBER(o, _12),       \
    CONCAT_MEMBER(o, _13), CONCAT_MEMBER(o, _14), CONCAT_MEMBER(o, _15),       \
    CONCAT_MEMBER(o, _16), CONCAT_MEMBER(o, _17), CONCAT_MEMBER(o, _18),       \
    CONCAT_MEMBER(o, _19), CONCAT_MEMBER(o, _20), CONCAT_MEMBER(o, _21),       \
    CONCAT_MEMBER(o, _22), CONCAT_MEMBER(o, _23), CONCAT_MEMBER(o, _24),       \
    CONCAT_MEMBER(o, _25), CONCAT_MEMBER(o, _26), CONCAT_MEMBER(o, _27),       \
    CONCAT_MEMBER(o, _28))
#define YLT_CALL_ARGS29(f, o, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11,    \
                        _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, \
                        _23, _24, _25, _26, _27, _28, _29)                     \
  f(CONCAT_MEMBER(o, _1), CONCAT_MEMBER(o, _2), CONCAT_MEMBER(o, _3),          \
    CONCAT_MEMBER(o, _4), CONCAT_MEMBER(o, _5), CONCAT_MEMBER(o, _6),          \
    CONCAT_MEMBER(o, _7), CONCAT_MEMBER(o, _8), CONCAT_MEMBER(o, _9),          \
    CONCAT_MEMBER(o, _10), CONCAT_MEMBER(o, _11), CONCAT_MEMBER(o, _12),       \
    CONCAT_MEMBER(o, _13), CONCAT_MEMBER(o, _14), CONCAT_MEMBER(o, _15),       \
    CONCAT_MEMBER(o, _16), CONCAT_MEMBER(o, _17), CONCAT_MEMBER(o, _18),       \
    CONCAT_MEMBER(o, _19), CONCAT_MEMBER(o, _20), CONCAT_MEMBER(o, _21),       \
    CONCAT_MEMBER(o, _22), CONCAT_MEMBER(o, _23), CONCAT_MEMBER(o, _24),       \
    CONCAT_MEMBER(o, _25), CONCAT_MEMBER(o, _26), CONCAT_MEMBER(o, _27),       \
    CONCAT_MEMBER(o, _28), CONCAT_MEMBER(o, _29))
#define YLT_CALL_ARGS30(f, o, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11,    \
                        _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, \
                        _23, _24, _25, _26, _27, _28, _29, _30)                \
  f(CONCAT_MEMBER(o, _1), CONCAT_MEMBER(o, _2), CONCAT_MEMBER(o, _3),          \
    CONCAT_MEMBER(o, _4), CONCAT_MEMBER(o, _5), CONCAT_MEMBER(o, _6),          \
    CONCAT_MEMBER(o, _7), CONCAT_MEMBER(o, _8), CONCAT_MEMBER(o, _9),          \
    CONCAT_MEMBER(o, _10), CONCAT_MEMBER(o, _11), CONCAT_MEMBER(o, _12),       \
    CONCAT_MEMBER(o, _13), CONCAT_MEMBER(o, _14), CONCAT_MEMBER(o, _15),       \
    CONCAT_MEMBER(o, _16), CONCAT_MEMBER(o, _17), CONCAT_MEMBER(o, _18),       \
    CONCAT_MEMBER(o, _19), CONCAT_MEMBER(o, _20), CONCAT_MEMBER(o, _21),       \
    CONCAT_MEMBER(o, _22), CONCAT_MEMBER(o, _23), CONCAT_MEMBER(o, _24),       \
    CONCAT_MEMBER(o, _25), CONCAT_MEMBER(o, _26), CONCAT_MEMBER(o, _27),       \
    CONCAT_MEMBER(o, _28), CONCAT_MEMBER(o, _29), CONCAT_MEMBER(o, _30))
#define YLT_CALL_ARGS31(f, o, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11,    \
                        _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, \
                        _23, _24, _25, _26, _27, _28, _29, _30, _31)           \
  f(CONCAT_MEMBER(o, _1), CONCAT_MEMBER(o, _2), CONCAT_MEMBER(o, _3),          \
    CONCAT_MEMBER(o, _4), CONCAT_MEMBER(o, _5), CONCAT_MEMBER(o, _6),          \
    CONCAT_MEMBER(o, _7), CONCAT_MEMBER(o, _8), CONCAT_MEMBER(o, _9),          \
    CONCAT_MEMBER(o, _10), CONCAT_MEMBER(o, _11), CONCAT_MEMBER(o, _12),       \
    CONCAT_MEMBER(o, _13), CONCAT_MEMBER(o, _14), CONCAT_MEMBER(o, _15),       \
    CONCAT_MEMBER(o, _16), CONCAT_MEMBER(o, _17), CONCAT_MEMBER(o, _18),       \
    CONCAT_MEMBER(o, _19), CONCAT_MEMBER(o, _20), CONCAT_MEMBER(o, _21),       \
    CONCAT_MEMBER(o, _22), CONCAT_MEMBER(o, _23), CONCAT_MEMBER(o, _24),       \
    CONCAT_MEMBER(o, _25), CONCAT_MEMBER(o, _26), CONCAT_MEMBER(o, _27),       \
    CONCAT_MEMBER(o, _28), CONCAT_MEMBER(o, _29), CONCAT_MEMBER(o, _30),       \
    CONCAT_MEMBER(o, _31))
#define YLT_CALL_ARGS32(f, o, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11,    \
                        _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, \
                        _23, _24, _25, _26, _27, _28, _29, _30, _31, _32)      \
  f(CONCAT_MEMBER(o, _1), CONCAT_MEMBER(o, _2), CONCAT_MEMBER(o, _3),          \
    CONCAT_MEMBER(o, _4), CONCAT_MEMBER(o, _5), CONCAT_MEMBER(o, _6),          \
    CONCAT_MEMBER(o, _7), CONCAT_MEMBER(o, _8), CONCAT_MEMBER(o, _9),          \
    CONCAT_MEMBER(o, _10), CONCAT_MEMBER(o, _11), CONCAT_MEMBER(o, _12),       \
    CONCAT_MEMBER(o, _13), CONCAT_MEMBER(o, _14), CONCAT_MEMBER(o, _15),       \
    CONCAT_MEMBER(o, _16), CONCAT_MEMBER(o, _17), CONCAT_MEMBER(o, _18),       \
    CONCAT_MEMBER(o, _19), CONCAT_MEMBER(o, _20), CONCAT_MEMBER(o, _21),       \
    CONCAT_MEMBER(o, _22), CONCAT_MEMBER(o, _23), CONCAT_MEMBER(o, _24),       \
    CONCAT_MEMBER(o, _25), CONCAT_MEMBER(o, _26), CONCAT_MEMBER(o, _27),       \
    CONCAT_MEMBER(o, _28), CONCAT_MEMBER(o, _29), CONCAT_MEMBER(o, _30),       \
    CONCAT_MEMBER(o, _31), CONCAT_MEMBER(o, _32))
#define YLT_CALL_ARGS33(f, o, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11,    \
                        _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, \
                        _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33) \
  f(CONCAT_MEMBER(o, _1), CONCAT_MEMBER(o, _2), CONCAT_MEMBER(o, _3),          \
    CONCAT_MEMBER(o, _4), CONCAT_MEMBER(o, _5), CONCAT_MEMBER(o, _6),          \
    CONCAT_MEMBER(o, _7), CONCAT_MEMBER(o, _8), CONCAT_MEMBER(o, _9),          \
    CONCAT_MEMBER(o, _10), CONCAT_MEMBER(o, _11), CONCAT_MEMBER(o, _12),       \
    CONCAT_MEMBER(o, _13), CONCAT_MEMBER(o, _14), CONCAT_MEMBER(o, _15),       \
    CONCAT_MEMBER(o, _16), CONCAT_MEMBER(o, _17), CONCAT_MEMBER(o, _18),       \
    CONCAT_MEMBER(o, _19), CONCAT_MEMBER(o, _20), CONCAT_MEMBER(o, _21),       \
    CONCAT_MEMBER(o, _22), CONCAT_MEMBER(o, _23), CONCAT_MEMBER(o, _24),       \
    CONCAT_MEMBER(o, _25), CONCAT_MEMBER(o, _26), CONCAT_MEMBER(o, _27),       \
    CONCAT_MEMBER(o, _28), CONCAT_MEMBER(o, _29), CONCAT_MEMBER(o, _30),       \
    CONCAT_MEMBER(o, _31), CONCAT_MEMBER(o, _32), CONCAT_MEMBER(o, _33))
#define YLT_CALL_ARGS34(f, o, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11,    \
                        _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, \
                        _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, \
                        _34)                                                   \
  f(CONCAT_MEMBER(o, _1), CONCAT_MEMBER(o, _2), CONCAT_MEMBER(o, _3),          \
    CONCAT_MEMBER(o, _4), CONCAT_MEMBER(o, _5), CONCAT_MEMBER(o, _6),          \
    CONCAT_MEMBER(o, _7), CONCAT_MEMBER(o, _8), CONCAT_MEMBER(o, _9),          \
    CONCAT_MEMBER(o, _10), CONCAT_MEMBER(o, _11), CONCAT_MEMBER(o, _12),       \
    CONCAT_MEMBER(o, _13), CONCAT_MEMBER(o, _14), CONCAT_MEMBER(o, _15),       \
    CONCAT_MEMBER(o, _16), CONCAT_MEMBER(o, _17), CONCAT_MEMBER(o, _18),       \
    CONCAT_MEMBER(o, _19), CONCAT_MEMBER(o, _20), CONCAT_MEMBER(o, _21),       \
    CONCAT_MEMBER(o, _22), CONCAT_MEMBER(o, _23), CONCAT_MEMBER(o, _24),       \
    CONCAT_MEMBER(o, _25), CONCAT_MEMBER(o, _26), CONCAT_MEMBER(o, _27),       \
    CONCAT_MEMBER(o, _28), CONCAT_MEMBER(o, _29), CONCAT_MEMBER(o, _30),       \
    CONCAT_MEMBER(o, _31), CONCAT_MEMBER(o, _32), CONCAT_MEMBER(o, _33),       \
    CONCAT_MEMBER(o, _34))
#define YLT_CALL_ARGS35(f, o, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11,    \
                        _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, \
                        _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, \
                        _34, _35)                                              \
  f(CONCAT_MEMBER(o, _1), CONCAT_MEMBER(o, _2), CONCAT_MEMBER(o, _3),          \
    CONCAT_MEMBER(o, _4), CONCAT_MEMBER(o, _5), CONCAT_MEMBER(o, _6),          \
    CONCAT_MEMBER(o, _7), CONCAT_MEMBER(o, _8), CONCAT_MEMBER(o, _9),          \
    CONCAT_MEMBER(o, _10), CONCAT_MEMBER(o, _11), CONCAT_MEMBER(o, _12),       \
    CONCAT_MEMBER(o, _13), CONCAT_MEMBER(o, _14), CONCAT_MEMBER(o, _15),       \
    CONCAT_MEMBER(o, _16), CONCAT_MEMBER(o, _17), CONCAT_MEMBER(o, _18),       \
    CONCAT_MEMBER(o, _19), CONCAT_MEMBER(o, _20), CONCAT_MEMBER(o, _21),       \
    CONCAT_MEMBER(o, _22), CONCAT_MEMBER(o, _23), CONCAT_MEMBER(o, _24),       \
    CONCAT_MEMBER(o, _25), CONCAT_MEMBER(o, _26), CONCAT_MEMBER(o, _27),       \
    CONCAT_MEMBER(o, _28), CONCAT_MEMBER(o, _29), CONCAT_MEMBER(o, _30),       \
    CONCAT_MEMBER(o, _31), CONCAT_MEMBER(o, _32), CONCAT_MEMBER(o, _33),       \
    CONCAT_MEMBER(o, _34), CONCAT_MEMBER(o, _35))
#define YLT_CALL_ARGS36(f, o, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11,    \
                        _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, \
                        _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, \
                        _34, _35, _36)                                         \
  f(CONCAT_MEMBER(o, _1), CONCAT_MEMBER(o, _2), CONCAT_MEMBER(o, _3),          \
    CONCAT_MEMBER(o, _4), CONCAT_MEMBER(o, _5), CONCAT_MEMBER(o, _6),          \
    CONCAT_MEMBER(o, _7), CONCAT_MEMBER(o, _8), CONCAT_MEMBER(o, _9),          \
    CONCAT_MEMBER(o, _10), CONCAT_MEMBER(o, _11), CONCAT_MEMBER(o, _12),       \
    CONCAT_MEMBER(o, _13), CONCAT_MEMBER(o, _14), CONCAT_MEMBER(o, _15),       \
    CONCAT_MEMBER(o, _16), CONCAT_MEMBER(o, _17), CONCAT_MEMBER(o, _18),       \
    CONCAT_MEMBER(o, _19), CONCAT_MEMBER(o, _20), CONCAT_MEMBER(o, _21),       \
    CONCAT_MEMBER(o, _22), CONCAT_MEMBER(o, _23), CONCAT_MEMBER(o, _24),       \
    CONCAT_MEMBER(o, _25), CONCAT_MEMBER(o, _26), CONCAT_MEMBER(o, _27),       \
    CONCAT_MEMBER(o, _28), CONCAT_MEMBER(o, _29), CONCAT_MEMBER(o, _30),       \
    CONCAT_MEMBER(o, _31), CONCAT_MEMBER(o, _32), CONCAT_MEMBER(o, _33),       \
    CONCAT_MEMBER(o, _34), CONCAT_MEMBER(o, _35), CONCAT_MEMBER(o, _36))
#define YLT_CALL_ARGS37(f, o, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11,    \
                        _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, \
                        _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, \
                        _34, _35, _36, _37)                                    \
  f(CONCAT_MEMBER(o, _1), CONCAT_MEMBER(o, _2), CONCAT_MEMBER(o, _3),          \
    CONCAT_MEMBER(o, _4), CONCAT_MEMBER(o, _5), CONCAT_MEMBER(o, _6),          \
    CONCAT_MEMBER(o, _7), CONCAT_MEMBER(o, _8), CONCAT_MEMBER(o, _9),          \
    CONCAT_MEMBER(o, _10), CONCAT_MEMBER(o, _11), CONCAT_MEMBER(o, _12),       \
    CONCAT_MEMBER(o, _13), CONCAT_MEMBER(o, _14), CONCAT_MEMBER(o, _15),       \
    CONCAT_MEMBER(o, _16), CONCAT_MEMBER(o, _17), CONCAT_MEMBER(o, _18),       \
    CONCAT_MEMBER(o, _19), CONCAT_MEMBER(o, _20), CONCAT_MEMBER(o, _21),       \
    CONCAT_MEMBER(o, _22), CONCAT_MEMBER(o, _23), CONCAT_MEMBER(o, _24),       \
    CONCAT_MEMBER(o, _25), CONCAT_MEMBER(o, _26), CONCAT_MEMBER(o, _27),       \
    CONCAT_MEMBER(o, _28), CONCAT_MEMBER(o, _29), CONCAT_MEMBER(o, _30),       \
    CONCAT_MEMBER(o, _31), CONCAT_MEMBER(o, _32), CONCAT_MEMBER(o, _33),       \
    CONCAT_MEMBER(o, _34), CONCAT_MEMBER(o, _35), CONCAT_MEMBER(o, _36),       \
    CONCAT_MEMBER(o, _37))
#define YLT_CALL_ARGS38(f, o, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11,    \
                        _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, \
                        _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, \
                        _34, _35, _36, _37, _38)                               \
  f(CONCAT_MEMBER(o, _1), CONCAT_MEMBER(o, _2), CONCAT_MEMBER(o, _3),          \
    CONCAT_MEMBER(o, _4), CONCAT_MEMBER(o, _5), CONCAT_MEMBER(o, _6),          \
    CONCAT_MEMBER(o, _7), CONCAT_MEMBER(o, _8), CONCAT_MEMBER(o, _9),          \
    CONCAT_MEMBER(o, _10), CONCAT_MEMBER(o, _11), CONCAT_MEMBER(o, _12),       \
    CONCAT_MEMBER(o, _13), CONCAT_MEMBER(o, _14), CONCAT_MEMBER(o, _15),       \
    CONCAT_MEMBER(o, _16), CONCAT_MEMBER(o, _17), CONCAT_MEMBER(o, _18),       \
    CONCAT_MEMBER(o, _19), CONCAT_MEMBER(o, _20), CONCAT_MEMBER(o, _21),       \
    CONCAT_MEMBER(o, _22), CONCAT_MEMBER(o, _23), CONCAT_MEMBER(o, _24),       \
    CONCAT_MEMBER(o, _25), CONCAT_MEMBER(o, _26), CONCAT_MEMBER(o, _27),       \
    CONCAT_MEMBER(o, _28), CONCAT_MEMBER(o, _29), CONCAT_MEMBER(o, _30),       \
    CONCAT_MEMBER(o, _31), CONCAT_MEMBER(o, _32), CONCAT_MEMBER(o, _33),       \
    CONCAT_MEMBER(o, _34), CONCAT_MEMBER(o, _35), CONCAT_MEMBER(o, _36),       \
    CONCAT_MEMBER(o, _37), CONCAT_MEMBER(o, _38))
#define YLT_CALL_ARGS39(f, o, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11,    \
                        _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, \
                        _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, \
                        _34, _35, _36, _37, _38, _39)                          \
  f(CONCAT_MEMBER(o, _1), CONCAT_MEMBER(o, _2), CONCAT_MEMBER(o, _3),          \
    CONCAT_MEMBER(o, _4), CONCAT_MEMBER(o, _5), CONCAT_MEMBER(o, _6),          \
    CONCAT_MEMBER(o, _7), CONCAT_MEMBER(o, _8), CONCAT_MEMBER(o, _9),          \
    CONCAT_MEMBER(o, _10), CONCAT_MEMBER(o, _11), CONCAT_MEMBER(o, _12),       \
    CONCAT_MEMBER(o, _13), CONCAT_MEMBER(o, _14), CONCAT_MEMBER(o, _15),       \
    CONCAT_MEMBER(o, _16), CONCAT_MEMBER(o, _17), CONCAT_MEMBER(o, _18),       \
    CONCAT_MEMBER(o, _19), CONCAT_MEMBER(o, _20), CONCAT_MEMBER(o, _21),       \
    CONCAT_MEMBER(o, _22), CONCAT_MEMBER(o, _23), CONCAT_MEMBER(o, _24),       \
    CONCAT_MEMBER(o, _25), CONCAT_MEMBER(o, _26), CONCAT_MEMBER(o, _27),       \
    CONCAT_MEMBER(o, _28), CONCAT_MEMBER(o, _29), CONCAT_MEMBER(o, _30),       \
    CONCAT_MEMBER(o, _31), CONCAT_MEMBER(o, _32), CONCAT_MEMBER(o, _33),       \
    CONCAT_MEMBER(o, _34), CONCAT_MEMBER(o, _35), CONCAT_MEMBER(o, _36),       \
    CONCAT_MEMBER(o, _37), CONCAT_MEMBER(o, _38), CONCAT_MEMBER(o, _39))
#define YLT_CALL_ARGS40(f, o, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11,    \
                        _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, \
                        _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, \
                        _34, _35, _36, _37, _38, _39, _40)                     \
  f(CONCAT_MEMBER(o, _1), CONCAT_MEMBER(o, _2), CONCAT_MEMBER(o, _3),          \
    CONCAT_MEMBER(o, _4), CONCAT_MEMBER(o, _5), CONCAT_MEMBER(o, _6),          \
    CONCAT_MEMBER(o, _7), CONCAT_MEMBER(o, _8), CONCAT_MEMBER(o, _9),          \
    CONCAT_MEMBER(o, _10), CONCAT_MEMBER(o, _11), CONCAT_MEMBER(o, _12),       \
    CONCAT_MEMBER(o, _13), CONCAT_MEMBER(o, _14), CONCAT_MEMBER(o, _15),       \
    CONCAT_MEMBER(o, _16), CONCAT_MEMBER(o, _17), CONCAT_MEMBER(o, _18),       \
    CONCAT_MEMBER(o, _19), CONCAT_MEMBER(o, _20), CONCAT_MEMBER(o, _21),       \
    CONCAT_MEMBER(o, _22), CONCAT_MEMBER(o, _23), CONCAT_MEMBER(o, _24),       \
    CONCAT_MEMBER(o, _25), CONCAT_MEMBER(o, _26), CONCAT_MEMBER(o, _27),       \
    CONCAT_MEMBER(o, _28), CONCAT_MEMBER(o, _29), CONCAT_MEMBER(o, _30),       \
    CONCAT_MEMBER(o, _31), CONCAT_MEMBER(o, _32), CONCAT_MEMBER(o, _33),       \
    CONCAT_MEMBER(o, _34), CONCAT_MEMBER(o, _35), CONCAT_MEMBER(o, _36),       \
    CONCAT_MEMBER(o, _37), CONCAT_MEMBER(o, _38), CONCAT_MEMBER(o, _39),       \
    CONCAT_MEMBER(o, _40))
#define YLT_CALL_ARGS41(f, o, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11,    \
                        _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, \
                        _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, \
                        _34, _35, _36, _37, _38, _39, _40, _41)                \
  f(CONCAT_MEMBER(o, _1), CONCAT_MEMBER(o, _2), CONCAT_MEMBER(o, _3),          \
    CONCAT_MEMBER(o, _4), CONCAT_MEMBER(o, _5), CONCAT_MEMBER(o, _6),          \
    CONCAT_MEMBER(o, _7), CONCAT_MEMBER(o, _8), CONCAT_MEMBER(o, _9),          \
    CONCAT_MEMBER(o, _10), CONCAT_MEMBER(o, _11), CONCAT_MEMBER(o, _12),       \
    CONCAT_MEMBER(o, _13), CONCAT_MEMBER(o, _14), CONCAT_MEMBER(o, _15),       \
    CONCAT_MEMBER(o, _16), CONCAT_MEMBER(o, _17), CONCAT_MEMBER(o, _18),       \
    CONCAT_MEMBER(o, _19), CONCAT_MEMBER(o, _20), CONCAT_MEMBER(o, _21),       \
    CONCAT_MEMBER(o, _22), CONCAT_MEMBER(o, _23), CONCAT_MEMBER(o, _24),       \
    CONCAT_MEMBER(o, _25), CONCAT_MEMBER(o, _26), CONCAT_MEMBER(o, _27),       \
    CONCAT_MEMBER(o, _28), CONCAT_MEMBER(o, _29), CONCAT_MEMBER(o, _30),       \
    CONCAT_MEMBER(o, _31), CONCAT_MEMBER(o, _32), CONCAT_MEMBER(o, _33),       \
    CONCAT_MEMBER(o, _34), CONCAT_MEMBER(o, _35), CONCAT_MEMBER(o, _36),       \
    CONCAT_MEMBER(o, _37), CONCAT_MEMBER(o, _38), CONCAT_MEMBER(o, _39),       \
    CONCAT_MEMBER(o, _40), CONCAT_MEMBER(o, _41))
#define YLT_CALL_ARGS42(f, o, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11,    \
                        _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, \
                        _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, \
                        _34, _35, _36, _37, _38, _39, _40, _41, _42)           \
  f(CONCAT_MEMBER(o, _1), CONCAT_MEMBER(o, _2), CONCAT_MEMBER(o, _3),          \
    CONCAT_MEMBER(o, _4), CONCAT_MEMBER(o, _5), CONCAT_MEMBER(o, _6),          \
    CONCAT_MEMBER(o, _7), CONCAT_MEMBER(o, _8), CONCAT_MEMBER(o, _9),          \
    CONCAT_MEMBER(o, _10), CONCAT_MEMBER(o, _11), CONCAT_MEMBER(o, _12),       \
    CONCAT_MEMBER(o, _13), CONCAT_MEMBER(o, _14), CONCAT_MEMBER(o, _15),       \
    CONCAT_MEMBER(o, _16), CONCAT_MEMBER(o, _17), CONCAT_MEMBER(o, _18),       \
    CONCAT_MEMBER(o, _19), CONCAT_MEMBER(o, _20), CONCAT_MEMBER(o, _21),       \
    CONCAT_MEMBER(o, _22), CONCAT_MEMBER(o, _23), CONCAT_MEMBER(o, _24),       \
    CONCAT_MEMBER(o, _25), CONCAT_MEMBER(o, _26), CONCAT_MEMBER(o, _27),       \
    CONCAT_MEMBER(o, _28), CONCAT_MEMBER(o, _29), CONCAT_MEMBER(o, _30),       \
    CONCAT_MEMBER(o, _31), CONCAT_MEMBER(o, _32), CONCAT_MEMBER(o, _33),       \
    CONCAT_MEMBER(o, _34), CONCAT_MEMBER(o, _35), CONCAT_MEMBER(o, _36),       \
    CONCAT_MEMBER(o, _37), CONCAT_MEMBER(o, _38), CONCAT_MEMBER(o, _39),       \
    CONCAT_MEMBER(o, _40), CONCAT_MEMBER(o, _41), CONCAT_MEMBER(o, _42))
#define YLT_CALL_ARGS43(f, o, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11,    \
                        _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, \
                        _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, \
                        _34, _35, _36, _37, _38, _39, _40, _41, _42, _43)      \
  f(CONCAT_MEMBER(o, _1), CONCAT_MEMBER(o, _2), CONCAT_MEMBER(o, _3),          \
    CONCAT_MEMBER(o, _4), CONCAT_MEMBER(o, _5), CONCAT_MEMBER(o, _6),          \
    CONCAT_MEMBER(o, _7), CONCAT_MEMBER(o, _8), CONCAT_MEMBER(o, _9),          \
    CONCAT_MEMBER(o, _10), CONCAT_MEMBER(o, _11), CONCAT_MEMBER(o, _12),       \
    CONCAT_MEMBER(o, _13), CONCAT_MEMBER(o, _14), CONCAT_MEMBER(o, _15),       \
    CONCAT_MEMBER(o, _16), CONCAT_MEMBER(o, _17), CONCAT_MEMBER(o, _18),       \
    CONCAT_MEMBER(o, _19), CONCAT_MEMBER(o, _20), CONCAT_MEMBER(o, _21),       \
    CONCAT_MEMBER(o, _22), CONCAT_MEMBER(o, _23), CONCAT_MEMBER(o, _24),       \
    CONCAT_MEMBER(o, _25), CONCAT_MEMBER(o, _26), CONCAT_MEMBER(o, _27),       \
    CONCAT_MEMBER(o, _28), CONCAT_MEMBER(o, _29), CONCAT_MEMBER(o, _30),       \
    CONCAT_MEMBER(o, _31), CONCAT_MEMBER(o, _32), CONCAT_MEMBER(o, _33),       \
    CONCAT_MEMBER(o, _34), CONCAT_MEMBER(o, _35), CONCAT_MEMBER(o, _36),       \
    CONCAT_MEMBER(o, _37), CONCAT_MEMBER(o, _38), CONCAT_MEMBER(o, _39),       \
    CONCAT_MEMBER(o, _40), CONCAT_MEMBER(o, _41), CONCAT_MEMBER(o, _42),       \
    CONCAT_MEMBER(o, _43))
#define YLT_CALL_ARGS44(f, o, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11,    \
                        _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, \
                        _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, \
                        _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44) \
  f(CONCAT_MEMBER(o, _1), CONCAT_MEMBER(o, _2), CONCAT_MEMBER(o, _3),          \
    CONCAT_MEMBER(o, _4), CONCAT_MEMBER(o, _5), CONCAT_MEMBER(o, _6),          \
    CONCAT_MEMBER(o, _7), CONCAT_MEMBER(o, _8), CONCAT_MEMBER(o, _9),          \
    CONCAT_MEMBER(o, _10), CONCAT_MEMBER(o, _11), CONCAT_MEMBER(o, _12),       \
    CONCAT_MEMBER(o, _13), CONCAT_MEMBER(o, _14), CONCAT_MEMBER(o, _15),       \
    CONCAT_MEMBER(o, _16), CONCAT_MEMBER(o, _17), CONCAT_MEMBER(o, _18),       \
    CONCAT_MEMBER(o, _19), CONCAT_MEMBER(o, _20), CONCAT_MEMBER(o, _21),       \
    CONCAT_MEMBER(o, _22), CONCAT_MEMBER(o, _23), CONCAT_MEMBER(o, _24),       \
    CONCAT_MEMBER(o, _25), CONCAT_MEMBER(o, _26), CONCAT_MEMBER(o, _27),       \
    CONCAT_MEMBER(o, _28), CONCAT_MEMBER(o, _29), CONCAT_MEMBER(o, _30),       \
    CONCAT_MEMBER(o, _31), CONCAT_MEMBER(o, _32), CONCAT_MEMBER(o, _33),       \
    CONCAT_MEMBER(o, _34), CONCAT_MEMBER(o, _35), CONCAT_MEMBER(o, _36),       \
    CONCAT_MEMBER(o, _37), CONCAT_MEMBER(o, _38), CONCAT_MEMBER(o, _39),       \
    CONCAT_MEMBER(o, _40), CONCAT_MEMBER(o, _41), CONCAT_MEMBER(o, _42),       \
    CONCAT_MEMBER(o, _43), CONCAT_MEMBER(o, _44))
#define YLT_CALL_ARGS45(                                                       \
    f, o, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15,    \
    _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, \
    _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45) \
  f(CONCAT_MEMBER(o, _1), CONCAT_MEMBER(o, _2), CONCAT_MEMBER(o, _3),          \
    CONCAT_MEMBER(o, _4), CONCAT_MEMBER(o, _5), CONCAT_MEMBER(o, _6),          \
    CONCAT_MEMBER(o, _7), CONCAT_MEMBER(o, _8), CONCAT_MEMBER(o, _9),          \
    CONCAT_MEMBER(o, _10), CONCAT_MEMBER(o, _11), CONCAT_MEMBER(o, _12),       \
    CONCAT_MEMBER(o, _13), CONCAT_MEMBER(o, _14), CONCAT_MEMBER(o, _15),       \
    CONCAT_MEMBER(o, _16), CONCAT_MEMBER(o, _17), CONCAT_MEMBER(o, _18),       \
    CONCAT_MEMBER(o, _19), CONCAT_MEMBER(o, _20), CONCAT_MEMBER(o, _21),       \
    CONCAT_MEMBER(o, _22), CONCAT_MEMBER(o, _23), CONCAT_MEMBER(o, _24),       \
    CONCAT_MEMBER(o, _25), CONCAT_MEMBER(o, _26), CONCAT_MEMBER(o, _27),       \
    CONCAT_MEMBER(o, _28), CONCAT_MEMBER(o, _29), CONCAT_MEMBER(o, _30),       \
    CONCAT_MEMBER(o, _31), CONCAT_MEMBER(o, _32), CONCAT_MEMBER(o, _33),       \
    CONCAT_MEMBER(o, _34), CONCAT_MEMBER(o, _35), CONCAT_MEMBER(o, _36),       \
    CONCAT_MEMBER(o, _37), CONCAT_MEMBER(o, _38), CONCAT_MEMBER(o, _39),       \
    CONCAT_MEMBER(o, _40), CONCAT_MEMBER(o, _41), CONCAT_MEMBER(o, _42),       \
    CONCAT_MEMBER(o, _43), CONCAT_MEMBER(o, _44), CONCAT_MEMBER(o, _45))
#define YLT_CALL_ARGS46(f, o, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11,    \
                        _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, \
                        _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, \
                        _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, \
                        _45, _46)                                              \
  f(CONCAT_MEMBER(o, _1), CONCAT_MEMBER(o, _2), CONCAT_MEMBER(o, _3),          \
    CONCAT_MEMBER(o, _4), CONCAT_MEMBER(o, _5), CONCAT_MEMBER(o, _6),          \
    CONCAT_MEMBER(o, _7), CONCAT_MEMBER(o, _8), CONCAT_MEMBER(o, _9),          \
    CONCAT_MEMBER(o, _10), CONCAT_MEMBER(o, _11), CONCAT_MEMBER(o, _12),       \
    CONCAT_MEMBER(o, _13), CONCAT_MEMBER(o, _14), CONCAT_MEMBER(o, _15),       \
    CONCAT_MEMBER(o, _16), CONCAT_MEMBER(o, _17), CONCAT_MEMBER(o, _18),       \
    CONCAT_MEMBER(o, _19), CONCAT_MEMBER(o, _20), CONCAT_MEMBER(o, _21),       \
    CONCAT_MEMBER(o, _22), CONCAT_MEMBER(o, _23), CONCAT_MEMBER(o, _24),       \
    CONCAT_MEMBER(o, _25), CONCAT_MEMBER(o, _26), CONCAT_MEMBER(o, _27),       \
    CONCAT_MEMBER(o, _28), CONCAT_MEMBER(o, _29), CONCAT_MEMBER(o, _30),       \
    CONCAT_MEMBER(o, _31), CONCAT_MEMBER(o, _32), CONCAT_MEMBER(o, _33),       \
    CONCAT_MEMBER(o, _34), CONCAT_MEMBER(o, _35), CONCAT_MEMBER(o, _36),       \
    CONCAT_MEMBER(o, _37), CONCAT_MEMBER(o, _38), CONCAT_MEMBER(o, _39),       \
    CONCAT_MEMBER(o, _40), CONCAT_MEMBER(o, _41), CONCAT_MEMBER(o, _42),       \
    CONCAT_MEMBER(o, _43), CONCAT_MEMBER(o, _44), CONCAT_MEMBER(o, _45),       \
    CONCAT_MEMBER(o, _46))
#define YLT_CALL_ARGS47(f, o, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11,    \
                        _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, \
                        _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, \
                        _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, \
                        _45, _46, _47)                                         \
  f(CONCAT_MEMBER(o, _1), CONCAT_MEMBER(o, _2), CONCAT_MEMBER(o, _3),          \
    CONCAT_MEMBER(o, _4), CONCAT_MEMBER(o, _5), CONCAT_MEMBER(o, _6),          \
    CONCAT_MEMBER(o, _7), CONCAT_MEMBER(o, _8), CONCAT_MEMBER(o, _9),          \
    CONCAT_MEMBER(o, _10), CONCAT_MEMBER(o, _11), CONCAT_MEMBER(o, _12),       \
    CONCAT_MEMBER(o, _13), CONCAT_MEMBER(o, _14), CONCAT_MEMBER(o, _15),       \
    CONCAT_MEMBER(o, _16), CONCAT_MEMBER(o, _17), CONCAT_MEMBER(o, _18),       \
    CONCAT_MEMBER(o, _19), CONCAT_MEMBER(o, _20), CONCAT_MEMBER(o, _21),       \
    CONCAT_MEMBER(o, _22), CONCAT_MEMBER(o, _23), CONCAT_MEMBER(o, _24),       \
    CONCAT_MEMBER(o, _25), CONCAT_MEMBER(o, _26), CONCAT_MEMBER(o, _27),       \
    CONCAT_MEMBER(o, _28), CONCAT_MEMBER(o, _29), CONCAT_MEMBER(o, _30),       \
    CONCAT_MEMBER(o, _31), CONCAT_MEMBER(o, _32), CONCAT_MEMBER(o, _33),       \
    CONCAT_MEMBER(o, _34), CONCAT_MEMBER(o, _35), CONCAT_MEMBER(o, _36),       \
    CONCAT_MEMBER(o, _37), CONCAT_MEMBER(o, _38), CONCAT_MEMBER(o, _39),       \
    CONCAT_MEMBER(o, _40), CONCAT_MEMBER(o, _41), CONCAT_MEMBER(o, _42),       \
    CONCAT_MEMBER(o, _43), CONCAT_MEMBER(o, _44), CONCAT_MEMBER(o, _45),       \
    CONCAT_MEMBER(o, _46), CONCAT_MEMBER(o, _47))
#define YLT_CALL_ARGS48(f, o, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11,    \
                        _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, \
                        _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, \
                        _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, \
                        _45, _46, _47, _48)                                    \
  f(CONCAT_MEMBER(o, _1), CONCAT_MEMBER(o, _2), CONCAT_MEMBER(o, _3),          \
    CONCAT_MEMBER(o, _4), CONCAT_MEMBER(o, _5), CONCAT_MEMBER(o, _6),          \
    CONCAT_MEMBER(o, _7), CONCAT_MEMBER(o, _8), CONCAT_MEMBER(o, _9),          \
    CONCAT_MEMBER(o, _10), CONCAT_MEMBER(o, _11), CONCAT_MEMBER(o, _12),       \
    CONCAT_MEMBER(o, _13), CONCAT_MEMBER(o, _14), CONCAT_MEMBER(o, _15),       \
    CONCAT_MEMBER(o, _16), CONCAT_MEMBER(o, _17), CONCAT_MEMBER(o, _18),       \
    CONCAT_MEMBER(o, _19), CONCAT_MEMBER(o, _20), CONCAT_MEMBER(o, _21),       \
    CONCAT_MEMBER(o, _22), CONCAT_MEMBER(o, _23), CONCAT_MEMBER(o, _24),       \
    CONCAT_MEMBER(o, _25), CONCAT_MEMBER(o, _26), CONCAT_MEMBER(o, _27),       \
    CONCAT_MEMBER(o, _28), CONCAT_MEMBER(o, _29), CONCAT_MEMBER(o, _30),       \
    CONCAT_MEMBER(o, _31), CONCAT_MEMBER(o, _32), CONCAT_MEMBER(o, _33),       \
    CONCAT_MEMBER(o, _34), CONCAT_MEMBER(o, _35), CONCAT_MEMBER(o, _36),       \
    CONCAT_MEMBER(o, _37), CONCAT_MEMBER(o, _38), CONCAT_MEMBER(o, _39),       \
    CONCAT_MEMBER(o, _40), CONCAT_MEMBER(o, _41), CONCAT_MEMBER(o, _42),       \
    CONCAT_MEMBER(o, _43), CONCAT_MEMBER(o, _44), CONCAT_MEMBER(o, _45),       \
    CONCAT_MEMBER(o, _46), CONCAT_MEMBER(o, _47), CONCAT_MEMBER(o, _48))
#define YLT_CALL_ARGS49(f, o, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11,    \
                        _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, \
                        _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, \
                        _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, \
                        _45, _46, _47, _48, _49)                               \
  f(CONCAT_MEMBER(o, _1), CONCAT_MEMBER(o, _2), CONCAT_MEMBER(o, _3),          \
    CONCAT_MEMBER(o, _4), CONCAT_MEMBER(o, _5), CONCAT_MEMBER(o, _6),          \
    CONCAT_MEMBER(o, _7), CONCAT_MEMBER(o, _8), CONCAT_MEMBER(o, _9),          \
    CONCAT_MEMBER(o, _10), CONCAT_MEMBER(o, _11), CONCAT_MEMBER(o, _12),       \
    CONCAT_MEMBER(o, _13), CONCAT_MEMBER(o, _14), CONCAT_MEMBER(o, _15),       \
    CONCAT_MEMBER(o, _16), CONCAT_MEMBER(o, _17), CONCAT_MEMBER(o, _18),       \
    CONCAT_MEMBER(o, _19), CONCAT_MEMBER(o, _20), CONCAT_MEMBER(o, _21),       \
    CONCAT_MEMBER(o, _22), CONCAT_MEMBER(o, _23), CONCAT_MEMBER(o, _24),       \
    CONCAT_MEMBER(o, _25), CONCAT_MEMBER(o, _26), CONCAT_MEMBER(o, _27),       \
    CONCAT_MEMBER(o, _28), CONCAT_MEMBER(o, _29), CONCAT_MEMBER(o, _30),       \
    CONCAT_MEMBER(o, _31), CONCAT_MEMBER(o, _32), CONCAT_MEMBER(o, _33),       \
    CONCAT_MEMBER(o, _34), CONCAT_MEMBER(o, _35), CONCAT_MEMBER(o, _36),       \
    CONCAT_MEMBER(o, _37), CONCAT_MEMBER(o, _38), CONCAT_MEMBER(o, _39),       \
    CONCAT_MEMBER(o, _40), CONCAT_MEMBER(o, _41), CONCAT_MEMBER(o, _42),       \
    CONCAT_MEMBER(o, _43), CONCAT_MEMBER(o, _44), CONCAT_MEMBER(o, _45),       \
    CONCAT_MEMBER(o, _46), CONCAT_MEMBER(o, _47), CONCAT_MEMBER(o, _48),       \
    CONCAT_MEMBER(o, _49))
#define YLT_CALL_ARGS50(f, o, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11,    \
                        _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, \
                        _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, \
                        _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, \
                        _45, _46, _47, _48, _49, _50)                          \
  f(CONCAT_MEMBER(o, _1), CONCAT_MEMBER(o, _2), CONCAT_MEMBER(o, _3),          \
    CONCAT_MEMBER(o, _4), CONCAT_MEMBER(o, _5), CONCAT_MEMBER(o, _6),          \
    CONCAT_MEMBER(o, _7), CONCAT_MEMBER(o, _8), CONCAT_MEMBER(o, _9),          \
    CONCAT_MEMBER(o, _10), CONCAT_MEMBER(o, _11), CONCAT_MEMBER(o, _12),       \
    CONCAT_MEMBER(o, _13), CONCAT_MEMBER(o, _14), CONCAT_MEMBER(o, _15),       \
    CONCAT_MEMBER(o, _16), CONCAT_MEMBER(o, _17), CONCAT_MEMBER(o, _18),       \
    CONCAT_MEMBER(o, _19), CONCAT_MEMBER(o, _20), CONCAT_MEMBER(o, _21),       \
    CONCAT_MEMBER(o, _22), CONCAT_MEMBER(o, _23), CONCAT_MEMBER(o, _24),       \
    CONCAT_MEMBER(o, _25), CONCAT_MEMBER(o, _26), CONCAT_MEMBER(o, _27),       \
    CONCAT_MEMBER(o, _28), CONCAT_MEMBER(o, _29), CONCAT_MEMBER(o, _30),       \
    CONCAT_MEMBER(o, _31), CONCAT_MEMBER(o, _32), CONCAT_MEMBER(o, _33),       \
    CONCAT_MEMBER(o, _34), CONCAT_MEMBER(o, _35), CONCAT_MEMBER(o, _36),       \
    CONCAT_MEMBER(o, _37), CONCAT_MEMBER(o, _38), CONCAT_MEMBER(o, _39),       \
    CONCAT_MEMBER(o, _40), CONCAT_MEMBER(o, _41), CONCAT_MEMBER(o, _42),       \
    CONCAT_MEMBER(o, _43), CONCAT_MEMBER(o, _44), CONCAT_MEMBER(o, _45),       \
    CONCAT_MEMBER(o, _46), CONCAT_MEMBER(o, _47), CONCAT_MEMBER(o, _48),       \
    CONCAT_MEMBER(o, _49), CONCAT_MEMBER(o, _50))
#define YLT_CALL_ARGS51(f, o, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11,    \
                        _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, \
                        _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, \
                        _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, \
                        _45, _46, _47, _48, _49, _50, _51)                     \
  f(CONCAT_MEMBER(o, _1), CONCAT_MEMBER(o, _2), CONCAT_MEMBER(o, _3),          \
    CONCAT_MEMBER(o, _4), CONCAT_MEMBER(o, _5), CONCAT_MEMBER(o, _6),          \
    CONCAT_MEMBER(o, _7), CONCAT_MEMBER(o, _8), CONCAT_MEMBER(o, _9),          \
    CONCAT_MEMBER(o, _10), CONCAT_MEMBER(o, _11), CONCAT_MEMBER(o, _12),       \
    CONCAT_MEMBER(o, _13), CONCAT_MEMBER(o, _14), CONCAT_MEMBER(o, _15),       \
    CONCAT_MEMBER(o, _16), CONCAT_MEMBER(o, _17), CONCAT_MEMBER(o, _18),       \
    CONCAT_MEMBER(o, _19), CONCAT_MEMBER(o, _20), CONCAT_MEMBER(o, _21),       \
    CONCAT_MEMBER(o, _22), CONCAT_MEMBER(o, _23), CONCAT_MEMBER(o, _24),       \
    CONCAT_MEMBER(o, _25), CONCAT_MEMBER(o, _26), CONCAT_MEMBER(o, _27),       \
    CONCAT_MEMBER(o, _28), CONCAT_MEMBER(o, _29), CONCAT_MEMBER(o, _30),       \
    CONCAT_MEMBER(o, _31), CONCAT_MEMBER(o, _32), CONCAT_MEMBER(o, _33),       \
    CONCAT_MEMBER(o, _34), CONCAT_MEMBER(o, _35), CONCAT_MEMBER(o, _36),       \
    CONCAT_MEMBER(o, _37), CONCAT_MEMBER(o, _38), CONCAT_MEMBER(o, _39),       \
    CONCAT_MEMBER(o, _40), CONCAT_MEMBER(o, _41), CONCAT_MEMBER(o, _42),       \
    CONCAT_MEMBER(o, _43), CONCAT_MEMBER(o, _44), CONCAT_MEMBER(o, _45),       \
    CONCAT_MEMBER(o, _46), CONCAT_MEMBER(o, _47), CONCAT_MEMBER(o, _48),       \
    CONCAT_MEMBER(o, _49), CONCAT_MEMBER(o, _50), CONCAT_MEMBER(o, _51))
#define YLT_CALL_ARGS52(f, o, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11,    \
                        _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, \
                        _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, \
                        _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, \
                        _45, _46, _47, _48, _49, _50, _51, _52)                \
  f(CONCAT_MEMBER(o, _1), CONCAT_MEMBER(o, _2), CONCAT_MEMBER(o, _3),          \
    CONCAT_MEMBER(o, _4), CONCAT_MEMBER(o, _5), CONCAT_MEMBER(o, _6),          \
    CONCAT_MEMBER(o, _7), CONCAT_MEMBER(o, _8), CONCAT_MEMBER(o, _9),          \
    CONCAT_MEMBER(o, _10), CONCAT_MEMBER(o, _11), CONCAT_MEMBER(o, _12),       \
    CONCAT_MEMBER(o, _13), CONCAT_MEMBER(o, _14), CONCAT_MEMBER(o, _15),       \
    CONCAT_MEMBER(o, _16), CONCAT_MEMBER(o, _17), CONCAT_MEMBER(o, _18),       \
    CONCAT_MEMBER(o, _19), CONCAT_MEMBER(o, _20), CONCAT_MEMBER(o, _21),       \
    CONCAT_MEMBER(o, _22), CONCAT_MEMBER(o, _23), CONCAT_MEMBER(o, _24),       \
    CONCAT_MEMBER(o, _25), CONCAT_MEMBER(o, _26), CONCAT_MEMBER(o, _27),       \
    CONCAT_MEMBER(o, _28), CONCAT_MEMBER(o, _29), CONCAT_MEMBER(o, _30),       \
    CONCAT_MEMBER(o, _31), CONCAT_MEMBER(o, _32), CONCAT_MEMBER(o, _33),       \
    CONCAT_MEMBER(o, _34), CONCAT_MEMBER(o, _35), CONCAT_MEMBER(o, _36),       \
    CONCAT_MEMBER(o, _37), CONCAT_MEMBER(o, _38), CONCAT_MEMBER(o, _39),       \
    CONCAT_MEMBER(o, _40), CONCAT_MEMBER(o, _41), CONCAT_MEMBER(o, _42),       \
    CONCAT_MEMBER(o, _43), CONCAT_MEMBER(o, _44), CONCAT_MEMBER(o, _45),       \
    CONCAT_MEMBER(o, _46), CONCAT_MEMBER(o, _47), CONCAT_MEMBER(o, _48),       \
    CONCAT_MEMBER(o, _49), CONCAT_MEMBER(o, _50), CONCAT_MEMBER(o, _51),       \
    CONCAT_MEMBER(o, _52))
#define YLT_CALL_ARGS53(f, o, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11,    \
                        _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, \
                        _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, \
                        _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, \
                        _45, _46, _47, _48, _49, _50, _51, _52, _53)           \
  f(CONCAT_MEMBER(o, _1), CONCAT_MEMBER(o, _2), CONCAT_MEMBER(o, _3),          \
    CONCAT_MEMBER(o, _4), CONCAT_MEMBER(o, _5), CONCAT_MEMBER(o, _6),          \
    CONCAT_MEMBER(o, _7), CONCAT_MEMBER(o, _8), CONCAT_MEMBER(o, _9),          \
    CONCAT_MEMBER(o, _10), CONCAT_MEMBER(o, _11), CONCAT_MEMBER(o, _12),       \
    CONCAT_MEMBER(o, _13), CONCAT_MEMBER(o, _14), CONCAT_MEMBER(o, _15),       \
    CONCAT_MEMBER(o, _16), CONCAT_MEMBER(o, _17), CONCAT_MEMBER(o, _18),       \
    CONCAT_MEMBER(o, _19), CONCAT_MEMBER(o, _20), CONCAT_MEMBER(o, _21),       \
    CONCAT_MEMBER(o, _22), CONCAT_MEMBER(o, _23), CONCAT_MEMBER(o, _24),       \
    CONCAT_MEMBER(o, _25), CONCAT_MEMBER(o, _26), CONCAT_MEMBER(o, _27),       \
    CONCAT_MEMBER(o, _28), CONCAT_MEMBER(o, _29), CONCAT_MEMBER(o, _30),       \
    CONCAT_MEMBER(o, _31), CONCAT_MEMBER(o, _32), CONCAT_MEMBER(o, _33),       \
    CONCAT_MEMBER(o, _34), CONCAT_MEMBER(o, _35), CONCAT_MEMBER(o, _36),       \
    CONCAT_MEMBER(o, _37), CONCAT_MEMBER(o, _38), CONCAT_MEMBER(o, _39),       \
    CONCAT_MEMBER(o, _40), CONCAT_MEMBER(o, _41), CONCAT_MEMBER(o, _42),       \
    CONCAT_MEMBER(o, _43), CONCAT_MEMBER(o, _44), CONCAT_MEMBER(o, _45),       \
    CONCAT_MEMBER(o, _46), CONCAT_MEMBER(o, _47), CONCAT_MEMBER(o, _48),       \
    CONCAT_MEMBER(o, _49), CONCAT_MEMBER(o, _50), CONCAT_MEMBER(o, _51),       \
    CONCAT_MEMBER(o, _52), CONCAT_MEMBER(o, _53))
#define YLT_CALL_ARGS54(f, o, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11,    \
                        _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, \
                        _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, \
                        _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, \
                        _45, _46, _47, _48, _49, _50, _51, _52, _53, _54)      \
  f(CONCAT_MEMBER(o, _1), CONCAT_MEMBER(o, _2), CONCAT_MEMBER(o, _3),          \
    CONCAT_MEMBER(o, _4), CONCAT_MEMBER(o, _5), CONCAT_MEMBER(o, _6),          \
    CONCAT_MEMBER(o, _7), CONCAT_MEMBER(o, _8), CONCAT_MEMBER(o, _9),          \
    CONCAT_MEMBER(o, _10), CONCAT_MEMBER(o, _11), CONCAT_MEMBER(o, _12),       \
    CONCAT_MEMBER(o, _13), CONCAT_MEMBER(o, _14), CONCAT_MEMBER(o, _15),       \
    CONCAT_MEMBER(o, _16), CONCAT_MEMBER(o, _17), CONCAT_MEMBER(o, _18),       \
    CONCAT_MEMBER(o, _19), CONCAT_MEMBER(o, _20), CONCAT_MEMBER(o, _21),       \
    CONCAT_MEMBER(o, _22), CONCAT_MEMBER(o, _23), CONCAT_MEMBER(o, _24),       \
    CONCAT_MEMBER(o, _25), CONCAT_MEMBER(o, _26), CONCAT_MEMBER(o, _27),       \
    CONCAT_MEMBER(o, _28), CONCAT_MEMBER(o, _29), CONCAT_MEMBER(o, _30),       \
    CONCAT_MEMBER(o, _31), CONCAT_MEMBER(o, _32), CONCAT_MEMBER(o, _33),       \
    CONCAT_MEMBER(o, _34), CONCAT_MEMBER(o, _35), CONCAT_MEMBER(o, _36),       \
    CONCAT_MEMBER(o, _37), CONCAT_MEMBER(o, _38), CONCAT_MEMBER(o, _39),       \
    CONCAT_MEMBER(o, _40), CONCAT_MEMBER(o, _41), CONCAT_MEMBER(o, _42),       \
    CONCAT_MEMBER(o, _43), CONCAT_MEMBER(o, _44), CONCAT_MEMBER(o, _45),       \
    CONCAT_MEMBER(o, _46), CONCAT_MEMBER(o, _47), CONCAT_MEMBER(o, _48),       \
    CONCAT_MEMBER(o, _49), CONCAT_MEMBER(o, _50), CONCAT_MEMBER(o, _51),       \
    CONCAT_MEMBER(o, _52), CONCAT_MEMBER(o, _53), CONCAT_MEMBER(o, _54))
#define YLT_CALL_ARGS55(f, o, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11,    \
                        _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, \
                        _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, \
                        _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, \
                        _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55) \
  f(CONCAT_MEMBER(o, _1), CONCAT_MEMBER(o, _2), CONCAT_MEMBER(o, _3),          \
    CONCAT_MEMBER(o, _4), CONCAT_MEMBER(o, _5), CONCAT_MEMBER(o, _6),          \
    CONCAT_MEMBER(o, _7), CONCAT_MEMBER(o, _8), CONCAT_MEMBER(o, _9),          \
    CONCAT_MEMBER(o, _10), CONCAT_MEMBER(o, _11), CONCAT_MEMBER(o, _12),       \
    CONCAT_MEMBER(o, _13), CONCAT_MEMBER(o, _14), CONCAT_MEMBER(o, _15),       \
    CONCAT_MEMBER(o, _16), CONCAT_MEMBER(o, _17), CONCAT_MEMBER(o, _18),       \
    CONCAT_MEMBER(o, _19), CONCAT_MEMBER(o, _20), CONCAT_MEMBER(o, _21),       \
    CONCAT_MEMBER(o, _22), CONCAT_MEMBER(o, _23), CONCAT_MEMBER(o, _24),       \
    CONCAT_MEMBER(o, _25), CONCAT_MEMBER(o, _26), CONCAT_MEMBER(o, _27),       \
    CONCAT_MEMBER(o, _28), CONCAT_MEMBER(o, _29), CONCAT_MEMBER(o, _30),       \
    CONCAT_MEMBER(o, _31), CONCAT_MEMBER(o, _32), CONCAT_MEMBER(o, _33),       \
    CONCAT_MEMBER(o, _34), CONCAT_MEMBER(o, _35), CONCAT_MEMBER(o, _36),       \
    CONCAT_MEMBER(o, _37), CONCAT_MEMBER(o, _38), CONCAT_MEMBER(o, _39),       \
    CONCAT_MEMBER(o, _40), CONCAT_MEMBER(o, _41), CONCAT_MEMBER(o, _42),       \
    CONCAT_MEMBER(o, _43), CONCAT_MEMBER(o, _44), CONCAT_MEMBER(o, _45),       \
    CONCAT_MEMBER(o, _46), CONCAT_MEMBER(o, _47), CONCAT_MEMBER(o, _48),       \
    CONCAT_MEMBER(o, _49), CONCAT_MEMBER(o, _50), CONCAT_MEMBER(o, _51),       \
    CONCAT_MEMBER(o, _52), CONCAT_MEMBER(o, _53), CONCAT_MEMBER(o, _54),       \
    CONCAT_MEMBER(o, _55))
#define YLT_CALL_ARGS56(                                                       \
    f, o, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15,    \
    _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, \
    _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, \
    _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56)                     \
  f(CONCAT_MEMBER(o, _1), CONCAT_MEMBER(o, _2), CONCAT_MEMBER(o, _3),          \
    CONCAT_MEMBER(o, _4), CONCAT_MEMBER(o, _5), CONCAT_MEMBER(o, _6),          \
    CONCAT_MEMBER(o, _7), CONCAT_MEMBER(o, _8), CONCAT_MEMBER(o, _9),          \
    CONCAT_MEMBER(o, _10), CONCAT_MEMBER(o, _11), CONCAT_MEMBER(o, _12),       \
    CONCAT_MEMBER(o, _13), CONCAT_MEMBER(o, _14), CONCAT_MEMBER(o, _15),       \
    CONCAT_MEMBER(o, _16), CONCAT_MEMBER(o, _17), CONCAT_MEMBER(o, _18),       \
    CONCAT_MEMBER(o, _19), CONCAT_MEMBER(o, _20), CONCAT_MEMBER(o, _21),       \
    CONCAT_MEMBER(o, _22), CONCAT_MEMBER(o, _23), CONCAT_MEMBER(o, _24),       \
    CONCAT_MEMBER(o, _25), CONCAT_MEMBER(o, _26), CONCAT_MEMBER(o, _27),       \
    CONCAT_MEMBER(o, _28), CONCAT_MEMBER(o, _29), CONCAT_MEMBER(o, _30),       \
    CONCAT_MEMBER(o, _31), CONCAT_MEMBER(o, _32), CONCAT_MEMBER(o, _33),       \
    CONCAT_MEMBER(o, _34), CONCAT_MEMBER(o, _35), CONCAT_MEMBER(o, _36),       \
    CONCAT_MEMBER(o, _37), CONCAT_MEMBER(o, _38), CONCAT_MEMBER(o, _39),       \
    CONCAT_MEMBER(o, _40), CONCAT_MEMBER(o, _41), CONCAT_MEMBER(o, _42),       \
    CONCAT_MEMBER(o, _43), CONCAT_MEMBER(o, _44), CONCAT_MEMBER(o, _45),       \
    CONCAT_MEMBER(o, _46), CONCAT_MEMBER(o, _47), CONCAT_MEMBER(o, _48),       \
    CONCAT_MEMBER(o, _49), CONCAT_MEMBER(o, _50), CONCAT_MEMBER(o, _51),       \
    CONCAT_MEMBER(o, _52), CONCAT_MEMBER(o, _53), CONCAT_MEMBER(o, _54),       \
    CONCAT_MEMBER(o, _55), CONCAT_MEMBER(o, _56))
#define YLT_CALL_ARGS57(                                                       \
    f, o, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15,    \
    _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, \
    _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, \
    _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57)                \
  f(CONCAT_MEMBER(o, _1), CONCAT_MEMBER(o, _2), CONCAT_MEMBER(o, _3),          \
    CONCAT_MEMBER(o, _4), CONCAT_MEMBER(o, _5), CONCAT_MEMBER(o, _6),          \
    CONCAT_MEMBER(o, _7), CONCAT_MEMBER(o, _8), CONCAT_MEMBER(o, _9),          \
    CONCAT_MEMBER(o, _10), CONCAT_MEMBER(o, _11), CONCAT_MEMBER(o, _12),       \
    CONCAT_MEMBER(o, _13), CONCAT_MEMBER(o, _14), CONCAT_MEMBER(o, _15),       \
    CONCAT_MEMBER(o, _16), CONCAT_MEMBER(o, _17), CONCAT_MEMBER(o, _18),       \
    CONCAT_MEMBER(o, _19), CONCAT_MEMBER(o, _20), CONCAT_MEMBER(o, _21),       \
    CONCAT_MEMBER(o, _22), CONCAT_MEMBER(o, _23), CONCAT_MEMBER(o, _24),       \
    CONCAT_MEMBER(o, _25), CONCAT_MEMBER(o, _26), CONCAT_MEMBER(o, _27),       \
    CONCAT_MEMBER(o, _28), CONCAT_MEMBER(o, _29), CONCAT_MEMBER(o, _30),       \
    CONCAT_MEMBER(o, _31), CONCAT_MEMBER(o, _32), CONCAT_MEMBER(o, _33),       \
    CONCAT_MEMBER(o, _34), CONCAT_MEMBER(o, _35), CONCAT_MEMBER(o, _36),       \
    CONCAT_MEMBER(o, _37), CONCAT_MEMBER(o, _38), CONCAT_MEMBER(o, _39),       \
    CONCAT_MEMBER(o, _40), CONCAT_MEMBER(o, _41), CONCAT_MEMBER(o, _42),       \
    CONCAT_MEMBER(o, _43), CONCAT_MEMBER(o, _44), CONCAT_MEMBER(o, _45),       \
    CONCAT_MEMBER(o, _46), CONCAT_MEMBER(o, _47), CONCAT_MEMBER(o, _48),       \
    CONCAT_MEMBER(o, _49), CONCAT_MEMBER(o, _50), CONCAT_MEMBER(o, _51),       \
    CONCAT_MEMBER(o, _52), CONCAT_MEMBER(o, _53), CONCAT_MEMBER(o, _54),       \
    CONCAT_MEMBER(o, _55), CONCAT_MEMBER(o, _56), CONCAT_MEMBER(o, _57))
#define YLT_CALL_ARGS58(                                                       \
    f, o, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15,    \
    _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, \
    _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, \
    _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58)           \
  f(CONCAT_MEMBER(o, _1), CONCAT_MEMBER(o, _2), CONCAT_MEMBER(o, _3),          \
    CONCAT_MEMBER(o, _4), CONCAT_MEMBER(o, _5), CONCAT_MEMBER(o, _6),          \
    CONCAT_MEMBER(o, _7), CONCAT_MEMBER(o, _8), CONCAT_MEMBER(o, _9),          \
    CONCAT_MEMBER(o, _10), CONCAT_MEMBER(o, _11), CONCAT_MEMBER(o, _12),       \
    CONCAT_MEMBER(o, _13), CONCAT_MEMBER(o, _14), CONCAT_MEMBER(o, _15),       \
    CONCAT_MEMBER(o, _16), CONCAT_MEMBER(o, _17), CONCAT_MEMBER(o, _18),       \
    CONCAT_MEMBER(o, _19), CONCAT_MEMBER(o, _20), CONCAT_MEMBER(o, _21),       \
    CONCAT_MEMBER(o, _22), CONCAT_MEMBER(o, _23), CONCAT_MEMBER(o, _24),       \
    CONCAT_MEMBER(o, _25), CONCAT_MEMBER(o, _26), CONCAT_MEMBER(o, _27),       \
    CONCAT_MEMBER(o, _28), CONCAT_MEMBER(o, _29), CONCAT_MEMBER(o, _30),       \
    CONCAT_MEMBER(o, _31), CONCAT_MEMBER(o, _32), CONCAT_MEMBER(o, _33),       \
    CONCAT_MEMBER(o, _34), CONCAT_MEMBER(o, _35), CONCAT_MEMBER(o, _36),       \
    CONCAT_MEMBER(o, _37), CONCAT_MEMBER(o, _38), CONCAT_MEMBER(o, _39),       \
    CONCAT_MEMBER(o, _40), CONCAT_MEMBER(o, _41), CONCAT_MEMBER(o, _42),       \
    CONCAT_MEMBER(o, _43), CONCAT_MEMBER(o, _44), CONCAT_MEMBER(o, _45),       \
    CONCAT_MEMBER(o, _46), CONCAT_MEMBER(o, _47), CONCAT_MEMBER(o, _48),       \
    CONCAT_MEMBER(o, _49), CONCAT_MEMBER(o, _50), CONCAT_MEMBER(o, _51),       \
    CONCAT_MEMBER(o, _52), CONCAT_MEMBER(o, _53), CONCAT_MEMBER(o, _54),       \
    CONCAT_MEMBER(o, _55), CONCAT_MEMBER(o, _56), CONCAT_MEMBER(o, _57),       \
    CONCAT_MEMBER(o, _58))
#define YLT_CALL_ARGS59(                                                       \
    f, o, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15,    \
    _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, \
    _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, \
    _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59)      \
  f(CONCAT_MEMBER(o, _1), CONCAT_MEMBER(o, _2), CONCAT_MEMBER(o, _3),          \
    CONCAT_MEMBER(o, _4), CONCAT_MEMBER(o, _5), CONCAT_MEMBER(o, _6),          \
    CONCAT_MEMBER(o, _7), CONCAT_MEMBER(o, _8), CONCAT_MEMBER(o, _9),          \
    CONCAT_MEMBER(o, _10), CONCAT_MEMBER(o, _11), CONCAT_MEMBER(o, _12),       \
    CONCAT_MEMBER(o, _13), CONCAT_MEMBER(o, _14), CONCAT_MEMBER(o, _15),       \
    CONCAT_MEMBER(o, _16), CONCAT_MEMBER(o, _17), CONCAT_MEMBER(o, _18),       \
    CONCAT_MEMBER(o, _19), CONCAT_MEMBER(o, _20), CONCAT_MEMBER(o, _21),       \
    CONCAT_MEMBER(o, _22), CONCAT_MEMBER(o, _23), CONCAT_MEMBER(o, _24),       \
    CONCAT_MEMBER(o, _25), CONCAT_MEMBER(o, _26), CONCAT_MEMBER(o, _27),       \
    CONCAT_MEMBER(o, _28), CONCAT_MEMBER(o, _29), CONCAT_MEMBER(o, _30),       \
    CONCAT_MEMBER(o, _31), CONCAT_MEMBER(o, _32), CONCAT_MEMBER(o, _33),       \
    CONCAT_MEMBER(o, _34), CONCAT_MEMBER(o, _35), CONCAT_MEMBER(o, _36),       \
    CONCAT_MEMBER(o, _37), CONCAT_MEMBER(o, _38), CONCAT_MEMBER(o, _39),       \
    CONCAT_MEMBER(o, _40), CONCAT_MEMBER(o, _41), CONCAT_MEMBER(o, _42),       \
    CONCAT_MEMBER(o, _43), CONCAT_MEMBER(o, _44), CONCAT_MEMBER(o, _45),       \
    CONCAT_MEMBER(o, _46), CONCAT_MEMBER(o, _47), CONCAT_MEMBER(o, _48),       \
    CONCAT_MEMBER(o, _49), CONCAT_MEMBER(o, _50), CONCAT_MEMBER(o, _51),       \
    CONCAT_MEMBER(o, _52), CONCAT_MEMBER(o, _53), CONCAT_MEMBER(o, _54),       \
    CONCAT_MEMBER(o, _55), CONCAT_MEMBER(o, _56), CONCAT_MEMBER(o, _57),       \
    CONCAT_MEMBER(o, _58), CONCAT_MEMBER(o, _59))
#define YLT_CALL_ARGS60(                                                       \
    f, o, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15,    \
    _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, \
    _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, \
    _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60) \
  f(CONCAT_MEMBER(o, _1), CONCAT_MEMBER(o, _2), CONCAT_MEMBER(o, _3),          \
    CONCAT_MEMBER(o, _4), CONCAT_MEMBER(o, _5), CONCAT_MEMBER(o, _6),          \
    CONCAT_MEMBER(o, _7), CONCAT_MEMBER(o, _8), CONCAT_MEMBER(o, _9),          \
    CONCAT_MEMBER(o, _10), CONCAT_MEMBER(o, _11), CONCAT_MEMBER(o, _12),       \
    CONCAT_MEMBER(o, _13), CONCAT_MEMBER(o, _14), CONCAT_MEMBER(o, _15),       \
    CONCAT_MEMBER(o, _16), CONCAT_MEMBER(o, _17), CONCAT_MEMBER(o, _18),       \
    CONCAT_MEMBER(o, _19), CONCAT_MEMBER(o, _20), CONCAT_MEMBER(o, _21),       \
    CONCAT_MEMBER(o, _22), CONCAT_MEMBER(o, _23), CONCAT_MEMBER(o, _24),       \
    CONCAT_MEMBER(o, _25), CONCAT_MEMBER(o, _26), CONCAT_MEMBER(o, _27),       \
    CONCAT_MEMBER(o, _28), CONCAT_MEMBER(o, _29), CONCAT_MEMBER(o, _30),       \
    CONCAT_MEMBER(o, _31), CONCAT_MEMBER(o, _32), CONCAT_MEMBER(o, _33),       \
    CONCAT_MEMBER(o, _34), CONCAT_MEMBER(o, _35), CONCAT_MEMBER(o, _36),       \
    CONCAT_MEMBER(o, _37), CONCAT_MEMBER(o, _38), CONCAT_MEMBER(o, _39),       \
    CONCAT_MEMBER(o, _40), CONCAT_MEMBER(o, _41), CONCAT_MEMBER(o, _42),       \
    CONCAT_MEMBER(o, _43), CONCAT_MEMBER(o, _44), CONCAT_MEMBER(o, _45),       \
    CONCAT_MEMBER(o, _46), CONCAT_MEMBER(o, _47), CONCAT_MEMBER(o, _48),       \
    CONCAT_MEMBER(o, _49), CONCAT_MEMBER(o, _50), CONCAT_MEMBER(o, _51),       \
    CONCAT_MEMBER(o, _52), CONCAT_MEMBER(o, _53), CONCAT_MEMBER(o, _54),       \
    CONCAT_MEMBER(o, _55), CONCAT_MEMBER(o, _56), CONCAT_MEMBER(o, _57),       \
    CONCAT_MEMBER(o, _58), CONCAT_MEMBER(o, _59), CONCAT_MEMBER(o, _60))
#define YLT_CALL_ARGS61(f, o, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11,    \
                        _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, \
                        _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, \
                        _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, \
                        _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, \
                        _56, _57, _58, _59, _60, _61)                          \
  f(CONCAT_MEMBER(o, _1), CONCAT_MEMBER(o, _2), CONCAT_MEMBER(o, _3),          \
    CONCAT_MEMBER(o, _4), CONCAT_MEMBER(o, _5), CONCAT_MEMBER(o, _6),          \
    CONCAT_MEMBER(o, _7), CONCAT_MEMBER(o, _8), CONCAT_MEMBER(o, _9),          \
    CONCAT_MEMBER(o, _10), CONCAT_MEMBER(o, _11), CONCAT_MEMBER(o, _12),       \
    CONCAT_MEMBER(o, _13), CONCAT_MEMBER(o, _14), CONCAT_MEMBER(o, _15),       \
    CONCAT_MEMBER(o, _16), CONCAT_MEMBER(o, _17), CONCAT_MEMBER(o, _18),       \
    CONCAT_MEMBER(o, _19), CONCAT_MEMBER(o, _20), CONCAT_MEMBER(o, _21),       \
    CONCAT_MEMBER(o, _22), CONCAT_MEMBER(o, _23), CONCAT_MEMBER(o, _24),       \
    CONCAT_MEMBER(o, _25), CONCAT_MEMBER(o, _26), CONCAT_MEMBER(o, _27),       \
    CONCAT_MEMBER(o, _28), CONCAT_MEMBER(o, _29), CONCAT_MEMBER(o, _30),       \
    CONCAT_MEMBER(o, _31), CONCAT_MEMBER(o, _32), CONCAT_MEMBER(o, _33),       \
    CONCAT_MEMBER(o, _34), CONCAT_MEMBER(o, _35), CONCAT_MEMBER(o, _36),       \
    CONCAT_MEMBER(o, _37), CONCAT_MEMBER(o, _38), CONCAT_MEMBER(o, _39),       \
    CONCAT_MEMBER(o, _40), CONCAT_MEMBER(o, _41), CONCAT_MEMBER(o, _42),       \
    CONCAT_MEMBER(o, _43), CONCAT_MEMBER(o, _44), CONCAT_MEMBER(o, _45),       \
    CONCAT_MEMBER(o, _46), CONCAT_MEMBER(o, _47), CONCAT_MEMBER(o, _48),       \
    CONCAT_MEMBER(o, _49), CONCAT_MEMBER(o, _50), CONCAT_MEMBER(o, _51),       \
    CONCAT_MEMBER(o, _52), CONCAT_MEMBER(o, _53), CONCAT_MEMBER(o, _54),       \
    CONCAT_MEMBER(o, _55), CONCAT_MEMBER(o, _56), CONCAT_MEMBER(o, _57),       \
    CONCAT_MEMBER(o, _58), CONCAT_MEMBER(o, _59), CONCAT_MEMBER(o, _60),       \
    CONCAT_MEMBER(o, _61))
#define YLT_CALL_ARGS62(f, o, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11,    \
                        _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, \
                        _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, \
                        _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, \
                        _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, \
                        _56, _57, _58, _59, _60, _61, _62)                     \
  f(CONCAT_MEMBER(o, _1), CONCAT_MEMBER(o, _2), CONCAT_MEMBER(o, _3),          \
    CONCAT_MEMBER(o, _4), CONCAT_MEMBER(o, _5), CONCAT_MEMBER(o, _6),          \
    CONCAT_MEMBER(o, _7), CONCAT_MEMBER(o, _8), CONCAT_MEMBER(o, _9),          \
    CONCAT_MEMBER(o, _10), CONCAT_MEMBER(o, _11), CONCAT_MEMBER(o, _12),       \
    CONCAT_MEMBER(o, _13), CONCAT_MEMBER(o, _14), CONCAT_MEMBER(o, _15),       \
    CONCAT_MEMBER(o, _16), CONCAT_MEMBER(o, _17), CONCAT_MEMBER(o, _18),       \
    CONCAT_MEMBER(o, _19), CONCAT_MEMBER(o, _20), CONCAT_MEMBER(o, _21),       \
    CONCAT_MEMBER(o, _22), CONCAT_MEMBER(o, _23), CONCAT_MEMBER(o, _24),       \
    CONCAT_MEMBER(o, _25), CONCAT_MEMBER(o, _26), CONCAT_MEMBER(o, _27),       \
    CONCAT_MEMBER(o, _28), CONCAT_MEMBER(o, _29), CONCAT_MEMBER(o, _30),       \
    CONCAT_MEMBER(o, _31), CONCAT_MEMBER(o, _32), CONCAT_MEMBER(o, _33),       \
    CONCAT_MEMBER(o, _34), CONCAT_MEMBER(o, _35), CONCAT_MEMBER(o, _36),       \
    CONCAT_MEMBER(o, _37), CONCAT_MEMBER(o, _38), CONCAT_MEMBER(o, _39),       \
    CONCAT_MEMBER(o, _40), CONCAT_MEMBER(o, _41), CONCAT_MEMBER(o, _42),       \
    CONCAT_MEMBER(o, _43), CONCAT_MEMBER(o, _44), CONCAT_MEMBER(o, _45),       \
    CONCAT_MEMBER(o, _46), CONCAT_MEMBER(o, _47), CONCAT_MEMBER(o, _48),       \
    CONCAT_MEMBER(o, _49), CONCAT_MEMBER(o, _50), CONCAT_MEMBER(o, _51),       \
    CONCAT_MEMBER(o, _52), CONCAT_MEMBER(o, _53), CONCAT_MEMBER(o, _54),       \
    CONCAT_MEMBER(o, _55), CONCAT_MEMBER(o, _56), CONCAT_MEMBER(o, _57),       \
    CONCAT_MEMBER(o, _58), CONCAT_MEMBER(o, _59), CONCAT_MEMBER(o, _60),       \
    CONCAT_MEMBER(o, _61), CONCAT_MEMBER(o, _62))
#define YLT_CALL_ARGS63(f, o, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11,    \
                        _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, \
                        _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, \
                        _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, \
                        _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, \
                        _56, _57, _58, _59, _60, _61, _62, _63)                \
  f(CONCAT_MEMBER(o, _1), CONCAT_MEMBER(o, _2), CONCAT_MEMBER(o, _3),          \
    CONCAT_MEMBER(o, _4), CONCAT_MEMBER(o, _5), CONCAT_MEMBER(o, _6),          \
    CONCAT_MEMBER(o, _7), CONCAT_MEMBER(o, _8), CONCAT_MEMBER(o, _9),          \
    CONCAT_MEMBER(o, _10), CONCAT_MEMBER(o, _11), CONCAT_MEMBER(o, _12),       \
    CONCAT_MEMBER(o, _13), CONCAT_MEMBER(o, _14), CONCAT_MEMBER(o, _15),       \
    CONCAT_MEMBER(o, _16), CONCAT_MEMBER(o, _17), CONCAT_MEMBER(o, _18),       \
    CONCAT_MEMBER(o, _19), CONCAT_MEMBER(o, _20), CONCAT_MEMBER(o, _21),       \
    CONCAT_MEMBER(o, _22), CONCAT_MEMBER(o, _23), CONCAT_MEMBER(o, _24),       \
    CONCAT_MEMBER(o, _25), CONCAT_MEMBER(o, _26), CONCAT_MEMBER(o, _27),       \
    CONCAT_MEMBER(o, _28), CONCAT_MEMBER(o, _29), CONCAT_MEMBER(o, _30),       \
    CONCAT_MEMBER(o, _31), CONCAT_MEMBER(o, _32), CONCAT_MEMBER(o, _33),       \
    CONCAT_MEMBER(o, _34), CONCAT_MEMBER(o, _35), CONCAT_MEMBER(o, _36),       \
    CONCAT_MEMBER(o, _37), CONCAT_MEMBER(o, _38), CONCAT_MEMBER(o, _39),       \
    CONCAT_MEMBER(o, _40), CONCAT_MEMBER(o, _41), CONCAT_MEMBER(o, _42),       \
    CONCAT_MEMBER(o, _43), CONCAT_MEMBER(o, _44), CONCAT_MEMBER(o, _45),       \
    CONCAT_MEMBER(o, _46), CONCAT_MEMBER(o, _47), CONCAT_MEMBER(o, _48),       \
    CONCAT_MEMBER(o, _49), CONCAT_MEMBER(o, _50), CONCAT_MEMBER(o, _51),       \
    CONCAT_MEMBER(o, _52), CONCAT_MEMBER(o, _53), CONCAT_MEMBER(o, _54),       \
    CONCAT_MEMBER(o, _55), CONCAT_MEMBER(o, _56), CONCAT_MEMBER(o, _57),       \
    CONCAT_MEMBER(o, _58), CONCAT_MEMBER(o, _59), CONCAT_MEMBER(o, _60),       \
    CONCAT_MEMBER(o, _61), CONCAT_MEMBER(o, _62), CONCAT_MEMBER(o, _63))
#define YLT_CALL_ARGS64(f, o, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11,    \
                        _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, \
                        _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, \
                        _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, \
                        _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, \
                        _56, _57, _58, _59, _60, _61, _62, _63, _64)           \
  f(CONCAT_MEMBER(o, _1), CONCAT_MEMBER(o, _2), CONCAT_MEMBER(o, _3),          \
    CONCAT_MEMBER(o, _4), CONCAT_MEMBER(o, _5), CONCAT_MEMBER(o, _6),          \
    CONCAT_MEMBER(o, _7), CONCAT_MEMBER(o, _8), CONCAT_MEMBER(o, _9),          \
    CONCAT_MEMBER(o, _10), CONCAT_MEMBER(o, _11), CONCAT_MEMBER(o, _12),       \
    CONCAT_MEMBER(o, _13), CONCAT_MEMBER(o, _14), CONCAT_MEMBER(o, _15),       \
    CONCAT_MEMBER(o, _16), CONCAT_MEMBER(o, _17), CONCAT_MEMBER(o, _18),       \
    CONCAT_MEMBER(o, _19), CONCAT_MEMBER(o, _20), CONCAT_MEMBER(o, _21),       \
    CONCAT_MEMBER(o, _22), CONCAT_MEMBER(o, _23), CONCAT_MEMBER(o, _24),       \
    CONCAT_MEMBER(o, _25), CONCAT_MEMBER(o, _26), CONCAT_MEMBER(o, _27),       \
    CONCAT_MEMBER(o, _28), CONCAT_MEMBER(o, _29), CONCAT_MEMBER(o, _30),       \
    CONCAT_MEMBER(o, _31), CONCAT_MEMBER(o, _32), CONCAT_MEMBER(o, _33),       \
    CONCAT_MEMBER(o, _34), CONCAT_MEMBER(o, _35), CONCAT_MEMBER(o, _36),       \
    CONCAT_MEMBER(o, _37), CONCAT_MEMBER(o, _38), CONCAT_MEMBER(o, _39),       \
    CONCAT_MEMBER(o, _40), CONCAT_MEMBER(o, _41), CONCAT_MEMBER(o, _42),       \
    CONCAT_MEMBER(o, _43), CONCAT_MEMBER(o, _44), CONCAT_MEMBER(o, _45),       \
    CONCAT_MEMBER(o, _46), CONCAT_MEMBER(o, _47), CONCAT_MEMBER(o, _48),       \
    CONCAT_MEMBER(o, _49), CONCAT_MEMBER(o, _50), CONCAT_MEMBER(o, _51),       \
    CONCAT_MEMBER(o, _52), CONCAT_MEMBER(o, _53), CONCAT_MEMBER(o, _54),       \
    CONCAT_MEMBER(o, _55), CONCAT_MEMBER(o, _56), CONCAT_MEMBER(o, _57),       \
    CONCAT_MEMBER(o, _58), CONCAT_MEMBER(o, _59), CONCAT_MEMBER(o, _60),       \
    CONCAT_MEMBER(o, _61), CONCAT_MEMBER(o, _62), CONCAT_MEMBER(o, _63),       \
    CONCAT_MEMBER(o, _64))
#define YLT_CALL_ARGS65(f, o, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11,    \
                        _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, \
                        _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, \
                        _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, \
                        _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, \
                        _56, _57, _58, _59, _60, _61, _62, _63, _64, _65)      \
  f(CONCAT_MEMBER(o, _1), CONCAT_MEMBER(o, _2), CONCAT_MEMBER(o, _3),          \
    CONCAT_MEMBER(o, _4), CONCAT_MEMBER(o, _5), CONCAT_MEMBER(o, _6),          \
    CONCAT_MEMBER(o, _7), CONCAT_MEMBER(o, _8), CONCAT_MEMBER(o, _9),          \
    CONCAT_MEMBER(o, _10), CONCAT_MEMBER(o, _11), CONCAT_MEMBER(o, _12),       \
    CONCAT_MEMBER(o, _13), CONCAT_MEMBER(o, _14), CONCAT_MEMBER(o, _15),       \
    CONCAT_MEMBER(o, _16), CONCAT_MEMBER(o, _17), CONCAT_MEMBER(o, _18),       \
    CONCAT_MEMBER(o, _19), CONCAT_MEMBER(o, _20), CONCAT_MEMBER(o, _21),       \
    CONCAT_MEMBER(o, _22), CONCAT_MEMBER(o, _23), CONCAT_MEMBER(o, _24),       \
    CONCAT_MEMBER(o, _25), CONCAT_MEMBER(o, _26), CONCAT_MEMBER(o, _27),       \
    CONCAT_MEMBER(o, _28), CONCAT_MEMBER(o, _29), CONCAT_MEMBER(o, _30),       \
    CONCAT_MEMBER(o, _31), CONCAT_MEMBER(o, _32), CONCAT_MEMBER(o, _33),       \
    CONCAT_MEMBER(o, _34), CONCAT_MEMBER(o, _35), CONCAT_MEMBER(o, _36),       \
    CONCAT_MEMBER(o, _37), CONCAT_MEMBER(o, _38), CONCAT_MEMBER(o, _39),       \
    CONCAT_MEMBER(o, _40), CONCAT_MEMBER(o, _41), CONCAT_MEMBER(o, _42),       \
    CONCAT_MEMBER(o, _43), CONCAT_MEMBER(o, _44), CONCAT_MEMBER(o, _45),       \
    CONCAT_MEMBER(o, _46), CONCAT_MEMBER(o, _47), CONCAT_MEMBER(o, _48),       \
    CONCAT_MEMBER(o, _49), CONCAT_MEMBER(o, _50), CONCAT_MEMBER(o, _51),       \
    CONCAT_MEMBER(o, _52), CONCAT_MEMBER(o, _53), CONCAT_MEMBER(o, _54),       \
    CONCAT_MEMBER(o, _55), CONCAT_MEMBER(o, _56), CONCAT_MEMBER(o, _57),       \
    CONCAT_MEMBER(o, _58), CONCAT_MEMBER(o, _59), CONCAT_MEMBER(o, _60),       \
    CONCAT_MEMBER(o, _61), CONCAT_MEMBER(o, _62), CONCAT_MEMBER(o, _63),       \
    CONCAT_MEMBER(o, _64), CONCAT_MEMBER(o, _65))
#define YLT_CALL_ARGS66(f, o, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11,    \
                        _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, \
                        _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, \
                        _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, \
                        _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, \
                        _56, _57, _58, _59, _60, _61, _62, _63, _64, _65, _66) \
  f(CONCAT_MEMBER(o, _1), CONCAT_MEMBER(o, _2), CONCAT_MEMBER(o, _3),          \
    CONCAT_MEMBER(o, _4), CONCAT_MEMBER(o, _5), CONCAT_MEMBER(o, _6),          \
    CONCAT_MEMBER(o, _7), CONCAT_MEMBER(o, _8), CONCAT_MEMBER(o, _9),          \
    CONCAT_MEMBER(o, _10), CONCAT_MEMBER(o, _11), CONCAT_MEMBER(o, _12),       \
    CONCAT_MEMBER(o, _13), CONCAT_MEMBER(o, _14), CONCAT_MEMBER(o, _15),       \
    CONCAT_MEMBER(o, _16), CONCAT_MEMBER(o, _17), CONCAT_MEMBER(o, _18),       \
    CONCAT_MEMBER(o, _19), CONCAT_MEMBER(o, _20), CONCAT_MEMBER(o, _21),       \
    CONCAT_MEMBER(o, _22), CONCAT_MEMBER(o, _23), CONCAT_MEMBER(o, _24),       \
    CONCAT_MEMBER(o, _25), CONCAT_MEMBER(o, _26), CONCAT_MEMBER(o, _27),       \
    CONCAT_MEMBER(o, _28), CONCAT_MEMBER(o, _29), CONCAT_MEMBER(o, _30),       \
    CONCAT_MEMBER(o, _31), CONCAT_MEMBER(o, _32), CONCAT_MEMBER(o, _33),       \
    CONCAT_MEMBER(o, _34), CONCAT_MEMBER(o, _35), CONCAT_MEMBER(o, _36),       \
    CONCAT_MEMBER(o, _37), CONCAT_MEMBER(o, _38), CONCAT_MEMBER(o, _39),       \
    CONCAT_MEMBER(o, _40), CONCAT_MEMBER(o, _41), CONCAT_MEMBER(o, _42),       \
    CONCAT_MEMBER(o, _43), CONCAT_MEMBER(o, _44), CONCAT_MEMBER(o, _45),       \
    CONCAT_MEMBER(o, _46), CONCAT_MEMBER(o, _47), CONCAT_MEMBER(o, _48),       \
    CONCAT_MEMBER(o, _49), CONCAT_MEMBER(o, _50), CONCAT_MEMBER(o, _51),       \
    CONCAT_MEMBER(o, _52), CONCAT_MEMBER(o, _53), CONCAT_MEMBER(o, _54),       \
    CONCAT_MEMBER(o, _55), CONCAT_MEMBER(o, _56), CONCAT_MEMBER(o, _57),       \
    CONCAT_MEMBER(o, _58), CONCAT_MEMBER(o, _59), CONCAT_MEMBER(o, _60),       \
    CONCAT_MEMBER(o, _61), CONCAT_MEMBER(o, _62), CONCAT_MEMBER(o, _63),       \
    CONCAT_MEMBER(o, _64), CONCAT_MEMBER(o, _65), CONCAT_MEMBER(o, _66))
#define YLT_CALL_ARGS67(                                                       \
    f, o, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15,    \
    _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, \
    _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, \
    _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, \
    _61, _62, _63, _64, _65, _66, _67)                                         \
  f(CONCAT_MEMBER(o, _1), CONCAT_MEMBER(o, _2), CONCAT_MEMBER(o, _3),          \
    CONCAT_MEMBER(o, _4), CONCAT_MEMBER(o, _5), CONCAT_MEMBER(o, _6),          \
    CONCAT_MEMBER(o, _7), CONCAT_MEMBER(o, _8), CONCAT_MEMBER(o, _9),          \
    CONCAT_MEMBER(o, _10), CONCAT_MEMBER(o, _11), CONCAT_MEMBER(o, _12),       \
    CONCAT_MEMBER(o, _13), CONCAT_MEMBER(o, _14), CONCAT_MEMBER(o, _15),       \
    CONCAT_MEMBER(o, _16), CONCAT_MEMBER(o, _17), CONCAT_MEMBER(o, _18),       \
    CONCAT_MEMBER(o, _19), CONCAT_MEMBER(o, _20), CONCAT_MEMBER(o, _21),       \
    CONCAT_MEMBER(o, _22), CONCAT_MEMBER(o, _23), CONCAT_MEMBER(o, _24),       \
    CONCAT_MEMBER(o, _25), CONCAT_MEMBER(o, _26), CONCAT_MEMBER(o, _27),       \
    CONCAT_MEMBER(o, _28), CONCAT_MEMBER(o, _29), CONCAT_MEMBER(o, _30),       \
    CONCAT_MEMBER(o, _31), CONCAT_MEMBER(o, _32), CONCAT_MEMBER(o, _33),       \
    CONCAT_MEMBER(o, _34), CONCAT_MEMBER(o, _35), CONCAT_MEMBER(o, _36),       \
    CONCAT_MEMBER(o, _37), CONCAT_MEMBER(o, _38), CONCAT_MEMBER(o, _39),       \
    CONCAT_MEMBER(o, _40), CONCAT_MEMBER(o, _41), CONCAT_MEMBER(o, _42),       \
    CONCAT_MEMBER(o, _43), CONCAT_MEMBER(o, _44), CONCAT_MEMBER(o, _45),       \
    CONCAT_MEMBER(o, _46), CONCAT_MEMBER(o, _47), CONCAT_MEMBER(o, _48),       \
    CONCAT_MEMBER(o, _49), CONCAT_MEMBER(o, _50), CONCAT_MEMBER(o, _51),       \
    CONCAT_MEMBER(o, _52), CONCAT_MEMBER(o, _53), CONCAT_MEMBER(o, _54),       \
    CONCAT_MEMBER(o, _55), CONCAT_MEMBER(o, _56), CONCAT_MEMBER(o, _57),       \
    CONCAT_MEMBER(o, _58), CONCAT_MEMBER(o, _59), CONCAT_MEMBER(o, _60),       \
    CONCAT_MEMBER(o, _61), CONCAT_MEMBER(o, _62), CONCAT_MEMBER(o, _63),       \
    CONCAT_MEMBER(o, _64), CONCAT_MEMBER(o, _65), CONCAT_MEMBER(o, _66),       \
    CONCAT_MEMBER(o, _67))
#define YLT_CALL_ARGS68(                                                       \
    f, o, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15,    \
    _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, \
    _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, \
    _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, \
    _61, _62, _63, _64, _65, _66, _67, _68)                                    \
  f(CONCAT_MEMBER(o, _1), CONCAT_MEMBER(o, _2), CONCAT_MEMBER(o, _3),          \
    CONCAT_MEMBER(o, _4), CONCAT_MEMBER(o, _5), CONCAT_MEMBER(o, _6),          \
    CONCAT_MEMBER(o, _7), CONCAT_MEMBER(o, _8), CONCAT_MEMBER(o, _9),          \
    CONCAT_MEMBER(o, _10), CONCAT_MEMBER(o, _11), CONCAT_MEMBER(o, _12),       \
    CONCAT_MEMBER(o, _13), CONCAT_MEMBER(o, _14), CONCAT_MEMBER(o, _15),       \
    CONCAT_MEMBER(o, _16), CONCAT_MEMBER(o, _17), CONCAT_MEMBER(o, _18),       \
    CONCAT_MEMBER(o, _19), CONCAT_MEMBER(o, _20), CONCAT_MEMBER(o, _21),       \
    CONCAT_MEMBER(o, _22), CONCAT_MEMBER(o, _23), CONCAT_MEMBER(o, _24),       \
    CONCAT_MEMBER(o, _25), CONCAT_MEMBER(o, _26), CONCAT_MEMBER(o, _27),       \
    CONCAT_MEMBER(o, _28), CONCAT_MEMBER(o, _29), CONCAT_MEMBER(o, _30),       \
    CONCAT_MEMBER(o, _31), CONCAT_MEMBER(o, _32), CONCAT_MEMBER(o, _33),       \
    CONCAT_MEMBER(o, _34), CONCAT_MEMBER(o, _35), CONCAT_MEMBER(o, _36),       \
    CONCAT_MEMBER(o, _37), CONCAT_MEMBER(o, _38), CONCAT_MEMBER(o, _39),       \
    CONCAT_MEMBER(o, _40), CONCAT_MEMBER(o, _41), CONCAT_MEMBER(o, _42),       \
    CONCAT_MEMBER(o, _43), CONCAT_MEMBER(o, _44), CONCAT_MEMBER(o, _45),       \
    CONCAT_MEMBER(o, _46), CONCAT_MEMBER(o, _47), CONCAT_MEMBER(o, _48),       \
    CONCAT_MEMBER(o, _49), CONCAT_MEMBER(o, _50), CONCAT_MEMBER(o, _51),       \
    CONCAT_MEMBER(o, _52), CONCAT_MEMBER(o, _53), CONCAT_MEMBER(o, _54),       \
    CONCAT_MEMBER(o, _55), CONCAT_MEMBER(o, _56), CONCAT_MEMBER(o, _57),       \
    CONCAT_MEMBER(o, _58), CONCAT_MEMBER(o, _59), CONCAT_MEMBER(o, _60),       \
    CONCAT_MEMBER(o, _61), CONCAT_MEMBER(o, _62), CONCAT_MEMBER(o, _63),       \
    CONCAT_MEMBER(o, _64), CONCAT_MEMBER(o, _65), CONCAT_MEMBER(o, _66),       \
    CONCAT_MEMBER(o, _67), CONCAT_MEMBER(o, _68))
#define YLT_CALL_ARGS69(                                                       \
    f, o, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15,    \
    _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, \
    _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, \
    _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, \
    _61, _62, _63, _64, _65, _66, _67, _68, _69)                               \
  f(CONCAT_MEMBER(o, _1), CONCAT_MEMBER(o, _2), CONCAT_MEMBER(o, _3),          \
    CONCAT_MEMBER(o, _4), CONCAT_MEMBER(o, _5), CONCAT_MEMBER(o, _6),          \
    CONCAT_MEMBER(o, _7), CONCAT_MEMBER(o, _8), CONCAT_MEMBER(o, _9),          \
    CONCAT_MEMBER(o, _10), CONCAT_MEMBER(o, _11), CONCAT_MEMBER(o, _12),       \
    CONCAT_MEMBER(o, _13), CONCAT_MEMBER(o, _14), CONCAT_MEMBER(o, _15),       \
    CONCAT_MEMBER(o, _16), CONCAT_MEMBER(o, _17), CONCAT_MEMBER(o, _18),       \
    CONCAT_MEMBER(o, _19), CONCAT_MEMBER(o, _20), CONCAT_MEMBER(o, _21),       \
    CONCAT_MEMBER(o, _22), CONCAT_MEMBER(o, _23), CONCAT_MEMBER(o, _24),       \
    CONCAT_MEMBER(o, _25), CONCAT_MEMBER(o, _26), CONCAT_MEMBER(o, _27),       \
    CONCAT_MEMBER(o, _28), CONCAT_MEMBER(o, _29), CONCAT_MEMBER(o, _30),       \
    CONCAT_MEMBER(o, _31), CONCAT_MEMBER(o, _32), CONCAT_MEMBER(o, _33),       \
    CONCAT_MEMBER(o, _34), CONCAT_MEMBER(o, _35), CONCAT_MEMBER(o, _36),       \
    CONCAT_MEMBER(o, _37), CONCAT_MEMBER(o, _38), CONCAT_MEMBER(o, _39),       \
    CONCAT_MEMBER(o, _40), CONCAT_MEMBER(o, _41), CONCAT_MEMBER(o, _42),       \
    CONCAT_MEMBER(o, _43), CONCAT_MEMBER(o, _44), CONCAT_MEMBER(o, _45),       \
    CONCAT_MEMBER(o, _46), CONCAT_MEMBER(o, _47), CONCAT_MEMBER(o, _48),       \
    CONCAT_MEMBER(o, _49), CONCAT_MEMBER(o, _50), CONCAT_MEMBER(o, _51),       \
    CONCAT_MEMBER(o, _52), CONCAT_MEMBER(o, _53), CONCAT_MEMBER(o, _54),       \
    CONCAT_MEMBER(o, _55), CONCAT_MEMBER(o, _56), CONCAT_MEMBER(o, _57),       \
    CONCAT_MEMBER(o, _58), CONCAT_MEMBER(o, _59), CONCAT_MEMBER(o, _60),       \
    CONCAT_MEMBER(o, _61), CONCAT_MEMBER(o, _62), CONCAT_MEMBER(o, _63),       \
    CONCAT_MEMBER(o, _64), CONCAT_MEMBER(o, _65), CONCAT_MEMBER(o, _66),       \
    CONCAT_MEMBER(o, _67), CONCAT_MEMBER(o, _68), CONCAT_MEMBER(o, _69))
#define YLT_CALL_ARGS70(                                                       \
    f, o, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15,    \
    _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, \
    _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, \
    _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, \
    _61, _62, _63, _64, _65, _66, _67, _68, _69, _70)                          \
  f(CONCAT_MEMBER(o, _1), CONCAT_MEMBER(o, _2), CONCAT_MEMBER(o, _3),          \
    CONCAT_MEMBER(o, _4), CONCAT_MEMBER(o, _5), CONCAT_MEMBER(o, _6),          \
    CONCAT_MEMBER(o, _7), CONCAT_MEMBER(o, _8), CONCAT_MEMBER(o, _9),          \
    CONCAT_MEMBER(o, _10), CONCAT_MEMBER(o, _11), CONCAT_MEMBER(o, _12),       \
    CONCAT_MEMBER(o, _13), CONCAT_MEMBER(o, _14), CONCAT_MEMBER(o, _15),       \
    CONCAT_MEMBER(o, _16), CONCAT_MEMBER(o, _17), CONCAT_MEMBER(o, _18),       \
    CONCAT_MEMBER(o, _19), CONCAT_MEMBER(o, _20), CONCAT_MEMBER(o, _21),       \
    CONCAT_MEMBER(o, _22), CONCAT_MEMBER(o, _23), CONCAT_MEMBER(o, _24),       \
    CONCAT_MEMBER(o, _25), CONCAT_MEMBER(o, _26), CONCAT_MEMBER(o, _27),       \
    CONCAT_MEMBER(o, _28), CONCAT_MEMBER(o, _29), CONCAT_MEMBER(o, _30),       \
    CONCAT_MEMBER(o, _31), CONCAT_MEMBER(o, _32), CONCAT_MEMBER(o, _33),       \
    CONCAT_MEMBER(o, _34), CONCAT_MEMBER(o, _35), CONCAT_MEMBER(o, _36),       \
    CONCAT_MEMBER(o, _37), CONCAT_MEMBER(o, _38), CONCAT_MEMBER(o, _39),       \
    CONCAT_MEMBER(o, _40), CONCAT_MEMBER(o, _41), CONCAT_MEMBER(o, _42),       \
    CONCAT_MEMBER(o, _43), CONCAT_MEMBER(o, _44), CONCAT_MEMBER(o, _45),       \
    CONCAT_MEMBER(o, _46), CONCAT_MEMBER(o, _47), CONCAT_MEMBER(o, _48),       \
    CONCAT_MEMBER(o, _49), CONCAT_MEMBER(o, _50), CONCAT_MEMBER(o, _51),       \
    CONCAT_MEMBER(o, _52), CONCAT_MEMBER(o, _53), CONCAT_MEMBER(o, _54),       \
    CONCAT_MEMBER(o, _55), CONCAT_MEMBER(o, _56), CONCAT_MEMBER(o, _57),       \
    CONCAT_MEMBER(o, _58), CONCAT_MEMBER(o, _59), CONCAT_MEMBER(o, _60),       \
    CONCAT_MEMBER(o, _61), CONCAT_MEMBER(o, _62), CONCAT_MEMBER(o, _63),       \
    CONCAT_MEMBER(o, _64), CONCAT_MEMBER(o, _65), CONCAT_MEMBER(o, _66),       \
    CONCAT_MEMBER(o, _67), CONCAT_MEMBER(o, _68), CONCAT_MEMBER(o, _69),       \
    CONCAT_MEMBER(o, _70))
#define YLT_CALL_ARGS71(                                                       \
    f, o, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15,    \
    _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, \
    _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, \
    _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, \
    _61, _62, _63, _64, _65, _66, _67, _68, _69, _70, _71)                     \
  f(CONCAT_MEMBER(o, _1), CONCAT_MEMBER(o, _2), CONCAT_MEMBER(o, _3),          \
    CONCAT_MEMBER(o, _4), CONCAT_MEMBER(o, _5), CONCAT_MEMBER(o, _6),          \
    CONCAT_MEMBER(o, _7), CONCAT_MEMBER(o, _8), CONCAT_MEMBER(o, _9),          \
    CONCAT_MEMBER(o, _10), CONCAT_MEMBER(o, _11), CONCAT_MEMBER(o, _12),       \
    CONCAT_MEMBER(o, _13), CONCAT_MEMBER(o, _14), CONCAT_MEMBER(o, _15),       \
    CONCAT_MEMBER(o, _16), CONCAT_MEMBER(o, _17), CONCAT_MEMBER(o, _18),       \
    CONCAT_MEMBER(o, _19), CONCAT_MEMBER(o, _20), CONCAT_MEMBER(o, _21),       \
    CONCAT_MEMBER(o, _22), CONCAT_MEMBER(o, _23), CONCAT_MEMBER(o, _24),       \
    CONCAT_MEMBER(o, _25), CONCAT_MEMBER(o, _26), CONCAT_MEMBER(o, _27),       \
    CONCAT_MEMBER(o, _28), CONCAT_MEMBER(o, _29), CONCAT_MEMBER(o, _30),       \
    CONCAT_MEMBER(o, _31), CONCAT_MEMBER(o, _32), CONCAT_MEMBER(o, _33),       \
    CONCAT_MEMBER(o, _34), CONCAT_MEMBER(o, _35), CONCAT_MEMBER(o, _36),       \
    CONCAT_MEMBER(o, _37), CONCAT_MEMBER(o, _38), CONCAT_MEMBER(o, _39),       \
    CONCAT_MEMBER(o, _40), CONCAT_MEMBER(o, _41), CONCAT_MEMBER(o, _42),       \
    CONCAT_MEMBER(o, _43), CONCAT_MEMBER(o, _44), CONCAT_MEMBER(o, _45),       \
    CONCAT_MEMBER(o, _46), CONCAT_MEMBER(o, _47), CONCAT_MEMBER(o, _48),       \
    CONCAT_MEMBER(o, _49), CONCAT_MEMBER(o, _50), CONCAT_MEMBER(o, _51),       \
    CONCAT_MEMBER(o, _52), CONCAT_MEMBER(o, _53), CONCAT_MEMBER(o, _54),       \
    CONCAT_MEMBER(o, _55), CONCAT_MEMBER(o, _56), CONCAT_MEMBER(o, _57),       \
    CONCAT_MEMBER(o, _58), CONCAT_MEMBER(o, _59), CONCAT_MEMBER(o, _60),       \
    CONCAT_MEMBER(o, _61), CONCAT_MEMBER(o, _62), CONCAT_MEMBER(o, _63),       \
    CONCAT_MEMBER(o, _64), CONCAT_MEMBER(o, _65), CONCAT_MEMBER(o, _66),       \
    CONCAT_MEMBER(o, _67), CONCAT_MEMBER(o, _68), CONCAT_MEMBER(o, _69),       \
    CONCAT_MEMBER(o, _70), CONCAT_MEMBER(o, _71))
#define YLT_CALL_ARGS72(                                                       \
    f, o, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15,    \
    _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, \
    _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, \
    _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, \
    _61, _62, _63, _64, _65, _66, _67, _68, _69, _70, _71, _72)                \
  f(CONCAT_MEMBER(o, _1), CONCAT_MEMBER(o, _2), CONCAT_MEMBER(o, _3),          \
    CONCAT_MEMBER(o, _4), CONCAT_MEMBER(o, _5), CONCAT_MEMBER(o, _6),          \
    CONCAT_MEMBER(o, _7), CONCAT_MEMBER(o, _8), CONCAT_MEMBER(o, _9),          \
    CONCAT_MEMBER(o, _10), CONCAT_MEMBER(o, _11), CONCAT_MEMBER(o, _12),       \
    CONCAT_MEMBER(o, _13), CONCAT_MEMBER(o, _14), CONCAT_MEMBER(o, _15),       \
    CONCAT_MEMBER(o, _16), CONCAT_MEMBER(o, _17), CONCAT_MEMBER(o, _18),       \
    CONCAT_MEMBER(o, _19), CONCAT_MEMBER(o, _20), CONCAT_MEMBER(o, _21),       \
    CONCAT_MEMBER(o, _22), CONCAT_MEMBER(o, _23), CONCAT_MEMBER(o, _24),       \
    CONCAT_MEMBER(o, _25), CONCAT_MEMBER(o, _26), CONCAT_MEMBER(o, _27),       \
    CONCAT_MEMBER(o, _28), CONCAT_MEMBER(o, _29), CONCAT_MEMBER(o, _30),       \
    CONCAT_MEMBER(o, _31), CONCAT_MEMBER(o, _32), CONCAT_MEMBER(o, _33),       \
    CONCAT_MEMBER(o, _34), CONCAT_MEMBER(o, _35), CONCAT_MEMBER(o, _36),       \
    CONCAT_MEMBER(o, _37), CONCAT_MEMBER(o, _38), CONCAT_MEMBER(o, _39),       \
    CONCAT_MEMBER(o, _40), CONCAT_MEMBER(o, _41), CONCAT_MEMBER(o, _42),       \
    CONCAT_MEMBER(o, _43), CONCAT_MEMBER(o, _44), CONCAT_MEMBER(o, _45),       \
    CONCAT_MEMBER(o, _46), CONCAT_MEMBER(o, _47), CONCAT_MEMBER(o, _48),       \
    CONCAT_MEMBER(o, _49), CONCAT_MEMBER(o, _50), CONCAT_MEMBER(o, _51),       \
    CONCAT_MEMBER(o, _52), CONCAT_MEMBER(o, _53), CONCAT_MEMBER(o, _54),       \
    CONCAT_MEMBER(o, _55), CONCAT_MEMBER(o, _56), CONCAT_MEMBER(o, _57),       \
    CONCAT_MEMBER(o, _58), CONCAT_MEMBER(o, _59), CONCAT_MEMBER(o, _60),       \
    CONCAT_MEMBER(o, _61), CONCAT_MEMBER(o, _62), CONCAT_MEMBER(o, _63),       \
    CONCAT_MEMBER(o, _64), CONCAT_MEMBER(o, _65), CONCAT_MEMBER(o, _66),       \
    CONCAT_MEMBER(o, _67), CONCAT_MEMBER(o, _68), CONCAT_MEMBER(o, _69),       \
    CONCAT_MEMBER(o, _70), CONCAT_MEMBER(o, _71), CONCAT_MEMBER(o, _72))
#define YLT_CALL_ARGS73(                                                       \
    f, o, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15,    \
    _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, \
    _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, \
    _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, \
    _61, _62, _63, _64, _65, _66, _67, _68, _69, _70, _71, _72, _73)           \
  f(CONCAT_MEMBER(o, _1), CONCAT_MEMBER(o, _2), CONCAT_MEMBER(o, _3),          \
    CONCAT_MEMBER(o, _4), CONCAT_MEMBER(o, _5), CONCAT_MEMBER(o, _6),          \
    CONCAT_MEMBER(o, _7), CONCAT_MEMBER(o, _8), CONCAT_MEMBER(o, _9),          \
    CONCAT_MEMBER(o, _10), CONCAT_MEMBER(o, _11), CONCAT_MEMBER(o, _12),       \
    CONCAT_MEMBER(o, _13), CONCAT_MEMBER(o, _14), CONCAT_MEMBER(o, _15),       \
    CONCAT_MEMBER(o, _16), CONCAT_MEMBER(o, _17), CONCAT_MEMBER(o, _18),       \
    CONCAT_MEMBER(o, _19), CONCAT_MEMBER(o, _20), CONCAT_MEMBER(o, _21),       \
    CONCAT_MEMBER(o, _22), CONCAT_MEMBER(o, _23), CONCAT_MEMBER(o, _24),       \
    CONCAT_MEMBER(o, _25), CONCAT_MEMBER(o, _26), CONCAT_MEMBER(o, _27),       \
    CONCAT_MEMBER(o, _28), CONCAT_MEMBER(o, _29), CONCAT_MEMBER(o, _30),       \
    CONCAT_MEMBER(o, _31), CONCAT_MEMBER(o, _32), CONCAT_MEMBER(o, _33),       \
    CONCAT_MEMBER(o, _34), CONCAT_MEMBER(o, _35), CONCAT_MEMBER(o, _36),       \
    CONCAT_MEMBER(o, _37), CONCAT_MEMBER(o, _38), CONCAT_MEMBER(o, _39),       \
    CONCAT_MEMBER(o, _40), CONCAT_MEMBER(o, _41), CONCAT_MEMBER(o, _42),       \
    CONCAT_MEMBER(o, _43), CONCAT_MEMBER(o, _44), CONCAT_MEMBER(o, _45),       \
    CONCAT_MEMBER(o, _46), CONCAT_MEMBER(o, _47), CONCAT_MEMBER(o, _48),       \
    CONCAT_MEMBER(o, _49), CONCAT_MEMBER(o, _50), CONCAT_MEMBER(o, _51),       \
    CONCAT_MEMBER(o, _52), CONCAT_MEMBER(o, _53), CONCAT_MEMBER(o, _54),       \
    CONCAT_MEMBER(o, _55), CONCAT_MEMBER(o, _56), CONCAT_MEMBER(o, _57),       \
    CONCAT_MEMBER(o, _58), CONCAT_MEMBER(o, _59), CONCAT_MEMBER(o, _60),       \
    CONCAT_MEMBER(o, _61), CONCAT_MEMBER(o, _62), CONCAT_MEMBER(o, _63),       \
    CONCAT_MEMBER(o, _64), CONCAT_MEMBER(o, _65), CONCAT_MEMBER(o, _66),       \
    CONCAT_MEMBER(o, _67), CONCAT_MEMBER(o, _68), CONCAT_MEMBER(o, _69),       \
    CONCAT_MEMBER(o, _70), CONCAT_MEMBER(o, _71), CONCAT_MEMBER(o, _72),       \
    CONCAT_MEMBER(o, _73))
#define YLT_CALL_ARGS74(                                                       \
    f, o, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15,    \
    _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, \
    _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, \
    _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, \
    _61, _62, _63, _64, _65, _66, _67, _68, _69, _70, _71, _72, _73, _74)      \
  f(CONCAT_MEMBER(o, _1), CONCAT_MEMBER(o, _2), CONCAT_MEMBER(o, _3),          \
    CONCAT_MEMBER(o, _4), CONCAT_MEMBER(o, _5), CONCAT_MEMBER(o, _6),          \
    CONCAT_MEMBER(o, _7), CONCAT_MEMBER(o, _8), CONCAT_MEMBER(o, _9),          \
    CONCAT_MEMBER(o, _10), CONCAT_MEMBER(o, _11), CONCAT_MEMBER(o, _12),       \
    CONCAT_MEMBER(o, _13), CONCAT_MEMBER(o, _14), CONCAT_MEMBER(o, _15),       \
    CONCAT_MEMBER(o, _16), CONCAT_MEMBER(o, _17), CONCAT_MEMBER(o, _18),       \
    CONCAT_MEMBER(o, _19), CONCAT_MEMBER(o, _20), CONCAT_MEMBER(o, _21),       \
    CONCAT_MEMBER(o, _22), CONCAT_MEMBER(o, _23), CONCAT_MEMBER(o, _24),       \
    CONCAT_MEMBER(o, _25), CONCAT_MEMBER(o, _26), CONCAT_MEMBER(o, _27),       \
    CONCAT_MEMBER(o, _28), CONCAT_MEMBER(o, _29), CONCAT_MEMBER(o, _30),       \
    CONCAT_MEMBER(o, _31), CONCAT_MEMBER(o, _32), CONCAT_MEMBER(o, _33),       \
    CONCAT_MEMBER(o, _34), CONCAT_MEMBER(o, _35), CONCAT_MEMBER(o, _36),       \
    CONCAT_MEMBER(o, _37), CONCAT_MEMBER(o, _38), CONCAT_MEMBER(o, _39),       \
    CONCAT_MEMBER(o, _40), CONCAT_MEMBER(o, _41), CONCAT_MEMBER(o, _42),       \
    CONCAT_MEMBER(o, _43), CONCAT_MEMBER(o, _44), CONCAT_MEMBER(o, _45),       \
    CONCAT_MEMBER(o, _46), CONCAT_MEMBER(o, _47), CONCAT_MEMBER(o, _48),       \
    CONCAT_MEMBER(o, _49), CONCAT_MEMBER(o, _50), CONCAT_MEMBER(o, _51),       \
    CONCAT_MEMBER(o, _52), CONCAT_MEMBER(o, _53), CONCAT_MEMBER(o, _54),       \
    CONCAT_MEMBER(o, _55), CONCAT_MEMBER(o, _56), CONCAT_MEMBER(o, _57),       \
    CONCAT_MEMBER(o, _58), CONCAT_MEMBER(o, _59), CONCAT_MEMBER(o, _60),       \
    CONCAT_MEMBER(o, _61), CONCAT_MEMBER(o, _62), CONCAT_MEMBER(o, _63),       \
    CONCAT_MEMBER(o, _64), CONCAT_MEMBER(o, _65), CONCAT_MEMBER(o, _66),       \
    CONCAT_MEMBER(o, _67), CONCAT_MEMBER(o, _68), CONCAT_MEMBER(o, _69),       \
    CONCAT_MEMBER(o, _70), CONCAT_MEMBER(o, _71), CONCAT_MEMBER(o, _72),       \
    CONCAT_MEMBER(o, _73), CONCAT_MEMBER(o, _74))
#define YLT_CALL_ARGS75(                                                       \
    f, o, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15,    \
    _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, \
    _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, \
    _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, \
    _61, _62, _63, _64, _65, _66, _67, _68, _69, _70, _71, _72, _73, _74, _75) \
  f(CONCAT_MEMBER(o, _1), CONCAT_MEMBER(o, _2), CONCAT_MEMBER(o, _3),          \
    CONCAT_MEMBER(o, _4), CONCAT_MEMBER(o, _5), CONCAT_MEMBER(o, _6),          \
    CONCAT_MEMBER(o, _7), CONCAT_MEMBER(o, _8), CONCAT_MEMBER(o, _9),          \
    CONCAT_MEMBER(o, _10), CONCAT_MEMBER(o, _11), CONCAT_MEMBER(o, _12),       \
    CONCAT_MEMBER(o, _13), CONCAT_MEMBER(o, _14), CONCAT_MEMBER(o, _15),       \
    CONCAT_MEMBER(o, _16), CONCAT_MEMBER(o, _17), CONCAT_MEMBER(o, _18),       \
    CONCAT_MEMBER(o, _19), CONCAT_MEMBER(o, _20), CONCAT_MEMBER(o, _21),       \
    CONCAT_MEMBER(o, _22), CONCAT_MEMBER(o, _23), CONCAT_MEMBER(o, _24),       \
    CONCAT_MEMBER(o, _25), CONCAT_MEMBER(o, _26), CONCAT_MEMBER(o, _27),       \
    CONCAT_MEMBER(o, _28), CONCAT_MEMBER(o, _29), CONCAT_MEMBER(o, _30),       \
    CONCAT_MEMBER(o, _31), CONCAT_MEMBER(o, _32), CONCAT_MEMBER(o, _33),       \
    CONCAT_MEMBER(o, _34), CONCAT_MEMBER(o, _35), CONCAT_MEMBER(o, _36),       \
    CONCAT_MEMBER(o, _37), CONCAT_MEMBER(o, _38), CONCAT_MEMBER(o, _39),       \
    CONCAT_MEMBER(o, _40), CONCAT_MEMBER(o, _41), CONCAT_MEMBER(o, _42),       \
    CONCAT_MEMBER(o, _43), CONCAT_MEMBER(o, _44), CONCAT_MEMBER(o, _45),       \
    CONCAT_MEMBER(o, _46), CONCAT_MEMBER(o, _47), CONCAT_MEMBER(o, _48),       \
    CONCAT_MEMBER(o, _49), CONCAT_MEMBER(o, _50), CONCAT_MEMBER(o, _51),       \
    CONCAT_MEMBER(o, _52), CONCAT_MEMBER(o, _53), CONCAT_MEMBER(o, _54),       \
    CONCAT_MEMBER(o, _55), CONCAT_MEMBER(o, _56), CONCAT_MEMBER(o, _57),       \
    CONCAT_MEMBER(o, _58), CONCAT_MEMBER(o, _59), CONCAT_MEMBER(o, _60),       \
    CONCAT_MEMBER(o, _61), CONCAT_MEMBER(o, _62), CONCAT_MEMBER(o, _63),       \
    CONCAT_MEMBER(o, _64), CONCAT_MEMBER(o, _65), CONCAT_MEMBER(o, _66),       \
    CONCAT_MEMBER(o, _67), CONCAT_MEMBER(o, _68), CONCAT_MEMBER(o, _69),       \
    CONCAT_MEMBER(o, _70), CONCAT_MEMBER(o, _71), CONCAT_MEMBER(o, _72),       \
    CONCAT_MEMBER(o, _73), CONCAT_MEMBER(o, _74), CONCAT_MEMBER(o, _75))
#define YLT_CALL_ARGS76(f, o, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11,    \
                        _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, \
                        _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, \
                        _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, \
                        _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, \
                        _56, _57, _58, _59, _60, _61, _62, _63, _64, _65, _66, \
                        _67, _68, _69, _70, _71, _72, _73, _74, _75, _76)      \
  f(CONCAT_MEMBER(o, _1), CONCAT_MEMBER(o, _2), CONCAT_MEMBER(o, _3),          \
    CONCAT_MEMBER(o, _4), CONCAT_MEMBER(o, _5), CONCAT_MEMBER(o, _6),          \
    CONCAT_MEMBER(o, _7), CONCAT_MEMBER(o, _8), CONCAT_MEMBER(o, _9),          \
    CONCAT_MEMBER(o, _10), CONCAT_MEMBER(o, _11), CONCAT_MEMBER(o, _12),       \
    CONCAT_MEMBER(o, _13), CONCAT_MEMBER(o, _14), CONCAT_MEMBER(o, _15),       \
    CONCAT_MEMBER(o, _16), CONCAT_MEMBER(o, _17), CONCAT_MEMBER(o, _18),       \
    CONCAT_MEMBER(o, _19), CONCAT_MEMBER(o, _20), CONCAT_MEMBER(o, _21),       \
    CONCAT_MEMBER(o, _22), CONCAT_MEMBER(o, _23), CONCAT_MEMBER(o, _24),       \
    CONCAT_MEMBER(o, _25), CONCAT_MEMBER(o, _26), CONCAT_MEMBER(o, _27),       \
    CONCAT_MEMBER(o, _28), CONCAT_MEMBER(o, _29), CONCAT_MEMBER(o, _30),       \
    CONCAT_MEMBER(o, _31), CONCAT_MEMBER(o, _32), CONCAT_MEMBER(o, _33),       \
    CONCAT_MEMBER(o, _34), CONCAT_MEMBER(o, _35), CONCAT_MEMBER(o, _36),       \
    CONCAT_MEMBER(o, _37), CONCAT_MEMBER(o, _38), CONCAT_MEMBER(o, _39),       \
    CONCAT_MEMBER(o, _40), CONCAT_MEMBER(o, _41), CONCAT_MEMBER(o, _42),       \
    CONCAT_MEMBER(o, _43), CONCAT_MEMBER(o, _44), CONCAT_MEMBER(o, _45),       \
    CONCAT_MEMBER(o, _46), CONCAT_MEMBER(o, _47), CONCAT_MEMBER(o, _48),       \
    CONCAT_MEMBER(o, _49), CONCAT_MEMBER(o, _50), CONCAT_MEMBER(o, _51),       \
    CONCAT_MEMBER(o, _52), CONCAT_MEMBER(o, _53), CONCAT_MEMBER(o, _54),       \
    CONCAT_MEMBER(o, _55), CONCAT_MEMBER(o, _56), CONCAT_MEMBER(o, _57),       \
    CONCAT_MEMBER(o, _58), CONCAT_MEMBER(o, _59), CONCAT_MEMBER(o, _60),       \
    CONCAT_MEMBER(o, _61), CONCAT_MEMBER(o, _62), CONCAT_MEMBER(o, _63),       \
    CONCAT_MEMBER(o, _64), CONCAT_MEMBER(o, _65), CONCAT_MEMBER(o, _66),       \
    CONCAT_MEMBER(o, _67), CONCAT_MEMBER(o, _68), CONCAT_MEMBER(o, _69),       \
    CONCAT_MEMBER(o, _70), CONCAT_MEMBER(o, _71), CONCAT_MEMBER(o, _72),       \
    CONCAT_MEMBER(o, _73), CONCAT_MEMBER(o, _74), CONCAT_MEMBER(o, _75),       \
    CONCAT_MEMBER(o, _76))
#define YLT_CALL_ARGS77(f, o, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11,    \
                        _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, \
                        _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, \
                        _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, \
                        _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, \
                        _56, _57, _58, _59, _60, _61, _62, _63, _64, _65, _66, \
                        _67, _68, _69, _70, _71, _72, _73, _74, _75, _76, _77) \
  f(CONCAT_MEMBER(o, _1), CONCAT_MEMBER(o, _2), CONCAT_MEMBER(o, _3),          \
    CONCAT_MEMBER(o, _4), CONCAT_MEMBER(o, _5), CONCAT_MEMBER(o, _6),          \
    CONCAT_MEMBER(o, _7), CONCAT_MEMBER(o, _8), CONCAT_MEMBER(o, _9),          \
    CONCAT_MEMBER(o, _10), CONCAT_MEMBER(o, _11), CONCAT_MEMBER(o, _12),       \
    CONCAT_MEMBER(o, _13), CONCAT_MEMBER(o, _14), CONCAT_MEMBER(o, _15),       \
    CONCAT_MEMBER(o, _16), CONCAT_MEMBER(o, _17), CONCAT_MEMBER(o, _18),       \
    CONCAT_MEMBER(o, _19), CONCAT_MEMBER(o, _20), CONCAT_MEMBER(o, _21),       \
    CONCAT_MEMBER(o, _22), CONCAT_MEMBER(o, _23), CONCAT_MEMBER(o, _24),       \
    CONCAT_MEMBER(o, _25), CONCAT_MEMBER(o, _26), CONCAT_MEMBER(o, _27),       \
    CONCAT_MEMBER(o, _28), CONCAT_MEMBER(o, _29), CONCAT_MEMBER(o, _30),       \
    CONCAT_MEMBER(o, _31), CONCAT_MEMBER(o, _32), CONCAT_MEMBER(o, _33),       \
    CONCAT_MEMBER(o, _34), CONCAT_MEMBER(o, _35), CONCAT_MEMBER(o, _36),       \
    CONCAT_MEMBER(o, _37), CONCAT_MEMBER(o, _38), CONCAT_MEMBER(o, _39),       \
    CONCAT_MEMBER(o, _40), CONCAT_MEMBER(o, _41), CONCAT_MEMBER(o, _42),       \
    CONCAT_MEMBER(o, _43), CONCAT_MEMBER(o, _44), CONCAT_MEMBER(o, _45),       \
    CONCAT_MEMBER(o, _46), CONCAT_MEMBER(o, _47), CONCAT_MEMBER(o, _48),       \
    CONCAT_MEMBER(o, _49), CONCAT_MEMBER(o, _50), CONCAT_MEMBER(o, _51),       \
    CONCAT_MEMBER(o, _52), CONCAT_MEMBER(o, _53), CONCAT_MEMBER(o, _54),       \
    CONCAT_MEMBER(o, _55), CONCAT_MEMBER(o, _56), CONCAT_MEMBER(o, _57),       \
    CONCAT_MEMBER(o, _58), CONCAT_MEMBER(o, _59), CONCAT_MEMBER(o, _60),       \
    CONCAT_MEMBER(o, _61), CONCAT_MEMBER(o, _62), CONCAT_MEMBER(o, _63),       \
    CONCAT_MEMBER(o, _64), CONCAT_MEMBER(o, _65), CONCAT_MEMBER(o, _66),       \
    CONCAT_MEMBER(o, _67), CONCAT_MEMBER(o, _68), CONCAT_MEMBER(o, _69),       \
    CONCAT_MEMBER(o, _70), CONCAT_MEMBER(o, _71), CONCAT_MEMBER(o, _72),       \
    CONCAT_MEMBER(o, _73), CONCAT_MEMBER(o, _74), CONCAT_MEMBER(o, _75),       \
    CONCAT_MEMBER(o, _76), CONCAT_MEMBER(o, _77))
#define YLT_CALL_ARGS78(                                                       \
    f, o, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15,    \
    _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, \
    _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, \
    _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, \
    _61, _62, _63, _64, _65, _66, _67, _68, _69, _70, _71, _72, _73, _74, _75, \
    _76, _77, _78)                                                             \
  f(CONCAT_MEMBER(o, _1), CONCAT_MEMBER(o, _2), CONCAT_MEMBER(o, _3),          \
    CONCAT_MEMBER(o, _4), CONCAT_MEMBER(o, _5), CONCAT_MEMBER(o, _6),          \
    CONCAT_MEMBER(o, _7), CONCAT_MEMBER(o, _8), CONCAT_MEMBER(o, _9),          \
    CONCAT_MEMBER(o, _10), CONCAT_MEMBER(o, _11), CONCAT_MEMBER(o, _12),       \
    CONCAT_MEMBER(o, _13), CONCAT_MEMBER(o, _14), CONCAT_MEMBER(o, _15),       \
    CONCAT_MEMBER(o, _16), CONCAT_MEMBER(o, _17), CONCAT_MEMBER(o, _18),       \
    CONCAT_MEMBER(o, _19), CONCAT_MEMBER(o, _20), CONCAT_MEMBER(o, _21),       \
    CONCAT_MEMBER(o, _22), CONCAT_MEMBER(o, _23), CONCAT_MEMBER(o, _24),       \
    CONCAT_MEMBER(o, _25), CONCAT_MEMBER(o, _26), CONCAT_MEMBER(o, _27),       \
    CONCAT_MEMBER(o, _28), CONCAT_MEMBER(o, _29), CONCAT_MEMBER(o, _30),       \
    CONCAT_MEMBER(o, _31), CONCAT_MEMBER(o, _32), CONCAT_MEMBER(o, _33),       \
    CONCAT_MEMBER(o, _34), CONCAT_MEMBER(o, _35), CONCAT_MEMBER(o, _36),       \
    CONCAT_MEMBER(o, _37), CONCAT_MEMBER(o, _38), CONCAT_MEMBER(o, _39),       \
    CONCAT_MEMBER(o, _40), CONCAT_MEMBER(o, _41), CONCAT_MEMBER(o, _42),       \
    CONCAT_MEMBER(o, _43), CONCAT_MEMBER(o, _44), CONCAT_MEMBER(o, _45),       \
    CONCAT_MEMBER(o, _46), CONCAT_MEMBER(o, _47), CONCAT_MEMBER(o, _48),       \
    CONCAT_MEMBER(o, _49), CONCAT_MEMBER(o, _50), CONCAT_MEMBER(o, _51),       \
    CONCAT_MEMBER(o, _52), CONCAT_MEMBER(o, _53), CONCAT_MEMBER(o, _54),       \
    CONCAT_MEMBER(o, _55), CONCAT_MEMBER(o, _56), CONCAT_MEMBER(o, _57),       \
    CONCAT_MEMBER(o, _58), CONCAT_MEMBER(o, _59), CONCAT_MEMBER(o, _60),       \
    CONCAT_MEMBER(o, _61), CONCAT_MEMBER(o, _62), CONCAT_MEMBER(o, _63),       \
    CONCAT_MEMBER(o, _64), CONCAT_MEMBER(o, _65), CONCAT_MEMBER(o, _66),       \
    CONCAT_MEMBER(o, _67), CONCAT_MEMBER(o, _68), CONCAT_MEMBER(o, _69),       \
    CONCAT_MEMBER(o, _70), CONCAT_MEMBER(o, _71), CONCAT_MEMBER(o, _72),       \
    CONCAT_MEMBER(o, _73), CONCAT_MEMBER(o, _74), CONCAT_MEMBER(o, _75),       \
    CONCAT_MEMBER(o, _76), CONCAT_MEMBER(o, _77), CONCAT_MEMBER(o, _78))
#define YLT_CALL_ARGS79(                                                       \
    f, o, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15,    \
    _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, \
    _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, \
    _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, \
    _61, _62, _63, _64, _65, _66, _67, _68, _69, _70, _71, _72, _73, _74, _75, \
    _76, _77, _78, _79)                                                        \
  f(CONCAT_MEMBER(o, _1), CONCAT_MEMBER(o, _2), CONCAT_MEMBER(o, _3),          \
    CONCAT_MEMBER(o, _4), CONCAT_MEMBER(o, _5), CONCAT_MEMBER(o, _6),          \
    CONCAT_MEMBER(o, _7), CONCAT_MEMBER(o, _8), CONCAT_MEMBER(o, _9),          \
    CONCAT_MEMBER(o, _10), CONCAT_MEMBER(o, _11), CONCAT_MEMBER(o, _12),       \
    CONCAT_MEMBER(o, _13), CONCAT_MEMBER(o, _14), CONCAT_MEMBER(o, _15),       \
    CONCAT_MEMBER(o, _16), CONCAT_MEMBER(o, _17), CONCAT_MEMBER(o, _18),       \
    CONCAT_MEMBER(o, _19), CONCAT_MEMBER(o, _20), CONCAT_MEMBER(o, _21),       \
    CONCAT_MEMBER(o, _22), CONCAT_MEMBER(o, _23), CONCAT_MEMBER(o, _24),       \
    CONCAT_MEMBER(o, _25), CONCAT_MEMBER(o, _26), CONCAT_MEMBER(o, _27),       \
    CONCAT_MEMBER(o, _28), CONCAT_MEMBER(o, _29), CONCAT_MEMBER(o, _30),       \
    CONCAT_MEMBER(o, _31), CONCAT_MEMBER(o, _32), CONCAT_MEMBER(o, _33),       \
    CONCAT_MEMBER(o, _34), CONCAT_MEMBER(o, _35), CONCAT_MEMBER(o, _36),       \
    CONCAT_MEMBER(o, _37), CONCAT_MEMBER(o, _38), CONCAT_MEMBER(o, _39),       \
    CONCAT_MEMBER(o, _40), CONCAT_MEMBER(o, _41), CONCAT_MEMBER(o, _42),       \
    CONCAT_MEMBER(o, _43), CONCAT_MEMBER(o, _44), CONCAT_MEMBER(o, _45),       \
    CONCAT_MEMBER(o, _46), CONCAT_MEMBER(o, _47), CONCAT_MEMBER(o, _48),       \
    CONCAT_MEMBER(o, _49), CONCAT_MEMBER(o, _50), CONCAT_MEMBER(o, _51),       \
    CONCAT_MEMBER(o, _52), CONCAT_MEMBER(o, _53), CONCAT_MEMBER(o, _54),       \
    CONCAT_MEMBER(o, _55), CONCAT_MEMBER(o, _56), CONCAT_MEMBER(o, _57),       \
    CONCAT_MEMBER(o, _58), CONCAT_MEMBER(o, _59), CONCAT_MEMBER(o, _60),       \
    CONCAT_MEMBER(o, _61), CONCAT_MEMBER(o, _62), CONCAT_MEMBER(o, _63),       \
    CONCAT_MEMBER(o, _64), CONCAT_MEMBER(o, _65), CONCAT_MEMBER(o, _66),       \
    CONCAT_MEMBER(o, _67), CONCAT_MEMBER(o, _68), CONCAT_MEMBER(o, _69),       \
    CONCAT_MEMBER(o, _70), CONCAT_MEMBER(o, _71), CONCAT_MEMBER(o, _72),       \
    CONCAT_MEMBER(o, _73), CONCAT_MEMBER(o, _74), CONCAT_MEMBER(o, _75),       \
    CONCAT_MEMBER(o, _76), CONCAT_MEMBER(o, _77), CONCAT_MEMBER(o, _78),       \
    CONCAT_MEMBER(o, _79))
#define YLT_CALL_ARGS80(                                                       \
    f, o, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15,    \
    _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, \
    _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, \
    _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, \
    _61, _62, _63, _64, _65, _66, _67, _68, _69, _70, _71, _72, _73, _74, _75, \
    _76, _77, _78, _79, _80)                                                   \
  f(CONCAT_MEMBER(o, _1), CONCAT_MEMBER(o, _2), CONCAT_MEMBER(o, _3),          \
    CONCAT_MEMBER(o, _4), CONCAT_MEMBER(o, _5), CONCAT_MEMBER(o, _6),          \
    CONCAT_MEMBER(o, _7), CONCAT_MEMBER(o, _8), CONCAT_MEMBER(o, _9),          \
    CONCAT_MEMBER(o, _10), CONCAT_MEMBER(o, _11), CONCAT_MEMBER(o, _12),       \
    CONCAT_MEMBER(o, _13), CONCAT_MEMBER(o, _14), CONCAT_MEMBER(o, _15),       \
    CONCAT_MEMBER(o, _16), CONCAT_MEMBER(o, _17), CONCAT_MEMBER(o, _18),       \
    CONCAT_MEMBER(o, _19), CONCAT_MEMBER(o, _20), CONCAT_MEMBER(o, _21),       \
    CONCAT_MEMBER(o, _22), CONCAT_MEMBER(o, _23), CONCAT_MEMBER(o, _24),       \
    CONCAT_MEMBER(o, _25), CONCAT_MEMBER(o, _26), CONCAT_MEMBER(o, _27),       \
    CONCAT_MEMBER(o, _28), CONCAT_MEMBER(o, _29), CONCAT_MEMBER(o, _30),       \
    CONCAT_MEMBER(o, _31), CONCAT_MEMBER(o, _32), CONCAT_MEMBER(o, _33),       \
    CONCAT_MEMBER(o, _34), CONCAT_MEMBER(o, _35), CONCAT_MEMBER(o, _36),       \
    CONCAT_MEMBER(o, _37), CONCAT_MEMBER(o, _38), CONCAT_MEMBER(o, _39),       \
    CONCAT_MEMBER(o, _40), CONCAT_MEMBER(o, _41), CONCAT_MEMBER(o, _42),       \
    CONCAT_MEMBER(o, _43), CONCAT_MEMBER(o, _44), CONCAT_MEMBER(o, _45),       \
    CONCAT_MEMBER(o, _46), CONCAT_MEMBER(o, _47), CONCAT_MEMBER(o, _48),       \
    CONCAT_MEMBER(o, _49), CONCAT_MEMBER(o, _50), CONCAT_MEMBER(o, _51),       \
    CONCAT_MEMBER(o, _52), CONCAT_MEMBER(o, _53), CONCAT_MEMBER(o, _54),       \
    CONCAT_MEMBER(o, _55), CONCAT_MEMBER(o, _56), CONCAT_MEMBER(o, _57),       \
    CONCAT_MEMBER(o, _58), CONCAT_MEMBER(o, _59), CONCAT_MEMBER(o, _60),       \
    CONCAT_MEMBER(o, _61), CONCAT_MEMBER(o, _62), CONCAT_MEMBER(o, _63),       \
    CONCAT_MEMBER(o, _64), CONCAT_MEMBER(o, _65), CONCAT_MEMBER(o, _66),       \
    CONCAT_MEMBER(o, _67), CONCAT_MEMBER(o, _68), CONCAT_MEMBER(o, _69),       \
    CONCAT_MEMBER(o, _70), CONCAT_MEMBER(o, _71), CONCAT_MEMBER(o, _72),       \
    CONCAT_MEMBER(o, _73), CONCAT_MEMBER(o, _74), CONCAT_MEMBER(o, _75),       \
    CONCAT_MEMBER(o, _76), CONCAT_MEMBER(o, _77), CONCAT_MEMBER(o, _78),       \
    CONCAT_MEMBER(o, _79), CONCAT_MEMBER(o, _80))
#define YLT_CALL_ARGS81(                                                       \
    f, o, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15,    \
    _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, \
    _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, \
    _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, \
    _61, _62, _63, _64, _65, _66, _67, _68, _69, _70, _71, _72, _73, _74, _75, \
    _76, _77, _78, _79, _80, _81)                                              \
  f(CONCAT_MEMBER(o, _1), CONCAT_MEMBER(o, _2), CONCAT_MEMBER(o, _3),          \
    CONCAT_MEMBER(o, _4), CONCAT_MEMBER(o, _5), CONCAT_MEMBER(o, _6),          \
    CONCAT_MEMBER(o, _7), CONCAT_MEMBER(o, _8), CONCAT_MEMBER(o, _9),          \
    CONCAT_MEMBER(o, _10), CONCAT_MEMBER(o, _11), CONCAT_MEMBER(o, _12),       \
    CONCAT_MEMBER(o, _13), CONCAT_MEMBER(o, _14), CONCAT_MEMBER(o, _15),       \
    CONCAT_MEMBER(o, _16), CONCAT_MEMBER(o, _17), CONCAT_MEMBER(o, _18),       \
    CONCAT_MEMBER(o, _19), CONCAT_MEMBER(o, _20), CONCAT_MEMBER(o, _21),       \
    CONCAT_MEMBER(o, _22), CONCAT_MEMBER(o, _23), CONCAT_MEMBER(o, _24),       \
    CONCAT_MEMBER(o, _25), CONCAT_MEMBER(o, _26), CONCAT_MEMBER(o, _27),       \
    CONCAT_MEMBER(o, _28), CONCAT_MEMBER(o, _29), CONCAT_MEMBER(o, _30),       \
    CONCAT_MEMBER(o, _31), CONCAT_MEMBER(o, _32), CONCAT_MEMBER(o, _33),       \
    CONCAT_MEMBER(o, _34), CONCAT_MEMBER(o, _35), CONCAT_MEMBER(o, _36),       \
    CONCAT_MEMBER(o, _37), CONCAT_MEMBER(o, _38), CONCAT_MEMBER(o, _39),       \
    CONCAT_MEMBER(o, _40), CONCAT_MEMBER(o, _41), CONCAT_MEMBER(o, _42),       \
    CONCAT_MEMBER(o, _43), CONCAT_MEMBER(o, _44), CONCAT_MEMBER(o, _45),       \
    CONCAT_MEMBER(o, _46), CONCAT_MEMBER(o, _47), CONCAT_MEMBER(o, _48),       \
    CONCAT_MEMBER(o, _49), CONCAT_MEMBER(o, _50), CONCAT_MEMBER(o, _51),       \
    CONCAT_MEMBER(o, _52), CONCAT_MEMBER(o, _53), CONCAT_MEMBER(o, _54),       \
    CONCAT_MEMBER(o, _55), CONCAT_MEMBER(o, _56), CONCAT_MEMBER(o, _57),       \
    CONCAT_MEMBER(o, _58), CONCAT_MEMBER(o, _59), CONCAT_MEMBER(o, _60),       \
    CONCAT_MEMBER(o, _61), CONCAT_MEMBER(o, _62), CONCAT_MEMBER(o, _63),       \
    CONCAT_MEMBER(o, _64), CONCAT_MEMBER(o, _65), CONCAT_MEMBER(o, _66),       \
    CONCAT_MEMBER(o, _67), CONCAT_MEMBER(o, _68), CONCAT_MEMBER(o, _69),       \
    CONCAT_MEMBER(o, _70), CONCAT_MEMBER(o, _71), CONCAT_MEMBER(o, _72),       \
    CONCAT_MEMBER(o, _73), CONCAT_MEMBER(o, _74), CONCAT_MEMBER(o, _75),       \
    CONCAT_MEMBER(o, _76), CONCAT_MEMBER(o, _77), CONCAT_MEMBER(o, _78),       \
    CONCAT_MEMBER(o, _79), CONCAT_MEMBER(o, _80), CONCAT_MEMBER(o, _81))
#define YLT_CALL_ARGS82(                                                       \
    f, o, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15,    \
    _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, \
    _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, \
    _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, \
    _61, _62, _63, _64, _65, _66, _67, _68, _69, _70, _71, _72, _73, _74, _75, \
    _76, _77, _78, _79, _80, _81, _82)                                         \
  f(CONCAT_MEMBER(o, _1), CONCAT_MEMBER(o, _2), CONCAT_MEMBER(o, _3),          \
    CONCAT_MEMBER(o, _4), CONCAT_MEMBER(o, _5), CONCAT_MEMBER(o, _6),          \
    CONCAT_MEMBER(o, _7), CONCAT_MEMBER(o, _8), CONCAT_MEMBER(o, _9),          \
    CONCAT_MEMBER(o, _10), CONCAT_MEMBER(o, _11), CONCAT_MEMBER(o, _12),       \
    CONCAT_MEMBER(o, _13), CONCAT_MEMBER(o, _14), CONCAT_MEMBER(o, _15),       \
    CONCAT_MEMBER(o, _16), CONCAT_MEMBER(o, _17), CONCAT_MEMBER(o, _18),       \
    CONCAT_MEMBER(o, _19), CONCAT_MEMBER(o, _20), CONCAT_MEMBER(o, _21),       \
    CONCAT_MEMBER(o, _22), CONCAT_MEMBER(o, _23), CONCAT_MEMBER(o, _24),       \
    CONCAT_MEMBER(o, _25), CONCAT_MEMBER(o, _26), CONCAT_MEMBER(o, _27),       \
    CONCAT_MEMBER(o, _28), CONCAT_MEMBER(o, _29), CONCAT_MEMBER(o, _30),       \
    CONCAT_MEMBER(o, _31), CONCAT_MEMBER(o, _32), CONCAT_MEMBER(o, _33),       \
    CONCAT_MEMBER(o, _34), CONCAT_MEMBER(o, _35), CONCAT_MEMBER(o, _36),       \
    CONCAT_MEMBER(o, _37), CONCAT_MEMBER(o, _38), CONCAT_MEMBER(o, _39),       \
    CONCAT_MEMBER(o, _40), CONCAT_MEMBER(o, _41), CONCAT_MEMBER(o, _42),       \
    CONCAT_MEMBER(o, _43), CONCAT_MEMBER(o, _44), CONCAT_MEMBER(o, _45),       \
    CONCAT_MEMBER(o, _46), CONCAT_MEMBER(o, _47), CONCAT_MEMBER(o, _48),       \
    CONCAT_MEMBER(o, _49), CONCAT_MEMBER(o, _50), CONCAT_MEMBER(o, _51),       \
    CONCAT_MEMBER(o, _52), CONCAT_MEMBER(o, _53), CONCAT_MEMBER(o, _54),       \
    CONCAT_MEMBER(o, _55), CONCAT_MEMBER(o, _56), CONCAT_MEMBER(o, _57),       \
    CONCAT_MEMBER(o, _58), CONCAT_MEMBER(o, _59), CONCAT_MEMBER(o, _60),       \
    CONCAT_MEMBER(o, _61), CONCAT_MEMBER(o, _62), CONCAT_MEMBER(o, _63),       \
    CONCAT_MEMBER(o, _64), CONCAT_MEMBER(o, _65), CONCAT_MEMBER(o, _66),       \
    CONCAT_MEMBER(o, _67), CONCAT_MEMBER(o, _68), CONCAT_MEMBER(o, _69),       \
    CONCAT_MEMBER(o, _70), CONCAT_MEMBER(o, _71), CONCAT_MEMBER(o, _72),       \
    CONCAT_MEMBER(o, _73), CONCAT_MEMBER(o, _74), CONCAT_MEMBER(o, _75),       \
    CONCAT_MEMBER(o, _76), CONCAT_MEMBER(o, _77), CONCAT_MEMBER(o, _78),       \
    CONCAT_MEMBER(o, _79), CONCAT_MEMBER(o, _80), CONCAT_MEMBER(o, _81),       \
    CONCAT_MEMBER(o, _82))
#define YLT_CALL_ARGS83(                                                       \
    f, o, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15,    \
    _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, \
    _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, \
    _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, \
    _61, _62, _63, _64, _65, _66, _67, _68, _69, _70, _71, _72, _73, _74, _75, \
    _76, _77, _78, _79, _80, _81, _82, _83)                                    \
  f(CONCAT_MEMBER(o, _1), CONCAT_MEMBER(o, _2), CONCAT_MEMBER(o, _3),          \
    CONCAT_MEMBER(o, _4), CONCAT_MEMBER(o, _5), CONCAT_MEMBER(o, _6),          \
    CONCAT_MEMBER(o, _7), CONCAT_MEMBER(o, _8), CONCAT_MEMBER(o, _9),          \
    CONCAT_MEMBER(o, _10), CONCAT_MEMBER(o, _11), CONCAT_MEMBER(o, _12),       \
    CONCAT_MEMBER(o, _13), CONCAT_MEMBER(o, _14), CONCAT_MEMBER(o, _15),       \
    CONCAT_MEMBER(o, _16), CONCAT_MEMBER(o, _17), CONCAT_MEMBER(o, _18),       \
    CONCAT_MEMBER(o, _19), CONCAT_MEMBER(o, _20), CONCAT_MEMBER(o, _21),       \
    CONCAT_MEMBER(o, _22), CONCAT_MEMBER(o, _23), CONCAT_MEMBER(o, _24),       \
    CONCAT_MEMBER(o, _25), CONCAT_MEMBER(o, _26), CONCAT_MEMBER(o, _27),       \
    CONCAT_MEMBER(o, _28), CONCAT_MEMBER(o, _29), CONCAT_MEMBER(o, _30),       \
    CONCAT_MEMBER(o, _31), CONCAT_MEMBER(o, _32), CONCAT_MEMBER(o, _33),       \
    CONCAT_MEMBER(o, _34), CONCAT_MEMBER(o, _35), CONCAT_MEMBER(o, _36),       \
    CONCAT_MEMBER(o, _37), CONCAT_MEMBER(o, _38), CONCAT_MEMBER(o, _39),       \
    CONCAT_MEMBER(o, _40), CONCAT_MEMBER(o, _41), CONCAT_MEMBER(o, _42),       \
    CONCAT_MEMBER(o, _43), CONCAT_MEMBER(o, _44), CONCAT_MEMBER(o, _45),       \
    CONCAT_MEMBER(o, _46), CONCAT_MEMBER(o, _47), CONCAT_MEMBER(o, _48),       \
    CONCAT_MEMBER(o, _49), CONCAT_MEMBER(o, _50), CONCAT_MEMBER(o, _51),       \
    CONCAT_MEMBER(o, _52), CONCAT_MEMBER(o, _53), CONCAT_MEMBER(o, _54),       \
    CONCAT_MEMBER(o, _55), CONCAT_MEMBER(o, _56), CONCAT_MEMBER(o, _57),       \
    CONCAT_MEMBER(o, _58), CONCAT_MEMBER(o, _59), CONCAT_MEMBER(o, _60),       \
    CONCAT_MEMBER(o, _61), CONCAT_MEMBER(o, _62), CONCAT_MEMBER(o, _63),       \
    CONCAT_MEMBER(o, _64), CONCAT_MEMBER(o, _65), CONCAT_MEMBER(o, _66),       \
    CONCAT_MEMBER(o, _67), CONCAT_MEMBER(o, _68), CONCAT_MEMBER(o, _69),       \
    CONCAT_MEMBER(o, _70), CONCAT_MEMBER(o, _71), CONCAT_MEMBER(o, _72),       \
    CONCAT_MEMBER(o, _73), CONCAT_MEMBER(o, _74), CONCAT_MEMBER(o, _75),       \
    CONCAT_MEMBER(o, _76), CONCAT_MEMBER(o, _77), CONCAT_MEMBER(o, _78),       \
    CONCAT_MEMBER(o, _79), CONCAT_MEMBER(o, _80), CONCAT_MEMBER(o, _81),       \
    CONCAT_MEMBER(o, _82), CONCAT_MEMBER(o, _83))
#define YLT_CALL_ARGS84(                                                       \
    f, o, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15,    \
    _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, \
    _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, \
    _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, \
    _61, _62, _63, _64, _65, _66, _67, _68, _69, _70, _71, _72, _73, _74, _75, \
    _76, _77, _78, _79, _80, _81, _82, _83, _84)                               \
  f(CONCAT_MEMBER(o, _1), CONCAT_MEMBER(o, _2), CONCAT_MEMBER(o, _3),          \
    CONCAT_MEMBER(o, _4), CONCAT_MEMBER(o, _5), CONCAT_MEMBER(o, _6),          \
    CONCAT_MEMBER(o, _7), CONCAT_MEMBER(o, _8), CONCAT_MEMBER(o, _9),          \
    CONCAT_MEMBER(o, _10), CONCAT_MEMBER(o, _11), CONCAT_MEMBER(o, _12),       \
    CONCAT_MEMBER(o, _13), CONCAT_MEMBER(o, _14), CONCAT_MEMBER(o, _15),       \
    CONCAT_MEMBER(o, _16), CONCAT_MEMBER(o, _17), CONCAT_MEMBER(o, _18),       \
    CONCAT_MEMBER(o, _19), CONCAT_MEMBER(o, _20), CONCAT_MEMBER(o, _21),       \
    CONCAT_MEMBER(o, _22), CONCAT_MEMBER(o, _23), CONCAT_MEMBER(o, _24),       \
    CONCAT_MEMBER(o, _25), CONCAT_MEMBER(o, _26), CONCAT_MEMBER(o, _27),       \
    CONCAT_MEMBER(o, _28), CONCAT_MEMBER(o, _29), CONCAT_MEMBER(o, _30),       \
    CONCAT_MEMBER(o, _31), CONCAT_MEMBER(o, _32), CONCAT_MEMBER(o, _33),       \
    CONCAT_MEMBER(o, _34), CONCAT_MEMBER(o, _35), CONCAT_MEMBER(o, _36),       \
    CONCAT_MEMBER(o, _37), CONCAT_MEMBER(o, _38), CONCAT_MEMBER(o, _39),       \
    CONCAT_MEMBER(o, _40), CONCAT_MEMBER(o, _41), CONCAT_MEMBER(o, _42),       \
    CONCAT_MEMBER(o, _43), CONCAT_MEMBER(o, _44), CONCAT_MEMBER(o, _45),       \
    CONCAT_MEMBER(o, _46), CONCAT_MEMBER(o, _47), CONCAT_MEMBER(o, _48),       \
    CONCAT_MEMBER(o, _49), CONCAT_MEMBER(o, _50), CONCAT_MEMBER(o, _51),       \
    CONCAT_MEMBER(o, _52), CONCAT_MEMBER(o, _53), CONCAT_MEMBER(o, _54),       \
    CONCAT_MEMBER(o, _55), CONCAT_MEMBER(o, _56), CONCAT_MEMBER(o, _57),       \
    CONCAT_MEMBER(o, _58), CONCAT_MEMBER(o, _59), CONCAT_MEMBER(o, _60),       \
    CONCAT_MEMBER(o, _61), CONCAT_MEMBER(o, _62), CONCAT_MEMBER(o, _63),       \
    CONCAT_MEMBER(o, _64), CONCAT_MEMBER(o, _65), CONCAT_MEMBER(o, _66),       \
    CONCAT_MEMBER(o, _67), CONCAT_MEMBER(o, _68), CONCAT_MEMBER(o, _69),       \
    CONCAT_MEMBER(o, _70), CONCAT_MEMBER(o, _71), CONCAT_MEMBER(o, _72),       \
    CONCAT_MEMBER(o, _73), CONCAT_MEMBER(o, _74), CONCAT_MEMBER(o, _75),       \
    CONCAT_MEMBER(o, _76), CONCAT_MEMBER(o, _77), CONCAT_MEMBER(o, _78),       \
    CONCAT_MEMBER(o, _79), CONCAT_MEMBER(o, _80), CONCAT_MEMBER(o, _81),       \
    CONCAT_MEMBER(o, _82), CONCAT_MEMBER(o, _83), CONCAT_MEMBER(o, _84))
#define YLT_CALL_ARGS85(                                                       \
    f, o, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15,    \
    _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, \
    _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, \
    _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, \
    _61, _62, _63, _64, _65, _66, _67, _68, _69, _70, _71, _72, _73, _74, _75, \
    _76, _77, _78, _79, _80, _81, _82, _83, _84, _85)                          \
  f(CONCAT_MEMBER(o, _1), CONCAT_MEMBER(o, _2), CONCAT_MEMBER(o, _3),          \
    CONCAT_MEMBER(o, _4), CONCAT_MEMBER(o, _5), CONCAT_MEMBER(o, _6),          \
    CONCAT_MEMBER(o, _7), CONCAT_MEMBER(o, _8), CONCAT_MEMBER(o, _9),          \
    CONCAT_MEMBER(o, _10), CONCAT_MEMBER(o, _11), CONCAT_MEMBER(o, _12),       \
    CONCAT_MEMBER(o, _13), CONCAT_MEMBER(o, _14), CONCAT_MEMBER(o, _15),       \
    CONCAT_MEMBER(o, _16), CONCAT_MEMBER(o, _17), CONCAT_MEMBER(o, _18),       \
    CONCAT_MEMBER(o, _19), CONCAT_MEMBER(o, _20), CONCAT_MEMBER(o, _21),       \
    CONCAT_MEMBER(o, _22), CONCAT_MEMBER(o, _23), CONCAT_MEMBER(o, _24),       \
    CONCAT_MEMBER(o, _25), CONCAT_MEMBER(o, _26), CONCAT_MEMBER(o, _27),       \
    CONCAT_MEMBER(o, _28), CONCAT_MEMBER(o, _29), CONCAT_MEMBER(o, _30),       \
    CONCAT_MEMBER(o, _31), CONCAT_MEMBER(o, _32), CONCAT_MEMBER(o, _33),       \
    CONCAT_MEMBER(o, _34), CONCAT_MEMBER(o, _35), CONCAT_MEMBER(o, _36),       \
    CONCAT_MEMBER(o, _37), CONCAT_MEMBER(o, _38), CONCAT_MEMBER(o, _39),       \
    CONCAT_MEMBER(o, _40), CONCAT_MEMBER(o, _41), CONCAT_MEMBER(o, _42),       \
    CONCAT_MEMBER(o, _43), CONCAT_MEMBER(o, _44), CONCAT_MEMBER(o, _45),       \
    CONCAT_MEMBER(o, _46), CONCAT_MEMBER(o, _47), CONCAT_MEMBER(o, _48),       \
    CONCAT_MEMBER(o, _49), CONCAT_MEMBER(o, _50), CONCAT_MEMBER(o, _51),       \
    CONCAT_MEMBER(o, _52), CONCAT_MEMBER(o, _53), CONCAT_MEMBER(o, _54),       \
    CONCAT_MEMBER(o, _55), CONCAT_MEMBER(o, _56), CONCAT_MEMBER(o, _57),       \
    CONCAT_MEMBER(o, _58), CONCAT_MEMBER(o, _59), CONCAT_MEMBER(o, _60),       \
    CONCAT_MEMBER(o, _61), CONCAT_MEMBER(o, _62), CONCAT_MEMBER(o, _63),       \
    CONCAT_MEMBER(o, _64), CONCAT_MEMBER(o, _65), CONCAT_MEMBER(o, _66),       \
    CONCAT_MEMBER(o, _67), CONCAT_MEMBER(o, _68), CONCAT_MEMBER(o, _69),       \
    CONCAT_MEMBER(o, _70), CONCAT_MEMBER(o, _71), CONCAT_MEMBER(o, _72),       \
    CONCAT_MEMBER(o, _73), CONCAT_MEMBER(o, _74), CONCAT_MEMBER(o, _75),       \
    CONCAT_MEMBER(o, _76), CONCAT_MEMBER(o, _77), CONCAT_MEMBER(o, _78),       \
    CONCAT_MEMBER(o, _79), CONCAT_MEMBER(o, _80), CONCAT_MEMBER(o, _81),       \
    CONCAT_MEMBER(o, _82), CONCAT_MEMBER(o, _83), CONCAT_MEMBER(o, _84),       \
    CONCAT_MEMBER(o, _85))
#define YLT_CALL_ARGS86(                                                       \
    f, o, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15,    \
    _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, \
    _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, \
    _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, \
    _61, _62, _63, _64, _65, _66, _67, _68, _69, _70, _71, _72, _73, _74, _75, \
    _76, _77, _78, _79, _80, _81, _82, _83, _84, _85, _86)                     \
  f(CONCAT_MEMBER(o, _1), CONCAT_MEMBER(o, _2), CONCAT_MEMBER(o, _3),          \
    CONCAT_MEMBER(o, _4), CONCAT_MEMBER(o, _5), CONCAT_MEMBER(o, _6),          \
    CONCAT_MEMBER(o, _7), CONCAT_MEMBER(o, _8), CONCAT_MEMBER(o, _9),          \
    CONCAT_MEMBER(o, _10), CONCAT_MEMBER(o, _11), CONCAT_MEMBER(o, _12),       \
    CONCAT_MEMBER(o, _13), CONCAT_MEMBER(o, _14), CONCAT_MEMBER(o, _15),       \
    CONCAT_MEMBER(o, _16), CONCAT_MEMBER(o, _17), CONCAT_MEMBER(o, _18),       \
    CONCAT_MEMBER(o, _19), CONCAT_MEMBER(o, _20), CONCAT_MEMBER(o, _21),       \
    CONCAT_MEMBER(o, _22), CONCAT_MEMBER(o, _23), CONCAT_MEMBER(o, _24),       \
    CONCAT_MEMBER(o, _25), CONCAT_MEMBER(o, _26), CONCAT_MEMBER(o, _27),       \
    CONCAT_MEMBER(o, _28), CONCAT_MEMBER(o, _29), CONCAT_MEMBER(o, _30),       \
    CONCAT_MEMBER(o, _31), CONCAT_MEMBER(o, _32), CONCAT_MEMBER(o, _33),       \
    CONCAT_MEMBER(o, _34), CONCAT_MEMBER(o, _35), CONCAT_MEMBER(o, _36),       \
    CONCAT_MEMBER(o, _37), CONCAT_MEMBER(o, _38), CONCAT_MEMBER(o, _39),       \
    CONCAT_MEMBER(o, _40), CONCAT_MEMBER(o, _41), CONCAT_MEMBER(o, _42),       \
    CONCAT_MEMBER(o, _43), CONCAT_MEMBER(o, _44), CONCAT_MEMBER(o, _45),       \
    CONCAT_MEMBER(o, _46), CONCAT_MEMBER(o, _47), CONCAT_MEMBER(o, _48),       \
    CONCAT_MEMBER(o, _49), CONCAT_MEMBER(o, _50), CONCAT_MEMBER(o, _51),       \
    CONCAT_MEMBER(o, _52), CONCAT_MEMBER(o, _53), CONCAT_MEMBER(o, _54),       \
    CONCAT_MEMBER(o, _55), CONCAT_MEMBER(o, _56), CONCAT_MEMBER(o, _57),       \
    CONCAT_MEMBER(o, _58), CONCAT_MEMBER(o, _59), CONCAT_MEMBER(o, _60),       \
    CONCAT_MEMBER(o, _61), CONCAT_MEMBER(o, _62), CONCAT_MEMBER(o, _63),       \
    CONCAT_MEMBER(o, _64), CONCAT_MEMBER(o, _65), CONCAT_MEMBER(o, _66),       \
    CONCAT_MEMBER(o, _67), CONCAT_MEMBER(o, _68), CONCAT_MEMBER(o, _69),       \
    CONCAT_MEMBER(o, _70), CONCAT_MEMBER(o, _71), CONCAT_MEMBER(o, _72),       \
    CONCAT_MEMBER(o, _73), CONCAT_MEMBER(o, _74), CONCAT_MEMBER(o, _75),       \
    CONCAT_MEMBER(o, _76), CONCAT_MEMBER(o, _77), CONCAT_MEMBER(o, _78),       \
    CONCAT_MEMBER(o, _79), CONCAT_MEMBER(o, _80), CONCAT_MEMBER(o, _81),       \
    CONCAT_MEMBER(o, _82), CONCAT_MEMBER(o, _83), CONCAT_MEMBER(o, _84),       \
    CONCAT_MEMBER(o, _85), CONCAT_MEMBER(o, _86))
#define YLT_CALL_ARGS87(                                                       \
    f, o, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15,    \
    _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, \
    _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, \
    _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, \
    _61, _62, _63, _64, _65, _66, _67, _68, _69, _70, _71, _72, _73, _74, _75, \
    _76, _77, _78, _79, _80, _81, _82, _83, _84, _85, _86, _87)                \
  f(CONCAT_MEMBER(o, _1), CONCAT_MEMBER(o, _2), CONCAT_MEMBER(o, _3),          \
    CONCAT_MEMBER(o, _4), CONCAT_MEMBER(o, _5), CONCAT_MEMBER(o, _6),          \
    CONCAT_MEMBER(o, _7), CONCAT_MEMBER(o, _8), CONCAT_MEMBER(o, _9),          \
    CONCAT_MEMBER(o, _10), CONCAT_MEMBER(o, _11), CONCAT_MEMBER(o, _12),       \
    CONCAT_MEMBER(o, _13), CONCAT_MEMBER(o, _14), CONCAT_MEMBER(o, _15),       \
    CONCAT_MEMBER(o, _16), CONCAT_MEMBER(o, _17), CONCAT_MEMBER(o, _18),       \
    CONCAT_MEMBER(o, _19), CONCAT_MEMBER(o, _20), CONCAT_MEMBER(o, _21),       \
    CONCAT_MEMBER(o, _22), CONCAT_MEMBER(o, _23), CONCAT_MEMBER(o, _24),       \
    CONCAT_MEMBER(o, _25), CONCAT_MEMBER(o, _26), CONCAT_MEMBER(o, _27),       \
    CONCAT_MEMBER(o, _28), CONCAT_MEMBER(o, _29), CONCAT_MEMBER(o, _30),       \
    CONCAT_MEMBER(o, _31), CONCAT_MEMBER(o, _32), CONCAT_MEMBER(o, _33),       \
    CONCAT_MEMBER(o, _34), CONCAT_MEMBER(o, _35), CONCAT_MEMBER(o, _36),       \
    CONCAT_MEMBER(o, _37), CONCAT_MEMBER(o, _38), CONCAT_MEMBER(o, _39),       \
    CONCAT_MEMBER(o, _40), CONCAT_MEMBER(o, _41), CONCAT_MEMBER(o, _42),       \
    CONCAT_MEMBER(o, _43), CONCAT_MEMBER(o, _44), CONCAT_MEMBER(o, _45),       \
    CONCAT_MEMBER(o, _46), CONCAT_MEMBER(o, _47), CONCAT_MEMBER(o, _48),       \
    CONCAT_MEMBER(o, _49), CONCAT_MEMBER(o, _50), CONCAT_MEMBER(o, _51),       \
    CONCAT_MEMBER(o, _52), CONCAT_MEMBER(o, _53), CONCAT_MEMBER(o, _54),       \
    CONCAT_MEMBER(o, _55), CONCAT_MEMBER(o, _56), CONCAT_MEMBER(o, _57),       \
    CONCAT_MEMBER(o, _58), CONCAT_MEMBER(o, _59), CONCAT_MEMBER(o, _60),       \
    CONCAT_MEMBER(o, _61), CONCAT_MEMBER(o, _62), CONCAT_MEMBER(o, _63),       \
    CONCAT_MEMBER(o, _64), CONCAT_MEMBER(o, _65), CONCAT_MEMBER(o, _66),       \
    CONCAT_MEMBER(o, _67), CONCAT_MEMBER(o, _68), CONCAT_MEMBER(o, _69),       \
    CONCAT_MEMBER(o, _70), CONCAT_MEMBER(o, _71), CONCAT_MEMBER(o, _72),       \
    CONCAT_MEMBER(o, _73), CONCAT_MEMBER(o, _74), CONCAT_MEMBER(o, _75),       \
    CONCAT_MEMBER(o, _76), CONCAT_MEMBER(o, _77), CONCAT_MEMBER(o, _78),       \
    CONCAT_MEMBER(o, _79), CONCAT_MEMBER(o, _80), CONCAT_MEMBER(o, _81),       \
    CONCAT_MEMBER(o, _82), CONCAT_MEMBER(o, _83), CONCAT_MEMBER(o, _84),       \
    CONCAT_MEMBER(o, _85), CONCAT_MEMBER(o, _86), CONCAT_MEMBER(o, _87))
#define YLT_CALL_ARGS88(                                                       \
    f, o, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15,    \
    _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, \
    _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, \
    _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, \
    _61, _62, _63, _64, _65, _66, _67, _68, _69, _70, _71, _72, _73, _74, _75, \
    _76, _77, _78, _79, _80, _81, _82, _83, _84, _85, _86, _87, _88)           \
  f(CONCAT_MEMBER(o, _1), CONCAT_MEMBER(o, _2), CONCAT_MEMBER(o, _3),          \
    CONCAT_MEMBER(o, _4), CONCAT_MEMBER(o, _5), CONCAT_MEMBER(o, _6),          \
    CONCAT_MEMBER(o, _7), CONCAT_MEMBER(o, _8), CONCAT_MEMBER(o, _9),          \
    CONCAT_MEMBER(o, _10), CONCAT_MEMBER(o, _11), CONCAT_MEMBER(o, _12),       \
    CONCAT_MEMBER(o, _13), CONCAT_MEMBER(o, _14), CONCAT_MEMBER(o, _15),       \
    CONCAT_MEMBER(o, _16), CONCAT_MEMBER(o, _17), CONCAT_MEMBER(o, _18),       \
    CONCAT_MEMBER(o, _19), CONCAT_MEMBER(o, _20), CONCAT_MEMBER(o, _21),       \
    CONCAT_MEMBER(o, _22), CONCAT_MEMBER(o, _23), CONCAT_MEMBER(o, _24),       \
    CONCAT_MEMBER(o, _25), CONCAT_MEMBER(o, _26), CONCAT_MEMBER(o, _27),       \
    CONCAT_MEMBER(o, _28), CONCAT_MEMBER(o, _29), CONCAT_MEMBER(o, _30),       \
    CONCAT_MEMBER(o, _31), CONCAT_MEMBER(o, _32), CONCAT_MEMBER(o, _33),       \
    CONCAT_MEMBER(o, _34), CONCAT_MEMBER(o, _35), CONCAT_MEMBER(o, _36),       \
    CONCAT_MEMBER(o, _37), CONCAT_MEMBER(o, _38), CONCAT_MEMBER(o, _39),       \
    CONCAT_MEMBER(o, _40), CONCAT_MEMBER(o, _41), CONCAT_MEMBER(o, _42),       \
    CONCAT_MEMBER(o, _43), CONCAT_MEMBER(o, _44), CONCAT_MEMBER(o, _45),       \
    CONCAT_MEMBER(o, _46), CONCAT_MEMBER(o, _47), CONCAT_MEMBER(o, _48),       \
    CONCAT_MEMBER(o, _49), CONCAT_MEMBER(o, _50), CONCAT_MEMBER(o, _51),       \
    CONCAT_MEMBER(o, _52), CONCAT_MEMBER(o, _53), CONCAT_MEMBER(o, _54),       \
    CONCAT_MEMBER(o, _55), CONCAT_MEMBER(o, _56), CONCAT_MEMBER(o, _57),       \
    CONCAT_MEMBER(o, _58), CONCAT_MEMBER(o, _59), CONCAT_MEMBER(o, _60),       \
    CONCAT_MEMBER(o, _61), CONCAT_MEMBER(o, _62), CONCAT_MEMBER(o, _63),       \
    CONCAT_MEMBER(o, _64), CONCAT_MEMBER(o, _65), CONCAT_MEMBER(o, _66),       \
    CONCAT_MEMBER(o, _67), CONCAT_MEMBER(o, _68), CONCAT_MEMBER(o, _69),       \
    CONCAT_MEMBER(o, _70), CONCAT_MEMBER(o, _71), CONCAT_MEMBER(o, _72),       \
    CONCAT_MEMBER(o, _73), CONCAT_MEMBER(o, _74), CONCAT_MEMBER(o, _75),       \
    CONCAT_MEMBER(o, _76), CONCAT_MEMBER(o, _77), CONCAT_MEMBER(o, _78),       \
    CONCAT_MEMBER(o, _79), CONCAT_MEMBER(o, _80), CONCAT_MEMBER(o, _81),       \
    CONCAT_MEMBER(o, _82), CONCAT_MEMBER(o, _83), CONCAT_MEMBER(o, _84),       \
    CONCAT_MEMBER(o, _85), CONCAT_MEMBER(o, _86), CONCAT_MEMBER(o, _87),       \
    CONCAT_MEMBER(o, _88))
#define YLT_CALL_ARGS89(                                                       \
    f, o, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15,    \
    _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, \
    _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, \
    _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, \
    _61, _62, _63, _64, _65, _66, _67, _68, _69, _70, _71, _72, _73, _74, _75, \
    _76, _77, _78, _79, _80, _81, _82, _83, _84, _85, _86, _87, _88, _89)      \
  f(CONCAT_MEMBER(o, _1), CONCAT_MEMBER(o, _2), CONCAT_MEMBER(o, _3),          \
    CONCAT_MEMBER(o, _4), CONCAT_MEMBER(o, _5), CONCAT_MEMBER(o, _6),          \
    CONCAT_MEMBER(o, _7), CONCAT_MEMBER(o, _8), CONCAT_MEMBER(o, _9),          \
    CONCAT_MEMBER(o, _10), CONCAT_MEMBER(o, _11), CONCAT_MEMBER(o, _12),       \
    CONCAT_MEMBER(o, _13), CONCAT_MEMBER(o, _14), CONCAT_MEMBER(o, _15),       \
    CONCAT_MEMBER(o, _16), CONCAT_MEMBER(o, _17), CONCAT_MEMBER(o, _18),       \
    CONCAT_MEMBER(o, _19), CONCAT_MEMBER(o, _20), CONCAT_MEMBER(o, _21),       \
    CONCAT_MEMBER(o, _22), CONCAT_MEMBER(o, _23), CONCAT_MEMBER(o, _24),       \
    CONCAT_MEMBER(o, _25), CONCAT_MEMBER(o, _26), CONCAT_MEMBER(o, _27),       \
    CONCAT_MEMBER(o, _28), CONCAT_MEMBER(o, _29), CONCAT_MEMBER(o, _30),       \
    CONCAT_MEMBER(o, _31), CONCAT_MEMBER(o, _32), CONCAT_MEMBER(o, _33),       \
    CONCAT_MEMBER(o, _34), CONCAT_MEMBER(o, _35), CONCAT_MEMBER(o, _36),       \
    CONCAT_MEMBER(o, _37), CONCAT_MEMBER(o, _38), CONCAT_MEMBER(o, _39),       \
    CONCAT_MEMBER(o, _40), CONCAT_MEMBER(o, _41), CONCAT_MEMBER(o, _42),       \
    CONCAT_MEMBER(o, _43), CONCAT_MEMBER(o, _44), CONCAT_MEMBER(o, _45),       \
    CONCAT_MEMBER(o, _46), CONCAT_MEMBER(o, _47), CONCAT_MEMBER(o, _48),       \
    CONCAT_MEMBER(o, _49), CONCAT_MEMBER(o, _50), CONCAT_MEMBER(o, _51),       \
    CONCAT_MEMBER(o, _52), CONCAT_MEMBER(o, _53), CONCAT_MEMBER(o, _54),       \
    CONCAT_MEMBER(o, _55), CONCAT_MEMBER(o, _56), CONCAT_MEMBER(o, _57),       \
    CONCAT_MEMBER(o, _58), CONCAT_MEMBER(o, _59), CONCAT_MEMBER(o, _60),       \
    CONCAT_MEMBER(o, _61), CONCAT_MEMBER(o, _62), CONCAT_MEMBER(o, _63),       \
    CONCAT_MEMBER(o, _64), CONCAT_MEMBER(o, _65), CONCAT_MEMBER(o, _66),       \
    CONCAT_MEMBER(o, _67), CONCAT_MEMBER(o, _68), CONCAT_MEMBER(o, _69),       \
    CONCAT_MEMBER(o, _70), CONCAT_MEMBER(o, _71), CONCAT_MEMBER(o, _72),       \
    CONCAT_MEMBER(o, _73), CONCAT_MEMBER(o, _74), CONCAT_MEMBER(o, _75),       \
    CONCAT_MEMBER(o, _76), CONCAT_MEMBER(o, _77), CONCAT_MEMBER(o, _78),       \
    CONCAT_MEMBER(o, _79), CONCAT_MEMBER(o, _80), CONCAT_MEMBER(o, _81),       \
    CONCAT_MEMBER(o, _82), CONCAT_MEMBER(o, _83), CONCAT_MEMBER(o, _84),       \
    CONCAT_MEMBER(o, _85), CONCAT_MEMBER(o, _86), CONCAT_MEMBER(o, _87),       \
    CONCAT_MEMBER(o, _88), CONCAT_MEMBER(o, _89))
#define YLT_CALL_ARGS90(                                                       \
    f, o, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15,    \
    _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, \
    _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, \
    _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, \
    _61, _62, _63, _64, _65, _66, _67, _68, _69, _70, _71, _72, _73, _74, _75, \
    _76, _77, _78, _79, _80, _81, _82, _83, _84, _85, _86, _87, _88, _89, _90) \
  f(CONCAT_MEMBER(o, _1), CONCAT_MEMBER(o, _2), CONCAT_MEMBER(o, _3),          \
    CONCAT_MEMBER(o, _4), CONCAT_MEMBER(o, _5), CONCAT_MEMBER(o, _6),          \
    CONCAT_MEMBER(o, _7), CONCAT_MEMBER(o, _8), CONCAT_MEMBER(o, _9),          \
    CONCAT_MEMBER(o, _10), CONCAT_MEMBER(o, _11), CONCAT_MEMBER(o, _12),       \
    CONCAT_MEMBER(o, _13), CONCAT_MEMBER(o, _14), CONCAT_MEMBER(o, _15),       \
    CONCAT_MEMBER(o, _16), CONCAT_MEMBER(o, _17), CONCAT_MEMBER(o, _18),       \
    CONCAT_MEMBER(o, _19), CONCAT_MEMBER(o, _20), CONCAT_MEMBER(o, _21),       \
    CONCAT_MEMBER(o, _22), CONCAT_MEMBER(o, _23), CONCAT_MEMBER(o, _24),       \
    CONCAT_MEMBER(o, _25), CONCAT_MEMBER(o, _26), CONCAT_MEMBER(o, _27),       \
    CONCAT_MEMBER(o, _28), CONCAT_MEMBER(o, _29), CONCAT_MEMBER(o, _30),       \
    CONCAT_MEMBER(o, _31), CONCAT_MEMBER(o, _32), CONCAT_MEMBER(o, _33),       \
    CONCAT_MEMBER(o, _34), CONCAT_MEMBER(o, _35), CONCAT_MEMBER(o, _36),       \
    CONCAT_MEMBER(o, _37), CONCAT_MEMBER(o, _38), CONCAT_MEMBER(o, _39),       \
    CONCAT_MEMBER(o, _40), CONCAT_MEMBER(o, _41), CONCAT_MEMBER(o, _42),       \
    CONCAT_MEMBER(o, _43), CONCAT_MEMBER(o, _44), CONCAT_MEMBER(o, _45),       \
    CONCAT_MEMBER(o, _46), CONCAT_MEMBER(o, _47), CONCAT_MEMBER(o, _48),       \
    CONCAT_MEMBER(o, _49), CONCAT_MEMBER(o, _50), CONCAT_MEMBER(o, _51),       \
    CONCAT_MEMBER(o, _52), CONCAT_MEMBER(o, _53), CONCAT_MEMBER(o, _54),       \
    CONCAT_MEMBER(o, _55), CONCAT_MEMBER(o, _56), CONCAT_MEMBER(o, _57),       \
    CONCAT_MEMBER(o, _58), CONCAT_MEMBER(o, _59), CONCAT_MEMBER(o, _60),       \
    CONCAT_MEMBER(o, _61), CONCAT_MEMBER(o, _62), CONCAT_MEMBER(o, _63),       \
    CONCAT_MEMBER(o, _64), CONCAT_MEMBER(o, _65), CONCAT_MEMBER(o, _66),       \
    CONCAT_MEMBER(o, _67), CONCAT_MEMBER(o, _68), CONCAT_MEMBER(o, _69),       \
    CONCAT_MEMBER(o, _70), CONCAT_MEMBER(o, _71), CONCAT_MEMBER(o, _72),       \
    CONCAT_MEMBER(o, _73), CONCAT_MEMBER(o, _74), CONCAT_MEMBER(o, _75),       \
    CONCAT_MEMBER(o, _76), CONCAT_MEMBER(o, _77), CONCAT_MEMBER(o, _78),       \
    CONCAT_MEMBER(o, _79), CONCAT_MEMBER(o, _80), CONCAT_MEMBER(o, _81),       \
    CONCAT_MEMBER(o, _82), CONCAT_MEMBER(o, _83), CONCAT_MEMBER(o, _84),       \
    CONCAT_MEMBER(o, _85), CONCAT_MEMBER(o, _86), CONCAT_MEMBER(o, _87),       \
    CONCAT_MEMBER(o, _88), CONCAT_MEMBER(o, _89), CONCAT_MEMBER(o, _90))
#define YLT_CALL_ARGS91(                                                       \
    f, o, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15,    \
    _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, \
    _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, \
    _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, \
    _61, _62, _63, _64, _65, _66, _67, _68, _69, _70, _71, _72, _73, _74, _75, \
    _76, _77, _78, _79, _80, _81, _82, _83, _84, _85, _86, _87, _88, _89, _90, \
    _91)                                                                       \
  f(CONCAT_MEMBER(o, _1), CONCAT_MEMBER(o, _2), CONCAT_MEMBER(o, _3),          \
    CONCAT_MEMBER(o, _4), CONCAT_MEMBER(o, _5), CONCAT_MEMBER(o, _6),          \
    CONCAT_MEMBER(o, _7), CONCAT_MEMBER(o, _8), CONCAT_MEMBER(o, _9),          \
    CONCAT_MEMBER(o, _10), CONCAT_MEMBER(o, _11), CONCAT_MEMBER(o, _12),       \
    CONCAT_MEMBER(o, _13), CONCAT_MEMBER(o, _14), CONCAT_MEMBER(o, _15),       \
    CONCAT_MEMBER(o, _16), CONCAT_MEMBER(o, _17), CONCAT_MEMBER(o, _18),       \
    CONCAT_MEMBER(o, _19), CONCAT_MEMBER(o, _20), CONCAT_MEMBER(o, _21),       \
    CONCAT_MEMBER(o, _22), CONCAT_MEMBER(o, _23), CONCAT_MEMBER(o, _24),       \
    CONCAT_MEMBER(o, _25), CONCAT_MEMBER(o, _26), CONCAT_MEMBER(o, _27),       \
    CONCAT_MEMBER(o, _28), CONCAT_MEMBER(o, _29), CONCAT_MEMBER(o, _30),       \
    CONCAT_MEMBER(o, _31), CONCAT_MEMBER(o, _32), CONCAT_MEMBER(o, _33),       \
    CONCAT_MEMBER(o, _34), CONCAT_MEMBER(o, _35), CONCAT_MEMBER(o, _36),       \
    CONCAT_MEMBER(o, _37), CONCAT_MEMBER(o, _38), CONCAT_MEMBER(o, _39),       \
    CONCAT_MEMBER(o, _40), CONCAT_MEMBER(o, _41), CONCAT_MEMBER(o, _42),       \
    CONCAT_MEMBER(o, _43), CONCAT_MEMBER(o, _44), CONCAT_MEMBER(o, _45),       \
    CONCAT_MEMBER(o, _46), CONCAT_MEMBER(o, _47), CONCAT_MEMBER(o, _48),       \
    CONCAT_MEMBER(o, _49), CONCAT_MEMBER(o, _50), CONCAT_MEMBER(o, _51),       \
    CONCAT_MEMBER(o, _52), CONCAT_MEMBER(o, _53), CONCAT_MEMBER(o, _54),       \
    CONCAT_MEMBER(o, _55), CONCAT_MEMBER(o, _56), CONCAT_MEMBER(o, _57),       \
    CONCAT_MEMBER(o, _58), CONCAT_MEMBER(o, _59), CONCAT_MEMBER(o, _60),       \
    CONCAT_MEMBER(o, _61), CONCAT_MEMBER(o, _62), CONCAT_MEMBER(o, _63),       \
    CONCAT_MEMBER(o, _64), CONCAT_MEMBER(o, _65), CONCAT_MEMBER(o, _66),       \
    CONCAT_MEMBER(o, _67), CONCAT_MEMBER(o, _68), CONCAT_MEMBER(o, _69),       \
    CONCAT_MEMBER(o, _70), CONCAT_MEMBER(o, _71), CONCAT_MEMBER(o, _72),       \
    CONCAT_MEMBER(o, _73), CONCAT_MEMBER(o, _74), CONCAT_MEMBER(o, _75),       \
    CONCAT_MEMBER(o, _76), CONCAT_MEMBER(o, _77), CONCAT_MEMBER(o, _78),       \
    CONCAT_MEMBER(o, _79), CONCAT_MEMBER(o, _80), CONCAT_MEMBER(o, _81),       \
    CONCAT_MEMBER(o, _82), CONCAT_MEMBER(o, _83), CONCAT_MEMBER(o, _84),       \
    CONCAT_MEMBER(o, _85), CONCAT_MEMBER(o, _86), CONCAT_MEMBER(o, _87),       \
    CONCAT_MEMBER(o, _88), CONCAT_MEMBER(o, _89), CONCAT_MEMBER(o, _90),       \
    CONCAT_MEMBER(o, _91))
#define YLT_CALL_ARGS92(                                                       \
    f, o, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15,    \
    _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, \
    _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, \
    _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, \
    _61, _62, _63, _64, _65, _66, _67, _68, _69, _70, _71, _72, _73, _74, _75, \
    _76, _77, _78, _79, _80, _81, _82, _83, _84, _85, _86, _87, _88, _89, _90, \
    _91, _92)                                                                  \
  f(CONCAT_MEMBER(o, _1), CONCAT_MEMBER(o, _2), CONCAT_MEMBER(o, _3),          \
    CONCAT_MEMBER(o, _4), CONCAT_MEMBER(o, _5), CONCAT_MEMBER(o, _6),          \
    CONCAT_MEMBER(o, _7), CONCAT_MEMBER(o, _8), CONCAT_MEMBER(o, _9),          \
    CONCAT_MEMBER(o, _10), CONCAT_MEMBER(o, _11), CONCAT_MEMBER(o, _12),       \
    CONCAT_MEMBER(o, _13), CONCAT_MEMBER(o, _14), CONCAT_MEMBER(o, _15),       \
    CONCAT_MEMBER(o, _16), CONCAT_MEMBER(o, _17), CONCAT_MEMBER(o, _18),       \
    CONCAT_MEMBER(o, _19), CONCAT_MEMBER(o, _20), CONCAT_MEMBER(o, _21),       \
    CONCAT_MEMBER(o, _22), CONCAT_MEMBER(o, _23), CONCAT_MEMBER(o, _24),       \
    CONCAT_MEMBER(o, _25), CONCAT_MEMBER(o, _26), CONCAT_MEMBER(o, _27),       \
    CONCAT_MEMBER(o, _28), CONCAT_MEMBER(o, _29), CONCAT_MEMBER(o, _30),       \
    CONCAT_MEMBER(o, _31), CONCAT_MEMBER(o, _32), CONCAT_MEMBER(o, _33),       \
    CONCAT_MEMBER(o, _34), CONCAT_MEMBER(o, _35), CONCAT_MEMBER(o, _36),       \
    CONCAT_MEMBER(o, _37), CONCAT_MEMBER(o, _38), CONCAT_MEMBER(o, _39),       \
    CONCAT_MEMBER(o, _40), CONCAT_MEMBER(o, _41), CONCAT_MEMBER(o, _42),       \
    CONCAT_MEMBER(o, _43), CONCAT_MEMBER(o, _44), CONCAT_MEMBER(o, _45),       \
    CONCAT_MEMBER(o, _46), CONCAT_MEMBER(o, _47), CONCAT_MEMBER(o, _48),       \
    CONCAT_MEMBER(o, _49), CONCAT_MEMBER(o, _50), CONCAT_MEMBER(o, _51),       \
    CONCAT_MEMBER(o, _52), CONCAT_MEMBER(o, _53), CONCAT_MEMBER(o, _54),       \
    CONCAT_MEMBER(o, _55), CONCAT_MEMBER(o, _56), CONCAT_MEMBER(o, _57),       \
    CONCAT_MEMBER(o, _58), CONCAT_MEMBER(o, _59), CONCAT_MEMBER(o, _60),       \
    CONCAT_MEMBER(o, _61), CONCAT_MEMBER(o, _62), CONCAT_MEMBER(o, _63),       \
    CONCAT_MEMBER(o, _64), CONCAT_MEMBER(o, _65), CONCAT_MEMBER(o, _66),       \
    CONCAT_MEMBER(o, _67), CONCAT_MEMBER(o, _68), CONCAT_MEMBER(o, _69),       \
    CONCAT_MEMBER(o, _70), CONCAT_MEMBER(o, _71), CONCAT_MEMBER(o, _72),       \
    CONCAT_MEMBER(o, _73), CONCAT_MEMBER(o, _74), CONCAT_MEMBER(o, _75),       \
    CONCAT_MEMBER(o, _76), CONCAT_MEMBER(o, _77), CONCAT_MEMBER(o, _78),       \
    CONCAT_MEMBER(o, _79), CONCAT_MEMBER(o, _80), CONCAT_MEMBER(o, _81),       \
    CONCAT_MEMBER(o, _82), CONCAT_MEMBER(o, _83), CONCAT_MEMBER(o, _84),       \
    CONCAT_MEMBER(o, _85), CONCAT_MEMBER(o, _86), CONCAT_MEMBER(o, _87),       \
    CONCAT_MEMBER(o, _88), CONCAT_MEMBER(o, _89), CONCAT_MEMBER(o, _90),       \
    CONCAT_MEMBER(o, _91), CONCAT_MEMBER(o, _92))
#define YLT_CALL_ARGS93(                                                       \
    f, o, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15,    \
    _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, \
    _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, \
    _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, \
    _61, _62, _63, _64, _65, _66, _67, _68, _69, _70, _71, _72, _73, _74, _75, \
    _76, _77, _78, _79, _80, _81, _82, _83, _84, _85, _86, _87, _88, _89, _90, \
    _91, _92, _93)                                                             \
  f(CONCAT_MEMBER(o, _1), CONCAT_MEMBER(o, _2), CONCAT_MEMBER(o, _3),          \
    CONCAT_MEMBER(o, _4), CONCAT_MEMBER(o, _5), CONCAT_MEMBER(o, _6),          \
    CONCAT_MEMBER(o, _7), CONCAT_MEMBER(o, _8), CONCAT_MEMBER(o, _9),          \
    CONCAT_MEMBER(o, _10), CONCAT_MEMBER(o, _11), CONCAT_MEMBER(o, _12),       \
    CONCAT_MEMBER(o, _13), CONCAT_MEMBER(o, _14), CONCAT_MEMBER(o, _15),       \
    CONCAT_MEMBER(o, _16), CONCAT_MEMBER(o, _17), CONCAT_MEMBER(o, _18),       \
    CONCAT_MEMBER(o, _19), CONCAT_MEMBER(o, _20), CONCAT_MEMBER(o, _21),       \
    CONCAT_MEMBER(o, _22), CONCAT_MEMBER(o, _23), CONCAT_MEMBER(o, _24),       \
    CONCAT_MEMBER(o, _25), CONCAT_MEMBER(o, _26), CONCAT_MEMBER(o, _27),       \
    CONCAT_MEMBER(o, _28), CONCAT_MEMBER(o, _29), CONCAT_MEMBER(o, _30),       \
    CONCAT_MEMBER(o, _31), CONCAT_MEMBER(o, _32), CONCAT_MEMBER(o, _33),       \
    CONCAT_MEMBER(o, _34), CONCAT_MEMBER(o, _35), CONCAT_MEMBER(o, _36),       \
    CONCAT_MEMBER(o, _37), CONCAT_MEMBER(o, _38), CONCAT_MEMBER(o, _39),       \
    CONCAT_MEMBER(o, _40), CONCAT_MEMBER(o, _41), CONCAT_MEMBER(o, _42),       \
    CONCAT_MEMBER(o, _43), CONCAT_MEMBER(o, _44), CONCAT_MEMBER(o, _45),       \
    CONCAT_MEMBER(o, _46), CONCAT_MEMBER(o, _47), CONCAT_MEMBER(o, _48),       \
    CONCAT_MEMBER(o, _49), CONCAT_MEMBER(o, _50), CONCAT_MEMBER(o, _51),       \
    CONCAT_MEMBER(o, _52), CONCAT_MEMBER(o, _53), CONCAT_MEMBER(o, _54),       \
    CONCAT_MEMBER(o, _55), CONCAT_MEMBER(o, _56), CONCAT_MEMBER(o, _57),       \
    CONCAT_MEMBER(o, _58), CONCAT_MEMBER(o, _59), CONCAT_MEMBER(o, _60),       \
    CONCAT_MEMBER(o, _61), CONCAT_MEMBER(o, _62), CONCAT_MEMBER(o, _63),       \
    CONCAT_MEMBER(o, _64), CONCAT_MEMBER(o, _65), CONCAT_MEMBER(o, _66),       \
    CONCAT_MEMBER(o, _67), CONCAT_MEMBER(o, _68), CONCAT_MEMBER(o, _69),       \
    CONCAT_MEMBER(o, _70), CONCAT_MEMBER(o, _71), CONCAT_MEMBER(o, _72),       \
    CONCAT_MEMBER(o, _73), CONCAT_MEMBER(o, _74), CONCAT_MEMBER(o, _75),       \
    CONCAT_MEMBER(o, _76), CONCAT_MEMBER(o, _77), CONCAT_MEMBER(o, _78),       \
    CONCAT_MEMBER(o, _79), CONCAT_MEMBER(o, _80), CONCAT_MEMBER(o, _81),       \
    CONCAT_MEMBER(o, _82), CONCAT_MEMBER(o, _83), CONCAT_MEMBER(o, _84),       \
    CONCAT_MEMBER(o, _85), CONCAT_MEMBER(o, _86), CONCAT_MEMBER(o, _87),       \
    CONCAT_MEMBER(o, _88), CONCAT_MEMBER(o, _89), CONCAT_MEMBER(o, _90),       \
    CONCAT_MEMBER(o, _91), CONCAT_MEMBER(o, _92), CONCAT_MEMBER(o, _93))
#define YLT_CALL_ARGS94(                                                       \
    f, o, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15,    \
    _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, \
    _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, \
    _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, \
    _61, _62, _63, _64, _65, _66, _67, _68, _69, _70, _71, _72, _73, _74, _75, \
    _76, _77, _78, _79, _80, _81, _82, _83, _84, _85, _86, _87, _88, _89, _90, \
    _91, _92, _93, _94)                                                        \
  f(CONCAT_MEMBER(o, _1), CONCAT_MEMBER(o, _2), CONCAT_MEMBER(o, _3),          \
    CONCAT_MEMBER(o, _4), CONCAT_MEMBER(o, _5), CONCAT_MEMBER(o, _6),          \
    CONCAT_MEMBER(o, _7), CONCAT_MEMBER(o, _8), CONCAT_MEMBER(o, _9),          \
    CONCAT_MEMBER(o, _10), CONCAT_MEMBER(o, _11), CONCAT_MEMBER(o, _12),       \
    CONCAT_MEMBER(o, _13), CONCAT_MEMBER(o, _14), CONCAT_MEMBER(o, _15),       \
    CONCAT_MEMBER(o, _16), CONCAT_MEMBER(o, _17), CONCAT_MEMBER(o, _18),       \
    CONCAT_MEMBER(o, _19), CONCAT_MEMBER(o, _20), CONCAT_MEMBER(o, _21),       \
    CONCAT_MEMBER(o, _22), CONCAT_MEMBER(o, _23), CONCAT_MEMBER(o, _24),       \
    CONCAT_MEMBER(o, _25), CONCAT_MEMBER(o, _26), CONCAT_MEMBER(o, _27),       \
    CONCAT_MEMBER(o, _28), CONCAT_MEMBER(o, _29), CONCAT_MEMBER(o, _30),       \
    CONCAT_MEMBER(o, _31), CONCAT_MEMBER(o, _32), CONCAT_MEMBER(o, _33),       \
    CONCAT_MEMBER(o, _34), CONCAT_MEMBER(o, _35), CONCAT_MEMBER(o, _36),       \
    CONCAT_MEMBER(o, _37), CONCAT_MEMBER(o, _38), CONCAT_MEMBER(o, _39),       \
    CONCAT_MEMBER(o, _40), CONCAT_MEMBER(o, _41), CONCAT_MEMBER(o, _42),       \
    CONCAT_MEMBER(o, _43), CONCAT_MEMBER(o, _44), CONCAT_MEMBER(o, _45),       \
    CONCAT_MEMBER(o, _46), CONCAT_MEMBER(o, _47), CONCAT_MEMBER(o, _48),       \
    CONCAT_MEMBER(o, _49), CONCAT_MEMBER(o, _50), CONCAT_MEMBER(o, _51),       \
    CONCAT_MEMBER(o, _52), CONCAT_MEMBER(o, _53), CONCAT_MEMBER(o, _54),       \
    CONCAT_MEMBER(o, _55), CONCAT_MEMBER(o, _56), CONCAT_MEMBER(o, _57),       \
    CONCAT_MEMBER(o, _58), CONCAT_MEMBER(o, _59), CONCAT_MEMBER(o, _60),       \
    CONCAT_MEMBER(o, _61), CONCAT_MEMBER(o, _62), CONCAT_MEMBER(o, _63),       \
    CONCAT_MEMBER(o, _64), CONCAT_MEMBER(o, _65), CONCAT_MEMBER(o, _66),       \
    CONCAT_MEMBER(o, _67), CONCAT_MEMBER(o, _68), CONCAT_MEMBER(o, _69),       \
    CONCAT_MEMBER(o, _70), CONCAT_MEMBER(o, _71), CONCAT_MEMBER(o, _72),       \
    CONCAT_MEMBER(o, _73), CONCAT_MEMBER(o, _74), CONCAT_MEMBER(o, _75),       \
    CONCAT_MEMBER(o, _76), CONCAT_MEMBER(o, _77), CONCAT_MEMBER(o, _78),       \
    CONCAT_MEMBER(o, _79), CONCAT_MEMBER(o, _80), CONCAT_MEMBER(o, _81),       \
    CONCAT_MEMBER(o, _82), CONCAT_MEMBER(o, _83), CONCAT_MEMBER(o, _84),       \
    CONCAT_MEMBER(o, _85), CONCAT_MEMBER(o, _86), CONCAT_MEMBER(o, _87),       \
    CONCAT_MEMBER(o, _88), CONCAT_MEMBER(o, _89), CONCAT_MEMBER(o, _90),       \
    CONCAT_MEMBER(o, _91), CONCAT_MEMBER(o, _92), CONCAT_MEMBER(o, _93),       \
    CONCAT_MEMBER(o, _94))
#define YLT_CALL_ARGS95(                                                       \
    f, o, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15,    \
    _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, \
    _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, \
    _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, \
    _61, _62, _63, _64, _65, _66, _67, _68, _69, _70, _71, _72, _73, _74, _75, \
    _76, _77, _78, _79, _80, _81, _82, _83, _84, _85, _86, _87, _88, _89, _90, \
    _91, _92, _93, _94, _95)                                                   \
  f(CONCAT_MEMBER(o, _1), CONCAT_MEMBER(o, _2), CONCAT_MEMBER(o, _3),          \
    CONCAT_MEMBER(o, _4), CONCAT_MEMBER(o, _5), CONCAT_MEMBER(o, _6),          \
    CONCAT_MEMBER(o, _7), CONCAT_MEMBER(o, _8), CONCAT_MEMBER(o, _9),          \
    CONCAT_MEMBER(o, _10), CONCAT_MEMBER(o, _11), CONCAT_MEMBER(o, _12),       \
    CONCAT_MEMBER(o, _13), CONCAT_MEMBER(o, _14), CONCAT_MEMBER(o, _15),       \
    CONCAT_MEMBER(o, _16), CONCAT_MEMBER(o, _17), CONCAT_MEMBER(o, _18),       \
    CONCAT_MEMBER(o, _19), CONCAT_MEMBER(o, _20), CONCAT_MEMBER(o, _21),       \
    CONCAT_MEMBER(o, _22), CONCAT_MEMBER(o, _23), CONCAT_MEMBER(o, _24),       \
    CONCAT_MEMBER(o, _25), CONCAT_MEMBER(o, _26), CONCAT_MEMBER(o, _27),       \
    CONCAT_MEMBER(o, _28), CONCAT_MEMBER(o, _29), CONCAT_MEMBER(o, _30),       \
    CONCAT_MEMBER(o, _31), CONCAT_MEMBER(o, _32), CONCAT_MEMBER(o, _33),       \
    CONCAT_MEMBER(o, _34), CONCAT_MEMBER(o, _35), CONCAT_MEMBER(o, _36),       \
    CONCAT_MEMBER(o, _37), CONCAT_MEMBER(o, _38), CONCAT_MEMBER(o, _39),       \
    CONCAT_MEMBER(o, _40), CONCAT_MEMBER(o, _41), CONCAT_MEMBER(o, _42),       \
    CONCAT_MEMBER(o, _43), CONCAT_MEMBER(o, _44), CONCAT_MEMBER(o, _45),       \
    CONCAT_MEMBER(o, _46), CONCAT_MEMBER(o, _47), CONCAT_MEMBER(o, _48),       \
    CONCAT_MEMBER(o, _49), CONCAT_MEMBER(o, _50), CONCAT_MEMBER(o, _51),       \
    CONCAT_MEMBER(o, _52), CONCAT_MEMBER(o, _53), CONCAT_MEMBER(o, _54),       \
    CONCAT_MEMBER(o, _55), CONCAT_MEMBER(o, _56), CONCAT_MEMBER(o, _57),       \
    CONCAT_MEMBER(o, _58), CONCAT_MEMBER(o, _59), CONCAT_MEMBER(o, _60),       \
    CONCAT_MEMBER(o, _61), CONCAT_MEMBER(o, _62), CONCAT_MEMBER(o, _63),       \
    CONCAT_MEMBER(o, _64), CONCAT_MEMBER(o, _65), CONCAT_MEMBER(o, _66),       \
    CONCAT_MEMBER(o, _67), CONCAT_MEMBER(o, _68), CONCAT_MEMBER(o, _69),       \
    CONCAT_MEMBER(o, _70), CONCAT_MEMBER(o, _71), CONCAT_MEMBER(o, _72),       \
    CONCAT_MEMBER(o, _73), CONCAT_MEMBER(o, _74), CONCAT_MEMBER(o, _75),       \
    CONCAT_MEMBER(o, _76), CONCAT_MEMBER(o, _77), CONCAT_MEMBER(o, _78),       \
    CONCAT_MEMBER(o, _79), CONCAT_MEMBER(o, _80), CONCAT_MEMBER(o, _81),       \
    CONCAT_MEMBER(o, _82), CONCAT_MEMBER(o, _83), CONCAT_MEMBER(o, _84),       \
    CONCAT_MEMBER(o, _85), CONCAT_MEMBER(o, _86), CONCAT_MEMBER(o, _87),       \
    CONCAT_MEMBER(o, _88), CONCAT_MEMBER(o, _89), CONCAT_MEMBER(o, _90),       \
    CONCAT_MEMBER(o, _91), CONCAT_MEMBER(o, _92), CONCAT_MEMBER(o, _93),       \
    CONCAT_MEMBER(o, _94), CONCAT_MEMBER(o, _95))
#define YLT_CALL_ARGS96(                                                       \
    f, o, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15,    \
    _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, \
    _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, \
    _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, \
    _61, _62, _63, _64, _65, _66, _67, _68, _69, _70, _71, _72, _73, _74, _75, \
    _76, _77, _78, _79, _80, _81, _82, _83, _84, _85, _86, _87, _88, _89, _90, \
    _91, _92, _93, _94, _95, _96)                                              \
  f(CONCAT_MEMBER(o, _1), CONCAT_MEMBER(o, _2), CONCAT_MEMBER(o, _3),          \
    CONCAT_MEMBER(o, _4), CONCAT_MEMBER(o, _5), CONCAT_MEMBER(o, _6),          \
    CONCAT_MEMBER(o, _7), CONCAT_MEMBER(o, _8), CONCAT_MEMBER(o, _9),          \
    CONCAT_MEMBER(o, _10), CONCAT_MEMBER(o, _11), CONCAT_MEMBER(o, _12),       \
    CONCAT_MEMBER(o, _13), CONCAT_MEMBER(o, _14), CONCAT_MEMBER(o, _15),       \
    CONCAT_MEMBER(o, _16), CONCAT_MEMBER(o, _17), CONCAT_MEMBER(o, _18),       \
    CONCAT_MEMBER(o, _19), CONCAT_MEMBER(o, _20), CONCAT_MEMBER(o, _21),       \
    CONCAT_MEMBER(o, _22), CONCAT_MEMBER(o, _23), CONCAT_MEMBER(o, _24),       \
    CONCAT_MEMBER(o, _25), CONCAT_MEMBER(o, _26), CONCAT_MEMBER(o, _27),       \
    CONCAT_MEMBER(o, _28), CONCAT_MEMBER(o, _29), CONCAT_MEMBER(o, _30),       \
    CONCAT_MEMBER(o, _31), CONCAT_MEMBER(o, _32), CONCAT_MEMBER(o, _33),       \
    CONCAT_MEMBER(o, _34), CONCAT_MEMBER(o, _35), CONCAT_MEMBER(o, _36),       \
    CONCAT_MEMBER(o, _37), CONCAT_MEMBER(o, _38), CONCAT_MEMBER(o, _39),       \
    CONCAT_MEMBER(o, _40), CONCAT_MEMBER(o, _41), CONCAT_MEMBER(o, _42),       \
    CONCAT_MEMBER(o, _43), CONCAT_MEMBER(o, _44), CONCAT_MEMBER(o, _45),       \
    CONCAT_MEMBER(o, _46), CONCAT_MEMBER(o, _47), CONCAT_MEMBER(o, _48),       \
    CONCAT_MEMBER(o, _49), CONCAT_MEMBER(o, _50), CONCAT_MEMBER(o, _51),       \
    CONCAT_MEMBER(o, _52), CONCAT_MEMBER(o, _53), CONCAT_MEMBER(o, _54),       \
    CONCAT_MEMBER(o, _55), CONCAT_MEMBER(o, _56), CONCAT_MEMBER(o, _57),       \
    CONCAT_MEMBER(o, _58), CONCAT_MEMBER(o, _59), CONCAT_MEMBER(o, _60),       \
    CONCAT_MEMBER(o, _61), CONCAT_MEMBER(o, _62), CONCAT_MEMBER(o, _63),       \
    CONCAT_MEMBER(o, _64), CONCAT_MEMBER(o, _65), CONCAT_MEMBER(o, _66),       \
    CONCAT_MEMBER(o, _67), CONCAT_MEMBER(o, _68), CONCAT_MEMBER(o, _69),       \
    CONCAT_MEMBER(o, _70), CONCAT_MEMBER(o, _71), CONCAT_MEMBER(o, _72),       \
    CONCAT_MEMBER(o, _73), CONCAT_MEMBER(o, _74), CONCAT_MEMBER(o, _75),       \
    CONCAT_MEMBER(o, _76), CONCAT_MEMBER(o, _77), CONCAT_MEMBER(o, _78),       \
    CONCAT_MEMBER(o, _79), CONCAT_MEMBER(o, _80), CONCAT_MEMBER(o, _81),       \
    CONCAT_MEMBER(o, _82), CONCAT_MEMBER(o, _83), CONCAT_MEMBER(o, _84),       \
    CONCAT_MEMBER(o, _85), CONCAT_MEMBER(o, _86), CONCAT_MEMBER(o, _87),       \
    CONCAT_MEMBER(o, _88), CONCAT_MEMBER(o, _89), CONCAT_MEMBER(o, _90),       \
    CONCAT_MEMBER(o, _91), CONCAT_MEMBER(o, _92), CONCAT_MEMBER(o, _93),       \
    CONCAT_MEMBER(o, _94), CONCAT_MEMBER(o, _95), CONCAT_MEMBER(o, _96))
#define YLT_CALL_ARGS97(                                                       \
    f, o, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15,    \
    _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, \
    _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, \
    _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, \
    _61, _62, _63, _64, _65, _66, _67, _68, _69, _70, _71, _72, _73, _74, _75, \
    _76, _77, _78, _79, _80, _81, _82, _83, _84, _85, _86, _87, _88, _89, _90, \
    _91, _92, _93, _94, _95, _96, _97)                                         \
  f(CONCAT_MEMBER(o, _1), CONCAT_MEMBER(o, _2), CONCAT_MEMBER(o, _3),          \
    CONCAT_MEMBER(o, _4), CONCAT_MEMBER(o, _5), CONCAT_MEMBER(o, _6),          \
    CONCAT_MEMBER(o, _7), CONCAT_MEMBER(o, _8), CONCAT_MEMBER(o, _9),          \
    CONCAT_MEMBER(o, _10), CONCAT_MEMBER(o, _11), CONCAT_MEMBER(o, _12),       \
    CONCAT_MEMBER(o, _13), CONCAT_MEMBER(o, _14), CONCAT_MEMBER(o, _15),       \
    CONCAT_MEMBER(o, _16), CONCAT_MEMBER(o, _17), CONCAT_MEMBER(o, _18),       \
    CONCAT_MEMBER(o, _19), CONCAT_MEMBER(o, _20), CONCAT_MEMBER(o, _21),       \
    CONCAT_MEMBER(o, _22), CONCAT_MEMBER(o, _23), CONCAT_MEMBER(o, _24),       \
    CONCAT_MEMBER(o, _25), CONCAT_MEMBER(o, _26), CONCAT_MEMBER(o, _27),       \
    CONCAT_MEMBER(o, _28), CONCAT_MEMBER(o, _29), CONCAT_MEMBER(o, _30),       \
    CONCAT_MEMBER(o, _31), CONCAT_MEMBER(o, _32), CONCAT_MEMBER(o, _33),       \
    CONCAT_MEMBER(o, _34), CONCAT_MEMBER(o, _35), CONCAT_MEMBER(o, _36),       \
    CONCAT_MEMBER(o, _37), CONCAT_MEMBER(o, _38), CONCAT_MEMBER(o, _39),       \
    CONCAT_MEMBER(o, _40), CONCAT_MEMBER(o, _41), CONCAT_MEMBER(o, _42),       \
    CONCAT_MEMBER(o, _43), CONCAT_MEMBER(o, _44), CONCAT_MEMBER(o, _45),       \
    CONCAT_MEMBER(o, _46), CONCAT_MEMBER(o, _47), CONCAT_MEMBER(o, _48),       \
    CONCAT_MEMBER(o, _49), CONCAT_MEMBER(o, _50), CONCAT_MEMBER(o, _51),       \
    CONCAT_MEMBER(o, _52), CONCAT_MEMBER(o, _53), CONCAT_MEMBER(o, _54),       \
    CONCAT_MEMBER(o, _55), CONCAT_MEMBER(o, _56), CONCAT_MEMBER(o, _57),       \
    CONCAT_MEMBER(o, _58), CONCAT_MEMBER(o, _59), CONCAT_MEMBER(o, _60),       \
    CONCAT_MEMBER(o, _61), CONCAT_MEMBER(o, _62), CONCAT_MEMBER(o, _63),       \
    CONCAT_MEMBER(o, _64), CONCAT_MEMBER(o, _65), CONCAT_MEMBER(o, _66),       \
    CONCAT_MEMBER(o, _67), CONCAT_MEMBER(o, _68), CONCAT_MEMBER(o, _69),       \
    CONCAT_MEMBER(o, _70), CONCAT_MEMBER(o, _71), CONCAT_MEMBER(o, _72),       \
    CONCAT_MEMBER(o, _73), CONCAT_MEMBER(o, _74), CONCAT_MEMBER(o, _75),       \
    CONCAT_MEMBER(o, _76), CONCAT_MEMBER(o, _77), CONCAT_MEMBER(o, _78),       \
    CONCAT_MEMBER(o, _79), CONCAT_MEMBER(o, _80), CONCAT_MEMBER(o, _81),       \
    CONCAT_MEMBER(o, _82), CONCAT_MEMBER(o, _83), CONCAT_MEMBER(o, _84),       \
    CONCAT_MEMBER(o, _85), CONCAT_MEMBER(o, _86), CONCAT_MEMBER(o, _87),       \
    CONCAT_MEMBER(o, _88), CONCAT_MEMBER(o, _89), CONCAT_MEMBER(o, _90),       \
    CONCAT_MEMBER(o, _91), CONCAT_MEMBER(o, _92), CONCAT_MEMBER(o, _93),       \
    CONCAT_MEMBER(o, _94), CONCAT_MEMBER(o, _95), CONCAT_MEMBER(o, _96),       \
    CONCAT_MEMBER(o, _97))
#define YLT_CALL_ARGS98(                                                       \
    f, o, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15,    \
    _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, \
    _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, \
    _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, \
    _61, _62, _63, _64, _65, _66, _67, _68, _69, _70, _71, _72, _73, _74, _75, \
    _76, _77, _78, _79, _80, _81, _82, _83, _84, _85, _86, _87, _88, _89, _90, \
    _91, _92, _93, _94, _95, _96, _97, _98)                                    \
  f(CONCAT_MEMBER(o, _1), CONCAT_MEMBER(o, _2), CONCAT_MEMBER(o, _3),          \
    CONCAT_MEMBER(o, _4), CONCAT_MEMBER(o, _5), CONCAT_MEMBER(o, _6),          \
    CONCAT_MEMBER(o, _7), CONCAT_MEMBER(o, _8), CONCAT_MEMBER(o, _9),          \
    CONCAT_MEMBER(o, _10), CONCAT_MEMBER(o, _11), CONCAT_MEMBER(o, _12),       \
    CONCAT_MEMBER(o, _13), CONCAT_MEMBER(o, _14), CONCAT_MEMBER(o, _15),       \
    CONCAT_MEMBER(o, _16), CONCAT_MEMBER(o, _17), CONCAT_MEMBER(o, _18),       \
    CONCAT_MEMBER(o, _19), CONCAT_MEMBER(o, _20), CONCAT_MEMBER(o, _21),       \
    CONCAT_MEMBER(o, _22), CONCAT_MEMBER(o, _23), CONCAT_MEMBER(o, _24),       \
    CONCAT_MEMBER(o, _25), CONCAT_MEMBER(o, _26), CONCAT_MEMBER(o, _27),       \
    CONCAT_MEMBER(o, _28), CONCAT_MEMBER(o, _29), CONCAT_MEMBER(o, _30),       \
    CONCAT_MEMBER(o, _31), CONCAT_MEMBER(o, _32), CONCAT_MEMBER(o, _33),       \
    CONCAT_MEMBER(o, _34), CONCAT_MEMBER(o, _35), CONCAT_MEMBER(o, _36),       \
    CONCAT_MEMBER(o, _37), CONCAT_MEMBER(o, _38), CONCAT_MEMBER(o, _39),       \
    CONCAT_MEMBER(o, _40), CONCAT_MEMBER(o, _41), CONCAT_MEMBER(o, _42),       \
    CONCAT_MEMBER(o, _43), CONCAT_MEMBER(o, _44), CONCAT_MEMBER(o, _45),       \
    CONCAT_MEMBER(o, _46), CONCAT_MEMBER(o, _47), CONCAT_MEMBER(o, _48),       \
    CONCAT_MEMBER(o, _49), CONCAT_MEMBER(o, _50), CONCAT_MEMBER(o, _51),       \
    CONCAT_MEMBER(o, _52), CONCAT_MEMBER(o, _53), CONCAT_MEMBER(o, _54),       \
    CONCAT_MEMBER(o, _55), CONCAT_MEMBER(o, _56), CONCAT_MEMBER(o, _57),       \
    CONCAT_MEMBER(o, _58), CONCAT_MEMBER(o, _59), CONCAT_MEMBER(o, _60),       \
    CONCAT_MEMBER(o, _61), CONCAT_MEMBER(o, _62), CONCAT_MEMBER(o, _63),       \
    CONCAT_MEMBER(o, _64), CONCAT_MEMBER(o, _65), CONCAT_MEMBER(o, _66),       \
    CONCAT_MEMBER(o, _67), CONCAT_MEMBER(o, _68), CONCAT_MEMBER(o, _69),       \
    CONCAT_MEMBER(o, _70), CONCAT_MEMBER(o, _71), CONCAT_MEMBER(o, _72),       \
    CONCAT_MEMBER(o, _73), CONCAT_MEMBER(o, _74), CONCAT_MEMBER(o, _75),       \
    CONCAT_MEMBER(o, _76), CONCAT_MEMBER(o, _77), CONCAT_MEMBER(o, _78),       \
    CONCAT_MEMBER(o, _79), CONCAT_MEMBER(o, _80), CONCAT_MEMBER(o, _81),       \
    CONCAT_MEMBER(o, _82), CONCAT_MEMBER(o, _83), CONCAT_MEMBER(o, _84),       \
    CONCAT_MEMBER(o, _85), CONCAT_MEMBER(o, _86), CONCAT_MEMBER(o, _87),       \
    CONCAT_MEMBER(o, _88), CONCAT_MEMBER(o, _89), CONCAT_MEMBER(o, _90),       \
    CONCAT_MEMBER(o, _91), CONCAT_MEMBER(o, _92), CONCAT_MEMBER(o, _93),       \
    CONCAT_MEMBER(o, _94), CONCAT_MEMBER(o, _95), CONCAT_MEMBER(o, _96),       \
    CONCAT_MEMBER(o, _97), CONCAT_MEMBER(o, _98))
#define YLT_CALL_ARGS99(                                                       \
    f, o, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15,    \
    _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, \
    _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, \
    _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, \
    _61, _62, _63, _64, _65, _66, _67, _68, _69, _70, _71, _72, _73, _74, _75, \
    _76, _77, _78, _79, _80, _81, _82, _83, _84, _85, _86, _87, _88, _89, _90, \
    _91, _92, _93, _94, _95, _96, _97, _98, _99)                               \
  f(CONCAT_MEMBER(o, _1), CONCAT_MEMBER(o, _2), CONCAT_MEMBER(o, _3),          \
    CONCAT_MEMBER(o, _4), CONCAT_MEMBER(o, _5), CONCAT_MEMBER(o, _6),          \
    CONCAT_MEMBER(o, _7), CONCAT_MEMBER(o, _8), CONCAT_MEMBER(o, _9),          \
    CONCAT_MEMBER(o, _10), CONCAT_MEMBER(o, _11), CONCAT_MEMBER(o, _12),       \
    CONCAT_MEMBER(o, _13), CONCAT_MEMBER(o, _14), CONCAT_MEMBER(o, _15),       \
    CONCAT_MEMBER(o, _16), CONCAT_MEMBER(o, _17), CONCAT_MEMBER(o, _18),       \
    CONCAT_MEMBER(o, _19), CONCAT_MEMBER(o, _20), CONCAT_MEMBER(o, _21),       \
    CONCAT_MEMBER(o, _22), CONCAT_MEMBER(o, _23), CONCAT_MEMBER(o, _24),       \
    CONCAT_MEMBER(o, _25), CONCAT_MEMBER(o, _26), CONCAT_MEMBER(o, _27),       \
    CONCAT_MEMBER(o, _28), CONCAT_MEMBER(o, _29), CONCAT_MEMBER(o, _30),       \
    CONCAT_MEMBER(o, _31), CONCAT_MEMBER(o, _32), CONCAT_MEMBER(o, _33),       \
    CONCAT_MEMBER(o, _34), CONCAT_MEMBER(o, _35), CONCAT_MEMBER(o, _36),       \
    CONCAT_MEMBER(o, _37), CONCAT_MEMBER(o, _38), CONCAT_MEMBER(o, _39),       \
    CONCAT_MEMBER(o, _40), CONCAT_MEMBER(o, _41), CONCAT_MEMBER(o, _42),       \
    CONCAT_MEMBER(o, _43), CONCAT_MEMBER(o, _44), CONCAT_MEMBER(o, _45),       \
    CONCAT_MEMBER(o, _46), CONCAT_MEMBER(o, _47), CONCAT_MEMBER(o, _48),       \
    CONCAT_MEMBER(o, _49), CONCAT_MEMBER(o, _50), CONCAT_MEMBER(o, _51),       \
    CONCAT_MEMBER(o, _52), CONCAT_MEMBER(o, _53), CONCAT_MEMBER(o, _54),       \
    CONCAT_MEMBER(o, _55), CONCAT_MEMBER(o, _56), CONCAT_MEMBER(o, _57),       \
    CONCAT_MEMBER(o, _58), CONCAT_MEMBER(o, _59), CONCAT_MEMBER(o, _60),       \
    CONCAT_MEMBER(o, _61), CONCAT_MEMBER(o, _62), CONCAT_MEMBER(o, _63),       \
    CONCAT_MEMBER(o, _64), CONCAT_MEMBER(o, _65), CONCAT_MEMBER(o, _66),       \
    CONCAT_MEMBER(o, _67), CONCAT_MEMBER(o, _68), CONCAT_MEMBER(o, _69),       \
    CONCAT_MEMBER(o, _70), CONCAT_MEMBER(o, _71), CONCAT_MEMBER(o, _72),       \
    CONCAT_MEMBER(o, _73), CONCAT_MEMBER(o, _74), CONCAT_MEMBER(o, _75),       \
    CONCAT_MEMBER(o, _76), CONCAT_MEMBER(o, _77), CONCAT_MEMBER(o, _78),       \
    CONCAT_MEMBER(o, _79), CONCAT_MEMBER(o, _80), CONCAT_MEMBER(o, _81),       \
    CONCAT_MEMBER(o, _82), CONCAT_MEMBER(o, _83), CONCAT_MEMBER(o, _84),       \
    CONCAT_MEMBER(o, _85), CONCAT_MEMBER(o, _86), CONCAT_MEMBER(o, _87),       \
    CONCAT_MEMBER(o, _88), CONCAT_MEMBER(o, _89), CONCAT_MEMBER(o, _90),       \
    CONCAT_MEMBER(o, _91), CONCAT_MEMBER(o, _92), CONCAT_MEMBER(o, _93),       \
    CONCAT_MEMBER(o, _94), CONCAT_MEMBER(o, _95), CONCAT_MEMBER(o, _96),       \
    CONCAT_MEMBER(o, _97), CONCAT_MEMBER(o, _98), CONCAT_MEMBER(o, _99))
#define YLT_CALL_ARGS100(                                                      \
    f, o, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15,    \
    _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, \
    _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, \
    _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, \
    _61, _62, _63, _64, _65, _66, _67, _68, _69, _70, _71, _72, _73, _74, _75, \
    _76, _77, _78, _79, _80, _81, _82, _83, _84, _85, _86, _87, _88, _89, _90, \
    _91, _92, _93, _94, _95, _96, _97, _98, _99, _100)                         \
  f(CONCAT_MEMBER(o, _1), CONCAT_MEMBER(o, _2), CONCAT_MEMBER(o, _3),          \
    CONCAT_MEMBER(o, _4), CONCAT_MEMBER(o, _5), CONCAT_MEMBER(o, _6),          \
    CONCAT_MEMBER(o, _7), CONCAT_MEMBER(o, _8), CONCAT_MEMBER(o, _9),          \
    CONCAT_MEMBER(o, _10), CONCAT_MEMBER(o, _11), CONCAT_MEMBER(o, _12),       \
    CONCAT_MEMBER(o, _13), CONCAT_MEMBER(o, _14), CONCAT_MEMBER(o, _15),       \
    CONCAT_MEMBER(o, _16), CONCAT_MEMBER(o, _17), CONCAT_MEMBER(o, _18),       \
    CONCAT_MEMBER(o, _19), CONCAT_MEMBER(o, _20), CONCAT_MEMBER(o, _21),       \
    CONCAT_MEMBER(o, _22), CONCAT_MEMBER(o, _23), CONCAT_MEMBER(o, _24),       \
    CONCAT_MEMBER(o, _25), CONCAT_MEMBER(o, _26), CONCAT_MEMBER(o, _27),       \
    CONCAT_MEMBER(o, _28), CONCAT_MEMBER(o, _29), CONCAT_MEMBER(o, _30),       \
    CONCAT_MEMBER(o, _31), CONCAT_MEMBER(o, _32), CONCAT_MEMBER(o, _33),       \
    CONCAT_MEMBER(o, _34), CONCAT_MEMBER(o, _35), CONCAT_MEMBER(o, _36),       \
    CONCAT_MEMBER(o, _37), CONCAT_MEMBER(o, _38), CONCAT_MEMBER(o, _39),       \
    CONCAT_MEMBER(o, _40), CONCAT_MEMBER(o, _41), CONCAT_MEMBER(o, _42),       \
    CONCAT_MEMBER(o, _43), CONCAT_MEMBER(o, _44), CONCAT_MEMBER(o, _45),       \
    CONCAT_MEMBER(o, _46), CONCAT_MEMBER(o, _47), CONCAT_MEMBER(o, _48),       \
    CONCAT_MEMBER(o, _49), CONCAT_MEMBER(o, _50), CONCAT_MEMBER(o, _51),       \
    CONCAT_MEMBER(o, _52), CONCAT_MEMBER(o, _53), CONCAT_MEMBER(o, _54),       \
    CONCAT_MEMBER(o, _55), CONCAT_MEMBER(o, _56), CONCAT_MEMBER(o, _57),       \
    CONCAT_MEMBER(o, _58), CONCAT_MEMBER(o, _59), CONCAT_MEMBER(o, _60),       \
    CONCAT_MEMBER(o, _61), CONCAT_MEMBER(o, _62), CONCAT_MEMBER(o, _63),       \
    CONCAT_MEMBER(o, _64), CONCAT_MEMBER(o, _65), CONCAT_MEMBER(o, _66),       \
    CONCAT_MEMBER(o, _67), CONCAT_MEMBER(o, _68), CONCAT_MEMBER(o, _69),       \
    CONCAT_MEMBER(o, _70), CONCAT_MEMBER(o, _71), CONCAT_MEMBER(o, _72),       \
    CONCAT_MEMBER(o, _73), CONCAT_MEMBER(o, _74), CONCAT_MEMBER(o, _75),       \
    CONCAT_MEMBER(o, _76), CONCAT_MEMBER(o, _77), CONCAT_MEMBER(o, _78),       \
    CONCAT_MEMBER(o, _79), CONCAT_MEMBER(o, _80), CONCAT_MEMBER(o, _81),       \
    CONCAT_MEMBER(o, _82), CONCAT_MEMBER(o, _83), CONCAT_MEMBER(o, _84),       \
    CONCAT_MEMBER(o, _85), CONCAT_MEMBER(o, _86), CONCAT_MEMBER(o, _87),       \
    CONCAT_MEMBER(o, _88), CONCAT_MEMBER(o, _89), CONCAT_MEMBER(o, _90),       \
    CONCAT_MEMBER(o, _91), CONCAT_MEMBER(o, _92), CONCAT_MEMBER(o, _93),       \
    CONCAT_MEMBER(o, _94), CONCAT_MEMBER(o, _95), CONCAT_MEMBER(o, _96),       \
    CONCAT_MEMBER(o, _97), CONCAT_MEMBER(o, _98), CONCAT_MEMBER(o, _99),       \
    CONCAT_MEMBER(o, _100))
#define YLT_CALL_ARGS101(                                                      \
    f, o, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15,    \
    _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, \
    _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, \
    _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, \
    _61, _62, _63, _64, _65, _66, _67, _68, _69, _70, _71, _72, _73, _74, _75, \
    _76, _77, _78, _79, _80, _81, _82, _83, _84, _85, _86, _87, _88, _89, _90, \
    _91, _92, _93, _94, _95, _96, _97, _98, _99, _100, _101)                   \
  f(CONCAT_MEMBER(o, _1), CONCAT_MEMBER(o, _2), CONCAT_MEMBER(o, _3),          \
    CONCAT_MEMBER(o, _4), CONCAT_MEMBER(o, _5), CONCAT_MEMBER(o, _6),          \
    CONCAT_MEMBER(o, _7), CONCAT_MEMBER(o, _8), CONCAT_MEMBER(o, _9),          \
    CONCAT_MEMBER(o, _10), CONCAT_MEMBER(o, _11), CONCAT_MEMBER(o, _12),       \
    CONCAT_MEMBER(o, _13), CONCAT_MEMBER(o, _14), CONCAT_MEMBER(o, _15),       \
    CONCAT_MEMBER(o, _16), CONCAT_MEMBER(o, _17), CONCAT_MEMBER(o, _18),       \
    CONCAT_MEMBER(o, _19), CONCAT_MEMBER(o, _20), CONCAT_MEMBER(o, _21),       \
    CONCAT_MEMBER(o, _22), CONCAT_MEMBER(o, _23), CONCAT_MEMBER(o, _24),       \
    CONCAT_MEMBER(o, _25), CONCAT_MEMBER(o, _26), CONCAT_MEMBER(o, _27),       \
    CONCAT_MEMBER(o, _28), CONCAT_MEMBER(o, _29), CONCAT_MEMBER(o, _30),       \
    CONCAT_MEMBER(o, _31), CONCAT_MEMBER(o, _32), CONCAT_MEMBER(o, _33),       \
    CONCAT_MEMBER(o, _34), CONCAT_MEMBER(o, _35), CONCAT_MEMBER(o, _36),       \
    CONCAT_MEMBER(o, _37), CONCAT_MEMBER(o, _38), CONCAT_MEMBER(o, _39),       \
    CONCAT_MEMBER(o, _40), CONCAT_MEMBER(o, _41), CONCAT_MEMBER(o, _42),       \
    CONCAT_MEMBER(o, _43), CONCAT_MEMBER(o, _44), CONCAT_MEMBER(o, _45),       \
    CONCAT_MEMBER(o, _46), CONCAT_MEMBER(o, _47), CONCAT_MEMBER(o, _48),       \
    CONCAT_MEMBER(o, _49), CONCAT_MEMBER(o, _50), CONCAT_MEMBER(o, _51),       \
    CONCAT_MEMBER(o, _52), CONCAT_MEMBER(o, _53), CONCAT_MEMBER(o, _54),       \
    CONCAT_MEMBER(o, _55), CONCAT_MEMBER(o, _56), CONCAT_MEMBER(o, _57),       \
    CONCAT_MEMBER(o, _58), CONCAT_MEMBER(o, _59), CONCAT_MEMBER(o, _60),       \
    CONCAT_MEMBER(o, _61), CONCAT_MEMBER(o, _62), CONCAT_MEMBER(o, _63),       \
    CONCAT_MEMBER(o, _64), CONCAT_MEMBER(o, _65), CONCAT_MEMBER(o, _66),       \
    CONCAT_MEMBER(o, _67), CONCAT_MEMBER(o, _68), CONCAT_MEMBER(o, _69),       \
    CONCAT_MEMBER(o, _70), CONCAT_MEMBER(o, _71), CONCAT_MEMBER(o, _72),       \
    CONCAT_MEMBER(o, _73), CONCAT_MEMBER(o, _74), CONCAT_MEMBER(o, _75),       \
    CONCAT_MEMBER(o, _76), CONCAT_MEMBER(o, _77), CONCAT_MEMBER(o, _78),       \
    CONCAT_MEMBER(o, _79), CONCAT_MEMBER(o, _80), CONCAT_MEMBER(o, _81),       \
    CONCAT_MEMBER(o, _82), CONCAT_MEMBER(o, _83), CONCAT_MEMBER(o, _84),       \
    CONCAT_MEMBER(o, _85), CONCAT_MEMBER(o, _86), CONCAT_MEMBER(o, _87),       \
    CONCAT_MEMBER(o, _88), CONCAT_MEMBER(o, _89), CONCAT_MEMBER(o, _90),       \
    CONCAT_MEMBER(o, _91), CONCAT_MEMBER(o, _92), CONCAT_MEMBER(o, _93),       \
    CONCAT_MEMBER(o, _94), CONCAT_MEMBER(o, _95), CONCAT_MEMBER(o, _96),       \
    CONCAT_MEMBER(o, _97), CONCAT_MEMBER(o, _98), CONCAT_MEMBER(o, _99),       \
    CONCAT_MEMBER(o, _100), CONCAT_MEMBER(o, _101))
#define YLT_CALL_ARGS102(                                                      \
    f, o, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15,    \
    _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, \
    _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, \
    _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, \
    _61, _62, _63, _64, _65, _66, _67, _68, _69, _70, _71, _72, _73, _74, _75, \
    _76, _77, _78, _79, _80, _81, _82, _83, _84, _85, _86, _87, _88, _89, _90, \
    _91, _92, _93, _94, _95, _96, _97, _98, _99, _100, _101, _102)             \
  f(CONCAT_MEMBER(o, _1), CONCAT_MEMBER(o, _2), CONCAT_MEMBER(o, _3),          \
    CONCAT_MEMBER(o, _4), CONCAT_MEMBER(o, _5), CONCAT_MEMBER(o, _6),          \
    CONCAT_MEMBER(o, _7), CONCAT_MEMBER(o, _8), CONCAT_MEMBER(o, _9),          \
    CONCAT_MEMBER(o, _10), CONCAT_MEMBER(o, _11), CONCAT_MEMBER(o, _12),       \
    CONCAT_MEMBER(o, _13), CONCAT_MEMBER(o, _14), CONCAT_MEMBER(o, _15),       \
    CONCAT_MEMBER(o, _16), CONCAT_MEMBER(o, _17), CONCAT_MEMBER(o, _18),       \
    CONCAT_MEMBER(o, _19), CONCAT_MEMBER(o, _20), CONCAT_MEMBER(o, _21),       \
    CONCAT_MEMBER(o, _22), CONCAT_MEMBER(o, _23), CONCAT_MEMBER(o, _24),       \
    CONCAT_MEMBER(o, _25), CONCAT_MEMBER(o, _26), CONCAT_MEMBER(o, _27),       \
    CONCAT_MEMBER(o, _28), CONCAT_MEMBER(o, _29), CONCAT_MEMBER(o, _30),       \
    CONCAT_MEMBER(o, _31), CONCAT_MEMBER(o, _32), CONCAT_MEMBER(o, _33),       \
    CONCAT_MEMBER(o, _34), CONCAT_MEMBER(o, _35), CONCAT_MEMBER(o, _36),       \
    CONCAT_MEMBER(o, _37), CONCAT_MEMBER(o, _38), CONCAT_MEMBER(o, _39),       \
    CONCAT_MEMBER(o, _40), CONCAT_MEMBER(o, _41), CONCAT_MEMBER(o, _42),       \
    CONCAT_MEMBER(o, _43), CONCAT_MEMBER(o, _44), CONCAT_MEMBER(o, _45),       \
    CONCAT_MEMBER(o, _46), CONCAT_MEMBER(o, _47), CONCAT_MEMBER(o, _48),       \
    CONCAT_MEMBER(o, _49), CONCAT_MEMBER(o, _50), CONCAT_MEMBER(o, _51),       \
    CONCAT_MEMBER(o, _52), CONCAT_MEMBER(o, _53), CONCAT_MEMBER(o, _54),       \
    CONCAT_MEMBER(o, _55), CONCAT_MEMBER(o, _56), CONCAT_MEMBER(o, _57),       \
    CONCAT_MEMBER(o, _58), CONCAT_MEMBER(o, _59), CONCAT_MEMBER(o, _60),       \
    CONCAT_MEMBER(o, _61), CONCAT_MEMBER(o, _62), CONCAT_MEMBER(o, _63),       \
    CONCAT_MEMBER(o, _64), CONCAT_MEMBER(o, _65), CONCAT_MEMBER(o, _66),       \
    CONCAT_MEMBER(o, _67), CONCAT_MEMBER(o, _68), CONCAT_MEMBER(o, _69),       \
    CONCAT_MEMBER(o, _70), CONCAT_MEMBER(o, _71), CONCAT_MEMBER(o, _72),       \
    CONCAT_MEMBER(o, _73), CONCAT_MEMBER(o, _74), CONCAT_MEMBER(o, _75),       \
    CONCAT_MEMBER(o, _76), CONCAT_MEMBER(o, _77), CONCAT_MEMBER(o, _78),       \
    CONCAT_MEMBER(o, _79), CONCAT_MEMBER(o, _80), CONCAT_MEMBER(o, _81),       \
    CONCAT_MEMBER(o, _82), CONCAT_MEMBER(o, _83), CONCAT_MEMBER(o, _84),       \
    CONCAT_MEMBER(o, _85), CONCAT_MEMBER(o, _86), CONCAT_MEMBER(o, _87),       \
    CONCAT_MEMBER(o, _88), CONCAT_MEMBER(o, _89), CONCAT_MEMBER(o, _90),       \
    CONCAT_MEMBER(o, _91), CONCAT_MEMBER(o, _92), CONCAT_MEMBER(o, _93),       \
    CONCAT_MEMBER(o, _94), CONCAT_MEMBER(o, _95), CONCAT_MEMBER(o, _96),       \
    CONCAT_MEMBER(o, _97), CONCAT_MEMBER(o, _98), CONCAT_MEMBER(o, _99),       \
    CONCAT_MEMBER(o, _100), CONCAT_MEMBER(o, _101), CONCAT_MEMBER(o, _102))
#define YLT_CALL_ARGS103(                                                      \
    f, o, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15,    \
    _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, \
    _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, \
    _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, \
    _61, _62, _63, _64, _65, _66, _67, _68, _69, _70, _71, _72, _73, _74, _75, \
    _76, _77, _78, _79, _80, _81, _82, _83, _84, _85, _86, _87, _88, _89, _90, \
    _91, _92, _93, _94, _95, _96, _97, _98, _99, _100, _101, _102, _103)       \
  f(CONCAT_MEMBER(o, _1), CONCAT_MEMBER(o, _2), CONCAT_MEMBER(o, _3),          \
    CONCAT_MEMBER(o, _4), CONCAT_MEMBER(o, _5), CONCAT_MEMBER(o, _6),          \
    CONCAT_MEMBER(o, _7), CONCAT_MEMBER(o, _8), CONCAT_MEMBER(o, _9),          \
    CONCAT_MEMBER(o, _10), CONCAT_MEMBER(o, _11), CONCAT_MEMBER(o, _12),       \
    CONCAT_MEMBER(o, _13), CONCAT_MEMBER(o, _14), CONCAT_MEMBER(o, _15),       \
    CONCAT_MEMBER(o, _16), CONCAT_MEMBER(o, _17), CONCAT_MEMBER(o, _18),       \
    CONCAT_MEMBER(o, _19), CONCAT_MEMBER(o, _20), CONCAT_MEMBER(o, _21),       \
    CONCAT_MEMBER(o, _22), CONCAT_MEMBER(o, _23), CONCAT_MEMBER(o, _24),       \
    CONCAT_MEMBER(o, _25), CONCAT_MEMBER(o, _26), CONCAT_MEMBER(o, _27),       \
    CONCAT_MEMBER(o, _28), CONCAT_MEMBER(o, _29), CONCAT_MEMBER(o, _30),       \
    CONCAT_MEMBER(o, _31), CONCAT_MEMBER(o, _32), CONCAT_MEMBER(o, _33),       \
    CONCAT_MEMBER(o, _34), CONCAT_MEMBER(o, _35), CONCAT_MEMBER(o, _36),       \
    CONCAT_MEMBER(o, _37), CONCAT_MEMBER(o, _38), CONCAT_MEMBER(o, _39),       \
    CONCAT_MEMBER(o, _40), CONCAT_MEMBER(o, _41), CONCAT_MEMBER(o, _42),       \
    CONCAT_MEMBER(o, _43), CONCAT_MEMBER(o, _44), CONCAT_MEMBER(o, _45),       \
    CONCAT_MEMBER(o, _46), CONCAT_MEMBER(o, _47), CONCAT_MEMBER(o, _48),       \
    CONCAT_MEMBER(o, _49), CONCAT_MEMBER(o, _50), CONCAT_MEMBER(o, _51),       \
    CONCAT_MEMBER(o, _52), CONCAT_MEMBER(o, _53), CONCAT_MEMBER(o, _54),       \
    CONCAT_MEMBER(o, _55), CONCAT_MEMBER(o, _56), CONCAT_MEMBER(o, _57),       \
    CONCAT_MEMBER(o, _58), CONCAT_MEMBER(o, _59), CONCAT_MEMBER(o, _60),       \
    CONCAT_MEMBER(o, _61), CONCAT_MEMBER(o, _62), CONCAT_MEMBER(o, _63),       \
    CONCAT_MEMBER(o, _64), CONCAT_MEMBER(o, _65), CONCAT_MEMBER(o, _66),       \
    CONCAT_MEMBER(o, _67), CONCAT_MEMBER(o, _68), CONCAT_MEMBER(o, _69),       \
    CONCAT_MEMBER(o, _70), CONCAT_MEMBER(o, _71), CONCAT_MEMBER(o, _72),       \
    CONCAT_MEMBER(o, _73), CONCAT_MEMBER(o, _74), CONCAT_MEMBER(o, _75),       \
    CONCAT_MEMBER(o, _76), CONCAT_MEMBER(o, _77), CONCAT_MEMBER(o, _78),       \
    CONCAT_MEMBER(o, _79), CONCAT_MEMBER(o, _80), CONCAT_MEMBER(o, _81),       \
    CONCAT_MEMBER(o, _82), CONCAT_MEMBER(o, _83), CONCAT_MEMBER(o, _84),       \
    CONCAT_MEMBER(o, _85), CONCAT_MEMBER(o, _86), CONCAT_MEMBER(o, _87),       \
    CONCAT_MEMBER(o, _88), CONCAT_MEMBER(o, _89), CONCAT_MEMBER(o, _90),       \
    CONCAT_MEMBER(o, _91), CONCAT_MEMBER(o, _92), CONCAT_MEMBER(o, _93),       \
    CONCAT_MEMBER(o, _94), CONCAT_MEMBER(o, _95), CONCAT_MEMBER(o, _96),       \
    CONCAT_MEMBER(o, _97), CONCAT_MEMBER(o, _98), CONCAT_MEMBER(o, _99),       \
    CONCAT_MEMBER(o, _100), CONCAT_MEMBER(o, _101), CONCAT_MEMBER(o, _102),    \
    CONCAT_MEMBER(o, _103))
#define YLT_CALL_ARGS104(                                                      \
    f, o, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15,    \
    _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, \
    _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, \
    _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, \
    _61, _62, _63, _64, _65, _66, _67, _68, _69, _70, _71, _72, _73, _74, _75, \
    _76, _77, _78, _79, _80, _81, _82, _83, _84, _85, _86, _87, _88, _89, _90, \
    _91, _92, _93, _94, _95, _96, _97, _98, _99, _100, _101, _102, _103, _104) \
  f(CONCAT_MEMBER(o, _1), CONCAT_MEMBER(o, _2), CONCAT_MEMBER(o, _3),          \
    CONCAT_MEMBER(o, _4), CONCAT_MEMBER(o, _5), CONCAT_MEMBER(o, _6),          \
    CONCAT_MEMBER(o, _7), CONCAT_MEMBER(o, _8), CONCAT_MEMBER(o, _9),          \
    CONCAT_MEMBER(o, _10), CONCAT_MEMBER(o, _11), CONCAT_MEMBER(o, _12),       \
    CONCAT_MEMBER(o, _13), CONCAT_MEMBER(o, _14), CONCAT_MEMBER(o, _15),       \
    CONCAT_MEMBER(o, _16), CONCAT_MEMBER(o, _17), CONCAT_MEMBER(o, _18),       \
    CONCAT_MEMBER(o, _19), CONCAT_MEMBER(o, _20), CONCAT_MEMBER(o, _21),       \
    CONCAT_MEMBER(o, _22), CONCAT_MEMBER(o, _23), CONCAT_MEMBER(o, _24),       \
    CONCAT_MEMBER(o, _25), CONCAT_MEMBER(o, _26), CONCAT_MEMBER(o, _27),       \
    CONCAT_MEMBER(o, _28), CONCAT_MEMBER(o, _29), CONCAT_MEMBER(o, _30),       \
    CONCAT_MEMBER(o, _31), CONCAT_MEMBER(o, _32), CONCAT_MEMBER(o, _33),       \
    CONCAT_MEMBER(o, _34), CONCAT_MEMBER(o, _35), CONCAT_MEMBER(o, _36),       \
    CONCAT_MEMBER(o, _37), CONCAT_MEMBER(o, _38), CONCAT_MEMBER(o, _39),       \
    CONCAT_MEMBER(o, _40), CONCAT_MEMBER(o, _41), CONCAT_MEMBER(o, _42),       \
    CONCAT_MEMBER(o, _43), CONCAT_MEMBER(o, _44), CONCAT_MEMBER(o, _45),       \
    CONCAT_MEMBER(o, _46), CONCAT_MEMBER(o, _47), CONCAT_MEMBER(o, _48),       \
    CONCAT_MEMBER(o, _49), CONCAT_MEMBER(o, _50), CONCAT_MEMBER(o, _51),       \
    CONCAT_MEMBER(o, _52), CONCAT_MEMBER(o, _53), CONCAT_MEMBER(o, _54),       \
    CONCAT_MEMBER(o, _55), CONCAT_MEMBER(o, _56), CONCAT_MEMBER(o, _57),       \
    CONCAT_MEMBER(o, _58), CONCAT_MEMBER(o, _59), CONCAT_MEMBER(o, _60),       \
    CONCAT_MEMBER(o, _61), CONCAT_MEMBER(o, _62), CONCAT_MEMBER(o, _63),       \
    CONCAT_MEMBER(o, _64), CONCAT_MEMBER(o, _65), CONCAT_MEMBER(o, _66),       \
    CONCAT_MEMBER(o, _67), CONCAT_MEMBER(o, _68), CONCAT_MEMBER(o, _69),       \
    CONCAT_MEMBER(o, _70), CONCAT_MEMBER(o, _71), CONCAT_MEMBER(o, _72),       \
    CONCAT_MEMBER(o, _73), CONCAT_MEMBER(o, _74), CONCAT_MEMBER(o, _75),       \
    CONCAT_MEMBER(o, _76), CONCAT_MEMBER(o, _77), CONCAT_MEMBER(o, _78),       \
    CONCAT_MEMBER(o, _79), CONCAT_MEMBER(o, _80), CONCAT_MEMBER(o, _81),       \
    CONCAT_MEMBER(o, _82), CONCAT_MEMBER(o, _83), CONCAT_MEMBER(o, _84),       \
    CONCAT_MEMBER(o, _85), CONCAT_MEMBER(o, _86), CONCAT_MEMBER(o, _87),       \
    CONCAT_MEMBER(o, _88), CONCAT_MEMBER(o, _89), CONCAT_MEMBER(o, _90),       \
    CONCAT_MEMBER(o, _91), CONCAT_MEMBER(o, _92), CONCAT_MEMBER(o, _93),       \
    CONCAT_MEMBER(o, _94), CONCAT_MEMBER(o, _95), CONCAT_MEMBER(o, _96),       \
    CONCAT_MEMBER(o, _97), CONCAT_MEMBER(o, _98), CONCAT_MEMBER(o, _99),       \
    CONCAT_MEMBER(o, _100), CONCAT_MEMBER(o, _101), CONCAT_MEMBER(o, _102),    \
    CONCAT_MEMBER(o, _103), CONCAT_MEMBER(o, _104))
#define YLT_CALL_ARGS105(                                                      \
    f, o, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15,    \
    _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, \
    _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, \
    _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, \
    _61, _62, _63, _64, _65, _66, _67, _68, _69, _70, _71, _72, _73, _74, _75, \
    _76, _77, _78, _79, _80, _81, _82, _83, _84, _85, _86, _87, _88, _89, _90, \
    _91, _92, _93, _94, _95, _96, _97, _98, _99, _100, _101, _102, _103, _104, \
    _105)                                                                      \
  f(CONCAT_MEMBER(o, _1), CONCAT_MEMBER(o, _2), CONCAT_MEMBER(o, _3),          \
    CONCAT_MEMBER(o, _4), CONCAT_MEMBER(o, _5), CONCAT_MEMBER(o, _6),          \
    CONCAT_MEMBER(o, _7), CONCAT_MEMBER(o, _8), CONCAT_MEMBER(o, _9),          \
    CONCAT_MEMBER(o, _10), CONCAT_MEMBER(o, _11), CONCAT_MEMBER(o, _12),       \
    CONCAT_MEMBER(o, _13), CONCAT_MEMBER(o, _14), CONCAT_MEMBER(o, _15),       \
    CONCAT_MEMBER(o, _16), CONCAT_MEMBER(o, _17), CONCAT_MEMBER(o, _18),       \
    CONCAT_MEMBER(o, _19), CONCAT_MEMBER(o, _20), CONCAT_MEMBER(o, _21),       \
    CONCAT_MEMBER(o, _22), CONCAT_MEMBER(o, _23), CONCAT_MEMBER(o, _24),       \
    CONCAT_MEMBER(o, _25), CONCAT_MEMBER(o, _26), CONCAT_MEMBER(o, _27),       \
    CONCAT_MEMBER(o, _28), CONCAT_MEMBER(o, _29), CONCAT_MEMBER(o, _30),       \
    CONCAT_MEMBER(o, _31), CONCAT_MEMBER(o, _32), CONCAT_MEMBER(o, _33),       \
    CONCAT_MEMBER(o, _34), CONCAT_MEMBER(o, _35), CONCAT_MEMBER(o, _36),       \
    CONCAT_MEMBER(o, _37), CONCAT_MEMBER(o, _38), CONCAT_MEMBER(o, _39),       \
    CONCAT_MEMBER(o, _40), CONCAT_MEMBER(o, _41), CONCAT_MEMBER(o, _42),       \
    CONCAT_MEMBER(o, _43), CONCAT_MEMBER(o, _44), CONCAT_MEMBER(o, _45),       \
    CONCAT_MEMBER(o, _46), CONCAT_MEMBER(o, _47), CONCAT_MEMBER(o, _48),       \
    CONCAT_MEMBER(o, _49), CONCAT_MEMBER(o, _50), CONCAT_MEMBER(o, _51),       \
    CONCAT_MEMBER(o, _52), CONCAT_MEMBER(o, _53), CONCAT_MEMBER(o, _54),       \
    CONCAT_MEMBER(o, _55), CONCAT_MEMBER(o, _56), CONCAT_MEMBER(o, _57),       \
    CONCAT_MEMBER(o, _58), CONCAT_MEMBER(o, _59), CONCAT_MEMBER(o, _60),       \
    CONCAT_MEMBER(o, _61), CONCAT_MEMBER(o, _62), CONCAT_MEMBER(o, _63),       \
    CONCAT_MEMBER(o, _64), CONCAT_MEMBER(o, _65), CONCAT_MEMBER(o, _66),       \
    CONCAT_MEMBER(o, _67), CONCAT_MEMBER(o, _68), CONCAT_MEMBER(o, _69),       \
    CONCAT_MEMBER(o, _70), CONCAT_MEMBER(o, _71), CONCAT_MEMBER(o, _72),       \
    CONCAT_MEMBER(o, _73), CONCAT_MEMBER(o, _74), CONCAT_MEMBER(o, _75),       \
    CONCAT_MEMBER(o, _76), CONCAT_MEMBER(o, _77), CONCAT_MEMBER(o, _78),       \
    CONCAT_MEMBER(o, _79), CONCAT_MEMBER(o, _80), CONCAT_MEMBER(o, _81),       \
    CONCAT_MEMBER(o, _82), CONCAT_MEMBER(o, _83), CONCAT_MEMBER(o, _84),       \
    CONCAT_MEMBER(o, _85), CONCAT_MEMBER(o, _86), CONCAT_MEMBER(o, _87),       \
    CONCAT_MEMBER(o, _88), CONCAT_MEMBER(o, _89), CONCAT_MEMBER(o, _90),       \
    CONCAT_MEMBER(o, _91), CONCAT_MEMBER(o, _92), CONCAT_MEMBER(o, _93),       \
    CONCAT_MEMBER(o, _94), CONCAT_MEMBER(o, _95), CONCAT_MEMBER(o, _96),       \
    CONCAT_MEMBER(o, _97), CONCAT_MEMBER(o, _98), CONCAT_MEMBER(o, _99),       \
    CONCAT_MEMBER(o, _100), CONCAT_MEMBER(o, _101), CONCAT_MEMBER(o, _102),    \
    CONCAT_MEMBER(o, _103), CONCAT_MEMBER(o, _104), CONCAT_MEMBER(o, _105))
#define YLT_CALL_ARGS106(                                                      \
    f, o, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15,    \
    _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, \
    _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, \
    _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, \
    _61, _62, _63, _64, _65, _66, _67, _68, _69, _70, _71, _72, _73, _74, _75, \
    _76, _77, _78, _79, _80, _81, _82, _83, _84, _85, _86, _87, _88, _89, _90, \
    _91, _92, _93, _94, _95, _96, _97, _98, _99, _100, _101, _102, _103, _104, \
    _105, _106)                                                                \
  f(CONCAT_MEMBER(o, _1), CONCAT_MEMBER(o, _2), CONCAT_MEMBER(o, _3),          \
    CONCAT_MEMBER(o, _4), CONCAT_MEMBER(o, _5), CONCAT_MEMBER(o, _6),          \
    CONCAT_MEMBER(o, _7), CONCAT_MEMBER(o, _8), CONCAT_MEMBER(o, _9),          \
    CONCAT_MEMBER(o, _10), CONCAT_MEMBER(o, _11), CONCAT_MEMBER(o, _12),       \
    CONCAT_MEMBER(o, _13), CONCAT_MEMBER(o, _14), CONCAT_MEMBER(o, _15),       \
    CONCAT_MEMBER(o, _16), CONCAT_MEMBER(o, _17), CONCAT_MEMBER(o, _18),       \
    CONCAT_MEMBER(o, _19), CONCAT_MEMBER(o, _20), CONCAT_MEMBER(o, _21),       \
    CONCAT_MEMBER(o, _22), CONCAT_MEMBER(o, _23), CONCAT_MEMBER(o, _24),       \
    CONCAT_MEMBER(o, _25), CONCAT_MEMBER(o, _26), CONCAT_MEMBER(o, _27),       \
    CONCAT_MEMBER(o, _28), CONCAT_MEMBER(o, _29), CONCAT_MEMBER(o, _30),       \
    CONCAT_MEMBER(o, _31), CONCAT_MEMBER(o, _32), CONCAT_MEMBER(o, _33),       \
    CONCAT_MEMBER(o, _34), CONCAT_MEMBER(o, _35), CONCAT_MEMBER(o, _36),       \
    CONCAT_MEMBER(o, _37), CONCAT_MEMBER(o, _38), CONCAT_MEMBER(o, _39),       \
    CONCAT_MEMBER(o, _40), CONCAT_MEMBER(o, _41), CONCAT_MEMBER(o, _42),       \
    CONCAT_MEMBER(o, _43), CONCAT_MEMBER(o, _44), CONCAT_MEMBER(o, _45),       \
    CONCAT_MEMBER(o, _46), CONCAT_MEMBER(o, _47), CONCAT_MEMBER(o, _48),       \
    CONCAT_MEMBER(o, _49), CONCAT_MEMBER(o, _50), CONCAT_MEMBER(o, _51),       \
    CONCAT_MEMBER(o, _52), CONCAT_MEMBER(o, _53), CONCAT_MEMBER(o, _54),       \
    CONCAT_MEMBER(o, _55), CONCAT_MEMBER(o, _56), CONCAT_MEMBER(o, _57),       \
    CONCAT_MEMBER(o, _58), CONCAT_MEMBER(o, _59), CONCAT_MEMBER(o, _60),       \
    CONCAT_MEMBER(o, _61), CONCAT_MEMBER(o, _62), CONCAT_MEMBER(o, _63),       \
    CONCAT_MEMBER(o, _64), CONCAT_MEMBER(o, _65), CONCAT_MEMBER(o, _66),       \
    CONCAT_MEMBER(o, _67), CONCAT_MEMBER(o, _68), CONCAT_MEMBER(o, _69),       \
    CONCAT_MEMBER(o, _70), CONCAT_MEMBER(o, _71), CONCAT_MEMBER(o, _72),       \
    CONCAT_MEMBER(o, _73), CONCAT_MEMBER(o, _74), CONCAT_MEMBER(o, _75),       \
    CONCAT_MEMBER(o, _76), CONCAT_MEMBER(o, _77), CONCAT_MEMBER(o, _78),       \
    CONCAT_MEMBER(o, _79), CONCAT_MEMBER(o, _80), CONCAT_MEMBER(o, _81),       \
    CONCAT_MEMBER(o, _82), CONCAT_MEMBER(o, _83), CONCAT_MEMBER(o, _84),       \
    CONCAT_MEMBER(o, _85), CONCAT_MEMBER(o, _86), CONCAT_MEMBER(o, _87),       \
    CONCAT_MEMBER(o, _88), CONCAT_MEMBER(o, _89), CONCAT_MEMBER(o, _90),       \
    CONCAT_MEMBER(o, _91), CONCAT_MEMBER(o, _92), CONCAT_MEMBER(o, _93),       \
    CONCAT_MEMBER(o, _94), CONCAT_MEMBER(o, _95), CONCAT_MEMBER(o, _96),       \
    CONCAT_MEMBER(o, _97), CONCAT_MEMBER(o, _98), CONCAT_MEMBER(o, _99),       \
    CONCAT_MEMBER(o, _100), CONCAT_MEMBER(o, _101), CONCAT_MEMBER(o, _102),    \
    CONCAT_MEMBER(o, _103), CONCAT_MEMBER(o, _104), CONCAT_MEMBER(o, _105),    \
    CONCAT_MEMBER(o, _106))
#define YLT_CALL_ARGS107(                                                      \
    f, o, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15,    \
    _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, \
    _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, \
    _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, \
    _61, _62, _63, _64, _65, _66, _67, _68, _69, _70, _71, _72, _73, _74, _75, \
    _76, _77, _78, _79, _80, _81, _82, _83, _84, _85, _86, _87, _88, _89, _90, \
    _91, _92, _93, _94, _95, _96, _97, _98, _99, _100, _101, _102, _103, _104, \
    _105, _106, _107)                                                          \
  f(CONCAT_MEMBER(o, _1), CONCAT_MEMBER(o, _2), CONCAT_MEMBER(o, _3),          \
    CONCAT_MEMBER(o, _4), CONCAT_MEMBER(o, _5), CONCAT_MEMBER(o, _6),          \
    CONCAT_MEMBER(o, _7), CONCAT_MEMBER(o, _8), CONCAT_MEMBER(o, _9),          \
    CONCAT_MEMBER(o, _10), CONCAT_MEMBER(o, _11), CONCAT_MEMBER(o, _12),       \
    CONCAT_MEMBER(o, _13), CONCAT_MEMBER(o, _14), CONCAT_MEMBER(o, _15),       \
    CONCAT_MEMBER(o, _16), CONCAT_MEMBER(o, _17), CONCAT_MEMBER(o, _18),       \
    CONCAT_MEMBER(o, _19), CONCAT_MEMBER(o, _20), CONCAT_MEMBER(o, _21),       \
    CONCAT_MEMBER(o, _22), CONCAT_MEMBER(o, _23), CONCAT_MEMBER(o, _24),       \
    CONCAT_MEMBER(o, _25), CONCAT_MEMBER(o, _26), CONCAT_MEMBER(o, _27),       \
    CONCAT_MEMBER(o, _28), CONCAT_MEMBER(o, _29), CONCAT_MEMBER(o, _30),       \
    CONCAT_MEMBER(o, _31), CONCAT_MEMBER(o, _32), CONCAT_MEMBER(o, _33),       \
    CONCAT_MEMBER(o, _34), CONCAT_MEMBER(o, _35), CONCAT_MEMBER(o, _36),       \
    CONCAT_MEMBER(o, _37), CONCAT_MEMBER(o, _38), CONCAT_MEMBER(o, _39),       \
    CONCAT_MEMBER(o, _40), CONCAT_MEMBER(o, _41), CONCAT_MEMBER(o, _42),       \
    CONCAT_MEMBER(o, _43), CONCAT_MEMBER(o, _44), CONCAT_MEMBER(o, _45),       \
    CONCAT_MEMBER(o, _46), CONCAT_MEMBER(o, _47), CONCAT_MEMBER(o, _48),       \
    CONCAT_MEMBER(o, _49), CONCAT_MEMBER(o, _50), CONCAT_MEMBER(o, _51),       \
    CONCAT_MEMBER(o, _52), CONCAT_MEMBER(o, _53), CONCAT_MEMBER(o, _54),       \
    CONCAT_MEMBER(o, _55), CONCAT_MEMBER(o, _56), CONCAT_MEMBER(o, _57),       \
    CONCAT_MEMBER(o, _58), CONCAT_MEMBER(o, _59), CONCAT_MEMBER(o, _60),       \
    CONCAT_MEMBER(o, _61), CONCAT_MEMBER(o, _62), CONCAT_MEMBER(o, _63),       \
    CONCAT_MEMBER(o, _64), CONCAT_MEMBER(o, _65), CONCAT_MEMBER(o, _66),       \
    CONCAT_MEMBER(o, _67), CONCAT_MEMBER(o, _68), CONCAT_MEMBER(o, _69),       \
    CONCAT_MEMBER(o, _70), CONCAT_MEMBER(o, _71), CONCAT_MEMBER(o, _72),       \
    CONCAT_MEMBER(o, _73), CONCAT_MEMBER(o, _74), CONCAT_MEMBER(o, _75),       \
    CONCAT_MEMBER(o, _76), CONCAT_MEMBER(o, _77), CONCAT_MEMBER(o, _78),       \
    CONCAT_MEMBER(o, _79), CONCAT_MEMBER(o, _80), CONCAT_MEMBER(o, _81),       \
    CONCAT_MEMBER(o, _82), CONCAT_MEMBER(o, _83), CONCAT_MEMBER(o, _84),       \
    CONCAT_MEMBER(o, _85), CONCAT_MEMBER(o, _86), CONCAT_MEMBER(o, _87),       \
    CONCAT_MEMBER(o, _88), CONCAT_MEMBER(o, _89), CONCAT_MEMBER(o, _90),       \
    CONCAT_MEMBER(o, _91), CONCAT_MEMBER(o, _92), CONCAT_MEMBER(o, _93),       \
    CONCAT_MEMBER(o, _94), CONCAT_MEMBER(o, _95), CONCAT_MEMBER(o, _96),       \
    CONCAT_MEMBER(o, _97), CONCAT_MEMBER(o, _98), CONCAT_MEMBER(o, _99),       \
    CONCAT_MEMBER(o, _100), CONCAT_MEMBER(o, _101), CONCAT_MEMBER(o, _102),    \
    CONCAT_MEMBER(o, _103), CONCAT_MEMBER(o, _104), CONCAT_MEMBER(o, _105),    \
    CONCAT_MEMBER(o, _106), CONCAT_MEMBER(o, _107))
#define YLT_CALL_ARGS108(                                                      \
    f, o, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15,    \
    _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, \
    _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, \
    _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, \
    _61, _62, _63, _64, _65, _66, _67, _68, _69, _70, _71, _72, _73, _74, _75, \
    _76, _77, _78, _79, _80, _81, _82, _83, _84, _85, _86, _87, _88, _89, _90, \
    _91, _92, _93, _94, _95, _96, _97, _98, _99, _100, _101, _102, _103, _104, \
    _105, _106, _107, _108)                                                    \
  f(CONCAT_MEMBER(o, _1), CONCAT_MEMBER(o, _2), CONCAT_MEMBER(o, _3),          \
    CONCAT_MEMBER(o, _4), CONCAT_MEMBER(o, _5), CONCAT_MEMBER(o, _6),          \
    CONCAT_MEMBER(o, _7), CONCAT_MEMBER(o, _8), CONCAT_MEMBER(o, _9),          \
    CONCAT_MEMBER(o, _10), CONCAT_MEMBER(o, _11), CONCAT_MEMBER(o, _12),       \
    CONCAT_MEMBER(o, _13), CONCAT_MEMBER(o, _14), CONCAT_MEMBER(o, _15),       \
    CONCAT_MEMBER(o, _16), CONCAT_MEMBER(o, _17), CONCAT_MEMBER(o, _18),       \
    CONCAT_MEMBER(o, _19), CONCAT_MEMBER(o, _20), CONCAT_MEMBER(o, _21),       \
    CONCAT_MEMBER(o, _22), CONCAT_MEMBER(o, _23), CONCAT_MEMBER(o, _24),       \
    CONCAT_MEMBER(o, _25), CONCAT_MEMBER(o, _26), CONCAT_MEMBER(o, _27),       \
    CONCAT_MEMBER(o, _28), CONCAT_MEMBER(o, _29), CONCAT_MEMBER(o, _30),       \
    CONCAT_MEMBER(o, _31), CONCAT_MEMBER(o, _32), CONCAT_MEMBER(o, _33),       \
    CONCAT_MEMBER(o, _34), CONCAT_MEMBER(o, _35), CONCAT_MEMBER(o, _36),       \
    CONCAT_MEMBER(o, _37), CONCAT_MEMBER(o, _38), CONCAT_MEMBER(o, _39),       \
    CONCAT_MEMBER(o, _40), CONCAT_MEMBER(o, _41), CONCAT_MEMBER(o, _42),       \
    CONCAT_MEMBER(o, _43), CONCAT_MEMBER(o, _44), CONCAT_MEMBER(o, _45),       \
    CONCAT_MEMBER(o, _46), CONCAT_MEMBER(o, _47), CONCAT_MEMBER(o, _48),       \
    CONCAT_MEMBER(o, _49), CONCAT_MEMBER(o, _50), CONCAT_MEMBER(o, _51),       \
    CONCAT_MEMBER(o, _52), CONCAT_MEMBER(o, _53), CONCAT_MEMBER(o, _54),       \
    CONCAT_MEMBER(o, _55), CONCAT_MEMBER(o, _56), CONCAT_MEMBER(o, _57),       \
    CONCAT_MEMBER(o, _58), CONCAT_MEMBER(o, _59), CONCAT_MEMBER(o, _60),       \
    CONCAT_MEMBER(o, _61), CONCAT_MEMBER(o, _62), CONCAT_MEMBER(o, _63),       \
    CONCAT_MEMBER(o, _64), CONCAT_MEMBER(o, _65), CONCAT_MEMBER(o, _66),       \
    CONCAT_MEMBER(o, _67), CONCAT_MEMBER(o, _68), CONCAT_MEMBER(o, _69),       \
    CONCAT_MEMBER(o, _70), CONCAT_MEMBER(o, _71), CONCAT_MEMBER(o, _72),       \
    CONCAT_MEMBER(o, _73), CONCAT_MEMBER(o, _74), CONCAT_MEMBER(o, _75),       \
    CONCAT_MEMBER(o, _76), CONCAT_MEMBER(o, _77), CONCAT_MEMBER(o, _78),       \
    CONCAT_MEMBER(o, _79), CONCAT_MEMBER(o, _80), CONCAT_MEMBER(o, _81),       \
    CONCAT_MEMBER(o, _82), CONCAT_MEMBER(o, _83), CONCAT_MEMBER(o, _84),       \
    CONCAT_MEMBER(o, _85), CONCAT_MEMBER(o, _86), CONCAT_MEMBER(o, _87),       \
    CONCAT_MEMBER(o, _88), CONCAT_MEMBER(o, _89), CONCAT_MEMBER(o, _90),       \
    CONCAT_MEMBER(o, _91), CONCAT_MEMBER(o, _92), CONCAT_MEMBER(o, _93),       \
    CONCAT_MEMBER(o, _94), CONCAT_MEMBER(o, _95), CONCAT_MEMBER(o, _96),       \
    CONCAT_MEMBER(o, _97), CONCAT_MEMBER(o, _98), CONCAT_MEMBER(o, _99),       \
    CONCAT_MEMBER(o, _100), CONCAT_MEMBER(o, _101), CONCAT_MEMBER(o, _102),    \
    CONCAT_MEMBER(o, _103), CONCAT_MEMBER(o, _104), CONCAT_MEMBER(o, _105),    \
    CONCAT_MEMBER(o, _106), CONCAT_MEMBER(o, _107), CONCAT_MEMBER(o, _108))
#define YLT_CALL_ARGS109(                                                      \
    f, o, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15,    \
    _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, \
    _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, \
    _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, \
    _61, _62, _63, _64, _65, _66, _67, _68, _69, _70, _71, _72, _73, _74, _75, \
    _76, _77, _78, _79, _80, _81, _82, _83, _84, _85, _86, _87, _88, _89, _90, \
    _91, _92, _93, _94, _95, _96, _97, _98, _99, _100, _101, _102, _103, _104, \
    _105, _106, _107, _108, _109)                                              \
  f(CONCAT_MEMBER(o, _1), CONCAT_MEMBER(o, _2), CONCAT_MEMBER(o, _3),          \
    CONCAT_MEMBER(o, _4), CONCAT_MEMBER(o, _5), CONCAT_MEMBER(o, _6),          \
    CONCAT_MEMBER(o, _7), CONCAT_MEMBER(o, _8), CONCAT_MEMBER(o, _9),          \
    CONCAT_MEMBER(o, _10), CONCAT_MEMBER(o, _11), CONCAT_MEMBER(o, _12),       \
    CONCAT_MEMBER(o, _13), CONCAT_MEMBER(o, _14), CONCAT_MEMBER(o, _15),       \
    CONCAT_MEMBER(o, _16), CONCAT_MEMBER(o, _17), CONCAT_MEMBER(o, _18),       \
    CONCAT_MEMBER(o, _19), CONCAT_MEMBER(o, _20), CONCAT_MEMBER(o, _21),       \
    CONCAT_MEMBER(o, _22), CONCAT_MEMBER(o, _23), CONCAT_MEMBER(o, _24),       \
    CONCAT_MEMBER(o, _25), CONCAT_MEMBER(o, _26), CONCAT_MEMBER(o, _27),       \
    CONCAT_MEMBER(o, _28), CONCAT_MEMBER(o, _29), CONCAT_MEMBER(o, _30),       \
    CONCAT_MEMBER(o, _31), CONCAT_MEMBER(o, _32), CONCAT_MEMBER(o, _33),       \
    CONCAT_MEMBER(o, _34), CONCAT_MEMBER(o, _35), CONCAT_MEMBER(o, _36),       \
    CONCAT_MEMBER(o, _37), CONCAT_MEMBER(o, _38), CONCAT_MEMBER(o, _39),       \
    CONCAT_MEMBER(o, _40), CONCAT_MEMBER(o, _41), CONCAT_MEMBER(o, _42),       \
    CONCAT_MEMBER(o, _43), CONCAT_MEMBER(o, _44), CONCAT_MEMBER(o, _45),       \
    CONCAT_MEMBER(o, _46), CONCAT_MEMBER(o, _47), CONCAT_MEMBER(o, _48),       \
    CONCAT_MEMBER(o, _49), CONCAT_MEMBER(o, _50), CONCAT_MEMBER(o, _51),       \
    CONCAT_MEMBER(o, _52), CONCAT_MEMBER(o, _53), CONCAT_MEMBER(o, _54),       \
    CONCAT_MEMBER(o, _55), CONCAT_MEMBER(o, _56), CONCAT_MEMBER(o, _57),       \
    CONCAT_MEMBER(o, _58), CONCAT_MEMBER(o, _59), CONCAT_MEMBER(o, _60),       \
    CONCAT_MEMBER(o, _61), CONCAT_MEMBER(o, _62), CONCAT_MEMBER(o, _63),       \
    CONCAT_MEMBER(o, _64), CONCAT_MEMBER(o, _65), CONCAT_MEMBER(o, _66),       \
    CONCAT_MEMBER(o, _67), CONCAT_MEMBER(o, _68), CONCAT_MEMBER(o, _69),       \
    CONCAT_MEMBER(o, _70), CONCAT_MEMBER(o, _71), CONCAT_MEMBER(o, _72),       \
    CONCAT_MEMBER(o, _73), CONCAT_MEMBER(o, _74), CONCAT_MEMBER(o, _75),       \
    CONCAT_MEMBER(o, _76), CONCAT_MEMBER(o, _77), CONCAT_MEMBER(o, _78),       \
    CONCAT_MEMBER(o, _79), CONCAT_MEMBER(o, _80), CONCAT_MEMBER(o, _81),       \
    CONCAT_MEMBER(o, _82), CONCAT_MEMBER(o, _83), CONCAT_MEMBER(o, _84),       \
    CONCAT_MEMBER(o, _85), CONCAT_MEMBER(o, _86), CONCAT_MEMBER(o, _87),       \
    CONCAT_MEMBER(o, _88), CONCAT_MEMBER(o, _89), CONCAT_MEMBER(o, _90),       \
    CONCAT_MEMBER(o, _91), CONCAT_MEMBER(o, _92), CONCAT_MEMBER(o, _93),       \
    CONCAT_MEMBER(o, _94), CONCAT_MEMBER(o, _95), CONCAT_MEMBER(o, _96),       \
    CONCAT_MEMBER(o, _97), CONCAT_MEMBER(o, _98), CONCAT_MEMBER(o, _99),       \
    CONCAT_MEMBER(o, _100), CONCAT_MEMBER(o, _101), CONCAT_MEMBER(o, _102),    \
    CONCAT_MEMBER(o, _103), CONCAT_MEMBER(o, _104), CONCAT_MEMBER(o, _105),    \
    CONCAT_MEMBER(o, _106), CONCAT_MEMBER(o, _107), CONCAT_MEMBER(o, _108),    \
    CONCAT_MEMBER(o, _109))
#define YLT_CALL_ARGS110(                                                      \
    f, o, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15,    \
    _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, \
    _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, \
    _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, \
    _61, _62, _63, _64, _65, _66, _67, _68, _69, _70, _71, _72, _73, _74, _75, \
    _76, _77, _78, _79, _80, _81, _82, _83, _84, _85, _86, _87, _88, _89, _90, \
    _91, _92, _93, _94, _95, _96, _97, _98, _99, _100, _101, _102, _103, _104, \
    _105, _106, _107, _108, _109, _110)                                        \
  f(CONCAT_MEMBER(o, _1), CONCAT_MEMBER(o, _2), CONCAT_MEMBER(o, _3),          \
    CONCAT_MEMBER(o, _4), CONCAT_MEMBER(o, _5), CONCAT_MEMBER(o, _6),          \
    CONCAT_MEMBER(o, _7), CONCAT_MEMBER(o, _8), CONCAT_MEMBER(o, _9),          \
    CONCAT_MEMBER(o, _10), CONCAT_MEMBER(o, _11), CONCAT_MEMBER(o, _12),       \
    CONCAT_MEMBER(o, _13), CONCAT_MEMBER(o, _14), CONCAT_MEMBER(o, _15),       \
    CONCAT_MEMBER(o, _16), CONCAT_MEMBER(o, _17), CONCAT_MEMBER(o, _18),       \
    CONCAT_MEMBER(o, _19), CONCAT_MEMBER(o, _20), CONCAT_MEMBER(o, _21),       \
    CONCAT_MEMBER(o, _22), CONCAT_MEMBER(o, _23), CONCAT_MEMBER(o, _24),       \
    CONCAT_MEMBER(o, _25), CONCAT_MEMBER(o, _26), CONCAT_MEMBER(o, _27),       \
    CONCAT_MEMBER(o, _28), CONCAT_MEMBER(o, _29), CONCAT_MEMBER(o, _30),       \
    CONCAT_MEMBER(o, _31), CONCAT_MEMBER(o, _32), CONCAT_MEMBER(o, _33),       \
    CONCAT_MEMBER(o, _34), CONCAT_MEMBER(o, _35), CONCAT_MEMBER(o, _36),       \
    CONCAT_MEMBER(o, _37), CONCAT_MEMBER(o, _38), CONCAT_MEMBER(o, _39),       \
    CONCAT_MEMBER(o, _40), CONCAT_MEMBER(o, _41), CONCAT_MEMBER(o, _42),       \
    CONCAT_MEMBER(o, _43), CONCAT_MEMBER(o, _44), CONCAT_MEMBER(o, _45),       \
    CONCAT_MEMBER(o, _46), CONCAT_MEMBER(o, _47), CONCAT_MEMBER(o, _48),       \
    CONCAT_MEMBER(o, _49), CONCAT_MEMBER(o, _50), CONCAT_MEMBER(o, _51),       \
    CONCAT_MEMBER(o, _52), CONCAT_MEMBER(o, _53), CONCAT_MEMBER(o, _54),       \
    CONCAT_MEMBER(o, _55), CONCAT_MEMBER(o, _56), CONCAT_MEMBER(o, _57),       \
    CONCAT_MEMBER(o, _58), CONCAT_MEMBER(o, _59), CONCAT_MEMBER(o, _60),       \
    CONCAT_MEMBER(o, _61), CONCAT_MEMBER(o, _62), CONCAT_MEMBER(o, _63),       \
    CONCAT_MEMBER(o, _64), CONCAT_MEMBER(o, _65), CONCAT_MEMBER(o, _66),       \
    CONCAT_MEMBER(o, _67), CONCAT_MEMBER(o, _68), CONCAT_MEMBER(o, _69),       \
    CONCAT_MEMBER(o, _70), CONCAT_MEMBER(o, _71), CONCAT_MEMBER(o, _72),       \
    CONCAT_MEMBER(o, _73), CONCAT_MEMBER(o, _74), CONCAT_MEMBER(o, _75),       \
    CONCAT_MEMBER(o, _76), CONCAT_MEMBER(o, _77), CONCAT_MEMBER(o, _78),       \
    CONCAT_MEMBER(o, _79), CONCAT_MEMBER(o, _80), CONCAT_MEMBER(o, _81),       \
    CONCAT_MEMBER(o, _82), CONCAT_MEMBER(o, _83), CONCAT_MEMBER(o, _84),       \
    CONCAT_MEMBER(o, _85), CONCAT_MEMBER(o, _86), CONCAT_MEMBER(o, _87),       \
    CONCAT_MEMBER(o, _88), CONCAT_MEMBER(o, _89), CONCAT_MEMBER(o, _90),       \
    CONCAT_MEMBER(o, _91), CONCAT_MEMBER(o, _92), CONCAT_MEMBER(o, _93),       \
    CONCAT_MEMBER(o, _94), CONCAT_MEMBER(o, _95), CONCAT_MEMBER(o, _96),       \
    CONCAT_MEMBER(o, _97), CONCAT_MEMBER(o, _98), CONCAT_MEMBER(o, _99),       \
    CONCAT_MEMBER(o, _100), CONCAT_MEMBER(o, _101), CONCAT_MEMBER(o, _102),    \
    CONCAT_MEMBER(o, _103), CONCAT_MEMBER(o, _104), CONCAT_MEMBER(o, _105),    \
    CONCAT_MEMBER(o, _106), CONCAT_MEMBER(o, _107), CONCAT_MEMBER(o, _108),    \
    CONCAT_MEMBER(o, _109), CONCAT_MEMBER(o, _110))
#define YLT_CALL_ARGS111(                                                      \
    f, o, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15,    \
    _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, \
    _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, \
    _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, \
    _61, _62, _63, _64, _65, _66, _67, _68, _69, _70, _71, _72, _73, _74, _75, \
    _76, _77, _78, _79, _80, _81, _82, _83, _84, _85, _86, _87, _88, _89, _90, \
    _91, _92, _93, _94, _95, _96, _97, _98, _99, _100, _101, _102, _103, _104, \
    _105, _106, _107, _108, _109, _110, _111)                                  \
  f(CONCAT_MEMBER(o, _1), CONCAT_MEMBER(o, _2), CONCAT_MEMBER(o, _3),          \
    CONCAT_MEMBER(o, _4), CONCAT_MEMBER(o, _5), CONCAT_MEMBER(o, _6),          \
    CONCAT_MEMBER(o, _7), CONCAT_MEMBER(o, _8), CONCAT_MEMBER(o, _9),          \
    CONCAT_MEMBER(o, _10), CONCAT_MEMBER(o, _11), CONCAT_MEMBER(o, _12),       \
    CONCAT_MEMBER(o, _13), CONCAT_MEMBER(o, _14), CONCAT_MEMBER(o, _15),       \
    CONCAT_MEMBER(o, _16), CONCAT_MEMBER(o, _17), CONCAT_MEMBER(o, _18),       \
    CONCAT_MEMBER(o, _19), CONCAT_MEMBER(o, _20), CONCAT_MEMBER(o, _21),       \
    CONCAT_MEMBER(o, _22), CONCAT_MEMBER(o, _23), CONCAT_MEMBER(o, _24),       \
    CONCAT_MEMBER(o, _25), CONCAT_MEMBER(o, _26), CONCAT_MEMBER(o, _27),       \
    CONCAT_MEMBER(o, _28), CONCAT_MEMBER(o, _29), CONCAT_MEMBER(o, _30),       \
    CONCAT_MEMBER(o, _31), CONCAT_MEMBER(o, _32), CONCAT_MEMBER(o, _33),       \
    CONCAT_MEMBER(o, _34), CONCAT_MEMBER(o, _35), CONCAT_MEMBER(o, _36),       \
    CONCAT_MEMBER(o, _37), CONCAT_MEMBER(o, _38), CONCAT_MEMBER(o, _39),       \
    CONCAT_MEMBER(o, _40), CONCAT_MEMBER(o, _41), CONCAT_MEMBER(o, _42),       \
    CONCAT_MEMBER(o, _43), CONCAT_MEMBER(o, _44), CONCAT_MEMBER(o, _45),       \
    CONCAT_MEMBER(o, _46), CONCAT_MEMBER(o, _47), CONCAT_MEMBER(o, _48),       \
    CONCAT_MEMBER(o, _49), CONCAT_MEMBER(o, _50), CONCAT_MEMBER(o, _51),       \
    CONCAT_MEMBER(o, _52), CONCAT_MEMBER(o, _53), CONCAT_MEMBER(o, _54),       \
    CONCAT_MEMBER(o, _55), CONCAT_MEMBER(o, _56), CONCAT_MEMBER(o, _57),       \
    CONCAT_MEMBER(o, _58), CONCAT_MEMBER(o, _59), CONCAT_MEMBER(o, _60),       \
    CONCAT_MEMBER(o, _61), CONCAT_MEMBER(o, _62), CONCAT_MEMBER(o, _63),       \
    CONCAT_MEMBER(o, _64), CONCAT_MEMBER(o, _65), CONCAT_MEMBER(o, _66),       \
    CONCAT_MEMBER(o, _67), CONCAT_MEMBER(o, _68), CONCAT_MEMBER(o, _69),       \
    CONCAT_MEMBER(o, _70), CONCAT_MEMBER(o, _71), CONCAT_MEMBER(o, _72),       \
    CONCAT_MEMBER(o, _73), CONCAT_MEMBER(o, _74), CONCAT_MEMBER(o, _75),       \
    CONCAT_MEMBER(o, _76), CONCAT_MEMBER(o, _77), CONCAT_MEMBER(o, _78),       \
    CONCAT_MEMBER(o, _79), CONCAT_MEMBER(o, _80), CONCAT_MEMBER(o, _81),       \
    CONCAT_MEMBER(o, _82), CONCAT_MEMBER(o, _83), CONCAT_MEMBER(o, _84),       \
    CONCAT_MEMBER(o, _85), CONCAT_MEMBER(o, _86), CONCAT_MEMBER(o, _87),       \
    CONCAT_MEMBER(o, _88), CONCAT_MEMBER(o, _89), CONCAT_MEMBER(o, _90),       \
    CONCAT_MEMBER(o, _91), CONCAT_MEMBER(o, _92), CONCAT_MEMBER(o, _93),       \
    CONCAT_MEMBER(o, _94), CONCAT_MEMBER(o, _95), CONCAT_MEMBER(o, _96),       \
    CONCAT_MEMBER(o, _97), CONCAT_MEMBER(o, _98), CONCAT_MEMBER(o, _99),       \
    CONCAT_MEMBER(o, _100), CONCAT_MEMBER(o, _101), CONCAT_MEMBER(o, _102),    \
    CONCAT_MEMBER(o, _103), CONCAT_MEMBER(o, _104), CONCAT_MEMBER(o, _105),    \
    CONCAT_MEMBER(o, _106), CONCAT_MEMBER(o, _107), CONCAT_MEMBER(o, _108),    \
    CONCAT_MEMBER(o, _109), CONCAT_MEMBER(o, _110), CONCAT_MEMBER(o, _111))
#define YLT_CALL_ARGS112(                                                      \
    f, o, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15,    \
    _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, \
    _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, \
    _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, \
    _61, _62, _63, _64, _65, _66, _67, _68, _69, _70, _71, _72, _73, _74, _75, \
    _76, _77, _78, _79, _80, _81, _82, _83, _84, _85, _86, _87, _88, _89, _90, \
    _91, _92, _93, _94, _95, _96, _97, _98, _99, _100, _101, _102, _103, _104, \
    _105, _106, _107, _108, _109, _110, _111, _112)                            \
  f(CONCAT_MEMBER(o, _1), CONCAT_MEMBER(o, _2), CONCAT_MEMBER(o, _3),          \
    CONCAT_MEMBER(o, _4), CONCAT_MEMBER(o, _5), CONCAT_MEMBER(o, _6),          \
    CONCAT_MEMBER(o, _7), CONCAT_MEMBER(o, _8), CONCAT_MEMBER(o, _9),          \
    CONCAT_MEMBER(o, _10), CONCAT_MEMBER(o, _11), CONCAT_MEMBER(o, _12),       \
    CONCAT_MEMBER(o, _13), CONCAT_MEMBER(o, _14), CONCAT_MEMBER(o, _15),       \
    CONCAT_MEMBER(o, _16), CONCAT_MEMBER(o, _17), CONCAT_MEMBER(o, _18),       \
    CONCAT_MEMBER(o, _19), CONCAT_MEMBER(o, _20), CONCAT_MEMBER(o, _21),       \
    CONCAT_MEMBER(o, _22), CONCAT_MEMBER(o, _23), CONCAT_MEMBER(o, _24),       \
    CONCAT_MEMBER(o, _25), CONCAT_MEMBER(o, _26), CONCAT_MEMBER(o, _27),       \
    CONCAT_MEMBER(o, _28), CONCAT_MEMBER(o, _29), CONCAT_MEMBER(o, _30),       \
    CONCAT_MEMBER(o, _31), CONCAT_MEMBER(o, _32), CONCAT_MEMBER(o, _33),       \
    CONCAT_MEMBER(o, _34), CONCAT_MEMBER(o, _35), CONCAT_MEMBER(o, _36),       \
    CONCAT_MEMBER(o, _37), CONCAT_MEMBER(o, _38), CONCAT_MEMBER(o, _39),       \
    CONCAT_MEMBER(o, _40), CONCAT_MEMBER(o, _41), CONCAT_MEMBER(o, _42),       \
    CONCAT_MEMBER(o, _43), CONCAT_MEMBER(o, _44), CONCAT_MEMBER(o, _45),       \
    CONCAT_MEMBER(o, _46), CONCAT_MEMBER(o, _47), CONCAT_MEMBER(o, _48),       \
    CONCAT_MEMBER(o, _49), CONCAT_MEMBER(o, _50), CONCAT_MEMBER(o, _51),       \
    CONCAT_MEMBER(o, _52), CONCAT_MEMBER(o, _53), CONCAT_MEMBER(o, _54),       \
    CONCAT_MEMBER(o, _55), CONCAT_MEMBER(o, _56), CONCAT_MEMBER(o, _57),       \
    CONCAT_MEMBER(o, _58), CONCAT_MEMBER(o, _59), CONCAT_MEMBER(o, _60),       \
    CONCAT_MEMBER(o, _61), CONCAT_MEMBER(o, _62), CONCAT_MEMBER(o, _63),       \
    CONCAT_MEMBER(o, _64), CONCAT_MEMBER(o, _65), CONCAT_MEMBER(o, _66),       \
    CONCAT_MEMBER(o, _67), CONCAT_MEMBER(o, _68), CONCAT_MEMBER(o, _69),       \
    CONCAT_MEMBER(o, _70), CONCAT_MEMBER(o, _71), CONCAT_MEMBER(o, _72),       \
    CONCAT_MEMBER(o, _73), CONCAT_MEMBER(o, _74), CONCAT_MEMBER(o, _75),       \
    CONCAT_MEMBER(o, _76), CONCAT_MEMBER(o, _77), CONCAT_MEMBER(o, _78),       \
    CONCAT_MEMBER(o, _79), CONCAT_MEMBER(o, _80), CONCAT_MEMBER(o, _81),       \
    CONCAT_MEMBER(o, _82), CONCAT_MEMBER(o, _83), CONCAT_MEMBER(o, _84),       \
    CONCAT_MEMBER(o, _85), CONCAT_MEMBER(o, _86), CONCAT_MEMBER(o, _87),       \
    CONCAT_MEMBER(o, _88), CONCAT_MEMBER(o, _89), CONCAT_MEMBER(o, _90),       \
    CONCAT_MEMBER(o, _91), CONCAT_MEMBER(o, _92), CONCAT_MEMBER(o, _93),       \
    CONCAT_MEMBER(o, _94), CONCAT_MEMBER(o, _95), CONCAT_MEMBER(o, _96),       \
    CONCAT_MEMBER(o, _97), CONCAT_MEMBER(o, _98), CONCAT_MEMBER(o, _99),       \
    CONCAT_MEMBER(o, _100), CONCAT_MEMBER(o, _101), CONCAT_MEMBER(o, _102),    \
    CONCAT_MEMBER(o, _103), CONCAT_MEMBER(o, _104), CONCAT_MEMBER(o, _105),    \
    CONCAT_MEMBER(o, _106), CONCAT_MEMBER(o, _107), CONCAT_MEMBER(o, _108),    \
    CONCAT_MEMBER(o, _109), CONCAT_MEMBER(o, _110), CONCAT_MEMBER(o, _111),    \
    CONCAT_MEMBER(o, _112))
#define YLT_CALL_ARGS113(                                                      \
    f, o, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15,    \
    _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, \
    _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, \
    _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, \
    _61, _62, _63, _64, _65, _66, _67, _68, _69, _70, _71, _72, _73, _74, _75, \
    _76, _77, _78, _79, _80, _81, _82, _83, _84, _85, _86, _87, _88, _89, _90, \
    _91, _92, _93, _94, _95, _96, _97, _98, _99, _100, _101, _102, _103, _104, \
    _105, _106, _107, _108, _109, _110, _111, _112, _113)                      \
  f(CONCAT_MEMBER(o, _1), CONCAT_MEMBER(o, _2), CONCAT_MEMBER(o, _3),          \
    CONCAT_MEMBER(o, _4), CONCAT_MEMBER(o, _5), CONCAT_MEMBER(o, _6),          \
    CONCAT_MEMBER(o, _7), CONCAT_MEMBER(o, _8), CONCAT_MEMBER(o, _9),          \
    CONCAT_MEMBER(o, _10), CONCAT_MEMBER(o, _11), CONCAT_MEMBER(o, _12),       \
    CONCAT_MEMBER(o, _13), CONCAT_MEMBER(o, _14), CONCAT_MEMBER(o, _15),       \
    CONCAT_MEMBER(o, _16), CONCAT_MEMBER(o, _17), CONCAT_MEMBER(o, _18),       \
    CONCAT_MEMBER(o, _19), CONCAT_MEMBER(o, _20), CONCAT_MEMBER(o, _21),       \
    CONCAT_MEMBER(o, _22), CONCAT_MEMBER(o, _23), CONCAT_MEMBER(o, _24),       \
    CONCAT_MEMBER(o, _25), CONCAT_MEMBER(o, _26), CONCAT_MEMBER(o, _27),       \
    CONCAT_MEMBER(o, _28), CONCAT_MEMBER(o, _29), CONCAT_MEMBER(o, _30),       \
    CONCAT_MEMBER(o, _31), CONCAT_MEMBER(o, _32), CONCAT_MEMBER(o, _33),       \
    CONCAT_MEMBER(o, _34), CONCAT_MEMBER(o, _35), CONCAT_MEMBER(o, _36),       \
    CONCAT_MEMBER(o, _37), CONCAT_MEMBER(o, _38), CONCAT_MEMBER(o, _39),       \
    CONCAT_MEMBER(o, _40), CONCAT_MEMBER(o, _41), CONCAT_MEMBER(o, _42),       \
    CONCAT_MEMBER(o, _43), CONCAT_MEMBER(o, _44), CONCAT_MEMBER(o, _45),       \
    CONCAT_MEMBER(o, _46), CONCAT_MEMBER(o, _47), CONCAT_MEMBER(o, _48),       \
    CONCAT_MEMBER(o, _49), CONCAT_MEMBER(o, _50), CONCAT_MEMBER(o, _51),       \
    CONCAT_MEMBER(o, _52), CONCAT_MEMBER(o, _53), CONCAT_MEMBER(o, _54),       \
    CONCAT_MEMBER(o, _55), CONCAT_MEMBER(o, _56), CONCAT_MEMBER(o, _57),       \
    CONCAT_MEMBER(o, _58), CONCAT_MEMBER(o, _59), CONCAT_MEMBER(o, _60),       \
    CONCAT_MEMBER(o, _61), CONCAT_MEMBER(o, _62), CONCAT_MEMBER(o, _63),       \
    CONCAT_MEMBER(o, _64), CONCAT_MEMBER(o, _65), CONCAT_MEMBER(o, _66),       \
    CONCAT_MEMBER(o, _67), CONCAT_MEMBER(o, _68), CONCAT_MEMBER(o, _69),       \
    CONCAT_MEMBER(o, _70), CONCAT_MEMBER(o, _71), CONCAT_MEMBER(o, _72),       \
    CONCAT_MEMBER(o, _73), CONCAT_MEMBER(o, _74), CONCAT_MEMBER(o, _75),       \
    CONCAT_MEMBER(o, _76), CONCAT_MEMBER(o, _77), CONCAT_MEMBER(o, _78),       \
    CONCAT_MEMBER(o, _79), CONCAT_MEMBER(o, _80), CONCAT_MEMBER(o, _81),       \
    CONCAT_MEMBER(o, _82), CONCAT_MEMBER(o, _83), CONCAT_MEMBER(o, _84),       \
    CONCAT_MEMBER(o, _85), CONCAT_MEMBER(o, _86), CONCAT_MEMBER(o, _87),       \
    CONCAT_MEMBER(o, _88), CONCAT_MEMBER(o, _89), CONCAT_MEMBER(o, _90),       \
    CONCAT_MEMBER(o, _91), CONCAT_MEMBER(o, _92), CONCAT_MEMBER(o, _93),       \
    CONCAT_MEMBER(o, _94), CONCAT_MEMBER(o, _95), CONCAT_MEMBER(o, _96),       \
    CONCAT_MEMBER(o, _97), CONCAT_MEMBER(o, _98), CONCAT_MEMBER(o, _99),       \
    CONCAT_MEMBER(o, _100), CONCAT_MEMBER(o, _101), CONCAT_MEMBER(o, _102),    \
    CONCAT_MEMBER(o, _103), CONCAT_MEMBER(o, _104), CONCAT_MEMBER(o, _105),    \
    CONCAT_MEMBER(o, _106), CONCAT_MEMBER(o, _107), CONCAT_MEMBER(o, _108),    \
    CONCAT_MEMBER(o, _109), CONCAT_MEMBER(o, _110), CONCAT_MEMBER(o, _111),    \
    CONCAT_MEMBER(o, _112), CONCAT_MEMBER(o, _113))
#define YLT_CALL_ARGS114(                                                      \
    f, o, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15,    \
    _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, \
    _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, \
    _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, \
    _61, _62, _63, _64, _65, _66, _67, _68, _69, _70, _71, _72, _73, _74, _75, \
    _76, _77, _78, _79, _80, _81, _82, _83, _84, _85, _86, _87, _88, _89, _90, \
    _91, _92, _93, _94, _95, _96, _97, _98, _99, _100, _101, _102, _103, _104, \
    _105, _106, _107, _108, _109, _110, _111, _112, _113, _114)                \
  f(CONCAT_MEMBER(o, _1), CONCAT_MEMBER(o, _2), CONCAT_MEMBER(o, _3),          \
    CONCAT_MEMBER(o, _4), CONCAT_MEMBER(o, _5), CONCAT_MEMBER(o, _6),          \
    CONCAT_MEMBER(o, _7), CONCAT_MEMBER(o, _8), CONCAT_MEMBER(o, _9),          \
    CONCAT_MEMBER(o, _10), CONCAT_MEMBER(o, _11), CONCAT_MEMBER(o, _12),       \
    CONCAT_MEMBER(o, _13), CONCAT_MEMBER(o, _14), CONCAT_MEMBER(o, _15),       \
    CONCAT_MEMBER(o, _16), CONCAT_MEMBER(o, _17), CONCAT_MEMBER(o, _18),       \
    CONCAT_MEMBER(o, _19), CONCAT_MEMBER(o, _20), CONCAT_MEMBER(o, _21),       \
    CONCAT_MEMBER(o, _22), CONCAT_MEMBER(o, _23), CONCAT_MEMBER(o, _24),       \
    CONCAT_MEMBER(o, _25), CONCAT_MEMBER(o, _26), CONCAT_MEMBER(o, _27),       \
    CONCAT_MEMBER(o, _28), CONCAT_MEMBER(o, _29), CONCAT_MEMBER(o, _30),       \
    CONCAT_MEMBER(o, _31), CONCAT_MEMBER(o, _32), CONCAT_MEMBER(o, _33),       \
    CONCAT_MEMBER(o, _34), CONCAT_MEMBER(o, _35), CONCAT_MEMBER(o, _36),       \
    CONCAT_MEMBER(o, _37), CONCAT_MEMBER(o, _38), CONCAT_MEMBER(o, _39),       \
    CONCAT_MEMBER(o, _40), CONCAT_MEMBER(o, _41), CONCAT_MEMBER(o, _42),       \
    CONCAT_MEMBER(o, _43), CONCAT_MEMBER(o, _44), CONCAT_MEMBER(o, _45),       \
    CONCAT_MEMBER(o, _46), CONCAT_MEMBER(o, _47), CONCAT_MEMBER(o, _48),       \
    CONCAT_MEMBER(o, _49), CONCAT_MEMBER(o, _50), CONCAT_MEMBER(o, _51),       \
    CONCAT_MEMBER(o, _52), CONCAT_MEMBER(o, _53), CONCAT_MEMBER(o, _54),       \
    CONCAT_MEMBER(o, _55), CONCAT_MEMBER(o, _56), CONCAT_MEMBER(o, _57),       \
    CONCAT_MEMBER(o, _58), CONCAT_MEMBER(o, _59), CONCAT_MEMBER(o, _60),       \
    CONCAT_MEMBER(o, _61), CONCAT_MEMBER(o, _62), CONCAT_MEMBER(o, _63),       \
    CONCAT_MEMBER(o, _64), CONCAT_MEMBER(o, _65), CONCAT_MEMBER(o, _66),       \
    CONCAT_MEMBER(o, _67), CONCAT_MEMBER(o, _68), CONCAT_MEMBER(o, _69),       \
    CONCAT_MEMBER(o, _70), CONCAT_MEMBER(o, _71), CONCAT_MEMBER(o, _72),       \
    CONCAT_MEMBER(o, _73), CONCAT_MEMBER(o, _74), CONCAT_MEMBER(o, _75),       \
    CONCAT_MEMBER(o, _76), CONCAT_MEMBER(o, _77), CONCAT_MEMBER(o, _78),       \
    CONCAT_MEMBER(o, _79), CONCAT_MEMBER(o, _80), CONCAT_MEMBER(o, _81),       \
    CONCAT_MEMBER(o, _82), CONCAT_MEMBER(o, _83), CONCAT_MEMBER(o, _84),       \
    CONCAT_MEMBER(o, _85), CONCAT_MEMBER(o, _86), CONCAT_MEMBER(o, _87),       \
    CONCAT_MEMBER(o, _88), CONCAT_MEMBER(o, _89), CONCAT_MEMBER(o, _90),       \
    CONCAT_MEMBER(o, _91), CONCAT_MEMBER(o, _92), CONCAT_MEMBER(o, _93),       \
    CONCAT_MEMBER(o, _94), CONCAT_MEMBER(o, _95), CONCAT_MEMBER(o, _96),       \
    CONCAT_MEMBER(o, _97), CONCAT_MEMBER(o, _98), CONCAT_MEMBER(o, _99),       \
    CONCAT_MEMBER(o, _100), CONCAT_MEMBER(o, _101), CONCAT_MEMBER(o, _102),    \
    CONCAT_MEMBER(o, _103), CONCAT_MEMBER(o, _104), CONCAT_MEMBER(o, _105),    \
    CONCAT_MEMBER(o, _106), CONCAT_MEMBER(o, _107), CONCAT_MEMBER(o, _108),    \
    CONCAT_MEMBER(o, _109), CONCAT_MEMBER(o, _110), CONCAT_MEMBER(o, _111),    \
    CONCAT_MEMBER(o, _112), CONCAT_MEMBER(o, _113), CONCAT_MEMBER(o, _114))
#define YLT_CALL_ARGS115(                                                      \
    f, o, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15,    \
    _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, \
    _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, \
    _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, \
    _61, _62, _63, _64, _65, _66, _67, _68, _69, _70, _71, _72, _73, _74, _75, \
    _76, _77, _78, _79, _80, _81, _82, _83, _84, _85, _86, _87, _88, _89, _90, \
    _91, _92, _93, _94, _95, _96, _97, _98, _99, _100, _101, _102, _103, _104, \
    _105, _106, _107, _108, _109, _110, _111, _112, _113, _114, _115)          \
  f(CONCAT_MEMBER(o, _1), CONCAT_MEMBER(o, _2), CONCAT_MEMBER(o, _3),          \
    CONCAT_MEMBER(o, _4), CONCAT_MEMBER(o, _5), CONCAT_MEMBER(o, _6),          \
    CONCAT_MEMBER(o, _7), CONCAT_MEMBER(o, _8), CONCAT_MEMBER(o, _9),          \
    CONCAT_MEMBER(o, _10), CONCAT_MEMBER(o, _11), CONCAT_MEMBER(o, _12),       \
    CONCAT_MEMBER(o, _13), CONCAT_MEMBER(o, _14), CONCAT_MEMBER(o, _15),       \
    CONCAT_MEMBER(o, _16), CONCAT_MEMBER(o, _17), CONCAT_MEMBER(o, _18),       \
    CONCAT_MEMBER(o, _19), CONCAT_MEMBER(o, _20), CONCAT_MEMBER(o, _21),       \
    CONCAT_MEMBER(o, _22), CONCAT_MEMBER(o, _23), CONCAT_MEMBER(o, _24),       \
    CONCAT_MEMBER(o, _25), CONCAT_MEMBER(o, _26), CONCAT_MEMBER(o, _27),       \
    CONCAT_MEMBER(o, _28), CONCAT_MEMBER(o, _29), CONCAT_MEMBER(o, _30),       \
    CONCAT_MEMBER(o, _31), CONCAT_MEMBER(o, _32), CONCAT_MEMBER(o, _33),       \
    CONCAT_MEMBER(o, _34), CONCAT_MEMBER(o, _35), CONCAT_MEMBER(o, _36),       \
    CONCAT_MEMBER(o, _37), CONCAT_MEMBER(o, _38), CONCAT_MEMBER(o, _39),       \
    CONCAT_MEMBER(o, _40), CONCAT_MEMBER(o, _41), CONCAT_MEMBER(o, _42),       \
    CONCAT_MEMBER(o, _43), CONCAT_MEMBER(o, _44), CONCAT_MEMBER(o, _45),       \
    CONCAT_MEMBER(o, _46), CONCAT_MEMBER(o, _47), CONCAT_MEMBER(o, _48),       \
    CONCAT_MEMBER(o, _49), CONCAT_MEMBER(o, _50), CONCAT_MEMBER(o, _51),       \
    CONCAT_MEMBER(o, _52), CONCAT_MEMBER(o, _53), CONCAT_MEMBER(o, _54),       \
    CONCAT_MEMBER(o, _55), CONCAT_MEMBER(o, _56), CONCAT_MEMBER(o, _57),       \
    CONCAT_MEMBER(o, _58), CONCAT_MEMBER(o, _59), CONCAT_MEMBER(o, _60),       \
    CONCAT_MEMBER(o, _61), CONCAT_MEMBER(o, _62), CONCAT_MEMBER(o, _63),       \
    CONCAT_MEMBER(o, _64), CONCAT_MEMBER(o, _65), CONCAT_MEMBER(o, _66),       \
    CONCAT_MEMBER(o, _67), CONCAT_MEMBER(o, _68), CONCAT_MEMBER(o, _69),       \
    CONCAT_MEMBER(o, _70), CONCAT_MEMBER(o, _71), CONCAT_MEMBER(o, _72),       \
    CONCAT_MEMBER(o, _73), CONCAT_MEMBER(o, _74), CONCAT_MEMBER(o, _75),       \
    CONCAT_MEMBER(o, _76), CONCAT_MEMBER(o, _77), CONCAT_MEMBER(o, _78),       \
    CONCAT_MEMBER(o, _79), CONCAT_MEMBER(o, _80), CONCAT_MEMBER(o, _81),       \
    CONCAT_MEMBER(o, _82), CONCAT_MEMBER(o, _83), CONCAT_MEMBER(o, _84),       \
    CONCAT_MEMBER(o, _85), CONCAT_MEMBER(o, _86), CONCAT_MEMBER(o, _87),       \
    CONCAT_MEMBER(o, _88), CONCAT_MEMBER(o, _89), CONCAT_MEMBER(o, _90),       \
    CONCAT_MEMBER(o, _91), CONCAT_MEMBER(o, _92), CONCAT_MEMBER(o, _93),       \
    CONCAT_MEMBER(o, _94), CONCAT_MEMBER(o, _95), CONCAT_MEMBER(o, _96),       \
    CONCAT_MEMBER(o, _97), CONCAT_MEMBER(o, _98), CONCAT_MEMBER(o, _99),       \
    CONCAT_MEMBER(o, _100), CONCAT_MEMBER(o, _101), CONCAT_MEMBER(o, _102),    \
    CONCAT_MEMBER(o, _103), CONCAT_MEMBER(o, _104), CONCAT_MEMBER(o, _105),    \
    CONCAT_MEMBER(o, _106), CONCAT_MEMBER(o, _107), CONCAT_MEMBER(o, _108),    \
    CONCAT_MEMBER(o, _109), CONCAT_MEMBER(o, _110), CONCAT_MEMBER(o, _111),    \
    CONCAT_MEMBER(o, _112), CONCAT_MEMBER(o, _113), CONCAT_MEMBER(o, _114),    \
    CONCAT_MEMBER(o, _115))
#define YLT_CALL_ARGS116(                                                      \
    f, o, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15,    \
    _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, \
    _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, \
    _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, \
    _61, _62, _63, _64, _65, _66, _67, _68, _69, _70, _71, _72, _73, _74, _75, \
    _76, _77, _78, _79, _80, _81, _82, _83, _84, _85, _86, _87, _88, _89, _90, \
    _91, _92, _93, _94, _95, _96, _97, _98, _99, _100, _101, _102, _103, _104, \
    _105, _106, _107, _108, _109, _110, _111, _112, _113, _114, _115, _116)    \
  f(CONCAT_MEMBER(o, _1), CONCAT_MEMBER(o, _2), CONCAT_MEMBER(o, _3),          \
    CONCAT_MEMBER(o, _4), CONCAT_MEMBER(o, _5), CONCAT_MEMBER(o, _6),          \
    CONCAT_MEMBER(o, _7), CONCAT_MEMBER(o, _8), CONCAT_MEMBER(o, _9),          \
    CONCAT_MEMBER(o, _10), CONCAT_MEMBER(o, _11), CONCAT_MEMBER(o, _12),       \
    CONCAT_MEMBER(o, _13), CONCAT_MEMBER(o, _14), CONCAT_MEMBER(o, _15),       \
    CONCAT_MEMBER(o, _16), CONCAT_MEMBER(o, _17), CONCAT_MEMBER(o, _18),       \
    CONCAT_MEMBER(o, _19), CONCAT_MEMBER(o, _20), CONCAT_MEMBER(o, _21),       \
    CONCAT_MEMBER(o, _22), CONCAT_MEMBER(o, _23), CONCAT_MEMBER(o, _24),       \
    CONCAT_MEMBER(o, _25), CONCAT_MEMBER(o, _26), CONCAT_MEMBER(o, _27),       \
    CONCAT_MEMBER(o, _28), CONCAT_MEMBER(o, _29), CONCAT_MEMBER(o, _30),       \
    CONCAT_MEMBER(o, _31), CONCAT_MEMBER(o, _32), CONCAT_MEMBER(o, _33),       \
    CONCAT_MEMBER(o, _34), CONCAT_MEMBER(o, _35), CONCAT_MEMBER(o, _36),       \
    CONCAT_MEMBER(o, _37), CONCAT_MEMBER(o, _38), CONCAT_MEMBER(o, _39),       \
    CONCAT_MEMBER(o, _40), CONCAT_MEMBER(o, _41), CONCAT_MEMBER(o, _42),       \
    CONCAT_MEMBER(o, _43), CONCAT_MEMBER(o, _44), CONCAT_MEMBER(o, _45),       \
    CONCAT_MEMBER(o, _46), CONCAT_MEMBER(o, _47), CONCAT_MEMBER(o, _48),       \
    CONCAT_MEMBER(o, _49), CONCAT_MEMBER(o, _50), CONCAT_MEMBER(o, _51),       \
    CONCAT_MEMBER(o, _52), CONCAT_MEMBER(o, _53), CONCAT_MEMBER(o, _54),       \
    CONCAT_MEMBER(o, _55), CONCAT_MEMBER(o, _56), CONCAT_MEMBER(o, _57),       \
    CONCAT_MEMBER(o, _58), CONCAT_MEMBER(o, _59), CONCAT_MEMBER(o, _60),       \
    CONCAT_MEMBER(o, _61), CONCAT_MEMBER(o, _62), CONCAT_MEMBER(o, _63),       \
    CONCAT_MEMBER(o, _64), CONCAT_MEMBER(o, _65), CONCAT_MEMBER(o, _66),       \
    CONCAT_MEMBER(o, _67), CONCAT_MEMBER(o, _68), CONCAT_MEMBER(o, _69),       \
    CONCAT_MEMBER(o, _70), CONCAT_MEMBER(o, _71), CONCAT_MEMBER(o, _72),       \
    CONCAT_MEMBER(o, _73), CONCAT_MEMBER(o, _74), CONCAT_MEMBER(o, _75),       \
    CONCAT_MEMBER(o, _76), CONCAT_MEMBER(o, _77), CONCAT_MEMBER(o, _78),       \
    CONCAT_MEMBER(o, _79), CONCAT_MEMBER(o, _80), CONCAT_MEMBER(o, _81),       \
    CONCAT_MEMBER(o, _82), CONCAT_MEMBER(o, _83), CONCAT_MEMBER(o, _84),       \
    CONCAT_MEMBER(o, _85), CONCAT_MEMBER(o, _86), CONCAT_MEMBER(o, _87),       \
    CONCAT_MEMBER(o, _88), CONCAT_MEMBER(o, _89), CONCAT_MEMBER(o, _90),       \
    CONCAT_MEMBER(o, _91), CONCAT_MEMBER(o, _92), CONCAT_MEMBER(o, _93),       \
    CONCAT_MEMBER(o, _94), CONCAT_MEMBER(o, _95), CONCAT_MEMBER(o, _96),       \
    CONCAT_MEMBER(o, _97), CONCAT_MEMBER(o, _98), CONCAT_MEMBER(o, _99),       \
    CONCAT_MEMBER(o, _100), CONCAT_MEMBER(o, _101), CONCAT_MEMBER(o, _102),    \
    CONCAT_MEMBER(o, _103), CONCAT_MEMBER(o, _104), CONCAT_MEMBER(o, _105),    \
    CONCAT_MEMBER(o, _106), CONCAT_MEMBER(o, _107), CONCAT_MEMBER(o, _108),    \
    CONCAT_MEMBER(o, _109), CONCAT_MEMBER(o, _110), CONCAT_MEMBER(o, _111),    \
    CONCAT_MEMBER(o, _112), CONCAT_MEMBER(o, _113), CONCAT_MEMBER(o, _114),    \
    CONCAT_MEMBER(o, _115), CONCAT_MEMBER(o, _116))
#define YLT_CALL_ARGS117(                                                      \
    f, o, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15,    \
    _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, \
    _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, \
    _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, \
    _61, _62, _63, _64, _65, _66, _67, _68, _69, _70, _71, _72, _73, _74, _75, \
    _76, _77, _78, _79, _80, _81, _82, _83, _84, _85, _86, _87, _88, _89, _90, \
    _91, _92, _93, _94, _95, _96, _97, _98, _99, _100, _101, _102, _103, _104, \
    _105, _106, _107, _108, _109, _110, _111, _112, _113, _114, _115, _116,    \
    _117)                                                                      \
  f(CONCAT_MEMBER(o, _1), CONCAT_MEMBER(o, _2), CONCAT_MEMBER(o, _3),          \
    CONCAT_MEMBER(o, _4), CONCAT_MEMBER(o, _5), CONCAT_MEMBER(o, _6),          \
    CONCAT_MEMBER(o, _7), CONCAT_MEMBER(o, _8), CONCAT_MEMBER(o, _9),          \
    CONCAT_MEMBER(o, _10), CONCAT_MEMBER(o, _11), CONCAT_MEMBER(o, _12),       \
    CONCAT_MEMBER(o, _13), CONCAT_MEMBER(o, _14), CONCAT_MEMBER(o, _15),       \
    CONCAT_MEMBER(o, _16), CONCAT_MEMBER(o, _17), CONCAT_MEMBER(o, _18),       \
    CONCAT_MEMBER(o, _19), CONCAT_MEMBER(o, _20), CONCAT_MEMBER(o, _21),       \
    CONCAT_MEMBER(o, _22), CONCAT_MEMBER(o, _23), CONCAT_MEMBER(o, _24),       \
    CONCAT_MEMBER(o, _25), CONCAT_MEMBER(o, _26), CONCAT_MEMBER(o, _27),       \
    CONCAT_MEMBER(o, _28), CONCAT_MEMBER(o, _29), CONCAT_MEMBER(o, _30),       \
    CONCAT_MEMBER(o, _31), CONCAT_MEMBER(o, _32), CONCAT_MEMBER(o, _33),       \
    CONCAT_MEMBER(o, _34), CONCAT_MEMBER(o, _35), CONCAT_MEMBER(o, _36),       \
    CONCAT_MEMBER(o, _37), CONCAT_MEMBER(o, _38), CONCAT_MEMBER(o, _39),       \
    CONCAT_MEMBER(o, _40), CONCAT_MEMBER(o, _41), CONCAT_MEMBER(o, _42),       \
    CONCAT_MEMBER(o, _43), CONCAT_MEMBER(o, _44), CONCAT_MEMBER(o, _45),       \
    CONCAT_MEMBER(o, _46), CONCAT_MEMBER(o, _47), CONCAT_MEMBER(o, _48),       \
    CONCAT_MEMBER(o, _49), CONCAT_MEMBER(o, _50), CONCAT_MEMBER(o, _51),       \
    CONCAT_MEMBER(o, _52), CONCAT_MEMBER(o, _53), CONCAT_MEMBER(o, _54),       \
    CONCAT_MEMBER(o, _55), CONCAT_MEMBER(o, _56), CONCAT_MEMBER(o, _57),       \
    CONCAT_MEMBER(o, _58), CONCAT_MEMBER(o, _59), CONCAT_MEMBER(o, _60),       \
    CONCAT_MEMBER(o, _61), CONCAT_MEMBER(o, _62), CONCAT_MEMBER(o, _63),       \
    CONCAT_MEMBER(o, _64), CONCAT_MEMBER(o, _65), CONCAT_MEMBER(o, _66),       \
    CONCAT_MEMBER(o, _67), CONCAT_MEMBER(o, _68), CONCAT_MEMBER(o, _69),       \
    CONCAT_MEMBER(o, _70), CONCAT_MEMBER(o, _71), CONCAT_MEMBER(o, _72),       \
    CONCAT_MEMBER(o, _73), CONCAT_MEMBER(o, _74), CONCAT_MEMBER(o, _75),       \
    CONCAT_MEMBER(o, _76), CONCAT_MEMBER(o, _77), CONCAT_MEMBER(o, _78),       \
    CONCAT_MEMBER(o, _79), CONCAT_MEMBER(o, _80), CONCAT_MEMBER(o, _81),       \
    CONCAT_MEMBER(o, _82), CONCAT_MEMBER(o, _83), CONCAT_MEMBER(o, _84),       \
    CONCAT_MEMBER(o, _85), CONCAT_MEMBER(o, _86), CONCAT_MEMBER(o, _87),       \
    CONCAT_MEMBER(o, _88), CONCAT_MEMBER(o, _89), CONCAT_MEMBER(o, _90),       \
    CONCAT_MEMBER(o, _91), CONCAT_MEMBER(o, _92), CONCAT_MEMBER(o, _93),       \
    CONCAT_MEMBER(o, _94), CONCAT_MEMBER(o, _95), CONCAT_MEMBER(o, _96),       \
    CONCAT_MEMBER(o, _97), CONCAT_MEMBER(o, _98), CONCAT_MEMBER(o, _99),       \
    CONCAT_MEMBER(o, _100), CONCAT_MEMBER(o, _101), CONCAT_MEMBER(o, _102),    \
    CONCAT_MEMBER(o, _103), CONCAT_MEMBER(o, _104), CONCAT_MEMBER(o, _105),    \
    CONCAT_MEMBER(o, _106), CONCAT_MEMBER(o, _107), CONCAT_MEMBER(o, _108),    \
    CONCAT_MEMBER(o, _109), CONCAT_MEMBER(o, _110), CONCAT_MEMBER(o, _111),    \
    CONCAT_MEMBER(o, _112), CONCAT_MEMBER(o, _113), CONCAT_MEMBER(o, _114),    \
    CONCAT_MEMBER(o, _115), CONCAT_MEMBER(o, _116), CONCAT_MEMBER(o, _117))
#define YLT_CALL_ARGS118(                                                      \
    f, o, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15,    \
    _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, \
    _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, \
    _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, \
    _61, _62, _63, _64, _65, _66, _67, _68, _69, _70, _71, _72, _73, _74, _75, \
    _76, _77, _78, _79, _80, _81, _82, _83, _84, _85, _86, _87, _88, _89, _90, \
    _91, _92, _93, _94, _95, _96, _97, _98, _99, _100, _101, _102, _103, _104, \
    _105, _106, _107, _108, _109, _110, _111, _112, _113, _114, _115, _116,    \
    _117, _118)                                                                \
  f(CONCAT_MEMBER(o, _1), CONCAT_MEMBER(o, _2), CONCAT_MEMBER(o, _3),          \
    CONCAT_MEMBER(o, _4), CONCAT_MEMBER(o, _5), CONCAT_MEMBER(o, _6),          \
    CONCAT_MEMBER(o, _7), CONCAT_MEMBER(o, _8), CONCAT_MEMBER(o, _9),          \
    CONCAT_MEMBER(o, _10), CONCAT_MEMBER(o, _11), CONCAT_MEMBER(o, _12),       \
    CONCAT_MEMBER(o, _13), CONCAT_MEMBER(o, _14), CONCAT_MEMBER(o, _15),       \
    CONCAT_MEMBER(o, _16), CONCAT_MEMBER(o, _17), CONCAT_MEMBER(o, _18),       \
    CONCAT_MEMBER(o, _19), CONCAT_MEMBER(o, _20), CONCAT_MEMBER(o, _21),       \
    CONCAT_MEMBER(o, _22), CONCAT_MEMBER(o, _23), CONCAT_MEMBER(o, _24),       \
    CONCAT_MEMBER(o, _25), CONCAT_MEMBER(o, _26), CONCAT_MEMBER(o, _27),       \
    CONCAT_MEMBER(o, _28), CONCAT_MEMBER(o, _29), CONCAT_MEMBER(o, _30),       \
    CONCAT_MEMBER(o, _31), CONCAT_MEMBER(o, _32), CONCAT_MEMBER(o, _33),       \
    CONCAT_MEMBER(o, _34), CONCAT_MEMBER(o, _35), CONCAT_MEMBER(o, _36),       \
    CONCAT_MEMBER(o, _37), CONCAT_MEMBER(o, _38), CONCAT_MEMBER(o, _39),       \
    CONCAT_MEMBER(o, _40), CONCAT_MEMBER(o, _41), CONCAT_MEMBER(o, _42),       \
    CONCAT_MEMBER(o, _43), CONCAT_MEMBER(o, _44), CONCAT_MEMBER(o, _45),       \
    CONCAT_MEMBER(o, _46), CONCAT_MEMBER(o, _47), CONCAT_MEMBER(o, _48),       \
    CONCAT_MEMBER(o, _49), CONCAT_MEMBER(o, _50), CONCAT_MEMBER(o, _51),       \
    CONCAT_MEMBER(o, _52), CONCAT_MEMBER(o, _53), CONCAT_MEMBER(o, _54),       \
    CONCAT_MEMBER(o, _55), CONCAT_MEMBER(o, _56), CONCAT_MEMBER(o, _57),       \
    CONCAT_MEMBER(o, _58), CONCAT_MEMBER(o, _59), CONCAT_MEMBER(o, _60),       \
    CONCAT_MEMBER(o, _61), CONCAT_MEMBER(o, _62), CONCAT_MEMBER(o, _63),       \
    CONCAT_MEMBER(o, _64), CONCAT_MEMBER(o, _65), CONCAT_MEMBER(o, _66),       \
    CONCAT_MEMBER(o, _67), CONCAT_MEMBER(o, _68), CONCAT_MEMBER(o, _69),       \
    CONCAT_MEMBER(o, _70), CONCAT_MEMBER(o, _71), CONCAT_MEMBER(o, _72),       \
    CONCAT_MEMBER(o, _73), CONCAT_MEMBER(o, _74), CONCAT_MEMBER(o, _75),       \
    CONCAT_MEMBER(o, _76), CONCAT_MEMBER(o, _77), CONCAT_MEMBER(o, _78),       \
    CONCAT_MEMBER(o, _79), CONCAT_MEMBER(o, _80), CONCAT_MEMBER(o, _81),       \
    CONCAT_MEMBER(o, _82), CONCAT_MEMBER(o, _83), CONCAT_MEMBER(o, _84),       \
    CONCAT_MEMBER(o, _85), CONCAT_MEMBER(o, _86), CONCAT_MEMBER(o, _87),       \
    CONCAT_MEMBER(o, _88), CONCAT_MEMBER(o, _89), CONCAT_MEMBER(o, _90),       \
    CONCAT_MEMBER(o, _91), CONCAT_MEMBER(o, _92), CONCAT_MEMBER(o, _93),       \
    CONCAT_MEMBER(o, _94), CONCAT_MEMBER(o, _95), CONCAT_MEMBER(o, _96),       \
    CONCAT_MEMBER(o, _97), CONCAT_MEMBER(o, _98), CONCAT_MEMBER(o, _99),       \
    CONCAT_MEMBER(o, _100), CONCAT_MEMBER(o, _101), CONCAT_MEMBER(o, _102),    \
    CONCAT_MEMBER(o, _103), CONCAT_MEMBER(o, _104), CONCAT_MEMBER(o, _105),    \
    CONCAT_MEMBER(o, _106), CONCAT_MEMBER(o, _107), CONCAT_MEMBER(o, _108),    \
    CONCAT_MEMBER(o, _109), CONCAT_MEMBER(o, _110), CONCAT_MEMBER(o, _111),    \
    CONCAT_MEMBER(o, _112), CONCAT_MEMBER(o, _113), CONCAT_MEMBER(o, _114),    \
    CONCAT_MEMBER(o, _115), CONCAT_MEMBER(o, _116), CONCAT_MEMBER(o, _117),    \
    CONCAT_MEMBER(o, _118))
#define YLT_CALL_ARGS119(                                                      \
    f, o, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15,    \
    _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, \
    _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, \
    _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, \
    _61, _62, _63, _64, _65, _66, _67, _68, _69, _70, _71, _72, _73, _74, _75, \
    _76, _77, _78, _79, _80, _81, _82, _83, _84, _85, _86, _87, _88, _89, _90, \
    _91, _92, _93, _94, _95, _96, _97, _98, _99, _100, _101, _102, _103, _104, \
    _105, _106, _107, _108, _109, _110, _111, _112, _113, _114, _115, _116,    \
    _117, _118, _119)                                                          \
  f(CONCAT_MEMBER(o, _1), CONCAT_MEMBER(o, _2), CONCAT_MEMBER(o, _3),          \
    CONCAT_MEMBER(o, _4), CONCAT_MEMBER(o, _5), CONCAT_MEMBER(o, _6),          \
    CONCAT_MEMBER(o, _7), CONCAT_MEMBER(o, _8), CONCAT_MEMBER(o, _9),          \
    CONCAT_MEMBER(o, _10), CONCAT_MEMBER(o, _11), CONCAT_MEMBER(o, _12),       \
    CONCAT_MEMBER(o, _13), CONCAT_MEMBER(o, _14), CONCAT_MEMBER(o, _15),       \
    CONCAT_MEMBER(o, _16), CONCAT_MEMBER(o, _17), CONCAT_MEMBER(o, _18),       \
    CONCAT_MEMBER(o, _19), CONCAT_MEMBER(o, _20), CONCAT_MEMBER(o, _21),       \
    CONCAT_MEMBER(o, _22), CONCAT_MEMBER(o, _23), CONCAT_MEMBER(o, _24),       \
    CONCAT_MEMBER(o, _25), CONCAT_MEMBER(o, _26), CONCAT_MEMBER(o, _27),       \
    CONCAT_MEMBER(o, _28), CONCAT_MEMBER(o, _29), CONCAT_MEMBER(o, _30),       \
    CONCAT_MEMBER(o, _31), CONCAT_MEMBER(o, _32), CONCAT_MEMBER(o, _33),       \
    CONCAT_MEMBER(o, _34), CONCAT_MEMBER(o, _35), CONCAT_MEMBER(o, _36),       \
    CONCAT_MEMBER(o, _37), CONCAT_MEMBER(o, _38), CONCAT_MEMBER(o, _39),       \
    CONCAT_MEMBER(o, _40), CONCAT_MEMBER(o, _41), CONCAT_MEMBER(o, _42),       \
    CONCAT_MEMBER(o, _43), CONCAT_MEMBER(o, _44), CONCAT_MEMBER(o, _45),       \
    CONCAT_MEMBER(o, _46), CONCAT_MEMBER(o, _47), CONCAT_MEMBER(o, _48),       \
    CONCAT_MEMBER(o, _49), CONCAT_MEMBER(o, _50), CONCAT_MEMBER(o, _51),       \
    CONCAT_MEMBER(o, _52), CONCAT_MEMBER(o, _53), CONCAT_MEMBER(o, _54),       \
    CONCAT_MEMBER(o, _55), CONCAT_MEMBER(o, _56), CONCAT_MEMBER(o, _57),       \
    CONCAT_MEMBER(o, _58), CONCAT_MEMBER(o, _59), CONCAT_MEMBER(o, _60),       \
    CONCAT_MEMBER(o, _61), CONCAT_MEMBER(o, _62), CONCAT_MEMBER(o, _63),       \
    CONCAT_MEMBER(o, _64), CONCAT_MEMBER(o, _65), CONCAT_MEMBER(o, _66),       \
    CONCAT_MEMBER(o, _67), CONCAT_MEMBER(o, _68), CONCAT_MEMBER(o, _69),       \
    CONCAT_MEMBER(o, _70), CONCAT_MEMBER(o, _71), CONCAT_MEMBER(o, _72),       \
    CONCAT_MEMBER(o, _73), CONCAT_MEMBER(o, _74), CONCAT_MEMBER(o, _75),       \
    CONCAT_MEMBER(o, _76), CONCAT_MEMBER(o, _77), CONCAT_MEMBER(o, _78),       \
    CONCAT_MEMBER(o, _79), CONCAT_MEMBER(o, _80), CONCAT_MEMBER(o, _81),       \
    CONCAT_MEMBER(o, _82), CONCAT_MEMBER(o, _83), CONCAT_MEMBER(o, _84),       \
    CONCAT_MEMBER(o, _85), CONCAT_MEMBER(o, _86), CONCAT_MEMBER(o, _87),       \
    CONCAT_MEMBER(o, _88), CONCAT_MEMBER(o, _89), CONCAT_MEMBER(o, _90),       \
    CONCAT_MEMBER(o, _91), CONCAT_MEMBER(o, _92), CONCAT_MEMBER(o, _93),       \
    CONCAT_MEMBER(o, _94), CONCAT_MEMBER(o, _95), CONCAT_MEMBER(o, _96),       \
    CONCAT_MEMBER(o, _97), CONCAT_MEMBER(o, _98), CONCAT_MEMBER(o, _99),       \
    CONCAT_MEMBER(o, _100), CONCAT_MEMBER(o, _101), CONCAT_MEMBER(o, _102),    \
    CONCAT_MEMBER(o, _103), CONCAT_MEMBER(o, _104), CONCAT_MEMBER(o, _105),    \
    CONCAT_MEMBER(o, _106), CONCAT_MEMBER(o, _107), CONCAT_MEMBER(o, _108),    \
    CONCAT_MEMBER(o, _109), CONCAT_MEMBER(o, _110), CONCAT_MEMBER(o, _111),    \
    CONCAT_MEMBER(o, _112), CONCAT_MEMBER(o, _113), CONCAT_MEMBER(o, _114),    \
    CONCAT_MEMBER(o, _115), CONCAT_MEMBER(o, _116), CONCAT_MEMBER(o, _117),    \
    CONCAT_MEMBER(o, _118), CONCAT_MEMBER(o, _119))
#define YLT_CALL_ARGS120(                                                      \
    f, o, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15,    \
    _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, \
    _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, \
    _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, \
    _61, _62, _63, _64, _65, _66, _67, _68, _69, _70, _71, _72, _73, _74, _75, \
    _76, _77, _78, _79, _80, _81, _82, _83, _84, _85, _86, _87, _88, _89, _90, \
    _91, _92, _93, _94, _95, _96, _97, _98, _99, _100, _101, _102, _103, _104, \
    _105, _106, _107, _108, _109, _110, _111, _112, _113, _114, _115, _116,    \
    _117, _118, _119, _120)                                                    \
  f(CONCAT_MEMBER(o, _1), CONCAT_MEMBER(o, _2), CONCAT_MEMBER(o, _3),          \
    CONCAT_MEMBER(o, _4), CONCAT_MEMBER(o, _5), CONCAT_MEMBER(o, _6),          \
    CONCAT_MEMBER(o, _7), CONCAT_MEMBER(o, _8), CONCAT_MEMBER(o, _9),          \
    CONCAT_MEMBER(o, _10), CONCAT_MEMBER(o, _11), CONCAT_MEMBER(o, _12),       \
    CONCAT_MEMBER(o, _13), CONCAT_MEMBER(o, _14), CONCAT_MEMBER(o, _15),       \
    CONCAT_MEMBER(o, _16), CONCAT_MEMBER(o, _17), CONCAT_MEMBER(o, _18),       \
    CONCAT_MEMBER(o, _19), CONCAT_MEMBER(o, _20), CONCAT_MEMBER(o, _21),       \
    CONCAT_MEMBER(o, _22), CONCAT_MEMBER(o, _23), CONCAT_MEMBER(o, _24),       \
    CONCAT_MEMBER(o, _25), CONCAT_MEMBER(o, _26), CONCAT_MEMBER(o, _27),       \
    CONCAT_MEMBER(o, _28), CONCAT_MEMBER(o, _29), CONCAT_MEMBER(o, _30),       \
    CONCAT_MEMBER(o, _31), CONCAT_MEMBER(o, _32), CONCAT_MEMBER(o, _33),       \
    CONCAT_MEMBER(o, _34), CONCAT_MEMBER(o, _35), CONCAT_MEMBER(o, _36),       \
    CONCAT_MEMBER(o, _37), CONCAT_MEMBER(o, _38), CONCAT_MEMBER(o, _39),       \
    CONCAT_MEMBER(o, _40), CONCAT_MEMBER(o, _41), CONCAT_MEMBER(o, _42),       \
    CONCAT_MEMBER(o, _43), CONCAT_MEMBER(o, _44), CONCAT_MEMBER(o, _45),       \
    CONCAT_MEMBER(o, _46), CONCAT_MEMBER(o, _47), CONCAT_MEMBER(o, _48),       \
    CONCAT_MEMBER(o, _49), CONCAT_MEMBER(o, _50), CONCAT_MEMBER(o, _51),       \
    CONCAT_MEMBER(o, _52), CONCAT_MEMBER(o, _53), CONCAT_MEMBER(o, _54),       \
    CONCAT_MEMBER(o, _55), CONCAT_MEMBER(o, _56), CONCAT_MEMBER(o, _57),       \
    CONCAT_MEMBER(o, _58), CONCAT_MEMBER(o, _59), CONCAT_MEMBER(o, _60),       \
    CONCAT_MEMBER(o, _61), CONCAT_MEMBER(o, _62), CONCAT_MEMBER(o, _63),       \
    CONCAT_MEMBER(o, _64), CONCAT_MEMBER(o, _65), CONCAT_MEMBER(o, _66),       \
    CONCAT_MEMBER(o, _67), CONCAT_MEMBER(o, _68), CONCAT_MEMBER(o, _69),       \
    CONCAT_MEMBER(o, _70), CONCAT_MEMBER(o, _71), CONCAT_MEMBER(o, _72),       \
    CONCAT_MEMBER(o, _73), CONCAT_MEMBER(o, _74), CONCAT_MEMBER(o, _75),       \
    CONCAT_MEMBER(o, _76), CONCAT_MEMBER(o, _77), CONCAT_MEMBER(o, _78),       \
    CONCAT_MEMBER(o, _79), CONCAT_MEMBER(o, _80), CONCAT_MEMBER(o, _81),       \
    CONCAT_MEMBER(o, _82), CONCAT_MEMBER(o, _83), CONCAT_MEMBER(o, _84),       \
    CONCAT_MEMBER(o, _85), CONCAT_MEMBER(o, _86), CONCAT_MEMBER(o, _87),       \
    CONCAT_MEMBER(o, _88), CONCAT_MEMBER(o, _89), CONCAT_MEMBER(o, _90),       \
    CONCAT_MEMBER(o, _91), CONCAT_MEMBER(o, _92), CONCAT_MEMBER(o, _93),       \
    CONCAT_MEMBER(o, _94), CONCAT_MEMBER(o, _95), CONCAT_MEMBER(o, _96),       \
    CONCAT_MEMBER(o, _97), CONCAT_MEMBER(o, _98), CONCAT_MEMBER(o, _99),       \
    CONCAT_MEMBER(o, _100), CONCAT_MEMBER(o, _101), CONCAT_MEMBER(o, _102),    \
    CONCAT_MEMBER(o, _103), CONCAT_MEMBER(o, _104), CONCAT_MEMBER(o, _105),    \
    CONCAT_MEMBER(o, _106), CONCAT_MEMBER(o, _107), CONCAT_MEMBER(o, _108),    \
    CONCAT_MEMBER(o, _109), CONCAT_MEMBER(o, _110), CONCAT_MEMBER(o, _111),    \
    CONCAT_MEMBER(o, _112), CONCAT_MEMBER(o, _113), CONCAT_MEMBER(o, _114),    \
    CONCAT_MEMBER(o, _115), CONCAT_MEMBER(o, _116), CONCAT_MEMBER(o, _117),    \
    CONCAT_MEMBER(o, _118), CONCAT_MEMBER(o, _119), CONCAT_MEMBER(o, _120))
#define YLT_CALL_ARGS121(                                                      \
    f, o, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15,    \
    _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, \
    _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, \
    _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, \
    _61, _62, _63, _64, _65, _66, _67, _68, _69, _70, _71, _72, _73, _74, _75, \
    _76, _77, _78, _79, _80, _81, _82, _83, _84, _85, _86, _87, _88, _89, _90, \
    _91, _92, _93, _94, _95, _96, _97, _98, _99, _100, _101, _102, _103, _104, \
    _105, _106, _107, _108, _109, _110, _111, _112, _113, _114, _115, _116,    \
    _117, _118, _119, _120, _121)                                              \
  f(CONCAT_MEMBER(o, _1), CONCAT_MEMBER(o, _2), CONCAT_MEMBER(o, _3),          \
    CONCAT_MEMBER(o, _4), CONCAT_MEMBER(o, _5), CONCAT_MEMBER(o, _6),          \
    CONCAT_MEMBER(o, _7), CONCAT_MEMBER(o, _8), CONCAT_MEMBER(o, _9),          \
    CONCAT_MEMBER(o, _10), CONCAT_MEMBER(o, _11), CONCAT_MEMBER(o, _12),       \
    CONCAT_MEMBER(o, _13), CONCAT_MEMBER(o, _14), CONCAT_MEMBER(o, _15),       \
    CONCAT_MEMBER(o, _16), CONCAT_MEMBER(o, _17), CONCAT_MEMBER(o, _18),       \
    CONCAT_MEMBER(o, _19), CONCAT_MEMBER(o, _20), CONCAT_MEMBER(o, _21),       \
    CONCAT_MEMBER(o, _22), CONCAT_MEMBER(o, _23), CONCAT_MEMBER(o, _24),       \
    CONCAT_MEMBER(o, _25), CONCAT_MEMBER(o, _26), CONCAT_MEMBER(o, _27),       \
    CONCAT_MEMBER(o, _28), CONCAT_MEMBER(o, _29), CONCAT_MEMBER(o, _30),       \
    CONCAT_MEMBER(o, _31), CONCAT_MEMBER(o, _32), CONCAT_MEMBER(o, _33),       \
    CONCAT_MEMBER(o, _34), CONCAT_MEMBER(o, _35), CONCAT_MEMBER(o, _36),       \
    CONCAT_MEMBER(o, _37), CONCAT_MEMBER(o, _38), CONCAT_MEMBER(o, _39),       \
    CONCAT_MEMBER(o, _40), CONCAT_MEMBER(o, _41), CONCAT_MEMBER(o, _42),       \
    CONCAT_MEMBER(o, _43), CONCAT_MEMBER(o, _44), CONCAT_MEMBER(o, _45),       \
    CONCAT_MEMBER(o, _46), CONCAT_MEMBER(o, _47), CONCAT_MEMBER(o, _48),       \
    CONCAT_MEMBER(o, _49), CONCAT_MEMBER(o, _50), CONCAT_MEMBER(o, _51),       \
    CONCAT_MEMBER(o, _52), CONCAT_MEMBER(o, _53), CONCAT_MEMBER(o, _54),       \
    CONCAT_MEMBER(o, _55), CONCAT_MEMBER(o, _56), CONCAT_MEMBER(o, _57),       \
    CONCAT_MEMBER(o, _58), CONCAT_MEMBER(o, _59), CONCAT_MEMBER(o, _60),       \
    CONCAT_MEMBER(o, _61), CONCAT_MEMBER(o, _62), CONCAT_MEMBER(o, _63),       \
    CONCAT_MEMBER(o, _64), CONCAT_MEMBER(o, _65), CONCAT_MEMBER(o, _66),       \
    CONCAT_MEMBER(o, _67), CONCAT_MEMBER(o, _68), CONCAT_MEMBER(o, _69),       \
    CONCAT_MEMBER(o, _70), CONCAT_MEMBER(o, _71), CONCAT_MEMBER(o, _72),       \
    CONCAT_MEMBER(o, _73), CONCAT_MEMBER(o, _74), CONCAT_MEMBER(o, _75),       \
    CONCAT_MEMBER(o, _76), CONCAT_MEMBER(o, _77), CONCAT_MEMBER(o, _78),       \
    CONCAT_MEMBER(o, _79), CONCAT_MEMBER(o, _80), CONCAT_MEMBER(o, _81),       \
    CONCAT_MEMBER(o, _82), CONCAT_MEMBER(o, _83), CONCAT_MEMBER(o, _84),       \
    CONCAT_MEMBER(o, _85), CONCAT_MEMBER(o, _86), CONCAT_MEMBER(o, _87),       \
    CONCAT_MEMBER(o, _88), CONCAT_MEMBER(o, _89), CONCAT_MEMBER(o, _90),       \
    CONCAT_MEMBER(o, _91), CONCAT_MEMBER(o, _92), CONCAT_MEMBER(o, _93),       \
    CONCAT_MEMBER(o, _94), CONCAT_MEMBER(o, _95), CONCAT_MEMBER(o, _96),       \
    CONCAT_MEMBER(o, _97), CONCAT_MEMBER(o, _98), CONCAT_MEMBER(o, _99),       \
    CONCAT_MEMBER(o, _100), CONCAT_MEMBER(o, _101), CONCAT_MEMBER(o, _102),    \
    CONCAT_MEMBER(o, _103), CONCAT_MEMBER(o, _104), CONCAT_MEMBER(o, _105),    \
    CONCAT_MEMBER(o, _106), CONCAT_MEMBER(o, _107), CONCAT_MEMBER(o, _108),    \
    CONCAT_MEMBER(o, _109), CONCAT_MEMBER(o, _110), CONCAT_MEMBER(o, _111),    \
    CONCAT_MEMBER(o, _112), CONCAT_MEMBER(o, _113), CONCAT_MEMBER(o, _114),    \
    CONCAT_MEMBER(o, _115), CONCAT_MEMBER(o, _116), CONCAT_MEMBER(o, _117),    \
    CONCAT_MEMBER(o, _118), CONCAT_MEMBER(o, _119), CONCAT_MEMBER(o, _120),    \
    CONCAT_MEMBER(o, _121))
#define YLT_CALL_ARGS122(                                                      \
    f, o, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15,    \
    _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, \
    _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, \
    _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, \
    _61, _62, _63, _64, _65, _66, _67, _68, _69, _70, _71, _72, _73, _74, _75, \
    _76, _77, _78, _79, _80, _81, _82, _83, _84, _85, _86, _87, _88, _89, _90, \
    _91, _92, _93, _94, _95, _96, _97, _98, _99, _100, _101, _102, _103, _104, \
    _105, _106, _107, _108, _109, _110, _111, _112, _113, _114, _115, _116,    \
    _117, _118, _119, _120, _121, _122)                                        \
  f(CONCAT_MEMBER(o, _1), CONCAT_MEMBER(o, _2), CONCAT_MEMBER(o, _3),          \
    CONCAT_MEMBER(o, _4), CONCAT_MEMBER(o, _5), CONCAT_MEMBER(o, _6),          \
    CONCAT_MEMBER(o, _7), CONCAT_MEMBER(o, _8), CONCAT_MEMBER(o, _9),          \
    CONCAT_MEMBER(o, _10), CONCAT_MEMBER(o, _11), CONCAT_MEMBER(o, _12),       \
    CONCAT_MEMBER(o, _13), CONCAT_MEMBER(o, _14), CONCAT_MEMBER(o, _15),       \
    CONCAT_MEMBER(o, _16), CONCAT_MEMBER(o, _17), CONCAT_MEMBER(o, _18),       \
    CONCAT_MEMBER(o, _19), CONCAT_MEMBER(o, _20), CONCAT_MEMBER(o, _21),       \
    CONCAT_MEMBER(o, _22), CONCAT_MEMBER(o, _23), CONCAT_MEMBER(o, _24),       \
    CONCAT_MEMBER(o, _25), CONCAT_MEMBER(o, _26), CONCAT_MEMBER(o, _27),       \
    CONCAT_MEMBER(o, _28), CONCAT_MEMBER(o, _29), CONCAT_MEMBER(o, _30),       \
    CONCAT_MEMBER(o, _31), CONCAT_MEMBER(o, _32), CONCAT_MEMBER(o, _33),       \
    CONCAT_MEMBER(o, _34), CONCAT_MEMBER(o, _35), CONCAT_MEMBER(o, _36),       \
    CONCAT_MEMBER(o, _37), CONCAT_MEMBER(o, _38), CONCAT_MEMBER(o, _39),       \
    CONCAT_MEMBER(o, _40), CONCAT_MEMBER(o, _41), CONCAT_MEMBER(o, _42),       \
    CONCAT_MEMBER(o, _43), CONCAT_MEMBER(o, _44), CONCAT_MEMBER(o, _45),       \
    CONCAT_MEMBER(o, _46), CONCAT_MEMBER(o, _47), CONCAT_MEMBER(o, _48),       \
    CONCAT_MEMBER(o, _49), CONCAT_MEMBER(o, _50), CONCAT_MEMBER(o, _51),       \
    CONCAT_MEMBER(o, _52), CONCAT_MEMBER(o, _53), CONCAT_MEMBER(o, _54),       \
    CONCAT_MEMBER(o, _55), CONCAT_MEMBER(o, _56), CONCAT_MEMBER(o, _57),       \
    CONCAT_MEMBER(o, _58), CONCAT_MEMBER(o, _59), CONCAT_MEMBER(o, _60),       \
    CONCAT_MEMBER(o, _61), CONCAT_MEMBER(o, _62), CONCAT_MEMBER(o, _63),       \
    CONCAT_MEMBER(o, _64), CONCAT_MEMBER(o, _65), CONCAT_MEMBER(o, _66),       \
    CONCAT_MEMBER(o, _67), CONCAT_MEMBER(o, _68), CONCAT_MEMBER(o, _69),       \
    CONCAT_MEMBER(o, _70), CONCAT_MEMBER(o, _71), CONCAT_MEMBER(o, _72),       \
    CONCAT_MEMBER(o, _73), CONCAT_MEMBER(o, _74), CONCAT_MEMBER(o, _75),       \
    CONCAT_MEMBER(o, _76), CONCAT_MEMBER(o, _77), CONCAT_MEMBER(o, _78),       \
    CONCAT_MEMBER(o, _79), CONCAT_MEMBER(o, _80), CONCAT_MEMBER(o, _81),       \
    CONCAT_MEMBER(o, _82), CONCAT_MEMBER(o, _83), CONCAT_MEMBER(o, _84),       \
    CONCAT_MEMBER(o, _85), CONCAT_MEMBER(o, _86), CONCAT_MEMBER(o, _87),       \
    CONCAT_MEMBER(o, _88), CONCAT_MEMBER(o, _89), CONCAT_MEMBER(o, _90),       \
    CONCAT_MEMBER(o, _91), CONCAT_MEMBER(o, _92), CONCAT_MEMBER(o, _93),       \
    CONCAT_MEMBER(o, _94), CONCAT_MEMBER(o, _95), CONCAT_MEMBER(o, _96),       \
    CONCAT_MEMBER(o, _97), CONCAT_MEMBER(o, _98), CONCAT_MEMBER(o, _99),       \
    CONCAT_MEMBER(o, _100), CONCAT_MEMBER(o, _101), CONCAT_MEMBER(o, _102),    \
    CONCAT_MEMBER(o, _103), CONCAT_MEMBER(o, _104), CONCAT_MEMBER(o, _105),    \
    CONCAT_MEMBER(o, _106), CONCAT_MEMBER(o, _107), CONCAT_MEMBER(o, _108),    \
    CONCAT_MEMBER(o, _109), CONCAT_MEMBER(o, _110), CONCAT_MEMBER(o, _111),    \
    CONCAT_MEMBER(o, _112), CONCAT_MEMBER(o, _113), CONCAT_MEMBER(o, _114),    \
    CONCAT_MEMBER(o, _115), CONCAT_MEMBER(o, _116), CONCAT_MEMBER(o, _117),    \
    CONCAT_MEMBER(o, _118), CONCAT_MEMBER(o, _119), CONCAT_MEMBER(o, _120),    \
    CONCAT_MEMBER(o, _121), CONCAT_MEMBER(o, _122))
#define YLT_CALL_ARGS123(                                                      \
    f, o, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15,    \
    _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, \
    _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, \
    _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, \
    _61, _62, _63, _64, _65, _66, _67, _68, _69, _70, _71, _72, _73, _74, _75, \
    _76, _77, _78, _79, _80, _81, _82, _83, _84, _85, _86, _87, _88, _89, _90, \
    _91, _92, _93, _94, _95, _96, _97, _98, _99, _100, _101, _102, _103, _104, \
    _105, _106, _107, _108, _109, _110, _111, _112, _113, _114, _115, _116,    \
    _117, _118, _119, _120, _121, _122, _123)                                  \
  f(CONCAT_MEMBER(o, _1), CONCAT_MEMBER(o, _2), CONCAT_MEMBER(o, _3),          \
    CONCAT_MEMBER(o, _4), CONCAT_MEMBER(o, _5), CONCAT_MEMBER(o, _6),          \
    CONCAT_MEMBER(o, _7), CONCAT_MEMBER(o, _8), CONCAT_MEMBER(o, _9),          \
    CONCAT_MEMBER(o, _10), CONCAT_MEMBER(o, _11), CONCAT_MEMBER(o, _12),       \
    CONCAT_MEMBER(o, _13), CONCAT_MEMBER(o, _14), CONCAT_MEMBER(o, _15),       \
    CONCAT_MEMBER(o, _16), CONCAT_MEMBER(o, _17), CONCAT_MEMBER(o, _18),       \
    CONCAT_MEMBER(o, _19), CONCAT_MEMBER(o, _20), CONCAT_MEMBER(o, _21),       \
    CONCAT_MEMBER(o, _22), CONCAT_MEMBER(o, _23), CONCAT_MEMBER(o, _24),       \
    CONCAT_MEMBER(o, _25), CONCAT_MEMBER(o, _26), CONCAT_MEMBER(o, _27),       \
    CONCAT_MEMBER(o, _28), CONCAT_MEMBER(o, _29), CONCAT_MEMBER(o, _30),       \
    CONCAT_MEMBER(o, _31), CONCAT_MEMBER(o, _32), CONCAT_MEMBER(o, _33),       \
    CONCAT_MEMBER(o, _34), CONCAT_MEMBER(o, _35), CONCAT_MEMBER(o, _36),       \
    CONCAT_MEMBER(o, _37), CONCAT_MEMBER(o, _38), CONCAT_MEMBER(o, _39),       \
    CONCAT_MEMBER(o, _40), CONCAT_MEMBER(o, _41), CONCAT_MEMBER(o, _42),       \
    CONCAT_MEMBER(o, _43), CONCAT_MEMBER(o, _44), CONCAT_MEMBER(o, _45),       \
    CONCAT_MEMBER(o, _46), CONCAT_MEMBER(o, _47), CONCAT_MEMBER(o, _48),       \
    CONCAT_MEMBER(o, _49), CONCAT_MEMBER(o, _50), CONCAT_MEMBER(o, _51),       \
    CONCAT_MEMBER(o, _52), CONCAT_MEMBER(o, _53), CONCAT_MEMBER(o, _54),       \
    CONCAT_MEMBER(o, _55), CONCAT_MEMBER(o, _56), CONCAT_MEMBER(o, _57),       \
    CONCAT_MEMBER(o, _58), CONCAT_MEMBER(o, _59), CONCAT_MEMBER(o, _60),       \
    CONCAT_MEMBER(o, _61), CONCAT_MEMBER(o, _62), CONCAT_MEMBER(o, _63),       \
    CONCAT_MEMBER(o, _64), CONCAT_MEMBER(o, _65), CONCAT_MEMBER(o, _66),       \
    CONCAT_MEMBER(o, _67), CONCAT_MEMBER(o, _68), CONCAT_MEMBER(o, _69),       \
    CONCAT_MEMBER(o, _70), CONCAT_MEMBER(o, _71), CONCAT_MEMBER(o, _72),       \
    CONCAT_MEMBER(o, _73), CONCAT_MEMBER(o, _74), CONCAT_MEMBER(o, _75),       \
    CONCAT_MEMBER(o, _76), CONCAT_MEMBER(o, _77), CONCAT_MEMBER(o, _78),       \
    CONCAT_MEMBER(o, _79), CONCAT_MEMBER(o, _80), CONCAT_MEMBER(o, _81),       \
    CONCAT_MEMBER(o, _82), CONCAT_MEMBER(o, _83), CONCAT_MEMBER(o, _84),       \
    CONCAT_MEMBER(o, _85), CONCAT_MEMBER(o, _86), CONCAT_MEMBER(o, _87),       \
    CONCAT_MEMBER(o, _88), CONCAT_MEMBER(o, _89), CONCAT_MEMBER(o, _90),       \
    CONCAT_MEMBER(o, _91), CONCAT_MEMBER(o, _92), CONCAT_MEMBER(o, _93),       \
    CONCAT_MEMBER(o, _94), CONCAT_MEMBER(o, _95), CONCAT_MEMBER(o, _96),       \
    CONCAT_MEMBER(o, _97), CONCAT_MEMBER(o, _98), CONCAT_MEMBER(o, _99),       \
    CONCAT_MEMBER(o, _100), CONCAT_MEMBER(o, _101), CONCAT_MEMBER(o, _102),    \
    CONCAT_MEMBER(o, _103), CONCAT_MEMBER(o, _104), CONCAT_MEMBER(o, _105),    \
    CONCAT_MEMBER(o, _106), CONCAT_MEMBER(o, _107), CONCAT_MEMBER(o, _108),    \
    CONCAT_MEMBER(o, _109), CONCAT_MEMBER(o, _110), CONCAT_MEMBER(o, _111),    \
    CONCAT_MEMBER(o, _112), CONCAT_MEMBER(o, _113), CONCAT_MEMBER(o, _114),    \
    CONCAT_MEMBER(o, _115), CONCAT_MEMBER(o, _116), CONCAT_MEMBER(o, _117),    \
    CONCAT_MEMBER(o, _118), CONCAT_MEMBER(o, _119), CONCAT_MEMBER(o, _120),    \
    CONCAT_MEMBER(o, _121), CONCAT_MEMBER(o, _122), CONCAT_MEMBER(o, _123))
#define YLT_CALL_ARGS124(                                                      \
    f, o, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15,    \
    _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, \
    _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, \
    _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, \
    _61, _62, _63, _64, _65, _66, _67, _68, _69, _70, _71, _72, _73, _74, _75, \
    _76, _77, _78, _79, _80, _81, _82, _83, _84, _85, _86, _87, _88, _89, _90, \
    _91, _92, _93, _94, _95, _96, _97, _98, _99, _100, _101, _102, _103, _104, \
    _105, _106, _107, _108, _109, _110, _111, _112, _113, _114, _115, _116,    \
    _117, _118, _119, _120, _121, _122, _123, _124)                            \
  f(CONCAT_MEMBER(o, _1), CONCAT_MEMBER(o, _2), CONCAT_MEMBER(o, _3),          \
    CONCAT_MEMBER(o, _4), CONCAT_MEMBER(o, _5), CONCAT_MEMBER(o, _6),          \
    CONCAT_MEMBER(o, _7), CONCAT_MEMBER(o, _8), CONCAT_MEMBER(o, _9),          \
    CONCAT_MEMBER(o, _10), CONCAT_MEMBER(o, _11), CONCAT_MEMBER(o, _12),       \
    CONCAT_MEMBER(o, _13), CONCAT_MEMBER(o, _14), CONCAT_MEMBER(o, _15),       \
    CONCAT_MEMBER(o, _16), CONCAT_MEMBER(o, _17), CONCAT_MEMBER(o, _18),       \
    CONCAT_MEMBER(o, _19), CONCAT_MEMBER(o, _20), CONCAT_MEMBER(o, _21),       \
    CONCAT_MEMBER(o, _22), CONCAT_MEMBER(o, _23), CONCAT_MEMBER(o, _24),       \
    CONCAT_MEMBER(o, _25), CONCAT_MEMBER(o, _26), CONCAT_MEMBER(o, _27),       \
    CONCAT_MEMBER(o, _28), CONCAT_MEMBER(o, _29), CONCAT_MEMBER(o, _30),       \
    CONCAT_MEMBER(o, _31), CONCAT_MEMBER(o, _32), CONCAT_MEMBER(o, _33),       \
    CONCAT_MEMBER(o, _34), CONCAT_MEMBER(o, _35), CONCAT_MEMBER(o, _36),       \
    CONCAT_MEMBER(o, _37), CONCAT_MEMBER(o, _38), CONCAT_MEMBER(o, _39),       \
    CONCAT_MEMBER(o, _40), CONCAT_MEMBER(o, _41), CONCAT_MEMBER(o, _42),       \
    CONCAT_MEMBER(o, _43), CONCAT_MEMBER(o, _44), CONCAT_MEMBER(o, _45),       \
    CONCAT_MEMBER(o, _46), CONCAT_MEMBER(o, _47), CONCAT_MEMBER(o, _48),       \
    CONCAT_MEMBER(o, _49), CONCAT_MEMBER(o, _50), CONCAT_MEMBER(o, _51),       \
    CONCAT_MEMBER(o, _52), CONCAT_MEMBER(o, _53), CONCAT_MEMBER(o, _54),       \
    CONCAT_MEMBER(o, _55), CONCAT_MEMBER(o, _56), CONCAT_MEMBER(o, _57),       \
    CONCAT_MEMBER(o, _58), CONCAT_MEMBER(o, _59), CONCAT_MEMBER(o, _60),       \
    CONCAT_MEMBER(o, _61), CONCAT_MEMBER(o, _62), CONCAT_MEMBER(o, _63),       \
    CONCAT_MEMBER(o, _64), CONCAT_MEMBER(o, _65), CONCAT_MEMBER(o, _66),       \
    CONCAT_MEMBER(o, _67), CONCAT_MEMBER(o, _68), CONCAT_MEMBER(o, _69),       \
    CONCAT_MEMBER(o, _70), CONCAT_MEMBER(o, _71), CONCAT_MEMBER(o, _72),       \
    CONCAT_MEMBER(o, _73), CONCAT_MEMBER(o, _74), CONCAT_MEMBER(o, _75),       \
    CONCAT_MEMBER(o, _76), CONCAT_MEMBER(o, _77), CONCAT_MEMBER(o, _78),       \
    CONCAT_MEMBER(o, _79), CONCAT_MEMBER(o, _80), CONCAT_MEMBER(o, _81),       \
    CONCAT_MEMBER(o, _82), CONCAT_MEMBER(o, _83), CONCAT_MEMBER(o, _84),       \
    CONCAT_MEMBER(o, _85), CONCAT_MEMBER(o, _86), CONCAT_MEMBER(o, _87),       \
    CONCAT_MEMBER(o, _88), CONCAT_MEMBER(o, _89), CONCAT_MEMBER(o, _90),       \
    CONCAT_MEMBER(o, _91), CONCAT_MEMBER(o, _92), CONCAT_MEMBER(o, _93),       \
    CONCAT_MEMBER(o, _94), CONCAT_MEMBER(o, _95), CONCAT_MEMBER(o, _96),       \
    CONCAT_MEMBER(o, _97), CONCAT_MEMBER(o, _98), CONCAT_MEMBER(o, _99),       \
    CONCAT_MEMBER(o, _100), CONCAT_MEMBER(o, _101), CONCAT_MEMBER(o, _102),    \
    CONCAT_MEMBER(o, _103), CONCAT_MEMBER(o, _104), CONCAT_MEMBER(o, _105),    \
    CONCAT_MEMBER(o, _106), CONCAT_MEMBER(o, _107), CONCAT_MEMBER(o, _108),    \
    CONCAT_MEMBER(o, _109), CONCAT_MEMBER(o, _110), CONCAT_MEMBER(o, _111),    \
    CONCAT_MEMBER(o, _112), CONCAT_MEMBER(o, _113), CONCAT_MEMBER(o, _114),    \
    CONCAT_MEMBER(o, _115), CONCAT_MEMBER(o, _116), CONCAT_MEMBER(o, _117),    \
    CONCAT_MEMBER(o, _118), CONCAT_MEMBER(o, _119), CONCAT_MEMBER(o, _120),    \
    CONCAT_MEMBER(o, _121), CONCAT_MEMBER(o, _122), CONCAT_MEMBER(o, _123),    \
    CONCAT_MEMBER(o, _124))
#define YLT_CALL_ARGS125(                                                      \
    f, o, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15,    \
    _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, \
    _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, \
    _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, \
    _61, _62, _63, _64, _65, _66, _67, _68, _69, _70, _71, _72, _73, _74, _75, \
    _76, _77, _78, _79, _80, _81, _82, _83, _84, _85, _86, _87, _88, _89, _90, \
    _91, _92, _93, _94, _95, _96, _97, _98, _99, _100, _101, _102, _103, _104, \
    _105, _106, _107, _108, _109, _110, _111, _112, _113, _114, _115, _116,    \
    _117, _118, _119, _120, _121, _122, _123, _124, _125)                      \
  f(CONCAT_MEMBER(o, _1), CONCAT_MEMBER(o, _2), CONCAT_MEMBER(o, _3),          \
    CONCAT_MEMBER(o, _4), CONCAT_MEMBER(o, _5), CONCAT_MEMBER(o, _6),          \
    CONCAT_MEMBER(o, _7), CONCAT_MEMBER(o, _8), CONCAT_MEMBER(o, _9),          \
    CONCAT_MEMBER(o, _10), CONCAT_MEMBER(o, _11), CONCAT_MEMBER(o, _12),       \
    CONCAT_MEMBER(o, _13), CONCAT_MEMBER(o, _14), CONCAT_MEMBER(o, _15),       \
    CONCAT_MEMBER(o, _16), CONCAT_MEMBER(o, _17), CONCAT_MEMBER(o, _18),       \
    CONCAT_MEMBER(o, _19), CONCAT_MEMBER(o, _20), CONCAT_MEMBER(o, _21),       \
    CONCAT_MEMBER(o, _22), CONCAT_MEMBER(o, _23), CONCAT_MEMBER(o, _24),       \
    CONCAT_MEMBER(o, _25), CONCAT_MEMBER(o, _26), CONCAT_MEMBER(o, _27),       \
    CONCAT_MEMBER(o, _28), CONCAT_MEMBER(o, _29), CONCAT_MEMBER(o, _30),       \
    CONCAT_MEMBER(o, _31), CONCAT_MEMBER(o, _32), CONCAT_MEMBER(o, _33),       \
    CONCAT_MEMBER(o, _34), CONCAT_MEMBER(o, _35), CONCAT_MEMBER(o, _36),       \
    CONCAT_MEMBER(o, _37), CONCAT_MEMBER(o, _38), CONCAT_MEMBER(o, _39),       \
    CONCAT_MEMBER(o, _40), CONCAT_MEMBER(o, _41), CONCAT_MEMBER(o, _42),       \
    CONCAT_MEMBER(o, _43), CONCAT_MEMBER(o, _44), CONCAT_MEMBER(o, _45),       \
    CONCAT_MEMBER(o, _46), CONCAT_MEMBER(o, _47), CONCAT_MEMBER(o, _48),       \
    CONCAT_MEMBER(o, _49), CONCAT_MEMBER(o, _50), CONCAT_MEMBER(o, _51),       \
    CONCAT_MEMBER(o, _52), CONCAT_MEMBER(o, _53), CONCAT_MEMBER(o, _54),       \
    CONCAT_MEMBER(o, _55), CONCAT_MEMBER(o, _56), CONCAT_MEMBER(o, _57),       \
    CONCAT_MEMBER(o, _58), CONCAT_MEMBER(o, _59), CONCAT_MEMBER(o, _60),       \
    CONCAT_MEMBER(o, _61), CONCAT_MEMBER(o, _62), CONCAT_MEMBER(o, _63),       \
    CONCAT_MEMBER(o, _64), CONCAT_MEMBER(o, _65), CONCAT_MEMBER(o, _66),       \
    CONCAT_MEMBER(o, _67), CONCAT_MEMBER(o, _68), CONCAT_MEMBER(o, _69),       \
    CONCAT_MEMBER(o, _70), CONCAT_MEMBER(o, _71), CONCAT_MEMBER(o, _72),       \
    CONCAT_MEMBER(o, _73), CONCAT_MEMBER(o, _74), CONCAT_MEMBER(o, _75),       \
    CONCAT_MEMBER(o, _76), CONCAT_MEMBER(o, _77), CONCAT_MEMBER(o, _78),       \
    CONCAT_MEMBER(o, _79), CONCAT_MEMBER(o, _80), CONCAT_MEMBER(o, _81),       \
    CONCAT_MEMBER(o, _82), CONCAT_MEMBER(o, _83), CONCAT_MEMBER(o, _84),       \
    CONCAT_MEMBER(o, _85), CONCAT_MEMBER(o, _86), CONCAT_MEMBER(o, _87),       \
    CONCAT_MEMBER(o, _88), CONCAT_MEMBER(o, _89), CONCAT_MEMBER(o, _90),       \
    CONCAT_MEMBER(o, _91), CONCAT_MEMBER(o, _92), CONCAT_MEMBER(o, _93),       \
    CONCAT_MEMBER(o, _94), CONCAT_MEMBER(o, _95), CONCAT_MEMBER(o, _96),       \
    CONCAT_MEMBER(o, _97), CONCAT_MEMBER(o, _98), CONCAT_MEMBER(o, _99),       \
    CONCAT_MEMBER(o, _100), CONCAT_MEMBER(o, _101), CONCAT_MEMBER(o, _102),    \
    CONCAT_MEMBER(o, _103), CONCAT_MEMBER(o, _104), CONCAT_MEMBER(o, _105),    \
    CONCAT_MEMBER(o, _106), CONCAT_MEMBER(o, _107), CONCAT_MEMBER(o, _108),    \
    CONCAT_MEMBER(o, _109), CONCAT_MEMBER(o, _110), CONCAT_MEMBER(o, _111),    \
    CONCAT_MEMBER(o, _112), CONCAT_MEMBER(o, _113), CONCAT_MEMBER(o, _114),    \
    CONCAT_MEMBER(o, _115), CONCAT_MEMBER(o, _116), CONCAT_MEMBER(o, _117),    \
    CONCAT_MEMBER(o, _118), CONCAT_MEMBER(o, _119), CONCAT_MEMBER(o, _120),    \
    CONCAT_MEMBER(o, _121), CONCAT_MEMBER(o, _122), CONCAT_MEMBER(o, _123),    \
    CONCAT_MEMBER(o, _124), CONCAT_MEMBER(o, _125))
