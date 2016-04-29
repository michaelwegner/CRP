/*
 * Graph.h
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

#ifndef GRAPH_H_
#define GRAPH_H_

#include <unordered_map>
#include <stddef.h>
#include <cassert>
#include <vector>

#include "../constants.h"
#include "LevelInfo.h"

namespace CRP {

enum STREET_TYPE {MOTORWAY, TRUNK, PRIMARY, SECONDARY, TERTIARY, UNCLASSIFIED, RESIDENTIAL, SERVICE, MOTORWAY_LINK, TRUNK_LINK, PRIMARY_LINK, SECONDARY_LINK, TERTIARY_LINK, LIVING_STREET, ROAD, INVALID};

struct Coordinate {
	float lat;
	float lon;
};

struct Vertex {
	index pvPtr;
	index turnTablePtr;
	index firstOut;
	index firstIn;

	Coordinate coord;
};

struct EdgeAttributes {
	edgeAttr stdAttributes;
	float maxHeight;

	weight getLength() const {
		return stdAttributes >> 12;
	}

	STREET_TYPE getStreetType() const {
		return static_cast<STREET_TYPE>(stdAttributes & ~(~0 << 4));
	}

	Speed getSpeed() const {
		return (Speed) ((stdAttributes >> 4) & ~(~0 << 8));
	}
};


/**
 * Model an outgoing edge of a vertex. The edge enters vertex @a head at @a entryPoint.
 */
struct ForwardEdge {
	index head;
	turnorder entryPoint;
	EdgeAttributes attributes;
};

/**
 * Model an incoming edge of a vertex. The edge exits vertex @a tail at @a exitPoint.
 */
struct BackwardEdge {
	index tail;
	turnorder exitPoint;
	EdgeAttributes attributes;
};

/**
 * Models an entry or exit point in the graph.
 */
struct SubVertex {
	index originalId;
	turnorder turnOrder;
	bool exit;

	bool operator==(const SubVertex &v2) const {
		return originalId == v2.originalId && turnOrder == v2.turnOrder && exit == v2.exit;
	}
};

struct SubVertexHasher {
	std::size_t operator()(const SubVertex &v) const {
		return (v.originalId << 8) | (v.turnOrder << 1) | v.exit;
	}
};

struct VertexIdPair {
	index originalVertex;
	index id;
};

class Graph {
public:
	enum TURN_TYPE {LEFT_TURN, RIGHT_TURN, STRAIGHT_ON, U_TURN, NO_ENTRY, NONE};

	Graph() = default;

	Graph(const std::vector<Vertex> &vertices, const std::vector<ForwardEdge> &forwardEdges, const std::vector<BackwardEdge> &backwardEdges);
	
	Graph(const std::vector<Vertex> &vertices, const std::vector<ForwardEdge> &forwardEdges, const std::vector<BackwardEdge> &backwardEdges, const std::vector<TURN_TYPE> &turnMatrices) :
		vertices(vertices), forwardEdges(forwardEdges), backwardEdges(backwardEdges), turnTables(turnMatrices), maxEdgesInCell(0) {}

	Graph(const std::vector<Vertex> &vertices, const std::vector<ForwardEdge> &forwardEdges, const std::vector<BackwardEdge> &backwardEdges, const std::vector<pv> &cellNumbers,
																											const std::unordered_map<SubVertex, index, SubVertexHasher> &overlayVertices);
	
	Graph(const std::vector<Vertex> &vertices, const std::vector<ForwardEdge> &forwardEdges, const std::vector<BackwardEdge> &backwardEdges, const std::vector<TURN_TYPE> &turnMatrices,
																		const std::vector<pv> &cellNumbers,	const std::unordered_map<SubVertex, index, SubVertexHasher> &overlayVertices,
																		index maxEdgesInCell, std::vector<index> &forwardEdgeCellOffsets, std::vector<index> &backwardEdgeCellOffsets) :
		vertices(vertices), forwardEdges(forwardEdges), backwardEdges(backwardEdges), turnTables(turnMatrices), cellNumbers(cellNumbers), maxEdgesInCell(maxEdgesInCell),
		forwardEdgeCellOffset(forwardEdgeCellOffsets), backwardEdgeCellOffset(backwardEdgeCellOffsets), overlayVertices(overlayVertices) {}

	Graph(const Graph &other) = default;

	Graph(Graph &&other) = default;

	~Graph() = default;

	Graph& operator=(const Graph &other) = default;

	Graph& operator=(Graph &&other) = default;

	inline count numberOfVertices() const {
		return vertices.size()-1;
	};

	inline count numberOfEdges() const {
		return forwardEdges.size();
	}

	inline count getOutDegree(index u) const {
		assert(u < vertices.size());
		return vertices[u+1].firstOut - vertices[u].firstOut;
	}

	inline count getInDegree(index u) const {
		assert(u < vertices.size());
		return vertices[u+1].firstIn - vertices[u].firstIn;
	}

	inline index getExitOffset(index u) const {
		assert(u < vertices.size());
		return vertices[u].firstOut;
	}

