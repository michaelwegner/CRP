/*
 * PathUnpacking.h
 *
 *  Created on: 28.01.2016
 *      Author: Matthias Wolf
 *
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

#ifndef ALGORITHM_PATHUNPACKER_H_
#define ALGORITHM_PATHUNPACKER_H_

#include <memory>
#include <vector>

#include "../constants.h"
#include "../datastructures/Graph.h"
#include "../datastructures/id_queue.h"
#include "../datastructures/OverlayWeights.h"
#include "../metrics/Metric.h"

namespace CRP {

class PathUnpacker {
public:
	PathUnpacker(const Graph& graph, const OverlayGraph& overlayGraph, const std::vector<Metric>& metrics);

	/**
	 * Unpacks a path in which original vertices are represented by the id of the entry point at this vertex (i.e.
	 * the id of the backward edge to this original vertex) and overlay vertices by their id + graph.numberOfEdges().
	 * @param packedPath a vector containing the packed path
	 * @param sourceCellNumber the cell number of the source vertex
	 * @param targetCellNumber the cell number of the target vertex
	 * @return the unpacked path
	 */
	std::vector<index> unpackPath(const std::vector<VertexIdPair> &packedPath, pv sourceCellNumber, pv targetCellNumber, index metricId);
private:
	const Graph &graph;
	const OverlayGraph &overlayGraph;
	const std::vector<Metric> &metrics;

	std::vector<weight> dist;
	std::vector<count> round;
	std::vector<VertexIdPair> parent;
	count currentRound;

	MinIDQueue<IDKeyTriple> graphPQ;
	MinIDQueue<IDKeyPair> overlayGraphPQ;

	/**
	 * Calculates the shortest path from the source overlay vertex to the target inside an overlay cell. The source
	 * has to be an entry overlay vertex and the target must be an exit overlay vertex of the same cell.
	 * @param source the id of the source overlay vertex
	 * @param target the id of the target overlay vertex
	 * @param l the level of the cell
	 * @param result the vector to which the result will be appended. The path is represented as the sequence
	 * of IDs of (original) vertices on the shortest path. The path includes the original vertices of the start and
	 * target.
	 */
	void unpackPathInOverlayCell(index source, index target, level l, index metricId, std::vector<index>& result);

	/**
	 * Calculates the shortest path from the source edge to the target edge inside a cell on level 1.
	 * The entry and exit vertices must belong to the same cell. In order to calculate the path
	 * between two entry overlay vertices use their original edges as startId and targetId, respectively.
	 * @param sourceId the ID of the source entry point corresponding to a backward edge
	 * @param targetId the ID of the target entry point corresponding to the backward edge whose tail is the actual target
	 * exit point.
	 * @param result the vector to which the result will be appended. The path is represented
	 * as the sequence of IDs of vertices on the shortest path. The sequence includes the start and the target.
	 */
	void unpackPathInLowestLevelCell(index sourceEdgeId, index targetEdgeId, index metricId, std::vector<index>& result);

};

} /* namespace CRP */

#endif /* ALGORITHM_PATHUNPACKER_H_ */
