#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "LSystemParser.h"

class TestData
{
	static const std::string s_snowflake2;
	static const std::string s_l_system_file;
};

BOOST_AUTO_TEST_CASE(LSystemParser_Empty)
{
	const std::string empty = "Empty {\n }\n";
	LSystemParser parser = LSystemParser::StackInstance();
	BOOST_CHECK(parser.Parse(empty));
	BOOST_CHECK_EQUAL(1, parser.Count());
	BOOST_CHECK_EQUAL("Empty", parser.Entry(0)->Id());
}

BOOST_AUTO_TEST_CASE(LSystemParser_Comments1)
{
	const std::string comments = "Comments1 { ; Comment on head\n"
		"}\n";
	LSystemParser parser = LSystemParser::StackInstance();
	BOOST_CHECK(parser.Parse(comments));
	BOOST_CHECK_EQUAL(1, parser.Count());
	const LSystemEntry *entry = parser.Entry(0);
	BOOST_CHECK_EQUAL("Comments1", entry->Id());
}

#if 0
BOOST_AUTO_TEST_CASE(LSystemParser_Angle)
{
	const std::string angle = "Angle {\n"
		"  angle 15\n"
		";\n"
		"}\n";
	LSystemParser parser = LSystemParser::StackInstance();
	BOOST_CHECK(parser.Parse(angle));
	BOOST_CHECK_EQUAL(1, parser.Count());
	const LSystemEntry *entry = parser.Entry(0);
	BOOST_CHECK_EQUAL("Angle", entry->Id());
	BOOST_CHECK_EQUAL(15, entry->Angle());
}

BOOST_AUTO_TEST_CASE(LSystemParser_Axiom)
{
	const std::string axiom = "Axiom {\naxiom F-F\n;\n}\n";
	LSystemParser parser = LSystemParser::StackInstance();
	BOOST_CHECK(parser.Parse(axiom));
	BOOST_CHECK_EQUAL(1, parser.Count());
	const LSystemEntry *entry = parser.Entry(0);
	BOOST_CHECK_EQUAL("Axiom", entry->Id());
	BOOST_CHECK_EQUAL("F-F", entry->Axiom());
}

BOOST_AUTO_TEST_CASE(LSystemParser_Production)
{
	const std::string production = "Production {\nF=FF\n}\n";
	LSystemParser parser = LSystemParser::StackInstance();
	BOOST_CHECK(parser.Parse(production));
	BOOST_CHECK_EQUAL(1, parser.Count());
	const LSystemEntry *entry = parser.Entry(0);
	BOOST_CHECK_EQUAL("Production", entry->Id());
	BOOST_CHECK_EQUAL(1, entry->ProductionCount());
	const LSystemProduction rule = entry->Production(0);
	BOOST_CHECK_EQUAL("F", rule.Symbol());
	BOOST_CHECK_EQUAL("FF", rule.Production());
}

BOOST_AUTO_TEST_CASE(LSystemParser_Production2)
{
	const std::string production2 = "Production2 {\n"
		"A = BC\n"
		"B = F-F\n"
		"C = F+F\n"
		"}\n";
	LSystemParser parser = LSystemParser::StackInstance();
	BOOST_CHECK(parser.Parse(production2));
	BOOST_CHECK_EQUAL(1, parser.Count());
	const LSystemEntry *entry = parser.Entry(0);
	BOOST_CHECK_EQUAL("Production2", entry->Id());
	BOOST_CHECK_EQUAL(3, entry->ProductionCount());
	{
		const LSystemProduction rule = entry->Production(0);
		BOOST_CHECK_EQUAL("A", rule.Symbol());
		BOOST_CHECK_EQUAL("BC", rule.Production());
	}
	{
		const LSystemProduction rule = entry->Production(1);
		BOOST_CHECK_EQUAL("B", rule.Symbol());
		BOOST_CHECK_EQUAL("F-F", rule.Production());
	}
	{
		const LSystemProduction rule = entry->Production(2);
		BOOST_CHECK_EQUAL("C", rule.Symbol());
		BOOST_CHECK_EQUAL("F+F", rule.Production());
	}
}

