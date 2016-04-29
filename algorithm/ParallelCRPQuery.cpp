/*
 * ParallelCRPQuery.cpp
 *
 *  Created on: Jan 8, 2016
 *      Author: Michael Wegner & Matthias Wolf
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

#include "ParallelCRPQuery.h"

#include <algorithm>
#include <cassert>

#include "../timer.h"
#include "omp.h"

namespace CRP {

ParallelCRPQuery::ParallelCRPQuery(const Graph& graph, const OverlayGraph& overlayGraph, const std::vector<Metric>& metrics, PathUnpacker& pathUnpacker) : Query(graph, overlayGraph, metrics), pathUnpacker(pathUnpacker) {
	const count vectorSize = 2 * graph.getMaxEdgesInCell() + overlayGraph.numberOfVertices();
	forwardInfo = std::vector<VertexInfo>(vectorSize, {inf_weight, 0});
	backwardInfo = std::vector<VertexInfo>(vectorSize, {inf_weight, 0});

	forwardGraphPQ = MinIDQueue<IDKeyTriple>(2 * graph.getMaxEdgesInCell());
	forwardOverlayGraphPQ = MinIDQueue<IDKeyTriple>(overlayGraph.numberOfVertices());

	backwardGraphPQ = MinIDQueue<IDKeyTriple>(2*graph.getMaxEdgesInCell());
	backwardOverlayGraphPQ = MinIDQueue<IDKeyTriple>(overlayGraph.numberOfVertices());

	currentRound = 0;
}

QueryResult ParallelCRPQuery::vertexQuery(index sourceVertexId, index targetVertexId, index metricId) {
	const BackwardEdge &backwardEdgeToStart = graph.getBackwardEdge(graph.getEntryOffset(sourceVertexId));
	index sourceEdgeId = graph.getExitOffset(backwardEdgeToStart.tail) + backwardEdgeToStart.exitPoint;

	const ForwardEdge &forwardEdgeFromTarget = graph.getForwardEdge(graph.getExitOffset(targetVertexId));
	index targetEdgeId = graph.getEntryOffset(forwardEdgeFromTarget.head) + forwardEdgeFromTarget.entryPoint;
	
	return edgeQuery(sourceEdgeId, targetEdgeId, metricId);
}

QueryResult ParallelCRPQuery::edgeQuery(index sourceEdgeId, index targetEdgeId, index metricId) {
	currentRound++;

	const index s = graph.getForwardEdge(sourceEdgeId).head;
	const index sGlobalId = graph.getEntryOffset(s) + graph.getForwardEdge(sourceEdgeId).entryPoint;
	const pv sCellNumber = graph.getCellNumber(s);
	const index t = graph.getBackwardEdge(targetEdgeId).tail;
	const pv tCellNumber = graph.getCellNumber(t);
	const index tGlobalId = graph.getExitOffset(t) + graph.getBackwardEdge(targetEdgeId).exitPoint;

	const int forwardSOffset = graph.getBackwardEdgeCellOffset(s);
	const int forwardTOffset = graph.getBackwardEdgeCellOffset(t) - graph.getMaxEdgesInCell();
	const int backwardSOffset = graph.getForwardEdgeCellOffset(s);
	const int backwardTOffset = graph.getForwardEdgeCellOffset(t) - graph.getMaxEdgesInCell();
	const index overlayOffset = 2 * graph.getMaxEdgesInCell();


	const index sForwardId = sGlobalId - forwardSOffset;
	assert(sForwardId < graph.getMaxEdgesInCell());
	const index tBackwardId = tGlobalId - ((sCellNumber == tCellNumber) ? backwardSOffset : backwardTOffset);
	assert(tBackwardId < 2*graph.getMaxEdgesInCell());

	weight shortestPath = 2 * inf_weight;
	weight forwardPath = 2 * inf_weight;
	weight backwardPath = 2 * inf_weight;
	for (index i = 0; i < overlayOffset; ++i) {
			forwardInfo[i].dist = inf_weight;
			backwardInfo[i].dist = inf_weight;
	}

	forwardInfo[sForwardId] = {0, currentRound, {s, sForwardId}};
	forwardGraphPQ.push({sForwardId, s, 0});

	backwardInfo[tBackwardId] = {0, currentRound, {t, tBackwardId}};
	backwardGraphPQ.push({tBackwardId, t, 0});

	VertexIdPair forwardMid;
	VertexIdPair backwardMid;

#pragma omp parallel num_threads(2)
	{
		index threadId = omp_get_thread_num();
		if (threadId == 0) { // forward
			while (forwardGraphPQ.size() + forwardOverlayGraphPQ.size() > 0) {
				if (std::min(forwardPath, backwardPath) < std::min(forwardGraphPQ.peekKey(), forwardOverlayGraphPQ.peekKey()) + std::min(backwardGraphPQ.peekKey(), backwardOverlayGraphPQ.peekKey())) break;

				if (forwardGraphPQ.empty() && forwardOverlayGraphPQ.empty()) continue;
				if (forwardGraphPQ.peekKey() < forwardOverlayGraphPQ.peekKey()) { // graph vertex
					IDKeyTriple triple = forwardGraphPQ.pop();
					index u = triple.vertexId;
					index uId = triple.id;
					turnorder entryPoint;
					if (uId < graph.getMaxEdgesInCell()) {
						entryPoint = uId + forwardSOffset - graph.getEntryOffset(u);
					} else {
						entryPoint = uId + forwardTOffset - graph.getEntryOffset(u);
					}

					// stalling
					count uInDeg = graph.getInDegree(u);
					count uOffset = uInDeg * entryPoint;
					index offset = uId < graph.getMaxEdgesInCell()? forwardSOffset : forwardTOffset;
					for (index j = 0, entryId = graph.getEntryOffset(u) - offset; entryId < graph.getEntryOffset(u) + uInDeg - offset; ++entryId, ++j) {
						if (forwardInfo[entryId].round < currentRound) { // only if we do not have a valid distance label already
							forwardInfo[entryId].dist = std::min(forwardInfo[entryId].dist, (weight) std::max(0, (int) forwardInfo[uId].dist + metrics[metricId].getMaxEntryTurnTableDiff(u, uOffset + j)));
						}
					}


					assert(entryPoint < graph.getInDegree(u));
					graph.forOutEdgesOf(u, entryPoint, [&](const ForwardEdge &e, index exitPoint, Graph::TURN_TYPE type) {
						index v = e.head;
						level vQueryLevel = overlayGraph.getQueryLevel(sCellNumber, tCellNumber, graph.getCellNumber(v));
						weight edgeWeight = metrics[metricId].getWeight(e.attributes);
						weight turnCosts = metrics[metricId].getTurnCosts(type);
						if (u == s) turnCosts = 0;
						weight newDist = forwardInfo[uId].dist + edgeWeight + turnCosts;

						if (newDist >= inf_weight) return;

						if (vQueryLevel == 0) { // graph
							index vId = graph.getEntryOffset(v) + e.entryPoint;
							bool vInSCell = graph.getCellNumber(v) == sCellNumber;
							if (vInSCell) {
								vId -= forwardSOffset;
								assert(vId < graph.getMaxEdgesInCell());
							} else {
								vId -= forwardTOffset;
								assert(graph.getMaxEdgesInCell() <= vId && vId < 2*graph.getMaxEdgesInCell());
							}

							if (forwardInfo[vId].round < currentRound && newDist > forwardInfo[vId].dist) return; // we haven't seen vId yet and we cannot improve anything from this entryPoint

							if (forwardInfo[vId].round < currentRound || newDist < forwardInfo[vId].dist) {
								forwardInfo[vId].dist = newDist;
								forwardGraphPQ.pushOrDecrease({vId, v, newDist});
								forwardInfo[vId].round = currentRound;
								forwardInfo[vId].parent = {u, uId};

								// check whether we already visited an exit point
								const index exitOffset = graph.getExitOffset(v) - (vInSCell ? backwardSOffset : backwardTOffset);
								index exitId = exitOffset;
								graph.forOutEdgesOf(v, e.entryPoint, [&](const ForwardEdge&, index vExitPoint, Graph::TURN_TYPE vType) {
									if (backwardInfo[exitId].round == currentRound) {
										weight newPathLength = forwardInfo[vId].dist + metrics[metricId].getTurnCosts(vType) + backwardInfo[exitId].dist;
										if (newPathLength < forwardPath) {
											forwardPath = newPathLength;
											forwardMid = {v, vId};
											backwardMid = {v, exitId};
										}
									}
									++exitId;
								});
							}
						} else { // v is in another cell on another level
							v = graph.getOverlayVertex(v, e.entryPoint, false);
							index vId = v + overlayOffset;
							assert(overlayOffset <= vId && vId < overlayOffset + overlayGraph.numberOfVertices());
							if (forwardInfo[vId].round < currentRound || newDist < forwardInfo[vId].dist) {
								forwardInfo[vId].dist = newDist;
								forwardOverlayGraphPQ.pushOrDecrease({v, vQueryLevel, newDist});
								forwardInfo[vId].round = currentRound;
								forwardInfo[vId].parent = {u, uId};
								if (backwardInfo[vId].round == currentRound && forwardInfo[vId].dist + backwardInfo[vId].dist < forwardPath) {
									forwardPath = forwardInfo[vId].dist + backwardInfo[vId].dist;
									forwardMid = {v, vId};
									backwardMid = {v, vId};
								}
							}
						}
					});
				} else { // overlay vertex
					IDKeyTriple triple = forwardOverlayGraphPQ.pop();
					index u = triple.id;
					index uId = u + overlayOffset;
					const OverlayVertex& uVertex = overlayGraph.getVertex(u);
					assert(graph.getCellNumber(uVertex.originalVertex) == uVertex.cellNumber);
					level uQueryLevel = triple.vertexId;
					overlayGraph.forOutNeighborsOf(u, uQueryLevel, [&](index v, index wOffset) {
						weight newDist = forwardInfo[uId].dist + metrics[metricId].getCellWeight(wOffset);
						if (newDist >= inf_weight) return;
						index vId = v + overlayOffset;
						if (forwardInfo[vId].round < currentRound || newDist < forwardInfo[vId].dist) {
							forwardInfo[vId].dist = newDist;
							forwardInfo[vId].round = currentRound;
							forwardInfo[vId].parent = {uVertex.originalVertex, uId};

							const OverlayVertex& vVertex = overlayGraph.getVertex(v);

							// traverse edge to next cell
							const ForwardEdge &fEdge = graph.getForwardEdge(vVertex.originalEdge);
							newDist = forwardInfo[vId].dist + metrics[metricId].getWeight(fEdge.attributes);

							if (newDist >= inf_weight) return;

							index w = vVertex.neighborOverlayVertex;
							const OverlayVertex& wVertex = overlayGraph.getVertex(w);
							level wQueryLevel = overlayGraph.getQueryLevel(sCellNumber, tCellNumber, wVertex.cellNumber);

							if (wQueryLevel == 0) { // we are back on the graph
								assert(wVertex.cellNumber == sCellNumber || wVertex.cellNumber == tCellNumber);
								bool wInSCell = wVertex.cellNumber == sCellNumber;
								index originalW = wVertex.originalVertex;
								index originalWId = graph.getEntryOffset(originalW) + fEdge.entryPoint;
								if (wInSCell) {
									originalWId -= forwardSOffset;
								} else {
									originalWId -= forwardTOffset;
								}

								if (forwardInfo[originalWId].round < currentRound && newDist > forwardInfo[originalWId].dist) return;  // we haven't seen originalWId yet and we cannot improve anything from this entryPoint

								if (forwardInfo[originalWId].round < currentRound || newDist < forwardInfo[originalWId].dist) {
									forwardInfo[originalWId].dist = newDist;
									forwardGraphPQ.pushOrDecrease({originalWId, originalW, newDist});
									forwardInfo[originalWId].round = currentRound;
									forwardInfo[originalWId].parent = {vVertex.originalVertex, vId};

									// check whether we already visited an exit point
									const index exitOffset = graph.getExitOffset(originalW) - (wInSCell ? backwardSOffset : backwardTOffset);
									index exitId = exitOffset;
									graph.forOutEdgesOf(originalW, fEdge.entryPoint, [&](const ForwardEdge&, index wExitPoint, Graph::TURN_TYPE wType) {
										if (backwardInfo[exitId].round == currentRound) {
											weight newPathLength = forwardInfo[originalWId].dist + metrics[metricId].getTurnCosts(wType) + backwardInfo[exitId].dist;
											if (newPathLength < forwardPath) {
												forwardPath = newPathLength;
												forwardMid = {originalW, originalWId};
												backwardMid = {originalW, exitId};
											}
										}
										++exitId;
									});
								}
							} else {
								index wId = w + overlayOffset;
								if (forwardInfo[wId].round < currentRound || newDist < forwardInfo[wId].dist) {
									forwardInfo[wId].dist = newDist;
									forwardOverlayGraphPQ.pushOrDecrease({w, wQueryLevel, newDist});
									forwardInfo[wId].round = currentRound;
									forwardInfo[wId].parent = {vVertex.originalVertex, vId};
									if (backwardInfo[wId].round == currentRound && forwardInfo[wId].dist + backwardInfo[wId].dist < forwardPath) {
										forwardPath = forwardInfo[wId].dist + backwardInfo[wId].dist;
										forwardMid = {wVertex.originalVertex, wId};
										backwardMid = {wVertex.originalVertex, wId};
									}
								}
							}
						}
					});
				}
			}
		} else { // threadId == 1 -> backward
			while (backwardGraphPQ.size() + backwardOverlayGraphPQ.size() > 0) {
				if (std::min(forwardPath, backwardPath) < std::min(forwardGraphPQ.peekKey(), forwardOverlayGraphPQ.peekKey()) + std::min(backwardGraphPQ.peekKey(), backwardOverlayGraphPQ.peekKey())) break;

				if (backwardGraphPQ.empty() && backwardOverlayGraphPQ.empty()) continue;
				if (backwardGraphPQ.peekKey() < backwardOverlayGraphPQ.peekKey()) { // graph vertex
					IDKeyTriple triple = backwardGraphPQ.pop();
					index u = triple.vertexId;
					index uId = triple.id;
					turnorder exitPoint;
					if (uId < graph.getMaxEdgesInCell()) {
						exitPoint = uId + backwardSOffset - graph.getExitOffset(u);
					} else {
						exitPoint = uId + backwardTOffset - graph.getExitOffset(u);
					}
					assert(exitPoint < graph.getOutDegree(u));

					// stalling
					count uOutDeg = graph.getOutDegree(u);
					count uOffset = uOutDeg * exitPoint;
					index offset = uId < graph.getMaxEdgesInCell()? backwardSOffset : backwardTOffset;
					for (index j = 0, exitId = graph.getExitOffset(u) - offset; exitId < graph.getExitOffset(u) + uOutDeg - offset; ++exitId, ++j) {
						if (backwardInfo[exitId].round < currentRound) { // only if we do not have a valid distance label already
							backwardInfo[exitId].dist = std::min(backwardInfo[exitId].dist, (weight) std::max(0, (int) backwardInfo[uId].dist + metrics[metricId].getMaxExitTurnTableDiff(u, uOffset + j)));
						}
					}

					graph.forInEdgesOf(u, exitPoint, [&](const BackwardEdge &e, index entryPoint, Graph::TURN_TYPE type) {
						index v = e.tail;
						level vQueryLevel = overlayGraph.getQueryLevel(sCellNumber, tCellNumber, graph.getCellNumber(v));
						weight edgeWeight = metrics[metricId].getWeight(e.attributes);
						weight turnCosts = metrics[metricId].getTurnCosts(type);
						if (u == t) turnCosts = 0;
						weight newDist = backwardInfo[uId].dist + edgeWeight + turnCosts;

						if (newDist >= inf_weight) return;

						if (vQueryLevel == 0) { // graph
							index vId = graph.getExitOffset(v) + e.exitPoint;
							bool vInSCell = graph.getCellNumber(v) == sCellNumber;
							if (vInSCell) {
								vId -= backwardSOffset;
								assert(vId < graph.getMaxEdgesInCell());
							} else {
								vId -= backwardTOffset;
								assert(graph.getMaxEdgesInCell() <= vId && vId < 2*graph.getMaxEdgesInCell());
							}


							if (backwardInfo[vId].round < currentRound && newDist > backwardInfo[vId].dist) return; // we haven't seen vId yet and we cannot improve anything from this entryPoint

							if (backwardInfo[vId].round < currentRound || newDist < backwardInfo[vId].dist) {
								backwardInfo[vId].dist = newDist;
								backwardGraphPQ.pushOrDecrease({vId, v, newDist});
								backwardInfo[vId].round = currentRound;
								backwardInfo[vId].parent = {u, uId};

								// check whether we already visited an entry point
								const index entryOffset = graph.getEntryOffset(v) - (vInSCell ? forwardSOffset : forwardTOffset);
								index entryId = entryOffset;
								graph.forInEdgesOf(v, e.exitPoint, [&](const BackwardEdge&, index vEntryPoint, Graph::TURN_TYPE vType) {
									if (forwardInfo[entryId].round == currentRound) {
										weight newPathLength = forwardInfo[entryId].dist + metrics[metricId].getTurnCosts(vType) + backwardInfo[vId].dist;
										if (newPathLength < backwardPath) {
											backwardPath = newPathLength;
											forwardMid = {v, entryId};
											backwardMid = {v, vId};
										}
									}
									++entryId;
								});
							}
						} else { // v is in another cell on another level
							v = graph.getOverlayVertex(v, e.exitPoint, true);
							index vId = v + overlayOffset;
							assert(overlayOffset <= vId && vId < overlayOffset + overlayGraph.numberOfVertices());
							if (backwardInfo[vId].round < currentRound || backwardInfo[uId].dist + edgeWeight < backwardInfo[vId].dist) {
								backwardInfo[vId].dist = backwardInfo[uId].dist + edgeWeight;
								backwardOverlayGraphPQ.pushOrDecrease({v, vQueryLevel, backwardInfo[vId].dist});
								backwardInfo[vId].round = currentRound;
								backwardInfo[vId].parent = {u, uId};
								if (forwardInfo[vId].round == currentRound && forwardInfo[vId].dist + backwardInfo[vId].dist < backwardPath) {
									backwardPath = forwardInfo[vId].dist + backwardInfo[vId].dist;
									forwardMid = {v, vId};
									backwardMid = {v, vId};
								}
							}
						}
					});
				} else { // overlay vertex
					IDKeyTriple triple = backwardOverlayGraphPQ.pop();
					index u = triple.id;
					index uId = u + overlayOffset;
					const OverlayVertex& uVertex = overlayGraph.getVertex(u);
					assert(graph.getCellNumber(uVertex.originalVertex) == uVertex.cellNumber);
					level uQueryLevel = triple.vertexId;

					overlayGraph.forInNeighborsOf(u, uQueryLevel, [&](index v, index wOffset) {
						weight newDist = backwardInfo[uId].dist + metrics[metricId].getCellWeight(wOffset);
						if (newDist >= inf_weight) return;
						index vId = v + overlayOffset;
						if (backwardInfo[vId].round < currentRound || newDist < backwardInfo[vId].dist) {
							backwardInfo[vId].dist = newDist;
							backwardInfo[vId].round = currentRound;
							backwardInfo[vId].parent = {uVertex.originalVertex, uId};

							const OverlayVertex& vVertex = overlayGraph.getVertex(v);

							// traverse edge to next cell
							const BackwardEdge &bEdge = graph.getBackwardEdge(vVertex.originalEdge);
							newDist = backwardInfo[vId].dist + metrics[metricId].getWeight(bEdge.attributes);
							if (newDist >= inf_weight) return;

							index w = vVertex.neighborOverlayVertex;
							const OverlayVertex& wVertex = overlayGraph.getVertex(w);
							level wQueryLevel = overlayGraph.getQueryLevel(sCellNumber, tCellNumber, wVertex.cellNumber);
							if (wQueryLevel == 0) { // we are back on the graph
								assert(wVertex.cellNumber == sCellNumber || wVertex.cellNumber == tCellNumber);
								bool wInSCell = wVertex.cellNumber == sCellNumber;
								index originalW = wVertex.originalVertex;
								index originalWId = graph.getExitOffset(originalW) + bEdge.exitPoint;
								if (wInSCell) {
									originalWId -= backwardSOffset;
								} else {
									originalWId -= backwardTOffset;
								}

								if (backwardInfo[originalWId].round < currentRound && newDist > backwardInfo[originalWId].dist) return;  // we haven't seen originalWId yet and we cannot improve anything from this exitPoint

								if (backwardInfo[originalWId].round < currentRound || newDist < backwardInfo[originalWId].dist) {
									backwardInfo[originalWId].dist = newDist;
									backwardGraphPQ.pushOrDecrease({originalWId, originalW, newDist});
									backwardInfo[originalWId].round = currentRound;
									backwardInfo[originalWId].parent = {vVertex.originalVertex, vId};

									// check whether we already visited an entry point
									const index entryOffset = graph.getEntryOffset(originalW) - (wInSCell ? forwardSOffset : forwardTOffset);
									index entryId = entryOffset;
									graph.forInEdgesOf(originalW, bEdge.exitPoint, [&](const BackwardEdge&, index wEntryPoint, Graph::TURN_TYPE wType) {
										if (forwardInfo[entryId].round == currentRound) {
											weight newPathLength = forwardInfo[entryId].dist + metrics[metricId].getTurnCosts(wType) + backwardInfo[originalWId].dist;
											if (newPathLength < backwardPath) {
												backwardPath = newPathLength;
												forwardMid = {originalW, entryId};
												backwardMid = {originalW, originalWId};
											}
										}
										++entryId;
									});
								}
							} else {
								index wId = w + overlayOffset;
								if (backwardInfo[wId].round < currentRound || newDist < backwardInfo[wId].dist) {
									backwardInfo[wId].dist = newDist;
									backwardOverlayGraphPQ.pushOrDecrease({w, wQueryLevel,  newDist});
									backwardInfo[wId].round = currentRound;
									backwardInfo[wId].parent = {vVertex.originalVertex, vId};
									if (forwardInfo[wId].round == currentRound && forwardInfo[wId].dist + backwardInfo[wId].dist < backwardPath) {
										backwardPath = forwardInfo[wId].dist + backwardInfo[wId].dist;
										forwardMid = {wVertex.originalVertex, wId};
										backwardMid = {wVertex.originalVertex, wId};
									}
								}
							}
						}
					});
				}
			}
		}
	}

	forwardGraphPQ.clear();
	forwardOverlayGraphPQ.clear();
	backwardGraphPQ.clear();
	backwardOverlayGraphPQ.clear();

	shortestPath = std::min(forwardPath, backwardPath);
	if (shortestPath > inf_weight) shortestPath = inf_weight;

	if (shortestPath == inf_weight) {
		return QueryResult({}, inf_weight);
	}

#ifdef QUERYTEST
	return QueryResult({}, shortestPath);
#endif

	// extract forward path
	std::vector<index> overlayPath;
	std::vector<VertexIdPair> idPath;
	index curId = forwardMid.id;
	while (forwardInfo[curId].parent.id != sForwardId) {
		overlayPath.push_back(forwardInfo[curId].parent.originalVertex);
		VertexIdPair pair = forwardInfo[curId].parent;
		if (pair.id < overlayOffset) {
			pair.id += (pair.id < graph.getMaxEdgesInCell()) ? forwardSOffset : forwardTOffset;
		} else {
			pair.id = pair.id - overlayOffset + graph.numberOfEdges();
		}
		idPath.push_back(pair);
		curId = forwardInfo[curId].parent.id;
	}

	idPath.push_back({s, sGlobalId});
	overlayPath.push_back(s);
	std::reverse(overlayPath.begin(), overlayPath.end());
	std::reverse(idPath.begin(), idPath.end());

	// extract backward path
	overlayPath.push_back(backwardMid.originalVertex);

	VertexIdPair mid = backwardMid;
	if (mid.id < overlayOffset) {
		mid.id += (mid.id < graph.getMaxEdgesInCell()) ? backwardSOffset : backwardTOffset;
	} else {
		mid.id = mid.id - overlayOffset + graph.numberOfEdges();
	}
	idPath.push_back(mid);

	curId = backwardMid.id;
	while (backwardInfo[curId].parent.id != tBackwardId) {
		overlayPath.push_back(backwardInfo[curId].parent.originalVertex);
		VertexIdPair pair = backwardInfo[curId].parent;
		if (pair.id < overlayOffset) {
			pair.id += (pair.id < graph.getMaxEdgesInCell()) ? backwardSOffset : backwardTOffset;
		} else {
			pair.id = pair.id - overlayOffset + graph.numberOfEdges();
		}
		idPath.push_back(pair);
		curId = backwardInfo[curId].parent.id;
	}

	idPath.push_back({t, tGlobalId});
	overlayPath.push_back(t);

#ifdef UNPACKPATHTEST
	pv start = get_micro_time();
#endif

	std::vector<index> unpackedPath = pathUnpacker.unpackPath(idPath, sCellNumber, tCellNumber, metricId);

#ifdef UNPACKPATHTEST
	index pathUnpackTime = get_micro_time() - start;
	shortestPath = pathUnpackTime;
#endif

	return QueryResult(unpackedPath, shortestPath);
}

} /* namespace CRP */
