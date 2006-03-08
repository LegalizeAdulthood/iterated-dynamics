/*

        various dialog-box code

*/

#define STRICT

#include "port.h"
#include "prototyp.h"

#include <windows.h>
#include <commdlg.h>
#include <print.h>
#include <string.h>
#include <direct.h>

#include "winfract.h"
#include "dialog.h"
#include "fractype.h"
#include "mathtool.h"
#include "externs.h"
#include "profile.h"

extern HWND hwnd;                               /* handle to main window */
extern char szHelpFileName[];                   /* Help file name*/

extern BOOL zoomflag;                /* TRUE is a zoom-box selected */

extern char *win_choices[];
extern int win_numchoices, win_choicemade;
int CurrentFractal;

extern HANDLE hDibInfo;                /* handle to the Device-independent bitmap */
extern LPBITMAPINFO pDibInfo;                /* pointer to the DIB info */

extern int time_to_restart;                               /* time to restart?  */
extern int time_to_reinit;                                /* time to reinit? */
extern int time_to_cycle;                               /* time to cycle? */

extern int xdots, ydots, colors;
extern long maxiter;
extern int ytop, ybottom, xleft, xright;

#if 0
extern double xxmin, xxmax, yymin, yymax;
extern int fractype;
extern int calc_status;
extern double param[4];
extern long bailout;
extern struct moreparams far moreparams[];

extern int inside, outside, usr_biomorph, decomp, debugflag;
extern int usr_stdcalcmode, usr_floatflag;
extern        int        invert;         /* non-zero if inversion active */
extern        double        inversion[3];        /* radius, xcenter, ycenter */
extern int numtrigfn;

extern int LogFlag, fillcolor;
extern unsigned char readname[];
#endif

int win_temp1, win_temp2, win_temp3, win_temp4;
long win_ltemp2;


int numparams,numtrig, numextra;
static char *trg[] = {"First Function","Second Function",
                      "Third Function","Fourth Function"};
static int paramt[] = {ID_FRACPARTX1, ID_FRACPARTX2,
                       ID_FRACPARTX3, ID_FRACPARTX4,
                       ID_FRACPARTX5, ID_FRACPARTX6 };
static int paramv[] = {ID_FRACPARAM1, ID_FRACPARAM2,
                       ID_FRACPARAM3, ID_FRACPARAM4,
                       ID_FRACPARAM5, ID_FRACPARAM6, };

extern int win_release;
extern char far win_comment[];

extern char far DialogTitle[];
extern unsigned char DefExt[];
extern unsigned char FullPathName[];

double far win_oldprompts[20];

extern int stopmsg(int, char far *);

/* far strings (near space is precious) */
char far about_msg01[] = "(C) 1990-2006 The Stone Soup Group";
char far about_msg02[] = "";
char far about_msg03[] = "";
char far about_msg04[] = "";
char far about_msg05[] = "Winfract is copyrighted freeware and may not be";
char far about_msg06[] = "distributed for commercial or promotional purposes";
char far about_msg07[] = "without written permission from the Stone Soup Group.";
char far about_msg08[] = "Distribution of Winfract by BBS, network, and";
char far about_msg09[] = "software shareware distributors, etc. is encouraged.";
char far about_msg10[] = "";

BOOL CALLBACK About(hDlg, message, wParam, lParam)
HWND hDlg;
UINT message;
WPARAM wParam;
LPARAM lParam;
{

extern char far winfract_title_text[];
char about_msg00[80];

    switch (message) {

        case WM_INITDIALOG:
            sprintf(about_msg00,"Winfract version %d.%02d, Fractals for Windows release",
                win_release/100, win_release%100);
            SetDlgItemText(hDlg, ID_VERSION  ,about_msg00);
            SetDlgItemText(hDlg, ID_COMMENT  ,about_msg01);
            SetDlgItemText(hDlg, ID_COMMENT2 ,about_msg02);
            SetDlgItemText(hDlg, ID_COMMENT3 ,about_msg03);
            SetDlgItemText(hDlg, ID_COMMENT4 ,about_msg04);
            SetDlgItemText(hDlg, ID_COMMENT5 ,about_msg05);
            SetDlgItemText(hDlg, ID_COMMENT6 ,about_msg06);
            SetDlgItemText(hDlg, ID_COMMENT7 ,about_msg07);
            SetDlgItemText(hDlg, ID_COMMENT8 ,about_msg08);
            SetDlgItemText(hDlg, ID_COMMENT9 ,about_msg09);
            SetDlgItemText(hDlg, ID_COMMENT10,about_msg10);
            return (TRUE);

        case WM_COMMAND:
            if (wParam == IDOK
                || wParam == IDCANCEL) {
                EndDialog(hDlg, TRUE);
                return (TRUE);
            }
            break;
    }
    return (FALSE);
}


BOOL CALLBACK Status(hDlg, message, wParam, lParam)
HWND hDlg;
UINT message;
WPARAM wParam;
LPARAM lParam;
{
char tempstring[100];
    switch (message) {

        case WM_INITDIALOG:
            sprintf(tempstring,"fractal type: ");
            if (fractalspecific[fractype].name[0] != '*')
                strcat(tempstring, fractalspecific[fractype].name);
            else
                strcat(tempstring, &fractalspecific[fractype].name[1]);
            if (calc_status == 1)
                strcat(tempstring,"    (still being calculated)");
            else if (calc_status == 2)
                strcat(tempstring,"    (interrupted, resumable)");
            else if (calc_status == 3)
                strcat(tempstring,"    (interrupted, not resumable)");
            else
                strcat(tempstring,"    (completed)");
            /* ##### */
            SetDlgItemText(hDlg, IDS_LINE1,tempstring);
            if(fractalspecific[fractype].param[0][0] == 0)
                tempstring[0] = 0;
            else
                sprintf(tempstring,"%-30.30s   %14.10f",
                    fractalspecific[fractype].param[0], param[0]);
            SetDlgItemText(hDlg, IDS_LINE2,tempstring);
            if(fractalspecific[fractype].param[1][0] == 0)
                tempstring[0] = 0;
            else
                sprintf(tempstring,"%-30.30s   %14.10f",
                    fractalspecific[fractype].param[1], param[1]);
            SetDlgItemText(hDlg, IDS_LINE3,tempstring);
            if(fractalspecific[fractype].param[2][0] == 0)
                tempstring[0] = 0;
            else
                sprintf(tempstring,"%-30.30s   %14.10f",
                    fractalspecific[fractype].param[2], param[2]);
            SetDlgItemText(hDlg, IDS_LINE4,tempstring);
            if(fractalspecific[fractype].param[3][0] == 0)
                tempstring[0] = 0;
            else
                sprintf(tempstring,"%-30.30s   %14.10f",
                    fractalspecific[fractype].param[3], param[3]);
            SetDlgItemText(hDlg, IDS_LINE5,tempstring);
            sprintf(tempstring,"Xmin:        %25.16f", xxmin);
            SetDlgItemText(hDlg, IDS_LINE6,tempstring);
            sprintf(tempstring,"Xmax:        %25.16f", xxmax);
            SetDlgItemText(hDlg, IDS_LINE7,tempstring);
            sprintf(tempstring,"Ymin:        %25.16f", yymin);
            SetDlgItemText(hDlg, IDS_LINE8,tempstring);
            sprintf(tempstring,"Ymax:        %25.16f", yymax);
            SetDlgItemText(hDlg, IDS_LINE9,tempstring);
            return (TRUE);

        case WM_COMMAND:
            if (wParam == IDOK
                || wParam == IDCANCEL) {
                EndDialog(hDlg, TRUE);
                return (TRUE);
            }
            break;
    }
    return (FALSE);
}


BOOL CALLBACK SelectFractal(hDlg, message, wParam, lParam)
HWND hDlg;
UINT message;
WPARAM wParam;
LPARAM lParam;
{

    int i;
    int index;

    switch (message) {

        case WM_INITDIALOG:
            SetDlgItemText(hDlg, ID_LISTTITLE,         DialogTitle);
            for (i = 0; i < win_numchoices; i++)
                SendDlgItemMessage(hDlg, IDM_FRACTAL, LB_ADDSTRING,
                    0, (LONG) (LPSTR) win_choices[i]);
            SendDlgItemMessage(hDlg, IDM_FRACTAL, LB_SETCURSEL,
                win_choicemade, 0L);
            return (TRUE);

        case WM_COMMAND:
            switch (wParam) {

                case IDOK:
okay:
                    index = (int)SendDlgItemMessage(hDlg, IDM_FRACTAL,
                        LB_GETCURSEL, 0, 0L);
                    if (index == LB_ERR) {
                        MessageBox(hDlg, "No Choice selected",
                            "Select From a List", MB_OK | MB_ICONEXCLAMATION);
                        break;
                        }
                    win_choicemade = index;
                    EndDialog(hDlg, 1);
                    break;

                case IDCANCEL:
                    win_choicemade = -1;
                    EndDialog(hDlg, 0);
                    break;

                case IDM_FRACTAL:
                    switch (HIWORD(lParam)) {
                        case LBN_SELCHANGE:
                            index = (int)SendDlgItemMessage(hDlg, IDM_FRACTAL,
                                LB_GETCURSEL, 0, 0L);
                            if (index == LB_ERR)
                                break;
                            break;

                       case LBN_DBLCLK:
                            goto okay;

                    }
                return (TRUE);
                }

        }
    return (FALSE);
}


