/*
 * Dijkstra.cpp
 *
 *  Created on: Mar 30, 2016
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

#include "Dijkstra.h"

#include <algorithm>
#include <cassert>
#include <string>

namespace CRP {

Dijkstra::Dijkstra(const Graph &graph, const OverlayGraph &overlayGraph, const std::vector<Metric> &metrics) : Query(graph, overlayGraph, metrics) {
	dist = std::vector<weight>(graph.numberOfEdges(), inf_weight);
	round = std::vector<count>(graph.numberOfEdges(), 0);
	parent = std::vector<VertexIdPair>(graph.numberOfEdges());
	currentRound = 0;
	graphPQ = MinIDQueue<IDKeyTriple>(graph.numberOfEdges());
}

QueryResult Dijkstra::vertexQuery(index sourceVertexId, index targetVertexId, index metricId) {
	BackwardEdge backwardEdgeToStart = graph.getBackwardEdge(graph.getEntryOffset(sourceVertexId));
	index sourceEdgeId = graph.getExitOffset(backwardEdgeToStart.tail) + backwardEdgeToStart.exitPoint;

	ForwardEdge forwardEdgeFromTarget = graph.getForwardEdge(graph.getExitOffset(targetVertexId));
	index targetEdgeId = graph.getEntryOffset(forwardEdgeFromTarget.head) + forwardEdgeFromTarget.entryPoint;

	return edgeQuery(sourceEdgeId, targetEdgeId, metricId);
}

QueryResult Dijkstra::edgeQuery(index sourceEdgeId, index targetEdgeId, index metricId) {
	currentRound++;
	graphPQ.clear();

	const index s = graph.getForwardEdge(sourceEdgeId).head;
	index sId = graph.getEntryOffset(s) + graph.getForwardEdge(sourceEdgeId).entryPoint;
	index t = graph.getBackwardEdge(targetEdgeId).tail;
	index tId;

	weight shortestPath = 2 * inf_weight;
	dist[sId] = 0;
	round[sId] = currentRound;
	parent[sId] = {s, sId};
	graphPQ.push({sId, s, 0});

	while (!graphPQ.empty()) {
		IDKeyTriple triple = graphPQ.pop();
		index u = triple.vertexId;
		index uId = triple.id;
		turnorder entryPoint = graph.getEntryOrder(u, uId);

		if (round[uId] == currentRound && dist[uId] > shortestPath) break;

		if (u == t) {
			weight newShortestPath = dist[uId];
			if (newShortestPath < shortestPath) {
				shortestPath = newShortestPath;
				tId = uId;
			}
		}

		graph.forOutEdgesOf(u, entryPoint, [&](const ForwardEdge &e, index exitPoint, Graph::TURN_TYPE type) {
			index v = e.head;
			weight edgeWeight = metrics[metricId].getWeight(e.attributes);
			weight turnCosts = metrics[metricId].getTurnCosts(type);
			if (u == s || u == t) turnCosts = 0; // hacky fix to problem when took a random backward edge to get our s
			weight newDist = dist[uId] + turnCosts + edgeWeight;

			if (newDist >= inf_weight) return;

			index vId = graph.getEntryOffset(v) + e.entryPoint;
			if (round[vId] < currentRound || newDist < dist[vId]) {
				dist[vId] = newDist;
				graphPQ.pushOrDecrease({vId, v, newDist});
				round[vId] = currentRound;
				parent[vId] = {u, uId};
			}
		});
	}

	if (shortestPath == 2 * inf_weight) {
		return QueryResult({}, inf_weight);
	}

	std::vector<index> path;
	index uId = tId;
	path.push_back(t);
	while (uId != sId) {
		path.push_back(parent[uId].originalVertex);
		uId = parent[uId].id;
	}

	std::reverse(path.begin(), path.end());

	return QueryResult(path, shortestPath);
}

} /* namespace CRP */
