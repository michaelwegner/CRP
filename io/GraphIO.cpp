/*
 * GraphIO.cpp
 *
 *  Created on: Dec 28, 2015
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

#include "GraphIO.h"

#include <boost/iostreams/device/file.hpp>
#include <boost/iostreams/filter/bzip2.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <algorithm>
#include <cassert>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <vector>

#include "../constants.h"
#include "../datastructures/LevelInfo.h"
#include "vector_io.h"
#include "OSMParser.h"

namespace CRP {

bool GraphIO::writeGraph(const Graph &graph, const std::string &outputFilePath) {
	std::fstream file(outputFilePath, std::ios::binary|std::ios::out);
	if (!file.is_open()) return false;
	boost::iostreams::filtering_stream<boost::iostreams::output> ostream;
	ostream.push(boost::iostreams::bzip2_compressor()) ;
	ostream.push(file);
	ostream << std::setprecision(16);
	ostream << graph.numberOfVertices() << " " << graph.numberOfEdges() << " " << graph.getNumberOfCellNumbers() << " " << graph.getNumberOfOverlayVertexMappings() << std::endl;

	graph.forVertices([&](index i, const Vertex &u) {
		ostream << u.pvPtr << " " << u.turnTablePtr << " " << u.firstOut << " " << u.firstIn << " " << u.coord.lat << " " << u.coord.lon << std::endl;
	});

	graph.forOutEdges([&](const ForwardEdge &e, std::size_t idx) {
		ostream << e.head << " " << (index) e.entryPoint << " " << e.attributes.stdAttributes << " " << e.attributes.maxHeight << std::endl;
	});

	graph.forInEdges([&](const BackwardEdge &e, std::size_t idx) {
		ostream << e.tail << " " << (index) e.exitPoint << " " << e.attributes.stdAttributes << " " << e.attributes.maxHeight << std::endl;
	});

	graph.forCellNumbers([&](const pv cellNumber) {
		ostream << cellNumber << std::endl;
	});

	const std::vector<Graph::TURN_TYPE>& turnTables = graph.getTurnTables();
	for (index i = 0; i < turnTables.size(); ++i) {
		ostream << (index) turnTables[i];
		if (i < turnTables.size()-1) ostream << " ";
	}
	ostream << std::endl;

	graph.forOverlayMappings([&](const SubVertex &v, const index id) {
		ostream << v.originalId << " " << (index) v.turnOrder << " " << (bool)v.exit << " " << id << std::endl;
	});

	ostream << graph.getMaxEdgesInCell() << std::endl;
	const std::vector<index>& forwardEdgeCellOffsets = graph.getForwardEdgeCellOffsets();
	for (index i = 0; i < forwardEdgeCellOffsets.size(); ++i) {
		ostream << forwardEdgeCellOffsets[i];
		if (i < forwardEdgeCellOffsets.size()-1) ostream << " ";
	}
	ostream << std::endl;

	const std::vector<index>& backwardEdgeCellOffsets = graph.getBackwardEdgeCellOffsets();
	for (index i = 0; i < backwardEdgeCellOffsets.size(); ++i) {
		ostream << backwardEdgeCellOffsets[i];
		if (i < backwardEdgeCellOffsets.size()-1) ostream << " ";
	}
	ostream << std::endl;

	ostream.flush();

	return true;
}

bool GraphIO::writeMetisGraph(const Graph &graph, const std::string &outputFilePath) {
	std::ofstream file;
	file.open(outputFilePath);
	if (!file.is_open()) return false;

	std::vector<std::vector<index>> neighbors(graph.numberOfVertices());
	std::vector<std::vector<bool>> isOneWay(graph.numberOfVertices());

	count m = 0;
	count oWay = 0;
	graph.forVertices([&](index u, const Vertex &vertex) {
		graph.forOutEdgesOf(u, 0, [&](const ForwardEdge &e, index exitPoint, Graph::TURN_TYPE type) {
			if (u < e.head) {
				neighbors[u].push_back(e.head);
				neighbors[e.head].push_back(u);
				m++;
				if (!graph.hasEdge(e.head, u)) {
					oWay++;
					isOneWay[u].push_back(true);
					isOneWay[e.head].push_back(true);
				} else {
					isOneWay[u].push_back(false);
					isOneWay[e.head].push_back(false);
				}
			} else if (u > e.head && !graph.hasEdge(e.head, u)) {
				neighbors[u].push_back(e.head);
				neighbors[e.head].push_back(u);
				m++;
				oWay++;
				isOneWay[u].push_back(true);
				isOneWay[e.head].push_back(true);
			}
		});
	});

	std::cout << m << ", " << graph.numberOfEdges() << ", " << oWay << std::endl;
	file << graph.numberOfVertices() << " " << m << " " << 1 << std::endl;
	for (index u = 0; u < graph.numberOfVertices(); ++u) {
		for (index j = 0; j < neighbors[u].size(); ++j) {
			if (isOneWay[u][j]) {
				file << neighbors[u][j]+1 << " " << 1 << " ";
			} else {
				file << neighbors[u][j]+1 << " " << 2 << " ";
			}
		}
		file << std::endl;
	}

	file.close();

	return true;
}

bool GraphIO::readGraph(Graph &graph, const std::string &inputFilePath) {
	std::ifstream file(inputFilePath, std::ios_base::in | std::ios_base::binary);
	if (!file.is_open()) return false;

	boost::iostreams::filtering_streambuf<boost::iostreams::input> inbuf;
	inbuf.push(boost::iostreams::bzip2_decompressor());
	inbuf.push(file);

	std::string line;
	std::istream instream(&inbuf);
	std::vector<std::string> tokens;

	if (file.eof()) return false;
	std::getline(instream, line);
	tokens = splitString(line, ' ');
	assert(tokens.size() == 4);
	count numVertices = stoui(tokens[0]);
	count numEdges = stoui(tokens[1]);
	count numCellNumbers = stoui(tokens[2]);
	count numOverlayMappings = stoui(tokens[3]);

	std::cout << "Reading graph with " << numVertices << " vertices and " << numEdges << " edges" << std::endl;

	std::vector<Vertex> vertices(numVertices+1);
	for (index i = 0; i < numVertices; ++i) {
		if (file.eof()) return false;
		std::getline(instream, line);
		tokens = splitString(line, ' ');
		assert(tokens.size() == 6);
		vertices[i] = {stoui(tokens[0]), stoui(tokens[1]), stoui(tokens[2]), stoui(tokens[3]), {std::stof(tokens[4]), std::stof(tokens[5])}};
	}
	vertices[numVertices] = {0, 0, numEdges, numEdges};

	std::vector<ForwardEdge> forwardEdges(numEdges);
	for (index i = 0; i < numEdges; ++i) {
		if (file.eof()) return false;
		std::getline(instream, line);
		tokens = splitString(line, ' ');
		assert(tokens.size() == 4);
		forwardEdges[i] = {stoui(tokens[0]), (turnorder)stoui(tokens[1]), stoui(tokens[2]), std::stof(tokens[3])};
	}

	std::vector<BackwardEdge> backwardEdges(numEdges);
	for (index i = 0; i < numEdges; ++i) {
		if (file.eof()) return false;
		std::getline(instream, line);
		tokens = splitString(line, ' ');
		assert(tokens.size() == 4);
		backwardEdges[i] = {stoui(tokens[0]), (turnorder)stoui(tokens[1]), stoui(tokens[2]), std::stof(tokens[3])};
	}

	std::vector<pv> cellNumbers(numCellNumbers);
	pv max = 0;
	for (index i = 0; i < numCellNumbers; ++i) {
		if (file.eof()) return false;
		std::getline(instream, line);
		tokens = splitString(line, ' ');
		assert(tokens.size() == 1);
		cellNumbers[i] = std::stoull(tokens[0]);
		if (max < cellNumbers[i]) max = cellNumbers[i];
	}

	std::cout << "largest cell number is " << max << std::endl;

	std::getline(instream, line);
	tokens = splitString(line, ' ');
	count numNoEntry = 0;
	std::vector<Graph::TURN_TYPE> turnTables(tokens.size());
	for (index i = 0; i < tokens.size(); ++i) {
		turnTables[i] = static_cast<Graph::TURN_TYPE>(stoui(tokens[i]));
		if (turnTables[i] == Graph::NO_ENTRY) numNoEntry++;
	}


	std::unordered_map<SubVertex, index, SubVertexHasher> overlayVertices;
	for (index i = 0; i < numOverlayMappings; ++i) {
		if (file.eof()) return false;
		std::getline(instream, line);
		tokens = splitString(line, ' ');
		assert(tokens.size() == 4);
		overlayVertices.insert({{stoui(tokens[0]), (turnorder) stoui(tokens[1]), (bool) stoui(tokens[2])}, stoui(tokens[3])});
	}

	index maxEdgesInCell = 0;

	std::vector<index> forwardEdgeCellOffset(cellNumbers.size());
	std::vector<index> backwardEdgeCellOffset(cellNumbers.size());
	if (cellNumbers.size() > 0) {
		std::getline(instream, line);
		tokens = splitString(line, ' ');
		assert(tokens.size() == 1);
		maxEdgesInCell = stoui(tokens[0]);

		if (!instream.eof()) {
			std::getline(instream, line);
			tokens = splitString(line, ' ');
			assert(tokens.size() == cellNumbers.size());
			for (index i = 0; i < tokens.size(); ++i) {
				forwardEdgeCellOffset[i] = stoui(tokens[i]);
			}
	
			std::getline(instream, line);
			tokens = splitString(line, ' ');
			assert(tokens.size() == cellNumbers.size());
			for (index i = 0; i < tokens.size(); ++i) {
				backwardEdgeCellOffset[i] = stoui(tokens[i]);
			}
		}
	}

	graph = Graph(vertices, forwardEdges, backwardEdges, turnTables, cellNumbers, overlayVertices, maxEdgesInCell, forwardEdgeCellOffset, backwardEdgeCellOffset);

	std::cout << "Read graph with " << graph.numberOfVertices() << " vertices and " << graph.numberOfEdges() << " edges" << std::endl;

	return true;
}

bool GraphIO::readDimacsGraph(Graph &graph, const std::string &dimacsFolder, bool dist) {
	std::vector<index> firstOut = load_vector<index>(dimacsFolder + "first_out");
	std::vector<index> head = load_vector<index>(dimacsFolder + "head");
	std::vector<float> lat = load_vector<float>(dimacsFolder + "latitude");
	std::vector<float> lon = load_vector<float>(dimacsFolder + "longitude");
	std::vector<weight> length = dist? load_vector<weight>(dimacsFolder + "travel_time") : load_vector<weight>(dimacsFolder + "geo_distance");
	assert(lat.size() == lon.size());
	assert(lat.size() == firstOut.size() - 1);

	std::vector<Vertex> vertices(firstOut.size());
	std::vector<std::vector<ForwardEdge>> fEdges(firstOut.size()-1);
	std::vector<std::vector<BackwardEdge>> bEdges(firstOut.size()-1);

	count maxDegree = 0;
	for (index u = 0; u < vertices.size()-1; ++u) {
		index first = firstOut[u];
		index last = firstOut[u+1];

		if (last - first > maxDegree) {
			maxDegree = last - first;
		}

		fEdges[u] = std::vector<ForwardEdge>(last - first);
		for (index j = first, k = 0; j < last; ++j, ++k) {
			index v = head[j];
			edgeAttr stdAttributes = 0;
			stdAttributes |= length[j] << 12;
			fEdges[u][k] = {v, (turnorder) bEdges[v].size(), {stdAttributes, 0}};
			bEdges[v].push_back({u, (turnorder) k, {stdAttributes, 0}});
			if (bEdges[v].size() > maxDegree) {
				maxDegree = bEdges[v].size();
			}
		}
	}

	std::vector<Graph::TURN_TYPE> turnTable(maxDegree * maxDegree, Graph::NONE);

	std::vector<ForwardEdge> forwardEdges = OSMParser::flatten(fEdges);
	std::vector<BackwardEdge> backwardEdges = OSMParser::flatten(bEdges);
	index forwardOffset = 0;
	index backwardOffset = 0;
	for (index u = 0; u < vertices.size()-1; ++u) {
		vertices[u].firstOut = forwardOffset;
		vertices[u].firstIn = backwardOffset;
		vertices[u].coord = {lat[u], lon[u]};
		vertices[u].turnTablePtr = 0;

		forwardOffset += fEdges[u].size();
		backwardOffset += bEdges[u].size();
	}
	vertices[vertices.size()-1].firstOut = forwardOffset;
	vertices[vertices.size()-1].firstIn = backwardOffset;

	graph = Graph(vertices, forwardEdges, backwardEdges, turnTable);

	return true;
}

bool GraphIO::writeOverlayGraph(const OverlayGraph &graph, const std::string &outputFilePath) {
	std::ofstream file;
	file.open(outputFilePath);
	if (!file.is_open()) return false;

	file << std::setprecision(16);

	// levelInfo
	const std::vector<uint8_t>& offsets = graph.getLevelInfo().getOffsets();
	for (size_t i = 0; i < offsets.size(); ++i) {
		if (i > 0) {
			file << " ";
		}
		file << (index) offsets[i];
	}
	file << std::endl;

	// vertexCountInLevel
	for (level l = 1; l <= graph.getLevelInfo().getLevelCount(); ++l) {
		if (l > 1) {
			file << " ";
		}
		file << graph.numberOfVerticesInLevel(l);
	}
	file << std::endl;

	// overlayVertices
	graph.forVertices([&](const OverlayVertex& v) {
		file << v.cellNumber << " " << v.neighborOverlayVertex << " " << v.originalVertex << " " << v.originalEdge;
		for (index e : v.entryExitPoint) {
			file << " " << e;
		}
		file << std::endl;
	});

	// weightVectorSize
	file << graph.getWeightVectorSize() << std::endl;

	// overlayIdMapping
	const std::vector<index>& overlayIdMapping = graph.getOverlayIdMapping();
	std::copy(overlayIdMapping.begin(), overlayIdMapping.end(), std::ostream_iterator<index>(file, " "));
	file << std::endl;

	// cellMapping
	for (level l = 1; l <= graph.getLevelInfo().getLevelCount(); ++l) {
		file << graph.numberOfCellsInLevel(l) << std::endl;
		graph.forCells(l, [&](const Cell& c, const pv truncatedCellNumber) {
			file << truncatedCellNumber << " " << c.numEntryPoints << " " << c.numExitPoints << " " << c.cellOffset << " "
					<< c.overlayIdOffset << std::endl;
		});
	}

	file.close();

	return true;
}

bool GraphIO::readOverlayGraph(OverlayGraph& graph, const std::string &inputFilePath) {
	std::ifstream file;
		file.open(inputFilePath);
		if (!file.is_open()) return false;

		std::string line;
		std::vector<std::string> tokens;

		if (file.eof()) return false;
		std::getline(file, line);
		tokens = splitString(line, ' ');
		std::vector<uint8_t> offsets;
		std::cout << "offsets ";
		for (const std::string& t : tokens) {
			offsets.push_back(stoui(t));
			std::cout << t << ", ";
		}
		std::cout << std::endl;
		const LevelInfo levelInfo(offsets);

		if (file.eof()) return false;
		std::getline(file, line);
		tokens = splitString(line, ' ');
		assert(tokens.size() == levelInfo.getLevelCount());
		std::vector<count> vertexCountInLevel;
		vertexCountInLevel.reserve(tokens.size());
		for (const auto& t : tokens) {
			vertexCountInLevel.push_back(stoui(t));
		}

		const count vertexCount = vertexCountInLevel[0];
		std::vector<OverlayVertex> vertices;
		vertices.reserve(vertexCount);
		for (index i = 0; i < vertexCount; ++i) {
			if (file.eof()) return false;
			std::getline(file, line);
			tokens = splitString(line, ' ');
			assert(tokens.size() >= 5);
			OverlayVertex v;
			v.cellNumber = std::stoull(tokens[0]);
			v.neighborOverlayVertex = stoui(tokens[1]);
			v.originalVertex = stoui(tokens[2]);
			v.originalEdge = stoui(tokens[3]);
			v.entryExitPoint.reserve(tokens.size() - 4);
			for (index i = 4; i < tokens.size(); ++i) {
				v.entryExitPoint.push_back(stoui(tokens[i]));
			}
			vertices.push_back(v);
		}

		if (file.eof()) return false;
		std::getline(file, line);
		const count weightVectorSize = stoui(line);

		if (file.eof()) return false;
		std::getline(file, line);
		tokens = splitString(line, ' ');
		std::vector<index> overlayIdMapping;
		overlayIdMapping.reserve(tokens.size());
		index counter = 0;
		for (const auto& t : tokens) {
			if (counter == tokens.size() -1) break;
			counter++;
			overlayIdMapping.push_back(stoui(t));
		}

		std::vector<std::unordered_map<pv, Cell>> cellMapping(levelInfo.getLevelCount());
		for (index i = 0; i < levelInfo.getLevelCount(); ++i) {
			if (file.eof()) return false;
			std::getline(file, line);
			const count cellsInLevel = stoui(line);
			for (index j = 0; j < cellsInLevel; ++j) {
				if (file.eof()) return false;
				std::getline(file, line);
				tokens = splitString(line, ' ');
				assert(tokens.size() == 5);
				cellMapping[i][std::stoull(tokens[0])] = {stoui(tokens[1]), stoui(tokens[2]), stoui(tokens[3]), stoui(tokens[4])};
			}
		}

		graph = OverlayGraph(vertices, vertexCountInLevel, cellMapping, overlayIdMapping, levelInfo, weightVectorSize);

		file.close();
		return true;
}

bool GraphIO::writeWeights(const OverlayWeights &weights, const std::string &outputFilePath) {
	save_vector(outputFilePath, weights.getWeights());
	return true;
}

bool GraphIO::readWeights(OverlayWeights &weights, const std::string &inputFilePath) {
	weights = OverlayWeights(load_vector<weight>(inputFilePath));
	return true;
}

std::vector<std::string> GraphIO::splitString(const std::string &str, char splitToken) {
	std::vector<std::string> tokens;

	std::stringstream sstream;
	for (index i = 0; i < str.size(); ++i) {
		if (str[i] == splitToken) {
			tokens.push_back(sstream.str());
			sstream.str(std::string());
		} else {
			sstream << str[i];
		}
	}
	tokens.push_back(sstream.str());

	return tokens;
}

index GraphIO::stoui(const std::string &str) {
	unsigned long val = std::stoul(str);
	index result = 	val;
	if (result != val) throw std::out_of_range("Cannot convert unsigned long to uint32_t without loss of information.");
	return result;
}

} /* namespace CRP */
