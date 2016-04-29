/*
 * OverlayGraph.h
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

#ifndef OVERLAYGRAPH_H_
#define OVERLAYGRAPH_H_

#include <vector>
#include <unordered_map>

#include "../constants.h"
#include "LevelInfo.h"

#include <iostream>

namespace CRP {
class Graph;
class MultiLevelPartition;
} /* namespace CRP */

namespace CRP {

/**
 * Stores an overlay vertex with all necessary information.
 * The neighborOverlayVertex is the neighboring vertex incident to originalEdge.
 * The originalEdge is either an index to a @ref ForwardEdge (in case the overlay vertex is an exit vertex) or an index to a @ref BackwardEdge
 * (in case the overlay vertex is an entry vertex).
 * The vector entryExitPoint stores for each level l on which this vertex is an overlay vertex the entry/exit point index in its cell on level l.
 */
struct OverlayVertex {
	index originalVertex;
	index neighborOverlayVertex;
	pv cellNumber;
	index originalEdge;
	std::vector<index> entryExitPoint;
};

struct Cell {
	index numEntryPoints; // p_C
	index numExitPoints; // q_C
	index cellOffset; // f_C
	index overlayIdOffset; // f_C~, maps entry/exit point of cell to overlay vertex.
};

class OverlayGraph {
public:
	OverlayGraph(const std::vector<OverlayVertex>& overlayVertices, const std::vector<index>& vertexCountInLevel,
			const std::vector<std::unordered_map<pv, Cell>>& cellMapping, const std::vector<index>& overlayIdMapping,
			const LevelInfo& levelInfo, count weightVectorSize) : overlayVertices(overlayVertices),
			vertexCountInLevel(vertexCountInLevel), cellMapping(cellMapping), overlayIdMapping(overlayIdMapping),
			levelInfo(levelInfo), weightVectorSize(weightVectorSize) {}

	OverlayGraph(Graph &graph, const MultiLevelPartition &mlp);

	OverlayGraph() = default;

	inline const OverlayVertex& getVertex(index u) const {
		assert(u < overlayVertices.size());
		return overlayVertices[u];
	}

	const Cell& getCell(pv cellNumber, level l) const;

	template <typename L> void forVertices(L handle) const;

	/**
	 * Iterates over all outgoing neighbors of @a u.
	 * @param u An entry vertex
	 * @param l Level of the cell
	 * @param handle must handle an index to exit OverlayVertex and an index into the weight array
	 */
	template <typename L> void forOutNeighborsOf(index u, level l, L handle) const;

	/**
	 * Iterates over all incoming neighbors of @a u.
	 * @param v An exit vertex
	 * @param l Level of the cell
	 * @param handle must handle an index to entry OverlayVertex and an index into the weight array
	 */
	template <typename L> void forInNeighborsOf(index v, level l, L handle) const;

	/**
	 * Iterates over all cells in level @a l.
	 * @param l the level
	 * @param handle must handle a const Cell& and its (truncated) pv.
	 */
	template <typename L> void forCells(level l, L handle) const;

	/**
	 * Iterates over all cells in level @a l in parallel.
	 * @param l the level
	 * @param handle must handle a const Cell& and its (truncated) pv.
	 */
	template <typename L> void parallelForCells(level l, L handle) const;


	inline count numberOfVertices() const {
		return overlayVertices.size();
	}

	inline count numberOfVerticesInLevel(level l) const {
		assert(0 < l && l <= vertexCountInLevel.size());
		return vertexCountInLevel[l - 1];
	}

	inline count numberOfCellsInLevel(level l) const {
		assert(0 < l && l <= cellMapping.size());
		return cellMapping[l - 1].size();
	}

	/**
	 * Returns the index of the OverlayVertex that is the entry point of the @a cell
	 * with index @a entryPointIndex
	 * @param cell
	 * @param entryPointIndex
	 * @return
	 */
	inline index getEntryPoint(const Cell& cell, index entryPointIndex) const {
		assert(entryPointIndex < cell.numEntryPoints);
		return overlayIdMapping[cell.overlayIdOffset + entryPointIndex];
	}

	/**
	 * Returns the index of the OverlayVertex that is the exit point of the @a cell
	 * with index @a exitPointIndex
	 * @param cell
	 * @param exitPointIndex
	 * @return
	 */
	inline index getExitPoint(const Cell& cell, index exitPointIndex) const {
		assert(exitPointIndex < cell.numExitPoints);
		return overlayIdMapping[cell.overlayIdOffset + cell.numEntryPoints + exitPointIndex];
	}

