/*
 * LevelInfo.h
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

#ifndef LEVELINFO_H_
#define LEVELINFO_H_

#include <vector>
#include <cstdint>
#include <cassert>
#include <cmath>

#include "../constants.h"
#include <iostream>

namespace CRP {

class LevelInfo {
private:
	std::vector<level> offset;
	
public:
	LevelInfo() = default;
	LevelInfo(const std::vector<uint8_t> &offset) : offset(offset) {}

	inline level getQueryLevel(pv sCellNumber, pv tCellNumber, pv vCellNumber) const {
		level l_sv = getHighestDifferingLevel(sCellNumber, vCellNumber);
		level l_tv = getHighestDifferingLevel(vCellNumber, tCellNumber);

		return std::min(l_sv, l_tv);
	}

	index getCellNumberOnLevel(level l, pv cellNumber) const {
		assert(0 < l && l < offset.size());
		return (cellNumber & ~(~0 << offset[l])) >> offset[l-1];
	}

	/**
	 * Returns the highest level on which vertex @a u and @a v are located on different
	 * cells. If no such level exists, this method returns 0.
	 * @param u
	 * @param v
	 * @return The highest level on which @a u and @a v differ or 0.
	 */
	level getHighestDifferingLevel(const pv c1, const pv c2) const {
		pv diff = c1 ^ c2;
		if (diff == 0) return 0;

		for (level l = offset.size()-1; l > 0; --l) {
			if (diff >> offset[l-1] > 0) return l;
		}
		return 0;
	}

	pv truncateToLevel(pv cellNumber, level l) const {
		assert(0 < l && l <= getLevelCount());
		return cellNumber >> offset[l-1];
	}

	count getLevelCount() const {
		return offset.size() - 1;
	}

	const std::vector<uint8_t>& getOffsets() const {
		return offset;
	}

};

} /* namespace CRP */

#endif /* LEVELINFO_H_ */
