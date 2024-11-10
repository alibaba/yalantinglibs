/*The following boilerplate code was generated using a Python script:
macro = "#define WRAP_ARGS"
with open("generated_args.txt", "w", encoding="utf-8") as codefile:
    codefile.write(
        "\n".join(
            [
                f"{macro}{i}(w,o,_1, ...) w(o, _1),
YLT_MACRO_EXPAND(WRAP_ARGS{i-1}(w,o,__VA_ARGS__))" for i in range(2, 126)
            ]
        )
    )
*/
#define WRAP_ARGS2(w, o, _1, ...) \
  w(o, _1), YLT_MACRO_EXPAND(WRAP_ARGS1(w, o, __VA_ARGS__))
#define WRAP_ARGS3(w, o, _1, ...) \
  w(o, _1), YLT_MACRO_EXPAND(WRAP_ARGS2(w, o, __VA_ARGS__))
#define WRAP_ARGS4(w, o, _1, ...) \
  w(o, _1), YLT_MACRO_EXPAND(WRAP_ARGS3(w, o, __VA_ARGS__))
#define WRAP_ARGS5(w, o, _1, ...) \
  w(o, _1), YLT_MACRO_EXPAND(WRAP_ARGS4(w, o, __VA_ARGS__))
#define WRAP_ARGS6(w, o, _1, ...) \
  w(o, _1), YLT_MACRO_EXPAND(WRAP_ARGS5(w, o, __VA_ARGS__))
#define WRAP_ARGS7(w, o, _1, ...) \
  w(o, _1), YLT_MACRO_EXPAND(WRAP_ARGS6(w, o, __VA_ARGS__))
#define WRAP_ARGS8(w, o, _1, ...) \
  w(o, _1), YLT_MACRO_EXPAND(WRAP_ARGS7(w, o, __VA_ARGS__))
#define WRAP_ARGS9(w, o, _1, ...) \
  w(o, _1), YLT_MACRO_EXPAND(WRAP_ARGS8(w, o, __VA_ARGS__))
#define WRAP_ARGS10(w, o, _1, ...) \
  w(o, _1), YLT_MACRO_EXPAND(WRAP_ARGS9(w, o, __VA_ARGS__))
#define WRAP_ARGS11(w, o, _1, ...) \
  w(o, _1), YLT_MACRO_EXPAND(WRAP_ARGS10(w, o, __VA_ARGS__))
#define WRAP_ARGS12(w, o, _1, ...) \
  w(o, _1), YLT_MACRO_EXPAND(WRAP_ARGS11(w, o, __VA_ARGS__))
#define WRAP_ARGS13(w, o, _1, ...) \
  w(o, _1), YLT_MACRO_EXPAND(WRAP_ARGS12(w, o, __VA_ARGS__))
#define WRAP_ARGS14(w, o, _1, ...) \
  w(o, _1), YLT_MACRO_EXPAND(WRAP_ARGS13(w, o, __VA_ARGS__))
#define WRAP_ARGS15(w, o, _1, ...) \
  w(o, _1), YLT_MACRO_EXPAND(WRAP_ARGS14(w, o, __VA_ARGS__))
#define WRAP_ARGS16(w, o, _1, ...) \
  w(o, _1), YLT_MACRO_EXPAND(WRAP_ARGS15(w, o, __VA_ARGS__))
#define WRAP_ARGS17(w, o, _1, ...) \
  w(o, _1), YLT_MACRO_EXPAND(WRAP_ARGS16(w, o, __VA_ARGS__))
#define WRAP_ARGS18(w, o, _1, ...) \
  w(o, _1), YLT_MACRO_EXPAND(WRAP_ARGS17(w, o, __VA_ARGS__))
#define WRAP_ARGS19(w, o, _1, ...) \
  w(o, _1), YLT_MACRO_EXPAND(WRAP_ARGS18(w, o, __VA_ARGS__))
