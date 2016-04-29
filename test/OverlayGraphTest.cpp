/*
 * overlay_graph_test.cpp
 *
 *  Created on: 02.01.2016
 *      Author: Matthias Wolf
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

#include <cassert>
#include <iostream>
#include <vector>

#include "../constants.h"
#include "../datastructures/Graph.h"
#include "../datastructures/LevelInfo.h"
#include "../datastructures/MultiLevelPartition.h"
#include "../datastructures/OverlayGraph.h"
#include "../datastructures/OverlayWeights.h"
#include "../io/GraphIO.h"
#include "../metrics/HopFunction.h"

namespace CRP {
class HopFunction;
} /* namespace CRP */

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

Graph buildGraph() {
	std::vector<Vertex> vertices;
	vertices.push_back({0, 0, 0, 0, {0, 0}});
	vertices.push_back({0, 0, 1, 1, {0, 0}});
	vertices.push_back({0, 0, 5, 2, {0, 0}});
	vertices.push_back({0, 0, 7, 4, {0, 0}});
	vertices.push_back({0, 0, 7, 7, {0, 0}});
	vertices.push_back({0, 0, 8, 9, {0, 0}});
	vertices.push_back({0, 0, 10, 10, {0, 0}});
	vertices.push_back({0, 0, 12, 12, {0, 0}});
	vertices.push_back({0, 0, 13, 13, {0, 0}}); // Dummy

	EdgeAttributes attr{1, 1};
	std::vector<ForwardEdge> forwardEdges;
	forwardEdges.push_back({4, 0, attr}); // 0->4
	forwardEdges.push_back({0, 0, attr}); // 1->0
	forwardEdges.push_back({2, 0, attr}); // 1->2
	forwardEdges.push_back({4, 1, attr}); // 1->4
	forwardEdges.push_back({6, 0, attr}); // 1->6
	forwardEdges.push_back({3, 0, attr}); // 2->3
	forwardEdges.push_back({5, 0, attr}); // 2->5
	forwardEdges.push_back({6, 1, attr}); // 4->6
	forwardEdges.push_back({3, 1, attr}); // 5->3
	forwardEdges.push_back({7, 0, attr}); // 5->7
	forwardEdges.push_back({1, 0, attr}); // 6->1
	forwardEdges.push_back({3, 2, attr}); // 6->3
	forwardEdges.push_back({2, 1, attr}); // 7->2

	std::vector<BackwardEdge> backwardEdges;
	backwardEdges.push_back({1, 0, attr}); // 1->0
	backwardEdges.push_back({6, 0, attr}); // 6->1
	backwardEdges.push_back({1, 1, attr}); // 1->2
	backwardEdges.push_back({7, 0, attr}); // 7->2
	backwardEdges.push_back({2, 0, attr}); // 2->3
	backwardEdges.push_back({5, 0, attr}); // 5->3
	backwardEdges.push_back({6, 1, attr}); // 6->3
	backwardEdges.push_back({0, 0, attr}); // 0->4
	backwardEdges.push_back({1, 2, attr}); // 1->4
	backwardEdges.push_back({2, 1, attr}); // 2->5
	backwardEdges.push_back({1, 3, attr}); // 1->6
	backwardEdges.push_back({4, 0, attr}); // 4->6
	backwardEdges.push_back({5, 1, attr}); // 5->7

	std::vector<Graph::TURN_TYPE> turnTables(4, Graph::NONE);

	return Graph(vertices, forwardEdges, backwardEdges, turnTables);
}

MultiLevelPartition buildMultiLevelPartition(const Graph& g) {
	MultiLevelPartition mlp;
	mlp.setNumberOfLevels(2);
	mlp.setNumberOfVertices(g.numberOfVertices());
	mlp.setNumberOfCellsInLevel(0, 4);
	mlp.setNumberOfCellsInLevel(1, 2);
	mlp.computeBitmap();

	std::vector<int> cells = {0, 0, 1, 3, 2, 1, 2, 1};
	std::vector<int> topLeveLCell = {0, 0, 1, 1};
	for (index v = 0; v < g.numberOfVertices(); ++v) {
		mlp.setCell(0, v, cells[v]);
		mlp.setCell(1, v, topLeveLCell[cells[v]]);
	}

	return mlp;
}

