/*
 * OverlayWeights.h
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

#ifndef DATASTRUCTURES_OVERLAYWEIGHTS_H_
#define DATASTRUCTURES_OVERLAYWEIGHTS_H_

#include <cassert>
#include <vector>

#include "../constants.h"
#include "Graph.h"
#include "OverlayGraph.h"
#include "../metrics/CostFunction.h"


namespace CRP {

class OverlayWeights {
public:
	OverlayWeights() = default;
	OverlayWeights(const std::vector<weight>& weights) : weights(weights) {}
	OverlayWeights(const Graph& graph, const OverlayGraph& overlayGraph, const CostFunction& costFunction);

	inline weight getWeight(index i) const {
		assert(i < weights.size());
		return weights[i];
	}

	inline weight operator[](index i) const {
		assert(i < weights.size());
		return weights[i];
	}

	inline const std::vector<weight> getWeights() const {
		return weights;
	}

private:
	std::vector<weight> weights;

	void build(const Graph& graph, const OverlayGraph& overlayGraph, const CostFunction& costFunction);
	void buildLowestLevel(const Graph& graph, const OverlayGraph& overlayGraph, const CostFunction& costFunction);
	void buildLevel(const Graph& graph, const OverlayGraph& overlayGraph, const CostFunction& costFunction, level l);
};

}

#endif /* DATASTRUCTURES_OVERLAYWEIGHTS_H_ */