	inline const LevelInfo& getLevelInfo() const {
		return levelInfo;
	}

	inline level getQueryLevel(const pv sCellNumber, const pv tCellNumber, const pv vCellNumber) const {
		return levelInfo.getQueryLevel(sCellNumber, tCellNumber, vCellNumber);
	}

	inline const count getWeightVectorSize() const {
		return weightVectorSize;
	}

	inline const std::vector<index> getOverlayIdMapping() const {
		return overlayIdMapping;
	}

private:
	std::vector<OverlayVertex> overlayVertices;
	std::vector<count> vertexCountInLevel;
	std::vector<std::unordered_map<pv, Cell>> cellMapping;
	std::vector<index> overlayIdMapping;
	LevelInfo levelInfo;
	count weightVectorSize;

	void build(Graph &graph, level numberOfLevels);
	/**
	 * Builds the overlay vertices but does not set the OverlayVertex::entryExitPoint (as this is
	 * still unknown). It does however reserve the memory needed to store this information, i.e.
	 * in the following it may be assumed that the vector has already the correct size.
	 * @param graph the graph
	 * @param numberOfLevels the number of levels
	 */
	std::vector<bool> buildOverlayVertices(Graph &graph, level numberOfLevels);
	/**
	 * Builds the cells and sets OverlayVertex::entryExitPoint for all overlayVertices.
	 * @param graph the original graph
	 * @param numberOfLevels the number of levels
	 * above)
	 */
	void buildCells(Graph &graph, level numberOfLevels, std::vector<bool> &exitFlagsArray);
};

template<typename L>
void OverlayGraph::forVertices(L handle) const {
	for (const OverlayVertex& vertex : overlayVertices) {
		handle(vertex);
	}
}

template<typename L>
void OverlayGraph::forOutNeighborsOf(index u, level l, L handle) const {
	const OverlayVertex& vertex = getVertex(u);
	assert(0 < l && l <= vertex.entryExitPoint.size());
	index entryPoint = vertex.entryExitPoint[l - 1];
	const Cell& cell = getCell(vertex.cellNumber, l);
	index weightOffset = cell.cellOffset + entryPoint * cell.numExitPoints;
	index overlayIdOffset = cell.overlayIdOffset + cell.numEntryPoints;
	for (index i = 0; i < cell.numExitPoints; ++i) {
		assert(overlayIdOffset+i < overlayIdMapping.size());
		handle(overlayIdMapping[overlayIdOffset + i], weightOffset + i);
	}
}

template<typename L>
void OverlayGraph::forInNeighborsOf(index v, level l, L handle) const {
	const OverlayVertex& vertex = getVertex(v);
	assert(0 < l && l <= vertex.entryExitPoint.size());
	index exitPoint = vertex.entryExitPoint[l - 1];
	const Cell& cell = getCell(vertex.cellNumber, l);
	index weightOffset = cell.cellOffset + exitPoint;
	index overlayIdOffset = cell.overlayIdOffset;
	for (index i = 0; i < cell.numEntryPoints; ++i) {
		assert(overlayIdOffset+i < overlayIdMapping.size());
		handle(overlayIdMapping[overlayIdOffset + i], weightOffset + cell.numExitPoints * i);
	}
}

template<typename L>
void OverlayGraph::forCells(level l, L handle) const {
	assert(0 < l && l <= levelInfo.getLevelCount());

	for (auto it = cellMapping[l-1].begin(); it != cellMapping[l-1].end(); ++it) {
		handle(it->second, it->first);
	}
}

template<typename L>
void OverlayGraph::parallelForCells(level l, L handle) const {
	assert(0 < l && l <= levelInfo.getLevelCount());
	std::vector<pv> keys(cellMapping[l-1].size());
	std::vector<Cell> cells(cellMapping[l-1].size());
	index i = 0;
	for (auto it = cellMapping[l-1].begin(); it != cellMapping[l-1].end(); ++it, ++i) {
		keys[i] = it->first;
		cells[i] = it->second;
	}

#pragma omp parallel for schedule(dynamic)
	for (index i = 0; i < keys.size(); ++i) {
		handle(cells[i], keys[i]);
	}
}

} /* namespace CRP */

#endif /* OVERLAYGRAPH_H_ */
