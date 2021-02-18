/*	ProgramFile.cpp

MIT License

Copyright (c) 2018-2019 Fabian Herb

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "ProgramFile.h"
#include <molecular/util/Parser.h>
#include <stdlib.h>
#include <iostream>
#include <algorithm>
#include <cassert>
namespace molecular
{
namespace programgenerator
{
using namespace util;
using namespace util::Parser;

bool ProgramFile::Parse(char* begin, char* end)
{
	// Brainfuck...
	typedef Concatenation<Alpha, Repetition<Alternation<Alpha, Digit, Char<'_'>> > > Identifier;
	typedef Action<Concatenation<Char<'f'>, Char<'r'>, Char<'a'>, Char<'g'>, Char<'m'>, Char<'e'>, Char<'n'>, Char<'t'> >, kFragmentStage> Fragment;
	typedef Action<Concatenation<Char<'v'>, Char<'e'>, Char<'r'>, Char<'t'>, Char<'e'>, Char<'x'> >, kVertexStage> Vertex;
	typedef Action<Concatenation<Char<'g'>, Char<'e'>, Char<'o'>, Char<'m'>, Char<'e'>, Char<'t'>, Char<'r'>, Char<'y'> >, kGeometryStage> Geometry;
	typedef Action<Concatenation<Char<'l'>, Char<'o'>, Char<'w'>, Char<'_'>, Char<'q'> >, kLowQuality> LowQ;
	typedef Concatenation<Char<'p'>, Char<'r'>, Char<'i'>, Char<'o'>, Char<'='>, Action<Integer, kPriority> > Prio;
	typedef Concatenation<Char<'i'>, Char<'n'>, Char<'_'>, Char<'p'>, Char<'r'>, Char<'i'>, Char<'m'>, Char<'='>,  Action<Identifier, kInPrimitive> > InPrimitive;
	typedef Concatenation<Char<'m'>, Char<'a'>, Char<'x'>, Char<'_'>, Char<'v'>, Char<'e'>, Char<'r'>, Char<'t'>, Char<'='>, Action<Integer, kMaxVertices> > MaxVertives;
	typedef Concatenation<Char<'o'>, Char<'u'>, Char<'t'>, Char<'_'>, Char<'p'>, Char<'r'>, Char<'i'>, Char<'m'>, Char<'='>, Action<Identifier, kOutPrimitive> > OutPrimitive;
	typedef Concatenation<Char<'a'>, Char<'t'>, Char<'t'>, Char<'r'> > Attr;
	typedef Concatenation<Char<'o'>, Char<'u'>, Char<'t'> > Out;
	typedef Concatenation<Char<'i'>, Char<'n'>> In;
	typedef Concatenation<Char<'i'>, Char<'n'>, Char<'o'>, Char<'u'>, Char<'t'> > Inout;
	typedef Concatenation<Char<'t'>, Char<'r'>, Char<'u'>, Char<'e'> > True;
	typedef Concatenation<Char<'f'>, Char<'a'>, Char<'l'>, Char<'s'>, Char<'e'> > False;
	typedef Concatenation<Char<'a'>, Char<'u'>, Char<'t'>, Char<'o'>, Char<'_'>, Char<'e'>, Char<'m'>, Char<'i'>, Char<'t'>, Char<'='>, Action< Alternation<True, False>, kAutoEmission > > AutoEmission;	
	typedef Concatenation<Char<'p'>, Char<'r'>, Char<'i'>, Char<'m'>, Char<'_'>, Char<'d'>, Char<'s'>, Char<'c'>, Char<'r'>, Char<'='> > Primitive;
	typedef Concatenation< Primitive, Concatenation< Action<Integer, kGeometryPrimitiveDescription >, Repetition< Concatenation< Char<','>, Action< Integer, kGeometryPrimitiveDescription> > > > > PrimitiveDescription;
	typedef Concatenation<Alternation<Fragment, Vertex, Geometry, LowQ, Prio, InPrimitive, OutPrimitive, MaxVertives, PrimitiveDescription, AutoEmission>, Whitespace> Attribute;
	typedef Action<Concatenation<Char<'p'>, Char<'u'>, Char<'r'>, Char<'e'>>, kPure> Pure;

	typedef Concatenation<
			Option<Alternation<Concatenation<In, Whitespace>, Concatenation<Inout, Whitespace>,Action<Concatenation<Attr, Whitespace>, kAttribute >, Action<Concatenation<Out, Whitespace>, kOutput > > >,
			Action<Identifier, kType>,
			Option<Action<Concatenation<Char<'['>, Char<']'> >, kArray> > > Type;

	typedef Concatenation<
			Option<Whitespace>,
			Type,
			Whitespace,
			Action<Identifier, kParameterName>,
			Option<Whitespace>,
			Option<Concatenation<Char<'['>, Integer, Char<']'>>>> Parameter;

	typedef Concatenation<Char<'('>,
			Option<Concatenation<Parameter, Repetition<Concatenation<Char<','>, Parameter> > > >,
			Char<')'>> ParameterList;

	typedef Concatenation<
			Type,
			Whitespace,
			Action<Identifier, kFunctionName>,
			Option<Whitespace>,
			ParameterList> Declaration;

	typedef Concatenation<
			Char<'{'>,
			Action<Body, kBody>,
			Option<Whitespace>,
			Char<'}'>
			> FunctionBody;
	
	typedef Action< Concatenation<
					Option<Whitespace>,
					Repetition<Attribute>,
					Declaration,
					Option<Whitespace>,
					FunctionBody,
					Option<Whitespace>,
					Repetition<Concatenation<Option<ParameterList>, Option<Whitespace>, FunctionBody, Option<Whitespace>>>
			>, kFunction> Function;

	typedef Concatenation<
				Option<Whitespace>,
				Repetition<Attribute>, 
				Pure, 
				Whitespace, 
				Repetition<Attribute>, 
				Action<
					Concatenation<
						Declaration, 
						Option<Whitespace>, 
						FunctionBody>, 
					kPureFunction>>
		PureFunction;


	typedef Concatenation<Repetition<Alternation<Function, PureFunction>>, Option<Whitespace>, Option<Char<0> > > File;
	

	bool success = File::Parse(begin, end, this);

	if(success && begin == end)
		return true;
	else
		return false;
}

void ProgramFile::ParserAction(int action, char* begin, char* end)
{
	char tmp;
	switch(action)
	{
	case kPriority:
		tmp = *end;
		*end = 0;
		mCurrentFunction.priority = strtol(begin, nullptr, 10);
		*end = tmp;
		break;

	case kLowQuality:
		mCurrentFunction.highQuality = false;
		break;

	case kVertexStage:
		mCurrentFunction.stage = ProgramGenerator::Function::Stage::kVertexStage;
		break;

	case kGeometryStage:
		mCurrentFunction.stage = ProgramGenerator::Function::Stage::kGeometryStage;
		break;

	case kFragmentStage:
		mCurrentFunction.stage = ProgramGenerator::Function::Stage::kFragmentStage;
		break;

	case kType:
		mCurrentVariable.type = std::string(begin, end);
		break;

	case kAttribute:
		mCurrentVariable.usage = ProgramGenerator::VariableInfo::Usage::kAttribute;
		break;

	case kOutput:
		mCurrentVariable.usage = ProgramGenerator::VariableInfo::Usage::kOutput;
		break;

	case kArray:
		mCurrentVariable.array = true;
		break;

	case kFunctionName:
		mCurrentFunction.output = HashUtils::MakeHash(begin, end);
		mCurrentVariable.name = std::string(begin, end);
		mVariables.push_back(mCurrentVariable);
		mCurrentVariable = ProgramGenerator::VariableInfo(); // Restore defaults
		mCurrentFunction.name = std::string(begin, end);
		break;

	case kParameterName:
	{
		//TODO: could be a collision. Handle this case
		auto hash = HashUtils::MakeHash(begin, end);
		auto item = std::find(mCurrentFunction.inputs.begin(), mCurrentFunction.inputs.end(), hash);
		/*GS could have multiple parameter lists and body 
			definitions (for each instance), 
			thus register only the first met 
			variable and ignore repetitions*/
		if(item != mCurrentFunction.inputs.end() || mCurrentFunction.pureFunction)
			break;
		mCurrentFunction.inputs.push_back(hash);
		mCurrentFunction.input_names.push_back(std::string(begin,end));
		mCurrentVariable.name = std::string(begin, end);
		mVariables.push_back(mCurrentVariable);
		mCurrentVariable = ProgramGenerator::VariableInfo(); // Restore defaults
		break;
	}

	case kBody:
		if(mCurrentFunction.source.size() > 0 && mCurrentFunction.stage != ProgramGenerator::Function::Stage::kGeometryStage)
			throw std::invalid_argument("Only geometry stage variables can have multiple instances of the code body");
		if(mCurrentFunction.pureFunction)
			break;
		mCurrentFunction.source.push_back(std::string(begin, end));
		break;

	case kFunction:
		mFunctions.push_back(mCurrentFunction);
		mCurrentFunction = ProgramGenerator::Function(); // Restore defaults
		break;

	case kInPrimitive:
	{
		std::string primitive(begin, end);
		if(!mCurrentFunction.gsInfo)
			mCurrentFunction.gsInfo = std::make_shared<ProgramGenerator::GSInfo>();
		mCurrentFunction.gsInfo->mInPrimitive = primitive;
		break;
	}

	case kOutPrimitive:
	{
		std::string primitive(begin, end);
		if(!mCurrentFunction.gsInfo)
			mCurrentFunction.gsInfo = std::make_shared<ProgramGenerator::GSInfo>();
		mCurrentFunction.gsInfo->mOutPrimitive = primitive;
		break;
	}

	case kMaxVertices:
		if(!mCurrentFunction.gsInfo)
			mCurrentFunction.gsInfo = std::make_shared<ProgramGenerator::GSInfo>();
		mCurrentFunction.gsInfo->mMaxVertices = std::strtoul(begin, nullptr, 10);
		break;

	case kGeometryPrimitiveDescription:
		if(!mCurrentFunction.gsInfo)
			mCurrentFunction.gsInfo = std::make_shared<ProgramGenerator::GSInfo>();
		mCurrentFunction.gsInfo->primitiveDescription.push_back(std::strtoul(begin, nullptr, 10));
		break;

	case kAutoEmission:
		if(!mCurrentFunction.gsInfo)
			mCurrentFunction.gsInfo = std::make_shared<ProgramGenerator::GSInfo>();
		mCurrentFunction.gsInfo->mEnableAutoEmission = (std::string("true") == std::string(begin, end));
		break;

	case kPureFunction:
		// By design pure function should not depend on any inputs
		if(mCurrentFunction.inputs.size() != 0 ||
			mCurrentFunction.source.size() != 0)
			throw std::runtime_error("Internal parser error");
		mCurrentFunction.source.push_back(std::string(begin, end));
		mFunctions.push_back(mCurrentFunction);
		mCurrentFunction = ProgramGenerator::Function(); // Restore defaults
		break;

	case kPure:
		mCurrentFunction.pureFunction = true;
		break;
	}
}

}
} // namespace
