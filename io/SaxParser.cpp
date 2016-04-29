/*
 * SaxParser.cpp
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

#include "SaxParser.h"
#include "../constants.h"

#include <fstream>
#include <sstream>
#include <iostream>

#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/filter/bzip2.hpp>

namespace CRP {

bool SaxParser::parse(const std::string &inputFileName, SaxHandler &handler) {
	std::ifstream file;
	file.open(inputFileName);
	if (!file.is_open()) return false;

	std::string line;
	bool ok = false;
	while (!file.eof()) {
		std::getline(file, line);
		//std::cout << line << std::endl;
		ok = parseLine(line, handler);
		if (!ok) {
			std::cout << "error in line " << line << std::endl;
			break;
		}
	}

	file.close();

	std::cout << "ElementStack: " << elementStack.size() << std::endl;
	for (unsigned i = 0; i < elementStack.size(); ++i) {
		Element e = elementStack.top(); elementStack.pop();
		std::cout << e.qName << std::endl;
	}

	ok = ok && elementStack.size() == 0;

	return ok;
}

bool SaxParser::parseBZ2(const std::string &inputFileName, SaxHandler &handler) {
	std::ifstream file(inputFileName, std::ios_base::in | std::ios_base::binary);
	if (!file.is_open()) return false;

	boost::iostreams::filtering_streambuf<boost::iostreams::input> inbuf;
	inbuf.push(boost::iostreams::bzip2_decompressor());
	inbuf.push(file);

	std::string line;
	std::istream instream(&inbuf);
	bool ok = false;
	while (std::getline(instream, line)) {
		ok = parseLine(line, handler);
		if (!ok) {
			std::cout << "error in line " << line << std::endl;
			break;
		}
	}

	file.close();

	std::cout << "ElementStack: " << elementStack.size() << std::endl;
	for (unsigned i = 0; i < elementStack.size(); ++i) {
		Element e = elementStack.top(); elementStack.pop();
		std::cout << e.qName << std::endl;
	}

	ok = ok && elementStack.size() == 0;

	return ok;
}

bool SaxParser::parseLine(std::string &line, SaxHandler &handler) {
	// remove front white space
	index frontWhiteSpace = 0;
	for (; frontWhiteSpace < line.size(); ++frontWhiteSpace) {
		if (line[frontWhiteSpace] != ' ' && line[frontWhiteSpace] != '\t') break;
	}

	line = line.substr(frontWhiteSpace);

	if (line.size() == 0) return true; // line is empty

	if (stringStartsWith(line, "</")) { // end element
		Element e = elementStack.top(); elementStack.pop();
		handler.endElement(e.uri, e.localName, e.qName);
	} else { // start element
		std::string qName;
		std::vector<Attribute> attributes;
		bool isEmpty = false;

		unsigned idx = 1;
		for (; idx < line.size(); ++idx) {
			if (line[idx] == ' ' || line[idx] == '\t' || line[idx] == '>') {
				while (line[idx] == ' ' || line[idx] == '\t') {
					idx++;
				}
				break;
			} else if (line[idx] == '?') {
				// do nothing
			} else {
				qName += line[idx];
			}
		}

		//std::cout << line << std::endl;
		//std::cout << qName << std::endl;
		bool doubleQuotes = true;
		
		Attribute curAttribute;
		while (idx < line.size()) {
			while (line[idx] == ' ') {
				idx++;
			}

			if ((line[idx] == '/' && line[idx+1] == '>') || line[idx] == '?') {
				handler.startElement("", "", qName, attributes);
				handler.endElement("", "", qName);
				isEmpty = true;
				break;
			} else if (line[idx] == '>') {
				break;
			}

			// key
			std::stringstream keyStream;
			while (line[idx] != '=') {
				keyStream << line[idx++];
			}
			idx++;
			curAttribute.qName = keyStream.str();

			// value
			if (line[idx] != '"' && line[idx] != '\'') {
				return false;
			}
			doubleQuotes = line[idx] == '"';
			idx++;

			std::stringstream valStream;
			while ((doubleQuotes && line[idx] != '"') || (!doubleQuotes && line[idx] != '\'')) {
				valStream << line[idx++];
			}
			idx++;
			curAttribute.value = valStream.str();

			attributes.push_back(curAttribute);
		}

		if (!isEmpty) {
			handler.startElement("", "", qName, attributes);
			elementStack.push({"", "", qName});
		}
	}

	return true;
}

bool SaxParser::stringStartsWith(const std::string &str, const std::string &prefix) {
	if (str.size() < prefix.size()) return false;
	for (unsigned i = 0; i < prefix.size(); ++i) {
		if (str[i] != prefix[i]) return false;
	}

	return true;
}

} /* namespace CRP */
