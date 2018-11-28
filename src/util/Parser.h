/*	Parser.h

MIT License

Copyright (c) 2018 Fabian Herb

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

#ifndef PARSER_H
#define PARSER_H

#include <cctype>

namespace molecular
{

/// A recursive-descent parser with backtracking
/** Can be used as a scannerless parser.
	@bug The actor is also invoked for decision paths which are later discarded. */
namespace Parser
{

/// Parsing always succeeds
/** Mostly used as standard template argument for Concatenation */
class True
{
public:
	template<class Iterator>
	static bool Parse (Iterator& begin, Iterator end, void*)
	{
		return true;
	}
};

/// Parsing always fails
/** Mostly used as standard template argument for Alternation */
class False
{
public:
	template<class Iterator>
	static bool Parse (Iterator& begin, Iterator end, void*)
	{
		return false;
	}
};

template<
		class P0,
		class P1,
		class P2 = False,
		class P3 = False,
		class P4 = False,
		class P5 = False,
		class P6 = False,
		class P7 = False,
		class P8 = False,
		class P9 = False>
class Alternation
{
public:
	template<class Iterator, class Actor>
	static bool Parse (Iterator& begin, Iterator end, Actor* actor)
	{
		if(P0::Parse(begin, end, actor) || P1::Parse(begin, end, actor)
		|| P2::Parse(begin, end, actor) || P3::Parse(begin, end, actor)
		|| P4::Parse(begin, end, actor) || P5::Parse(begin, end, actor)
		|| P6::Parse(begin, end, actor) || P7::Parse(begin, end, actor)
		|| P8::Parse(begin, end, actor) || P9::Parse(begin, end, actor))
			return true;

		return false;
	}
};

template<
		class P0,
		class P1,
		class P2 = True,
		class P3 = True,
		class P4 = True,
		class P5 = True,
		class P6 = True,
		class P7 = True,
		class P8 = True,
		class P9 = True,
		class Pa = True>
class Concatenation
{
public:
	template<class Iterator, class Actor>
	static bool Parse (Iterator& begin, Iterator end, Actor* actor)
	{
		Iterator originalBegin = begin;
		if(P0::Parse(begin, end, actor) && P1::Parse(begin, end, actor)
		&& P2::Parse(begin, end, actor) && P3::Parse(begin, end, actor)
		&& P4::Parse(begin, end, actor) && P5::Parse(begin, end, actor)
		&& P6::Parse(begin, end, actor) && P7::Parse(begin, end, actor)
		&& P8::Parse(begin, end, actor) && P9::Parse(begin, end, actor)
		&& Pa::Parse(begin, end, actor))
			return true;
		begin = originalBegin;
		return false;
	}
};

template<class P0>
class Repetition
{
public:
	template<class Iterator, class Actor>
	static bool Parse(Iterator& begin, Iterator end, Actor* actor)
	{
		while(begin != end && P0::Parse(begin, end, actor))
		{}
		return true;
	}
};

template<class P0>
class Option
{
public:
	template<class Iterator, class Actor>
	static bool Parse(Iterator& begin, Iterator end, Actor* actor)
	{
		P0::Parse(begin, end, actor);
		return true;
	}
};

template<char c>
class Char
{
public:
	template<class Iterator>
	static bool Parse(Iterator& begin, Iterator end, void*)
	{
		if(begin == end)
			return false;

		if(*begin == c)
		{
			begin++;
			return true;
		}
		return false;
	}
};

/// Executes Functor if P0 was successful
template<class P0, int action>
class Action
{
public:
	template<class Iterator, class Actor>
	static bool Parse(Iterator& begin, Iterator end, Actor* actor)
	{
		Iterator oldBegin = begin;
		if(P0::Parse(begin, end, actor))
		{
			if(actor)
				actor->ParserAction(action, oldBegin, begin);
			return true;
		}
		return false;
	}
};

/********************************** Derived **********************************/

/// Matches one or more whitespace characters
/** Uses isspace(). */
class Whitespace
{
public:
	template<class Iterator>
	static bool Parse(Iterator& begin, Iterator end, void*)
	{
		if(begin == end || !isspace(*begin))
			return false;

		while(begin != end && isspace(*begin))
		{
			begin++;
		}
		return true;
	}
};

class Alpha
{
public:
	template<class Iterator>
	static bool Parse(Iterator& begin, Iterator end, void*)
	{
		if(begin == end)
			return false;

		if(isalpha(*begin))
		{
			begin++;
			return true;
		}
		return false;
	}
};

template<char from, char to>
class CharRange
{
public:
	template<class Iterator>
	static bool Parse(Iterator& begin, Iterator end, void*)
	{
		if(begin == end)
			return false;

		if(*begin >= from && *begin <= to)
		{
			begin++;
			return true;
		}
		return false;
	}
};

using Digit = CharRange<'0', '9'>;
using LowerCaseLetter = CharRange<'a', 'z'>;
using UpperCaseLetter = CharRange<'A', 'Z'>;
typedef Concatenation<Digit, Repetition<Digit> > UnsignedInteger;
typedef Concatenation<Option<Char<'-'> >, UnsignedInteger> Integer;
typedef Concatenation<Integer, Option<Concatenation<Char<'.'>, UnsignedInteger> >, Option<Concatenation<Char<'e'>, Integer> > > Real;

} // namespace parser

} // namespace molecular
#endif // PARSER_H