BOOL CALLBACK SelectFracParams(hDlg, message, wParam, lParam)
HWND hDlg;
UINT message;
WPARAM wParam;
LPARAM lParam;
{

    int i, j;
    char temp[30];

    switch (message) {

        case WM_INITDIALOG:
                win_temp1 = CurrentFractal;
                SetDlgItemText(hDlg, ID_FRACNAME,   fractalspecific[win_temp1].name);
                for (numparams = 0; numparams < 4; numparams++)
                    if (fractalspecific[win_temp1].param[numparams][0] == 0)
                        break;
                numtrig = (fractalspecific[win_temp1].flags >> 6) & 7;
                if (numparams+numtrig > 6) numparams = 6 - numtrig;
                for (i = 0; i < 6; i++) {
                    temp[0] = 0;
                    if (i < numparams)
                        sprintf(temp,"%f",param[i]);
                    SetDlgItemText(hDlg, paramv[i], temp);
                    SetDlgItemText(hDlg, paramt[i],"(n/a)");
                    if (i < numparams)
                       SetDlgItemText(hDlg, paramt[i], fractalspecific[win_temp1].param[i]);
                    }
               for(i=0; i<numtrig; i++) {
                    SetDlgItemText(hDlg, paramt[i+numparams], trg[i]);
                    SetDlgItemText(hDlg, paramv[i+numparams],
                        trigfn[trigndx[i]].name);
                    }
                numextra = 0;
                if (fractalspecific[win_temp1].flags & MORE) {
                    /* uh-oh - over four parameters! */
                    int i, extra;
                    if ((extra = find_extra_param(win_temp1)) > -1)
                        for (i = 0; i < 6 - numparams - numtrig; i++)
                            if (moreparams[extra].param[i][0] != 0) {
                                numextra++;
                                sprintf(temp,"%f",
                                    moreparams[extra].paramvalue[i]);
                                SetDlgItemText(hDlg, paramt[i+numparams+numtrig],
                                    moreparams[extra].param[i]);
                                SetDlgItemText(hDlg, paramv[i+numparams+numtrig],
                                    temp);
                                }
                    }
                sprintf(temp,"%d",bailout);
                SetDlgItemText(hDlg, ID_BAILOUT,   temp);
                sprintf(temp,"%.12f",xxmin);
                SetDlgItemText(hDlg, ID_FRACXMIN,   temp);
                sprintf(temp,"%.12f",xxmax);
                SetDlgItemText(hDlg, ID_FRACXMAX,   temp);
                sprintf(temp,"%.12f",yymin);
                SetDlgItemText(hDlg, ID_FRACYMIN,   temp);
                sprintf(temp,"%.12f",yymax);
                SetDlgItemText(hDlg, ID_FRACYMAX,   temp);
                return (TRUE);

        case WM_COMMAND:
            switch (wParam) {

                case IDOK:
                    {
                    for (i = 0; i < numtrig; i++) {
                        GetDlgItemText(hDlg, paramv[i+numparams], temp, 10);
                        temp[6] = 0;
                        for (j = 0; j <= 6; j++)
                            if(temp[j] == ' ') temp[j] = 0;
                        strlwr(temp);
                        for(j=0;j<numtrigfn;j++)
                            if(strcmp(temp,trigfn[j].name)==0)
                                break;
                        if (j >= numtrigfn) {
                            char oops[80];
                            sprintf(oops, "Trig param %d, '%s' is not a valid trig function\n", i+1, temp);
                            strcat(oops, "Try sin, cos, tan, cotan, sinh, etc.");
                            stopmsg(0,oops);
                            break;
                            }
                        }
                        if (i != numtrig) break;
                    }
                    for (i = 0; i < numparams; i++) {
                        GetDlgItemText(hDlg, paramv[i], temp, 20);
                        param[i] = atof(temp);
                        }
                    for (i = 0; i < numtrig; i++) {
                        GetDlgItemText(hDlg, paramv[i+numparams], temp, 10);
                        temp[6] = 0;
                        for (j = 0; j <= 6; j++)
                            if (temp[j] == 32) temp[j] = 0;
                        set_trig_array(i, temp);
                        }
                    for (i = 0; i < numextra; i++) {
                        GetDlgItemText(hDlg, paramv[i+numparams+numtrig], temp, 20);
                        param[i+4] = atof(temp);
                        }
                    GetDlgItemText(hDlg, ID_BAILOUT   , temp, 10);
                    bailout = (long)atof(temp);
                    GetDlgItemText(hDlg, ID_FRACXMIN  , temp, 20);
                    xxmin = atof(temp);
                    GetDlgItemText(hDlg, ID_FRACXMAX  , temp, 20);
                    xxmax = atof(temp);
                    GetDlgItemText(hDlg, ID_FRACYMIN  , temp, 20);
                    yymin = atof(temp);
                    GetDlgItemText(hDlg, ID_FRACYMAX  , temp, 20);
                    yymax = atof(temp);
                    invert = 0;
                    inversion[0] = inversion[1] = inversion[2] = 0;
                    fractype = CurrentFractal;
                    EndDialog(hDlg, 1);
                    break;


                case IDCANCEL:
                    EndDialog(hDlg, 0);
                    break;

                }

        }
    return (FALSE);
}


BOOL CALLBACK SelectImage(hDlg, message, wParam, lParam)
HWND hDlg;
UINT message;
WPARAM wParam;
LPARAM lParam;
{

    int i;
    char temp[15];

    switch (message) {

        case WM_INITDIALOG:
            win_temp1 = colors;
            if (win_temp1 == 2)
                CheckDlgButton(hDlg, ID_ICOLORS1, 1);
            else if (win_temp1 == 16)
                CheckDlgButton(hDlg, ID_ICOLORS2, 1);
            else
                CheckDlgButton(hDlg, ID_ICOLORS3, 1);
            sprintf(temp,"%d",xdots);
            SetDlgItemText(hDlg, ID_ISIZEX, temp);
            sprintf(temp,"%d",ydots);
            SetDlgItemText(hDlg, ID_ISIZEY, temp);
            i = ID_ISIZE7;
            if (xdots ==  200 && ydots == 150) i = ID_ISIZE1;
            if (xdots ==  320 && ydots == 200) i = ID_ISIZE2;
            if (xdots ==  640 && ydots == 350) i = ID_ISIZE3;
            if (xdots ==  640 && ydots == 480) i = ID_ISIZE4;
            if (xdots ==  800 && ydots == 600) i = ID_ISIZE5;
            if (xdots == 1024 && ydots == 768) i = ID_ISIZE6;
            CheckRadioButton(hDlg, ID_ISIZE1, ID_ISIZE7, i);
            return (TRUE);

        case WM_COMMAND:
            switch (wParam) {

                case IDOK:
                    /* retrieve and validate the results */
                    GetDlgItemText(hDlg, ID_ISIZEX, temp, 10);
                    xdots = atoi(temp);
                    if (xdots < 50) xdots = 50;
                    if (xdots > 2048) xdots = 2048;
                    GetDlgItemText(hDlg, ID_ISIZEY, temp, 10);
                    ydots = atoi(temp);
                    if (ydots < 50) ydots = 50;
                    if (ydots > 2048) ydots = 2048;
                    colors = win_temp1;
                    win_savedac();
                    /* allocate and lock a pixel array for the bitmap */
                    /* problem, here - can't just RETURN!!! */
                    tryagain:
                    if (!clear_screen(0)) {
                        MessageBox(hDlg, "Not Enough Memory for that sized Image",
                            NULL, MB_OK | MB_ICONHAND);
                        xdots = ydots = 100;
                        goto tryagain;
                        };
                    ytop    = 0;                /* reset the zoom-box */
                    ybottom = ydots-1;
                    xleft   = 0;
                    xright  = xdots-1;
                    set_win_offset();
                    zoomflag = TRUE;
                    time_to_restart = 1;

                    ProgStr = Winfract;
                    SaveIntParam(ImageWidthStr, xdots);
                    SaveIntParam(ImageHeightStr, ydots);

                    EndDialog(hDlg, 1);
                    break;

                case IDCANCEL:
                    EndDialog(hDlg, 0);
                    break;

                case ID_ISIZE1:
                    CheckRadioButton(hDlg, ID_ISIZE1, ID_ISIZE7, ID_ISIZE1);
                    SetDlgItemInt(hDlg, ID_ISIZEX, 200, TRUE);
                    SetDlgItemInt(hDlg, ID_ISIZEY, 150, TRUE);
                    break;

                case ID_ISIZE2:
                    CheckRadioButton(hDlg, ID_ISIZE1, ID_ISIZE7, ID_ISIZE2);
                    SetDlgItemInt(hDlg, ID_ISIZEX, 320, TRUE);
                    SetDlgItemInt(hDlg, ID_ISIZEY, 200, TRUE);
                    break;

                case ID_ISIZE3:
                    CheckRadioButton(hDlg, ID_ISIZE1, ID_ISIZE7, ID_ISIZE3);
                    SetDlgItemInt(hDlg, ID_ISIZEX, 640, TRUE);
                    SetDlgItemInt(hDlg, ID_ISIZEY, 350, TRUE);
                    break;

                case ID_ISIZE4:
                    CheckRadioButton(hDlg, ID_ISIZE1, ID_ISIZE7, ID_ISIZE4);
                    SetDlgItemInt(hDlg, ID_ISIZEX, 640, TRUE);
                    SetDlgItemInt(hDlg, ID_ISIZEY, 480, TRUE);
                    break;

                case ID_ISIZE5:
                    CheckRadioButton(hDlg, ID_ISIZE1, ID_ISIZE7, ID_ISIZE5);
                    SetDlgItemInt(hDlg, ID_ISIZEX, 800, TRUE);
                    SetDlgItemInt(hDlg, ID_ISIZEY, 600, TRUE);
                    break;

                case ID_ISIZE6:
                    CheckRadioButton(hDlg, ID_ISIZE1, ID_ISIZE7, ID_ISIZE6);
                    SetDlgItemInt(hDlg, ID_ISIZEX, 1024, TRUE);
                    SetDlgItemInt(hDlg, ID_ISIZEY, 768, TRUE);
                    break;

                case ID_ISIZE7:
                    CheckRadioButton(hDlg, ID_ISIZE1, ID_ISIZE7, ID_ISIZE7);
                    break;

                case ID_ICOLORS1:
                    CheckRadioButton(hDlg, ID_ICOLORS1, ID_ICOLORS3, ID_ICOLORS1);
                    win_temp1 = 2;
                    break;

                case ID_ICOLORS2:
                    CheckRadioButton(hDlg, ID_ICOLORS1, ID_ICOLORS3, ID_ICOLORS2);
                    win_temp1 = 16;
                    break;

                case ID_ICOLORS3:
                    CheckRadioButton(hDlg, ID_ICOLORS1, ID_ICOLORS3, ID_ICOLORS3);
                    win_temp1 = 256;
                    break;

                }

        }
    return (FALSE);
}


