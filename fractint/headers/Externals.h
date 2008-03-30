#pragma once

#include <complex>
#include <boost/filesystem/path.hpp>

#include "Browse.h"
#include "calcfrac.h"
#include "EscapeTime.h"
#include "SoundState.h"
#include "UserInterfaceState.h"
#include "ZoomBox.h"

class ViewWindow;

class Externals
{
public:
	virtual ~Externals() { }

	virtual float AspectDrift() const = 0;
	virtual void SetAspectDrift(float value) = 0;
	virtual int AtanColors() const = 0;
	virtual void SetAtanColors(int value) = 0;
	virtual double AutoStereoWidth() const = 0;
	virtual void SetAutoStereoWidth(double value) = 0;
	virtual bool BadOutside() const = 0;
	virtual void SetBadOutside(bool value) = 0;
	virtual long BailOut() const = 0;
	virtual void SetBailOut(long value) = 0;
	typedef int BailOutFunction();
	virtual int BailOutFp() = 0;
	virtual void SetBailOutFp(BailOutFunction *value) = 0;
	virtual int BailOutL() = 0;
	virtual void SetBailOutL(BailOutFunction *value) = 0;
	virtual int BailOutBf() = 0;
	virtual void SetBailOutBf(BailOutFunction *value) = 0;
	virtual int BailOutBn() = 0;
	virtual void SetBailOutBn(BailOutFunction *value) = 0;
	virtual BailOutType BailOutTest() const = 0;
	virtual void SetBailOutTest(BailOutType value) = 0;
	virtual int Basin() const = 0;
	virtual void SetBasin(int value) = 0;
	virtual int BfSaveLen() const = 0;
	virtual void SetBfSaveLen(int value) = 0;
	virtual int BfDigits() const = 0;
	virtual void SetBfDigits(int value) = 0;
	virtual int Biomorph() const = 0;
	virtual void SetBiomorph(int value) = 0;
	virtual int BitShift() const = 0;
	virtual void SetBitShift(int value) = 0;
	virtual int BitShiftMinus1() const = 0;
	virtual void SetBitShiftMinus1(int value) = 0;
	virtual BYTE const *Block() const = 0;
	virtual void SetBlock(BYTE const *value) = 0;
	virtual long CalculationTime() const = 0;
	virtual void SetCalculationTime(long value) = 0;
	typedef long CalculateMandelbrotFunction();
	virtual CalculateMandelbrotFunction *CalculateMandelbrotAsmFp() const = 0;
	virtual void SetCalculateMandelbrotAsmFp(CalculateMandelbrotFunction *value) = 0;
	typedef int CalculateTypeFunction();
	virtual CalculateTypeFunction *CalculateType() const = 0;
	virtual void SetCalculateType(CalculateTypeFunction *value) = 0;
	virtual CalculationStatusType CalculationStatus() const = 0;
	virtual void SetCalculationStatus(CalculationStatusType value) = 0;
	virtual int const *CfgLineNums() const = 0;
	virtual void SetCfgLineNums(int const *value)= 0;
	virtual bool CheckCurrentDir() const = 0;
	virtual void SetCheckCurrentDir(bool value) = 0;
	virtual long CImag() const = 0;
	virtual void SetCImag(long value) = 0;
	virtual double CloseEnough() const = 0;
	virtual void SetCloseEnough(double value) = 0;
	virtual double Proximity() const = 0;
	virtual void SetProximity(double value) = 0;
	virtual ComplexD Coefficient() const = 0;
	virtual void SetCoefficient(ComplexD value) = 0;
	virtual int Col() const = 0;
	virtual void SetCol(int value) = 0;
	virtual int Color() const = 0;
	virtual void SetColor(int value) = 0;
	virtual std::string const &ColorFile() const = 0;
	virtual void SetColorFile(std::string const &value) = 0;
	virtual long ColorIter() const = 0;
	virtual void SetColorIter(long value) = 0;
	virtual bool ColorPreloaded() const = 0;
	virtual void SetColorPreloaded(bool value) = 0;
	virtual int Colors() const = 0;
	virtual void SetColors(int value) = 0;
	virtual bool CompareGif() const = 0;
	virtual void SetCompareGif(bool value) = 0;
	virtual double CosX() const = 0;
	virtual void SetCosX(double value) = 0;
	virtual long CReal() const = 0;
	virtual void SetCReal(long value) = 0;
	virtual int CurrentCol() const = 0;
	virtual void SetCurrentCol(int value) = 0;
	virtual int CurrentPass() const = 0;
	virtual void SetCurrentPass(int value) = 0;
	virtual int CurrentRow() const = 0;
	virtual void SetCurrentRow(int value) = 0;
	virtual int CycleLimit() const = 0;
	virtual void SetCycleLimit(int value) = 0;
	virtual int CExp() const = 0;
	virtual void SetCExp(int value) = 0;
	virtual double DeltaMinFp() const = 0;
	virtual void SetDeltaMinFp(double value) = 0;
	virtual int DebugMode() const = 0;
	virtual void SetDebugMode(int value) = 0;
	virtual int Decimals() const = 0;
	virtual void SetDecimals(int value) = 0;
	virtual BYTE const *DecoderLine() const = 0;
	virtual void SetDecoderLine(BYTE const *value) = 0;
	virtual int const *Decomposition() const = 0;
	virtual void SetDecomposition(int const *value) = 0;
	virtual int Degree() const = 0;
	virtual void SetDegree(int value) = 0;
	virtual long DeltaMin() const = 0;
	virtual void SetDeltaMin(long value) = 0;
	virtual float DepthFp() const = 0;
	virtual void SetDepthFp(float value) = 0;
	virtual bool Disk16Bit() const = 0;
	virtual void SetDisk16Bit(bool value) = 0;
	virtual bool DiskFlag() const = 0;
	virtual void SetDiskFlag(bool value) = 0;
	virtual bool DiskTarga() const = 0;
	virtual void SetDiskTarga(bool value) = 0;
	virtual Display3DType Display3D() const = 0;
	virtual void SetDisplay3D(Display3DType value) = 0;
	virtual long DistanceTest() const = 0;
	virtual void SetDistanceTest(long value) = 0;
	virtual int DistanceTestWidth() const = 0;
	virtual void SetDistanceTestWidth(int value) = 0;
	virtual float ScreenDistanceFp() const = 0;
	virtual void SetScreenDistanceFp(float value) = 0;
	virtual bool DitherFlag() const = 0;
	virtual void SetDitherFlag(bool value) = 0;
	virtual bool DontReadColor() const = 0;
	virtual void SetDontReadColor(bool value) = 0;
	virtual double DeltaParameterImageX() const = 0;
	virtual void SetDeltaParameterImageX(double value) = 0;
	virtual double DeltaParameterImageY() const = 0;
	virtual void SetDeltaParameterImageY(double value) = 0;
	virtual BYTE const *Stack() const = 0;
	virtual void SetStack(BYTE const *value) = 0;
	typedef double DxPixelFunction();
	virtual double DxPixel() = 0;
	virtual void SetDxPixel(DxPixelFunction *value) = 0;
	virtual double DxSize() const = 0;
	virtual void SetDxSize(double value) = 0;
	typedef double DyPixelFunction();
	virtual double DyPixel() = 0;
	virtual ComplexD DPixel() = 0;
	virtual void SetDyPixel(DyPixelFunction *value) = 0;
	virtual double DySize() const = 0;
	virtual void SetDySize(double value) = 0;
	virtual bool EscapeExitFlag() const = 0;
	virtual void SetEscapeExitFlag(bool value) = 0;
	virtual int EvolvingFlags() const = 0;
	virtual void SetEvolvingFlags(int value) = 0;
	virtual evolution_info const *Evolve() const = 0;
	virtual evolution_info *Evolve() = 0;
	virtual void SetEvolve(evolution_info *value) = 0;
	virtual float EyesFp() const = 0;
	virtual void SetEyesFp(float value) = 0;
	virtual bool FastRestore() const = 0;
	virtual void SetFastRestore(bool value) = 0;
	virtual double FudgeLimit() const = 0;
	virtual void SetFudgeLimit(double value) = 0;
	virtual long OneFudge() const = 0;
	virtual void SetOneFudge(long value) = 0;
	virtual long TwoFudge() const = 0;
	virtual void SetTwoFudge(long value) = 0;
	virtual int GridSize() const = 0;
	virtual void SetGridSize(int value) = 0;
	virtual double FiddleFactor() const = 0;
	virtual void SetFiddleFactor(double value) = 0;
	virtual double FiddleReduction() const = 0;
	virtual void SetFiddleReduction(double value) = 0;
	virtual float FileAspectRatio() const = 0;
	virtual void SetFileAspectRatio(float value) = 0;
	virtual int FileColors() const = 0;
	virtual void SetFileColors(int value) = 0;
	virtual int FileXDots() const = 0;
	virtual void SetFileXDots(int value) = 0;
	virtual int FileYDots() const = 0;
	virtual void SetFileYDots(int value) = 0;
	virtual int FillColor() const = 0;
	virtual void SetFillColor(int value) = 0;
	virtual int FinishRow() const = 0;
	virtual void SetFinishRow(int value) = 0;
	virtual bool CommandInitialize() const = 0;
	virtual void SetCommandInitialize(bool value) = 0;
	virtual int FirstSavedAnd() const = 0;
	virtual void SetFirstSavedAnd(int value) = 0;
	virtual bool FloatFlag() const = 0;
	virtual void SetFloatFlag(bool value) = 0;
	virtual ComplexD const *FloatParameter() const = 0;
	virtual void SetFloatParameter(ComplexD const *value) = 0;
	virtual BYTE const *Font8x8() const = 0;
	virtual int ForceSymmetry() const = 0;
	virtual void SetForceSymmetry(int value) = 0;
	virtual int FractalType() const = 0;
	virtual void SetFractalType(int value) = 0;
	virtual bool FromTextFlag() const = 0;
	virtual void SetFromTextFlag(bool value) = 0;
	virtual long Fudge() const = 0;
	virtual void SetFudge(long value) = 0;
	virtual FunctionListItem const *FunctionList() const = 0;
	virtual void SetFunctionList(FunctionListItem const *value) = 0;
	virtual bool FunctionPreloaded() const = 0;
	virtual void SetFunctionPreloaded(bool value) = 0;
	virtual double FRadius() const = 0;
	virtual void SetFRadius(double value) = 0;
	virtual double FXCenter() const = 0;
	virtual void SetFXCenter(double value) = 0;
	virtual double FYCenter() const = 0;
	virtual void SetFYCenter(double value) = 0;
	virtual GENEBASE const *Genes() const = 0;
	virtual GENEBASE *Genes() = 0;
	virtual bool Gif87aFlag() const = 0;
	virtual void SetGif87aFlag(bool value) = 0;
	virtual TabStatusType TabStatus() const = 0;
	virtual void SetTabStatus(TabStatusType value) = 0;
	virtual bool GrayscaleDepth() const = 0;
	virtual void SetGrayscaleDepth(bool value) = 0;
	virtual bool HasInverse() const = 0;
	virtual void SetHasInverse(bool value) = 0;
	virtual unsigned int Height() const = 0;
	virtual void SetHeight(int value) = 0;
	virtual float HeightFp() const = 0;
	virtual void SetHeightFp(float value) = 0;
	virtual float const *IfsDefinition() const = 0;
	virtual void SetIfsDefinition(float const *value) = 0;
	virtual int IfsType() const = 0;
	virtual void SetIfsType(int value) = 0;
	virtual ComplexD InitialZ() const = 0;
	virtual void SetInitialZ(ComplexD value) = 0;
	virtual InitializeBatchType InitializeBatch() const = 0;
	virtual void SetInitializeBatch(InitializeBatchType value) = 0;
	virtual int InitialCycleLimit() const = 0;
	virtual void SetInitialCycleLimit(int value) = 0;

