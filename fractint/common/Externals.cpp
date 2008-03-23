#include <complex>

#include "port.h"
#include "id.h"
#include "cmplx.h"
#include "externs.h"
#include "winprot.h"

#include "line3d.h"
#include "framain2.h"
#include "fimain.h"
#include "Externals.h"
#include "gifview.h"
#include "ViewWindow.h"

class ExternalsImpl : public Externals
{
public:
	virtual ~ExternalsImpl() { }

	virtual float AspectDrift() const							{ return g_aspect_drift; }
	virtual void SetAspectDrift(float value)					{ g_aspect_drift = value; }
	virtual int AtanColors() const								{ return g_atan_colors; }
	virtual void SetAtanColors(int value)						{ g_atan_colors = value; }
	virtual double AutoStereoWidth() const						{ return g_auto_stereo_width; }
	virtual void SetAutoStereoWidth(double value)				{ g_auto_stereo_width = value; }
	virtual bool BadOutside() const								{ return g_bad_outside; }
	virtual void SetBadOutside(bool value)						{ g_bad_outside = value; }
	virtual long BailOut() const								{ return g_bail_out; }
	virtual void SetBailOut(long value)							{ g_bail_out = value; }
	virtual BailOutFunction *BailOutFp() const					{ return g_bail_out_fp; }
	virtual void SetBailOutFp(BailOutFunction *value)			{ g_bail_out_fp = value; }
	virtual BailOutFunction *BailOutL() const					{ return g_bail_out_l; }
	virtual void SetBailOutL(BailOutFunction *value)			{ g_bail_out_l = value; }
	virtual BailOutFunction *BailOutBf() const					{ return g_bail_out_bf; }
	virtual void SetBailOutBf(BailOutFunction *value)			{ g_bail_out_bf = value; }
	virtual BailOutFunction *BailOutBn() const					{ return g_bail_out_bn; }
	virtual void SetBailOutBn(BailOutFunction *value)			{ g_bail_out_bn = value; }
	virtual enum bailouts BailOutTest() const					{ return g_bail_out_test; }
	virtual void SetBailOutTest(bailouts value)					{ g_bail_out_test = value; }
	virtual int Basin() const									{ return g_basin; }
	virtual void SetBasin(int value)							{ g_basin = value; }
	virtual int BfSaveLen() const								{ return g_bf_save_len; }
	virtual void SetBfSaveLen(int value)						{ g_bf_save_len = value; }
	virtual int BfDigits() const								{ return g_bf_digits; }
	virtual void SetBfDigits(int value)							{ g_bf_digits = value; }
	virtual int Biomorph() const								{ return g_biomorph; }
	virtual void SetBiomorph(int value)							{ g_biomorph = value; }
	virtual int BitShift() const								{ return g_bit_shift; }
	virtual void SetBitShift(int value)							{ g_bit_shift = value; }
	virtual int BitShiftMinus1() const							{ return g_bit_shift_minus_1; }
	virtual void SetBitShiftMinus1(int value)					{ g_bit_shift_minus_1 = value; }
	virtual BYTE const *Block() const							{ return g_block; }
	virtual void SetBlock(BYTE const *value)					{ /*g_block = value;*/ }
	virtual long CalculationTime() const						{ return g_calculation_time; }
	virtual void SetCalculationTime(long value)					{ g_calculation_time = value; }
	virtual CalculateMandelbrotFunction *CalculateMandelbrotAsmFp() const { return g_calculate_mandelbrot_asm_fp; }
	virtual void SetCalculateMandelbrotAsmFp(CalculateMandelbrotFunction *value) { g_calculate_mandelbrot_asm_fp = value; }
	virtual Externals::CalculateTypeFunction *CalculateType() const	{ return g_calculate_type; }
	virtual void SetCalculateType(CalculateTypeFunction *value) { g_calculate_type = value; }
	virtual int CalculationStatus() const						{ return g_calculation_status; }
	virtual void SetCalculationStatus(int value)				{ g_calculation_status = value; }
	virtual int const *CfgLineNums() const						{ return g_cfg_line_nums; }
	virtual void SetCfgLineNums(int const *value)				{ /*g_cfg_line_nums = value;*/ }
	virtual bool CheckCurrentDir() const						{ return g_check_current_dir; }
	virtual void SetCheckCurrentDir(bool value)					{ g_check_current_dir = value; }
	virtual long CImag() const									{ return g_c_imag; }
	virtual void SetCImag(long value)							{ g_c_imag = value; }
	virtual double CloseEnough() const							{ return g_close_enough; }
	virtual void SetCloseEnough(double value)					{ g_close_enough = value; }
	virtual double Proximity() const							{ return g_proximity; }
	virtual void SetProximity(double value)						{ g_proximity = value; }
	virtual ComplexD Coefficient() const						{ return g_coefficient; }
	virtual void SetCoefficient(ComplexD value)					{ g_coefficient = value; }
	virtual int Col() const										{ return g_col; }
	virtual void SetCol(int value)								{ g_col = value; }
	virtual int Color() const									{ return g_color; }
	virtual void SetColor(int value)							{ g_color = value; }
	virtual long ColorIter() const								{ return g_color_iter; }
	virtual void SetColorIter(long value)						{ g_color_iter = value; }
	virtual bool ColorPreloaded() const							{ return g_color_preloaded; }
	virtual void SetColorPreloaded(bool value)					{ g_color_preloaded = value; }
	virtual int Colors() const									{ return g_colors; }
	virtual void SetColors(int value)							{ g_colors = value; }
	virtual bool CompareGif() const								{ return g_compare_gif; }
	virtual void SetCompareGif(bool value)						{ g_compare_gif = value; }
	virtual long GaussianConstant() const						{ return g_gaussian_constant; }
	virtual void SetGaussianConstant(long value)				{ g_gaussian_constant = value; }
	virtual double CosX() const									{ return g_cos_x; }
	virtual void SetCosX(double value)							{ g_cos_x = value; }
	virtual long CReal() const									{ return g_c_real; }
	virtual void SetCReal(long value)							{ g_c_real = value; }
	virtual int CurrentCol() const								{ return g_current_col; }
	virtual void SetCurrentCol(int value)						{ g_current_col = value; }
	virtual int CurrentPass() const								{ return g_current_pass; }
	virtual void SetCurrentPass(int value)						{ g_current_pass = value; }
	virtual int CurrentRow() const								{ return g_current_row; }
	virtual void SetCurrentRow(int value)						{ g_current_row = value; }
	virtual int CycleLimit() const								{ return g_cycle_limit; }
	virtual void SetCycleLimit(int value)						{ g_cycle_limit = value; }
	virtual int CExp() const									{ return g_c_exp; }
	virtual void SetCExp(int value)								{ g_c_exp = value; }
	virtual double DeltaMinFp() const							{ return g_delta_min_fp; }
	virtual void SetDeltaMinFp(double value)					{ g_delta_min_fp = value; }
	virtual int DebugMode() const								{ return g_debug_mode; }
	virtual void SetDebugMode(int value)						{ g_debug_mode = value; }
	virtual int Decimals() const								{ return g_decimals; }
	virtual void SetDecimals(int value)							{ g_decimals = value; }
	virtual BYTE const *DecoderLine() const						{ return g_decoder_line; }
	virtual void SetDecoderLine(BYTE const *value)				{ /*g_decoder_line = value;*/ }
	virtual int const *Decomposition() const					{ return g_decomposition; }
	virtual void SetDecomposition(int const *value)				{ /*g_decomposition = value;*/ }
	virtual int Degree() const									{ return g_degree; }
	virtual void SetDegree(int value)							{ g_degree = value; }
	virtual long DeltaMin() const								{ return g_delta_min; }
	virtual void SetDeltaMin(long value)						{ g_delta_min = value; }
	virtual float DepthFp() const								{ return g_depth_fp; }
	virtual void SetDepthFp(float value)						{ g_depth_fp = value; }
	virtual bool Disk16Bit() const								{ return g_disk_16bit; }
	virtual void SetDisk16Bit(bool value)						{ g_disk_16bit = value; }
	virtual bool DiskFlag() const								{ return g_disk_flag; }
	virtual void SetDiskFlag(bool value)						{ g_disk_flag = value; }
	virtual bool DiskTarga() const								{ return g_disk_targa; }
	virtual void SetDiskTarga(bool value)						{ g_disk_targa = value; }
	virtual Display3DType Display3D() const						{ return g_display_3d; }
	virtual void SetDisplay3D(Display3DType value)				{ g_display_3d = value; }
	virtual long DistanceTest() const							{ return g_distance_test; }
	virtual void SetDistanceTest(long value)					{ g_distance_test = value; }
	virtual int DistanceTestWidth() const						{ return g_distance_test_width; }
	virtual void SetDistanceTestWidth(int value)				{ g_distance_test_width = value; }
	virtual float ScreenDistanceFp() const						{ return g_screen_distance_fp; }
	virtual void SetScreenDistanceFp(float value)				{ g_screen_distance_fp = value; }
	virtual int GaussianDistribution() const					{ return g_gaussian_distribution; }
	virtual void SetGaussianDistribution(int value)				{ g_gaussian_distribution = value; }
	virtual bool DitherFlag() const								{ return g_dither_flag; }
	virtual void SetDitherFlag(bool value)						{ g_dither_flag = value; }
	virtual bool DontReadColor() const							{ return g_dont_read_color; }
	virtual void SetDontReadColor(bool value)					{ g_dont_read_color = value; }
	virtual double DeltaParameterImageX() const					{ return g_delta_parameter_image_x; }
	virtual void SetDeltaParameterImageX(double value)			{ g_delta_parameter_image_x = value; }
	virtual double DeltaParameterImageY() const					{ return g_delta_parameter_image_y; }
	virtual void SetDeltaParameterImageY(double value)			{ g_delta_parameter_image_y = value; }
	virtual BYTE const *Stack() const							{ return g_stack; }
	virtual void SetStack(BYTE const *value)					{ /*g_stack = value;*/ }
	virtual DxPixelFunction *DxPixel() const					{ return g_dx_pixel; }
	virtual void SetDxPixel(DxPixelFunction *value)				{ g_dx_pixel = value; }
	virtual double DxSize() const								{ return g_dx_size; }
	virtual void SetDxSize(double value)						{ g_dx_size = value; }
	virtual DyPixelFunction *DyPixel() const					{ return g_dy_pixel; }
	virtual void SetDyPixel(DyPixelFunction *value)				{ g_dy_pixel = value; }
	virtual double DySize() const								{ return g_dy_size; }
	virtual void SetDySize(double value)						{ g_dy_size = value; }
	virtual bool EscapeExitFlag() const							{ return g_escape_exit_flag; }
	virtual void SetEscapeExitFlag(bool value)					{ g_escape_exit_flag = value; }
	virtual int EvolvingFlags() const							{ return g_evolving_flags; }
	virtual void SetEvolvingFlags(int value)					{ g_evolving_flags = value; }
	virtual evolution_info const *EvolveInfo() const			{ return g_evolve_info; }
	virtual evolution_info *EvolveInfo()						{ return g_evolve_info; }
	virtual void SetEvolveInfo(evolution_info *value)			{ g_evolve_info = value; }
	virtual float EyesFp() const								{ return g_eyes_fp; }
	virtual void SetEyesFp(float value)							{ g_eyes_fp = value; }
	virtual bool FastRestore() const							{ return g_fast_restore; }
	virtual void SetFastRestore(bool value)						{ g_fast_restore = value; }
	virtual double FudgeLimit() const							{ return g_fudge_limit; }
	virtual void SetFudgeLimit(double value)					{ g_fudge_limit = value; }
	virtual long OneFudge() const								{ return g_one_fudge; }
	virtual void SetOneFudge(long value)						{ g_one_fudge = value; }
	virtual long TwoFudge() const								{ return g_two_fudge; }
	virtual void SetTwoFudge(long value)						{ g_two_fudge = value; }
	virtual int GridSize() const								{ return g_grid_size; }
	virtual void SetGridSize(int value)							{ g_grid_size = value; }
	virtual double FiddleFactor() const							{ return g_fiddle_factor; }
	virtual void SetFiddleFactor(double value)					{ g_fiddle_factor = value; }
	virtual double FiddleReduction() const						{ return g_fiddle_reduction; }
	virtual void SetFiddleReduction(double value)				{ g_fiddle_reduction = value; }
	virtual float FileAspectRatio() const						{ return g_file_aspect_ratio; }
	virtual void SetFileAspectRatio(float value)				{ g_file_aspect_ratio = value; }
	virtual int FileColors() const								{ return g_file_colors; }
	virtual void SetFileColors(int value)						{ g_file_colors = value; }
	virtual int FileXDots() const								{ return g_file_x_dots; }
	virtual void SetFileXDots(int value)						{ g_file_x_dots = value; }
	virtual int FileYDots() const								{ return g_file_y_dots; }
	virtual void SetFileYDots(int value)						{ g_file_y_dots = value; }
	virtual int FillColor() const								{ return g_fill_color; }
	virtual void SetFillColor(int value)						{ g_fill_color = value; }
	virtual int FinishRow() const								{ return g_finish_row; }
	virtual void SetFinishRow(int value)						{ g_finish_row = value; }
	virtual bool CommandInitialize() const						{ return g_command_initialize; }
	virtual void SetCommandInitialize(bool value)				{ g_command_initialize = value; }
	virtual int FirstSavedAnd() const							{ return g_first_saved_and; }
	virtual void SetFirstSavedAnd(int value)					{ g_first_saved_and = value; }
	virtual bool FloatFlag() const								{ return g_float_flag; }
	virtual void SetFloatFlag(bool value)						{ g_float_flag = value; }
	virtual ComplexD const *FloatParameter() const				{ return g_float_parameter; }
	virtual void SetFloatParameter(ComplexD const *value)		{ /*g_float_parameter = value;*/ }
	virtual BYTE const *Font8x8() const							{ return &g_font_8x8[0][0]; }
	virtual int ForceSymmetry() const							{ return g_force_symmetry; }
	virtual void SetForceSymmetry(int value)					{ g_force_symmetry = value; }
	virtual int FractalType() const								{ return g_fractal_type; }
	virtual void SetFractalType(int value)						{ g_fractal_type = value; }
	virtual bool FromTextFlag() const							{ return g_from_text_flag; }
	virtual void SetFromTextFlag(bool value)					{ g_from_text_flag = value; }
	virtual long Fudge() const									{ return g_fudge; }
	virtual void SetFudge(long value)							{ g_fudge = value; }
	virtual FunctionListItem const *FunctionList() const		{ return g_function_list; }
	virtual void SetFunctionList(FunctionListItem const *value) { /*g_function_list = value;*/ }
	virtual bool FunctionPreloaded() const						{ return g_function_preloaded; }
	virtual void SetFunctionPreloaded(bool value)				{ g_function_preloaded = value; }
	virtual double FRadius() const								{ return g_f_radius; }
	virtual void SetFRadius(double value)						{ g_f_radius = value; }
	virtual double FXCenter() const								{ return g_f_x_center; }
	virtual void SetFXCenter(double value)						{ g_f_x_center = value; }
	virtual double FYCenter() const								{ return g_f_y_center; }
	virtual void SetFYCenter(double value)						{ g_f_y_center = value; }
	virtual GENEBASE *Genes() const								{ return g_genes; }
	virtual bool Gif87aFlag() const								{ return g_gif87a_flag; }
	virtual void SetGif87aFlag(bool value)						{ g_gif87a_flag = value; }
	virtual int GotStatus() const								{ return g_got_status; }
	virtual void SetGotStatus(int value)						{ g_got_status = value; }
	virtual bool GrayscaleDepth() const							{ return g_grayscale_depth; }
	virtual void SetGrayscaleDepth(bool value)					{ g_grayscale_depth = value; }
	virtual bool HasInverse() const								{ return g_has_inverse; }
	virtual void SetHasInverse(bool value)						{ g_has_inverse = value; }
	virtual unsigned int Height() const							{ return g_height; }
	virtual void SetHeight(int value)							{ g_height = value; }
	virtual float HeightFp() const								{ return g_height_fp; }
	virtual void SetHeightFp(float value)						{ g_height_fp = value; }
	virtual float const *IfsDefinition() const					{ return g_ifs_definition; }
	virtual void SetIfsDefinition(float const *value)			{ /*g_ifs_definition = value;*/ }
	virtual int IfsType() const									{ return g_ifs_type; }
	virtual void SetIfsType(int value)							{ g_ifs_type = value; }
	virtual ComplexD InitialZ() const							{ return g_initial_z; }
	virtual void SetInitialZ(ComplexD value)					{ g_initial_z = value; }
	virtual InitializeBatchType InitializeBatch() const			{ return g_initialize_batch; }
	virtual void SetInitializeBatch(InitializeBatchType value)	{ g_initialize_batch = value; }
	virtual int InitialCycleLimit() const						{ return g_initial_cycle_limit; }
	virtual void SetInitialCycleLimit(int value)				{ g_initial_cycle_limit = value; }
	virtual ComplexD InitialOrbitZ() const						{ return g_initial_orbit_z; }
	virtual void SetInitialOrbitZ(ComplexD value)				{ g_initial_orbit_z = value; }
	virtual int SaveTime() const								{ return g_save_time; }
	virtual void SetSaveTime(int value)							{ g_save_time = value; }
	virtual int Inside() const									{ return g_inside; }
	virtual void SetInside(int value)							{ g_inside = value; }
	virtual int IntegerFractal() const							{ return g_integer_fractal; }
	virtual void SetIntegerFractal(int value)					{ g_integer_fractal = value; }
	virtual double const *Inversion() const						{ return g_inversion; }
	virtual void SetInversion(double const *value)				{ /*g_inversion = value;*/ }
	virtual int Invert() const									{ return g_invert; }
	virtual void SetInvert(int value)							{ g_invert = value; }
	virtual int IsTrueColor() const								{ return g_is_true_color; }
	virtual void SetIsTrueColor(int value)						{ g_is_true_color = value; }
	virtual bool IsMandelbrot() const							{ return g_is_mandelbrot; }
	virtual void SetIsMandelbrot(bool value)					{ g_is_mandelbrot = value; }
	virtual int XStop() const									{ return g_x_stop; }
	virtual void SetXStop(int value)							{ g_x_stop = value; }
	virtual int YStop() const									{ return g_y_stop; }
	virtual void SetYStop(int value)							{ g_y_stop = value; }
	virtual std::complex<double> JuliaC() const					{ return g_julia_c; }
	virtual void SetJuliaC(std::complex<double> const &value)	{ g_julia_c = value; }
	virtual int Juli3DMode() const								{ return g_juli_3d_mode; }
	virtual void SetJuli3DMode(int value)						{ g_juli_3d_mode = value; }
	virtual char const **Juli3DOptions() const					{ return &g_juli_3d_options[0]; }
	virtual bool Julibrot() const								{ return g_julibrot; }
	virtual void SetJulibrot(bool value)						{ g_julibrot = value; }
	virtual int InputCounter() const							{ return g_input_counter; }
	virtual void SetInputCounter(int value)						{ g_input_counter = value; }
	virtual bool KeepScreenCoords() const						{ return g_keep_screen_coords; }
	virtual void SetKeepScreenCoords(bool value)				{ g_keep_screen_coords = value; }
	virtual long CloseEnoughL() const							{ return g_close_enough_l; }
	virtual void SetCloseEnoughL(long value)					{ g_close_enough_l = value; }
	virtual ComplexL CoefficientL() const						{ return g_coefficient_l; }
	virtual void SetCoefficientL(ComplexL value)				{ g_coefficient_l = value; }
	virtual bool UseOldComplexPower() const						{ return g_use_old_complex_power; }
	virtual void SetUseOldComplexPower(bool value)				{ g_use_old_complex_power = value; }
	virtual BYTE const *LineBuffer() const						{ return g_line_buffer; }
	virtual void SetLineBuffer(BYTE const *value)				{ /*g_line_buffer = value;*/ }
	virtual ComplexL InitialZL() const							{ return g_initial_z_l; }
	virtual void SetInitialZL(ComplexL value)					{ g_initial_z_l = value; }
	virtual ComplexL InitOrbitL() const							{ return g_init_orbit_l; }
	virtual void SetInitOrbitL(ComplexL value)					{ g_init_orbit_l = value; }
	virtual long InitialXL() const								{ return g_initial_x_l; }
	virtual void SetInitialXL(long value)						{ g_initial_x_l = value; }
	virtual long InitialYL() const								{ return g_initial_y_l; }
	virtual void SetInitialYL(long value)						{ g_initial_y_l = value; }
	virtual long Limit2L() const								{ return g_limit2_l; }
	virtual void SetLimit2L(long value)							{ g_limit2_l = value; }
	virtual long LimitL() const									{ return g_limit_l; }
	virtual void SetLimitL(long value)							{ g_limit_l = value; }
	virtual long MagnitudeL() const								{ return g_magnitude_l; }
	virtual void SetMagnitudeL(long value)						{ g_magnitude_l = value; }
	virtual ComplexL NewZL() const								{ return g_new_z_l; }
	virtual void SetNewZL(ComplexL value)						{ g_new_z_l = value; }
	virtual bool Loaded3D() const								{ return g_loaded_3d; }
	virtual void SetLoaded3D(bool value)						{ g_loaded_3d = value; }
	virtual bool LogAutomaticFlag() const						{ return g_log_automatic_flag; }
	virtual void SetLogAutomaticFlag(bool value)				{ g_log_automatic_flag = value; }
	virtual bool LogCalculation() const							{ return g_log_calculation; }
	virtual void SetLogCalculation(bool value)					{ g_log_calculation = value; }
	virtual int LogDynamicCalculate() const						{ return g_log_dynamic_calculate; }
	virtual void SetLogDynamicCalculate(int value)				{ g_log_dynamic_calculate = value; }
	virtual long LogPaletteMode() const							{ return g_log_palette_mode; }
	virtual void SetLogPaletteMode(long value)					{ g_log_palette_mode = value; }
	virtual BYTE const *LogTable() const						{ return g_log_table; }
	virtual void SetLogTable(BYTE const *value)					{ /*g_log_table = value;*/ }
	virtual ComplexL OldZL() const								{ return g_old_z_l; }
	virtual void SetOldZL(ComplexL value)						{ g_old_z_l = value; }
	virtual ComplexL const *LongParameter() const				{ return g_long_parameter; }
	virtual void SetLongParameter(ComplexL const *value)		{ /*g_long_parameter = value;*/ }
	virtual ComplexL Parameter2L() const						{ return g_parameter2_l; }
	virtual void SetParameter2L(ComplexL value)					{ g_parameter2_l = value; }
	virtual ComplexL ParameterL() const							{ return g_parameter_l; }
	virtual void SetParameterL(ComplexL value)					{ g_parameter_l = value; }
	virtual long TempSqrXL() const								{ return g_temp_sqr_x_l; }
	virtual void SetTempSqrXL(long value)						{ g_temp_sqr_x_l = value; }
	virtual long TempSqrYL() const								{ return g_temp_sqr_y_l; }
	virtual void SetTempSqrYL(long value)						{ g_temp_sqr_y_l = value; }
	virtual ComplexL TmpZL() const								{ return g_tmp_z_l; }
	virtual void SetTmpZL(ComplexL value)						{ g_tmp_z_l = value; }
	virtual PixelFunction *LxPixel() const						{ return g_lx_pixel; }
	virtual void SetLxPixel(PixelFunction *value)				{ g_lx_pixel = value; }
	virtual PixelFunction *LyPixel() const						{ return g_ly_pixel; }
	virtual void SetLyPixel(PixelFunction *value)				{ g_ly_pixel = value; }
	virtual TrigFunction *Trig0L() const						{ return g_trig0_l; }
	virtual void SetTrig0L(TrigFunction *value)					{ g_trig0_l = value; }
	virtual TrigFunction *Trig1L() const						{ return g_trig1_l; }
	virtual void SetTrig1L(TrigFunction *value)					{ g_trig1_l = value; }
	virtual TrigFunction *Trig2L() const						{ return g_trig2_l; }
	virtual void SetTrig2L(TrigFunction *value)					{ g_trig2_l = value; }
	virtual TrigFunction *Trig3L() const						{ return g_trig3_l; }
	virtual void SetTrig3L(TrigFunction *value)					{ g_trig3_l = value; }
	virtual TrigFunction *Trig0D() const						{ return g_trig0_d; }
	virtual void SetTrig0D(TrigFunction *value)					{ g_trig0_d = value; }
	virtual TrigFunction *Trig1D() const						{ return g_trig1_d; }
	virtual void SetTrig1D(TrigFunction *value)					{ g_trig1_d = value; }
	virtual TrigFunction *Trig2D() const						{ return g_trig2_d; }
	virtual void SetTrig2D(TrigFunction *value)					{ g_trig2_d = value; }
	virtual TrigFunction *Trig3D() const						{ return g_trig3_d; }
	virtual void SetTrig3D(TrigFunction *value)					{ g_trig3_d = value; }
	virtual double Magnitude() const							{ return g_magnitude; }
	virtual void SetMagnitude(double value)						{ g_magnitude = value; }
	virtual unsigned long magnitudeLimit() const				{ return g_magnitude_limit; }
	virtual void SetMagnitudeLimit(long value)					{ g_magnitude_limit = value; }
	virtual MajorMethodType MajorMethod() const					{ return g_major_method; }
	virtual void SetMajorMethod(MajorMethodType value)			{ g_major_method = value; }
	virtual int MathErrorCount() const							{ return g_math_error_count; }
	virtual void SetMathErrorCount(int value)					{ g_math_error_count = value; }
	virtual double const *MathTolerance() const					{ return g_math_tolerance; }
	virtual void SetMathTolerance(double const *value)			{ /*g_math_tolerance = value;*/ }
	virtual long MaxCount() const								{ return g_max_count; }
	virtual void SetMaxCount(long value)						{ g_max_count = value; }
	virtual long MaxIteration() const							{ return g_max_iteration; }
	virtual void SetMaxIteration(long value)					{ g_max_iteration = value; }
	virtual int MaxLineLength() const							{ return g_max_line_length; }
	virtual void SetMaxLineLength(int value)					{ g_max_line_length = value; }
	virtual long MaxLogTableSize() const						{ return g_max_log_table_size; }
	virtual void SetMaxLogTableSize(long value)					{ g_max_log_table_size = value; }
	virtual long BnMaxStack() const								{ return g_bn_max_stack; }
	virtual void SetBnMaxStack(long value)						{ g_bn_max_stack = value; }
	virtual int MaxColors() const								{ return g_max_colors; }
	virtual void SetMaxColors(int value)						{ g_max_colors = value; }
	virtual int MaxInputCounter() const							{ return g_max_input_counter; }
	virtual void SetMaxInputCounter(int value)					{ g_max_input_counter = value; }
	virtual int MaxHistory() const								{ return g_max_history; }
	virtual void SetMaxHistory(int value)						{ g_max_history = value; }
	virtual MinorMethodType MinorMethod() const					{ return g_minor_method; }
	virtual void SetMinorMethod(MinorMethodType value)			{ g_minor_method = value; }
	virtual more_parameters const *MoreParameters() const		{ return g_more_parameters; }
	virtual void SetMoreParameters(more_parameters const *value) { /*g_more_parameters = value;*/ }
	virtual int OverflowMp() const								{ return g_overflow_mp; }
	virtual void SetOverflowMp(int value)						{ g_overflow_mp = value; }
	virtual double MXMaxFp() const								{ return g_m_x_max_fp; }
	virtual void SetMXMaxFp(double value)						{ g_m_x_max_fp = value; }
	virtual double MXMinFp() const								{ return g_m_x_min_fp; }
	virtual void SetMXMinFp(double value)						{ g_m_x_min_fp = value; }
	virtual double MYMaxFp() const								{ return g_m_y_max_fp; }
	virtual void SetMYMaxFp(double value)						{ g_m_y_max_fp = value; }
	virtual double MYMinFp() const								{ return g_m_y_min_fp; }
	virtual void SetMYMinFp(double value)						{ g_m_y_min_fp = value; }
	virtual int NameStackPtr() const							{ return g_name_stack_ptr; }
	virtual void SetNameStackPtr(int value)						{ g_name_stack_ptr = value; }
	virtual ComplexD NewZ() const								{ return g_new_z; }
	virtual void SetNewZ(ComplexD value)						{ g_new_z = value; }
	virtual int NewDiscreteParameterOffsetX() const				{ return g_new_discrete_parameter_offset_x; }
	virtual void SetNewDiscreteParameterOffsetX(int value)		{ g_new_discrete_parameter_offset_x = value; }
	virtual int NewDiscreteParameterOffsetY() const				{ return g_new_discrete_parameter_offset_y; }
	virtual void SetNewDiscreteParameterOffsetY(int value)		{ g_new_discrete_parameter_offset_y = value; }
	virtual double NewParameterOffsetX() const					{ return g_new_parameter_offset_x; }
	virtual void SetNewParameterOffsetX(double value)			{ g_new_parameter_offset_x = value; }
	virtual double NewParameterOffsetY() const					{ return g_new_parameter_offset_y; }
	virtual void SetNewParameterOffsetY(double value)			{ g_new_parameter_offset_y = value; }
	virtual int NewOrbitType() const							{ return g_new_orbit_type; }
	virtual void SetNewOrbitType(int value)						{ g_new_orbit_type = value; }
	virtual int NextSavedIncr() const							{ return g_next_saved_incr; }
	virtual void SetNextSavedIncr(int value)					{ g_next_saved_incr = value; }
	virtual bool NoMagnitudeCalculation() const					{ return g_no_magnitude_calculation; }
	virtual void SetNoMagnitudeCalculation(bool value)			{ g_no_magnitude_calculation = value; }
	virtual int NumAffine() const								{ return g_num_affine; }
	virtual void SetNumAffine(int value)						{ g_num_affine = value; }
	virtual const int NumFunctionList() const					{ return g_num_function_list; }
	virtual int NumFractalTypes() const							{ return g_num_fractal_types; }
	virtual bool NextScreenFlag() const							{ return g_next_screen_flag; }
	virtual void SetNextScreenFlag(bool value)					{ g_next_screen_flag = value; }
	virtual int GaussianOffset() const							{ return g_gaussian_offset; }
	virtual void SetGaussianOffset(int value)					{ g_gaussian_offset = value; }
	virtual bool OkToPrint() const								{ return g_ok_to_print; }
	virtual void SetOkToPrint(bool value)						{ g_ok_to_print = value; }
	virtual ComplexD OldZ() const								{ return g_old_z; }
	virtual void SetOldZ(ComplexD value)						{ g_old_z = value; }
	virtual long OldColorIter() const							{ return g_old_color_iter; }
	virtual void SetOldColorIter(long value)					{ g_old_color_iter = value; }
	virtual bool OldDemmColors() const							{ return g_old_demm_colors; }
	virtual void SetOldDemmColors(bool value)					{ g_old_demm_colors = value; }
	virtual int DiscreteParameterOffsetX() const				{ return g_discrete_parameter_offset_x; }
	virtual void SetDiscreteParameterOffsetX(int value)			{ g_discrete_parameter_offset_x = value; }
	virtual int DiscreteParameterOffsetY() const				{ return g_discrete_parameter_offset_y; }
	virtual void SetDiscreteParameterOffsetY(int value)			{ g_discrete_parameter_offset_y = value; }
	virtual double ParameterOffsetX() const						{ return g_parameter_offset_x; }
	virtual void SetParameterOffsetX(double value)				{ g_parameter_offset_x = value; }
	virtual double ParameterOffsetY() const						{ return g_parameter_offset_y; }
	virtual void SetParameterOffsetY(double value)				{ g_parameter_offset_y = value; }
	virtual int OrbitSave() const								{ return g_orbit_save; }
	virtual void SetOrbitSave(int value)						{ g_orbit_save = value; }
	virtual int OrbitColor() const								{ return g_orbit_color; }
	virtual void SetOrbitColor(int value)						{ g_orbit_color = value; }
	virtual int OrbitDelay() const								{ return g_orbit_delay; }
	virtual void SetOrbitDelay(int value)						{ g_orbit_delay = value; }
	virtual int OrbitDrawMode() const							{ return g_orbit_draw_mode; }
	virtual void SetOrbitDrawMode(int value)					{ g_orbit_draw_mode = value; }
	virtual long OrbitInterval() const							{ return g_orbit_interval; }
	virtual void SetOrbitInterval(long value)					{ g_orbit_interval = value; }
	virtual int OrbitIndex() const								{ return g_orbit_index; }
	virtual void SetOrbitIndex(int value)						{ g_orbit_index = value; }
	virtual bool OrganizeFormulaSearch() const					{ return g_organize_formula_search; }
	virtual void SetOrganizeFormulaSearch(bool value)			{ g_organize_formula_search = value; }
	virtual float OriginFp() const								{ return g_origin_fp; }
	virtual void SetOriginFp(float value)						{ g_origin_fp = value; }
	virtual OutLineFunction *OutLine() const					{ return g_out_line; }
	virtual void SetOutLine(OutLineFunction *value)				{ g_out_line = value; }
	virtual OutLineCleanupFunction *OutLineCleanup() const		{ return g_out_line_cleanup; }
	virtual void SetOutLineCleanup(OutLineCleanupFunction *value) { g_out_line_cleanup = value; }
	virtual int Outside() const									{ return g_outside; }
	virtual void SetOutside(int value)							{ g_outside = value; }
	virtual bool Overflow() const								{ return g_overflow; }
	virtual void SetOverflow(bool value)						{ g_overflow = value; }
	virtual bool Overlay3D() const								{ return g_overlay_3d; }
	virtual void SetOverlay3D(bool value)						{ g_overlay_3d = value; }
	virtual bool FractalOverwrite() const						{ return g_fractal_overwrite; }
	virtual void SetFractalOverwrite(bool value)				{ g_fractal_overwrite = value; }
	virtual double OrbitX3rd() const							{ return g_orbit_x_3rd; }
	virtual void SetOrbitX3rd(double value)						{ g_orbit_x_3rd = value; }
	virtual double OrbitXMax() const							{ return g_orbit_x_max; }
	virtual void SetOrbitXMax(double value)						{ g_orbit_x_max = value; }
	virtual double OrbitXMin() const							{ return g_orbit_x_min; }
	virtual void SetOrbitXMin(double value)						{ g_orbit_x_min = value; }
	virtual double OrbitY3rd() const							{ return g_orbit_y_3rd; }
	virtual void SetOrbitY3rd(double value)						{ g_orbit_y_3rd = value; }
	virtual double OrbitYMax() const							{ return g_orbit_y_max; }
	virtual void SetOrbitYMax(double value)						{ g_orbit_y_max = value; }
	virtual double OrbitYMin() const							{ return g_orbit_y_min; }
	virtual void SetOrbitYMin(double value)						{ g_orbit_y_min = value; }
	virtual double const *Parameters() const					{ return g_parameters; }
	virtual void SetParameters(double const *value)				{ /*g_parameters = value;*/ }
	virtual double ParameterRangeX() const						{ return g_parameter_range_x; }
	virtual void SetParameterRangeX(double value)				{ g_parameter_range_x = value; }
	virtual double ParameterRangeY() const						{ return g_parameter_range_y; }
	virtual void SetParameterRangeY(double value)				{ g_parameter_range_y = value; }
	virtual double ParameterZoom() const						{ return g_parameter_zoom; }
	virtual void SetParameterZoom(double value)					{ g_parameter_zoom = value; }
	virtual ComplexD Parameter2() const							{ return g_parameter2; }
	virtual void SetParameter2(ComplexD value)					{ g_parameter2 = value; }
	virtual ComplexD Parameter() const							{ return g_parameter; }
	virtual void SetParameter(ComplexD value)					{ g_parameter = value; }
	virtual int Passes() const									{ return g_passes; }
	virtual void SetPasses(int value)							{ g_passes = value; }
	virtual int PatchLevel() const								{ return g_patch_level; }
	virtual void SetPatchLevel(int value)						{ g_patch_level = value; }
	virtual int PeriodicityCheck() const						{ return g_periodicity_check; }
	virtual void SetPeriodicityCheck(int value)					{ g_periodicity_check = value; }
	virtual PlotColorFunction *PlotColor() const				{ return g_plot_color; }
	virtual void SetPlotColor(PlotColorFunction *value)			{ g_plot_color = value; }
	virtual double PlotMx1() const								{ return g_plot_mx1; }
	virtual void SetPlotMx1(double value)						{ g_plot_mx1 = value; }
	virtual double PlotMx2() const								{ return g_plot_mx2; }
	virtual void SetPlotMx2(double value)						{ g_plot_mx2 = value; }
	virtual double PlotMy1() const								{ return g_plot_my1; }
	virtual void SetPlotMy1(double value)						{ g_plot_my1 = value; }
	virtual double PlotMy2() const								{ return g_plot_my2; }
	virtual void SetPlotMy2(double value)						{ g_plot_my2 = value; }
	virtual bool Potential16Bit() const							{ return g_potential_16bit; }
	virtual void SetPotential16Bit(bool value)					{ g_potential_16bit = value; }
	virtual bool PotentialFlag() const							{ return g_potential_flag; }
	virtual void SetPotentialFlag(bool value)					{ g_potential_flag = value; }
	virtual double const *PotentialParameter() const			{ return g_potential_parameter; }
	virtual void SetPotentialParameter(double const *value)		{ /*g_potential_parameter = value;*/ }
	virtual int Px() const										{ return g_px; }
	virtual void SetPx(int value)								{ g_px = value; }
	virtual int Py() const										{ return g_py; }
	virtual void SetPy(int value)								{ g_py = value; }
	virtual int ParameterBoxCount() const						{ return g_parameter_box_count; }
	virtual void SetParameterBoxCount(int value)				{ g_parameter_box_count = value; }
	virtual int PseudoX() const									{ return g_pseudo_x; }
	virtual void SetPseudoX(int value)							{ g_pseudo_x = value; }
	virtual int PseudoY() const									{ return g_pseudo_y; }
	virtual void SetPseudoY(int value)							{ g_pseudo_y = value; }
	virtual PlotColorPutColorFunction *PlotColorPutColor() const { return g_plot_color_put_color; }
	virtual void SetPlotColorPutColor(PlotColorPutColorFunction *value) { g_plot_color_put_color = value; }
	virtual ComplexD Power() const								{ return g_power; }
	virtual void SetPower(ComplexD value)						{ g_power = value; }
	virtual bool QuickCalculate() const							{ return g_quick_calculate; }
	virtual void SetQuickCalculate(bool value)					{ g_quick_calculate = value; }
	virtual int const *Ranges() const							{ return g_ranges; }
	virtual void SetRanges(int const *value)					{ /*g_ranges = value;*/ }
	virtual int RangesLength() const							{ return g_ranges_length; }
	virtual void SetRangesLength(int value)						{ g_ranges_length = value; }
	virtual long RealColorIter() const							{ return g_real_color_iter; }
	virtual void SetRealColorIter(long value)					{ g_real_color_iter = value; }
	virtual int Release() const									{ return g_release; }
	virtual void SetRelease(int value)							{ g_release = value; }
	virtual int ResaveMode() const								{ return g_resave_mode; }
	virtual void SetResaveMode(int value)						{ g_resave_mode = value; }
	virtual bool ResetPeriodicity() const						{ return g_reset_periodicity; }
	virtual void SetResetPeriodicity(bool value)				{ g_reset_periodicity = value; }
	virtual char const *ResumeInfo() const						{ return g_resume_info; }
	virtual void SetResumeInfo(char const *value)				{ /*g_resume_info = value;*/ }
	virtual int ResumeLength() const							{ return g_resume_length; }
	virtual void SetResumeLength(int value)						{ g_resume_length = value; }
	virtual bool Resuming() const								{ return g_resuming; }
	virtual void SetResuming(bool value)						{ g_resuming = value; }
	virtual bool UseFixedRandomSeed() const						{ return g_use_fixed_random_seed; }
	virtual void SetUseFixedRandomSeed(bool value)				{ g_use_fixed_random_seed = value; }
	virtual char const *RleBuffer() const						{ return g_rle_buffer; }
	virtual void SetRleBuffer(char const *value)				{ /*g_rle_buffer = value;*/ }
	virtual int RotateHi() const								{ return g_rotate_hi; }
	virtual void SetRotateHi(int value)							{ g_rotate_hi = value; }
	virtual int RotateLo() const								{ return g_rotate_lo; }
	virtual void SetRotateLo(int value)							{ g_rotate_lo = value; }
	virtual int Row() const										{ return g_row; }
	virtual void SetRow(int value)								{ g_row = value; }
	virtual int RowCount() const								{ return g_row_count; }
	virtual void SetRowCount(int value)							{ g_row_count = value; }
	virtual double RqLimit2() const								{ return g_rq_limit2; }
	virtual void SetRqLimit2(double value)						{ g_rq_limit2 = value; }
	virtual double RqLimit() const								{ return g_rq_limit; }
	virtual void SetRqLimit(double value)						{ g_rq_limit = value; }
	virtual int RandomSeed() const								{ return g_random_seed; }
	virtual void SetRandomSeed(int value)						{ g_random_seed = value; }
	virtual long SaveBase() const								{ return g_save_base; }
	virtual void SetSaveBase(long value)						{ g_save_base = value; }
	virtual ComplexD SaveC() const								{ return g_save_c; }
	virtual void SetSaveC(ComplexD value)						{ g_save_c = value; }
	virtual long SaveTicks() const								{ return g_save_ticks; }
	virtual void SetSaveTicks(long value)						{ g_save_ticks = value; }
	virtual int SaveRelease() const								{ return g_save_release; }
	virtual void SetSaveRelease(int value)						{ g_save_release = value; }
	virtual float ScreenAspectRatio() const						{ return g_screen_aspect_ratio; }
	virtual void SetScreenAspectRatio(float value)				{ g_screen_aspect_ratio = value; }
	virtual int ScreenHeight() const							{ return g_screen_height; }
	virtual void SetScreenHeight(int value)						{ g_screen_height = value; }
	virtual int ScreenWidth() const								{ return g_screen_width; }
	virtual void SetScreenWidth(int value)						{ g_screen_width = value; }
	virtual bool SetOrbitCorners() const						{ return g_set_orbit_corners; }
	virtual void SetSetOrbitCorners(bool value)					{ g_set_orbit_corners = value; }
	virtual int ShowDot() const									{ return g_show_dot; }
	virtual void SetShowDot(int value)							{ g_show_dot = value; }
	virtual ShowFileType ShowFile() const						{ return g_show_file; }
	virtual void SetShowFile(ShowFileType value)				{ g_show_file = value; }
	virtual bool ShowOrbit() const								{ return g_show_orbit; }
	virtual void SetShowOrbit(bool value)						{ g_show_orbit = value; }
	virtual double SinX() const									{ return g_sin_x; }
	virtual void SetSinX(double value)							{ g_sin_x = value; }
	virtual int SizeDot() const									{ return g_size_dot; }
	virtual void SetSizeDot(int value)							{ g_size_dot = value; }
	virtual short const *SizeOfString() const					{ return g_size_of_string; }
	virtual void SetSizeOfString(short const *value)			{ /*g_size_of_string = value;*/ }
	virtual int SkipXDots() const								{ return g_skip_x_dots; }
	virtual void SetSkipXDots(int value)						{ g_skip_x_dots = value; }
	virtual int SkipYDots() const								{ return g_skip_y_dots; }
	virtual void SetSkipYDots(int value)						{ g_skip_y_dots = value; }
	virtual int GaussianSlope() const							{ return g_gaussian_slope; }
	virtual void SetGaussianSlope(int value)					{ g_gaussian_slope = value; }
	virtual PlotColorStandardFunction *PlotColorStandard() const { return g_plot_color_standard; }
	virtual void SetPlotColorStandard(PlotColorStandardFunction *value) { g_plot_color_standard = value; }
	virtual bool StartShowOrbit() const							{ return g_start_show_orbit; }
	virtual void SetStartShowOrbit(bool value)					{ g_start_show_orbit = value; }
	virtual bool StartedResaves() const							{ return g_started_resaves; }
	virtual void SetStartedResaves(bool value)					{ g_started_resaves = value; }
	virtual int StopPass() const								{ return g_stop_pass; }
	virtual void SetStopPass(int value)							{ g_stop_pass = value; }
	virtual unsigned int const *StringLocation() const			{ return g_string_location; }
	virtual void SetStringLocation(unsigned int const *value)	{ /*g_string_location = value;*/ }
	virtual BYTE const *Suffix() const							{ return g_suffix; }
	virtual void SetSuffix(BYTE const *value)					{ /*g_suffix = value;*/ }
	virtual double Sx3rd() const								{ return g_sx_3rd; }
	virtual void SetSx3rd(double value)							{ g_sx_3rd = value; }
	virtual double SxMax() const								{ return g_sx_max; }
	virtual void SetSxMax(double value)							{ g_sx_max = value; }
	virtual double SxMin() const								{ return g_sx_min; }
	virtual void SetSxMin(double value)							{ g_sx_min = value; }
	virtual int ScreenXOffset() const							{ return g_screen_x_offset; }
	virtual void SetScreenXOffset(int value)					{ g_screen_x_offset = value; }
	virtual double Sy3rd() const								{ return g_sy_3rd; }
	virtual void SetSy3rd(double value)							{ g_sy_3rd = value; }
	virtual double SyMax() const								{ return g_sy_max; }
	virtual void SetSyMax(double value)							{ g_sy_max = value; }
	virtual double SyMin() const								{ return g_sy_min; }
	virtual void SetSyMin(double value)							{ g_sy_min = value; }
	virtual SymmetryType Symmetry() const						{ return g_symmetry; }
	virtual void SetSymmetry(SymmetryType value)				{ g_symmetry = value; }
	virtual int ScreenYOffset() const							{ return g_screen_y_offset; }
	virtual void SetScreenYOffset(int value)					{ g_screen_y_offset = value; }
	virtual bool TabDisplayEnabled() const						{ return g_tab_display_enabled; }
	virtual void SetTabDisplayEnabled(bool value)				{ g_tab_display_enabled = value; }
	virtual bool TargaOutput() const							{ return g_targa_output; }
	virtual void SetTargaOutput(bool value)						{ g_targa_output = value; }
	virtual bool TargaOverlay() const							{ return g_targa_overlay; }
	virtual void SetTargaOverlay(bool value)					{ g_targa_overlay = value; }
	virtual double TempSqrX() const								{ return g_temp_sqr_x; }
	virtual void SetTempSqrX(double value)						{ g_temp_sqr_x = value; }
	virtual double TempSqrY() const								{ return g_temp_sqr_y; }
	virtual void SetTempSqrY(double value)						{ g_temp_sqr_y = value; }
	virtual int TextCbase() const								{ return g_text_cbase; }
	virtual void SetTextCbase(int value)						{ g_text_cbase = value; }
	virtual int TextCol() const									{ return g_text_col; }
	virtual void SetTextCol(int value)							{ g_text_col = value; }
	virtual int TextRbase() const								{ return g_text_rbase; }
	virtual void SetTextRbase(int value)						{ g_text_rbase = value; }
	virtual int TextRow() const									{ return g_text_row; }
	virtual void SetTextRow(int value)							{ g_text_row = value; }
	virtual unsigned int RhisGenerationRandomSeed() const		{ return g_this_generation_random_seed; }
	virtual void SetThisGenerationRandomSeed(int value)			{ g_this_generation_random_seed = value; }
	virtual bool ThreePass() const								{ return g_three_pass; }
	virtual void SetThreePass(bool value)						{ g_three_pass = value; }
	virtual double Threshold() const							{ return g_threshold; }
	virtual void SetThreshold(double value)						{ g_threshold = value; }
	virtual TimedSaveType TimedSave() const						{ return g_timed_save; }
	virtual void SetTimedSave(TimedSaveType value)				{ g_timed_save = value; }
	virtual bool TimerFlag() const								{ return g_timer_flag; }
	virtual void SetTimerFlag(bool value)						{ g_timer_flag = value; }
	virtual long TimerInterval() const							{ return g_timer_interval; }
	virtual void SetTimerInterval(long value)					{ g_timer_interval = value; }
	virtual long TimerStart() const								{ return g_timer_start; }
	virtual void SetTimerStart(long value)						{ g_timer_start = value; }
	virtual ComplexD TempZ() const								{ return g_temp_z; }
	virtual void SetTempZ(ComplexD value)						{ g_temp_z = value; }
	virtual int TotalPasses() const								{ return g_total_passes; }
	virtual void SetTotalPasses(int value)						{ g_total_passes = value; }
	virtual int const *FunctionIndex() const					{ return g_function_index; }
	virtual void SetFunctionIndex(int const *value)				{ /*g_function_index = value;*/ }
	virtual bool TrueColor() const								{ return g_true_color; }
	virtual void SetTrueColor(bool value)						{ g_true_color = value; }
	virtual bool TrueModeIterates() const						{ return g_true_mode_iterates; }
	virtual void SetTrueModeIterates(bool value)				{ g_true_mode_iterates = value; }
	virtual char const *TextStack() const						{ return g_text_stack; }
	virtual void SetTextStack(char const *value)				{ /*g_text_stack = value;*/ }
	virtual double TwoPi() const								{ return g_two_pi; }
	virtual void SetTwoPi(double value)							{ g_two_pi = value; }
	virtual UserInterfaceState const &UiState() const			{ return g_ui_state; }
	virtual UserInterfaceState &UiState()						{ return g_ui_state; }
	virtual InitialZType UseInitialOrbitZ() const				{ return g_use_initial_orbit_z; }
	virtual void SetUseInitialOrbitZ(InitialZType value)		{ g_use_initial_orbit_z = value; }
	virtual bool UseCenterMag() const							{ return g_use_center_mag; }
	virtual void SetUseCenterMag(bool value)					{ g_use_center_mag = value; }
	virtual bool UseOldPeriodicity() const						{ return g_use_old_periodicity; }
	virtual void SetUseOldPeriodicity(bool value)				{ g_use_old_periodicity = value; }
	virtual bool UsingJiim() const								{ return g_using_jiim; }
	virtual void SetUsingJiim(bool value)						{ g_using_jiim = value; }
	virtual int UserBiomorph() const							{ return g_user_biomorph; }
	virtual void SetUserBiomorph(int value)						{ g_user_biomorph = value; }
	virtual long UserDistanceTest() const						{ return g_user_distance_test; }
	virtual void SetUserDistanceTest(long value)				{ g_user_distance_test = value; }
	virtual bool UserFloatFlag() const							{ return g_user_float_flag; }
	virtual void SetUserFloatFlag(bool value)					{ g_user_float_flag = value; }
	virtual int UserPeriodicityCheck() const					{ return g_user_periodicity_check; }
	virtual void SetUserPeriodicityCheck(int value)				{ g_user_periodicity_check = value; }
	virtual int VxDots() const									{ return g_vx_dots; }
	virtual void SetVxDots(int value)							{ g_vx_dots = value; }
	virtual int WhichImage() const								{ return g_which_image; }
	virtual void SetWhichImage(int value)						{ g_which_image = value; }
	virtual float WidthFp() const								{ return g_width_fp; }
	virtual void SetWidthFp(float value)						{ g_width_fp = value; }
	virtual int XDots() const									{ return g_x_dots; }
	virtual void SetXDots(int value)							{ g_x_dots = value; }
	virtual int XShift1() const									{ return g_x_shift1; }
	virtual void SetXShift1(int value)							{ g_x_shift1 = value; }
	virtual int XShift() const									{ return g_x_shift; }
	virtual void SetXShift(int value)							{ g_x_shift = value; }
	virtual int XxAdjust1() const								{ return g_xx_adjust1; }
	virtual void SetXxAdjust1(int value)						{ g_xx_adjust1 = value; }
	virtual int XxAdjust() const								{ return g_xx_adjust; }
	virtual void SetXxAdjust(int value)							{ g_xx_adjust = value; }
	virtual int YDots() const									{ return g_y_dots; }
	virtual void SetYDots(int value)							{ g_y_dots = value; }
	virtual int YShift() const									{ return g_y_shift; }
	virtual void SetYShift(int value)							{ g_y_shift = value; }
	virtual int YShift1() const									{ return g_y_shift1; }
	virtual void SetYShift1(int value)							{ g_y_shift1 = value; }
	virtual int YyAdjust() const								{ return g_yy_adjust; }
	virtual void SetYyAdjust(int value)							{ g_yy_adjust = value; }
	virtual int YyAdjust1() const								{ return g_yy_adjust1; }
	virtual void SetYyAdjust1(int value)						{ g_yy_adjust1 = value; }
	virtual double Zbx() const									{ return g_zbx; }
	virtual void SetZbx(double value)							{ g_zbx = value; }
	virtual double Zby() const									{ return g_zby; }
	virtual void SetZby(double value)							{ g_zby = value; }
	virtual double ZDepth() const								{ return g_z_depth; }
	virtual void SetZDepth(double value)						{ g_z_depth = value; }
	virtual int ZDots() const									{ return g_z_dots; }
	virtual void SetZDots(int value)							{ g_z_dots = value; }
	virtual int ZRotate() const									{ return g_z_rotate; }
	virtual void SetZRotate(int value)							{ g_z_rotate = value; }
	virtual double ZSkew() const								{ return g_z_skew; }
	virtual void SetZSkew(double value)							{ g_z_skew = value; }
	virtual double ZWidth() const								{ return g_z_width; }
	virtual void SetZWidth(double value)						{ g_z_width = value; }
	virtual bool ZoomOff() const								{ return g_zoom_off; }
	virtual void SetZoomOff(bool value)							{ g_zoom_off = value; }
	virtual ZoomBox const &Zoom() const							{ return g_zoomBox; }
	virtual ZoomBox &Zoom()										{ return g_zoomBox; }
	virtual void SetZoom(ZoomBox const &value)					{ g_zoomBox = value; }
	virtual SoundState const &Sound() const						{ return g_sound_state; }
	virtual SoundState &Sound()									{ return g_sound_state; }
	virtual EscapeTimeState const &EscapeTime() const			{ return g_escape_time_state; }
	virtual EscapeTimeState &EscapeTime()						{ return g_escape_time_state; }
	virtual int BfMath() const									{ return g_bf_math; }
	virtual void SetBfMath(int value)							{ g_bf_math = value; }
	virtual bf_t SxMinBf() const								{ return g_sx_min_bf; }
	virtual void SetSxMinBf(bf_t value)							{ g_sx_min_bf = value; }
	virtual bf_t SxMaxBf() const								{ return g_sx_max_bf; }
	virtual void SetSxMaxBf(bf_t value)							{ g_sx_max_bf = value; }
	virtual bf_t SyMinBf() const								{ return g_sy_min_bf; }
	virtual void SetSyMinBf(bf_t value)							{ g_sy_min_bf = value; }
	virtual bf_t SyMaxBf() const								{ return g_sy_max_bf; }
	virtual void SetSyMaxBf(bf_t value)							{ g_sy_max_bf = value; }
	virtual bf_t Sx3rdBf() const								{ return g_sx_3rd_bf; }
	virtual void SetSx3rdBf(bf_t value)							{ g_sx_3rd_bf = value; }
	virtual bf_t Sy3rdBf() const								{ return g_sy_3rd_bf; }
	virtual void SetSy3rdBf(bf_t value)							{ g_sy_3rd_bf = value; }
	virtual BrowseState const &Browse() const					{ return g_browse_state; }
	virtual BrowseState &Browse()								{ return g_browse_state; }
	virtual evolution_info const *Evolve() const				{ return g_evolve_info; }
	virtual evolution_info *Evolve()							{ return g_evolve_info; }
	virtual void SetEvolve(evolution_info *value) 				{ g_evolve_info = value; }
	virtual unsigned int ThisGenerationRandomSeed() const		{ return g_this_generation_random_seed; }
	virtual void SetThisGenerationRandomSeed(unsigned int value) { g_this_generation_random_seed = value; }
	virtual GIFViewFunction *GIFView() const					{ return gifview; }
	virtual OutLineFunction *OutLinePotential() const			{ return out_line_potential; }
	virtual OutLineFunction *OutLineSound() const				{ return out_line_sound; }
	virtual OutLineFunction *OutLineRegular() const				{ return out_line; }
	virtual OutLineCleanupFunction *OutLineCleanupNull() const	{ return out_line_cleanup_null; }
	virtual OutLineFunction *OutLine3D() const					{ return out_line_3d; }
	virtual OutLineFunction *OutLineCompare() const				{ return out_line_compare; }
	virtual std::string const &FractDir1() const				{ return g_fract_dir1; }
	virtual void SetFractDir1(std::string const &value)			{ g_fract_dir1 = value; }
	virtual std::string const &FractDir2() const				{ return g_fract_dir2; }
	virtual void SetFractDir2(std::string const &value)			{ g_fract_dir2 = value; }
	virtual std::string const &ReadName() const					{ return g_read_name; }
	virtual std::string &ReadName()								{ return g_read_name; }
	virtual void SetReadName(std::string const &value)			{ g_read_name = value; }
	virtual std::string &GIFMask()								{ return g_gif_mask; }
	virtual ViewWindow const &View() const						{ return g_viewWindow; }
	virtual ViewWindow &View()									{ return g_viewWindow; }
	virtual void SetFileNameStackTop(std::string const &value)	{ g_file_name_stack[g_name_stack_ptr] = value; }
	virtual boost::filesystem::path const &WorkDirectory() const { return g_work_dir; }
};

static ExternalsImpl s_externs;
Externals &g_externs(s_externs);
