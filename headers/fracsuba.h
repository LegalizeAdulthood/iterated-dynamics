#pragma once
#if !defined(FRACSUBA_H)
#define FRACSUBA_H

// fracsuba -- assembler file prototypes
extern int asmlMODbailout();
extern int asmlREALbailout();
extern int asmlIMAGbailout();
extern int asmlORbailout();
extern int asmlANDbailout();
extern int asmlMANHbailout();
extern int asmlMANRbailout();
extern int asm386lMODbailout();
extern int asm386lREALbailout();
extern int asm386lIMAGbailout();
extern int asm386lORbailout();
extern int asm386lANDbailout();
extern int asm386lMANHbailout();
extern int asm386lMANRbailout();
extern int asmfpMODbailout();
extern int asmfpREALbailout();
extern int asmfpIMAGbailout();
extern int asmfpORbailout();
extern int asmfpANDbailout();
extern int asmfpMANHbailout();
extern int asmfpMANRbailout();

#endif
