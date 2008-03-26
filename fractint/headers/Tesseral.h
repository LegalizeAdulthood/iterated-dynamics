#if !defined(TESSERAL_H)
#define TESSERAL_H

class TesseralScan
{
public:
	virtual void Execute() = 0;

protected:
	virtual ~TesseralScan() { }

};

extern TesseralScan &g_tesseralScan;

#endif