	virtual ComplexL InitialOrbitL() const = 0;
	virtual void SetInitialOrbitL(ComplexL value) = 0;
	virtual ComplexD InitialOrbitZ() const = 0;
	virtual void SetInitialOrbitZ(ComplexD value) = 0;

	virtual int SaveTime() const = 0;
	virtual void SetSaveTime(int value) = 0;
	virtual int Inside() const = 0;
	virtual void SetInside(int value) = 0;
	virtual int IntegerFractal() const = 0;
	virtual void SetIntegerFractal(int value) = 0;
	virtual double const *Inversion() const = 0;
	virtual void SetInversion(double const *value) = 0;
	virtual int Invert() const = 0;
	virtual void SetInvert(int value) = 0;
	virtual int IsTrueColor() const = 0;
	virtual void SetIsTrueColor(int value) = 0;
	virtual bool IsMandelbrot() const = 0;
	virtual void SetIsMandelbrot(bool value) = 0;
	virtual int XStop() const = 0;
	virtual void SetXStop(int value) = 0;
	virtual int YStop() const = 0;
	virtual void SetYStop(int value) = 0;
	virtual std::complex<double> JuliaC() const = 0;
	virtual void SetJuliaC(std::complex<double> const &value) = 0;
	virtual int Juli3DMode() const = 0;
	virtual void SetJuli3DMode(int value) = 0;
	virtual char const **Juli3DOptions() const = 0;
	virtual bool Julibrot() const = 0;
	virtual void SetJulibrot(bool value) = 0;
	virtual int InputCounter() const = 0;
	virtual void SetInputCounter(int value) = 0;
	virtual bool KeepScreenCoords() const = 0;
	virtual void SetKeepScreenCoords(bool value) = 0;
	virtual long CloseEnoughL() const = 0;
	virtual void SetCloseEnoughL(long value) = 0;
	virtual ComplexL CoefficientL() const = 0;
	virtual void SetCoefficientL(ComplexL value) = 0;
	virtual bool UseOldComplexPower() const = 0;
	virtual void SetUseOldComplexPower(bool value) = 0;
	virtual BYTE const *LineBuffer() const = 0;
	virtual void SetLineBuffer(BYTE const *value) = 0;
	virtual ComplexL InitialZL() const = 0;
	virtual void SetInitialZL(ComplexL value) = 0;
	virtual long InitialXL() const = 0;
	virtual void SetInitialXL(long value) = 0;
	virtual long InitialYL() const = 0;
	virtual void SetInitialYL(long value) = 0;
	virtual long RqLimit2L() const = 0;
	virtual void SetRqLimit2L(long value) = 0;
	virtual long RqLimitL() const = 0;
	virtual void SetRqLimitL(long value) = 0;
	virtual long MagnitudeL() const = 0;
	virtual void SetMagnitudeL(long value) = 0;
	virtual ComplexL NewZL() const = 0;
	virtual void SetNewZL(ComplexL value) = 0;
	virtual bool Loaded3D() const = 0;
	virtual void SetLoaded3D(bool value) = 0;
	virtual bool LogAutomaticFlag() const = 0;
	virtual void SetLogAutomaticFlag(bool value) = 0;
	virtual bool LogCalculation() const = 0;
	virtual void SetLogCalculation(bool value) = 0;
	virtual int LogDynamicCalculate() const = 0;
	virtual void SetLogDynamicCalculate(int value) = 0;
	virtual long LogPaletteMode() const = 0;
	virtual void SetLogPaletteMode(long value) = 0;
	virtual BYTE const *LogTable() const = 0;
	virtual void SetLogTable(BYTE const *value) = 0;
	virtual ComplexL OldZL() const = 0;
	virtual void SetOldZL(ComplexL value) = 0;
	virtual ComplexL const *LongParameter() const = 0;
	virtual void SetLongParameter(ComplexL const *value) = 0;
	virtual ComplexL Parameter2L() const = 0;
	virtual void SetParameter2L(ComplexL value) = 0;
	virtual ComplexL ParameterL() const =0 ;
	virtual void SetParameterL(ComplexL value) = 0;
	virtual long TempSqrXL() const = 0;
	virtual void SetTempSqrXL(long value) = 0;
	virtual long TempSqrYL() const = 0;
	virtual void SetTempSqrYL(long value) = 0;
	typedef long PixelFunctionL();
	virtual long LxPixel() = 0;
	virtual void SetLxPixel(PixelFunctionL *value) = 0;
	virtual long LyPixel() = 0;
	virtual void SetLyPixel(PixelFunctionL *value) = 0;
	virtual ComplexL LPixel() = 0;
	typedef void TrigFunction();
	virtual TrigFunction *Trig0L() const = 0;
	virtual void SetTrig0L(TrigFunction *value) = 0;
	virtual TrigFunction *Trig1L() const = 0;
	virtual void SetTrig1L(TrigFunction *value) = 0;
	virtual TrigFunction *Trig2L() const = 0;
	virtual void SetTrig2L(TrigFunction *value) = 0;
	virtual TrigFunction *Trig3L() const = 0;
	virtual void SetTrig3L(TrigFunction *value) = 0;
	virtual TrigFunction *Trig0D() const = 0;
	virtual void SetTrig0D(TrigFunction *value) = 0;
	virtual TrigFunction *Trig1D() const = 0;
	virtual void SetTrig1D(TrigFunction *value) = 0;
	virtual TrigFunction *Trig2D() const = 0;
	virtual void SetTrig2D(TrigFunction *value) = 0;
	virtual TrigFunction *Trig3D() const = 0;
	virtual void SetTrig3D(TrigFunction *value) = 0;
	virtual double Magnitude() const = 0;
	virtual void SetMagnitude(double value) = 0;
	virtual unsigned long MagnitudeLimit() const = 0;
	virtual void SetMagnitudeLimit(long value) = 0;
	virtual MajorMethodType MajorMethod() const = 0;
	virtual void SetMajorMethod(MajorMethodType value) = 0;
	virtual int MathErrorCount() const = 0;
	virtual void SetMathErrorCount(int value) = 0;
	virtual double const *MathTolerance() const = 0;
	virtual void SetMathTolerance(double const *value) = 0;
	virtual long MaxCount() const = 0;
	virtual void SetMaxCount(long value) = 0;
	virtual long MaxIteration() const = 0;
	virtual void SetMaxIteration(long value) = 0;
	virtual int MaxLineLength() const = 0;
	virtual void SetMaxLineLength(int value) = 0;
	virtual long MaxLogTableSize() const = 0;
	virtual void SetMaxLogTableSize(long value) = 0;
	virtual long BnMaxStack() const = 0;
	virtual void SetBnMaxStack(long value) = 0;
	virtual int MaxColors() const = 0;
	virtual void SetMaxColors(int value) = 0;
	virtual int MaxInputCounter() const = 0;
	virtual void SetMaxInputCounter(int value) = 0;
	virtual int MaxHistory() const = 0;
	virtual void SetMaxHistory(int value) = 0;
	virtual MinorMethodType MinorMethod() const = 0;
	virtual void SetMinorMethod(MinorMethodType value) = 0;
	virtual more_parameters const *MoreParameters() const = 0;
	virtual void SetMoreParameters(more_parameters const *value) = 0;
	virtual int OverflowMp() const = 0;
	virtual void SetOverflowMp(int value) = 0;
	virtual double MXMaxFp() const = 0;
	virtual void SetMXMaxFp(double value) = 0;
	virtual double MXMinFp() const = 0;
	virtual void SetMXMinFp(double value) = 0;
	virtual double MYMaxFp() const = 0;
	virtual void SetMYMaxFp(double value) = 0;
	virtual double MYMinFp() const = 0;
	virtual void SetMYMinFp(double value) = 0;
	virtual int NameStackPtr() const = 0;
	virtual void SetNameStackPtr(int value) = 0;
	virtual ComplexD NewZ() const = 0;
	virtual void SetNewZ(ComplexD value) = 0;
	virtual int NewDiscreteParameterOffsetX() const = 0;
	virtual void SetNewDiscreteParameterOffsetX(int value) = 0;
	virtual int NewDiscreteParameterOffsetY() const = 0;
	virtual void SetNewDiscreteParameterOffsetY(int value) = 0;
	virtual double NewParameterOffsetX() const = 0;
	virtual void SetNewParameterOffsetX(double value) = 0;
	virtual double NewParameterOffsetY() const = 0;
	virtual void SetNewParameterOffsetY(double value) = 0;
	virtual int NewOrbitType() const = 0;
	virtual void SetNewOrbitType(int value) = 0;
	virtual int NextSavedIncr() const = 0;
	virtual void SetNextSavedIncr(int value) = 0;
	virtual bool NoMagnitudeCalculation() const = 0;
	virtual void SetNoMagnitudeCalculation(bool value) = 0;
	virtual int NumAffine() const = 0;
	virtual void SetNumAffine(int value) = 0;
	virtual const int NumFunctionList() const = 0;
	virtual int NumFractalTypes() const = 0;
	virtual bool NextScreenFlag() const = 0;
	virtual void SetNextScreenFlag(bool value) = 0;
	virtual bool OkToPrint() const = 0;
	virtual void SetOkToPrint(bool value) = 0;
	virtual ComplexD OldZ() const = 0;
	virtual void SetOldZ(ComplexD value) = 0;
	virtual long OldColorIter() const = 0;
	virtual void SetOldColorIter(long value) = 0;
	virtual bool OldDemmColors() const = 0;
	virtual void SetOldDemmColors(bool value) = 0;
	virtual int DiscreteParameterOffsetX() const = 0;
	virtual void SetDiscreteParameterOffsetX(int value) = 0;
	virtual int DiscreteParameterOffsetY() const = 0;
	virtual void SetDiscreteParameterOffsetY(int value) = 0;
	virtual double ParameterOffsetX() const = 0;
	virtual void SetParameterOffsetX(double value) = 0;
	virtual double ParameterOffsetY() const = 0;
	virtual void SetParameterOffsetY(double value) = 0;
	virtual int OrbitSave() const = 0;
	virtual void SetOrbitSave(int value) = 0;
	virtual int OrbitColor() const = 0;
	virtual void SetOrbitColor(int value) = 0;
	virtual int OrbitDelay() const = 0;
	virtual void SetOrbitDelay(int value) = 0;
	virtual int OrbitDrawMode() const = 0;
	virtual void SetOrbitDrawMode(int value) = 0;
	virtual long OrbitInterval() const = 0;
	virtual void SetOrbitInterval(long value) = 0;
	virtual int OrbitIndex() const = 0;
	virtual void SetOrbitIndex(int value) = 0;
	virtual bool OrganizeFormulaSearch() const = 0;
	virtual void SetOrganizeFormulaSearch(bool value) = 0;
	virtual float OriginFp() const = 0;
	virtual void SetOriginFp(float value) = 0;
	typedef int OutLineFunction(BYTE const *pixels, int length);
	virtual OutLineFunction *OutLine() const = 0;
	virtual void SetOutLine(OutLineFunction *value) = 0;
	typedef void OutLineCleanupFunction();
	virtual OutLineCleanupFunction *OutLineCleanup() const = 0;
	virtual void SetOutLineCleanup(OutLineCleanupFunction *value) = 0;
	virtual int Outside() const = 0;
	virtual void SetOutside(int value) = 0;
	virtual bool Overflow() const = 0;
	virtual void SetOverflow(bool value) = 0;
	virtual bool Overlay3D() const = 0;
	virtual void SetOverlay3D(bool value) = 0;
	virtual bool FractalOverwrite() const = 0;
	virtual void SetFractalOverwrite(bool value) = 0;
	virtual double OrbitX3rd() const = 0;
	virtual void SetOrbitX3rd(double value) = 0;
	virtual double OrbitXMax() const = 0;
	virtual void SetOrbitXMax(double value) = 0;
	virtual double OrbitXMin() const = 0;
	virtual void SetOrbitXMin(double value) = 0;
	virtual double OrbitY3rd() const = 0;
	virtual void SetOrbitY3rd(double value) = 0;
	virtual double OrbitYMax() const = 0;
	virtual void SetOrbitYMax(double value) = 0;
	virtual double OrbitYMin() const = 0;
	virtual void SetOrbitYMin(double value) = 0;
	virtual double const *Parameters() const = 0;
	virtual void SetParameters(double const *value) = 0;
	virtual double ParameterRangeX() const = 0;
	virtual void SetParameterRangeX(double value) = 0;
	virtual double ParameterRangeY() const = 0;
	virtual void SetParameterRangeY(double value) = 0;
	virtual double ParameterZoom() const = 0;
	virtual void SetParameterZoom(double value) = 0;
	virtual ComplexD Parameter2() const = 0;
	virtual void SetParameter2(ComplexD value) = 0;
	virtual ComplexD Parameter() const = 0;
	virtual void SetParameter(ComplexD value) = 0;
	virtual int PatchLevel() const = 0;
	virtual void SetPatchLevel(int value) = 0;
	virtual int PeriodicityCheck() const = 0;
	virtual void SetPeriodicityCheck(int value) = 0;
	typedef void PlotColorFunction(int x, int y, int color);
	virtual PlotColorFunction *PlotColor() const = 0;
	virtual void SetPlotColor(PlotColorFunction *value) = 0;
	virtual double PlotMx1() const = 0;
	virtual void SetPlotMx1(double value) = 0;
	virtual double PlotMx2() const = 0;
	virtual void SetPlotMx2(double value) = 0;
	virtual double PlotMy1() const = 0;
	virtual void SetPlotMy1(double value) = 0;
	virtual double PlotMy2() const = 0;
	virtual void SetPlotMy2(double value) = 0;
	virtual bool Potential16Bit() const = 0;
	virtual void SetPotential16Bit(bool value) = 0;
	virtual bool PotentialFlag() const = 0;
	virtual void SetPotentialFlag(bool value) = 0;
	virtual double const *PotentialParameter() const = 0;
	virtual void SetPotentialParameter(double const *value) = 0;
	virtual int Px() const = 0;
	virtual void SetPx(int value) = 0;
	virtual int Py() const = 0;
	virtual void SetPy(int value) = 0;
	virtual int ParameterBoxCount() const = 0;
	virtual void SetParameterBoxCount(int value) = 0;
	virtual int PseudoX() const = 0;
	virtual void SetPseudoX(int value) = 0;
	virtual int PseudoY() const = 0;
	virtual void SetPseudoY(int value) = 0;
	typedef void PlotColorPutColorFunction(int x, int y, int color);
	virtual PlotColorPutColorFunction *PlotColorPutColor() const = 0;
	virtual void SetPlotColorPutColor(PlotColorPutColorFunction *value) = 0;
	virtual ComplexD Power() const = 0;
	virtual void SetPower(ComplexD value) = 0;
	virtual bool QuickCalculate() const = 0;
	virtual void SetQuickCalculate(bool value) = 0;
	virtual int const *Ranges() const = 0;
	virtual void SetRanges(int const *value) = 0;
	virtual int RangesLength() const = 0;
	virtual void SetRangesLength(int value) = 0;
	virtual long RealColorIter() const = 0;
	virtual void SetRealColorIter(long value) = 0;
	virtual int Release() const = 0;
	virtual void SetRelease(int value) = 0;
	virtual int ResaveMode() const = 0;
	virtual void SetResaveMode(int value) = 0;
	virtual bool ResetPeriodicity() const = 0;
	virtual void SetResetPeriodicity(bool value) = 0;
	virtual char const *ResumeInfo() const = 0;
	virtual void SetResumeInfo(char const *value) = 0;
	virtual int ResumeLength() const = 0;
	virtual void SetResumeLength(int value) = 0;
	virtual bool Resuming() const = 0;
	virtual void SetResuming(bool value) = 0;
	virtual bool UseFixedRandomSeed() const = 0;
	virtual void SetUseFixedRandomSeed(bool value) = 0;
	virtual char const *RleBuffer() const = 0;
	virtual void SetRleBuffer(char const *value) = 0;
	virtual int RotateHi() const = 0;
	virtual void SetRotateHi(int value) = 0;
	virtual int RotateLo() const = 0;
	virtual void SetRotateLo(int value) = 0;
	virtual int Row() const = 0;
	virtual void SetRow(int value) = 0;
	virtual int RowCount() const = 0;
	virtual void SetRowCount(int value) = 0;
	virtual double RqLimit2() const = 0;
	virtual void SetRqLimit2(double value) = 0;
	virtual double RqLimit() const = 0;
	virtual void SetRqLimit(double value) = 0;
	virtual int RandomSeed() const = 0;
	virtual void SetRandomSeed(int value) = 0;
	virtual long SaveBase() const = 0;
	virtual void SetSaveBase(long value) = 0;
	virtual ComplexD SaveC() const = 0;
	virtual void SetSaveC(ComplexD value) = 0;
	virtual long SaveTicks() const = 0;
	virtual void SetSaveTicks(long value) = 0;
	virtual int SaveRelease() const = 0;
	virtual void SetSaveRelease(int value) = 0;
	virtual float ScreenAspectRatio() const = 0;
	virtual void SetScreenAspectRatio(float value) = 0;
	virtual int ScreenHeight() const = 0;
	virtual void SetScreenHeight(int value) = 0;
	virtual int ScreenWidth() const = 0;
	virtual void SetScreenWidth(int value) = 0;
	virtual bool SetOrbitCorners() const = 0;
	virtual void SetSetOrbitCorners(bool value) = 0;
	virtual int ShowDot() const = 0;
	virtual void SetShowDot(int value) = 0;
	virtual ShowFileType ShowFile() const = 0;
	virtual void SetShowFile(ShowFileType value) = 0;
	virtual bool ShowOrbit() const = 0;
	virtual void SetShowOrbit(bool value) = 0;
	virtual double SinX() const = 0;
	virtual void SetSinX(double value) = 0;
	virtual int SizeDot() const = 0;
	virtual void SetSizeDot(int value) = 0;
	virtual short const *SizeOfString() const = 0;
	virtual void SetSizeOfString(short const *value) = 0;
	virtual int SkipXDots() const = 0;
	virtual void SetSkipXDots(int value) = 0;
	virtual int SkipYDots() const = 0;
	virtual void SetSkipYDots(int value) = 0;
	typedef void PlotColorStandardFunction(int x, int y, int color);
	virtual PlotColorStandardFunction *PlotColorStandard() const = 0;
	virtual void SetPlotColorStandard(PlotColorStandardFunction *value) = 0;
	virtual bool StartShowOrbit() const = 0;
	virtual void SetStartShowOrbit(bool value) = 0;
	virtual bool StartedResaves() const = 0;
	virtual void SetStartedResaves(bool value) = 0;
	virtual int StopPass() const = 0;
	virtual void SetStopPass(int value) = 0;
	virtual unsigned int const *StringLocation() const = 0;
	virtual void SetStringLocation(unsigned int const *value) = 0;
	virtual BYTE const *Suffix() const = 0;
	virtual void SetSuffix(BYTE const *value) = 0;
	virtual double Sx3rd() const = 0;
	virtual void SetSx3rd(double value) = 0;
	virtual double SxMax() const = 0;
	virtual void SetSxMax(double value) = 0;
	virtual double SxMin() const = 0;
	virtual void SetSxMin(double value) = 0;
	virtual int ScreenXOffset() const = 0;
	virtual void SetScreenXOffset(int value) = 0;
	virtual double Sy3rd() const = 0;
	virtual void SetSy3rd(double value) = 0;
	virtual double SyMax() const = 0;
	virtual void SetSyMax(double value) = 0;
	virtual double SyMin() const = 0;
	virtual void SetSyMin(double value) = 0;
	virtual SymmetryType Symmetry() const = 0;
	virtual void SetSymmetry(SymmetryType value) = 0;
	virtual int ScreenYOffset() const = 0;
	virtual void SetScreenYOffset(int value) = 0;
	virtual bool TabDisplayEnabled() const = 0;
	virtual void SetTabDisplayEnabled(bool value) = 0;
	virtual bool TargaOutput() const = 0;
	virtual void SetTargaOutput(bool value) = 0;
	virtual bool TargaOverlay() const = 0;
	virtual void SetTargaOverlay(bool value) = 0;
	virtual double TempSqrX() const = 0;
	virtual void SetTempSqrX(double value) = 0;
	virtual double TempSqrY() const = 0;
	virtual void SetTempSqrY(double value) = 0;
	virtual int TextCbase() const = 0;
	virtual void SetTextCbase(int value) = 0;
	virtual int TextCol() const = 0;
	virtual void SetTextCol(int value) = 0;
	virtual int TextRbase() const = 0;
	virtual void SetTextRbase(int value) = 0;
	virtual int TextRow() const = 0;
	virtual void SetTextRow(int value) = 0;
	virtual unsigned int RhisGenerationRandomSeed() const = 0;
	virtual void SetThisGenerationRandomSeed(int value) = 0;
	virtual bool ThreePass() const = 0;
	virtual void SetThreePass(bool value) = 0;
	virtual double Threshold() const = 0;
	virtual void SetThreshold(double value) = 0;
	virtual TimedSaveType TimedSave() const = 0;
	virtual void SetTimedSave(TimedSaveType value) = 0;
	virtual bool TimerFlag() const = 0;
	virtual void SetTimerFlag(bool value) = 0;
	virtual long TimerInterval() const = 0;
	virtual void SetTimerInterval(long value) = 0;
	virtual long TimerStart() const = 0;
	virtual void SetTimerStart(long value) = 0;