BOOL CALLBACK SelectDoodads(hDlg, message, wParam, lParam)
HWND hDlg;
UINT message;
WPARAM wParam;
LPARAM lParam;
{

    char temp[80];

    switch (message) {

        case WM_INITDIALOG:
            win_temp1 = usr_floatflag;
            win_temp2 = 0;
            if (usr_stdcalcmode == '2') win_temp2 = 1;
            if (usr_stdcalcmode == 'g') win_temp2 = 2;
            if (usr_stdcalcmode == 'b') win_temp2 = 3;
            if (usr_stdcalcmode == 't') win_temp2 = 4;
            win_temp3 = 0;
            if (inside == -1)    win_temp3 = 1;
            if (inside == -59)   win_temp3 = 2;
            if (inside == -60)   win_temp3 = 3;
            if (inside == -61)   win_temp3 = 4;
            if (inside == -100)  win_temp3 = 5;
            if (inside == -101)  win_temp3 = 6;
            win_temp4 = 0;
            if (outside < 0 && outside > -6) win_temp4 = 0 - outside;
            CheckDlgButton(hDlg, ID_PASS1+win_temp2,1);
            CheckDlgButton(hDlg, ID_INSIDEC+win_temp3, 1);
            CheckDlgButton(hDlg, ID_OUTSIDEN+win_temp4, 1);
            if (win_temp1)
                CheckDlgButton(hDlg, ID_MATHF, 1);
            else
                CheckDlgButton(hDlg, ID_MATHF, 0);
            sprintf(temp,"%d",maxiter);
            SetDlgItemText(hDlg, ID_MAXIT, temp);
            sprintf(temp,"%d",usr_biomorph);
            SetDlgItemText(hDlg, ID_BIOMORPH, temp);
            sprintf(temp,"%d",LogFlag);
            SetDlgItemText(hDlg, ID_LOGP, temp);
            sprintf(temp,"%d",decomp);
            SetDlgItemText(hDlg, ID_DECOMP, temp);
            sprintf(temp,"%d",fillcolor);
            SetDlgItemText(hDlg, ID_FILLC, temp);
            sprintf(temp,"%d",max(inside,0));
            SetDlgItemText(hDlg, ID_INSIDE, temp);
            sprintf(temp,"%d",max(outside,0));
            SetDlgItemText(hDlg, ID_OUTSIDE, temp);
            return (TRUE);

        case WM_COMMAND:
            switch (wParam) {

                case IDOK:
                    /* retrieve and validate the results */
                    usr_stdcalcmode = '1';
                    if (win_temp2 == 1) usr_stdcalcmode = '2';
                    if (win_temp2 == 2) usr_stdcalcmode = 'g';
                    if (win_temp2 == 3) usr_stdcalcmode = 'b';
                    if (win_temp2 == 4) usr_stdcalcmode = 't';
                    usr_floatflag = win_temp1;
                    GetDlgItemText(hDlg, ID_MAXIT, temp, 10);
                    maxiter = atoi(temp);
                    if (maxiter < 10) maxiter = 10;
                    if (maxiter > 32000) maxiter = 32000;
                    GetDlgItemText(hDlg, ID_LOGP, temp, 10);
                    LogFlag = atoi(temp);
                    GetDlgItemText(hDlg, ID_BIOMORPH, temp, 10);
                    usr_biomorph = atoi(temp);
                    if (usr_biomorph < 0) usr_biomorph = -1;
                    if (usr_biomorph >= colors) usr_biomorph = colors-1;
                    GetDlgItemText(hDlg, ID_DECOMP, temp, 10);
                    decomp[0] = atoi(temp);
                    if (decomp[0] < 0) decomp[0] = 0;
                    if (decomp[0] > 256) decomp[0] = 256;
                    GetDlgItemText(hDlg, ID_FILLC, temp, 10);
                    fillcolor = atoi(temp);
                    GetDlgItemText(hDlg, ID_INSIDE, temp, 10);
                    inside = atoi(temp);
                    if (inside < 0) inside = 0;
                    if (inside >= colors) inside = colors-1;
                    if (win_temp3 == 1) inside = -1;
                    if (win_temp3 == 2) inside = -59;
                    if (win_temp3 == 3) inside = -60;
                    if (win_temp3 == 4) inside = -61;
                    if (win_temp3 == 5) inside = -100;
                    if (win_temp3 == 6) inside = -101;
                    GetDlgItemText(hDlg, ID_OUTSIDE, temp, 10);
                    outside = atoi(temp);
                    if (outside < 0) outside = -1;
                    if (outside >= colors) outside = colors-1;
                    if (win_temp4 > 0) outside = 0 - win_temp4;
                    time_to_restart = 1;
                    EndDialog(hDlg, 1);
                    break;

                case IDCANCEL:
                    EndDialog(hDlg, 0);
                    break;

                case ID_PASS1:
                case ID_PASS2:
                case ID_PASSS:
                case ID_PASSB:
                case ID_PASST:
                    win_temp2 = wParam - ID_PASS1;
                    CheckRadioButton(hDlg, ID_PASS1, ID_PASST, wParam);
                    break;

                case ID_INSIDEC:
                case ID_INSIDEM:
                case ID_INSIDEZ:
                case ID_INSIDE60:
                case ID_INSIDE61:
                case ID_INSIDEE:
                case ID_INSIDES:
                    win_temp3 = wParam - ID_INSIDEC;
                    CheckRadioButton(hDlg, ID_INSIDEC, ID_INSIDES, wParam);
                    break;

                case ID_OUTSIDEN:
                case ID_OUTSIDEIT:
                case ID_OUTSIDER:
                case ID_OUTSIDEIM:
                case ID_OUTSIDEM:
                case ID_OUTSIDES:
                    win_temp4 = wParam - ID_OUTSIDEN;
                    CheckRadioButton(hDlg, ID_OUTSIDEN, ID_OUTSIDES, wParam);
                    break;

                case ID_MATHF:
                    if (win_temp1 == 0)
                        win_temp1 = 1;
                    else
                        win_temp1 = 0;
                    CheckDlgButton(hDlg, ID_MATHF, win_temp1);
                    break;

                }

        }
    return (FALSE);
}


BOOL CALLBACK SelectExtended(hDlg, message, wParam, lParam)
HWND hDlg;
UINT message;
WPARAM wParam;
LPARAM lParam;
{
   char temp[80];
   int i;
#if 0
   extern        int        finattract;        /* finite attractor switch */
   extern        double        potparam[3];        /* three potential parameters*/
   extern        int        pot16bit;
   extern        int        usr_distest;        /* distance estimator option */
   extern        int        distestwidth;
   extern        int        rotate_lo,rotate_hi;
#endif
    switch (message) {

        case WM_INITDIALOG:
            if (finattract)
                win_temp1 = 1;
            else
                win_temp1 = 0;
            CheckDlgButton(hDlg, ID_FINITE, win_temp1);
            sprintf(temp,"%f",potparam[0]);
            SetDlgItemText(hDlg, ID_POTENTMAX, temp);
            sprintf(temp,"%f",potparam[1]);
            SetDlgItemText(hDlg, ID_POTENTSLOPE, temp);
            sprintf(temp,"%f",potparam[2]);
            SetDlgItemText(hDlg, ID_POTENTBAIL, temp);
            if (pot16bit)
                win_temp2 = 1;
            else
                win_temp2 = 0;
            CheckDlgButton(hDlg, ID_POTENT16, win_temp2);
            sprintf(temp,"%i",usr_distest);
            SetDlgItemText(hDlg, ID_DISTEST, temp);
            sprintf(temp,"%i",distestwidth);
            SetDlgItemText(hDlg, ID_DISTESTWID, temp);
            for (i = 0; i < 3; i++) {
                sprintf(temp,"%.12f",inversion[i]);
                if (inversion[i] == AUTOINVERT)
                    SetDlgItemText(hDlg, ID_INVERTRAD+i, "auto");
                else
                    SetDlgItemText(hDlg, ID_INVERTRAD+i, temp);
                }
            sprintf(temp,"%i",rotate_lo);
            SetDlgItemText(hDlg, ID_COLORMIN, temp);
            sprintf(temp,"%i",rotate_hi);
            SetDlgItemText(hDlg, ID_COLORMAX, temp);
            win_oldprompts[0] = win_temp1;
            win_oldprompts[1] = potparam[0];
            win_oldprompts[2] = potparam[1];
            win_oldprompts[3] = potparam[2];
            win_oldprompts[4] = win_temp2;
            win_oldprompts[5] = usr_distest;
            win_oldprompts[6] = distestwidth;
            win_oldprompts[7] = inversion[0];
            win_oldprompts[8] = inversion[1];
            win_oldprompts[9] = inversion[2];
            return (TRUE);

        case WM_COMMAND:
            switch (wParam) {

                case IDOK:
                    /* retrieve and validate the results */
                    finattract = win_temp1;
                    GetDlgItemText(hDlg, ID_POTENTMAX, temp, 10);
                    potparam[0] = atof(temp);
                    GetDlgItemText(hDlg, ID_POTENTSLOPE, temp, 10);
                    potparam[1] = atof(temp);
                    GetDlgItemText(hDlg, ID_POTENTBAIL, temp, 10);
                    potparam[2] = atof(temp);
                    pot16bit = win_temp2;
                    GetDlgItemText(hDlg, ID_DISTEST, temp, 10);
                    usr_distest = atoi(temp);
                    GetDlgItemText(hDlg, ID_DISTESTWID, temp, 10);
                    distestwidth = atoi(temp);
                    for (i = 0; i < 3; i++) {
                        GetDlgItemText(hDlg, ID_INVERTRAD+i, temp, 20);
                        if (temp[0] == 'a' || temp[0] == 'A')
                            inversion[i] = AUTOINVERT;
                        else
                            inversion[i] = atof(temp);
                        }
                    invert = (inversion[0] == 0.0) ? 0 : 3;
                    GetDlgItemText(hDlg, ID_COLORMIN, temp, 10);
                    rotate_lo = atoi(temp);
                    GetDlgItemText(hDlg, ID_COLORMAX, temp, 10);
                    rotate_hi = atoi(temp);
                    if (rotate_lo < 0 || rotate_hi > 255 || rotate_lo > rotate_hi) {
                        rotate_lo = 0;
                        rotate_hi = 255;
                        }
                    time_to_restart = 0;
                    if (
                        win_oldprompts[0] != win_temp1   ||
                        win_oldprompts[1] != potparam[0]  ||
                        win_oldprompts[2] != potparam[1]  ||
                        win_oldprompts[3] != potparam[2]  ||
                        win_oldprompts[4] != win_temp2    ||
                        win_oldprompts[5] != usr_distest  ||
                        win_oldprompts[6] != distestwidth ||
                        win_oldprompts[7] != inversion[0] ||
                        win_oldprompts[8] != inversion[1] ||
                        win_oldprompts[9] != inversion[2]
                        ) time_to_restart = 1;
                    EndDialog(hDlg, time_to_restart);
                    break;

                case IDCANCEL:
                    EndDialog(hDlg, 0);
                    break;

                case ID_FINITE:
                    if (win_temp1 == 0)
                        win_temp1 = 1;
                    else
                        win_temp1 = 0;
                    CheckDlgButton(hDlg, ID_FINITE, win_temp1);
                    break;

                case ID_POTENT16:
                    if (win_temp2 == 0)
                        win_temp2 = 1;
                    else
                        win_temp2 = 0;
                    CheckDlgButton(hDlg, ID_POTENT16, win_temp2);
                    break;

                }

        }
    return (FALSE);
}