	inline index getEntryOffset(index u) const {
		assert(u < vertices.size());
		return vertices[u].firstIn;
	}

	inline index getHeadOfBackwardEdge(index e) const {
		assert(e < numberOfEdges());
		const BackwardEdge& backwardEdge = getBackwardEdge(e);
		const Vertex& tail = vertices[backwardEdge.tail];
		const ForwardEdge& forwardEdge = getForwardEdge(tail.firstOut + backwardEdge.exitPoint);
		return forwardEdge.head;
	}

	inline index getTailOfForwardEdge(index e) const {
		assert(e < numberOfEdges());
		const ForwardEdge& forwardEdge = getForwardEdge(e);
		const Vertex& head = vertices[forwardEdge.head];
		const BackwardEdge& backwardEdge = getBackwardEdge(head.firstIn + forwardEdge.entryPoint);
		return backwardEdge.tail;
	}


	/**
	 * Returns the index of the exit point of a forward edge (u, v) at vertex @a u.
	 * @param u the tail of the edge
	 * @param forwardEdge the forward edge
	 * @return the exit point at @a u
	 */
	inline turnorder getExitOrder(index u, index forwardEdge) const {
		assert(u < vertices.size());
		assert(vertices[u].firstOut <= forwardEdge && forwardEdge < vertices[u+1].firstOut);
		return forwardEdge - vertices[u].firstOut;
	}

	/**
	 * Returns the index of the entry point of a backward edge (u, v) at vertex @a v.
	 * @param v the head of the edge
	 * @param backwardEdge the backward edge
	 * @return the exit point at @a v
	 */
	inline turnorder getEntryOrder(index v, index backwardEdge) const {
		assert(v < vertices.size());
		assert(vertices[v].firstIn <= backwardEdge && backwardEdge < vertices[v+1].firstIn);
		return backwardEdge - vertices[v].firstIn;
	}

	inline TURN_TYPE getTurnType(index u, turnorder entryPoint, turnorder exitPoint) const {
		assert(u < numberOfVertices());
		assert(entryPoint < getInDegree(u));
		assert(exitPoint < getOutDegree(u));
		index turnTableOffset = vertices[u].turnTablePtr + entryPoint * getOutDegree(u) + exitPoint;
		return turnTables[turnTableOffset];
	}

	void setCellNumbers(std::vector<pv> &cellNumbers) {
		this->cellNumbers = cellNumbers;
	}

	void setOverlayMapping(std::unordered_map<SubVertex, index, SubVertexHasher> &overlayVertices) {
		this->overlayVertices = overlayVertices;
	}

	inline index getOverlayVertex(index u, turnorder turnOrder, bool exit) const {
		assert(u < vertices.size());
		assert(overlayVertices.find({u, turnOrder, exit}) != overlayVertices.end());
		const auto it = overlayVertices.find({u, turnOrder, exit}); 		
		return it->second;
	}

	inline std::vector<TURN_TYPE> getTurnTables() const {
		return turnTables;
	}

	inline pv getCellNumber(index u) const {
		assert(u < vertices.size());
		assert(0 <= vertices[u].pvPtr && vertices[u].pvPtr < cellNumbers.size());
		return cellNumbers[vertices[u].pvPtr];
	}

	inline const ForwardEdge& getForwardEdge(index e) const {
		assert(e < forwardEdges.size());
		return forwardEdges[e];
	}

	inline const BackwardEdge& getBackwardEdge(index e) const {
		assert(e < backwardEdges.size());
		return backwardEdges[e];
	}

	void sortVerticesByCellNumber();

	/**
	 * Finds the index of the backward edge that corresponds to the edge (u, v).
	 * Note that this needs to traverse all incoming edges of @a v and might be slow.
	 * @param u
	 * @param v
	 * @return
	 */
	index findBackwardEdge(index u, index v) const;

	bool hasEdge(index u, index v) const;

	inline count getNumberOfCellNumbers() const {
		return cellNumbers.size();
	}

	inline count getNumberOfOverlayVertexMappings() const {
		return overlayVertices.size();
	}

	inline Coordinate getCoordinate(index v) const {
		assert(v < vertices.size());
		return vertices[v].coord;
	}

	inline index getMaxEdgesInCell() const {
		return maxEdgesInCell;
	}

	inline index getForwardEdgeCellOffset(index v) const {
		assert(v < numberOfVertices());
		return forwardEdgeCellOffset[vertices[v].pvPtr];
	}

	inline index getBackwardEdgeCellOffset(index v) const {
		assert(v < numberOfVertices());
		return backwardEdgeCellOffset[vertices[v].pvPtr];
	}

	inline std::vector<index> getForwardEdgeCellOffsets() const {
		return forwardEdgeCellOffset;
	}

	inline std::vector<index> getBackwardEdgeCellOffsets() const {
		return backwardEdgeCellOffset;
	}

	inline TURN_TYPE getTurnType(index v, index entryPoint, index exitPoint) const {
		assert(v < numberOfVertices());
		assert(entryPoint < getInDegree(v) && exitPoint < getOutDegree(v));
		return turnTables[vertices[v].turnTablePtr + entryPoint * getOutDegree(v) + exitPoint];
	}

