/*
 * PathUnpacking.cpp
 *
 *  Created on: 28.01.2016
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

#include "PathUnpacker.h"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <iterator>

#include "../datastructures/LevelInfo.h"
#include "../datastructures/OverlayGraph.h"

namespace CRP {

PathUnpacker::PathUnpacker(const Graph& graph, const OverlayGraph& overlayGraph, const std::vector<Metric>& metrics) : graph(graph), overlayGraph(overlayGraph),
		metrics(metrics), dist(graph.numberOfEdges() + overlayGraph.numberOfVertices()), round(graph.numberOfEdges() + overlayGraph.numberOfVertices(), 0),
		parent(graph.numberOfEdges() + overlayGraph.numberOfVertices()), currentRound(1), graphPQ(graph.numberOfEdges()), overlayGraphPQ(overlayGraph.numberOfVertices()) {}

std::vector<index> PathUnpacker::unpackPath(const std::vector<VertexIdPair> &packedPath, pv sourceCellNumber, pv targetCellNumber, index metricId) {
	std::vector<index> result;

	for (auto it = packedPath.begin(); it != packedPath.end(); ++it) {
		if (it->id < graph.numberOfEdges()) {
			// original vertex
			result.push_back(it->originalVertex);
		} else {
			// overlay vertex
			index entryVertex = it->id - graph.numberOfEdges();
			pv cellNumber = overlayGraph.getVertex(entryVertex).cellNumber;
			level queryLevel = overlayGraph.getLevelInfo().getQueryLevel(sourceCellNumber, targetCellNumber, cellNumber);
			it++;
			assert(it != packedPath.end());
			assert(it->id >= graph.numberOfEdges());
			index exitVertex = it->id - graph.numberOfEdges();
			unpackPathInOverlayCell(entryVertex, exitVertex, queryLevel, metricId, result);
		}
	}

	return result;
}



void PathUnpacker::unpackPathInOverlayCell(index sourceId, index targetId, level l, index metricId, std::vector<index>& result) {
	if (l == 1) {
		index newSourceId = overlayGraph.getVertex(sourceId).originalEdge;
		index neighborOfTarget = overlayGraph.getVertex(targetId).neighborOverlayVertex;
		index newTargetId = overlayGraph.getVertex(neighborOfTarget).originalEdge;
		unpackPathInLowestLevelCell(newSourceId, newTargetId, metricId, result);
		return;
	}

	const pv truncatedCellNumber = overlayGraph.getLevelInfo().truncateToLevel(overlayGraph.getVertex(sourceId).cellNumber, l);
	assert(truncatedCellNumber == overlayGraph.getLevelInfo().truncateToLevel(overlayGraph.getVertex(targetId).cellNumber, l));

	assert(overlayGraphPQ.empty());
	currentRound++;
	dist[sourceId] = 0;
	round[sourceId] = currentRound;
	overlayGraphPQ.push({sourceId, 0}); // the queue contains only entry overlay vertices and the target exit overlay vertex

	while (!overlayGraphPQ.empty()) {
		IDKeyPair minPair = overlayGraphPQ.pop();
		assert(dist[minPair.id] == minPair.key);
		assert(round[minPair.id] == currentRound);

		if (minPair.id == targetId) break;

		const LevelInfo& levelInfo = overlayGraph.getLevelInfo();
		assert(levelInfo.truncateToLevel(overlayGraph.getVertex(minPair.id).cellNumber, l) == truncatedCellNumber);

		overlayGraph.forOutNeighborsOf(minPair.id, l - 1, [&](index exit, index wOffset) {
			weight newDist = minPair.key + metrics[metricId].getCellWeight(wOffset);
			if (round[exit] == currentRound && dist[exit] <= newDist) return;

			dist[exit] = newDist;
			round[exit] = currentRound;
			parent[exit] = {overlayGraph.getVertex(minPair.id).originalVertex, minPair.id};
			if (exit == targetId) {
				overlayGraphPQ.pushOrDecrease({exit, newDist});
			}

			// traverse edge to next sub-cell (but only if we stay in the same cell in level l)
			index entry = overlayGraph.getVertex(exit).neighborOverlayVertex;
			pv entryCellNumber = overlayGraph.getVertex(entry).cellNumber;
			if (levelInfo.truncateToLevel(entryCellNumber, l) != truncatedCellNumber) return;

			ForwardEdge fEdge = graph.getForwardEdge(overlayGraph.getVertex(exit).originalEdge);
			newDist +=  metrics[metricId].getWeight(fEdge.attributes);

			dist[entry] = newDist;
			overlayGraphPQ.pushOrDecrease({entry, newDist});
			round[entry] = currentRound;
			parent[entry] = {overlayGraph.getVertex(exit).originalVertex, exit};
		});
	}

	overlayGraphPQ.clear();

	std::vector<index> overlayPath;
	index uId = targetId;
	overlayPath.push_back(targetId);
	while (uId != sourceId) {
		overlayPath.push_back(parent[uId].id);
		uId = parent[uId].id;
	}

	if (overlayPath.size() % 2 != 0) {
		std::copy(overlayPath.rbegin(), overlayPath.rend(), std::ostream_iterator<index>(std::cerr, " "));
		std::cerr << std::endl << sourceId << " " << targetId << std::endl;
	}
	assert(overlayPath.size() % 2 == 0);

	std::vector<index> path;
	for (auto it = overlayPath.rbegin(); it != overlayPath.rend(); it += 2) {
		unpackPathInOverlayCell(*it, *(it+1), l-1, metricId,  result);
	}
}


void PathUnpacker::unpackPathInLowestLevelCell(index sourceId, index targetId, index metricId, std::vector<index>& result) {
	index sourceVertex = graph.getHeadOfBackwardEdge(sourceId);

	assert(graph.getCellNumber(sourceVertex) != graph.getCellNumber(graph.getBackwardEdge(sourceId).tail));
	assert(graph.getCellNumber(graph.getBackwardEdge(targetId).tail) != graph.getCellNumber(graph.getHeadOfBackwardEdge(targetId)));

	const pv cellNumber = graph.getCellNumber(sourceVertex);
	assert(graph.getCellNumber(graph.getBackwardEdge(targetId).tail) == cellNumber);

	assert(graphPQ.empty());
	currentRound++;
	dist[sourceId] = 0;
	round[sourceId] = currentRound;
	graphPQ.push({sourceId, sourceVertex, 0});

	while (!graphPQ.empty()) {
		IDKeyTriple minTriple = graphPQ.pop();
		assert(dist[minTriple.id] == minTriple.key);
		assert(round[minTriple.id] == currentRound);

		if (minTriple.id == targetId) break;
		assert(graph.getCellNumber(graph.getHeadOfBackwardEdge(minTriple.id)) == cellNumber);

		graph.forOutEdgesOf(minTriple.vertexId, graph.getEntryOrder(minTriple.vertexId, minTriple.id),
				[&](const ForwardEdge& edge, index exitPoint, Graph::TURN_TYPE type) {
			index headVertex = edge.head;
			index headId = graph.getEntryOffset(headVertex) + edge.entryPoint;
			if (graph.getCellNumber(headVertex) != cellNumber && headId != targetId) return;

			weight newDist = minTriple.key + metrics[metricId].getTurnCosts(type) + metrics[metricId].getWeight(edge.attributes);
			if (newDist > inf_weight) return;
			if (round[headId] == currentRound && dist[headId] <= newDist) return;

			graphPQ.pushOrDecrease({headId, headVertex, newDist});
			round[headId] = currentRound;
			dist[headId] = newDist;
			parent[headId] = {minTriple.vertexId, minTriple.id};
		});
	}

	graphPQ.clear();

	std::vector<index> path;
	index uId = targetId;
	while (parent[uId].id != sourceId) {
		path.push_back(parent[uId].originalVertex);
		uId = parent[uId].id;
	}
	path.push_back(parent[uId].originalVertex);

	std::copy(path.rbegin(), path.rend(), back_inserter(result));
}

} /* namespace CRP */