BOOL CALLBACK SelectSavePar(hDlg, message, wParam, lParam)
HWND hDlg;
UINT message;
WPARAM wParam;
LPARAM lParam;
{
   char temp[80];
   extern int  colorstate;         /* comments in cmdfiles */
   extern char CommandFile[];
   extern char CommandName[];
   extern char colorfile[];
   extern int  colorstate;
   extern int  potflag;                        /* continuous potential flag */
   extern        double        potparam[3];        /* three potential parameters*/

    switch (message) {

        case WM_INITDIALOG:
            win_temp1 = 1;
            if (colorstate == 1)
                win_temp1 = 1;
            if (colorstate == 2)
                win_temp1 = 2;
            win_ltemp2 = colors - 1;
            if (maxiter < win_ltemp2) win_ltemp2 = maxiter;
            if (inside  > 0 && inside    > win_ltemp2) win_ltemp2 = inside;
            if (outside > 0 && outside   > win_ltemp2) win_ltemp2 = outside;
            if (usr_distest < 0 && 0-usr_distest > win_ltemp2) win_ltemp2 = 0-usr_distest;
            if (decomp[0] > win_ltemp2) win_ltemp2 = decomp[0] - 1;
            if (potflag && potparam[0] >= win_ltemp2) win_ltemp2 = (long)potparam[0];
            if (++win_ltemp2 > 256) win_ltemp2 = 256;
            SetDlgItemText(hDlg, ID_PFILE, CommandFile);
            SetDlgItemText(hDlg, ID_PENTRY, CommandName);
            if (CommandName[0] == 0)
                SetDlgItemText(hDlg, ID_PENTRY, "test");
            SetDlgItemText(hDlg, ID_PCOM1, CommandComment[0]);
            SetDlgItemText(hDlg, ID_PCOM2, CommandComment[1]);
            CheckDlgButton(hDlg, ID_PCOL1+win_temp1, 1);
            sprintf(temp,"%i",win_ltemp2);
            SetDlgItemText(hDlg, ID_PCNUM, temp);
            if (colorstate == 2)
                SetDlgItemText(hDlg, ID_PCFILE, colorfile);
            return (TRUE);

        case WM_COMMAND:
            switch (wParam) {

                case IDOK:
                    /* retrieve and validate the results */
                    GetDlgItemText(hDlg, ID_PFILE, CommandFile,     50);
                    GetDlgItemText(hDlg, ID_PENTRY, CommandName,    50);
                    GetDlgItemText(hDlg, ID_PCOM1, CommandComment[0], 57);
                    GetDlgItemText(hDlg, ID_PCOM2, CommandComment[1], 57);
                    GetDlgItemText(hDlg, ID_PCFILE, colorfile,      50);
                    EndDialog(hDlg, 1);
                    break;

                case IDCANCEL:
                    EndDialog(hDlg, 0);
                    break;

                case ID_PCOL1:
                case ID_PCOL2:
                case ID_PCOL3:
                    win_temp1 = wParam - ID_PCOL1;
                    CheckRadioButton(hDlg, ID_PCOL1, ID_PCOL3, wParam);
                }

        }
    return (FALSE);
}


int win_cycledir = -1, win_cyclerand = 0, win_cyclefreq = 0, win_cycledelay = 0;
int win_tempcycle, win_tempcycledir, win_tempcyclerand, win_tempcyclefreq;

BOOL CALLBACK SelectCycle(hDlg, message, wParam, lParam)
HWND hDlg;
UINT message;
WPARAM wParam;
LPARAM lParam;
{
    switch (message) {

        case WM_INITDIALOG:
            win_tempcycle = time_to_cycle;
            win_tempcycledir = win_cycledir;
            win_tempcyclerand = win_cyclerand;
            win_tempcyclefreq = win_cyclefreq;
            if (win_tempcycle == 0)
                CheckDlgButton(hDlg, ID_CYCLEOFF, 1);
            else
                CheckDlgButton(hDlg, ID_CYCLEON, 1);
            if (win_tempcycledir == -1)
                CheckDlgButton(hDlg, ID_CYCLEOUT, 1);
            else
                CheckDlgButton(hDlg, ID_CYCLEIN, 1);
            if (win_tempcyclerand == 0)
                CheckDlgButton(hDlg, ID_CYCLESTAT, 1);
            else
                CheckDlgButton(hDlg, ID_CYCLECHG, 1);
            if (win_tempcyclefreq == 0)
                CheckDlgButton(hDlg, ID_CYCLELOW, 1);
            else if (win_tempcyclefreq == 1)
                CheckDlgButton(hDlg, ID_CYCLEMED, 1);
            else
                CheckDlgButton(hDlg, ID_CYCLEHIGH, 1);
            return (TRUE);

        case WM_COMMAND:
            switch (wParam) {

                case IDOK:
                    /* retrieve and validate the results */
                    time_to_cycle = win_tempcycle;
                    win_cycledir = win_tempcycledir;
                    win_cyclerand = win_tempcyclerand;
                    win_cyclefreq = win_tempcyclefreq;
                    EndDialog(hDlg, 1);
                    break;

                case IDCANCEL:
                    EndDialog(hDlg, 0);
                    break;

                case ID_CYCLEOFF:
                    win_tempcycle = 0;
                    CheckRadioButton(hDlg, ID_CYCLEOFF, ID_CYCLEON, ID_CYCLEOFF);
                    break;

                case ID_CYCLEON:
                    win_tempcycle = 1;
                    CheckRadioButton(hDlg, ID_CYCLEOFF, ID_CYCLEON, ID_CYCLEON);
                    break;

                case ID_CYCLEOUT:
                    win_tempcycledir = -1;
                    CheckRadioButton(hDlg, ID_CYCLEOUT, ID_CYCLEIN, ID_CYCLEOUT);
                    break;

                case ID_CYCLEIN:
                    win_tempcycledir = 1;
                    CheckRadioButton(hDlg, ID_CYCLEOUT, ID_CYCLEIN, ID_CYCLEIN);
                    break;

                case ID_CYCLESTAT:
                    win_tempcyclerand = 0;
                    CheckRadioButton(hDlg, ID_CYCLESTAT, ID_CYCLECHG, ID_CYCLESTAT);
                    break;

                case ID_CYCLECHG:
                    win_tempcyclerand = 1;
                    CheckRadioButton(hDlg, ID_CYCLESTAT, ID_CYCLECHG, ID_CYCLECHG);
                    break;

                case ID_CYCLELOW:
                    win_tempcyclefreq = 0;
                    CheckRadioButton(hDlg, ID_CYCLELOW, ID_CYCLEHIGH, ID_CYCLELOW);
                    break;

                case ID_CYCLEMED:
                    win_tempcyclefreq = 1;
                    CheckRadioButton(hDlg, ID_CYCLELOW, ID_CYCLEHIGH, ID_CYCLEMED);
                    break;

                case ID_CYCLEHIGH:
                    win_tempcyclefreq = 2;
                    CheckRadioButton(hDlg, ID_CYCLELOW, ID_CYCLEHIGH, ID_CYCLEHIGH);
                    break;

                }

        }
    return (FALSE);
}

FARPROC lpSelectFullScreen;

extern HANDLE hInst;

