#pragma once

#include <boost/filesystem/path.hpp>

#include "Externals.h"
#include "FakeBrowseState.h"
#include "FakeZoomBox.h"
#include "NotImplementedException.h"

class FakeExternals : public Externals
{
public:
	FakeExternals() : _showFileCalled(false), _showFileFakeResult(SHOWFILE_PENDING),
		_calculationStatusCalled(false), _calculationStatusFakeResult(CALCSTAT_NO_FRACTAL),
		_initializeBatchCalled(false), _initializeBatchFakeResult(INITBATCH_NONE),
		_setZoomOffCalled(false), _setZoomOffLastValue(false),
		_evolvingFlagsCalled(false), _evolvingFlagsFakeResult(0),
		_escapeTimeCalled(false), _escapeTimeFakeResult(),
		_setSxMinCalled(false), _setSxMinLastValue(0.0),
		_setSxMaxCalled(false), _setSxMaxLastValue(0.0),
		_setSx3rdCalled(false), _setSx3rdLastValue(0.0),
		_setSyMinCalled(false), _setSyMinLastValue(0.0),
		_setSyMaxCalled(false), _setSyMaxLastValue(0.0),
		_setSy3rdCalled(false), _setSy3rdLastValue(0.0),
		_bfMathCalled(false), _bfMathFakeResult(0),
		_saveTimeCalled(false), _saveTimeFakeResult(0),
		_browseCalled(false), _browseFakeResult(),
		_setSaveTicksCalled(false), _setSaveTicksLastValue(0),
		_zoomCalled(false), _zoomFakeResult(),
		_setZWidthCalled(false), _setZWidthLastValue(0.0),
		_fractalTypeCalled(false), _fractalTypeFakeResult(0),
		_timedSaveCalled(false), _timedSaveFakeResult(TIMEDSAVE_DONE),
		_zWidthCalled(false), _zWidthFakeResult(0.0),
		_quickCalculateCalled(false), _quickCalculateFakeResult(false),
		_zoomOffCalled(false), _zoomOffFakeResult(false)
	{
	}
	virtual ~FakeExternals() { }

