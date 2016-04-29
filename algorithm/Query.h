/*
 * Query.h
 *
 *  Created on: Apr 28, 2016
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

#ifndef ALGORITHM_QUERY_H_
#define ALGORITHM_QUERY_H_

#include "../datastructures/Graph.h"
#include "../datastructures/OverlayGraph.h"
#include "../datastructures/QueryResult.h"
#include "../metrics/Metric.h"

#include <vector>

namespace CRP {

/**
 * Abstract base class for query classes.
 */
class Query {
public:
	Query(const Graph& graph, const OverlayGraph& overlayGraph, const std::vector<Metric>& metrics) : graph(graph), overlayGraph(overlayGraph), metrics(metrics) {}
	virtual ~Query() = default;

	/**
	 * Compute shortest path from a source edge with @a sourceEdgeId to a target edge with @a targetEdgeId.
	 * Use the metric with @a metricId as cost function.
	 * @param sourceEdgeId
	 * @param targetEdgeId
	 * @param metricId
	 * @return QueryResult with the found path and its weight. If no path could be found, the path will be empty and
	 * the weight will be inf_weight.
	 */
	virtual QueryResult edgeQuery(index sourceEdgeId, index targetEdgeId, index metricId) = 0;

	/**
	 * Compute shortest path from a source vertex with @a sourceVertexId to a target vertex with @a targetVertexId.
	 * Use the metric with @a metricId as cost function.
	 * @param sourceEdgeId
	 * @param targetEdgeId
	 * @param metricId
	 * @return QueryResult with the found path and its weight. If no path could be found, the path will be empty and
	 * the weight will be inf_weight.
	 */
	virtual QueryResult vertexQuery(index sourceVertexId, index targetVertexId, index metricId) = 0;

protected:
	const Graph& graph;
	const OverlayGraph& overlayGraph;
	const std::vector<Metric>& metrics;
};

} /* namespace CRP */



#endif /* ALGORITHM_QUERY_H_ */
