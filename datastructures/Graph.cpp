/*
 * Graph.cpp
 *
 *  Created on: Dec 14, 2015
 *      Author: Matthias Wolf & Michael Wegner
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

#include "Graph.h"

namespace CRP {

Graph::Graph(const std::vector<Vertex> &vertices, const std::vector<ForwardEdge> &forwardEdges, const std::vector<BackwardEdge> &backwardEdges) : vertices(vertices), forwardEdges(forwardEdges),
		backwardEdges(backwardEdges), maxEdgesInCell(0) {
}

Graph::Graph(const std::vector<Vertex> &vertices, const std::vector<ForwardEdge> &forwardEdges, const std::vector<BackwardEdge> &backwardEdges,
							const std::vector<pv> &cellNumbers,	const std::unordered_map<SubVertex, index, SubVertexHasher> &overlayVertices) : vertices(vertices),
							forwardEdges(forwardEdges), backwardEdges(backwardEdges), cellNumbers(cellNumbers), maxEdgesInCell(0), overlayVertices(overlayVertices) {
}

index Graph::findBackwardEdge(index u, index v) const {
	assert(u < vertices.size() && v < vertices.size());
	for (index e = vertices[v].firstIn; e < vertices[v+1].firstIn; ++e) {
		if (backwardEdges[e].tail == u) return e;
	}
	return static_cast<index>(-1);
}

bool Graph::hasEdge(index u, index v) const {
        assert(u < vertices.size() && v < vertices.size());
        for (index e = vertices[u].firstOut; e < vertices[u+1].firstOut; ++e) {
                if (forwardEdges[e].head == v) return true;
        }

        return false;
}

void Graph::sortVerticesByCellNumber() {
	std::vector<std::vector<std::pair<Vertex, index>>> cellVertices(cellNumbers.size());
	std::vector<count> numForwardEdgesInCell(cellNumbers.size(), 0);
	std::vector<count> numBackwardEdgesInCell(cellNumbers.size(), 0);
	std::vector<std::vector<ForwardEdge>> fEdges(vertices.size()-1);
	std::vector<std::vector<BackwardEdge>> bEdges(vertices.size()-1);

	maxEdgesInCell = 0;
	for (index i = 0; i < vertices.size()-1; ++i) {
		index cell = vertices[i].pvPtr;
		cellVertices[cell].push_back(std::make_pair(vertices[i], i));
		fEdges[i] = std::vector<ForwardEdge>(getOutDegree(i));
		bEdges[i] = std::vector<BackwardEdge>(getInDegree(i));
		for (index k = 0, e = vertices[i].firstOut; e < vertices[i+1].firstOut; ++e, ++k) {
			fEdges[i][k] = forwardEdges[e];
		}
		for (index k = 0, e = vertices[i].firstIn; e < vertices[i+1].firstIn; ++e, ++k) {
			bEdges[i][k] = backwardEdges[e];
		}

		numForwardEdgesInCell[cell] += getOutDegree(i);
		numBackwardEdgesInCell[cell] += getInDegree(i);

		if (maxEdgesInCell < numForwardEdgesInCell[cell]) {
			maxEdgesInCell = numForwardEdgesInCell[cell];
		}

		if (maxEdgesInCell < numBackwardEdgesInCell[cell]) {
			maxEdgesInCell = numBackwardEdgesInCell[cell];
		}
	}

	std::vector<index> newId(vertices.size()-1);
	index newVId = 0;
	for (index i = 0; i < cellVertices.size(); ++i) {
		for (index v = 0; v < cellVertices[i].size(); ++v, ++newVId) {
			newId[cellVertices[i][v].second] = newVId;
		}
	}
	assert(newVId == vertices.size()-1);

	index vId = 0;
	index forwardOffset = 0;
	forwardEdgeCellOffset = std::vector<index>(cellNumbers.size());
	index backwardOffset = 0;
	backwardEdgeCellOffset = std::vector<index>(cellNumbers.size());

	for (index i = 0; i < cellNumbers.size(); ++i) {
		forwardEdgeCellOffset[i] = forwardOffset;
		backwardEdgeCellOffset[i] = backwardOffset;
		for (index v = 0; v < cellVertices[i].size(); ++v, vId++) {
			vertices[vId] = cellVertices[i][v].first;
			index vOldId = cellVertices[i][v].second;
			vertices[vId].firstOut = forwardOffset;
			vertices[vId].firstIn = backwardOffset;
			for (index k = 0; k < fEdges[vOldId].size(); ++k) {
				forwardEdges[forwardOffset] = fEdges[vOldId][k];
				forwardEdges[forwardOffset].head = newId[fEdges[vOldId][k].head];
				forwardOffset++;
			}

			for (index k = 0; k < bEdges[vOldId].size(); ++k) {
				backwardEdges[backwardOffset] = bEdges[vOldId][k];
				backwardEdges[backwardOffset].tail = newId[bEdges[vOldId][k].tail];
				backwardOffset++;
			}
		}
	}

	assert(vId == vertices.size()-1);
	assert(forwardOffset == forwardEdges.size());
	assert(backwardOffset == backwardEdges.size());

	this->forEdges([&](index start, index target, index forwardEdge) {
		assert(start < vertices.size());
		assert(target < vertices.size());
	});
}

} /* namespace CRP */
