/*
 * MultiLevelPartition.cpp
 *
 *  Created on: Dec 15, 2015
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

#include "MultiLevelPartition.h"

#include <cassert>
#include <cmath>
#include <fstream>
#include <string>
#include <iostream>

namespace CRP {

void MultiLevelPartition::setNumberOfLevels(const size_t numLevels) {
	numCells = std::vector<count>(numLevels, 0);
}

void MultiLevelPartition::setNumberOfVertices(const int numVertices) {
	cellNumbers = std::vector<pv>(numVertices, 0);
}

void MultiLevelPartition::setNumberOfCellsInLevel(const int level, const int numCells) {
	assert(0 <= level && level < this->numCells.size());
	this->numCells[level] = numCells;
}

void MultiLevelPartition::computeBitmap() {
	pvOffset = std::vector<uint8_t>(numCells.size()+1);
	for (index i = 0; i < numCells.size(); ++i) {
		pvOffset[i+1] = pvOffset[i] + ceil(log2(numCells[i]));
	}
}

void MultiLevelPartition::setCell(const int level, const int vertexId, const int cellId) {
	assert(0 <= level && level < numCells.size());
	assert(0 <= vertexId && vertexId < cellNumbers.size());
	assert(0 <= cellId && cellId < numCells[level]);

	cellNumbers[vertexId] |= (((pv)cellId) << pvOffset[level]);
}

index MultiLevelPartition::getCell(const int level, const int vertexId) const {
	return (cellNumbers[vertexId] >> pvOffset[level]) & ~(~0 << pvOffset[level+1]);
}

count MultiLevelPartition::getNumberOfVertices() const {
	return cellNumbers.size();
}

void MultiLevelPartition::write(const std::string &outputFileName) const {
	std::ofstream file;
	file.open(outputFileName);
	if (file.is_open()) {
		file << numCells.size() << std::endl;
		for (index i = 0; i < numCells.size(); ++i) {
			file << numCells[i] << std::endl;
		}
		file << getNumberOfVertices() << std::endl;
		for (index i = 0; i < getNumberOfVertices(); ++i) {
			file << cellNumbers[i] << std::endl;
		}

		file.close();
	}
}

void MultiLevelPartition::read(const std::string &inputFileName) {
	std::ifstream file;
	file.open(inputFileName);
	if (file.is_open()) {
		std::string line;
		getline(file, line);

		count numLevels = std::stoi(line);
		numCells = std::vector<count>(numLevels);
		for (index i = 0; i < numLevels; ++i) {
			std::getline(file, line);
			numCells[i] = std::stoi(line);
		}

		computeBitmap();

		std::getline(file, line);
		count numVertices = std::stoi(line);

		cellNumbers = std::vector<pv>(numVertices);
		for (index i = 0; i < numVertices; ++i) {
			std::getline(file, line);
			cellNumbers[i] = std::stoull(line);
		}
	}
}

count MultiLevelPartition::getNumberOfLevels() const {
	return numCells.size();
}

count MultiLevelPartition::getNumberOfCellsInLevel(level l) const {
	assert(0 <= l && l < numCells.size());
	return numCells[l];
}

std::vector<uint8_t> MultiLevelPartition::getPVOffsets() const {
	return pvOffset;
}

pv MultiLevelPartition::getCellNumber(index u) const {
	assert(0 <= u && u < cellNumbers.size());
	return cellNumbers[u];
}



} /* namespace CRP */