#define WRAP_ARGS20(w, o, _1, ...) \
  w(o, _1), YLT_MACRO_EXPAND(WRAP_ARGS19(w, o, __VA_ARGS__))
#define WRAP_ARGS21(w, o, _1, ...) \
  w(o, _1), YLT_MACRO_EXPAND(WRAP_ARGS20(w, o, __VA_ARGS__))
#define WRAP_ARGS22(w, o, _1, ...) \
  w(o, _1), YLT_MACRO_EXPAND(WRAP_ARGS21(w, o, __VA_ARGS__))
#define WRAP_ARGS23(w, o, _1, ...) \
  w(o, _1), YLT_MACRO_EXPAND(WRAP_ARGS22(w, o, __VA_ARGS__))
#define WRAP_ARGS24(w, o, _1, ...) \
  w(o, _1), YLT_MACRO_EXPAND(WRAP_ARGS23(w, o, __VA_ARGS__))
#define WRAP_ARGS25(w, o, _1, ...) \
  w(o, _1), YLT_MACRO_EXPAND(WRAP_ARGS24(w, o, __VA_ARGS__))
#define WRAP_ARGS26(w, o, _1, ...) \
  w(o, _1), YLT_MACRO_EXPAND(WRAP_ARGS25(w, o, __VA_ARGS__))
#define WRAP_ARGS27(w, o, _1, ...) \
  w(o, _1), YLT_MACRO_EXPAND(WRAP_ARGS26(w, o, __VA_ARGS__))
#define WRAP_ARGS28(w, o, _1, ...) \
  w(o, _1), YLT_MACRO_EXPAND(WRAP_ARGS27(w, o, __VA_ARGS__))
#define WRAP_ARGS29(w, o, _1, ...) \
  w(o, _1), YLT_MACRO_EXPAND(WRAP_ARGS28(w, o, __VA_ARGS__))
#define WRAP_ARGS30(w, o, _1, ...) \
  w(o, _1), YLT_MACRO_EXPAND(WRAP_ARGS29(w, o, __VA_ARGS__))
#define WRAP_ARGS31(w, o, _1, ...) \
  w(o, _1), YLT_MACRO_EXPAND(WRAP_ARGS30(w, o, __VA_ARGS__))
#define WRAP_ARGS32(w, o, _1, ...) \
  w(o, _1), YLT_MACRO_EXPAND(WRAP_ARGS31(w, o, __VA_ARGS__))
#define WRAP_ARGS33(w, o, _1, ...) \
  w(o, _1), YLT_MACRO_EXPAND(WRAP_ARGS32(w, o, __VA_ARGS__))
#define WRAP_ARGS34(w, o, _1, ...) \
  w(o, _1), YLT_MACRO_EXPAND(WRAP_ARGS33(w, o, __VA_ARGS__))
#define WRAP_ARGS35(w, o, _1, ...) \
  w(o, _1), YLT_MACRO_EXPAND(WRAP_ARGS34(w, o, __VA_ARGS__))
#define WRAP_ARGS36(w, o, _1, ...) \
  w(o, _1), YLT_MACRO_EXPAND(WRAP_ARGS35(w, o, __VA_ARGS__))
#define WRAP_ARGS37(w, o, _1, ...) \
  w(o, _1), YLT_MACRO_EXPAND(WRAP_ARGS36(w, o, __VA_ARGS__))
#define WRAP_ARGS38(w, o, _1, ...) \
  w(o, _1), YLT_MACRO_EXPAND(WRAP_ARGS37(w, o, __VA_ARGS__))
#define WRAP_ARGS39(w, o, _1, ...) \
  w(o, _1), YLT_MACRO_EXPAND(WRAP_ARGS38(w, o, __VA_ARGS__))
#define WRAP_ARGS40(w, o, _1, ...) \
  w(o, _1), YLT_MACRO_EXPAND(WRAP_ARGS39(w, o, __VA_ARGS__))