void testCell(const OverlayGraph& og, pv cellNumber, level lev, index entryPoints, index exitPoints) {
	//std::cout << "testing cell " << cellNumber << " on level " << (int) lev << std::endl;

	const CRP::Cell& c = og.getCell(cellNumber, lev);
	assert(c.numEntryPoints == entryPoints);
	assert(c.numExitPoints == exitPoints);

	const LevelInfo& levelInfo = og.getLevelInfo();
	const pv truncatedCellNumber = levelInfo.truncateToLevel(cellNumber, lev);
	for (index i = 0; i < c.numEntryPoints; ++i) {
		index v = og.getEntryPoint(c, i);
		const OverlayVertex& vertex = og.getVertex(v);
		assert(truncatedCellNumber == levelInfo.truncateToLevel(og.getVertex(v).cellNumber, lev));
		assert(og.getVertex(v).entryExitPoint.size() >= lev);
		assert(og.getVertex(v).entryExitPoint[lev-1] == i);
	}

	for (index i = 0; i < c.numExitPoints; ++i) {
		index v = og.getExitPoint(c, i);
		const OverlayVertex& vertex = og.getVertex(v);
		assert(truncatedCellNumber == levelInfo.truncateToLevel(vertex.cellNumber, lev));
		assert(vertex.entryExitPoint.size() >= lev);
		assert(vertex.entryExitPoint[lev-1] == i);
	}
}

} /* namespace CRP */



int main(int argc, char **argv) {
	CRP::Graph g = CRP::buildGraph();
	std::cout << "graph built" << std::endl;

	CRP::MultiLevelPartition mlp = buildMultiLevelPartition(g);
	std::cout << "mlp built" << std::endl;
	mlp.write("mlp");
	mlp.read("mlp");

	std::cout << "building cell numbers" << std::endl;
	CRP::buildCellNumbers(g, mlp);

	std::cout << "sorting vertices by cell number" << std::endl;
	g.sortVerticesByCellNumber();

	std::cout << "building overlay graph" << std::endl;
	CRP::OverlayGraph og(g, mlp);

	og.forVertices([&](const CRP::OverlayVertex& v) {
		std::cout << "(" << v.entryExitPoint.size() << ", " << v.cellNumber << ") ";
	});
	std::cout << std::endl;

	std::cout << "number of overlay vertices: " << og.numberOfVertices() << std::endl;
	std::cout << og.numberOfVerticesInLevel(1) << " " << og.numberOfVerticesInLevel(2) << std::endl;
	std::cout << "number of levels: " << og.getLevelInfo().getLevelCount() << std::endl;
	assert(mlp.getNumberOfLevels() == og.getLevelInfo().getLevelCount());

	CRP::testCell(og, 0, 1, 1, 4);
	CRP::testCell(og, 1, 1, 1, 2);
	CRP::testCell(og, 6, 1, 3, 2);
	CRP::testCell(og, 7, 1, 3, 0);

	CRP::testCell(og, 0, 2, 1, 5);
	CRP::testCell(og, 1, 2, 1, 5);
	CRP::testCell(og, 6, 2, 5, 1);
	CRP::testCell(og, 7, 2, 5, 1);
	std::cout << "tested cells" << std::endl;

	assert(og.getWeightVectorSize() == 22);

	std::cout << "tested OverlayGraph" << std::endl;

	CRP::OverlayWeights weights(g, og, CRP::HopFunction{});

	assert(weights.getWeights().size() == og.getWeightVectorSize());

	std::cout << "calculated weights" << std::endl;

	for (CRP::level lev = 1; lev <= og.getLevelInfo().getLevelCount(); ++lev) {
		og.forCells(lev, [&](const CRP::Cell& cell, const CRP::pv truncatedCellNumber) {
			std::cout << "level " << (int) lev << ": entry points=" << cell.numEntryPoints << ", exit points=" << cell.numExitPoints << "\n";
			for (CRP::index i = 0; i < cell.numEntryPoints; ++i) {
				const CRP::index entryPoint = og.getEntryPoint(cell, i);
				const CRP::OverlayVertex& entryVertex = og.getVertex(entryPoint);
				std::cout << "  " << entryVertex.originalVertex << std::endl;
				og.forOutNeighborsOf(entryPoint, lev, [&](CRP::index exitPoint, CRP::index weightIndex) {
					const CRP::OverlayVertex exitVertex = og.getVertex(exitPoint);
					std::cout << "    " << entryVertex.originalVertex << " " << exitVertex.originalVertex << " w="
							<< weights.getWeight(weightIndex) << std::endl;
				});
			}

		});
	}
}