	/**
	 * Iterates over all cell numbers.
	 * @param handle must handle a pv.
	 */
	template <typename L> void forCellNumbers(L handle) const;

	/**
	 * Iterates over the overlay vertex hashmap.
	 * @param handle must handle a SubVertex and an index
	 */
	template <typename L> void forOverlayMappings(L handle) const;

	/**
	 * Iterates over all vertices in the graph and calls @a handle on each vertex.
	 * @param handle must handle (index, Vertex)
	 */
	template <typename L> void forVertices(L handle) const;

	/**
	 * Iterates over all vertices in the graph and calls @a handle on each vertex.
	 * @param handle must handle (index, Vertex)
	 */
	template <typename L> void forVertices(L handle);

	/**
	 * Iterates over all outgoing edges.
	 * @param handle must handle a ForwardEdge and its index.
	 */
	template <typename L> void forOutEdges(L handle) const;

	/**
	 * Iterates over all incoming edges.
	 * @param handle must handle a BackwardEdge and its index.
	 */
	template <typename L> void forInEdges(L handle) const;

	/**
	 * Iterates over all forward edges in the graph and calls @a handle on each edge.
	 * @param handle must handle (index tailVertex, index headVertex, index forwardEdge)
	 */
	template <typename L> void forEdges(L handle) const;

	/**
	 * Iterates over all outgoing edges of @a u.
	 * @param u
	 * @param entryPoint
	 * @param handle must handle a ForwardEdge, an exitPoint and a TURN_TYPE
	 */
	template <typename L> void forOutEdgesOf(index u, index entryPoint, L handle) const;

	/**
	 * Iterates over all incoming edges of @a v.
	 * @param v
	 * @param exitPoint
	 * @param handle must handle a BackwardEdge, an entryPoint and a TURN_TYPE
	 */
	template <typename L> void forInEdgesOf(index v, index exitPoint, L handle) const;

private:
	std::vector<Vertex> vertices;

	// edges
	std::vector<ForwardEdge> forwardEdges;
	std::vector<BackwardEdge> backwardEdges;

	// turn tables
	std::vector<TURN_TYPE> turnTables;

	// PV
	std::vector<pv> cellNumbers;

	index maxEdgesInCell;

	std::vector<index> forwardEdgeCellOffset;

	std::vector<index> backwardEdgeCellOffset;

	// Graph vertices -> OverlayGraph vertices
	std::unordered_map<SubVertex, index, SubVertexHasher> overlayVertices;
};

template <typename L>
void Graph::forEdges(L handle) const {
	forVertices([&](index u, const Vertex& tail) {
		for (index e = tail.firstOut; e < vertices[u+1].firstOut; ++e) {
			handle(u, forwardEdges[e].head, e);
		}
	});
}

template <typename L>
void Graph::forCellNumbers(L handle) const {
	for (index i = 0; i < cellNumbers.size(); ++i) {
		handle(cellNumbers[i]);
	}
}

template <typename L>
void Graph::forOverlayMappings(L handle) const {
	for (const auto &element : overlayVertices) {
		handle(element.first, element.second);
	}

}

template <typename L>
void Graph::forVertices(L handle) const {
	for (index i = 0; i < vertices.size() - 1; ++i) {
		handle(i, vertices[i]);
	}
}

template <typename L>
void Graph::forVertices(L handle) {
	for (index i = 0; i < vertices.size() - 1; ++i) {
		handle(i, vertices[i]);
	}
}

template <typename L>
void Graph::forOutEdges(L handle) const {
	for (std::size_t i = 0; i < forwardEdges.size(); ++i) {
		handle(forwardEdges[i], i);
	}
}

template <typename L>
void Graph::forInEdges(L handle) const {
	for (std::size_t i = 0; i < backwardEdges.size(); ++i) {
		handle(backwardEdges[i], i);
	}
}

template <typename L>
void Graph::forOutEdgesOf(index u, index entryPoint, L handle) const {
	assert(u < vertices.size());
	index turnTableOffset = vertices[u].turnTablePtr + entryPoint * getOutDegree(u);
	for (index e = vertices[u].firstOut, exitPoint = 0; e < vertices[u+1].firstOut; ++e, ++exitPoint, ++turnTableOffset) {
		handle(forwardEdges[e], exitPoint, turnTables[turnTableOffset]);
	}
}

template <typename L>
void Graph::forInEdgesOf(index v, index exitPoint, L handle) const {
	assert(v < vertices.size());
	index turnTableOffset = vertices[v].turnTablePtr + exitPoint;
	count outDeg = getOutDegree(v);
	for (index e = vertices[v].firstIn, entryPoint = 0; e < vertices[v+1].firstIn; ++e, ++entryPoint, turnTableOffset += outDeg) {
		handle(backwardEdges[e], entryPoint, turnTables[turnTableOffset]);
	}
}


} /* namespace CRP */

#endif /* GRAPH_H_ */