#define WRAP_ARGS41(w, o, _1, ...) \
  w(o, _1), YLT_MACRO_EXPAND(WRAP_ARGS40(w, o, __VA_ARGS__))
#define WRAP_ARGS42(w, o, _1, ...) \
  w(o, _1), YLT_MACRO_EXPAND(WRAP_ARGS41(w, o, __VA_ARGS__))
#define WRAP_ARGS43(w, o, _1, ...) \
  w(o, _1), YLT_MACRO_EXPAND(WRAP_ARGS42(w, o, __VA_ARGS__))
#define WRAP_ARGS44(w, o, _1, ...) \
  w(o, _1), YLT_MACRO_EXPAND(WRAP_ARGS43(w, o, __VA_ARGS__))
#define WRAP_ARGS45(w, o, _1, ...) \
  w(o, _1), YLT_MACRO_EXPAND(WRAP_ARGS44(w, o, __VA_ARGS__))
#define WRAP_ARGS46(w, o, _1, ...) \
  w(o, _1), YLT_MACRO_EXPAND(WRAP_ARGS45(w, o, __VA_ARGS__))
#define WRAP_ARGS47(w, o, _1, ...) \
  w(o, _1), YLT_MACRO_EXPAND(WRAP_ARGS46(w, o, __VA_ARGS__))
#define WRAP_ARGS48(w, o, _1, ...) \
  w(o, _1), YLT_MACRO_EXPAND(WRAP_ARGS47(w, o, __VA_ARGS__))
#define WRAP_ARGS49(w, o, _1, ...) \
  w(o, _1), YLT_MACRO_EXPAND(WRAP_ARGS48(w, o, __VA_ARGS__))
#define WRAP_ARGS50(w, o, _1, ...) \
  w(o, _1), YLT_MACRO_EXPAND(WRAP_ARGS49(w, o, __VA_ARGS__))
#define WRAP_ARGS51(w, o, _1, ...) \
  w(o, _1), YLT_MACRO_EXPAND(WRAP_ARGS50(w, o, __VA_ARGS__))
#define WRAP_ARGS52(w, o, _1, ...) \
  w(o, _1), YLT_MACRO_EXPAND(WRAP_ARGS51(w, o, __VA_ARGS__))
#define WRAP_ARGS53(w, o, _1, ...) \
  w(o, _1), YLT_MACRO_EXPAND(WRAP_ARGS52(w, o, __VA_ARGS__))
#define WRAP_ARGS54(w, o, _1, ...) \
  w(o, _1), YLT_MACRO_EXPAND(WRAP_ARGS53(w, o, __VA_ARGS__))
#define WRAP_ARGS55(w, o, _1, ...) \
  w(o, _1), YLT_MACRO_EXPAND(WRAP_ARGS54(w, o, __VA_ARGS__))
#define WRAP_ARGS56(w, o, _1, ...) \
  w(o, _1), YLT_MACRO_EXPAND(WRAP_ARGS55(w, o, __VA_ARGS__))
#define WRAP_ARGS57(w, o, _1, ...) \
  w(o, _1), YLT_MACRO_EXPAND(WRAP_ARGS56(w, o, __VA_ARGS__))
#define WRAP_ARGS58(w, o, _1, ...) \
  w(o, _1), YLT_MACRO_EXPAND(WRAP_ARGS57(w, o, __VA_ARGS__))
#define WRAP_ARGS59(w, o, _1, ...) \
  w(o, _1), YLT_MACRO_EXPAND(WRAP_ARGS58(w, o, __VA_ARGS__))
#define WRAP_ARGS60(w, o, _1, ...) \
  w(o, _1), YLT_MACRO_EXPAND(WRAP_ARGS59(w, o, __VA_ARGS__))
#define WRAP_ARGS61(w, o, _1, ...) \
  w(o, _1), YLT_MACRO_EXPAND(WRAP_ARGS60(w, o, __VA_ARGS__))
#define WRAP_ARGS62(w, o, _1, ...) \
  w(o, _1), YLT_MACRO_EXPAND(WRAP_ARGS61(w, o, __VA_ARGS__))
