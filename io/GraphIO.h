/*
 * GraphIO.h
 *
 *  Created on: Dec 28, 2015
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

#ifndef IO_GRAPHIO_H_
#define IO_GRAPHIO_H_

#include "../datastructures/Graph.h"
#include "../datastructures/OverlayGraph.h"
#include "../datastructures/OverlayWeights.h"

#include <string>


namespace CRP {

class GraphIO {

public:
	static bool writeGraph(const Graph &graph, const std::string &outputFilePath);

	static bool writeMetisGraph(const Graph &graph, const std::string &outputFilePath);

	static bool readGraph(Graph &graph, const std::string &inputFilePath);

	static bool readDimacsGraph(Graph &graph, const std::string &dimacsFolder, bool dist);

	static bool writeOverlayGraph(const OverlayGraph &graph, const std::string &outputFilePath);

	static bool readOverlayGraph(OverlayGraph &graph, const std::string &inputFilePath);

	static bool writeWeights(const OverlayWeights &weights, const std::string &outputFilePath);

	static bool readWeights(OverlayWeights &weights, const std::string &inputFilePath);

	static std::vector<std::string> splitString(const std::string &str, char splitToken);

	static index stoui(const std::string &str);
};

} /* namespace CRP */

#endif /* IO_GRAPHIO_H_ */
