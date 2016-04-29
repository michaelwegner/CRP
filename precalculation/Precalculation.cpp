/*
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

#include <iostream>

#include "../datastructures/MultiLevelPartition.h"
#include "../io/GraphIO.h"
#include "../datastructures/Graph.h"
#include "../datastructures/OverlayGraph.h"

using namespace std;

namespace CRP {

void buildCellNumbers(Graph& graph, const MultiLevelPartition& mlp) {
	std::vector<pv> cellNumbers;
	cellNumbers.reserve(mlp.getNumberOfCellsInLevel(0));
	std::unordered_map<pv, index> pvMap;
	pvMap.reserve(mlp.getNumberOfCellsInLevel(0));

	graph.forVertices([&](index u, Vertex& vertex) {
		pv cellNumber = mlp.getCellNumber(u);
		auto it = pvMap.find(cellNumber);
		if (it == pvMap.end()) {
			cellNumbers.push_back(cellNumber);
			pvMap[cellNumber] = cellNumbers.size() - 1;
			vertex.pvPtr = cellNumbers.size() - 1;
		} else {
			vertex.pvPtr = it->second;
		}
	});

	assert(cellNumbers.size() == mlp.getNumberOfCellsInLevel(0));
	assert(pvMap.size() == mlp.getNumberOfCellsInLevel(0));

	graph.setCellNumbers(cellNumbers);
}
}
int main(int argc, char* argv[]) {
	if (argc != 4) {
		cout << "Usage:" << argv[0] << " pathToGraph pathToMLP pathToOutputDirectory" << endl;
		return 1;
	}

	string graphFileName(argv[1]);
	string mlpFileName(argv[2]);
	string outputDir(argv[3]);

	string graphName = graphFileName.substr(graphFileName.find_last_of("/\\"));
	string newGraphFile = outputDir + graphName;

	string overlayGraphFile = outputDir + graphName.substr(0, graphName.find_first_of(".")) + ".overlay";


	CRP::MultiLevelPartition mlp;
	mlp.read(mlpFileName);

	cout << "Reading graph" << endl;
	CRP::Graph graph;
	CRP::GraphIO::readGraph(graph, graphFileName);

	CRP::buildCellNumbers(graph, mlp);
	graph.sortVerticesByCellNumber();


	cout << "Building Overlay Graph" << endl;
	CRP::OverlayGraph overlayGraph(graph, mlp);

	cout << "Writing graph " << endl;
	CRP::GraphIO::writeGraph(graph, newGraphFile);

	cout << "Writing overlay graph" << endl;
	CRP::GraphIO::writeOverlayGraph(overlayGraph, overlayGraphFile);

	cout << "Done" << endl;

	return 0;
}
