/* file menu items */

#define ID_CANCEL	102

#define     IDM_NEW	 100
#define     IDM_OPEN	 101
#define     IDM_SAVE	 102
#define     IDM_SAVEAS	 103
#define     IDM_PRINT	 104
#define     IDM_MAPIN	 115
#define     IDM_MAPOUT	 116
#define     IDM_3D	 113
#define     IDM_3DOVER	 114
#define     IDM_EXIT	 105
#define     IDM_ABOUT	 106
#define     IDM_COPY	 117
#define     IDM_PARFILE  118
#define     IDM_SAVEPAR  119

#define     IDM_HELP_INDEX     107
#define     IDM_HELP_KEYBOARD  108
#define     IDM_HELP_HELP      109
#define     IDM_HELP_FRACTINT  201
#define     ID_VERSION	       110
#define     ID_COMMENT	       111
#define     ID_COMMENT2        112
#define     ID_COMMENT3        113
#define     ID_COMMENT4        114
#define     ID_COMMENT5        115
#define     ID_COMMENT6        116
#define     ID_COMMENT7        117
#define     ID_COMMENT8        118
#define     ID_COMMENT9        119
#define     ID_COMMENT10       120
#define     ID_FILETITLE       112
#define     ID_LISTTITLE       112

#define     IDS_STATUS	  340

/* fractal formula stuff */

#define     IDM_FRACTAL  171
#define     IDM_FORMULA  172
#define     IDM_IMAGE    173
#define     IDM_DOODADX  174
#define     IDM_DOODADY  175
#define     IDM_DOODADZ  176
#define     IDM_CYCLE    177
#define     IDM_IFS3D    178
#define     IDM_STARFIELD  179
#define     IDM_PIXELS   180
#define     IDM_ORBITS   181
#define     IDM_RESTART  182
#define     IDM_PASSES   183

#define     ID_GIF89A	113
#define     ID_GIF87A	114
#define     ID_BMP	115

/* Control IDs */

#define     IDC_FILENAME  400
#define     IDC_EDIT	  401
#define     IDC_FILES	  402
#define     IDC_PATH	  403
#define     IDC_LISTBOX   404

/* hot-keys (Fractint-compatible accelerator keys) */

#define     IDF_FRACTINTSTYLE  599
#define     IDF_WINFRACTSTYLE  598

#define     IDF_HELP_INDEX     500
#define     IDF_HELP_FRACTINT  501
#define     IDF_HOTNOZOOM      502
#define     IDF_HOTCYCLEON     503
#define     IDF_HOTCYCLERIGHT  504
#define     IDF_HOTCYCLELEFT   505
#define     IDF_HOTCYCLERAND   506
#define     IDF_HOTCYCLEFAST   507
#define     IDF_HOTCYCLESLOW   508
#define     IDF_STATUS	       509
#define     IDF_IMAGE	       510
#define     IDF_FORMULA        511
#define     IDF_OPEN	       512
#define     IDF_SAVE	       513
#define     IDF_3D	       514
#define     IDF_3DOVER	       515
#define     IDF_PRINT	       516
#define     IDF_DOODADX        517
#define     IDF_DOODADY        518
#define     IDF_DOODADZ        519
#define     IDF_CYCLE	       520
#define     IDF_PARFILE        521
#define     IDF_SAVEPAR        522
#define     IDF_MAPIN	       523
#define     IDF_MAPOUT	       524
#define     IDF_HOTCYCLELSTEP  525
#define     IDF_HOTCYCLERSTEP  526
#define     IDF_IFS3D          527
#define     IDF_STARFIELD      528
#define     IDF_RESTART        529
#define     IDF_PASSES         530
#define     IDF_CMDSTRING      534
#define     IDF_HISTORY_B      535
#define     IDF_HISTORY_F      536

/* Math Tools */
#define     IDM_MATH_TOOLS    1001
#define     IDM_COORD	      1002
#define     IDM_ZOOM	      1003
#define     IDM_SIZING	      1004
#define     IDM_TRACKING      1005
/* #define     IDM_ZOOMIN        1006 this is odd!! */
#define     IDM_ZOOMIN        1060
#define     IDM_ZOOMOUT       1007

/* Print stuff */

#define     ID_PR_DEVICE  450
#define     ID_PR_ORIENT  451
#define     ID_PRO_PORTR  452
#define     ID_PRO_LANDS  453
#define     ID_PR_SIZE	  454
#define     ID_PRS_MAX	  455
#define     ID_PRS_MAXSIZ 456
#define     ID_PRS_CUST   457
#define     ID_PRS_WIDTH  458

/* Windows 3.0 vs 3.1 SDK patch */

#ifndef COLOR_ENDCOLORS
#define COLOR_ENDCOLORS 	18
#endif

int PASCAL WinMain(HINSTANCE, HINSTANCE, LPSTR, int);
BOOL InitApplication(HANDLE);
BOOL InitInstance(HANDLE, int);
LRESULT CALLBACK MainWndProc(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK SaveStatusProc(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK About(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK HelpBox(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK SelectFractal(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK SelectFracParams(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK SelectImage(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK SelectDoodads(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK SelectExtended(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK SelectSavePar(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK SelectCycle(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK Select3D(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK Select3DPlanar(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK Select3DSpherical(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK SelectFullScreen(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK Status(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK SelectIFS3D(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK SelectFunnyGlasses(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK SelectLightSource(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK SelectStarfield(HWND, UINT, WPARAM, LPARAM);
void SeparateFile(HWND, LPSTR, LPSTR, LPSTR);
void UpdateListBox(HWND);
void AddExt(PSTR, PSTR);
void ChangeDefExt(PSTR, PSTR);
HDC PASCAL GetPrinterDC();
void OpenStatusBox(HWND, HANDLE);
void CloseStatusBox(void);
void UpdateStatusBox(unsigned long, unsigned long);
extern char far StatusTitle[];
void SaveBitmapFile(HWND, char *);
extern void win_title_text(int);
int set_win_offset(void);
void win_savedac(void);
int default_dib_palette(void);
void win_kill_all_zooming(void);
int win_stop_cycling(void);
int win_oktocycle(void);
int win_copy_to_clipboard(void);
void check_funnyglasses_name(void);
int get_formula_names(void);
int parse_formula_names(void);
int get_lsys_name(void);
BOOL cant_clip(void);
extern void mono_dib_palette(void);
extern int rgb_dib_palette(void);
extern void SecondaryWndProc(void);


/*  prototypes are here because windows.h doesn't play nice with others */
/*  BOOL is defined in windows.h  */

/*  profile -- C file prototypes */

extern void SetToolsPath(void);
extern BOOL GetParamSwitch(char *);
extern void SaveParamSwitch(char *, BOOL);
extern void PositionWindow(HWND, char *);
extern void SaveWindowPosition(HWND, char *);
extern void InitializeParameters(HWND);
extern void SaveParameters(HWND);
extern void SaveIntParam(char *, int);

/*  wintext -- C file prototypes */

extern BOOL wintext_initialize(HANDLE, HWND, LPSTR);

