/*
 * MultiLevelPartition.h
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

#ifndef MULTILEVELPARTITION_H_
#define MULTILEVELPARTITION_H_

#include <stddef.h>
#include <iostream>
#include <string>
#include <vector>

#include "../constants.h"

namespace CRP {

/**
 * Stores for every vertex in which cell it is located on every level of the overlay graph.
 *
 * @note: First set the basic information as @ref SetNumberOfLevels, SetNumberOfItems, SetNumberOfCellsInLevel. Then call @ref ComputeBitmap()
 * */

class MultiLevelPartition {
public:
	void setNumberOfLevels(const size_t numLevels);
	void setNumberOfVertices(const int numVertices);
	void setNumberOfCellsInLevel(const int level, const int numCells);
	void computeBitmap();

	/**
	 * Sets the @a cellId for vertex with @a vertexId on @a level.
	 * @param level
	 * @param vertexId
	 * @param cellId
	 */
	void setCell(const int level, const int vertexId, const int cellId);
	index getCell(const int level, const int vertexId) const;

	count getNumberOfVertices() const;

	void write(const std::string &outputFileName) const;

	void read(const std::string &inputFileName);

	count getNumberOfLevels() const;

	count getNumberOfCellsInLevel(level l) const;

	std::vector<uint8_t> getPVOffsets() const;

	pv getCellNumber(index u) const;

private:
	std::vector<count> numCells;
	std::vector<uint8_t> pvOffset;
	std::vector<pv> cellNumbers;
};

} /* namespace CRP */

#endif /* MULTILEVELPARTITION_H_ */