#define WRAP_ARGS63(w, o, _1, ...) \
  w(o, _1), YLT_MACRO_EXPAND(WRAP_ARGS62(w, o, __VA_ARGS__))
#define WRAP_ARGS64(w, o, _1, ...) \
  w(o, _1), YLT_MACRO_EXPAND(WRAP_ARGS63(w, o, __VA_ARGS__))
#define WRAP_ARGS65(w, o, _1, ...) \
  w(o, _1), YLT_MACRO_EXPAND(WRAP_ARGS64(w, o, __VA_ARGS__))
#define WRAP_ARGS66(w, o, _1, ...) \
  w(o, _1), YLT_MACRO_EXPAND(WRAP_ARGS65(w, o, __VA_ARGS__))
#define WRAP_ARGS67(w, o, _1, ...) \
  w(o, _1), YLT_MACRO_EXPAND(WRAP_ARGS66(w, o, __VA_ARGS__))
#define WRAP_ARGS68(w, o, _1, ...) \
  w(o, _1), YLT_MACRO_EXPAND(WRAP_ARGS67(w, o, __VA_ARGS__))
#define WRAP_ARGS69(w, o, _1, ...) \
  w(o, _1), YLT_MACRO_EXPAND(WRAP_ARGS68(w, o, __VA_ARGS__))
#define WRAP_ARGS70(w, o, _1, ...) \
  w(o, _1), YLT_MACRO_EXPAND(WRAP_ARGS69(w, o, __VA_ARGS__))
#define WRAP_ARGS71(w, o, _1, ...) \
  w(o, _1), YLT_MACRO_EXPAND(WRAP_ARGS70(w, o, __VA_ARGS__))
#define WRAP_ARGS72(w, o, _1, ...) \
  w(o, _1), YLT_MACRO_EXPAND(WRAP_ARGS71(w, o, __VA_ARGS__))
#define WRAP_ARGS73(w, o, _1, ...) \
  w(o, _1), YLT_MACRO_EXPAND(WRAP_ARGS72(w, o, __VA_ARGS__))
#define WRAP_ARGS74(w, o, _1, ...) \
  w(o, _1), YLT_MACRO_EXPAND(WRAP_ARGS73(w, o, __VA_ARGS__))
#define WRAP_ARGS75(w, o, _1, ...) \
  w(o, _1), YLT_MACRO_EXPAND(WRAP_ARGS74(w, o, __VA_ARGS__))
#define WRAP_ARGS76(w, o, _1, ...) \
  w(o, _1), YLT_MACRO_EXPAND(WRAP_ARGS75(w, o, __VA_ARGS__))
#define WRAP_ARGS77(w, o, _1, ...) \
  w(o, _1), YLT_MACRO_EXPAND(WRAP_ARGS76(w, o, __VA_ARGS__))
#define WRAP_ARGS78(w, o, _1, ...) \
  w(o, _1), YLT_MACRO_EXPAND(WRAP_ARGS77(w, o, __VA_ARGS__))
#define WRAP_ARGS79(w, o, _1, ...) \
  w(o, _1), YLT_MACRO_EXPAND(WRAP_ARGS78(w, o, __VA_ARGS__))
#define WRAP_ARGS80(w, o, _1, ...) \
  w(o, _1), YLT_MACRO_EXPAND(WRAP_ARGS79(w, o, __VA_ARGS__))
#define WRAP_ARGS81(w, o, _1, ...) \
  w(o, _1), YLT_MACRO_EXPAND(WRAP_ARGS80(w, o, __VA_ARGS__))
#define WRAP_ARGS82(w, o, _1, ...) \
  w(o, _1), YLT_MACRO_EXPAND(WRAP_ARGS81(w, o, __VA_ARGS__))
#define WRAP_ARGS83(w, o, _1, ...) \
  w(o, _1), YLT_MACRO_EXPAND(WRAP_ARGS82(w, o, __VA_ARGS__))
