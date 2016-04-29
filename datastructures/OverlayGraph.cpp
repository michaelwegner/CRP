/*
 * OverlayGraph.cpp
 *
 *  Created on: Dec 14, 2015
 *      Author: Matthias Wolf & Michael Wegner
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

#include "OverlayGraph.h"

#include <bits/hashtable_policy.h>
#include <bits/unordered_map.h>
#include <algorithm>
#include <cassert>
#include <iostream>
#include <iterator>
#include <numeric>

#include "Graph.h"
#include "MultiLevelPartition.h"

namespace CRP {

OverlayGraph::OverlayGraph(Graph &graph, const MultiLevelPartition &mlp) : vertexCountInLevel(), levelInfo(mlp.getPVOffsets()) {
	build(graph, mlp.getNumberOfLevels());
}


void OverlayGraph::build(Graph &graph, level numberOfLevels) {
	std::vector<bool> exitFlagArray = buildOverlayVertices(graph, numberOfLevels);
	buildCells(graph, numberOfLevels, exitFlagArray);
}

std::vector<bool> OverlayGraph::buildOverlayVertices(Graph& graph, level numberOfLevels) {
	// Iterate over all edges to determine which induce overlay vertices
	// Store these according to the highest level in which the edge is a boundary edge
	std::vector<std::vector<OverlayVertex>> overlayVerticesByLevel(numberOfLevels);
	graph.forEdges([&](index start, index target, index forwardEdge) {
		pv startPV = graph.getCellNumber(start);
		pv targetPV = graph.getCellNumber(target);
		int overlayLevel = levelInfo.getHighestDifferingLevel(startPV, targetPV);
		if (overlayLevel > 0) {
			OverlayVertex startVertex;
			startVertex.cellNumber = startPV;
			startVertex.originalEdge = forwardEdge;
			startVertex.originalVertex = start;
			startVertex.neighborOverlayVertex = overlayVerticesByLevel[overlayLevel - 1].size() + 1;
			startVertex.entryExitPoint.resize(overlayLevel);
			overlayVerticesByLevel[overlayLevel - 1].push_back(startVertex);

			OverlayVertex targetVertex;
			targetVertex.cellNumber = targetPV;
			targetVertex.originalEdge = graph.findBackwardEdge(start, target);
			targetVertex.originalVertex = target;
			targetVertex.neighborOverlayVertex = overlayVerticesByLevel[overlayLevel - 1].size() - 1;
			targetVertex.entryExitPoint.resize(overlayLevel);
			overlayVerticesByLevel[overlayLevel - 1].push_back(targetVertex);
		}
	});

	// Calculates the number of overlay vertices in each level and the total number of overlay vertices
	assert(vertexCountInLevel.empty());
	vertexCountInLevel.reserve(overlayVerticesByLevel.size());
	for (auto& v : overlayVerticesByLevel) {
		vertexCountInLevel.push_back(v.size());
	}
	std::partial_sum(vertexCountInLevel.rbegin(), vertexCountInLevel.rend(), vertexCountInLevel.rbegin());
	const count overlayVertexCount = vertexCountInLevel[0];

	std::cout << "Num Overlay Vertices = " << overlayVertexCount << std::endl;

	// sort the overlay vertices by their cell numbers (but still keep the vertices ordered by their levels)
	// Additionally the reference to the neighboring overlay vertex is set correctly and the map from sub-vertices
	// of the original graph to vertices of the overlay graph is built.
	std::unordered_map<SubVertex, index, SubVertexHasher> originalToOverlayVertexMap;
	originalToOverlayVertexMap.reserve(overlayVertexCount);
	std::vector<bool> exitFlagsArray(overlayVertexCount);

	for (size_t j = 0; j < overlayVerticesByLevel.size(); ++j) {
		auto& v = overlayVerticesByLevel[j];
		const index vertexOffset = vertexCountInLevel[j] - v.size();
		std::vector<index> newToOldPosition(v.size());
		std::iota(newToOldPosition.begin(), newToOldPosition.end(), 0);

		std::sort(newToOldPosition.begin(), newToOldPosition.end(), [&](index lhs, index rhs) {
			return v[lhs].cellNumber < v[rhs].cellNumber;
		});

		std::vector<index> oldToNewPosition(newToOldPosition.size());
		for (index i = 0; i < newToOldPosition.size(); ++i) {
			oldToNewPosition[newToOldPosition[i]] = i;
		}
		std::vector<OverlayVertex> sortedVertices;
		sortedVertices.reserve(v.size());
		for (index i = 0; i < v.size(); ++i) {
			OverlayVertex& vertex = v[newToOldPosition[i]];
			vertex.neighborOverlayVertex = oldToNewPosition[vertex.neighborOverlayVertex] + vertexOffset;
			sortedVertices.push_back(vertex);

			bool isExitPoint = newToOldPosition[i] % 2 == 0;
			exitFlagsArray[i + vertexOffset] = isExitPoint;

			turnorder order;
			if (isExitPoint) {
				order = graph.getExitOrder(vertex.originalVertex, vertex.originalEdge);
			} else {
				order = graph.getEntryOrder(vertex.originalVertex, vertex.originalEdge);
			}
			SubVertex subVertex = {vertex.originalVertex, order, isExitPoint};
			originalToOverlayVertexMap[subVertex] = i + vertexOffset;
		}

		std::swap(v, sortedVertices);
	}

	// Starting with the vector containing the overlay vertices on the highest level, merge
	// the vectors into one vector
	assert(overlayVertices.empty());
	overlayVertices.reserve(overlayVertexCount);
	for (auto it = overlayVerticesByLevel.rbegin(); it != overlayVerticesByLevel.rend(); ++it) {
		assert(it->size() % 2 == 0);
		overlayVertices.insert(overlayVertices.end(), it->begin(), it->end());
	}
	assert(overlayVertices.size() == overlayVertexCount);

	// Build the mapping from original vertices to overlay vertices
	assert(originalToOverlayVertexMap.size() == overlayVertexCount);
	graph.setOverlayMapping(originalToOverlayVertexMap);
	return exitFlagsArray;
}

void OverlayGraph::buildCells(Graph& graph, level numberOfLevels, std::vector<bool> &exitFlagsArray) {
	struct BuildingCell {
		std::vector<index> entryPoints;
		std::vector<index> exitPoints;
	};

	cellMapping.resize(numberOfLevels);
	index cellOffset = 0;
	index overlayIdOffset = 0;
	for (level l = numberOfLevels - 1; l != static_cast<level>(-1); --l) {
		// Note that the actual level in the overlay graph is l+1
		auto& cellsInLevel = cellMapping[l];

		for (index v = 0; v < vertexCountInLevel[l]; ++v) {
			OverlayVertex& vertex = overlayVertices[v];
			bool isExitPoint = exitFlagsArray[v];
			pv cellNumberInLevel = levelInfo.truncateToLevel(vertex.cellNumber, l+1);

			auto cellPtr = cellsInLevel.find(cellNumberInLevel);
			if (cellPtr == cellsInLevel.end()) {
				// new cell
				vertex.entryExitPoint[l] = 0;

				Cell cell;
				if (isExitPoint) {
					cell.numExitPoints = 1;
					cell.numEntryPoints = 0;
				} else {
					cell.numExitPoints = 0;
					cell.numEntryPoints = 1;
				}
				cellsInLevel[cellNumberInLevel] = cell;
			} else {
				Cell& cell = cellPtr->second;
				if (isExitPoint) {
					vertex.entryExitPoint[l] = cell.numExitPoints++;
				} else {
					vertex.entryExitPoint[l] = cell.numEntryPoints++;
				}
			}
		}

		// calculate offsets
		for (auto it = cellsInLevel.begin(); it != cellsInLevel.end(); ++it) {
			Cell& cell = it->second;
			cell.overlayIdOffset = overlayIdOffset;
			cell.cellOffset = cellOffset;
			overlayIdOffset += cell.numEntryPoints + cell.numExitPoints;
			cellOffset += cell.numEntryPoints * cell.numExitPoints;
		}
	}

	// fill overlayIdMapping
	overlayIdMapping.resize(overlayIdOffset);
	for (level l = numberOfLevels - 1; l != static_cast<level>(-1); --l) {
		// Note that the actual level in the overlay graph is l+1
		auto& cellsInLevel = cellMapping[l];
		for (index v = 0; v < vertexCountInLevel[l]; ++v) {
			const OverlayVertex& vertex = overlayVertices[v];
			const bool isExitVertex = exitFlagsArray[v];
			const pv cellNumberInLevel = levelInfo.truncateToLevel(vertex.cellNumber, l + 1);
			assert(cellsInLevel.find(cellNumberInLevel) != cellsInLevel.end());
			const Cell& cell = cellsInLevel[cellNumberInLevel];

			index mappingIndex = cell.overlayIdOffset + vertex.entryExitPoint[l];
			if (isExitVertex) {
				mappingIndex += cell.numEntryPoints;
			}
			overlayIdMapping[mappingIndex] = v;
		}
	}

	weightVectorSize = cellOffset;
}

const Cell& OverlayGraph::getCell(pv cellNumber, level l) const {
	assert(0 < l && l <= levelInfo.getLevelCount());
	pv truncatedCellNumber = levelInfo.truncateToLevel(cellNumber, l);
	const auto& mappingForLevel = cellMapping[l - 1];
	auto it = mappingForLevel.find(truncatedCellNumber);
	assert(it != mappingForLevel.end());
	return it->second;
}

} /* namespace CRP */
