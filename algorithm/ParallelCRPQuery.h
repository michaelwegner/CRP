/*
 * ParallelCRPQuery.h
 *
 *  Created on: Jan 8, 2016
 *      Author: Michael Wegner
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

#ifndef ALGORITHM_PARALLEL_CRPQUERY_H_
#define ALGORITHM_PARALLEL_CRPQUERY_H_

#include <memory>
#include <vector>

#include "../constants.h"
#include "../datastructures/Graph.h"
#include "../datastructures/id_queue.h"
#include "../datastructures/OverlayWeights.h"
#include "../metrics/Metric.h"
#include "Query.h"
#include "PathUnpacker.h"


namespace CRP {

class ParallelCRPQuery : public Query {
private:
	PathUnpacker& pathUnpacker;

	struct VertexInfo {
		weight dist;
		count round;
		VertexIdPair parent;
	};

	std::vector<VertexInfo> forwardInfo;
	std::vector<VertexInfo> backwardInfo;

	count currentRound;

	MinIDQueue<IDKeyTriple> forwardGraphPQ;
	MinIDQueue<IDKeyTriple> backwardGraphPQ;
	MinIDQueue<IDKeyTriple> forwardOverlayGraphPQ;
	MinIDQueue<IDKeyTriple> backwardOverlayGraphPQ;

public:
	ParallelCRPQuery(const Graph& graph, const OverlayGraph& overlayGraph, const std::vector<Metric>& metrics, PathUnpacker& pathUnpacker);
	virtual ~ParallelCRPQuery() = default;
	
	virtual QueryResult edgeQuery(index sourceEdgeId, index targetEdgeId, index metricId);
	virtual QueryResult vertexQuery(index sourceVertexId, index targetVertexId, index metricId);
};

} /* namespace CRP */

#endif /* ALGORITHM_PARALLEL_CRPQUERY_H_ */