	virtual float AspectDrift() const				{ throw not_implemented("AspectDrift"); }
	virtual void SetAspectDrift(float value)		{ throw not_implemented("SetAspectDrift"); }
	virtual int AtanColors() const					{ throw not_implemented("AtanColors"); }
	virtual void SetAtanColors(int value)			{ throw not_implemented("SetAtanColors"); }
	virtual double AutoStereoWidth() const			{ throw not_implemented("AutoStereoWidth"); }
	virtual void SetAutoStereoWidth(double value)	{ throw not_implemented("SetAutoStereoWidth"); }
	virtual bool BadOutside() const					{ throw not_implemented("BadOutside"); }
	virtual void SetBadOutside(bool value)			{ throw not_implemented("SetBadOutside"); }
	virtual long BailOut() const					{ throw not_implemented("BailOut"); }
	virtual void SetBailOut(long value)				{ throw not_implemented("SetBailOut"); }
	virtual int BailOutFp()							{ throw not_implemented("BailOutFp"); }
	virtual void SetBailOutFp(BailOutFunction *value) { throw not_implemented("SetBailOutFp"); }
	virtual int BailOutL()							{ throw not_implemented("BailOutL"); }
	virtual void SetBailOutL(BailOutFunction *value) { throw not_implemented("SetBailOutL"); }
	virtual int BailOutBf()							{ throw not_implemented("BailOutBf"); }
	virtual void SetBailOutBf(BailOutFunction *value) { throw not_implemented("SetBailOutBf"); }
	virtual int BailOutBn()							{ throw not_implemented("BailOutBn"); }
	virtual void SetBailOutBn(BailOutFunction *value) { throw not_implemented("SetBailOutBn"); }
	virtual BailOutType BailOutTest() const			{ throw not_implemented("BailOutTest"); }
	virtual void SetBailOutTest(BailOutType value)	{ throw not_implemented("SetBailOutTest"); }
	virtual int Basin() const						{ throw not_implemented("Basin"); }
	virtual void SetBasin(int value)				{ throw not_implemented("SetBasin"); }
	virtual int BfSaveLen() const					{ throw not_implemented("BfSaveLen"); }
	virtual void SetBfSaveLen(int value)			{ throw not_implemented("SetBfSaveLen"); }
	virtual int BfDigits() const					{ throw not_implemented("BfDigits"); }
	virtual void SetBfDigits(int value)				{ throw not_implemented("SetBfDigits"); }
	virtual int Biomorph() const					{ throw not_implemented("Biomorph"); }
	virtual void SetBiomorph(int value)				{ throw not_implemented("SetBiomorph"); }
	virtual int BitShift() const					{ throw not_implemented("BitShift"); }
	virtual void SetBitShift(int value)				{ throw not_implemented("SetBitShift"); }
	virtual int BitShiftMinus1() const				{ throw not_implemented("BitShiftMinus1"); }
	virtual void SetBitShiftMinus1(int value)		{ throw not_implemented("SetBitShiftMinus1"); }
	virtual BYTE const *Block() const				{ throw not_implemented("Block"); }
	virtual void SetBlock(BYTE const *value)		{ throw not_implemented("SetBlock"); }
	virtual long CalculationTime() const			{ throw not_implemented("CalculationTime"); }
	virtual void SetCalculationTime(long value)		{ throw not_implemented("SetCalculationTime"); }
	virtual CalculateMandelbrotFunction *CalculateMandelbrotAsmFp() const { throw not_implemented("CalculateMandelbrotAsmFp"); }
	virtual void SetCalculateMandelbrotAsmFp(CalculateMandelbrotFunction *value) { throw not_implemented("SetCalculateMandelbrotAsmFp"); }
	virtual CalculateTypeFunction *CalculateType() const { throw not_implemented("CalculateType"); }
	virtual void SetCalculateType(CalculateTypeFunction *value)	{ throw not_implemented("SetCalculateType"); }
	virtual CalculationStatusType CalculationStatus() const { mutate()->_calculationStatusCalled = true; return _calculationStatusFakeResult; }
	bool CalculationStatusCalled() const			{ return _calculationStatusCalled; }
	virtual void SetCalculationStatusFakeResult(CalculationStatusType value) { _calculationStatusFakeResult = value; }
	virtual void SetCalculationStatus(CalculationStatusType value)	{ throw not_implemented("SetCalculationStatus"); }
	virtual int const *CfgLineNums() const	{ throw not_implemented("CfgLineNums"); }
	virtual void SetCfgLineNums(int const *value)	{ throw not_implemented("SetCfgLineNums"); }
	virtual bool CheckCurrentDir() const			{ throw not_implemented("CheckCurrentDir"); }
	virtual void SetCheckCurrentDir(bool value)		{ throw not_implemented("SetCheckCurrentDir"); }
	virtual long CImag() const						{ throw not_implemented("CImag"); }
	virtual void SetCImag(long value)				{ throw not_implemented("SetCImag"); }
	virtual double CloseEnough() const				{ throw not_implemented("CloseEnough"); }
	virtual void SetCloseEnough(double value)		{ throw not_implemented("SetCloseEnough"); }
	virtual double Proximity() const				{ throw not_implemented("Proximity"); }
	virtual void SetProximity(double value)			{ throw not_implemented("SetProximity"); }
	virtual ComplexD Coefficient() const			{ throw not_implemented("Coefficient"); }
	virtual void SetCoefficient(ComplexD value)		{ throw not_implemented("SetCoefficient"); }
	virtual int Col() const							{ throw not_implemented("Col"); }
	virtual void SetCol(int value)					{ throw not_implemented("SetCol"); }
	virtual int Color() const						{ throw not_implemented("Color"); }
	virtual void SetColor(int value)				{ throw not_implemented("SetColor"); }
	virtual long ColorIter() const					{ throw not_implemented("ColorIter"); }
	virtual void SetColorIter(long value)			{ throw not_implemented("SetColorIter"); }
	virtual bool ColorPreloaded() const				{ throw not_implemented("ColorPreloaded"); }
	virtual void SetColorPreloaded(bool value)		{ throw not_implemented("SetColorPreloaded"); }
	virtual int Colors() const						{ throw not_implemented("Colors"); }
	virtual void SetColors(int value)				{ throw not_implemented("SetColors"); }
	virtual bool CompareGif() const					{ throw not_implemented("CompareGif"); }
	virtual void SetCompareGif(bool value)			{ throw not_implemented("SetCompareGif"); }
	virtual long GaussianConstant() const			{ throw not_implemented("GaussianConstant"); }
	virtual void SetGaussianConstant(long value)	{ throw not_implemented("SetGaussianConstant"); }
	virtual double CosX() const						{ throw not_implemented("CosX"); }
	virtual void SetCosX(double value)				{ throw not_implemented("SetCosX"); }
	virtual long CReal() const						{ throw not_implemented("CReal"); }
	virtual void SetCReal(long value)				{ throw not_implemented("SetCReal"); }
	virtual int CurrentCol() const					{ throw not_implemented("CurrentCol"); }
	virtual void SetCurrentCol(int value)			{ throw not_implemented("SetCurrentCol"); }
	virtual int CurrentPass() const					{ throw not_implemented("CurrentPass"); }
	virtual void SetCurrentPass(int value)			{ throw not_implemented("SetCurrentPass"); }
	virtual int CurrentRow() const					{ throw not_implemented("CurrentRow"); }
	virtual void SetCurrentRow(int value)			{ throw not_implemented("SetCurrentRow"); }
	virtual int CycleLimit() const					{ throw not_implemented("CycleLimit"); }
	virtual void SetCycleLimit(int value)			{ throw not_implemented("SetCycleLimit"); }
	virtual int CExp() const						{ throw not_implemented("CExp"); }
	virtual void SetCExp(int value)					{ throw not_implemented("SetCExp"); }
	virtual double DeltaMinFp() const				{ throw not_implemented("DeltaMinFp"); }
	virtual void SetDeltaMinFp(double value)		{ throw not_implemented("SetDeltaMinFp"); }
	virtual int DebugMode() const					{ throw not_implemented("DebugMode"); }
	virtual void SetDebugMode(int value)			{ throw not_implemented("SetDebugMode"); }
	virtual int Decimals() const					{ throw not_implemented("Decimals"); }
	virtual void SetDecimals(int value)				{ throw not_implemented("SetDecimals"); }
	virtual BYTE const *DecoderLine() const			{ throw not_implemented("DecoderLine"); }
	virtual void SetDecoderLine(BYTE const *value)	{ throw not_implemented("SetDecoderLine"); }
	virtual int const *Decomposition() const		{ throw not_implemented("Decomposition"); }
	virtual void SetDecomposition(int const *value)	{ throw not_implemented("SetDecomposition"); }
	virtual int Degree() const						{ throw not_implemented("Degree"); }
	virtual void SetDegree(int value)				{ throw not_implemented("SetDegree"); }
	virtual long DeltaMin() const					{ throw not_implemented("DeltaMin"); }
	virtual void SetDeltaMin(long value)			{ throw not_implemented("SetDeltaMin"); }
	virtual float DepthFp() const					{ throw not_implemented("DepthFp"); }
	virtual void SetDepthFp(float value)			{ throw not_implemented("SetDepthFp"); }
	virtual bool Disk16Bit() const					{ throw not_implemented("Disk16Bit"); }
	virtual void SetDisk16Bit(bool value)			{ throw not_implemented("SetDisk16Bit"); }
	virtual bool DiskFlag() const					{ throw not_implemented("DiskFlag"); }
	virtual void SetDiskFlag(bool value)			{ throw not_implemented("SetDiskFlag"); }
	virtual bool DiskTarga() const					{ throw not_implemented("DiskTarga"); }
	virtual void SetDiskTarga(bool value)			{ throw not_implemented("SetDiskTarga"); }
	virtual Display3DType Display3D() const			{ throw not_implemented("Display3D"); }
	virtual void SetDisplay3D(Display3DType value)	{ throw not_implemented("SetDisplay3D"); }
	virtual long DistanceTest() const				{ throw not_implemented("DistanceTest"); }
	virtual void SetDistanceTest(long value)		{ throw not_implemented("SetDistanceTest"); }
	virtual int DistanceTestWidth() const			{ throw not_implemented("DistanceTestWidth"); }
	virtual void SetDistanceTestWidth(int value)	{ throw not_implemented("SetDistanceTestWidth"); }
	virtual float ScreenDistanceFp() const			{ throw not_implemented("ScreenDistanceFp"); }
	virtual void SetScreenDistanceFp(float value)	{ throw not_implemented("SetScreenDistanceFp"); }
	virtual int GaussianDistribution() const		{ throw not_implemented("GaussianDistribution"); }
	virtual void SetGaussianDistribution(int value) { throw not_implemented("SetGaussianDistribution"); }
	virtual bool DitherFlag() const					{ throw not_implemented("DitherFlag"); }
	virtual void SetDitherFlag(bool value)			{ throw not_implemented("SetDitherFlag"); }
	virtual bool DontReadColor() const				{ throw not_implemented("DontReadColor"); }
	virtual void SetDontReadColor(bool value)		{ throw not_implemented("SetDontReadColor"); }
	virtual double DeltaParameterImageX() const		{ throw not_implemented("DeltaParameterImageX"); }
	virtual void SetDeltaParameterImageX(double value) { throw not_implemented("SetDeltaParameterImageX"); }
	virtual double DeltaParameterImageY() const		{ throw not_implemented("DeltaParameterImageY"); }
	virtual void SetDeltaParameterImageY(double value) { throw not_implemented("SetDeltaParameterImageY"); }
	virtual BYTE const *Stack() const				{ throw not_implemented("Stack"); }
	virtual void SetStack(BYTE const *value)		{ throw not_implemented("SetStack"); }
	virtual DxPixelFunction *DxPixel() const		{ throw not_implemented("DxPixel"); }
	virtual void SetDxPixel(DxPixelFunction *value) { throw not_implemented("SetDxPixel"); }
	virtual double DxSize() const					{ throw not_implemented("DxSize"); }
	virtual void SetDxSize(double value)			{ throw not_implemented("SetDxSize"); }
	virtual DyPixelFunction *DyPixel() const		{ throw not_implemented("DyPixel"); }
	virtual void SetDyPixel(DyPixelFunction *value) { throw not_implemented("SetDyPixel"); }
	virtual double DySize() const					{ throw not_implemented("DySize"); }
	virtual void SetDySize(double value)			{ throw not_implemented("SetDySize"); }
	virtual bool EscapeExitFlag() const				{ throw not_implemented("EscapeExitFlag"); }
	virtual void SetEscapeExitFlag(bool value)		{ throw not_implemented("SetEscapeExitFlag"); }
	virtual int EvolvingFlags() const				{ mutate()->_evolvingFlagsCalled = true; return _evolvingFlagsFakeResult; }
	bool EvolvingFlagsCalled() const				{ return _evolvingFlagsCalled; }
	void SetEvolvingFlagsFakeResult(int value)		{ _evolvingFlagsFakeResult = value; }
	virtual void SetEvolvingFlags(int value)		{ throw not_implemented("SetEvolvingFlags"); }
	virtual evolution_info const *Evolve() const	{ throw not_implemented("Evolve"); }
	virtual evolution_info *Evolve()				{ throw not_implemented("Evolve"); }
	virtual void SetEvolve(evolution_info *value)	{ throw not_implemented("SetEvolve"); }
	virtual float EyesFp() const					{ throw not_implemented("EyesFp"); }
	virtual void SetEyesFp(float value)				{ throw not_implemented("SetEyesFp"); }
	virtual bool FastRestore() const				{ throw not_implemented("FastRestore"); }
	virtual void SetFastRestore(bool value)			{ throw not_implemented("SetFastRestore"); }
	virtual double FudgeLimit() const				{ throw not_implemented("FudgeLimit"); }
	virtual void SetFudgeLimit(double value)		{ throw not_implemented("SetFudgeLimit"); }
	virtual long OneFudge() const					{ throw not_implemented("OneFudge"); }
	virtual void SetOneFudge(long value)			{ throw not_implemented("SetOneFudge"); }
	virtual long TwoFudge() const					{ throw not_implemented("TwoFudge"); }
	virtual void SetTwoFudge(long value)			{ throw not_implemented("SetTwoFudge"); }
	virtual int GridSize() const					{ throw not_implemented("GridSize"); }
	virtual void SetGridSize(int value) 			{ throw not_implemented("SetGridSize"); }
	virtual double FiddleFactor() const 			{ throw not_implemented("FiddleFactor"); }
	virtual void SetFiddleFactor(double value)		{ throw not_implemented("SetFiddleFactor"); }
	virtual double FiddleReduction() const			{ throw not_implemented("FiddleReduction"); }
	virtual void SetFiddleReduction(double value)	{ throw not_implemented("SetFiddleReduction"); }
	virtual float FileAspectRatio() const			{ throw not_implemented("FileAspectRatio"); }
	virtual void SetFileAspectRatio(float value)	{ throw not_implemented("SetFileAspectRatio"); }
	virtual int FileColors() const					{ throw not_implemented("FileColors"); }
	virtual void SetFileColors(int value)			{ throw not_implemented("SetFileColors"); }
	virtual int FileXDots() const					{ throw not_implemented("FileXDots"); }
	virtual void SetFileXDots(int value)			{ throw not_implemented("SetFileXDots"); }
	virtual int FileYDots() const					{ throw not_implemented("FileYDots"); }
	virtual void SetFileYDots(int value)			{ throw not_implemented("SetFileYDots"); }
	virtual int FillColor() const 					{ throw not_implemented("FillColor"); }
	virtual void SetFillColor(int value)			{ throw not_implemented("SetFillColor"); }
	virtual int FinishRow() const 					{ throw not_implemented("FinishRow"); }
	virtual void SetFinishRow(int value) 			{ throw not_implemented("SetFinishRow"); }
	virtual bool CommandInitialize() const 			{ throw not_implemented("CommandInitialize"); }
	virtual void SetCommandInitialize(bool value)	{ throw not_implemented("SetCommandInitialize"); }
	virtual int FirstSavedAnd() const 				{ throw not_implemented("FirstSavedAnd"); }
	virtual void SetFirstSavedAnd(int value)		{ throw not_implemented("SetFirstSavedAnd"); }
	virtual bool FloatFlag() const 					{ throw not_implemented("FloatFlag"); }
	virtual void SetFloatFlag(bool value) 			{ throw not_implemented("SetFloatFlag"); }
	virtual ComplexD const *FloatParameter() const	{ throw not_implemented("FloatParameter"); }
	virtual void SetFloatParameter(ComplexD const *value) { throw not_implemented("SetFloatParameter"); }
	virtual BYTE const *Font8x8() const 			{ throw not_implemented("Font8x8"); }
	virtual int ForceSymmetry() const 				{ throw not_implemented("ForceSymmetry"); }
	virtual void SetForceSymmetry(int value)		{ throw not_implemented("SetForceSymmetry"); }
	virtual int FractalType() const 				{ mutate()->_fractalTypeCalled = true; return _fractalTypeFakeResult; }
	bool FractalTypeCalled() const					{ return _fractalTypeCalled; }
	void SetFractalTypeFakeResult(int value)		{ _fractalTypeFakeResult = value; }
	virtual void SetFractalType(int value) 			{ throw not_implemented("SetFractalType"); }
	virtual bool FromTextFlag() const 				{ throw not_implemented("FromTextFlag"); }
	virtual void SetFromTextFlag(bool value)		{ throw not_implemented("SetFromTextFlag"); }
	virtual long Fudge() const						{ throw not_implemented("Fudge"); }
	virtual void SetFudge(long value) 				{ throw not_implemented("SetFudge"); }
	virtual FunctionListItem const *FunctionList() const { throw not_implemented("FunctionList"); }
	virtual void SetFunctionList(FunctionListItem const *value) { throw not_implemented("SetFunctionList"); }
	virtual bool FunctionPreloaded() const			{ throw not_implemented("FunctionPreloaded"); }
	virtual void SetFunctionPreloaded(bool value)	{ throw not_implemented("SetFunctionPreloaded"); }
	virtual double FRadius() const 					{ throw not_implemented("FRadius"); }
	virtual void SetFRadius(double value) 			{ throw not_implemented("SetFRadius"); }
	virtual double FXCenter() const 				{ throw not_implemented("FXCenter"); }
	virtual void SetFXCenter(double value)			{ throw not_implemented("SetFXCenter"); }
	virtual double FYCenter() const 				{ throw not_implemented("FYCenter"); }
	virtual void SetFYCenter(double value) 			{ throw not_implemented("SetFYCenter"); }
	virtual GENEBASE const *Genes() const 			{ throw not_implemented("Genes"); }
	virtual GENEBASE *Genes()						{ throw not_implemented("Genes"); }
	virtual bool Gif87aFlag() const 				{ throw not_implemented("Gif87aFlag"); }
	virtual void SetGif87aFlag(bool value) 			{ throw not_implemented("SetGif87aFlag"); }
	virtual TabStatusType TabStatus() const 		{ throw not_implemented("TabStatus"); }
	virtual void SetTabStatus(TabStatusType value) 	{ throw not_implemented("SetTabStatus"); }
	virtual bool GrayscaleDepth() const 			{ throw not_implemented("GrayscaleDepth"); }
	virtual void SetGrayscaleDepth(bool value)		{ throw not_implemented("SetGrayscaleDepth"); }
	virtual bool HasInverse() const 				{ throw not_implemented("HasInverse"); }
	virtual void SetHasInverse(bool value) 			{ throw not_implemented("SetHasInverse"); }
	virtual unsigned int Height() const 			{ throw not_implemented("Height"); }
	virtual void SetHeight(int value) 				{ throw not_implemented("SetHeight"); }
	virtual float HeightFp() const 					{ throw not_implemented("HeightFp"); }
	virtual void SetHeightFp(float value) 			{ throw not_implemented("SetHeightFp"); }
	virtual float const *IfsDefinition() const		{ throw not_implemented("IfsDefinition"); }
	virtual void SetIfsDefinition(float const *value) { throw not_implemented("SetIfsDefinition"); }
	virtual int IfsType() const 					{ throw not_implemented("IfsType"); }
	virtual void SetIfsType(int value) 				{ throw not_implemented("SetIfsType"); }
	virtual ComplexD InitialZ() const 				{ throw not_implemented("InitialZ"); }
	virtual void SetInitialZ(ComplexD value)		{ throw not_implemented("SetInitialZ"); }
	virtual InitializeBatchType InitializeBatch() const { mutate()->_initializeBatchCalled = true; return _initializeBatchFakeResult; }
	bool InitializeBatchCalled() const				{ return _initializeBatchCalled; }
	void SetInitializeBatchFakeResult(InitializeBatchType value) { _initializeBatchFakeResult = value; }
	virtual void SetInitializeBatch(InitializeBatchType value) { throw not_implemented("SetInitializeBatch"); }
	virtual int InitialCycleLimit() const 			{ throw not_implemented("InitialCycleLimit"); }
	virtual void SetInitialCycleLimit(int value)	{ throw not_implemented("SetInitialCycleLimit"); }
	virtual ComplexD InitialOrbitZ() const 			{ throw not_implemented("InitialOrbitZ"); }
	virtual void SetInitialOrbitZ(ComplexD value)	{ throw not_implemented("SetInitialOrbitZ"); }
	virtual int SaveTime() const 					{ mutate()->_saveTimeCalled = true; return _saveTimeFakeResult; }
	bool SaveTimeCalled() const						{ return _saveTimeCalled; }
	void SetSaveTimeFakeResult(int value)			{ _saveTimeFakeResult = value; }
	virtual void SetSaveTime(int value) 			{ throw not_implemented("SetSaveTime"); }
	virtual int Inside() const 						{ throw not_implemented("Inside"); }
	virtual void SetInside(int value) 				{ throw not_implemented("SetInside"); }
	virtual int IntegerFractal() const 				{ throw not_implemented("IntegerFractal"); }
	virtual void SetIntegerFractal(int value)		{ throw not_implemented("SetIntegerFractal"); }
	virtual double const *Inversion() const			{ throw not_implemented("Inversion"); }
	virtual void SetInversion(double const *value)	{ throw not_implemented("SetInversion"); }
	virtual int Invert() const 						{ throw not_implemented("Invert"); }
	virtual void SetInvert(int value) 				{ throw not_implemented("SetInvert"); }
	virtual int IsTrueColor() const 				{ throw not_implemented("IsTrueColor"); }
	virtual void SetIsTrueColor(int value) 			{ throw not_implemented("SetIsTrueColor"); }
	virtual bool IsMandelbrot() const 				{ throw not_implemented("IsMandelbrot"); }
	virtual void SetIsMandelbrot(bool value) 		{ throw not_implemented("SetIsMandelbrot"); }
	virtual int XStop() const 						{ throw not_implemented("XStop"); }
	virtual void SetXStop(int value) 				{ throw not_implemented("SetXStop"); }
	virtual int YStop() const 						{ throw not_implemented("YStop"); }
	virtual void SetYStop(int value) 				{ throw not_implemented("SetYStop"); }
	virtual std::complex<double> JuliaC() const 	{ throw not_implemented("JuliaC"); }
	virtual void SetJuliaC(std::complex<double> const &value) { throw not_implemented("SetJuliaC"); }
	virtual int Juli3DMode() const					{ throw not_implemented("Juli3DMode"); }
	virtual void SetJuli3DMode(int value) 			{ throw not_implemented("SetJuli3DMode"); }
	virtual char const **Juli3DOptions() const		{ throw not_implemented("Juli3DOptions"); }
	virtual bool Julibrot() const 					{ throw not_implemented("Julibrot"); }
	virtual void SetJulibrot(bool value) 			{ throw not_implemented("SetJulibrot"); }
	virtual int InputCounter() const 				{ throw not_implemented("InputCounter"); }
	virtual void SetInputCounter(int value) 		{ throw not_implemented("SetInputCounter"); }
	virtual bool KeepScreenCoords() const 			{ throw not_implemented("KeepScreenCoords"); }
	virtual void SetKeepScreenCoords(bool value)	{ throw not_implemented("SetKeepScreenCoords"); }
	virtual long CloseEnoughL() const 				{ throw not_implemented("CloseEnoughL"); }
	virtual void SetCloseEnoughL(long value)		{ throw not_implemented("SetCloseEnoughL"); }
	virtual ComplexL CoefficientL() const 			{ throw not_implemented("CoefficientL"); }
	virtual void SetCoefficientL(ComplexL value)	{ throw not_implemented("SetCoefficientL"); }
	virtual bool UseOldComplexPower() const 		{ throw not_implemented("UseOldComplexPower"); }
	virtual void SetUseOldComplexPower(bool value)	{ throw not_implemented("SetUseOldComplexPower"); }
	virtual BYTE const *LineBuffer() const 			{ throw not_implemented("LineBuffer"); }
	virtual void SetLineBuffer(BYTE const *value)	{ throw not_implemented("SetLineBuffer"); }
	virtual ComplexL InitialZL() const 				{ throw not_implemented("InitialZL"); }
	virtual void SetInitialZL(ComplexL value) 		{ throw not_implemented("SetInitialZL"); }
	virtual ComplexL InitialOrbitL() const 			{ throw not_implemented("InitialOrbitL"); }
	virtual void SetInitialOrbitL(ComplexL value)	{ throw not_implemented("SetInitialOrbitL"); }
	virtual long InitialXL() const 					{ throw not_implemented("InitialXL"); }
	virtual void SetInitialXL(long value) 			{ throw not_implemented("SetInitialXL"); }
	virtual long InitialYL() const 					{ throw not_implemented("InitialYL"); }
	virtual void SetInitialYL(long value) 			{ throw not_implemented("SetInitialYL"); }
	virtual long RqLimit2L() const 					{ throw not_implemented("RqLimit2L"); }
	virtual void SetRqLimit2L(long value) 			{ throw not_implemented("SetRqLimit2L"); }
	virtual long RqLimitL() const 					{ throw not_implemented("RqLimitL"); }
	virtual void SetRqLimitL(long value) 			{ throw not_implemented("SetRqLimitL"); }
	virtual long MagnitudeL() const 				{ throw not_implemented("MagnitudeL"); }
	virtual void SetMagnitudeL(long value) 			{ throw not_implemented("SetMagnitudeL"); }
	virtual ComplexL NewZL() const 					{ throw not_implemented("NewZL"); }
	virtual void SetNewZL(ComplexL value) 			{ throw not_implemented("SetNewZL"); }
	virtual bool Loaded3D() const 					{ throw not_implemented("Loaded3D"); }
	virtual void SetLoaded3D(bool value) 			{ throw not_implemented("SetLoaded3D"); }
	virtual bool LogAutomaticFlag() const 			{ throw not_implemented("LogAutomaticFlag"); }
	virtual void SetLogAutomaticFlag(bool value)	{ throw not_implemented("SetLogAutomaticFlag"); }
	virtual bool LogCalculation() const 			{ throw not_implemented("LogCalculation"); }
	virtual void SetLogCalculation(bool value)		{ throw not_implemented("SetLogCalculation"); }
	virtual int LogDynamicCalculate() const			{ throw not_implemented("LogDynamicCalculate"); }
	virtual void SetLogDynamicCalculate(int value)	{ throw not_implemented("SetLogDynamicCalculate"); }
	virtual long LogPaletteMode() const 			{ throw not_implemented("LogPaletteMode"); }
	virtual void SetLogPaletteMode(long value)		{ throw not_implemented("SetLogPaletteMode"); }
	virtual BYTE const *LogTable() const 			{ throw not_implemented("LogTable"); }
	virtual void SetLogTable(BYTE const *value)		{ throw not_implemented("SetLogTable"); }
	virtual ComplexL OldZL() const 					{ throw not_implemented("OldZL"); }
	virtual void SetOldZL(ComplexL value) 			{ throw not_implemented("SetOldZL"); }
	virtual ComplexL const *LongParameter() const	{ throw not_implemented("LongParameter"); }
	virtual void SetLongParameter(ComplexL const *value) { throw not_implemented("SetLongParameter"); }
	virtual ComplexL Parameter2L() const 			{ throw not_implemented("Parameter2L"); }
	virtual void SetParameter2L(ComplexL value)		{ throw not_implemented("SetParameter2L"); }
	virtual ComplexL ParameterL() const 			{ throw not_implemented("ParameterL"); }
	virtual void SetParameterL(ComplexL value)		{ throw not_implemented("SetParameterL"); }
	virtual long TempSqrXL() const 					{ throw not_implemented("TempSqrXL"); }
	virtual void SetTempSqrXL(long value) 			{ throw not_implemented("SetTempSqrXL"); }
	virtual long TempSqrYL() const					{ throw not_implemented("TempSqrYL"); }
	virtual void SetTempSqrYL(long value) 			{ throw not_implemented("SetTempSqrYL"); }
	virtual ComplexL TmpZL() const					{ throw not_implemented("TmpZL"); }
	virtual void SetTmpZL(ComplexL value) 			{ throw not_implemented("SetTmpZL"); }
	virtual PixelFunction *LxPixel() const 			{ throw not_implemented("LxPixel"); }
	virtual void SetLxPixel(PixelFunction *value)	{ throw not_implemented("SetLxPixel"); }
	virtual PixelFunction *LyPixel() const 			{ throw not_implemented("LyPixel"); }
	virtual void SetLyPixel(PixelFunction *value)	{ throw not_implemented("SetLyPixel"); }
	virtual TrigFunction *Trig0L() const 			{ throw not_implemented("Trig0L"); }
	virtual void SetTrig0L(TrigFunction *value)		{ throw not_implemented("SetTrig0L"); }
	virtual TrigFunction *Trig1L() const 			{ throw not_implemented("Trig1L"); }
	virtual void SetTrig1L(TrigFunction *value)		{ throw not_implemented("SetTrig1L"); }
	virtual TrigFunction *Trig2L() const 			{ throw not_implemented("Trig2L"); }
	virtual void SetTrig2L(TrigFunction *value) 	{ throw not_implemented("SetTrig2L"); }
	virtual TrigFunction *Trig3L() const 			{ throw not_implemented("Trig3L"); }
	virtual void SetTrig3L(TrigFunction *value)		{ throw not_implemented("SetTrig3L"); }
	virtual TrigFunction *Trig0D() const 			{ throw not_implemented("Trig0D"); }
	virtual void SetTrig0D(TrigFunction *value) 	{ throw not_implemented("SetTrig0D"); }
	virtual TrigFunction *Trig1D() const 			{ throw not_implemented("Trig1D"); }
	virtual void SetTrig1D(TrigFunction *value)		{ throw not_implemented("SetTrig1D"); }
	virtual TrigFunction *Trig2D() const 			{ throw not_implemented("Trig2D"); }
	virtual void SetTrig2D(TrigFunction *value)		{ throw not_implemented("SetTrig2D"); }
	virtual TrigFunction *Trig3D() const 			{ throw not_implemented("Trig3D"); }
	virtual void SetTrig3D(TrigFunction *value)		{ throw not_implemented("SetTrig3D"); }
	virtual double Magnitude() const 				{ throw not_implemented("Magnitude"); }
	virtual void SetMagnitude(double value) 		{ throw not_implemented("SetMagnitude"); }
	virtual unsigned long MagnitudeLimit() const	{ throw not_implemented("MagnitudeLimit"); }
	virtual void SetMagnitudeLimit(long value) 		{ throw not_implemented("SetMagnitudeLimit"); }
	virtual MajorMethodType MajorMethod() const		{ throw not_implemented("MajorMethod"); }
	virtual void SetMajorMethod(MajorMethodType value) { throw not_implemented("SetMajorMethod"); }
	virtual int MathErrorCount() const 				{ throw not_implemented("MathErrorCount"); }
	virtual void SetMathErrorCount(int value) 		{ throw not_implemented("SetMathErrorCount"); }
	virtual double const *MathTolerance() const		{ throw not_implemented("MathTolerance"); }
	virtual void SetMathTolerance(double const *value) { throw not_implemented("SetMathTolerance"); }
	virtual long MaxCount() const 					{ throw not_implemented("MaxCount"); }
	virtual void SetMaxCount(long value) 			{ throw not_implemented("SetMaxCount"); }
	virtual long MaxIteration() const 				{ throw not_implemented("MaxIteration"); }
	virtual void SetMaxIteration(long value) 		{ throw not_implemented("SetMaxIteration"); }
	virtual int MaxLineLength() const 				{ throw not_implemented("MaxLineLength"); }
	virtual void SetMaxLineLength(int value)		{ throw not_implemented("SetMaxLineLength"); }
	virtual long MaxLogTableSize() const 			{ throw not_implemented("MaxLogTableSize"); }
	virtual void SetMaxLogTableSize(long value)		{ throw not_implemented("SetMaxLogTableSize"); }
	virtual long BnMaxStack() const 				{ throw not_implemented("BnMaxStack"); }
	virtual void SetBnMaxStack(long value) 			{ throw not_implemented("SetBnMaxStack"); }
	virtual int MaxColors() const 					{ throw not_implemented("MaxColors"); }
	virtual void SetMaxColors(int value) 			{ throw not_implemented("SetMaxColors"); }
	virtual int MaxInputCounter() const 			{ throw not_implemented("MaxInputCounter"); }
	virtual void SetMaxInputCounter(int value)		{ throw not_implemented("SetMaxInputCounter"); }
	virtual int MaxHistory() const 					{ throw not_implemented("MaxHistory"); }
	virtual void SetMaxHistory(int value)			{ throw not_implemented("SetMaxHistory"); }
	virtual MinorMethodType MinorMethod() const		{ throw not_implemented("MinorMethod"); }
	virtual void SetMinorMethod(MinorMethodType value) { throw not_implemented("SetMinorMethod"); }
	virtual more_parameters const *MoreParameters() const { throw not_implemented("MoreParameters"); }
	virtual void SetMoreParameters(more_parameters const *value) { throw not_implemented("SetMoreParameters"); }
	virtual int OverflowMp() const 					{ throw not_implemented("OverflowMp"); }
	virtual void SetOverflowMp(int value) 			{ throw not_implemented("SetOverflowMp"); }
	virtual double MXMaxFp() const 					{ throw not_implemented("MXMaxFp"); }
	virtual void SetMXMaxFp(double value) 			{ throw not_implemented("SetMXMaxFp"); }
	virtual double MXMinFp() const 					{ throw not_implemented("MXMinFp"); }
	virtual void SetMXMinFp(double value) 			{ throw not_implemented("SetMXMinFp"); }
	virtual double MYMaxFp() const 					{ throw not_implemented("MYMaxFp"); }
	virtual void SetMYMaxFp(double value) 			{ throw not_implemented("SetMYMaxFp"); }
	virtual double MYMinFp() const 					{ throw not_implemented("MYMinFp"); }
	virtual void SetMYMinFp(double value) 			{ throw not_implemented("SetMYMinFp"); }
	virtual int NameStackPtr() const 				{ throw not_implemented("NameStackPtr"); }
	virtual void SetNameStackPtr(int value)			{ throw not_implemented("SetNameStackPtr"); }
	virtual ComplexD NewZ() const 					{ throw not_implemented("NewZ"); }
	virtual void SetNewZ(ComplexD value) 			{ throw not_implemented("SetNewZ"); }
	virtual int NewDiscreteParameterOffsetX() const { throw not_implemented("NewDiscreteParameterOffsetX"); }
	virtual void SetNewDiscreteParameterOffsetX(int value) { throw not_implemented("SetNewDiscreteParameterOffsetX"); }
	virtual int NewDiscreteParameterOffsetY() const { throw not_implemented("NewDiscreteParameterOffsetY"); }
	virtual void SetNewDiscreteParameterOffsetY(int value) { throw not_implemented("SetNewDiscreteParameterOffsetY"); }
	virtual double NewParameterOffsetX() const 		{ throw not_implemented("NewParameterOffsetX"); }
	virtual void SetNewParameterOffsetX(double value) { throw not_implemented("SetNewParameterOffsetX"); }
	virtual double NewParameterOffsetY() const		{ throw not_implemented("NewParameterOffsetY"); }
	virtual void SetNewParameterOffsetY(double value) { throw not_implemented("SetNewParameterOffsetY"); }
	virtual int NewOrbitType() const 				{ throw not_implemented("NewOrbitType"); }
	virtual void SetNewOrbitType(int value)			{ throw not_implemented("SetNewOrbitType"); }
	virtual int NextSavedIncr() const 				{ throw not_implemented("NextSavedIncr"); }
	virtual void SetNextSavedIncr(int value) 		{ throw not_implemented("SetNextSavedIncr"); }
	virtual bool NoMagnitudeCalculation() const		{ throw not_implemented("NoMagnitudeCalculation"); }
	virtual void SetNoMagnitudeCalculation(bool value) { throw not_implemented("SetNoMagnitudeCalculation"); }
	virtual int NumAffine() const 					{ throw not_implemented("NumAffine"); }
	virtual void SetNumAffine(int value) 			{ throw not_implemented("SetNumAffine"); }
	virtual const int NumFunctionList() const 		{ throw not_implemented("NumFunctionList"); }
	virtual int NumFractalTypes() const 			{ throw not_implemented("NumFractalTypes"); }
	virtual void SetNumFractalTypes(int value)		{ throw not_implemented("SetNumFractalTypes"); }
	virtual bool NextScreenFlag() const 			{ throw not_implemented("NextScreenFlag"); }
	virtual void SetNextScreenFlag(bool value)		{ throw not_implemented("SetNextScreenFlag"); }
	virtual int GaussianOffset() const 				{ throw not_implemented("GaussianOffset"); }
	virtual void SetGaussianOffset(int value) 		{ throw not_implemented("SetGaussianOffset"); }
	virtual bool OkToPrint() const 					{ throw not_implemented("OkToPrint"); }
	virtual void SetOkToPrint(bool value)			{ throw not_implemented("SetOkToPrint"); }
	virtual ComplexD OldZ() const					{ throw not_implemented("OldZ"); }
	virtual void SetOldZ(ComplexD value)			{ throw not_implemented("SetOldZ"); }
	virtual long OldColorIter() const 				{ throw not_implemented("OldColorIter"); }
	virtual void SetOldColorIter(long value)		{ throw not_implemented("SetOldColorIter"); }
	virtual bool OldDemmColors() const 				{ throw not_implemented("OldDemmColors"); }
	virtual void SetOldDemmColors(bool value)		{ throw not_implemented("SetOldDemmColors"); }
	virtual int DiscreteParameterOffsetX() const	{ throw not_implemented("DiscreteParameterOffsetX"); }
	virtual void SetDiscreteParameterOffsetX(int value) { throw not_implemented("SetDiscreteParameterOffsetX"); }
	virtual int DiscreteParameterOffsetY() const	{ throw not_implemented("DiscreteParameterOffsetY"); }
	virtual void SetDiscreteParameterOffsetY(int value) { throw not_implemented("SetDiscreteParameterOffsetY"); }
	virtual double ParameterOffsetX() const 		{ throw not_implemented("ParameterOffsetX"); }
	virtual void SetParameterOffsetX(double value)	{ throw not_implemented("SetParameterOffsetX"); }
	virtual double ParameterOffsetY() const 		{ throw not_implemented("ParameterOffsetY"); }
	virtual void SetParameterOffsetY(double value)	{ throw not_implemented("SetParameterOffsetY"); }
	virtual int OrbitSave() const 					{ throw not_implemented("OrbitSave"); }
	virtual void SetOrbitSave(int value) 			{ throw not_implemented("SetOrbitSave"); }
	virtual int OrbitColor() const 					{ throw not_implemented("OrbitColor"); }
	virtual void SetOrbitColor(int value) 			{ throw not_implemented("SetOrbitColor"); }
	virtual int OrbitDelay() const 					{ throw not_implemented("OrbitDelay"); }
	virtual void SetOrbitDelay(int value) 			{ throw not_implemented("SetOrbitDelay"); }
	virtual int OrbitDrawMode() const 				{ throw not_implemented("OrbitDrawMode"); }
	virtual void SetOrbitDrawMode(int value) 		{ throw not_implemented("SetOrbitDrawMode"); }
	virtual long OrbitInterval() const 				{ throw not_implemented("OrbitInterval"); }
	virtual void SetOrbitInterval(long value)		{ throw not_implemented("SetOrbitInterval"); }
	virtual int OrbitIndex() const 					{ throw not_implemented("OrbitIndex"); }
	virtual void SetOrbitIndex(int value) 			{ throw not_implemented("SetOrbitIndex"); }
	virtual bool OrganizeFormulaSearch() const		{ throw not_implemented("OrganizeFormulaSearch"); }
	virtual void SetOrganizeFormulaSearch(bool value) { throw not_implemented("SetOrganizeFormulaSearch"); }
	virtual float OriginFp() const 					{ throw not_implemented("OriginFp"); }
	virtual void SetOriginFp(float value) 			{ throw not_implemented("SetOriginFp"); }
	virtual OutLineFunction *OutLine() const		{ throw not_implemented("OutLine"); }
	virtual void SetOutLine(OutLineFunction *value) { throw not_implemented("SetOutLine"); }
	virtual OutLineCleanupFunction *OutLineCleanup() const { throw not_implemented("OutLineCleanup"); }
	virtual void SetOutLineCleanup(OutLineCleanupFunction *value) { throw not_implemented("SetOutLineCleanup"); }
	virtual int Outside() const 					{ throw not_implemented("Outside"); }
	virtual void SetOutside(int value) 				{ throw not_implemented("SetOutside"); }
	virtual bool Overflow() const 					{ throw not_implemented("Overflow"); }
	virtual void SetOverflow(bool value) 			{ throw not_implemented("SetOverflow"); }
	virtual bool Overlay3D() const 					{ throw not_implemented("Overlay3D"); }
	virtual void SetOverlay3D(bool value) 			{ throw not_implemented("SetOverlay3D"); }
	virtual bool FractalOverwrite() const 			{ throw not_implemented("FractalOverwrite"); }
	virtual void SetFractalOverwrite(bool value)	{ throw not_implemented("SetFractalOverwrite"); }
	virtual double OrbitX3rd() const 				{ throw not_implemented("OrbitX3rd"); }
	virtual void SetOrbitX3rd(double value) 		{ throw not_implemented("SetOrbitX3rd"); }
	virtual double OrbitXMax() const 				{ throw not_implemented("OrbitXMax"); }
	virtual void SetOrbitXMax(double value)			{ throw not_implemented("SetOrbitXMax"); }
	virtual double OrbitXMin() const 				{ throw not_implemented("OrbitXMin"); }
	virtual void SetOrbitXMin(double value)			{ throw not_implemented("SetOrbitXMin"); }
	virtual double OrbitY3rd() const 				{ throw not_implemented("OrbitY3rd"); }
	virtual void SetOrbitY3rd(double value)			{ throw not_implemented("SetOrbitY3rd"); }
	virtual double OrbitYMax() const 				{ throw not_implemented("OrbitYMax"); }
	virtual void SetOrbitYMax(double value) 		{ throw not_implemented("SetOrbitYMax"); }
	virtual double OrbitYMin() const 				{ throw not_implemented("OrbitYMin"); }
	virtual void SetOrbitYMin(double value)			{ throw not_implemented("SetOrbitYMin"); }
	virtual double const *Parameters() const		{ throw not_implemented("Parameters"); }
	virtual void SetParameters(double const *value) { throw not_implemented("SetParameters"); }
	virtual double ParameterRangeX() const 			{ throw not_implemented("ParameterRangeX"); }
	virtual void SetParameterRangeX(double value)	{ throw not_implemented("SetParameterRangeX"); }
	virtual double ParameterRangeY() const 			{ throw not_implemented("ParameterRangeY"); }
	virtual void SetParameterRangeY(double value)	{ throw not_implemented("SetParameterRangeY"); }
	virtual double ParameterZoom() const 			{ throw not_implemented("ParameterZoom"); }
	virtual void SetParameterZoom(double value)		{ throw not_implemented("SetParameterZoom"); }
	virtual ComplexD Parameter2() const 			{ throw not_implemented("Parameter2"); }
	virtual void SetParameter2(ComplexD value)		{ throw not_implemented("SetParameter2"); }
	virtual ComplexD Parameter() const 				{ throw not_implemented("Parameter"); }
	virtual void SetParameter(ComplexD value)		{ throw not_implemented("SetParameter"); }
	virtual int Passes() const 						{ throw not_implemented("Passes"); }
	virtual void SetPasses(int value) 				{ throw not_implemented("SetPasses"); }
	virtual int PatchLevel() const 					{ throw not_implemented("PatchLevel"); }
	virtual void SetPatchLevel(int value) 			{ throw not_implemented("SetPatchLevel"); }
	virtual int PeriodicityCheck() const 			{ throw not_implemented("PeriodicityCheck"); }
	virtual void SetPeriodicityCheck(int value)		{ throw not_implemented("SetPeriodicityCheck"); }
	virtual PlotColorFunction *PlotColor() const	{ throw not_implemented("PlotColor"); }
	virtual void SetPlotColor(PlotColorFunction *value) { throw not_implemented("SetPlotColor"); }
	virtual double PlotMx1() const 					{ throw not_implemented("PlotMx1"); }
	virtual void SetPlotMx1(double value) 			{ throw not_implemented("SetPlotMx1"); }
	virtual double PlotMx2() const 					{ throw not_implemented("PlotMx2"); }
	virtual void SetPlotMx2(double value) 			{ throw not_implemented("SetPlotMx2"); }
	virtual double PlotMy1() const 					{ throw not_implemented("PlotMy1"); }
	virtual void SetPlotMy1(double value) 			{ throw not_implemented("SetPlotMy1"); }
	virtual double PlotMy2() const 					{ throw not_implemented("PlotMy2"); }
	virtual void SetPlotMy2(double value) 			{ throw not_implemented("SetPlotMy2"); }
	virtual bool Potential16Bit() const 			{ throw not_implemented("Potential16Bit"); }
	virtual void SetPotential16Bit(bool value) 		{ throw not_implemented("SetPotential16Bit"); }
	virtual bool PotentialFlag() const 				{ throw not_implemented("PotentialFlag"); }
	virtual void SetPotentialFlag(bool value) 		{ throw not_implemented("SetPotentialFlag"); }
	virtual double const *PotentialParameter() const { throw not_implemented("PotentialParameter"); }
	virtual void SetPotentialParameter(double const *value) { throw not_implemented("SetPotentialParameter"); }
	virtual int Px() const 							{ throw not_implemented("Px"); }
	virtual void SetPx(int value) 					{ throw not_implemented("SetPx"); }
	virtual int Py() const 							{ throw not_implemented("Py"); }
	virtual void SetPy(int value) 					{ throw not_implemented("SetPy"); }
	virtual int ParameterBoxCount() const 			{ throw not_implemented("ParameterBoxCount"); }
	virtual void SetParameterBoxCount(int value)	{ throw not_implemented("SetParameterBoxCount"); }
	virtual int PseudoX() const 					{ throw not_implemented("PseudoX"); }
	virtual void SetPseudoX(int value) 				{ throw not_implemented("SetPseudoX"); }
	virtual int PseudoY() const 					{ throw not_implemented("PseudoY"); }
	virtual void SetPseudoY(int value) 				{ throw not_implemented("SetPseudoY"); }
	virtual PlotColorPutColorFunction *PlotColorPutColor() const { throw not_implemented("PlotColorPutColor"); }
	virtual void SetPlotColorPutColor(PlotColorPutColorFunction *value) { throw not_implemented("SetPlotColorPutColor"); }
	virtual ComplexD Power() const 					{ throw not_implemented("Power"); }
	virtual void SetPower(ComplexD value) 			{ throw not_implemented("SetPower"); }
	virtual bool QuickCalculate() const 			{ mutate()->_quickCalculateCalled = true; return _quickCalculateFakeResult; }
	bool QuickCalculateCalled() const				{ return _quickCalculateCalled; }
	void SetQuickCalculateFakeResult(bool value)	{ _quickCalculateFakeResult = value; }
	virtual void SetQuickCalculate(bool value) 		{ throw not_implemented("SetQuickCalculate"); }
	virtual int const *Ranges() const 				{ throw not_implemented("Ranges"); }
	virtual void SetRanges(int const *value) 		{ throw not_implemented("SetRanges"); }
	virtual int RangesLength() const 				{ throw not_implemented("RangesLength"); }
	virtual void SetRangesLength(int value) 		{ throw not_implemented("SetRangesLength"); }
	virtual long RealColorIter() const 				{ throw not_implemented("RealColorIter"); }
	virtual void SetRealColorIter(long value) 		{ throw not_implemented("SetRealColorIter"); }
	virtual int Release() const 					{ throw not_implemented("Release"); }
	virtual void SetRelease(int value) 				{ throw not_implemented("SetRelease"); }
	virtual int ResaveMode() const 					{ throw not_implemented("ResaveMode"); }
	virtual void SetResaveMode(int value) 			{ throw not_implemented("SetResaveMode"); }
	virtual bool ResetPeriodicity() const 			{ throw not_implemented("ResetPeriodicity"); }
	virtual void SetResetPeriodicity(bool value)	{ throw not_implemented("SetResetPeriodicity"); }
	virtual char const *ResumeInfo() const 			{ throw not_implemented("ResumeInfo"); }
	virtual void SetResumeInfo(char const *value)	{ throw not_implemented("SetResumeInfo"); }
	virtual int ResumeLength() const 				{ throw not_implemented("ResumeLength"); }
	virtual void SetResumeLength(int value) 		{ throw not_implemented("SetResumeLength"); }
	virtual bool Resuming() const 					{ throw not_implemented("Resuming"); }
	virtual void SetResuming(bool value) 			{ throw not_implemented("SetResuming"); }
	virtual bool UseFixedRandomSeed() const 		{ throw not_implemented("UseFixedRandomSeed"); }
	virtual void SetUseFixedRandomSeed(bool value)	{ throw not_implemented("SetUseFixedRandomSeed"); }
	virtual char const *RleBuffer() const 			{ throw not_implemented("RleBuffer"); }
	virtual void SetRleBuffer(char const *value) 	{ throw not_implemented("SetRleBuffer"); }
	virtual int RotateHi() const 					{ throw not_implemented("RotateHi"); }
	virtual void SetRotateHi(int value) 			{ throw not_implemented("SetRotateHi"); }
	virtual int RotateLo() const 					{ throw not_implemented("RotateLo"); }
	virtual void SetRotateLo(int value) 			{ throw not_implemented("SetRotateLo"); }
	virtual int Row() const 						{ throw not_implemented("Row"); }
	virtual void SetRow(int value) 					{ throw not_implemented("SetRow"); }
	virtual int RowCount() const 					{ throw not_implemented("RowCount"); }
	virtual void SetRowCount(int value) 			{ throw not_implemented("SetRowCount"); }
	virtual double RqLimit2() const 				{ throw not_implemented("RqLimit2"); }
	virtual void SetRqLimit2(double value) 			{ throw not_implemented("SetRqLimit2"); }
	virtual double RqLimit() const 					{ throw not_implemented("RqLimit"); }
	virtual void SetRqLimit(double value) 			{ throw not_implemented("SetRqLimit"); }
	virtual int RandomSeed() const 					{ throw not_implemented("RandomSeed"); }
	virtual void SetRandomSeed(int value) 			{ throw not_implemented("SetRandomSeed"); }
	virtual long SaveBase() const 					{ throw not_implemented("SaveBase"); }
	virtual void SetSaveBase(long value) 			{ throw not_implemented("SetSaveBase"); }
	virtual ComplexD SaveC() const 					{ throw not_implemented("SaveC"); }
	virtual void SetSaveC(ComplexD value) 			{ throw not_implemented("SetSaveC"); }
	virtual long SaveTicks() const 					{ throw not_implemented("SaveTicks"); }
	virtual void SetSaveTicks(long value) 			{ _setSaveTicksCalled = true; _setSaveTicksLastValue = value; }
	bool SetSaveTicksCalled() const					{ return _setSaveTicksCalled; }
	long SetSaveTicksLastValue() const				{ return _setSaveTicksLastValue; }
	virtual int SaveRelease() const 				{ throw not_implemented("SaveRelease"); }
	virtual void SetSaveRelease(int value) 			{ throw not_implemented("SetSaveRelease"); }
	virtual float ScreenAspectRatio() const 		{ throw not_implemented("ScreenAspectRatio"); }
	virtual void SetScreenAspectRatio(float value)	{ throw not_implemented("SetScreenAspectRatio"); }
	virtual int ScreenHeight() const 				{ throw not_implemented("ScreenHeight"); }
	virtual void SetScreenHeight(int value) 		{ throw not_implemented("SetScreenHeight"); }
	virtual int ScreenWidth() const 				{ throw not_implemented("ScreenWidth"); }
	virtual void SetScreenWidth(int value) 			{ throw not_implemented("SetScreenWidth"); }
	virtual bool SetOrbitCorners() const 			{ throw not_implemented("SetOrbitCorners"); }
	virtual void SetSetOrbitCorners(bool value)		{ throw not_implemented("SetSetOrbitCorners"); }
	virtual int ShowDot() const 					{ throw not_implemented("ShowDot"); }
	virtual void SetShowDot(int value) 				{ throw not_implemented("SetShowDot"); }
	virtual ShowFileType ShowFile() const			{ mutate()->_showFileCalled = true; return _showFileFakeResult; }
	bool ShowFileCalled() const						{ return _showFileCalled; }
	void SetShowFileFakeResult(ShowFileType value)	{ _showFileFakeResult = value; }
	virtual void SetShowFile(ShowFileType value)	{ throw not_implemented("SetShowFile"); }
	virtual bool ShowOrbit() const 					{ throw not_implemented("ShowOrbit"); }
	virtual void SetShowOrbit(bool value) 			{ throw not_implemented("SetShowOrbit"); }
	virtual double SinX() const 					{ throw not_implemented("SinX"); }
	virtual void SetSinX(double value) 				{ throw not_implemented("SetSinX"); }
	virtual int SizeDot() const 					{ throw not_implemented("SizeDot"); }
	virtual void SetSizeDot(int value) 				{ throw not_implemented("SetSizeDot"); }
	virtual short const *SizeOfString() const 		{ throw not_implemented("SizeOfString"); }
	virtual void SetSizeOfString(short const *value) { throw not_implemented("SetSizeOfString"); }
	virtual int SkipXDots() const 					{ throw not_implemented("SkipXDots"); }
	virtual void SetSkipXDots(int value) 			{ throw not_implemented("SetSkipXDots"); }
	virtual int SkipYDots() const 					{ throw not_implemented("SkipYDots"); }
	virtual void SetSkipYDots(int value) 			{ throw not_implemented("SetSkipYDots"); }
	virtual int GaussianSlope() const 				{ throw not_implemented("GaussianSlope"); }
	virtual void SetGaussianSlope(int value) 		{ throw not_implemented("SetGaussianSlope"); }
	virtual PlotColorStandardFunction *PlotColorStandard() const { throw not_implemented("PlotColorStandard"); }
	virtual void SetPlotColorStandard(PlotColorStandardFunction *value) { throw not_implemented("SetPlotColorStandard"); }
	virtual bool StartShowOrbit() const 			{ throw not_implemented("StartShowOrbit"); }
	virtual void SetStartShowOrbit(bool value) 		{ throw not_implemented("SetStartShowOrbit"); }
	virtual bool StartedResaves() const 			{ throw not_implemented("StartedResaves"); }
	virtual void SetStartedResaves(bool value) 		{ throw not_implemented("SetStartedResaves"); }
	virtual int StopPass() const 					{ throw not_implemented("StopPass"); }
	virtual void SetStopPass(int value) 			{ throw not_implemented("SetStopPass"); }
	virtual unsigned int const *StringLocation() const { throw not_implemented("StringLocation"); }
	virtual void SetStringLocation(unsigned int const *value) { throw not_implemented("SetStringLocation"); }
	virtual BYTE const *Suffix() const 				{ throw not_implemented("Suffix"); }
	virtual void SetSuffix(BYTE const *value) 		{ throw not_implemented("SetSuffix"); }
	virtual double Sx3rd() const 					{ throw not_implemented("Sx3rd"); }
	virtual void SetSx3rd(double value) 			{ _setSx3rdCalled = true; _setSx3rdLastValue = value; }
	bool SetSx3rdCalled() const						{ return _setSx3rdCalled; }
	double SetSx3rdLastValue() const				{ return _setSx3rdLastValue; }
	virtual double SxMax() const 					{ throw not_implemented("SxMax"); }
	virtual void SetSxMax(double value) 			{ _setSxMaxCalled = true; _setSxMaxLastValue = value; }
	bool SetSxMaxCalled() const						{ return _setSxMaxCalled; }
	double SetSxMaxLastValue() const				{ return _setSxMaxLastValue; }
	virtual double SxMin() const 					{ throw not_implemented("SxMin"); }
	virtual void SetSxMin(double value) 			{ _setSxMinCalled = true; _setSxMinLastValue = value; }
	bool SetSxMinCalled() const						{ return _setSxMinCalled; }
	double SetSxMinLastValue() const				{ return _setSxMinLastValue; }
	virtual int ScreenXOffset() const 				{ throw not_implemented("ScreenXOffset"); }
	virtual void SetScreenXOffset(int value) 		{ throw not_implemented("SetScreenXOffset"); }
	virtual double Sy3rd() const 					{ throw not_implemented("Sy3rd"); }
	virtual void SetSy3rd(double value) 			{ _setSy3rdCalled = true; _setSy3rdLastValue = value; }
	bool SetSy3rdCalled() const						{ return _setSy3rdCalled; }
	double SetSy3rdLastValue() const				{ return _setSy3rdLastValue; }
	virtual double SyMax() const 					{ throw not_implemented("SyMax"); }
	virtual void SetSyMax(double value) 			{ _setSyMaxCalled = true; _setSyMaxLastValue = value; }
	bool SetSyMaxCalled() const						{ return _setSyMaxCalled; }
	double SetSyMaxLastValue() const				{ return _setSyMaxLastValue; }
	virtual double SyMin() const 					{ throw not_implemented("SyMin"); }
	virtual void SetSyMin(double value) 			{ _setSyMinCalled = true; _setSyMinLastValue = value; }
	bool SetSyMinCalled() const						{ return _setSyMinCalled; }
	double SetSyMinLastValue() const				{ return _setSyMinLastValue; }
	virtual SymmetryType Symmetry() const 			{ throw not_implemented("Symmetry"); }
	virtual void SetSymmetry(SymmetryType value)	{ throw not_implemented("SetSymmetry"); }
	virtual int ScreenYOffset() const 				{ throw not_implemented("ScreenYOffset"); }
	virtual void SetScreenYOffset(int value) 		{ throw not_implemented("SetScreenYOffset"); }
	virtual bool TabDisplayEnabled() const 			{ throw not_implemented("TabDisplayEnabled"); }
	virtual void SetTabDisplayEnabled(bool value)	{ throw not_implemented("SetTabDisplayEnabled"); }
	virtual bool TargaOutput() const 				{ throw not_implemented("TargaOutput"); }
	virtual void SetTargaOutput(bool value) 		{ throw not_implemented("SetTargaOutput"); }
	virtual bool TargaOverlay() const 				{ throw not_implemented("TargaOverlay"); }
	virtual void SetTargaOverlay(bool value) 		{ throw not_implemented("SetTargaOverlay"); }
	virtual double TempSqrX() const 				{ throw not_implemented("TempSqrX"); }
	virtual void SetTempSqrX(double value) 			{ throw not_implemented("SetTempSqrX"); }
	virtual double TempSqrY() const 				{ throw not_implemented("TempSqrY"); }
	virtual void SetTempSqrY(double value) 			{ throw not_implemented("SetTempSqrY"); }
	virtual int TextCbase() const 					{ throw not_implemented("TextCbase"); }
	virtual void SetTextCbase(int value) 			{ throw not_implemented("SetTextCbase"); }
	virtual int TextCol() const 					{ throw not_implemented("TextCol"); }
	virtual void SetTextCol(int value) 				{ throw not_implemented("SetTextCol"); }
	virtual int TextRbase() const 					{ throw not_implemented("TextRbase"); }
	virtual void SetTextRbase(int value) 			{ throw not_implemented("SetTextRbase"); }
	virtual int TextRow() const 					{ throw not_implemented("TextRow"); }
	virtual void SetTextRow(int value) 				{ throw not_implemented("SetTextRow"); }
	virtual unsigned int RhisGenerationRandomSeed() const { throw not_implemented("RhisGenerationRandomSeed"); }
	virtual void SetThisGenerationRandomSeed(int value) { throw not_implemented("SetThisGenerationRandomSeed"); }
	virtual bool ThreePass() const 					{ throw not_implemented("ThreePass"); }
	virtual void SetThreePass(bool value) 			{ throw not_implemented("SetThreePass"); }
	virtual double Threshold() const 				{ throw not_implemented("Threshold"); }
	virtual void SetThreshold(double value) 		{ throw not_implemented("SetThreshold"); }
	virtual TimedSaveType TimedSave() const 		{ mutate()->_timedSaveCalled = true; return _timedSaveFakeResult; }
	bool TimedSaveCalled() const					{ return _timedSaveCalled; }
	void SetTimedSaveFakeResult(TimedSaveType value) { _timedSaveFakeResult = value; }
	virtual void SetTimedSave(TimedSaveType value)	{ throw not_implemented("SetTimedSave"); }
	virtual bool TimerFlag() const 					{ throw not_implemented("TimerFlag"); }
	virtual void SetTimerFlag(bool value) 			{ throw not_implemented("SetTimerFlag"); }
	virtual long TimerInterval() const 				{ throw not_implemented("TimerInterval"); }
	virtual void SetTimerInterval(long value) 		{ throw not_implemented("SetTimerInterval"); }
	virtual long TimerStart() const 				{ throw not_implemented("TimerStart"); }
	virtual void SetTimerStart(long value) 			{ throw not_implemented("SetTimerStart"); }
	virtual ComplexD TempZ() const 					{ throw not_implemented("TempZ"); }
	virtual void SetTempZ(ComplexD value) 			{ throw not_implemented("SetTempZ"); }
	virtual int TotalPasses() const 				{ throw not_implemented("TotalPasses"); }
	virtual void SetTotalPasses(int value) 			{ throw not_implemented("SetTotalPasses"); }
	virtual int const *FunctionIndex() const 		{ throw not_implemented("FunctionIndex"); }
	virtual void SetFunctionIndex(int const *value) { throw not_implemented("SetFunctionIndex"); }
	virtual bool TrueColor() const 					{ throw not_implemented("TrueColor"); }
	virtual void SetTrueColor(bool value) 			{ throw not_implemented("SetTrueColor"); }
	virtual bool TrueModeIterates() const 			{ throw not_implemented("TrueModeIterates"); }
	virtual void SetTrueModeIterates(bool value) 	{ throw not_implemented("SetTrueModeIterates"); }
	virtual char const *TextStack() const 			{ throw not_implemented("TextStack"); }
	virtual void SetTextStack(char const *value)	{ throw not_implemented("SetTextStack"); }
	virtual double TwoPi() const 					{ throw not_implemented("TwoPi"); }
	virtual void SetTwoPi(double value) 			{ throw not_implemented("SetTwoPi"); }
	virtual UserInterfaceState const &UiState() const { throw not_implemented("UiState"); }
	virtual UserInterfaceState &UiState() 			{ throw not_implemented("UiState"); }
	virtual InitialZType UseInitialOrbitZ() const	{ throw not_implemented("UseInitialOrbitZ"); }
	virtual void SetUseInitialOrbitZ(InitialZType value) { throw not_implemented("SetUseInitialOrbitZ"); }
	virtual bool UseCenterMag() const 				{ throw not_implemented("UseCenterMag"); }
	virtual void SetUseCenterMag(bool value) 		{ throw not_implemented("SetUseCenterMag"); }
	virtual bool UseOldPeriodicity() const 			{ throw not_implemented("UseOldPeriodicity"); }
	virtual void SetUseOldPeriodicity(bool value)	{ throw not_implemented("SetUseOldPeriodicity"); }
	virtual bool UsingJiim() const 					{ throw not_implemented("UsingJiim"); }
	virtual void SetUsingJiim(bool value) 			{ throw not_implemented("SetUsingJiim"); }
	virtual int UserBiomorph() const 				{ throw not_implemented("UserBiomorph"); }
	virtual void SetUserBiomorph(int value) 		{ throw not_implemented("SetUserBiomorph"); }
	virtual long UserDistanceTest() const 			{ throw not_implemented("UserDistanceTest"); }
	virtual void SetUserDistanceTest(long value) 	{ throw not_implemented("SetUserDistanceTest"); }
	virtual bool UserFloatFlag() const 				{ throw not_implemented("UserFloatFlag"); }
	virtual void SetUserFloatFlag(bool value) 		{ throw not_implemented("SetUserFloatFlag"); }
	virtual int UserPeriodicityCheck() const		{ throw not_implemented("UserPeriodicityCheck"); }
	virtual void SetUserPeriodicityCheck(int value) { throw not_implemented("SetUserPeriodicityCheck"); }
	virtual int VxDots() const 						{ throw not_implemented("VxDots"); }
	virtual void SetVxDots(int value) 				{ throw not_implemented("SetVxDots"); }
	virtual int WhichImage() const 					{ throw not_implemented("WhichImage"); }
	virtual void SetWhichImage(int value) 			{ throw not_implemented("SetWhichImage"); }
	virtual float WidthFp() const 					{ throw not_implemented("WidthFp"); }
	virtual void SetWidthFp(float value) 			{ throw not_implemented("SetWidthFp"); }
	virtual int XDots() const 						{ throw not_implemented("XDots"); }
	virtual void SetXDots(int value) 				{ throw not_implemented("SetXDots"); }
	virtual int XShift1() const 					{ throw not_implemented("XShift1"); }
	virtual void SetXShift1(int value) 				{ throw not_implemented("SetXShift1"); }
	virtual int XShift() const 						{ throw not_implemented("XShift"); }
	virtual void SetXShift(int value) 				{ throw not_implemented("SetXShift"); }
	virtual int XxAdjust1() const 					{ throw not_implemented("XxAdjust1"); }
	virtual void SetXxAdjust1(int value) 			{ throw not_implemented("SetXxAdjust1"); }
	virtual int XxAdjust() const 					{ throw not_implemented("XxAdjust"); }
	virtual void SetXxAdjust(int value) 			{ throw not_implemented("SetXxAdjust"); }
	virtual int YDots() const 						{ throw not_implemented("YDots"); }
	virtual void SetYDots(int value) 				{ throw not_implemented("SetYDots"); }
	virtual int YShift() const 						{ throw not_implemented("YShift"); }
	virtual void SetYShift(int value) 				{ throw not_implemented("SetYShift"); }
	virtual int YShift1() const 					{ throw not_implemented("YShift1"); }
	virtual void SetYShift1(int value) 				{ throw not_implemented("SetYShift1"); }
	virtual int YyAdjust() const 					{ throw not_implemented("YyAdjust"); }
	virtual void SetYyAdjust(int value) 			{ throw not_implemented("SetYyAdjust"); }
	virtual int YyAdjust1() const 					{ throw not_implemented("YyAdjust1"); }
	virtual void SetYyAdjust1(int value) 			{ throw not_implemented("SetYyAdjust1"); }
	virtual double Zbx() const 						{ throw not_implemented("Zbx"); }
	virtual void SetZbx(double value) 				{ throw not_implemented("SetZbx"); }
	virtual double Zby() const 						{ throw not_implemented("Zby"); }
	virtual void SetZby(double value) 				{ throw not_implemented("SetZby"); }
	virtual double ZDepth() const 					{ throw not_implemented("ZDepth"); }
	virtual void SetZDepth(double value) 			{ throw not_implemented("SetZDepth"); }
	virtual int ZDots() const 						{ throw not_implemented("ZDots"); }
	virtual void SetZDots(int value) 				{ throw not_implemented("SetZDots"); }
	virtual int ZRotate() const 					{ throw not_implemented("ZRotate"); }
	virtual void SetZRotate(int value) 				{ throw not_implemented("SetZRotate"); }
	virtual double ZSkew() const 					{ throw not_implemented("ZSkew"); }
	virtual void SetZSkew(double value) 			{ throw not_implemented("SetZSkew"); }
	virtual double ZWidth() const 					{ mutate()->_zWidthCalled = true; return _zWidthFakeResult; }
	bool ZWidthCalled() const						{ return _zWidthCalled; }
	void SetZWidthFakeResult(double value)			{ _zWidthFakeResult = value; }
	virtual void SetZWidth(double value) 			{ _setZWidthCalled = true; _setZWidthLastValue = value; }
	virtual bool ZoomOff() const 					{ mutate()->_zoomOffCalled = true; return _zoomOffFakeResult; }
	bool ZoomOffCalled() const						{ return _zoomOffCalled; }
	void SetZoomOffFakeResult(bool value)			{ _zoomOffFakeResult = value; }
	virtual void SetZoomOff(bool value)				{ _setZoomOffCalled = true; _setZoomOffLastValue = value; }
	virtual ZoomBox const &Zoom() const 			{ mutate()->_zoomCalled = true; return _zoomFakeResult; }
	virtual ZoomBox &Zoom() 						{ _zoomCalled = true; return _zoomFakeResult; }
	bool ZoomCalled() const							{ return _zoomCalled; }
	void SetZoomFakeResult(const FakeZoomBox &value) { _zoomFakeResult = value; }
	virtual void SetZoom(ZoomBox const &value) 		{ throw not_implemented("SetZoom"); }
	virtual SoundState const &Sound() const 		{ throw not_implemented("Sound"); }
	virtual SoundState &Sound() 					{ throw not_implemented("Sound"); }
	virtual EscapeTimeState const &EscapeTime() const { mutate()->_escapeTimeCalled = true; return _escapeTimeFakeResult; }
	bool EscapeTimeCalled() const					{ return _escapeTimeCalled; }
	void SetEscapeTimeFakeResult(EscapeTimeState const &value) { _escapeTimeFakeResult = value; }
	virtual EscapeTimeState &EscapeTime()			{ _escapeTimeCalled = true; return _escapeTimeFakeResult; }
	virtual int BfMath() const 						{ mutate()->_bfMathCalled = true; return _bfMathFakeResult; }
	bool BfMathCalled() const						{ return _bfMathCalled; }
	void SetBfMathFakeResult(int value)				{ _bfMathFakeResult = value; }
	virtual void SetBfMath(int value) 				{ throw not_implemented("SetBfMath"); }
	virtual bf_t SxMinBf() const 					{ throw not_implemented("SxMinBf"); }
	virtual void SetSxMinBf(bf_t value) 			{ throw not_implemented("SetSxMinBf"); }
	virtual bf_t SxMaxBf() const 					{ throw not_implemented("SxMaxBf"); }
	virtual void SetSxMaxBf(bf_t value) 			{ throw not_implemented("SetSxMaxBf"); }
	virtual bf_t SyMinBf() const 					{ throw not_implemented("SyMinBf"); }
	virtual void SetSyMinBf(bf_t value) 			{ throw not_implemented("SetSyMinBf"); }
	virtual bf_t SyMaxBf() const 					{ throw not_implemented("SyMaxBf"); }
	virtual void SetSyMaxBf(bf_t value) 			{ throw not_implemented("SetSyMaxBf"); }
	virtual bf_t Sx3rdBf() const 					{ throw not_implemented("Sx3rdBf"); }
	virtual void SetSx3rdBf(bf_t value) 			{ throw not_implemented("SetSx3rdBf"); }
	virtual bf_t Sy3rdBf() const 					{ throw not_implemented("Sy3rdBf"); }
	virtual void SetSy3rdBf(bf_t value) 			{ throw not_implemented("SetSy3rdBf"); }
	virtual BrowseState const &Browse() const 		{ mutate()->_browseCalled = true; return _browseFakeResult; }
	bool BrowseCalled() const						{ return _browseCalled; }
	void SetBrowseFakeResult(FakeBrowseState const &value) { _browseFakeResult = value; }
	virtual BrowseState &Browse() 					{ _browseCalled = true; return _browseFakeResult; }
	virtual unsigned int ThisGenerationRandomSeed() const { throw not_implemented("ThisGenerationRandomSeed"); }
	virtual void SetThisGenerationRandomSeed(unsigned int value) { throw not_implemented("SetThisGenerationRandomSeed"); }
	virtual GIFViewFunction *GIFView() const 		{ throw not_implemented("GIFView"); }
	virtual OutLineFunction *OutLinePotential() const { throw not_implemented("OutLinePotential"); }
	virtual OutLineFunction *OutLineSound() const	{ throw not_implemented("OutLineSound"); }
	virtual OutLineFunction *OutLineRegular() const	{ throw not_implemented("OutLineRegular"); }
	virtual OutLineCleanupFunction *OutLineCleanupNull() const { throw not_implemented("OutLineCleanupNull"); }
	virtual OutLineFunction *OutLine3D() const		{ throw not_implemented("OutLine3D"); }
	virtual OutLineFunction *OutLineCompare() const { throw not_implemented("OutLineCompare"); }
	virtual std::string const &FractDir1() const	{ throw not_implemented("FractDir1"); }
	virtual void SetFractDir1(std::string const &value) { throw not_implemented("SetFractDir1"); }
	virtual std::string const &FractDir2() const	{ throw not_implemented("FractDir2"); }
	virtual void SetFractDir2(std::string const &value) { throw not_implemented("SetFractDir2"); }
	virtual std::string const &ReadName() const		{ throw not_implemented("ReadName"); }
	virtual std::string &ReadName() 				{ throw not_implemented("ReadName"); }
	virtual void SetReadName(std::string const &vlaue) { throw not_implemented("SetReadName"); }
	virtual std::string &GIFMask() 					{ throw not_implemented("GIFMask"); }
	virtual ViewWindow const &View() const 			{ throw not_implemented("View"); }
	virtual ViewWindow &View() 						{ throw not_implemented("View"); }
	virtual void SetFileNameStackTop(std::string const &value) { throw not_implemented("SetFileNameStackTop"); }
	virtual boost::filesystem::path const &WorkDirectory() const { throw not_implemented("WorkingDirectory"); }
	virtual CalculationMode StandardCalculationMode() const { throw not_implemented("StandardCalculationMode"); }
	virtual void SetStandardCalculationMode(CalculationMode value) { throw not_implemented("SetStandardCalculationMode"); }
	virtual CalculationMode UserStandardCalculationMode() const { throw not_implemented("UserStandardCalculationMode"); }
	virtual void SetUserStandardCalculationMode(CalculationMode value) { throw not_implemented("SetUserStandardCalculationMode"); }

private:
	FakeExternals *mutate() const					{ return const_cast<FakeExternals *>(this); }
	bool _showFileCalled;
	ShowFileType _showFileFakeResult;
	bool _calculationStatusCalled;
	CalculationStatusType _calculationStatusFakeResult;
	bool _initializeBatchCalled;
	InitializeBatchType _initializeBatchFakeResult;
	bool _setZoomOffCalled;
	bool _setZoomOffLastValue;
	bool _evolvingFlagsCalled;
	int _evolvingFlagsFakeResult;
	bool _escapeTimeCalled;
	EscapeTimeState _escapeTimeFakeResult;
	bool _setSxMinCalled;
	double _setSxMinLastValue;
	bool _setSxMaxCalled;
	double _setSxMaxLastValue;
	bool _setSx3rdCalled;
	double _setSx3rdLastValue;
	bool _setSyMinCalled;
	double _setSyMinLastValue;
	bool _setSyMaxCalled;
	double _setSyMaxLastValue;
	bool _setSy3rdCalled;
	double _setSy3rdLastValue;
	bool _bfMathCalled;
	int _bfMathFakeResult;
	bool _saveTimeCalled;
	int _saveTimeFakeResult;
	bool _browseCalled;
	FakeBrowseState _browseFakeResult;
	bool _setSaveTicksCalled;
	long _setSaveTicksLastValue;
	bool _zoomCalled;
	FakeZoomBox _zoomFakeResult;
	bool _setZWidthCalled;
	double _setZWidthLastValue;
	bool _fractalTypeCalled;
	int _fractalTypeFakeResult;
	bool _timedSaveCalled;
	TimedSaveType _timedSaveFakeResult;
	bool _zWidthCalled;
	double _zWidthFakeResult;
	bool _quickCalculateCalled;
	bool _quickCalculateFakeResult;
	bool _zoomOffCalled;
	bool _zoomOffFakeResult;
};
