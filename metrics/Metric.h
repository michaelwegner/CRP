/*
 * Metric.h
 *
 *  Created on: 07.01.2016
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

#ifndef DATASTRUCTURES_METRIC_H_
#define DATASTRUCTURES_METRIC_H_

#include "../constants.h"
#include "../datastructures/Graph.h"
#include "../datastructures/OverlayGraph.h"
#include "../datastructures/OverlayWeights.h"
#include "../io/GraphIO.h"

#include "CostFunction.h"

#include <vector>
#include <unordered_map>
#include <fstream>
#include <string>
#include <memory>
#include <iostream>
#include "../timer.h"

namespace CRP {

/** Hashes an int vector. Taken from http://stackoverflow.com/questions/20511347/a-good-hash-function-for-a-vector */
struct IntVectorHasher {
	std::size_t operator()(std::vector<int> const& vec) const {
		std::size_t seed = 0;
		for(auto& i : vec) {
			seed ^= i + 0x9e3779b9 + (seed << 6) + (seed >> 2);
		}
		return seed;
	}
};

class Metric {
public:
	Metric() = default;
	Metric(const Graph &graph, const OverlayGraph &overlayGraph, std::unique_ptr<CostFunction> costFunction) : costFunction(std::move(costFunction)) {
		std::cout << "Computing weights" << std::endl;
#ifndef NPROFILE
		pv start = get_micro_time();
#endif
		weights = OverlayWeights(graph, overlayGraph, *(this->costFunction));
#ifndef NPROFILE
		pv end = get_micro_time();
		std::cout << "Took " << (double) (end - start) / 1000.0 << " ms" << std::endl;
#endif
		std::cout << "Done" << std::endl;

		index matrixOffset = 0;
		std::unordered_map<std::vector<int>, index, IntVectorHasher> matrixMap;
		turnTableDiffs = std::vector<int>();
		turnTablePtr = std::vector<index>(graph.numberOfVertices());

		for (index v = 0; v < graph.numberOfVertices(); ++v) {
			count n = graph.getInDegree(v);
			count m = graph.getOutDegree(v);
			if (n == 0 || m == 0) continue;
			std::vector<int> entryTurnTableDifferences(n*n);
			for (index i = 0; i < n; ++i) {
				for (index j = 0; j < n; ++j) {
					int maxDiff = (int) this->costFunction->getTurnCosts(graph.getTurnType(v, i, 0)) - (int) this->costFunction->getTurnCosts(graph.getTurnType(v, j, 0));
					for (index k = 1; k < m; ++k) {
						maxDiff = std::max(maxDiff, (int) this->costFunction->getTurnCosts(graph.getTurnType(v, i, k)) - (int) this->costFunction->getTurnCosts(graph.getTurnType(v, j, k)));
					}
					
					
					entryTurnTableDifferences[i * n + j] = maxDiff;
				}
			}

			std::vector<int> exitTurnTableDifferences(m*m);
			for (index i = 0; i < m; ++i) {
				for (index j = 0; j < m; ++j) {
					int maxDiff = (int) this->costFunction->getTurnCosts(graph.getTurnType(v, 0, i)) - (int) this->costFunction->getTurnCosts(graph.getTurnType(v, 0, j));
					for (index k = 1; k < n; ++k) {
						maxDiff = std::max(maxDiff, (int) this->costFunction->getTurnCosts(graph.getTurnType(v, k, i)) - (int) this->costFunction->getTurnCosts(graph.getTurnType(v, k, j)));
					}

					exitTurnTableDifferences[i * m + j] = maxDiff;
				}
			}


			auto it = matrixMap.find(entryTurnTableDifferences);
			if (it != matrixMap.end()) {
				turnTablePtr[v] = it->second;
			} else {
				turnTablePtr[v] = matrixOffset;
				matrixMap.insert(std::make_pair(entryTurnTableDifferences, matrixOffset));
				for (index i = 0; i < entryTurnTableDifferences.size(); ++i) {
					turnTableDiffs.push_back(entryTurnTableDifferences[i]);
				}
				matrixOffset += entryTurnTableDifferences.size();
			}

			it = matrixMap.find(exitTurnTableDifferences);
			if (it != matrixMap.end()) {
				turnTablePtr[v] |= it->second << 16;
			} else {
				turnTablePtr[v] |= matrixOffset << 16;
				matrixMap.insert(std::make_pair(exitTurnTableDifferences, matrixOffset));
				for (index i = 0; i < exitTurnTableDifferences.size(); ++i) {
					turnTableDiffs.push_back(exitTurnTableDifferences[i]);
				}
				matrixOffset += exitTurnTableDifferences.size();
			}
		}

		std::cout << "Found " << matrixMap.size() << " turn Matrices" << std::endl;
	}
	Metric(Metric &&other) = default;
	Metric& operator=(Metric &&other) = default;
	virtual ~Metric() = default;

	/**
	 * Returns the weight of the edge with given @a attributes.
	 * @param attributes
	 */
	inline weight getWeight(const EdgeAttributes& attributes) const {
		return costFunction->getWeight(attributes);
	}