int win_fullscreen_count;
char * far win_fullscreen_prompts[20];
char *win_fullscreen_heading;
static struct fullscreenvalues win_fullscreen_values[20];

int xxx_fullscreen_prompt(        /* full-screen prompting routine */
        char *hdg,                /* heading, lines separated by \n */
        int numprompts,         /* there are this many prompts (max) */
        char * far *prompts,        /* array of prompting pointers */
        struct fullscreenvalues values[], /* array of values */
        int options,                /* future use bits in case we need them */
        int fkeymask                /* bit n on if Fn to cause return */
        )
{
int i;
int Return;

win_fullscreen_count = numprompts;
win_fullscreen_heading = hdg;
win_fullscreen_count = numprompts;
for (i = 0; i < win_fullscreen_count; i++) {
   win_fullscreen_prompts[i] = prompts[i];
   win_fullscreen_values[i]  = values[i];
   }

lpSelectFullScreen = MakeProcInstance((FARPROC)SelectFullScreen, hInst);
Return = DialogBox(hInst, "SelectFullScreen", hwnd, (DLGPROC)lpSelectFullScreen);
FreeProcInstance(lpSelectFullScreen);

if (Return) {
    for (i = 0; i < win_fullscreen_count; i++) {
        values[i] = win_fullscreen_values[i];
    }
    return(0);
    }

return(-1);
}

BOOL CALLBACK SelectFullScreen(hDlg, message, wParam, lParam)
HWND hDlg;
UINT message;
WPARAM wParam;
LPARAM lParam;
{

    int i;
    char temp[80];

    switch (message) {

        case WM_INITDIALOG:
            SetDlgItemText(hDlg, ID_PROMPT00,win_fullscreen_heading);
            for (i = 0; i < win_fullscreen_count; i++) {
                SetDlgItemText(hDlg, ID_PROMPT01+i,win_fullscreen_prompts[i]);
                if (win_fullscreen_values[i].type == 'd' ||
                    win_fullscreen_values[i].type == 'f')
                    sprintf(temp,"%10.5f",win_fullscreen_values[i].uval.dval);
                else if(win_fullscreen_values[i].type == 'i')
                    sprintf(temp,"%d",win_fullscreen_values[i].uval.ival);
                else if(win_fullscreen_values[i].type == 's')
                {
                    strncpy(temp,win_fullscreen_values[i].uval.sval,16);
                    temp[15] = 0;
                }
                else if(win_fullscreen_values[i].type == 'l')
                    strcpy(temp,win_fullscreen_values[i].uval.ch.list[win_fullscreen_values[i].uval.ch.val]);
                else
                    strcpy(temp,win_fullscreen_values[i].uval.sval);
                SetDlgItemText(hDlg, ID_ANSWER01+i,temp);
                }
            return (TRUE);

        case WM_COMMAND:
            switch (wParam) {

                case IDOK:
                    for (i = 0; i < win_fullscreen_count; i++) {
                        GetDlgItemText(hDlg, ID_ANSWER01+i , temp, 20);
                        if (win_fullscreen_values[i].type == 'd' ||
                            win_fullscreen_values[i].type == 'f')
                            win_fullscreen_values[i].uval.dval = atof(temp);
                        else if(win_fullscreen_values[i].type == 'i')
                            win_fullscreen_values[i].uval.ival = atoi(temp);
                        else if(win_fullscreen_values[i].type == 's')
                            strncpy(win_fullscreen_values[i].uval.sval,temp,16);
                        else if(win_fullscreen_values[i].type == 'l')
                            strcpy(win_fullscreen_values[i].uval.ch.list[win_fullscreen_values[i].uval.ch.val],temp);
                        else
                            strcpy(win_fullscreen_values[i].uval.sval,temp);
                    }
                    EndDialog(hDlg, 1);
                    break;

                case IDCANCEL:
                    EndDialog(hDlg, 0);
                    break;

                }

        }
    return (FALSE);
}


extern int init3d[];
extern int win_3dspherical;
extern char preview, showbox;
extern int previewfactor, glassestype, whichimage;
extern int xtrans, ytrans, transparent[2], RANDOMIZE;
extern int red_crop_left, red_crop_right;
extern int blue_crop_left, blue_crop_right;
extern int red_bright, blue_bright;
extern        int RAY;
extern        int BRIEF;
extern        int Ambient;
extern        char ray_name[];
extern int Targa_Overlay;
extern int Targa_Out;
extern        int        overlay3d;            /* 3D overlay flag: 0 = OFF */
extern int xadjust;
extern int eyeseparation;

static int far win_answers[20];

BOOL CALLBACK Select3D(hDlg, message, wParam, lParam)
HWND hDlg;
UINT message;
WPARAM wParam;
LPARAM lParam;
{

    int i;
    char temp[80];

    switch (message) {

        case WM_INITDIALOG:
            win_answers[0] = preview;
            win_answers[1] = showbox;
            win_answers[2] = SPHERE;
            win_answers[3] = previewfactor;
            win_answers[4] = glassestype;
            win_answers[5] = FILLTYPE+1;
            win_answers[6] = RAY;
            win_answers[7] = BRIEF;
            win_answers[8] = Targa_Out;
            CheckDlgButton(hDlg, ID_PREVIEW, win_answers[0]);
            CheckDlgButton(hDlg, ID_SHOWBOX, win_answers[1]);
            CheckDlgButton(hDlg, ID_SPHERICAL, win_answers[2]);
            CheckDlgButton(hDlg, ID_RAYB, win_answers[7]);
/*
            CheckDlgButton(hDlg, ID_TARGA, win_answers[8]);
*/
            sprintf(temp,"%d",win_answers[3]);
            SetDlgItemText(hDlg, ID_PREVIEWFACTOR, temp);
            CheckRadioButton(hDlg, ID_STEREO1, ID_STEREO4,
                ID_STEREO1+win_answers[4]);
            CheckRadioButton(hDlg, ID_FILL1, ID_FILL8,
                ID_FILL1+win_answers[5]);
            CheckRadioButton(hDlg, ID_RAY0, ID_RAY6,
                ID_RAY0+win_answers[6]);
            check_writefile(ray_name,".ray");
            SetDlgItemText(hDlg, ID_RAYN, ray_name);
            return (TRUE);

        case WM_COMMAND:
            switch (wParam) {

                case IDOK:
                    if(win_answers[2] != SPHERE) {
                        SPHERE = win_answers[2];
                        set_3d_defaults();
                        }
                    preview = win_answers[0];
                    showbox = win_answers[1];
                    SPHERE  = win_answers[2];
                    RAY     = win_answers[6];
                    BRIEF   = win_answers[7];
                    Targa_Out = win_answers[8];
                    GetDlgItemText(hDlg, ID_PREVIEWFACTOR, temp, 10);
                    GetDlgItemText(hDlg, ID_RAYN, temp, 30);
                    strcpy(ray_name, temp);
                    previewfactor = atoi(temp);
                    glassestype = win_answers[4];
                    FILLTYPE = win_answers[5]-1;
                    win_3dspherical = SPHERE;
                    if(previewfactor < 8)
                       previewfactor = 8;
                    if(previewfactor > 128)
                       previewfactor = 128;
                    if(glassestype < 0)
                       glassestype = 0;
                    if(glassestype > 3)
                       glassestype = 3;
                    whichimage = 0;
                    if(glassestype)
                       whichimage = 1;
                    if (Targa_Out && overlay3d)
                       Targa_Overlay = 1;
                    EndDialog(hDlg, 1);
                    break;

                case ID_PREVIEW:
                case ID_SHOWBOX:
                case ID_SPHERICAL:
                    i = wParam - ID_PREVIEW;
                    win_answers[i] = 1 - win_answers[i];
                    CheckDlgButton(hDlg, ID_PREVIEW + i, win_answers[i]);
                    break;

                case ID_FILL1:
                case ID_FILL2:
                case ID_FILL3:
                case ID_FILL4:
                case ID_FILL5:
                case ID_FILL6:
                case ID_FILL7:
                case ID_FILL8:
                    i = wParam - ID_FILL1;
                    win_answers[5] = i;
                    CheckRadioButton(hDlg, ID_FILL1, ID_FILL8,
                        ID_FILL1+win_answers[5]);
                    break;

                case ID_STEREO1:
                case ID_STEREO2:
                case ID_STEREO3:
                case ID_STEREO4:
                    i = wParam - ID_STEREO1;
                    win_answers[4] = i;
                    CheckRadioButton(hDlg, ID_STEREO1, ID_STEREO4,
                        ID_STEREO1+win_answers[4]);
                    break;

                case ID_RAY0:
                case ID_RAY1:
                case ID_RAY2:
                case ID_RAY3:
                case ID_RAY4:
                case ID_RAY5:
                case ID_RAY6:
                    i = wParam - ID_RAY0;
                    win_answers[6] = i;
                    CheckRadioButton(hDlg, ID_RAY0, ID_RAY6,
                        ID_RAY0+win_answers[6]);
                    break;

                case ID_RAYB:
                    win_answers[7] = 1 - win_answers[7];
                    CheckDlgButton(hDlg, ID_RAYB, win_answers[7]);
                    break;

                case ID_TARGA:
                    win_answers[8] = 1 - win_answers[8];
                    CheckDlgButton(hDlg, ID_TARGA, win_answers[8]);
                    break;

                case IDCANCEL:
                    EndDialog(hDlg, 0);
                    break;

                }

        }
    return (FALSE);
}

