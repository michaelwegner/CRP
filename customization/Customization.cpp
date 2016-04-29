/*
 * Customization.cpp
 *
 *  Created on: Jan 13, 2016
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

#include <unordered_map>
#include <iostream>
#include <memory>
#include <string>

#include "../datastructures/Graph.h"
#include "../datastructures/OverlayGraph.h"
#include "../datastructures/OverlayWeights.h"
#include "../io/GraphIO.h"
#include "../metrics/DistanceFunction.h"
#include "../metrics/HopFunction.h"
#include "../metrics/Metric.h"
#include "../metrics/TimeFunction.h"

using namespace std;



int main(int argc, char* argv[]) {
	if (argc != 5) {
		std::cout << "Usage: " << argv[0] << " path_to_graph path_to_overlay_graph metric_output_path metric_type" << std::endl;
		return 1;
	}

	string graphFile(argv[1]);
	string overlayGraphFile(argv[2]);
	string metricPath(argv[3]);
	string metricType(argv[4]);

	unordered_map<string, unique_ptr<CRP::CostFunction>> costFunctions;
	costFunctions["hop"] = unique_ptr<CRP::CostFunction>(new CRP::HopFunction());
	costFunctions["dist"] = unique_ptr<CRP::CostFunction>(new CRP::DistanceFunction());
	costFunctions["time"] = unique_ptr<CRP::CostFunction>(new CRP::TimeFunction());

	CRP::Graph graph;
	CRP::OverlayGraph overlayGraph;

	cout << "reading graph" << endl;
	CRP::GraphIO::readGraph(graph, graphFile);
	cout << "reading overlay graph" << endl;
	CRP::GraphIO::readOverlayGraph(overlayGraph, overlayGraphFile);

	if (metricType == "all") {
		for (auto &pair : costFunctions) {
			CRP::Metric m(graph, overlayGraph, std::move(pair.second));
			std::ofstream stream(metricPath + pair.first);
			CRP::Metric::write(stream, m);
			stream.close();
		}
	} else {
		auto it = costFunctions.find(metricType);
		if (it == costFunctions.end()) {
			cout << "unknown metric" << std::endl;
			return 0;
		}

		CRP::Metric m(graph, overlayGraph, std::move(it->second));
		std::ofstream stream(metricPath + it->first);
		CRP::Metric::write(stream, m);
		stream.close();
	}

	return 0;
}


