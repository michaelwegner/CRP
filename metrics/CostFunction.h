/*
 * CostFunction.h
 *
 *  Created on: Feb 3, 2016
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

#ifndef METRICS_COSTFUNCTION_H_
#define METRICS_COSTFUNCTION_H_

#include "../datastructures/Graph.h"

namespace CRP {

/** Abstract class that computes the weight of an edge based on its attributes and the turn costs. */
class CostFunction {
public:
	CostFunction() = default;
	virtual ~CostFunction() = default;

	virtual weight getWeight(const EdgeAttributes& attributes) const = 0;
	virtual weight getTurnCosts(const Graph::TURN_TYPE turnType) const = 0;
};

}

#endif /* METRICS_COSTFUNCTION_H_ */