BOOL CALLBACK Select3DPlanar(hDlg, message, wParam, lParam)
HWND hDlg;
UINT message;
WPARAM wParam;
LPARAM lParam;
{

    int i;
    char temp[80];

    switch (message) {

        case WM_INITDIALOG:
            win_answers[0] = XROT;
            win_answers[1] = YROT;
            win_answers[2] = ZROT;
            win_answers[3] = XSCALE;
            win_answers[4] = YSCALE;
            win_answers[5] = ROUGH;
            win_answers[6] = WATERLINE;
            win_answers[7] = ZVIEWER;
            win_answers[8] = XSHIFT;
            win_answers[9] = YSHIFT;
            win_answers[10] = xtrans;
            win_answers[11] = ytrans;
            win_answers[12] = transparent[0];
            win_answers[13] = transparent[1];
            win_answers[14] = RANDOMIZE;
            for (i = 0; i < 15; i++) {
                sprintf(temp,"%d", win_answers[i]);
                SetDlgItemText(hDlg, ID_ANS1+i,temp);
                }
            return (TRUE);

        case WM_COMMAND:
            switch (wParam) {

                case IDOK:
                    for (i = 0; i < 15; i++) {
                        GetDlgItemText(hDlg, ID_ANS1+i, temp, 20);
                        win_answers[i] = (int)atof(temp);
                        }
                    XROT =           win_answers[0];
                    YROT =           win_answers[1];
                    ZROT =           win_answers[2];
                    XSCALE =         win_answers[3];
                    YSCALE =         win_answers[4];
                    ROUGH =          win_answers[5];
                    WATERLINE =      win_answers[6];
                    ZVIEWER =        win_answers[7];
                    XSHIFT =         win_answers[8];
                    YSHIFT =         win_answers[9];
                    xtrans =         win_answers[10];
                    ytrans =         win_answers[11];
                    transparent[0] = win_answers[12];
                    transparent[1] = win_answers[13];
                    RANDOMIZE =      win_answers[14];
                    if (RANDOMIZE >= 7) RANDOMIZE = 7;
                    if (RANDOMIZE <= 0) RANDOMIZE = 0;
                    EndDialog(hDlg, 1);
                    break;

                case IDCANCEL:
                    EndDialog(hDlg, 0);
                    break;

                }

        }
    return (FALSE);
}


BOOL CALLBACK SelectIFS3D(hDlg, message, wParam, lParam)
HWND hDlg;
UINT message;
WPARAM wParam;
LPARAM lParam;
{

    int i, numanswers;
    char temp[80];

    numanswers = 5;

    switch (message) {

        case WM_INITDIALOG:
            win_answers[0] = XROT;
            win_answers[1] = YROT;
            win_answers[2] = ZROT;
            win_answers[3] = ZVIEWER;
            win_answers[4] = XSHIFT;
            win_answers[5] = YSHIFT;
            win_answers[6] = glassestype;
            for (i = 0; i <= numanswers; i++) {
                sprintf(temp,"%d", win_answers[i]);
                SetDlgItemText(hDlg, ID_ANS1+i,temp);
                }
            CheckRadioButton(hDlg, ID_STEREO1, ID_STEREO4,
                ID_STEREO1+win_answers[6]);
            return (TRUE);

        case WM_COMMAND:
            switch (wParam) {

                case ID_STEREO1:
                case ID_STEREO2:
                case ID_STEREO3:
                case ID_STEREO4:
                    i = wParam - ID_STEREO1;
                    win_answers[6] = i;
                    CheckRadioButton(hDlg, ID_STEREO1, ID_STEREO4,
                        ID_STEREO1+win_answers[6]);
                    break;

                case IDOK:
                    for (i = 0; i <= numanswers; i++) {
                        GetDlgItemText(hDlg, ID_ANS1+i, temp, 20);
                        win_answers[i] = (int)atof(temp);
                        }
                    XROT =           win_answers[0];
                    YROT =           win_answers[1];
                    ZROT =           win_answers[2];
                    ZVIEWER =        win_answers[3];
                    XSHIFT =         win_answers[4];
                    YSHIFT =         win_answers[5];
                    glassestype =    win_answers[6];
                    EndDialog(hDlg, 1);
                    break;

                case IDCANCEL:
                    EndDialog(hDlg, 0);
                    break;

                }

        }
    return (FALSE);
}

char win_funnyglasses_map_name[41];

BOOL CALLBACK SelectFunnyGlasses(hDlg, message, wParam, lParam)
HWND hDlg;
UINT message;
WPARAM wParam;
LPARAM lParam;
{

    int i, numanswers;
    char temp[80];

    numanswers = 7;

    switch (message) {

        case WM_INITDIALOG:

            /* defaults */
            if(ZVIEWER == 0)
               ZVIEWER = 150;
            if(eyeseparation == 0) {
               if(fractype==IFS3D || fractype==LLORENZ3D || fractype==FPLORENZ3D) {
                       eyeseparation =  2;
                  xadjust       = -2;
                  }
               else {
                  eyeseparation =  3;
                  xadjust       =  0;
                  }
               }

            win_funnyglasses_map_name[0] = 0;
            if(glassestype == 1)
                strcpy(win_funnyglasses_map_name,"glasses1.map");
            else if(glassestype == 2) {
                if(FILLTYPE == -1)
                         strcpy(win_funnyglasses_map_name,"grid.map");
                else
                    strcpy(win_funnyglasses_map_name,"glasses2.map");
                }

            win_answers[0] = eyeseparation;
            win_answers[1] = xadjust;
            win_answers[2] = red_crop_left;
            win_answers[3] = red_crop_right;
            win_answers[4] = blue_crop_left;
            win_answers[5] = blue_crop_right;
            win_answers[6] = red_bright;
            win_answers[7] = blue_bright;
            for (i = 0; i < numanswers+1;i++) {
                sprintf(temp,"%d", win_answers[i]);
                SetDlgItemText(hDlg, ID_ANS1+i,temp);
                }
            SetDlgItemText(hDlg, ID_ANS9,win_funnyglasses_map_name);
            return (TRUE);

        case WM_COMMAND:
            switch (wParam) {

                case IDOK:
                    for (i = 0; i < numanswers+1; i++) {
                        GetDlgItemText(hDlg, ID_ANS1+i, temp, 20);
                        win_answers[i] = (int)atof(temp);
                        }
                    GetDlgItemText(hDlg, ID_ANS9, temp, 40);
                    strcpy(win_funnyglasses_map_name, temp);
                    eyeseparation   =  win_answers[0];
                    xadjust         =  win_answers[1];
                    red_crop_left   =  win_answers[2];
                    red_crop_right  =  win_answers[3];
                    blue_crop_left  =  win_answers[4];
                    blue_crop_right =  win_answers[5];
                    red_bright      =  win_answers[6];
                    blue_bright     =  win_answers[7];
                    EndDialog(hDlg, 1);
                    break;

                case IDCANCEL:
                    EndDialog(hDlg, 0);
                    break;

                }

        }
    return (FALSE);
}

BOOL CALLBACK SelectLightSource(hDlg, message, wParam, lParam)
HWND hDlg;
UINT message;
WPARAM wParam;
LPARAM lParam;
{

    int i, numanswers;
    char temp[80];

    numanswers = 5;

    switch (message) {

        case WM_INITDIALOG:
            win_answers[0] = XLIGHT;
            win_answers[1] = YLIGHT;
            win_answers[2] = ZLIGHT;
            win_answers[3] = LIGHTAVG;
            win_answers[4] = Ambient;
            for (i = 0; i < numanswers+1;i++) {
                sprintf(temp,"%d", win_answers[i]);
                SetDlgItemText(hDlg, ID_ANS1+i,temp);
                }
            return (TRUE);

        case WM_COMMAND:
            switch (wParam) {

                case IDOK:
                    for (i = 0; i < numanswers+1; i++) {
                        GetDlgItemText(hDlg, ID_ANS1+i, temp, 20);
                        win_answers[i] = (int)atof(temp);
                        }
                    XLIGHT   =  win_answers[0];
                    YLIGHT   =  win_answers[1];
                    ZLIGHT   =  win_answers[2];
                    LIGHTAVG =  win_answers[3];
                    Ambient  =  win_answers[4];
                    EndDialog(hDlg, 1);
                    break;

                case IDCANCEL:
                    EndDialog(hDlg, 0);
                    break;

                }

        }
    return (FALSE);
}

BOOL CALLBACK Select3DSpherical(hDlg, message, wParam, lParam)
HWND hDlg;
UINT message;
WPARAM wParam;
LPARAM lParam;
{

    int i;
    char temp[80];

    switch (message) {

        case WM_INITDIALOG:
            win_answers[0] = XROT;
            win_answers[1] = YROT;
            win_answers[2] = ZROT;
            win_answers[3] = XSCALE;
            win_answers[4] = YSCALE;
            win_answers[5] = ROUGH;
            win_answers[6] = WATERLINE;
            win_answers[7] = ZVIEWER;
            win_answers[8] = XSHIFT;
            win_answers[9] = YSHIFT;
            win_answers[10] = xtrans;
            win_answers[11] = ytrans;
            win_answers[12] = transparent[0];
            win_answers[13] = transparent[1];
            win_answers[14] = RANDOMIZE;
            for (i = 0; i < 15; i++) {
                sprintf(temp,"%d", win_answers[i]);
                SetDlgItemText(hDlg, ID_ANS1+i,temp);
                }
            return (TRUE);

        case WM_COMMAND:
            switch (wParam) {

                case IDOK:
                    for (i = 0; i < 15; i++) {
                        GetDlgItemText(hDlg, ID_ANS1+i, temp, 10);
                        win_answers[i] = (int)atof(temp);
                        }
                    XROT =           win_answers[0];
                    YROT =           win_answers[1];
                    ZROT =           win_answers[2];
                    XSCALE =         win_answers[3];
                    YSCALE =         win_answers[4];
                    ROUGH =          win_answers[5];
                    WATERLINE =      win_answers[6];
                    ZVIEWER =        win_answers[7];
                    XSHIFT =         win_answers[8];
                    YSHIFT =         win_answers[9];
                    xtrans =         win_answers[10];
                    ytrans =         win_answers[11];
                    transparent[0] = win_answers[12];
                    transparent[1] = win_answers[13];
                    RANDOMIZE =      win_answers[14];
                    if (RANDOMIZE >= 7) RANDOMIZE = 7;
                    if (RANDOMIZE <= 0) RANDOMIZE = 0;
                    EndDialog(hDlg, 1);
                    break;

                case IDCANCEL:
                    EndDialog(hDlg, 0);
                    break;

                }

        }
    return (FALSE);
}