BOOST_AUTO_TEST_CASE(LSystemParser_Koch1)
{
	const std::string koch1 =
		"Koch1 { ; Adrian Mariano\n"
		"; from The Fractal Geometry of Nature by Mandelbrot\n"
		"  Angle 6\n"
		"  Axiom F--F--F\n"
		"  F=F+F--F+F\n"
		"  }\n"
		"\n";
	LSystemParser parser = LSystemParser::StackInstance();

	BOOST_CHECK(parser.Parse(koch1));
	BOOST_CHECK_EQUAL(1, parser.Count());
	const LSystemEntry *entry = parser.Entry(0);
	BOOST_CHECK_EQUAL("Koch1", entry->Id());
	BOOST_CHECK_EQUAL(6, entry->Angle());
	BOOST_CHECK_EQUAL("F--F--F", entry->Axiom());
	BOOST_CHECK_EQUAL(1, entry->ProductionCount());
	const LSystemProduction rule = entry->Production(0);
	BOOST_CHECK_EQUAL("F", rule.Symbol());
	BOOST_CHECK_EQUAL("F+F--F+F", rule.Production());
}

BOOST_AUTO_TEST_CASE(LSystemParser_Snowflake2)
{
	const std::string snowflake2 =
		"Snowflake2 { ; Adrian Mariano\n"
		"; from The Fractal Geometry of Nature by Mandelbrot\n"
		"  angle 12\n"
		"  axiom F\n"
		"  F=++!F!F--F--F@IQ3|+F!F--\n"
		"  F=F--F!+++@Q3F@QI3|+F!F@Q3|+F!F\n"
		"  }\n"
		"\n"
		"SnowflakeColor { ; Adrian Mariano\n"
		"; from The Fractal Geometry of Nature by Mandelbrot\n"
		"  angle 12\n"
		"  axiom F\n"
		"  F=--!F<1!F<1++F<1++F<1@IQ3|-F<1!F<1++\n"
		"  F=F<1++F<1!---@Q3F<1@QI3|-F<1!F<1@Q3|-F<1!F<1\n"
		"  F=\n"
		"  }\n"
		"\n";

	LSystemParser parser = LSystemParser::StackInstance();

	BOOST_CHECK(parser.Parse(snowflake2));
	BOOST_CHECK_EQUAL(2, parser.Count());
	const LSystemEntry *entry0 = parser.Entry(0);
	BOOST_CHECK_EQUAL("Snowflake2", entry0->Id());
	BOOST_CHECK_EQUAL(12, entry0->Angle());
	BOOST_CHECK_EQUAL("F", entry0->Axiom());
	BOOST_CHECK_EQUAL(2, entry0->ProductionCount());
	{
		const LSystemProduction rule = entry0->Production(0);
		BOOST_CHECK_EQUAL("F", rule.Symbol());
		BOOST_CHECK_EQUAL("++!F!F--F--F@IQ3|+F!F--", rule.Production());
	}
	{
		const LSystemProduction rule = entry0->Production(1);
		BOOST_CHECK_EQUAL("F", rule.Symbol());
		BOOST_CHECK_EQUAL("F--F!+++@Q3F@QI3|+F!F@Q3|+F!F", rule.Production());
	}

	const LSystemEntry *entry1 = parser.Entry(1);
	BOOST_CHECK_EQUAL("SnowflakeColor", entry1->Id());
	BOOST_CHECK_EQUAL(12, entry1->Angle());
	BOOST_CHECK_EQUAL("F", entry1->Axiom());
	BOOST_CHECK_EQUAL(3, entry1->ProductionCount());
	{
		const LSystemProduction rule = entry1->Production(0);
		BOOST_CHECK_EQUAL("F", rule.Symbol());
		BOOST_CHECK_EQUAL("--!F<1!F<1++F<1++F<1@IQ3|-F<1!F<1++", rule.Production());
	}
	{
		const LSystemProduction rule = entry1->Production(1);
		BOOST_CHECK_EQUAL("F", rule.Symbol());
		BOOST_CHECK_EQUAL("F<1++F<1!---@Q3F<1@QI3|-F<1!F<1@Q3|-F<1!F<1", rule.Production());
	}
	{
		const LSystemProduction rule = entry1->Production(2);
		BOOST_CHECK_EQUAL("F", rule.Symbol());
		BOOST_CHECK_EQUAL("", rule.Production());
	}
}
#endif

const std::string TestData::s_snowflake2 =
	"Snowflake2 { ; Adrian Mariano\n"
	"; from The Fractal Geometry of Nature by Mandelbrot\n"
	"  angle 12\n"
	"  axiom F\n"
	"  F=++!F!F--F--F@IQ3|+F!F--\n"
	"  F=F--F!+++@Q3F@QI3|+F!F@Q3|+F!F\n"
	"  }\n"
	"\n"
	"SnowflakeColor { ; Adrian Mariano\n"
	"; from The Fractal Geometry of Nature by Mandelbrot\n"
	"  angle 12\n"
	"  axiom F\n"
	"  F=--!F<1!F<1++F<1++F<1@IQ3|-F<1!F<1++\n"
	"  F=F<1++F<1!---@Q3F<1@QI3|-F<1!F<1@Q3|-F<1!F<1\n"
	"  F=\n"
	"  }\n"
	"\n";

