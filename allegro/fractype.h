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
#define MANDELFP                 0     /*   0 mandel           */
#define NEWTBASIN                1     /*   1 newtbasin        */
#define NEWTON                   2     /*   2 newton           */
#define JULIAFP                  3     /*   3 julia            */
#define PLASMA                   4     /*   4 plasma           */
#define MANDELTRIGFP             5     /*   5 mandelfn         */
#define MANOWARFP                6     /*   6 manowar          */
#define TEST                     7     /*   7 test             */
#define SQRTRIGFP                8     /*   8 sqr(fn)          */
#define IFS                      9     /*   9 ifs              */
#define IFS3D                   10     /*  10 ifs3d            */
#define TRIGSQRFP               11     /*  11 fn(z*z)          */
#define BIFURCATION             12     /*  12 bifurcation      */
#define TRIGPLUSTRIGFP          13     /*  13 fn+fn            */
#define TRIGXTRIGFP             14     /*  14 fn*fn            */
#define SQR1OVERTRIGFP          15     /*  15 sqr(1/fn)        */
#define ZXTRIGPLUSZFP           16     /*  16 fn*z+z           */
#define KAMFP                   17     /*  17 kamtorus         */
#define KAM3DFP                 18     /*  18 kamtorus3d       */
#define FPMANTRIGPLUSZSQRD      19     /*  19 manfn+zsqrd      */
#define FPJULTRIGPLUSZSQRD      20     /*  20 julfn+zsqrd      */
#define LAMBDATRIGFP            21     /*  21 lambdafn         */
#define FPMANDELZPOWER          22     /*  22 manzpower        */
#define FPJULIAZPOWER           23     /*  23 julzpower        */
#define FPMANZTOZPLUSZPWR       24     /*  24 manzzpwr         */
#define FPJULZTOZPLUSZPWR       25     /*  25 julzzpwr         */
#define FPMANTRIGPLUSEXP        26     /*  26 manfn+exp        */
#define FPJULTRIGPLUSEXP        27     /*  27 julfn+exp        */
#define FPPOPCORN               28     /*  28 popcorn          */
#define FPLORENZ                29     /*  29 lorenz           */
#define COMPLEXNEWTON           30     /*  30 complexnewton    */
#define COMPLEXBASIN            31     /*  31 complexbasin     */
#define COMPLEXMARKSMAND        32     /*  32 cmplxmarksmand   */
#define COMPLEXMARKSJUL         33     /*  33 cmplxmarksjul    */
#define FFORMULA                34     /*  34 formula          */
#define SIERPINSKIFP            35     /*  35 sierpinski       */
#define LAMBDAFP                36     /*  36 lambda           */
#define BARNSLEYM1FP            37     /*  37 barnsleym1       */
#define BARNSLEYJ1FP            38     /*  38 barnsleyj1       */
#define BARNSLEYM2FP            39     /*  39 barnsleym2       */
#define BARNSLEYJ2FP            40     /*  40 barnsleyj2       */
#define BARNSLEYM3FP            41     /*  41 barnsleym3       */
#define BARNSLEYJ3FP            42     /*  42 barnsleyj3       */
#define MANDELLAMBDAFP          43     /*  43 mandellambda     */
#define FPLORENZ3D              44     /*  44 lorenz3d         */
#define FPROSSLER               45     /*  45 rossler3d        */
#define FPHENON                 46     /*  46 henon            */
#define FPPICKOVER              47     /*  47 pickover         */
#define FPGINGERBREAD           48     /*  48 gingerbreadman   */
#define DIFFUSION               49     /*  49 diffusion        */
#define UNITYFP                 50     /*  50 unity            */
#define SPIDERFP                51     /*  51 spider           */
#define TETRATEFP               52     /*  52 tetrate          */
#define MAGNET1M                53     /*  53 magnet1m         */
#define MAGNET1J                54     /*  54 magnet1j         */
#define MAGNET2M                55     /*  55 magnet2m         */
#define MAGNET2J                56     /*  56 magnet2j         */
#define BIFLAMBDA               57     /*  57 biflambda        */
#define BIFADSINPI              58     /*  58 bif+sinpi        */
#define BIFEQSINPI              59     /*  59 bif=sinpi        */
#define FPPOPCORNJUL            60     /*  60 popcornjul       */
#define LSYSTEM                 61     /*  61 lsystem          */
#define MANOWARJFP              62     /*  62 manowarj         */
#define FNPLUSFNPIXFP           63     /*  63 fn(z)+fn(pix)    */
#define MARKSMANDELPWRFP        64     /*  64 marksmandelpwr   */
#define TIMSERRORFP             65     /*  65 tim's_error      */
#define BIFSTEWART              66     /*  66 bifstewart       */
#define FPHOPALONG              67     /*  67 hopalong         */
#define FPCIRCLE                68     /*  68 circle           */
#define FPMARTIN                69     /*  69 martin           */
#define LYAPUNOV                70     /*  70 lyapunov         */
#define FPLORENZ3D1             71     /*  71 lorenz3d1        */
#define FPLORENZ3D3             72     /*  72 lorenz3d3        */
#define FPLORENZ3D4             73     /*  73 lorenz3d4        */
#define FPLAMBDAFNFN            74     /*  74 lambda(fn||fn)   */
#define FPJULFNFN               75     /*  75 julia(fn||fn)    */
#define FPMANLAMFNFN            76     /*  76 manlam(fn||fn)   */
#define FPMANFNFN               77     /*  77 mandel(fn||fn)   */
#define BIFMAY                  78     /*  78 bifmay           */
#define HALLEY                  79     /*  79 halley           */
#define DYNAMICFP               80     /*  80 dynamic          */
#define QUATFP                  81     /*  81 quat             */
#define QUATJULFP               82     /*  82 quatjul          */
#define CELLULAR                83     /*  83 cellular         */
#define JULIBROTFP              84     /*  84 julibrot         */
#define INVERSEJULIAFP          85     /*  85 julia_inverse    */
#define MANDELCLOUD             86     /*  86 mandelcloud      */
#define PHOENIXFP               87     /*  87 phoenix          */
#define MANDPHOENIXFP           88     /*  88 mandphoenix      */
#define HYPERCMPLXFP            89     /*  89 hypercomplex     */
#define HYPERCMPLXJFP           90     /*  90 hypercomplexj    */
#define FROTHFP                 91     /*  91 frothybasin      */
#define MANDEL4FP               92     /*  92 mandel4          */
#define JULIA4FP                93     /*  93 julia4           */
#define MARKSMANDELFP           94     /*  94 marksmandel      */
#define MARKSJULIAFP            95     /*  95 marksjulia       */
#define ICON                    96     /*  96 icons            */
#define ICON3D                  97     /*  97 icons3d          */
#define PHOENIXFPCPLX           98     /*  98 phoenixcplx      */
#define MANDPHOENIXFPCPLX       99     /*  99 mandphoenixclx   */
#define ANT                    100     /* 100 ant              */
#define CHIP                   101     /* 101 chip             */
#define QUADRUPTWO             102     /* 102 quadruptwo       */
#define THREEPLY               103     /* 103 threeply         */
#define VL                     104     /* 104 volterra-lotka   */
#define ESCHER                 105     /* 105 escher_julia     */
#define LATOO                  106     /* 106 latoocarfian     */

#endif
