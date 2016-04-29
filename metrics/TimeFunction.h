/*
 * TimeFunction.h
 *
 *  Created on: Jan 25, 2016
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

#ifndef METRICS_TIMEFUNCTION_H_
#define METRICS_TIMEFUNCTION_H_

#include "CostFunction.h"

namespace CRP {

/** Metric that computes the approximate time needed to travel from s to t. Adapt average speeds if necessary. */
class TimeFunction : public CostFunction {
public:
	virtual weight getWeight(const EdgeAttributes& attributes) const {
		Speed speed = attributes.getSpeed();
		if (speed == 0) {
			switch (attributes.getStreetType()) {
				case MOTORWAY:
					speed = 100;
					break;
				case TRUNK:
					speed = 85;
					break;
				case PRIMARY:
					speed = 70;
					break;
				case SECONDARY:
					speed = 60;
					break;
				case TERTIARY:
					speed = 50;
					break;
				case UNCLASSIFIED:
					speed = 40;
					break;
				case RESIDENTIAL:
					speed = 20;
					break;
				case SERVICE:
					speed = 5;
					break;
				case MOTORWAY_LINK:
					speed = 60;
					break;
				case TRUNK_LINK:
					speed = 60;
					break;
				case PRIMARY_LINK:
					speed = 55;
					break;
				case SECONDARY_LINK:
					speed = 50;
					break;
				case TERTIARY_LINK:
					speed = 40;
					break;
				case LIVING_STREET:
					speed = 5;
					break;
				case ROAD:
					speed = 50;
					break;
				default:
					speed = 30;
					break;
			}
		}
		assert(speed > 0);
		weight length = attributes.getLength();
		assert(length >= 0);
		weight w = static_cast<weight>(3.6f * length / speed);

		return (w > inf_weight) ? inf_weight : w;
	}

	virtual weight getTurnCosts(const Graph::TURN_TYPE turnType) const {
		weight cost;
		switch (turnType) {
		case Graph::U_TURN:
			cost = inf_weight;
			break;
		case Graph::NO_ENTRY:
			cost = inf_weight;
			break;
		default:
			cost = 0;
			break;
		}
		return cost;
	}
};

}




#endif /* METRICS_TIMEFUNCTION_H_ */
