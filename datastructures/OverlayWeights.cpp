/*
 * OverlayWeights.cpp
 *
 *  Created on: 04.01.2016
 *      Author: Michael Wegner & Matthias Wolf
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

#include "OverlayWeights.h"
#include "id_queue.h"
#include "omp.h"

namespace CRP {

OverlayWeights::OverlayWeights(const Graph& graph, const OverlayGraph& overlayGraph, const CostFunction& costFunction)
	: weights(overlayGraph.getWeightVectorSize(), inf_weight)
{
	build(graph, overlayGraph, costFunction);
}

void OverlayWeights::build(const Graph& graph, const OverlayGraph& overlayGraph, const CostFunction& costFunction) {
	buildLowestLevel(graph, overlayGraph, costFunction);
	const LevelInfo& levelInfo = overlayGraph.getLevelInfo();
	for (level l = 2; l <= levelInfo.getLevelCount(); ++l) {
		buildLevel(graph, overlayGraph, costFunction, l);
	}
}

void OverlayWeights::buildLowestLevel(const Graph& graph, const OverlayGraph& overlayGraph, const CostFunction& costFunction) {
	// The weight of an edge to an exit point is always inf_weight if the boundary arc from the exit
	// point to a neighboring cell has inf_weight. Conceptually this should not be the case. The result,
	// however, is still correct since all paths that use the overlay edge to this exit point also contain the
	// boundary arc and, hence, have infinite weight.

	// overlayDist can be used by all threads in parallel since all threads work on different
	// cells and only update vertices in their overlay cell.
	std::vector<weight> overlayDist(overlayGraph.numberOfVertices(), inf_weight);

	index maxNumThreads = omp_get_max_threads();

	std::vector<std::vector<weight>> dist(maxNumThreads, std::vector<weight>(graph.getMaxEdgesInCell(), inf_weight));
	std::vector<MinIDQueue<IDKeyTriple>> queue(maxNumThreads, MinIDQueue<IDKeyTriple>(graph.getMaxEdgesInCell()));
	std::vector<std::vector<index>> round(maxNumThreads, std::vector<index>(graph.getMaxEdgesInCell(), 0));
	std::vector<index> currentRound(maxNumThreads, 0);

	overlayGraph.parallelForCells(1, [&](const Cell& cell, const pv cellNumber) {
		index threadId = omp_get_thread_num();
		for (index i = 0; i < cell.numEntryPoints; ++i) {
			index startOverlay = overlayGraph.getEntryPoint(cell, i);
			const OverlayVertex& overlayVertex = overlayGraph.getVertex(startOverlay);
			index start = overlayVertex.originalVertex;
			const index forwardCellOffset = graph.getBackwardEdgeCellOffset(start);
			index startId = overlayVertex.originalEdge - forwardCellOffset;
			assert(startId < graph.getMaxEdgesInCell());

			assert(overlayVertex.cellNumber == cellNumber);
			assert(queue[threadId].empty());

			currentRound[threadId]++;
			dist[threadId][startId] = 0;
			round[threadId][startId] = currentRound[threadId];
			queue[threadId].push({startId, start, 0});

			while (!queue[threadId].empty()) {
				auto minTriple = queue[threadId].pop();
				index uId = minTriple.id;
				index u = minTriple.vertexId;
				assert(uId < graph.getMaxEdgesInCell());

				assert(graph.getCellNumber(u) == cellNumber);
				assert(round[threadId][uId] == currentRound[threadId]);
				assert(dist[threadId][uId] == minTriple.key);

				graph.forOutEdgesOf(u, graph.getEntryOrder(u, uId + forwardCellOffset),
						[&](const ForwardEdge& edge, index exitPoint, Graph::TURN_TYPE turnType) {
					index v = edge.head;
					weight exitPointDist = minTriple.key + costFunction.getTurnCosts(turnType);
					weight newDist = exitPointDist + costFunction.getWeight(edge.attributes);
					if (newDist >= inf_weight) return;

					if (graph.getCellNumber(v) == cellNumber) {
						index vId = graph.getEntryOffset(v) + edge.entryPoint - forwardCellOffset;
						assert(vId < graph.getMaxEdgesInCell());
						if (round[threadId][vId] == currentRound[threadId] && newDist >= dist[threadId][vId]) return;
						dist[threadId][vId] = newDist;
						round[threadId][vId] = currentRound[threadId];
						queue[threadId].pushOrDecrease({vId, v, newDist});
					} else {
						// we found an exit point of the cell
						index exitOverlay = graph.getOverlayVertex(u, exitPoint, true);
						assert(exitOverlay < overlayGraph.numberOfVertices());
						if (exitPointDist < overlayDist[exitOverlay]) {
							overlayDist[exitOverlay] = exitPointDist;
						}
					}
				});
			}

			assert(queue[threadId].empty());
			for (index j = 0; j < cell.numExitPoints; ++j) {
				const index exitPoint = overlayGraph.getExitPoint(cell, j);
				assert(overlayDist[exitPoint] <= inf_weight);
				weights[cell.cellOffset + i*cell.numExitPoints + j] = overlayDist[exitPoint];
				overlayDist[exitPoint] = inf_weight;
			}

		}
	});
}

void OverlayWeights::buildLevel(const Graph& graph, const OverlayGraph& overlayGraph, const CostFunction& costFunction, level l) {
	assert(1 < l && l <= overlayGraph.getLevelInfo().getLevelCount());

	const LevelInfo& levelInfo = overlayGraph.getLevelInfo();
	const count numberOfOverlayVertices = overlayGraph.numberOfVerticesInLevel(l - 1);

	index maxNumThreads = omp_get_max_threads();

	std::vector<std::vector<weight>> dist(maxNumThreads, std::vector<weight>(numberOfOverlayVertices, inf_weight));
	std::vector<MinIDQueue<IDKeyPair>> queue(maxNumThreads, MinIDQueue<IDKeyPair>(numberOfOverlayVertices));
	std::vector<std::vector<index>> round(maxNumThreads, std::vector<index>(numberOfOverlayVertices, 0));
	std::vector<index> currentRound(maxNumThreads, 0);

	overlayGraph.parallelForCells(l, [&](const Cell& cell, const pv truncatedCellNumber) {
		index threadId = omp_get_thread_num();

		for (index i = 0; i < cell.numEntryPoints; ++i) {
			index start = overlayGraph.getEntryPoint(cell, i);

			++currentRound[threadId];
			dist[threadId][start] = 0;
			round[threadId][start] = currentRound[threadId];
			queue[threadId].push({start, 0});	// the queue only contains the entry points of (sub-)cells

			while (!queue[threadId].empty()) {
				auto minPair = queue[threadId].pop();
				index entry = minPair.id;
				assert(dist[threadId][entry] == minPair.key);
				assert(levelInfo.truncateToLevel(overlayGraph.getVertex(entry).cellNumber, l) == truncatedCellNumber);

				overlayGraph.forOutNeighborsOf(entry, l - 1, [&](index exit, index w) {
					weight newDist = minPair.key + weights[w];
					if (newDist >= inf_weight) return;
					if (round[threadId][exit] == currentRound[threadId] && newDist >= dist[threadId][exit]) return;

					// update distance of exit vertex
					dist[threadId][exit] = newDist;
					round[threadId][exit] = currentRound[threadId];

					// traverse original edge to neighboring (sub-)cell
					const OverlayVertex& exitVertex = overlayGraph.getVertex(exit);
					index neighbor = exitVertex.neighborOverlayVertex;
					const OverlayVertex& neighborVertex = overlayGraph.getVertex(neighbor);

					// check if the neighbor is still in the same overlay cell in level l
					if (levelInfo.truncateToLevel(neighborVertex.cellNumber, l) != truncatedCellNumber) return;

					weight edgeWeight = costFunction.getWeight(graph.getForwardEdge(exitVertex.originalEdge).attributes);
					dist[threadId][neighbor] = newDist + edgeWeight;
					if (queue[threadId].contains_id(neighbor)) {
						queue[threadId].decrease_key({neighbor, newDist + edgeWeight});
						assert(round[threadId][neighbor] == currentRound[threadId]);
					} else {
						queue[threadId].push({neighbor, newDist + edgeWeight});
						round[threadId][neighbor] = currentRound[threadId];
					}
				});
			}
			assert(queue[threadId].empty());

			// the distances of the exit points are the weights
			for (index j = 0; j < cell.numExitPoints; ++j) {
				index exit = overlayGraph.getExitPoint(cell, j);
				weights[cell.cellOffset + i * cell.numExitPoints + j] = (round[threadId][exit] == currentRound[threadId]) ? dist[threadId][exit] : inf_weight;
			}

		}
	});
}


}