#define WRAP_ARGS84(w, o, _1, ...) \
  w(o, _1), YLT_MACRO_EXPAND(WRAP_ARGS83(w, o, __VA_ARGS__))
#define WRAP_ARGS85(w, o, _1, ...) \
  w(o, _1), YLT_MACRO_EXPAND(WRAP_ARGS84(w, o, __VA_ARGS__))
#define WRAP_ARGS86(w, o, _1, ...) \
  w(o, _1), YLT_MACRO_EXPAND(WRAP_ARGS85(w, o, __VA_ARGS__))
#define WRAP_ARGS87(w, o, _1, ...) \
  w(o, _1), YLT_MACRO_EXPAND(WRAP_ARGS86(w, o, __VA_ARGS__))
#define WRAP_ARGS88(w, o, _1, ...) \
  w(o, _1), YLT_MACRO_EXPAND(WRAP_ARGS87(w, o, __VA_ARGS__))
#define WRAP_ARGS89(w, o, _1, ...) \
  w(o, _1), YLT_MACRO_EXPAND(WRAP_ARGS88(w, o, __VA_ARGS__))
#define WRAP_ARGS90(w, o, _1, ...) \
  w(o, _1), YLT_MACRO_EXPAND(WRAP_ARGS89(w, o, __VA_ARGS__))
#define WRAP_ARGS91(w, o, _1, ...) \
  w(o, _1), YLT_MACRO_EXPAND(WRAP_ARGS90(w, o, __VA_ARGS__))
#define WRAP_ARGS92(w, o, _1, ...) \
  w(o, _1), YLT_MACRO_EXPAND(WRAP_ARGS91(w, o, __VA_ARGS__))
#define WRAP_ARGS93(w, o, _1, ...) \
  w(o, _1), YLT_MACRO_EXPAND(WRAP_ARGS92(w, o, __VA_ARGS__))
#define WRAP_ARGS94(w, o, _1, ...) \
  w(o, _1), YLT_MACRO_EXPAND(WRAP_ARGS93(w, o, __VA_ARGS__))
#define WRAP_ARGS95(w, o, _1, ...) \
  w(o, _1), YLT_MACRO_EXPAND(WRAP_ARGS94(w, o, __VA_ARGS__))
#define WRAP_ARGS96(w, o, _1, ...) \
  w(o, _1), YLT_MACRO_EXPAND(WRAP_ARGS95(w, o, __VA_ARGS__))
#define WRAP_ARGS97(w, o, _1, ...) \
  w(o, _1), YLT_MACRO_EXPAND(WRAP_ARGS96(w, o, __VA_ARGS__))
#define WRAP_ARGS98(w, o, _1, ...) \
  w(o, _1), YLT_MACRO_EXPAND(WRAP_ARGS97(w, o, __VA_ARGS__))
#define WRAP_ARGS99(w, o, _1, ...) \
  w(o, _1), YLT_MACRO_EXPAND(WRAP_ARGS98(w, o, __VA_ARGS__))
#define WRAP_ARGS100(w, o, _1, ...) \
  w(o, _1), YLT_MACRO_EXPAND(WRAP_ARGS99(w, o, __VA_ARGS__))
#define WRAP_ARGS101(w, o, _1, ...) \
  w(o, _1), YLT_MACRO_EXPAND(WRAP_ARGS100(w, o, __VA_ARGS__))
#define WRAP_ARGS102(w, o, _1, ...) \
  w(o, _1), YLT_MACRO_EXPAND(WRAP_ARGS101(w, o, __VA_ARGS__))
#define WRAP_ARGS103(w, o, _1, ...) \
  w(o, _1), YLT_MACRO_EXPAND(WRAP_ARGS102(w, o, __VA_ARGS__))
#define WRAP_ARGS104(w, o, _1, ...) \
  w(o, _1), YLT_MACRO_EXPAND(WRAP_ARGS103(w, o, __VA_ARGS__))