BOOL CALLBACK SelectStarfield(hDlg, message, wParam, lParam)
HWND hDlg;
UINT message;
WPARAM wParam;
LPARAM lParam;
{
extern double starfield_values[4];

   switch (message) {

        case WM_INITDIALOG:
            SetDlgItemInt(hDlg, ID_NUMSTARS,   (int)starfield_values[0], FALSE);
            SetDlgItemInt(hDlg, ID_CLUMPINESS, (int)starfield_values[1], FALSE);
            SetDlgItemInt(hDlg, ID_DIMRATIO,   (int)starfield_values[2], FALSE);
            return (TRUE);

        case WM_COMMAND:
            switch (wParam) {

                case IDOK:
                    starfield_values[0] = GetDlgItemInt(hDlg, ID_NUMSTARS,   NULL, FALSE);
                    starfield_values[1] = GetDlgItemInt(hDlg, ID_CLUMPINESS, NULL, FALSE);
                    starfield_values[2] = GetDlgItemInt(hDlg, ID_DIMRATIO,   NULL, FALSE);
                    EndDialog(hDlg, 1);
                    break;

                case IDCANCEL:
                    EndDialog(hDlg, 0);
                    break;

                }

        }
    return (FALSE);
}

extern int time_to_save;
extern int gif87a_flag;
int FileFormat = 0;
FARPROC lpStatusBox = NULL;
HWND hStatusBox;
BOOL OperCancelled;
char far StatusTitle[80];

extern LPBITMAPINFO pDibInfo;
extern char huge *pixels;
extern LPLOGPALETTE pLogPal;

BOOL CALLBACK StatusBoxProc(HWND hDlg, UINT Msg, WPARAM wParam,
                LPARAM lParam)
{
   char PerStr[10];
   RECT Rect, TextRect;
   HWND hBar, hCancel;
   HDC hBarDC;
   unsigned Percent;

   switch(Msg)
   {
      case WM_INITDIALOG:
         SetWindowText(hDlg, StatusTitle);
         hCancel = GetDlgItem(hDlg, ID_CANCEL);
         SetFocus(hCancel);
         return(FALSE);

      case WM_CHAR:
         if(wParam == VK_RETURN || wParam == VK_ESCAPE)
            OperCancelled = TRUE;
         break;

      case WM_COMMAND:

         if(wParam == ID_CANCEL)
            OperCancelled = TRUE;
         break;

      case WM_USER:
         /* Invalidate Bar Window */
         hBar = GetDlgItem(hDlg, ID_PERCENT);
         GetClientRect(hBar, &TextRect);

         /* Calculate Percentage */
         hBarDC = GetDC(hBar);
         Percent = (unsigned)((lParam * 100) / wParam);
         if(Percent <= 100)
         {
            wsprintf(PerStr, "%d", Percent);
            strcat(PerStr, "%");

            /* Display Bar */
            Rect = TextRect;
            Rect.right = (unsigned)((((long)Percent) * Rect.right) / 100);
            Rect.top += 2;
            Rect.bottom -= 2;
            Rectangle(hBarDC, Rect.left, Rect.top, Rect.right, Rect.bottom);

            /* Display Percentage */
            DrawText(hBarDC, PerStr, lstrlen(PerStr), &TextRect,
                     DT_CENTER | DT_VCENTER | DT_SINGLELINE);
         }

         ReleaseDC(hBar, hBarDC);
         break;

      case WM_DESTROY:
         break;

      default:
         return(FALSE);
   }
   return(TRUE);
}

void OpenStatusBox(HWND hWnd, HANDLE hInst)
{
   if(lpStatusBox == NULL)
      lpStatusBox = MakeProcInstance((FARPROC)StatusBoxProc, hInst);
   hStatusBox = CreateDialog(hInst, "StatusBox", hWnd, (DLGPROC)lpStatusBox);
}

void CloseStatusBox(void)
{
   DestroyWindow(hStatusBox);
}

void UpdateStatusBox(unsigned long Partial, unsigned long Total)
{
   if(Total > 0xffff)
   {
      Total >>= 16;
      Partial >>= 16;
   }
   SendMessage(hStatusBox, WM_USER, (unsigned)Total, Partial);
}

void center_window(HWND hchild, int xadj, int yadj)
{
    RECT crect,prect;
    int i,cwidth,cheight;
    POINT center;
    GetWindowRect(hchild, &crect);
    GetClientRect(hwnd, &prect);   /* main Fractint window */
    cwidth  = crect.right - crect.left;
    cheight = crect.bottom - crect.top;
    center.x = (prect.right + prect.left) / 2;
    center.y = (prect.bottom + prect.top) / 2;
    ClientToScreen(hwnd, &center);
    if ((center.x += xadj - (cwidth  / 2)) < 0) center.x = 0;
    if ((center.y += yadj - (cheight / 2)) < 0) center.y = 0;
    if ((i = GetSystemMetrics(SM_CXSCREEN) - cwidth ) < center.x) center.x = i;
    if ((i = GetSystemMetrics(SM_CYSCREEN) - cheight) < center.y) center.y = i;
    MoveWindow(hchild, center.x, center.y, cwidth, cheight, FALSE);
}

void SaveBitmapFile(HWND hWnd, char *FullPathName)
{
   long TotalSize, Saved = 0, ImageSize, n;
   BITMAPFILEHEADER FileHeader;
   unsigned PalSize, BitCount;
   unsigned BlockSize = 10240;
   int hFile;
   char Temp[FILE_MAX_DIR];
   OFSTRUCT OfStruct;
   HANDLE hPal;
   RGBQUAD FAR *Pal;

   hFile = OpenFile(FullPathName, &OfStruct, OF_CREATE);
   if(hFile == 0)
   {
FileError:
      wsprintf(Temp, "File I/O error while saving %s.", (LPSTR)FullPathName);
      MessageBox(hWnd, Temp, "File Error . . .", MB_OK | MB_ICONEXCLAMATION);

GeneralError:
      _lclose(hFile);
      OpenFile(FullPathName, &OfStruct, OF_DELETE);
      CloseStatusBox();
      return;
   }

   BitCount = pDibInfo->bmiHeader.biBitCount;
   if(BitCount != 24)
      PalSize = (1 << BitCount) * sizeof(RGBQUAD);
   else
      PalSize = 0;

   ImageSize = pDibInfo->bmiHeader.biSizeImage;
   FileHeader.bfType = 0x4d42; /* 'BM'; */
   FileHeader.bfSize = sizeof(FileHeader) +
                       pDibInfo->bmiHeader.biSize +
                       PalSize + ImageSize;
   TotalSize = FileHeader.bfSize;
   FileHeader.bfReserved1 = FileHeader.bfReserved2 = 0;
   FileHeader.bfOffBits = FileHeader.bfSize - ImageSize;
   Saved += _lwrite(hFile, (LPSTR)&FileHeader, sizeof(FileHeader));
   Saved += _lwrite(hFile, (LPSTR)pDibInfo, (int)pDibInfo->bmiHeader.biSize);
   if(PalSize)
   {
      hPal = GlobalAlloc(GMEM_FIXED, PalSize);
      Pal = (RGBQUAD FAR *)GlobalLock(hPal);
      if(Pal == NULL)
      {
         MessageBox(hWnd, "Insufficient Memory", "Memory Error . . .",
                          MB_ICONEXCLAMATION | MB_OK);
         goto GeneralError;
      }
      for(n = 0; n < (1 << BitCount); n++)
      {
         Pal[n].rgbRed   = pLogPal->palPalEntry[n].peRed;
         Pal[n].rgbGreen = pLogPal->palPalEntry[n].peGreen;
         Pal[n].rgbBlue  = pLogPal->palPalEntry[n].peBlue;
         Pal[n].rgbReserved = 0;
      }
      Saved += _lwrite(hFile, (LPSTR)Pal, PalSize);
      GlobalUnlock(hPal);
      GlobalFree(hPal);
   }
   UpdateStatusBox(Saved, TotalSize);
   keypressed();

   /* We should have saved enough bytes to reach the image offset.  If not,
      then there was an error. */
   if(Saved != (long)FileHeader.bfOffBits)
      goto FileError;

   for(n = 0; n < (ImageSize - BlockSize); n += BlockSize)
   {
      if(_lwrite(hFile, (LPSTR)&pixels[n], BlockSize) != BlockSize)
         goto FileError;
      Saved += BlockSize;
      UpdateStatusBox(Saved, TotalSize);
      keypressed();
      if(OperCancelled)
      {
         MessageBox(hWnd, "File save cancelled.", "Save Cancelled", MB_OK);
         goto GeneralError;
      }
   }
   Saved += _lwrite(hFile, (LPSTR)&pixels[n], (int)(ImageSize - n));
   if(Saved != TotalSize)
      goto FileError;

   UpdateStatusBox(Saved, TotalSize);
   _lclose(hFile);
   CloseStatusBox();
}

/* common dialog boxes */

static char far *win_common_type[] = {
    "BMP Files (*.BMP)|*.bmp|",
    "GIF Files (*.GIF)|*.gif|",
    "Palette Files (*.MAP)|*.map|",
    "Parameter Files (*.PAR)|*.par|",
    "IFS Files (*.IFS)|*.ifs|",
    "Formula Files (*.FRM)|*.frm|",
    "L-system Files (*.L)|*.l|",
    "Any File (*.*)|*.*|",
    };