	virtual ComplexD TempZ() const = 0;
	virtual void SetTempZ(ComplexD value) = 0;
	virtual ComplexL TempZL() const = 0;
	virtual void SetTempZL(ComplexL value) = 0;

	virtual int TotalPasses() const = 0;
	virtual void SetTotalPasses(int value) = 0;
	virtual int const *FunctionIndex() const = 0;
	virtual void SetFunctionIndex(int const *value) = 0;
	virtual bool TrueColor() const = 0;
	virtual void SetTrueColor(bool value) = 0;
	virtual bool TrueModeIterates() const = 0;
	virtual void SetTrueModeIterates(bool value) = 0;
	virtual char const *TextStack() const = 0;
	virtual void SetTextStack(char const *value) = 0;
	virtual double TwoPi() const = 0;
	virtual void SetTwoPi(double value) = 0;
	virtual UserInterfaceState const &UiState() const = 0;
	virtual UserInterfaceState &UiState() = 0;
	virtual InitialZType UseInitialOrbitZ() const = 0;
	virtual void SetUseInitialOrbitZ(InitialZType value) = 0;
	virtual bool UseCenterMag() const = 0;
	virtual void SetUseCenterMag(bool value) = 0;
	virtual bool UseOldPeriodicity() const = 0;
	virtual void SetUseOldPeriodicity(bool value) = 0;
	virtual bool UsingJiim() const = 0;
	virtual void SetUsingJiim(bool value) = 0;
	virtual int UserBiomorph() const = 0;
	virtual void SetUserBiomorph(int value) = 0;
	virtual long UserDistanceTest() const = 0;
	virtual void SetUserDistanceTest(long value) = 0;
	virtual bool UserFloatFlag() const = 0;
	virtual void SetUserFloatFlag(bool value) = 0;
	virtual int UserPeriodicityCheck() const = 0;
	virtual void SetUserPeriodicityCheck(int value) = 0;
	virtual int VxDots() const = 0;
	virtual void SetVxDots(int value) = 0;
	virtual int WhichImage() const = 0;
	virtual void SetWhichImage(int value) = 0;
	virtual float WidthFp() const = 0;
	virtual void SetWidthFp(float value) = 0;
	virtual int XDots() const = 0;
	virtual void SetXDots(int value) = 0;
	virtual int XShift1() const = 0;
	virtual void SetXShift1(int value) = 0;
	virtual int XShift() const = 0;
	virtual void SetXShift(int value) = 0;
	virtual int XxAdjust1() const = 0;
	virtual void SetXxAdjust1(int value) = 0;
	virtual int XxAdjust() const = 0;
	virtual void SetXxAdjust(int value) = 0;
	virtual int YDots() const = 0;
	virtual void SetYDots(int value) = 0;
	virtual int YShift() const = 0;
	virtual void SetYShift(int value) = 0;
	virtual int YShift1() const = 0;
	virtual void SetYShift1(int value) = 0;
	virtual int YyAdjust() const = 0;
	virtual void SetYyAdjust(int value) = 0;
	virtual int YyAdjust1() const = 0;
	virtual void SetYyAdjust1(int value) = 0;
	virtual double Zbx() const = 0;
	virtual void SetZbx(double value) = 0;
	virtual double Zby() const = 0;
	virtual void SetZby(double value) = 0;
	virtual double ZDepth() const = 0;
	virtual void SetZDepth(double value) = 0;
	virtual int ZDots() const = 0;
	virtual void SetZDots(int value) = 0;
	virtual int ZRotate() const = 0;
	virtual void SetZRotate(int value) = 0;
	virtual double ZSkew() const = 0;
	virtual void SetZSkew(double value) = 0;
	virtual double ZWidth() const = 0;
	virtual void SetZWidth(double value) = 0;
	virtual bool ZoomOff() const = 0;
	virtual void SetZoomOff(bool value) = 0;
	virtual ZoomBox const &Zoom() const = 0;
	virtual ZoomBox &Zoom() = 0;
	virtual void SetZoom(ZoomBox const &value) = 0;
	virtual SoundState const &Sound() const = 0;
	virtual SoundState &Sound() = 0;
	virtual EscapeTimeState const &EscapeTime() const = 0;
	virtual EscapeTimeState &EscapeTime() = 0;
	virtual int BfMath() const = 0;
	virtual void SetBfMath(int value) = 0;
	virtual bf_t SxMinBf() const = 0;
	virtual void SetSxMinBf(bf_t value) = 0;
	virtual bf_t SxMaxBf() const = 0;
	virtual void SetSxMaxBf(bf_t value) = 0;
	virtual bf_t SyMinBf() const = 0;
	virtual void SetSyMinBf(bf_t value) = 0;
	virtual bf_t SyMaxBf() const = 0;
	virtual void SetSyMaxBf(bf_t value) = 0;
	virtual bf_t Sx3rdBf() const = 0;
	virtual void SetSx3rdBf(bf_t value) = 0;
	virtual bf_t Sy3rdBf() const = 0;
	virtual void SetSy3rdBf(bf_t value) = 0;
	virtual BrowseState const &Browse() const = 0;
	virtual BrowseState &Browse() = 0;
	virtual unsigned int ThisGenerationRandomSeed() const = 0;
	virtual void SetThisGenerationRandomSeed(unsigned int value) = 0;
	typedef int GIFViewFunction();
	virtual GIFViewFunction *GIFView() const = 0;
	virtual OutLineFunction *OutLinePotential() const = 0;
	virtual OutLineFunction *OutLineSound() const = 0;
	virtual OutLineFunction *OutLineRegular() const = 0;
	virtual OutLineCleanupFunction *OutLineCleanupNull() const = 0;
	virtual OutLineFunction *OutLine3D() const = 0;
	virtual OutLineFunction *OutLineCompare() const = 0;
	virtual std::string const &FractDir1() const = 0;
	virtual void SetFractDir1(std::string const &value) = 0;
	virtual std::string const &FractDir2() const = 0;
	virtual void SetFractDir2(std::string const &value) = 0;
	virtual std::string const &ReadName() const = 0;
	virtual std::string &ReadName() = 0;
	virtual void SetReadName(std::string const &value) = 0;
	virtual std::string &GIFMask() = 0;
	virtual ViewWindow const &View() const = 0;
	virtual ViewWindow &View() = 0;
	virtual void SetFileNameStackTop(std::string const &value) = 0;
	virtual boost::filesystem::path const &WorkDirectory() const = 0;
	virtual CalculationMode StandardCalculationMode() const = 0;
	virtual void SetStandardCalculationMode(CalculationMode value) = 0;
	virtual CalculationMode UserStandardCalculationMode() const = 0;
	virtual void SetUserStandardCalculationMode(CalculationMode value) = 0;
};

extern Externals &g_externs;