#define WRAP_ARGS105(w, o, _1, ...) \
  w(o, _1), YLT_MACRO_EXPAND(WRAP_ARGS104(w, o, __VA_ARGS__))
#define WRAP_ARGS106(w, o, _1, ...) \
  w(o, _1), YLT_MACRO_EXPAND(WRAP_ARGS105(w, o, __VA_ARGS__))
#define WRAP_ARGS107(w, o, _1, ...) \
  w(o, _1), YLT_MACRO_EXPAND(WRAP_ARGS106(w, o, __VA_ARGS__))
#define WRAP_ARGS108(w, o, _1, ...) \
  w(o, _1), YLT_MACRO_EXPAND(WRAP_ARGS107(w, o, __VA_ARGS__))
#define WRAP_ARGS109(w, o, _1, ...) \
  w(o, _1), YLT_MACRO_EXPAND(WRAP_ARGS108(w, o, __VA_ARGS__))
#define WRAP_ARGS110(w, o, _1, ...) \
  w(o, _1), YLT_MACRO_EXPAND(WRAP_ARGS109(w, o, __VA_ARGS__))
#define WRAP_ARGS111(w, o, _1, ...) \
  w(o, _1), YLT_MACRO_EXPAND(WRAP_ARGS110(w, o, __VA_ARGS__))
#define WRAP_ARGS112(w, o, _1, ...) \
  w(o, _1), YLT_MACRO_EXPAND(WRAP_ARGS111(w, o, __VA_ARGS__))
#define WRAP_ARGS113(w, o, _1, ...) \
  w(o, _1), YLT_MACRO_EXPAND(WRAP_ARGS112(w, o, __VA_ARGS__))
#define WRAP_ARGS114(w, o, _1, ...) \
  w(o, _1), YLT_MACRO_EXPAND(WRAP_ARGS113(w, o, __VA_ARGS__))
#define WRAP_ARGS115(w, o, _1, ...) \
  w(o, _1), YLT_MACRO_EXPAND(WRAP_ARGS114(w, o, __VA_ARGS__))
#define WRAP_ARGS116(w, o, _1, ...) \
  w(o, _1), YLT_MACRO_EXPAND(WRAP_ARGS115(w, o, __VA_ARGS__))
#define WRAP_ARGS117(w, o, _1, ...) \
  w(o, _1), YLT_MACRO_EXPAND(WRAP_ARGS116(w, o, __VA_ARGS__))
#define WRAP_ARGS118(w, o, _1, ...) \
  w(o, _1), YLT_MACRO_EXPAND(WRAP_ARGS117(w, o, __VA_ARGS__))
#define WRAP_ARGS119(w, o, _1, ...) \
  w(o, _1), YLT_MACRO_EXPAND(WRAP_ARGS118(w, o, __VA_ARGS__))
#define WRAP_ARGS120(w, o, _1, ...) \
  w(o, _1), YLT_MACRO_EXPAND(WRAP_ARGS119(w, o, __VA_ARGS__))
#define WRAP_ARGS121(w, o, _1, ...) \
  w(o, _1), YLT_MACRO_EXPAND(WRAP_ARGS120(w, o, __VA_ARGS__))
#define WRAP_ARGS122(w, o, _1, ...) \
  w(o, _1), YLT_MACRO_EXPAND(WRAP_ARGS121(w, o, __VA_ARGS__))
#define WRAP_ARGS123(w, o, _1, ...) \
  w(o, _1), YLT_MACRO_EXPAND(WRAP_ARGS122(w, o, __VA_ARGS__))
#define WRAP_ARGS124(w, o, _1, ...) \
  w(o, _1), YLT_MACRO_EXPAND(WRAP_ARGS123(w, o, __VA_ARGS__))
#define WRAP_ARGS125(w, o, _1, ...) \
  w(o, _1), YLT_MACRO_EXPAND(WRAP_ARGS124(w, o, __VA_ARGS__))
