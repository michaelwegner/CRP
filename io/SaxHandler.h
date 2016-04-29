/*
 * SaxHandler.h
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

#ifndef IO_SAXHANDLER_H_
#define IO_SAXHANDLER_H_

#include <string>
#include <vector>
#include <unordered_map>

namespace CRP {

struct Attribute {
	std::string qName;
	std::string value;
};

class SaxHandler {
public:
	SaxHandler() = default;

	virtual ~SaxHandler() = default;

	/** Receive notification of the beginning of the document. */
	virtual void startDocument() {}

	/**
	 * Receive notification of the start of an element.
	 * @param uri TODO
	 * @param localName TODO
	 * @param qName The qualified name (with prefix), or the empty string if qualified names are not available.
	 * @param attributes The attributes attached to this element.
	 */
	virtual void startElement(const std::string &uri, const std::string &localName, const std::string &qName, const std::vector<Attribute> &attributes) {}

	/**
	 * Receive notification of the end of an element.
	 * @param uri TODO
	 * @param localName TODO
	 * @param qName The qualified name (with prefix), or the empty string if qualified names are not available.
	 */
	virtual void endElement(const std::string &uri, const std::string &localName, const std::string &qName) {}
};

} /* namespace CRP */


#endif /* IO_SAXHANDLER_H_ */