static char *win_common_type2[] = {
    ".bmp",
    ".gif",
    ".map",
    ".par",
    ".ifs",
    ".frm",
    ".l",
    ""
    };

int Win_OpenFile(unsigned char FileName[])
{
OPENFILENAME ofn;
char szFilter[FILE_MAX_DIR], szDirName[FILE_MAX_DIR], szFile[FILE_MAX_DIR];
char szFileTitle[FILE_MAX_DIR];
char chReplace;
int i;
char currdir[FILE_MAX_DIR], tempdir[FILE_MAX_DIR], tempext[FILE_MAX_DIR];

szFilter[0] = '\0';
szDirName[0] = '\0';
szFile[0] = '\0';
szFileTitle[0] = '\0';

   _getcwd(currdir, FILE_MAX_DIR); /* save current working directory so we */
   lstrcpy(szDirName,currdir);     /* can restore it when we're done here */
   if (FileName[0] != 0) {
      lstrcpy(szFile,FileName);
      splitpath(FileName,"",tempdir,szFile,tempext);
      if (tempext[0] != 0)
         lstrcat(szFile,tempext);
      else
         lstrcat(szFile,DefExt);
   }
   if (tempdir[0] != 0) {
      lstrcat(szDirName,SLASH);
      lstrcat(szDirName,tempdir);
   }
/* GetSystemDirectory(szDirName, sizeof(szDirName)); */

for (i = 0; i < 7; i++)
    if (strcmp(DefExt,win_common_type2[i]) == 0)
        break;
lstrcpy(szFilter, win_common_type[i]);
lstrcat(szFilter, win_common_type[7]);
chReplace = szFilter[strlen(szFilter) - 1];
for (i = 0; szFilter[i] != '\0'; i++) {
    if (szFilter[i] == chReplace)
        szFilter[i] = '\0';
        }

memset(&ofn, 0, sizeof(OPENFILENAME));
ofn.lStructSize = sizeof(OPENFILENAME);
ofn.hwndOwner = hwnd;
ofn.lpstrFilter = szFilter;
ofn.nFilterIndex = 1;
ofn.lpstrFile = szFile;
ofn.nMaxFile = sizeof(szFile);
ofn.lpstrFileTitle = szFileTitle;
ofn.nMaxFileTitle = sizeof(szFileTitle);
ofn.lpstrInitialDir = szDirName;
ofn.lpstrTitle = DialogTitle;
ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;

i = GetOpenFileName(&ofn);
lstrcpy(FileName,szFile);
_chdir(currdir);

return i;

}

Win_SaveFile(unsigned char FileName[])
{
OPENFILENAME ofn;
char szFilter[FILE_MAX_DIR], szDirName[FILE_MAX_DIR], szFile[FILE_MAX_DIR];
char szFileTitle[FILE_MAX_DIR];
char chReplace;
int i;
char currdir[FILE_MAX_DIR], tempdir[FILE_MAX_DIR], tempext[FILE_MAX_DIR];

szFilter[0] = '\0';
szDirName[0] = '\0';
szFile[0] = '\0';
szFileTitle[0] = '\0';

   _getcwd(currdir, FILE_MAX_DIR); /* save current working directory so we */
   lstrcpy(szDirName,currdir);     /* can restore it when we're done here */
   if (FileName[0] != 0) {
      lstrcpy(szFile,FileName);
      splitpath(FileName,"",tempdir,szFile,tempext);
      if (tempext[0] != 0)
         lstrcat(szFile,tempext);
      else
         lstrcat(szFile,DefExt);
   }
   if (tempdir[0] != 0) {
      lstrcat(szDirName,SLASH);
      lstrcat(szDirName,tempdir);
   }

/* lstrcpy(szFile,FileName); */
/* GetSystemDirectory(szDirName, sizeof(szDirName)); */

for (i = 0; i < 7; i++)
    if (strcmp(DefExt,win_common_type2[i]) == 0)
        break;
lstrcpy(szFilter, win_common_type[i]);
if (i == 1)
    lstrcat(szFilter,win_common_type[0]);
lstrcat(szFilter,win_common_type[7]);

chReplace = szFilter[strlen(szFilter) - 1];
for (i = 0; szFilter[i] != '\0'; i++) {
    if (szFilter[i] == chReplace)
        szFilter[i] = '\0';
        }

memset(&ofn, 0, sizeof(OPENFILENAME));
ofn.lStructSize = sizeof(OPENFILENAME);
ofn.hwndOwner = hwnd;
ofn.lpstrFilter = szFilter;
ofn.nFilterIndex = 1;
ofn.lpstrFile = szFile;
ofn.nMaxFile = sizeof(szFile);
ofn.lpstrFileTitle = szFileTitle;
ofn.nMaxFileTitle = sizeof(szFileTitle);
ofn.lpstrInitialDir = szDirName;
ofn.lpstrTitle = DialogTitle;
//ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY;
ofn.Flags = OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;

i = GetSaveFileName(&ofn);
lstrcpy(FileName,szFile);
lstrcpy(FullPathName,szFile);
lstrcpy(readname,szFile);

FileFormat = ID_GIF89A;
if (strlen(FileName) > 4)
    if (stricmp(&FileName[strlen(FileName)-4],".BMP") == 0)
        FileFormat = ID_BMP;

_chdir(currdir);

return i;

}

static PRINTDLG pd;
static BOOL print_abort_flag;
static HWND print_abort_dialog;

int CALLBACK PrintAbortDlg(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_COMMAND) { /* Cancel button (Enter, Esc, Return, Space) */
        print_abort_flag = TRUE;
        DestroyWindow(hWnd);
        return (TRUE);
        }
    if (msg == WM_INITDIALOG) {
        center_window(hWnd,0,0);
        SetFocus(hWnd);
        return (TRUE);
        }
    return (FALSE);
}

int CALLBACK PrintAbort(HDC hPr, int Code)
{
    MSG msg;
    while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
        if (!IsDialogMessage(print_abort_dialog, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            }
    return (! print_abort_flag); /* return FALSE iff aborted */
}

void PrintFile(void)
{
int printer_xdots, printer_ydots;
float aspect;
extern int win_xdots, win_ydots;
extern int pixelshift_per_byte;
extern int bytes_per_pixelline;

    /* initialize the structure */
    memset(&pd, 0, sizeof(pd));
    pd.lStructSize = sizeof(pd);
    pd.hwndOwner = (HWND)NULL;
    pd.Flags = PD_RETURNDC | PD_NOPAGENUMS | PD_NOSELECTION | PD_PRINTSETUP;

    if (PrintDlg(&pd) == TRUE) {
        int printer_bandable;
        long firstpixel;
        RECT printerRect;
        FARPROC p_print_abort;
        FARPROC p_pabort_dialog;
        int more;

        print_abort_flag = FALSE;

        printer_bandable = GetDeviceCaps(pd.hDC,RASTERCAPS) & RC_BANDING;

        if (GetDeviceCaps(pd.hDC,NUMCOLORS) <= 2)
            mono_dib_palette();        /* B&W stripes for B&W printers */
        else
            rgb_dib_palette();

        printer_xdots = GetDeviceCaps(pd.hDC,HORZRES);
        printer_ydots = GetDeviceCaps(pd.hDC,VERTRES);
        aspect = (float)((((1.0 * printer_ydots) / printer_xdots) * xdots) / ydots);
        if (aspect > 1.0) printer_ydots = (int)(printer_ydots / aspect);
        if (aspect < 1.0) printer_xdots = (int)(printer_xdots / aspect);

        firstpixel = win_ydots - ydots;
        firstpixel = firstpixel * bytes_per_pixelline;

        p_print_abort  = MakeProcInstance((FARPROC)PrintAbort,    hInst);
        p_pabort_dialog = MakeProcInstance((FARPROC)PrintAbortDlg, hInst);
        print_abort_dialog = CreateDialog(hInst, "Printabort", hwnd, (DLGPROC)p_pabort_dialog);
        ShowWindow(print_abort_dialog, SW_NORMAL);
        UpdateWindow(print_abort_dialog);
        EnableWindow(hwnd, FALSE);
        Escape (pd.hDC, SETABORTPROC, 0, (LPSTR)p_print_abort, NULL);

        Escape(pd.hDC, STARTDOC,  17, (LPSTR)"Winfract Printout", NULL);

        if (printer_bandable)
                Escape(pd.hDC, NEXTBAND, 0, (LPSTR) NULL, (LPSTR) &printerRect);

        more = 1;
        while (more) {
            if (printer_bandable)
                DPtoLP(pd.hDC, (LPPOINT) &printerRect, 2);
            StretchDIBits(pd.hDC,
                0, 0,
                printer_xdots, printer_ydots,
                0, 0,
                xdots, ydots,
                (LPSTR)&pixels[firstpixel], (LPBITMAPINFO)pDibInfo,
                DIB_RGB_COLORS, SRCCOPY);
               if (printer_bandable)
                Escape(pd.hDC, NEXTBAND, 0, (LPSTR) NULL, (LPSTR) &printerRect);
            more = ! (IsRectEmpty(&printerRect));
            if (print_abort_flag) more = FALSE;
            }
        Escape(pd.hDC, NEWFRAME, 0, NULL, NULL);
        Escape(pd.hDC, ENDDOC, 0, NULL, NULL );

        EnableWindow(hwnd, TRUE);
        DestroyWindow(print_abort_dialog);
        FreeProcInstance(p_print_abort);
        FreeProcInstance(p_pabort_dialog);
        DeleteDC(pd.hDC);
        if (pd.hDevMode)
            GlobalFree(pd.hDevMode);
        if (pd.hDevNames)
            GlobalFree(pd.hDevNames);
    }
    default_dib_palette();   /* replace the palette */
}
