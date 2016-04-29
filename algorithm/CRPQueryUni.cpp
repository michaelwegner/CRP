/*
 * CRPQueryUni.cpp
 *
 *  Created on: Jan 8, 2016
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

#include <algorithm>
#include <cassert>

#include "CRPQueryUni.h"

namespace CRP {

CRPQueryUni::CRPQueryUni(const Graph& graph, const OverlayGraph& overlayGraph, const std::vector<Metric>& metrics, PathUnpacker& pathUnpacker) : Query(graph, overlayGraph, metrics), pathUnpacker(pathUnpacker) {
	dist = std::vector<weight>(graph.numberOfEdges() + overlayGraph.numberOfVertices(), inf_weight);
	round = std::vector<count>(graph.numberOfEdges() + overlayGraph.numberOfVertices(), 0);
	parent = std::vector<VertexIdPair>(graph.numberOfEdges() + overlayGraph.numberOfVertices());
	currentRound = 0;
	graphPQ = MinIDQueue<IDKeyTriple>(graph.numberOfEdges());
	overlayGraphPQ = MinIDQueue<IDKeyPair>(overlayGraph.numberOfVertices());
}

QueryResult CRPQueryUni::vertexQuery(index sourceVertexId, index targetVertexId, index metricId) {
	const BackwardEdge& backwardEdgeToStart = graph.getBackwardEdge(graph.getEntryOffset(sourceVertexId));
	index sourceEdgeId = graph.getExitOffset(backwardEdgeToStart.tail) + backwardEdgeToStart.exitPoint;

	const ForwardEdge &forwardEdgeFromTarget = graph.getForwardEdge(graph.getExitOffset(targetVertexId));
	index targetEdgeId = graph.getEntryOffset(forwardEdgeFromTarget.head) + forwardEdgeFromTarget.entryPoint;


	return edgeQuery(sourceEdgeId, targetEdgeId, metricId);
}

QueryResult CRPQueryUni::edgeQuery(index sourceEdgeId, index targetEdgeId, index metricId) {
	currentRound++;
	graphPQ.clear();
	overlayGraphPQ.clear();

	const index s = graph.getForwardEdge(sourceEdgeId).head;
	index sId = graph.getEntryOffset(s) + graph.getForwardEdge(sourceEdgeId).entryPoint;
	pv sCellNumber = graph.getCellNumber(s);
	index t = graph.getBackwardEdge(targetEdgeId).tail;
	pv tCellNumber = graph.getCellNumber(t);
	index tId = std::numeric_limits<index>::max();

	weight shortestPath = 2 * inf_weight;
	dist[sId] = 0;
	round[sId] = currentRound;
	parent[sId] = {s, sId};
	graphPQ.push({sId, s, 0});

	while (!graphPQ.empty() || !overlayGraphPQ.empty()) {
		if (overlayGraphPQ.empty() || (!graphPQ.empty() && graphPQ.peek().key < overlayGraphPQ.peek().key)) { // graph vertex
			IDKeyTriple triple = graphPQ.pop();
			index u = triple.vertexId;
			index uId = triple.id;
			turnorder entryPoint = graph.getEntryOrder(u, uId);

			assert(round[uId] == currentRound);
			if (round[uId] == currentRound && dist[uId] > shortestPath) break;

			if (u == t) {
				if (dist[uId] < shortestPath) {
					shortestPath = dist[uId];
					tId = uId;
				}
			}
			graph.forOutEdgesOf(u, entryPoint, [&](const ForwardEdge &e, index exitPoint, Graph::TURN_TYPE type) {
				index v = e.head;
				level vQueryLevel = overlayGraph.getQueryLevel(sCellNumber, tCellNumber, graph.getCellNumber(v));
				weight edgeWeight = metrics[metricId].getWeight(e.attributes);
				weight turnCosts = metrics[metricId].getTurnCosts(type);
				if (u == s || u == t) turnCosts = 0; // hacky fix to problem when took a random backward edge to get our s
				weight newDist = dist[uId] + turnCosts + edgeWeight;

				if (newDist >= inf_weight) return;

				if (vQueryLevel == 0) { // graph
					index vId = graph.getEntryOffset(v) + e.entryPoint;
					if (round[vId] < currentRound || newDist < dist[vId]) {
						dist[vId] = newDist;
						graphPQ.pushOrDecrease({vId, v, newDist});
						round[vId] = currentRound;
						parent[vId] = {u, uId};
					}
				} else { // v is in another cell on another level
					v = graph.getOverlayVertex(v, e.entryPoint, false);
					index vId = v + graph.numberOfEdges(); // add offset to differ between overlay and original graph vertices
					if (round[vId] < currentRound || newDist < dist[vId]) {
						dist[vId] = newDist;
						overlayGraphPQ.pushOrDecrease({v, newDist});
						round[vId] = currentRound;
						parent[vId] = {u, uId};
					}
				}
			});
		} else {
			index u = overlayGraphPQ.pop().id;
			index uId = u + graph.numberOfEdges();
			if (round[uId] == currentRound && dist[uId] > shortestPath) break;

			level uQueryLevel = overlayGraph.getQueryLevel(sCellNumber, tCellNumber, overlayGraph.getVertex(u).cellNumber);
			overlayGraph.forOutNeighborsOf(u, uQueryLevel, [&](index v, index wOffset) {
				weight newDist = dist[uId] + metrics[metricId].getCellWeight(wOffset);
				if (newDist >= inf_weight) return;
				index vId = v + graph.numberOfEdges();
				if (round[vId] < currentRound || newDist < dist[vId]) {
					dist[vId] = newDist;
					round[vId] = currentRound;
					parent[vId] = {overlayGraph.getVertex(u).originalVertex, uId};
					// traverse edge to next cell
					index w = overlayGraph.getVertex(v).neighborOverlayVertex;
					ForwardEdge fEdge = graph.getForwardEdge(overlayGraph.getVertex(v).originalEdge);
					newDist = dist[vId] + metrics[metricId].getWeight(fEdge.attributes);
					if (newDist >= inf_weight) return;
					level wQueryLevel = overlayGraph.getQueryLevel(sCellNumber, tCellNumber, overlayGraph.getVertex(w).cellNumber);

					if (wQueryLevel == 0) { // we are back on the graph
						assert(overlayGraph.getVertex(w).cellNumber == sCellNumber || overlayGraph.getVertex(w).cellNumber == tCellNumber);
						index originalW = overlayGraph.getVertex(w).originalVertex;
						index originalWId = graph.getEntryOffset(originalW) + fEdge.entryPoint;
						if (round[originalWId] < currentRound || newDist < dist[originalWId]) {
							dist[originalWId] = newDist;
							graphPQ.pushOrDecrease({originalWId, originalW, newDist});
							round[originalWId] = currentRound;
							parent[originalWId] = {overlayGraph.getVertex(v).originalVertex, vId};
						}
					} else {
						index wId = w + graph.numberOfEdges();
						if (round[wId] < currentRound || newDist < dist[wId]) {
							dist[wId] = newDist;
							overlayGraphPQ.pushOrDecrease({w, newDist});
							round[wId] = currentRound;
							parent[wId] = {overlayGraph.getVertex(v).originalVertex, vId};
						}
					}
				}
			});
		}
	}
	graphPQ.clear();
	overlayGraphPQ.clear();

	if (shortestPath == 2 * inf_weight) {
		return QueryResult({}, inf_weight);
	}

#ifdef QUERYTEST
	return QueryResult({}, shortestPath);
#endif

	std::vector<index> path;
	std::vector<VertexIdPair> idPath;
	index uId = tId;
	path.push_back(t);
	idPath.push_back({t, tId});
	while (uId != sId) {
		path.push_back(parent[uId].originalVertex);
		idPath.push_back(parent[uId]);
		uId = parent[uId].id;
	}
	
	std::reverse(path.begin(), path.end());
	std::reverse(idPath.begin(), idPath.end());


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
