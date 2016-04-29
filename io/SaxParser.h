/*
 * SaxParser.h
 *
 *  Created on: Dec 24, 2015
 *      Author: Michael Wegner
 *
 * Copyright (c) 2016 Michael Wegner and Matthias Wolf
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef IO_SAXPARSER_H_
#define IO_SAXPARSER_H_

#include <string>
#include <stack>

#include "SaxHandler.h"

namespace CRP {

class SaxParser {
public:
	SaxParser() = default;

	bool parse(const std::string &inputFileName, SaxHandler &handler);

	bool parseBZ2(const std::string &inputFileName, SaxHandler &handler);

	static bool stringStartsWith(const std::string &str, const std::string &prefix);

private:
	struct Element {
		std::string uri;
		std::string localName;
		std::string qName;
	};

	std::stack<Element> elementStack;

	bool parseLine(std::string &line, SaxHandler &handler);


};

} /* namespace CRP */

#endif /* IO_SAXPARSER_H_ */