const std::string TestData::s_l_system_file =
	"Koch1 { ; Adrian Mariano\n"
	"; from The Fractal Geometry of Nature by Mandelbrot\n"
	"  Angle 6\n"
	"  Axiom F--F--F\n"
	"  F=F+F--F+F\n"
	"  }\n"
	"\n"
	"Koch2 { ; Adrian Mariano\n"
	"; from The Fractal Geometry of Nature by Mandelbrot\n"
	"  Angle 12\n"
	"  Axiom F---F---F---F\n"
	"  F=-F+++F---F+\n"
	"  }\n"
	"\n"
	"Koch3 { ; Adrian Mariano\n"
	"; from The Fractal Geometry of Nature by Mandelbrot\n"
	"  Angle 4\n"
	"  Axiom F-F-F-F\n"
	"  F=F-F+F+FF-F-F+F\n"
	"  }\n"
	"\n"
	"Koch6 { ; Adrian Mariano\n"
	"   axiom f+f+f+f\n"
	"   f=f-ff+ff+f+f-f-ff+f+f-f-ff-ff+f\n"
	"   angle 4\n"
	"    }\n"
	"\n"
	"Dragon { ; Adrian Mariano\n"
	"; from The Fractal Geometry of Nature by Mandelbrot\n"
	"  Angle 8\n"
	"  Axiom FX\n"
	"  F=\n"
	"  y=+FX--FY+\n"
	"  x=-FX++FY-\n"
	"  }\n"
	"\n"
	"Peano1 { ; Adrian Mariano\n"
	"; from The Fractal Geometry of Nature by Mandelbrot\n"
	"  Angle 4\n"
	"  Axiom F-F-F-F\n"
	"  F=F-F+F+F+F-F-F-F+F\n"
	"  }\n"
	"\n"
	"Cesaro { ; Adrian Mariano\n"
	"; from The Fractal Geometry of Nature by Mandelbrot\n"
	"  Angle 34\n"
	"  Axiom FX\n"
	"  F=\n"
	"  X=----F!X!++++++++F!X!----\n"
	"  }\n"
	"\n"
	"DoubleCesaro { ; Adrian Mariano\n"
	"; from The Fractal Geometry of Nature by Mandelbrot\n"
	"  Angle 4\n"
	"  axiom D\\90D\\90D\\90D\\90\n"
	"  D=\\42!D!/84!D!\\42\n"
	"  }\n"
	"\n"
	"FlowSnake { ; Adrian Mariano\n"
	"; from The Fractal Geometry of Nature by Mandelbrot\n"
	"  angle=6;\n"
	"  axiom FL\n"
	"  L=FL-FR--FR+FL++FLFL+FR-\",\n"
	"  R=+FL-FRFR--FR-FL++FL+FR\",\n"
	"  F=\n"
	"  }\n"
	"\n"
	"CantorDust { ; Adrian Mariano\n"
	"; from The Fractal Geometry of Nature by Mandelbrot\n"
	"  Angle 6\n"
	"  Axiom F\n"
	"  F=FGF\n"
	"  G=GGG\n"
	"  }\n"
	"\n"
	"Snowflake2 { ; Adrian Mariano\n"
	"; from The Fractal Geometry of Nature by Mandelbrot\n"
	"  angle 12\n"
	"  axiom F\n"
	"  F=++!F!F--F--F@IQ3|+F!F--\n"
	"  F=F--F!+++@Q3F@QI3|+F!F@Q3|+F!F\n"
	"  }\n"
	"\n"
	"SnowflakeColor { ; Adrian Mariano\n"
	"; from The Fractal Geometry of Nature by Mandelbrot\n"
	"  angle 12\n"
	"  axiom F\n"
	"  F=--!F<1!F<1++F<1++F<1@IQ3|-F<1!F<1++\n"
	"  F=F<1++F<1!---@Q3F<1@QI3|-F<1!F<1@Q3|-F<1!F<1\n"
	"  F=\n"
	"  }\n"
	"\n"
	"Island1 { ; Adrian Mariano\n"
	"; from The Fractal Geometry of Nature by Mandelbrot\n"
	"  angle 4\n"
	"  axiom F+F+F+F\n"
	"  F=FFFF-F+F+F-F[-GFF+F+FF+F]FF\n"
	"  G=@8G@I8\n"
	"  }\n"
	"\n"
	"Island2 { ; Adrian Mariano\n"
	"; from The Fractal Geometry of Nature by Mandelbrot\n"
	"  angle 4\n"
	"  axiom f+f+f+f\n"
	"  f=f+gf-ff-f-ff+g+ff-gf+ff+f+ff-g-fff\n"
	"  g=@6G@I6\n"
	"  }\n"
	"\n"
	"Quartet { ; Adrian Mariano\n"
	"; from The Fractal Geometry of Nature by Mandelbrot\n"
	"  angle 4\n"
	"  axiom fb\n"
	"  A=FBFA+HFA+FB-FA\n"
	"  B=FB+FA-FB-JFBFA\n"
	"  F=\n"
	"  H=-\n"
	"  J=+\n"
	"  }\n"
	"\n"
	"SnowFlake1 { ; Adrian Mariano\n"
	"; from The Fractal Geometry of Nature by Mandelbrot\n"
	"  Angle 12\n"
	"  Axiom FR\n"
	"  R=++!FRFU++FU++FU!---@Q3FU|-@IQ3!FRFU!\n"
	"  U=!FRFU!|+@Q3FR@IQ3+++!FR--FR--FRFU!--\n"
	"  F=\n"
	"  }\n"
	"\n"
	"SnowFlake3 { ; Adrian Mariano\n"
	"; from The Fractal Geometry of Nature by Mandelbrot\n"
	"  angle 12\n"
	"  axiom fx\n"
	"  x=++f!x!fy--fx--fy|+@iq3fyf!x!++f!y!++f!y!fx@q3+++f!y!fx\n"
	"  y=fyf!x!+++@iq3fyf!x!++f!x!++f!y!fx@q3|+fx--fy--fxf!y!++\n"
	"  f=\n"
	"  }\n"
	"\n"
	"Tree1 { ; Adrian Mariano\n"
	"; from The Fractal Geometry of Nature by Mandelbrot\n"
	"  angle=12;\n"
	"  axiom +++FX\n"
	"  X=@.6[-FX]+FX\n"
	"  }\n"
	"\n"
	"Peano2 { ; Adrian Mariano\n"
	"; from The Fractal Geometry of Nature by Mandelbrot\n"
	"  Angle 8\n"
	"  Axiom FXY++F++FXY++F\n"
	"  X=XY@Q2-F@IQ2-FXY++F++FXY\n"
	"  Y=-@Q2F-@IQ2FXY\n"
	"  }\n"
	"\n"
	"Sierpinski1 { ; Adrian Mariano\n"
	"; from The Fractal Geometry of Nature by Mandelbrot\n"
	"  angle 3\n"
	"  axiom F\n"
	"  F=FXF\n"
	"  X=+FXF-FXF-FXF+\n"
	"  }\n"
	"\n"
	"Koch4 { ; Adrian Mariano\n"
	"; from The Fractal Geometry of Nature by Mandelbrot\n"
	"  angle 12\n"
	"  axiom f++++f++++f\n"
	"  f=+f--f++f-\n"
	"  }\n"
	"\n"
	"\n"
	"Plant07 { ; Ken Philip, from The Science of Fractal Images p.285b\n"
	"  axiom Z\n"
	"  z=zFX[+Z][-Z]\n"
	"  x=x[-FFF][+FFF]FX\n"
	"  angle 14\n"
	"  }\n"
	"\n"
	"Plant08 { ; Ken Philip, from The Science of Fractal Images, p.286\n"
	"  axiom SLFFF\n"
	"  s=[+++Z][---Z]TS\n"
	"  z=+H[-Z]L\n"
	"  h=-Z[+H]L\n"
	"  t=TL\n"
	"  l=[-FFF][+FFF]F\n"
	"  angle 20\n"
	"  }\n"
	"\n"
	"Hilbert { ; Ken Philip, from The Science of Fractal Images\n"
	"  axiom x\n"
	"  x=-YF+XFX+FY-\n"
	"  y=+XF-YFY-FX+\n"
	"  angle 4\n"
	"  }\n"
	"\n"
	"Sierpinski3 { ; From Jim Hanan via Corbit\n"
	"  axiom F-F-F\n"
	"  f=F[-F]F\n"
	"  angle 3\n"
	"  }\n"
	"\n"
	"\n"
	"Peano3 {\n"
	"  axiom x\n"
	"  x=XFYFX+F+YFXFY-F-XFYFX\n"
	"  y=YFXFY-F-XFYFX+F+YFXFY\n"
	"  angle 4\n"
	"  }\n"
	"\n"
	"Koch5 {\n"
	"  axiom f+F+F+F\n"
	"  f=F+F-F-FFF+F+F-F\n"
	"  angle 4\n"
	"  }\n"
	"\n"
	"Sierpinski2 { ; from The Science of Fractal Images\n"
	"  axiom FXF--FF--FF\n"
	"  f=FF\n"
	"  x=--FXF++FXF++FXF--\n"
	"  angle 6\n"
	"  }\n"
	"\n"
	"SierpinskiSquare {\n"
	"  axiom F+F+F+F\n"
	"  f=FF+F+F+F+FF\n"
	"  angle 4\n"
	"  }\n"
	"\n"
	"\n"
	"Pentagram { ; created by Adrian Mariano\n"
	"  angle 10\n"
	"  axiom fx++fx++fx++fx++fx\n"
	"; f=f[++++@1.618033989f]\n"
	"  x=[++++@i1.618033989f@.618033989f!x!@i.618033989f]\n"
	"  }\n"
	"\n"
	"\n"
	"QuadKoch { ; Adrian Mariano, from the Algorithmic Beauty of Plants\n"
	"	   ; Quadratic Koch island, Figure 1.7a p.9\n"
	"  angle 4\n"
	"  AXIOM F-F-F-F-\n"
	"  F=F+FF-FF-F-F+F+FF-F-F+F+FF+FF-F\n"
	"  }\n"
	"\n"
	"Fass1 { ; Adrian Mariano, from the Algorithmic Beauty of Plants\n"
	"	; FASS curve (3x3 tiles form macrotile), Figure 1.16a p.17\n"
	"  axiom -l\n"
	"  angle 4\n"
	"  L=LF+RFR+FL-F-LFLFL-FRFR+\n"
	"  R=-LFLF+RFRFR+F+RF-LFL-FR\n"
	"  }\n"
	"\n"
	"Fass2 { ; Adrian Mariano, from the Algorithmic Beauty of Plants\n"
	"	; FASS curve (4x4 tiles form macrotile), Figure 1.16b p.17\n"
	"  angle 4\n"
	"  axiom -l\n"
	"  L=LFLF+RFR+FLFL-FRF-LFL-FR+F+RF-LFL-FRFRFR+\n"
	"  R=-LFLFLF+RFR+FL-F-LF+RFR+FLF+RFRF-LFL-FRFR\n"
	"  }\n"
	"\n"
	"QuadGosper { ; Adrian Mariano, from the Algorithmic Beauty of Plants\n"
	"	     ; Quadratic Gosper curve, Figure 1.11b p.12\n"
	"  angle 4\n"
	"  axiom -Fr\n"
	"  l=FlFl-Fr-Fr+Fl+Fl-Fr-FrFl+Fr+FlFlFr-Fl+Fr+FlFl+Fr-FlFr-Fr-Fl+Fl+FrFr-\n"
	"  r=+FlFl-Fr-Fr+Fl+FlFr+Fl-FrFr-Fl-Fr+FlFrFr-Fl-FrFl+Fl+Fr-Fr-Fl+Fl+FrFr\n"
	"  f=\n"
	"  }\n"
	"\n"
	"Plant01 { ; Adrian Mariano, from the Algorithmic Beauty of Plants\n"
	"	 ; Plant-like structure, figure 1.24a p.25\n"
	"	 ; also p.285a The Science of Fractal Images\n"
	"  angle 14\n"
	"  axiom f\n"
	"  f=F[+F]F[-F]F\n"
	"  }\n"
	"\n"
	"Plant02 { ; Adrian Mariano, from the Algorithmic Beauty of Plants\n"
	"	 ; Plant-like structure, figure 1.24b p.25\n"
	"  angle 18\n"
	"  axiom f\n"
	"  f=F[+F]F[-F][F]\n"
	"  }\n"
	"\n"
	"Plant03 { ; Adrian Mariano, from the Algorithmic Beauty of Plants\n"
	"	 ; Plant-like structure, figure 1.24c p.25\n"
	"  angle 16\n"
	"  axiom f\n"
	"  f=FF-[-F+F+F]+[+F-F-F]\n"
	"  }\n"
	"\n"
	"Plant04 { ; Adrian Mariano, from the Algorithmic Beauty of Plants\n"
	"	 ; Plant-like structure, figure 1.24d p.25\n"
	"  angle 18\n"
	"  axiom x\n"
	"  X=F[+X]F[-X]+X\n"
	"  F=FF\n"
	"  }\n"
	"\n"
	"Plant05 { ; Adrian Mariano, from the Algorithmic Beauty of Plants\n"
	"	 ; Plant-like structure, figure 1.24e p.25\n"
	"  angle 14\n"
	"  axiom x\n"
	"  X=f[+X][-X]FX\n"
	"  F=FF\n"
	"  }\n"
	"\n"
	"Plant06 { ; Adrian Mariano, from the Algorithmic Beauty of Plants\n"
	"	 ; Plant-like structure, figure 1.24f p.25\n"
	"  angle 16\n"
	"  axiom x\n"
	"  X=F-[[X]+X]+F[+FX]-X\n"
	"  F=FF\n"
	"  }\n"
	"\n"
	"Plant09 { ; Adrian Mariano\n"
	"   axiom y\n"
	"   x=X[-FFF][+FFF]FX\n"
	"   y=YFX[+Y][-Y]\n"
	"   angle 14\n"
	"}\n"
	"\n"
	"Plant10 { ; Adrian Mariano\n"
	"   axiom f\n"
	"   f=f[+ff][-ff]f[+ff][-ff]f\n"
	"   angle 10\n"
	"   }\n"
	"\n"
	"\n"
	"Plant11 { ; Adrian Mariano\n"
	"   axiom f\n"
	"   f=F[+F[+F][-F]F][-F[+F][-F]F]F[+F][-F]F\n"
	"   angle 12\n"
	"   }\n"
	"\n"
	"Curve1 { ; Adrian Mariano, from the Algorithmic Beauty of Plants\n"
	"	 ; curve from figure 1.9a p.10\n"
	"  angle 4\n"
	"  axiom F-F-F-F-\n"
	"  f=FF-F-F-F-F-F+F\n"
	"  }\n"
	"\n"
	"Curve2 { ; Adrian Mariano, from the Algorithmic Beauty of Plants\n"
	"  angle 4\n"
	"  axiom F-F-F-F-\n"
	"  f=FF-F+F-F-FF\n"
	"  }\n"
	"\n"
	"Curve3 { ; Adrian Mariano, from the Algorithmic Beauty of Plants\n"
	"	 ; curve from figure 1.9e p.10\n"
	"  axiom F-F-F-F-\n"
	"  angle 4\n"
	"  F=F-FF--F-F\n"
	"  }\n"
	"\n"
	"Curve4 { ; Adrian Mariano\n"
	"   axiom yf\n"
	"   x=YF+XF+Y\n"
	"   y=XF-YF-X\n"
	"   angle 6\n"
	"   }\n"
	"\n"
	"Leaf1 { ; Adrian Mariano, from the Algorithmic Beauty of Plants\n"
	"	; Compound leaf with alternating branches, Figure 5.12b p.130\n"
	"  angle 8\n"
	"  axiom x\n"
	"  a=n\n"
	"  n=o\n"
	"  o=p\n"
	"  p=x\n"
	"  b=e\n"
	"  e=h\n"
	"  h=j\n"
	"  j=y\n"
	"  x=F[+A(4)]Fy\n"
	"  y=F[-B(4)]Fx\n"
	"  F=@1.18F@i1.18\n"
	"  }\n"
	"\n"
	"Leaf2 { ; Adrian Mariano, from the Algorithmic Beauty of Plants\n"
	"	; Compound leaf with alternating branches, Figure 5.12a p.130\n"
	"  angle 8\n"
	"  axiom a\n"
	"  a=f[+x]fb\n"
	"  b=f[-y]fa\n"
	"  x=a\n"
	"  y=b\n"
	"  f=@1.36f@i1.36\n"
	"  }\n"
	"\n"
	"Bush { ; Adrian Mariano\n"
	"  Angle 16\n"
	"  Axiom ++++F\n"
	"  F=FF-[-F+F+F]+[+F-F-F]\n"
	"  }\n"
	"\n"
	"MyTree { ; Adrian Mariano\n"
	"  Angle 16\n"
	"  Axiom ++++F\n"
	"  F=FF-[XY]+[XY]\n"
	"  X=+FY\n"
	"  Y=-FX\n"
	"  }\n"
	"\n"
	"ColorTriangGasket { ; Adrian Mariano\n"
	"  Angle 6\n"
	"  Axiom --X\n"
	"  X=++FXF++FXF++FXF>1\n"
	"  F=FF\n"
	"  }\n"
	"\n"
	"SquareGasket { ; Adrian Mariano\n"
	"  Angle 4\n"
	"  Axiom X\n"
	"  X=+FXF+FXF+FXF+FXF\n"
	"  F=FF\n"
	"  }\n"
	"\n"
	"DragonCurve { ; Adrian Mariano\n"
	"  Angle 4\n"
	"  Axiom X\n"
	"  X=X-YF-\n"
	"  Y=+FX+Y\n"
	"  }\n"
	"\n"
	"Square { ; Adrian Mariano\n"
	"  Angle 4\n"
	"  Axiom F+F+F+F\n"
	"  F=FF+F+F+F+FF\n"
	"  }\n"
	"\n"
	"KochCurve { ; Adrian Mariano\n"
	"  Angle 6\n"
	"  Axiom F\n"
	"  F=F+F--F+F\n"
	"  }\n"
	"\n"
	"\n"
	"Penrose1 { ; by Herb Savage\n"
	"; based on Martin Gardner's \"Penrose Tiles to Trapdoor Ciphers\",\n"
	"; Roger Penrose's rhombuses\n"
	"  Angle 10\n"
	"  Axiom +WF--XF---YF--ZF\n"
	"  W=YF++ZF----XF[-YF----WF]++\n"
	"  X=+YF--ZF[---WF--XF]+\n"
	"  Y=-WF++XF[+++YF++ZF]-\n"
	"  Z=--YF++++WF[+ZF++++XF]--XF\n"
	"  F=\n"
	"}\n"
	"\n"
	"ColorPenrose1 { ; by Herb Savage\n"
	"; based on Martin Gardner's \"Penrose Tiles to Trapdoor Ciphers\",\n"
	"; Roger Penrose's rhombuses\n"
	"; Uses color to show the edge matching rules to force nonperiodicy\n"
	"  Angle 10\n"
	"  Axiom +WC02F--XC04F---YC04F--ZC02F\n"
	"  W=YC04F++ZC02F----XC04F[-YC04F----WC02F]++\n"
	"  X=+YC04F--ZC02F[---WC02F--XC04F]+\n"
	"  Y=-WC02F++XC04F[+++YC04F++ZC02F]-\n"
	"  Z=--YC04F++++WC02F[+ZC02F++++XC04F]--XC04F\n"
	"  F=\n"
	"  }\n"
	"\n"
	"Penrose2 { ; by Herb Savage\n"
	"; based on Martin Gardner's \"Penrose Tiles to Trapdoor Ciphers\",\n"
	"; Roger Penrose's rhombuses\n"
	"  Angle 10\n"
	"  Axiom ++ZF----XF-YF----WF\n"
	"  W=YF++ZF----XF[-YF----WF]++\n"
	"  X=+YF--ZF[---WF--XF]+\n"
	"  Y=-WF++XF[+++YF++ZF]-\n"
	"  Z=--YF++++WF[+ZF++++XF]--XF\n"
	"  F=\n"
	"  }\n"
	"\n"
	"Penrose3 { ; by Herb Savage\n"
	"; based on Martin Gardner's \"Penrose Tiles to Trapdoor Ciphers\",\n"
	"; Roger Penrose's rhombuses\n"
	"  Angle 10\n"
	"  Axiom [X]++[X]++[X]++[X]++[X]\n"
	"  W=YF++ZF----XF[-YF----WF]++\n"
	"  X=+YF--ZF[---WF--XF]+\n"
	"  Y=-WF++XF[+++YF++ZF]-\n"
	"  Z=--YF++++WF[+ZF++++XF]--XF\n"
	"  F=\n"
	"  }\n"
	"\n"
	"Penrose4 { ; by Herb Savage\n"
	"; based on Martin Gardner's \"Penrose Tiles to Trapdoor Ciphers\",\n"
	"; Roger Penrose's rhombuses\n"
	"  Angle 10\n"
	"  Axiom [Y]++[Y]++[Y]++[Y]++[Y]\n"
	"  W=YF++ZF----XF[-YF----WF]++\n"
	"  X=+YF--ZF[---WF--XF]+\n"
	"  Y=-WF++XF[+++YF++ZF]-\n"
	"  Z=--YF++++WF[+ZF++++XF]--XF\n"
	"  F=\n"
	"  }\n"
	"\n"
	"DoublePenrose { ; by Herb Savage\n"
	"; This is Penrose3 and Penrose4 superimposed\n"
	"  Angle 10\n"
	"  Axiom [X][Y]++[X][Y]++[X][Y]++[X][Y]++[X][Y]\n"
	"  W=YF++ZF----XF[-YF----WF]++\n"
	"  X=+YF--ZF[---WF--XF]+\n"
	"  Y=-WF++XF[+++YF++ZF]-\n"
	"  Z=--YF++++WF[+ZF++++XF]--XF\n"
	"  F=\n"
	"  }\n"
	"\n"
	"Sphinx { ; by Herb Savage\n"
	"; based on Martin Gardner's \"Penrose Tiles to Trapdoor Ciphers\"\n"
	"; This is an example of a \"reptile\"\n"
	"  Angle 6\n"
	"  Axiom X\n"
	"  X=+FF-YFF+FF--FFF|X|F--YFFFYFFF|\n"
	"  Y=-FF+XFF-FF++FFF|Y|F++XFFFXFFF|\n"
	"  F=GG\n"
	"  G=GG\n"
	"  }\n"
	"\n"
	"PentaPlexity {\n"
	"; Manual construction by Roger Penrose as a prelude to his development of\n"
	"; the famous Penrose tiles (the kites and darts) that tile the plane\n"
	"; only non-periodically.\n"
	"; Translated first to a \"dragon curve\" and finally to an L-system\n"
	"; by Joe Saverino.\n"
	"  Angle 10\n"
	"  Axiom F++F++F++F++F\n"
	"  F=F++F++F|F-F++F\n"
	"  }\n"
	"\n"
	"; old PentaPlexity:\n"
	"; Angle 10\n"
	"; Axiom F++F++F++F++Fabxjeabxykabxyelbxyeahxyeabiye\n"
	"; F=\n"
	"; a=Fabxjea\n"
	"; b=++F--bxykab\n"
	"; x=++++F----xyelbx\n"
	"; y=----F++++yeahxy\n"
	"; e=--F++eabiye\n"
	"; h=+++++F-----hijxlh\n"
	"; i=---F+++ijkyhi\n"
	"; j=-F+jkleij\n"
	"; k=+F-klhajk\n"
	"; l=+++F---lhibkl\n"
	"\n"
	"CircularTile { ; Adrian Mariano\n"
	"  axiom X+X+X+X+X+X+X+X+X+X+X+X+X+X+X+X+X+X+X+X+X+X+X+X\n"
	"  x=[F+F+F+F[---X-Y]+++++F++++++++F-F-F-F]\n"
	"  y=[F+F+F+F[---Y]+++++F++++++++F-F-F-F]\n"
	"  angle 24\n"
	"  }\n"
	"\n"
	"Lars1{ ; By Jonathan Osuch [73277,1432]\n"
	"       ; Based on a suggestion by Lars Vangsgaard\n"
	"  Angle 8  ; angle increment/decrement is 45\n"
	"  axiom [F]++[F]++[F]++F\n"
	"  F=F[+F][-F]\n"
	"  }\n"
	"\n"
	"Lars2{ ; By Jonathan Osuch [73277,1432]\n"
	"       ; Based on a suggestion by Lars Vangsgaard\n"
	"  Angle 8  ; angle increment/decrement is 45\n"
	"  axiom +[F]++[F]++[F]++F\n"
	"  F=F[+F][-F]\n"
	"  }\n"
	"\n"
	"Lars1Color{ ; By Jonathan Osuch [73277,1432]\n"
	"       ; Based on a suggestion by Lars Vangsgaard\n"
	"  Angle 8  ; angle increment/decrement is 45\n"
	"  axiom C1[F]++[F]++[F]++F\n"
	"  F=F<1[+F][-F]>1\n"
	"  }\n"
	"\n"
	"Lars2Color{ ; By Jonathan Osuch [73277,1432]\n"
	"       ; Based on a suggestion by Lars Vangsgaard\n"
	"  Angle 8  ; angle increment/decrement is 45\n"
	"  axiom C1+[F]++[F]++[F]++F\n"
	"  F=F<1[+F][-F]>1\n"
	"  }\n"
	"\n"
	"Man { ; From Ivan Daunis daunis@teleline.es\n"
	"   ; looks like man with an odd number of iterations\n"
	"  Angle 8\n"
	"  Axiom F++F++F++F\n"
	"  F=-F-FF+++F+FF-F\n"
	"}\n"
	"\n"
	"Lace { ; From Ivan Daunis daunis@teleline.es\n"
	"  Angle 8\n"
	"  Axiom F++F++F++F\n"
	"  F=F+++F---F+F---F++F--F++F\n"
	"}\n";
;