	/**
	 * Returns the turn costs for the given @a turnType.
	 * @param turnType
	 */
	inline weight getTurnCosts(const Graph::TURN_TYPE turnType) const {
		return costFunction->getTurnCosts(turnType);
	}

	/**
	 * Returns the cell weight with given @a offset (modeling an edge in the cell)
	 * @param offset
	 */
	inline weight getCellWeight(index offset) const {
		return weights.getWeight(offset);
	}

	/**
	 * Returns max_k \{ T_v[i,k] - T_v[j,k] \} where i and j are entry points and T_v[i,k] is the turnCost from entry point i to exit point k.
	 * This is used for stalling (see Section 4.2.1 in the CRP paper).
	 * @param v
	 * @param offset Offset is equal to i * numEntryPoints + j.
	 */
	inline int getMaxEntryTurnTableDiff(const index v, const index offset) const {
		return turnTableDiffs[(turnTablePtr[v] & ~(~0 << 16)) + offset];
	}

	/**
	 * Returns max_k \{ T_v[k,i] - T_v[k,j] \} where i and j are exit points and T_v[i,k] is the turnCost from entry point i to exit point k.
	 * This is used for stalling (see Section 4.2.1 in the CRP paper).
	 * @param v
	 * @param offset Offset is equal to i * numExitPoints + j.
	 */
	inline int getMaxExitTurnTableDiff(const index v, const index offset) const {
		return turnTableDiffs[(turnTablePtr[v] >> 16) + offset];
	}

	inline const std::vector<weight> getWeights() const {
		return weights.getWeights();
	}

	/**
	 * Writes @a metric to disk with @a stream.
	 * @param metric
	 */
	static bool write(std::ofstream &stream, const Metric &metric) {
		if (!stream.is_open()) return false;

		std::cout << "Writing weights" << std::endl;
		stream << metric.weights.getWeights().size() << " " << metric.turnTablePtr.size() << " " << metric.turnTableDiffs.size() << std::endl;
		std::vector<weight> weights = metric.weights.getWeights();
		for (index i = 0; i < weights.size(); ++i) {
			stream << weights[i];
			if (i < weights.size()-1) stream << " ";
		}
		stream << std::endl;

		std::cout << "Writing turnTablePtr if size " << metric.turnTablePtr.size() << std::endl;
		for (index i = 0; i < metric.turnTablePtr.size(); ++i) {
			stream << metric.turnTablePtr[i];
			if (i < metric.turnTablePtr.size()-1) stream << " ";
		}
		stream << std::endl;

		std::cout << "Writing turnTableDiffs of size " << metric.turnTableDiffs.size() << std::endl;
		for (index i = 0; i < metric.turnTableDiffs.size(); ++i) {
			stream << metric.turnTableDiffs[i];
			if (i < metric.turnTableDiffs.size()-1) stream << " ";
		}

		return true;
	}

	/**
	 * Reads @a metric from input stream @a stream and sets the cost function of this metric to @a costFunction.
	 * @param stream
	 * @param metric
	 * @param costFunction
	 */
	static bool read(std::ifstream &stream, Metric &metric, std::unique_ptr<CostFunction> costFunction) {
		if (!stream.is_open()) return false;

		std::string line;
		std::getline(stream, line);
		std::vector<std::string> tokens = GraphIO::splitString(line, ' ');
		assert(tokens.size() == 3);
		std::vector<weight> w(std::stoul(tokens[0]));
		std::vector<index> turnTablePtr = std::vector<index>(std::stoul(tokens[1]));
		std::vector<int> turnTableDiffs = std::vector<int>(std::stoul(tokens[2]));

		std::getline(stream, line);
		tokens = GraphIO::splitString(line, ' ');
		assert(tokens.size() == w.size());
		for (index i = 0; i < w.size(); ++i) {
			w[i] = std::stoul(tokens[i]);
		}

		std::getline(stream, line);
		tokens = GraphIO::splitString(line, ' ');
		assert(tokens.size() == turnTablePtr.size());
		for (index i = 0; i < turnTablePtr.size(); ++i) {
			turnTablePtr[i] = std::stoul(tokens[i]);
		}

		std::getline(stream, line);
		tokens = GraphIO::splitString(line, ' ');
		assert(tokens.size() == turnTableDiffs.size());
		for (index i = 0; i < turnTableDiffs.size(); ++i) {
			turnTableDiffs[i] = std::stoi(tokens[i]);
		}

		metric.weights = OverlayWeights(w);
		metric.turnTablePtr = turnTablePtr;
		metric.turnTableDiffs = turnTableDiffs;
		metric.costFunction = std::move(costFunction);

		return true;
	}

private:
	std::unique_ptr<CostFunction> costFunction;
	OverlayWeights weights;
	std::vector<index> turnTablePtr;
	std::vector<int> turnTableDiffs;
};

} /* namespace CRP */

#endif /* DATASTRUCTURES_METRIC_H_ */
