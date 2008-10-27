#ifndef FRACTYPE_H
#define FRACTYPE_H

#define SIN             0
#define COS             1   /* Beware this is really COSXX */
#define SINH            2
#define COSH            3
#define EXP             4
#define LOG             5
#define SQR             6
#define TAN            10

/* These MUST match the corresponding fractalspecific record in fractals.c */
#define NOFRACTAL               -1
#define MANDEL                   0
#define JULIA                    1
#define NEWTBASIN                2
#define LAMBDA                   3
#define MANDELFP                 4
#define NEWTON                   5
#define JULIAFP                  6
#define PLASMA                   7
#define LAMBDASINE               8 /* obsolete */
#define MANDELTRIGFP             8
#define LAMBDACOS                9 /* obsolete */
#define MANOWARFP                9
#define LAMBDAEXP               10 /* obsolete */
#define MANOWAR                 10
#define TEST                    11
#define SIERPINSKI              12
#define BARNSLEYM1              13
#define BARNSLEYJ1              14
#define BARNSLEYM2              15
#define BARNSLEYJ2              16
#define MANDELSINE              17 /* obsolete */
#define SQRTRIG                 17
#define MANDELCOS               18 /* obsolete */
#define SQRTRIGFP               18
#define MANDELEXP               19 /* obsolete */
#define TRIGPLUSTRIG            19
#define MANDELLAMBDA            20
#define MARKSMANDEL             21
#define MARKSJULIA              22
#define UNITY                   23
#define MANDEL4                 24
#define JULIA4                  25
#define IFS                     26
#define IFS3D                   27
#define BARNSLEYM3              28
#define BARNSLEYJ3              29
#define DEMM                    30 /* obsolete */
#define TRIGSQR                 30
#define DEMJ                    31 /* obsolete */
#define TRIGSQRFP               31
#define BIFURCATION             32
#define MANDELSINH              33 /* obsolete */
#define TRIGPLUSTRIGFP          33
#define LAMBDASINH              34 /* obsolete */
#define TRIGXTRIG               34
#define MANDELCOSH              35 /* obsolete */
#define TRIGXTRIGFP             35
#define LAMBDACOSH              36 /* obsolete */
#define SQR1OVERTRIG            36
#define LMANDELSINE             37 /* obsolete */
#define SQR1OVERTRIGFP          37
#define LLAMBDASINE             38 /* obsolete */
#define ZXTRIGPLUSZ             38
#define LMANDELCOS              39 /* obsolete */
#define ZXTRIGPLUSZFP           39
#define LLAMBDACOS              40 /* obsolete */
#define KAMFP                   40
#define LMANDELSINH             41 /* obsolete */
#define KAM                     41
#define LLAMBDASINH             42 /* obsolete */
#define KAM3DFP                 42
#define LMANDELCOSH             43 /* obsolete */
#define KAM3D                   43
#define LLAMBDACOSH             44 /* obsolete */
#define LAMBDATRIG              44
#define LMANTRIGPLUSZSQRD       45
#define LJULTRIGPLUSZSQRD       46
#define FPMANTRIGPLUSZSQRD      47
#define FPJULTRIGPLUSZSQRD      48
#define LMANDELEXP              49 /* obsolete */
#define LAMBDATRIGFP            49
#define LLAMBDAEXP              50 /* obsolete */
#define MANDELTRIG              50
#define LMANDELZPOWER           51
#define LJULIAZPOWER            52
#define FPMANDELZPOWER          53
#define FPJULIAZPOWER           54
#define FPMANZTOZPLUSZPWR       55
#define FPJULZTOZPLUSZPWR       56
#define LMANTRIGPLUSEXP         57
#define LJULTRIGPLUSEXP         58
#define FPMANTRIGPLUSEXP        59
#define FPJULTRIGPLUSEXP        60
#define FPPOPCORN               61
#define LPOPCORN                62
#define FPLORENZ                63
#define LLORENZ                 64
#define LLORENZ3D               65
#define MPNEWTON                66
#define MPNEWTBASIN             67
#define COMPLEXNEWTON           68
#define COMPLEXBASIN            69
#define COMPLEXMARKSMAND        70
#define COMPLEXMARKSJUL         71
#define FORMULA                 72
#define FFORMULA                73
#define SIERPINSKIFP            74
#define LAMBDAFP                75
#define BARNSLEYM1FP            76
#define BARNSLEYJ1FP            77
#define BARNSLEYM2FP            78
#define BARNSLEYJ2FP            79
#define BARNSLEYM3FP            80
#define BARNSLEYJ3FP            81
#define MANDELLAMBDAFP          82
#define JULIBROT                83
#define FPLORENZ3D              84
#define LROSSLER                85
#define FPROSSLER               86
#define LHENON                  87
#define FPHENON                 88
#define FPPICKOVER              89
#define FPGINGERBREAD           90
#define DIFFUSION               91
#define UNITYFP                 92
#define SPIDERFP                93
#define SPIDER                  94
#define TETRATEFP               95
#define MAGNET1M                96
#define MAGNET1J                97
#define MAGNET2M                98
#define MAGNET2J                99
#define LBIFURCATION           100
#define LBIFLAMBDA             101
#define BIFLAMBDA              102
#define BIFADSINPI             103
#define BIFEQSINPI             104
#define FPPOPCORNJUL           105
#define LPOPCORNJUL            106
#define LSYSTEM                107
#define MANOWARJFP             108
#define MANOWARJ               109
#define FNPLUSFNPIXFP          110
#define FNPLUSFNPIXLONG        111
#define MARKSMANDELPWRFP       112
#define MARKSMANDELPWR         113
#define TIMSERRORFP            114
#define TIMSERROR              115
#define LBIFEQSINPI            116
#define LBIFADSINPI            117
#define BIFSTEWART             118
#define LBIFSTEWART            119
#define FPHOPALONG             120
#define FPCIRCLE               121
#define FPMARTIN               122
#define LYAPUNOV               123
#define FPLORENZ3D1            124
#define FPLORENZ3D3            125
#define FPLORENZ3D4            126
#define LLAMBDAFNFN            127
#define FPLAMBDAFNFN           128
#define LJULFNFN               129
#define FPJULFNFN              130
#define LMANLAMFNFN            131
#define FPMANLAMFNFN           132
#define LMANFNFN               133
#define FPMANFNFN              134
#define LBIFMAY                135
#define BIFMAY                 136
#define MPHALLEY               137
#define HALLEY                 138
#define DYNAMICFP              139
#define QUATFP                 140
#define QUATJULFP              141
#define CELLULAR               142
#define JULIBROTFP             143
#define INVERSEJULIA           144
#define INVERSEJULIAFP         145
#define MANDELCLOUD            146
#define PHOENIX                147
#define PHOENIXFP              148
#define MANDPHOENIX            149
#define MANDPHOENIXFP          150
#define HYPERCMPLXFP           151
#define HYPERCMPLXJFP          152
#define FROTH                  153
#define FROTHFP                154
#define MANDEL4FP              155
#define JULIA4FP               156
#define MARKSMANDELFP          157
#define MARKSJULIAFP           158
#define ICON                   159
#define ICON3D                 160
#define PHOENIXCPLX            161
#define PHOENIXFPCPLX          162
#define MANDPHOENIXCPLX        163
#define MANDPHOENIXFPCPLX      164
#define ANT                    165
#define CHIP                   166
#define QUADRUPTWO             167
#define THREEPLY               168
#define VL                     169
#define ESCHER                 170
#define LATOO                  171
/* #define MANDELBROTMIX4         172 */
#define DIVIDEBROT5            172
#endif
