/*
 * OSMParser.h
 *
 *  Created on: Dec 24, 2015
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

#ifndef IO_OSMPARSER_H_
#define IO_OSMPARSER_H_

#include "SaxHandler.h"
#include "../datastructures/Graph.h"
#include "SaxParser.h"

#include <limits>
#include <unordered_map>
#include <cmath>
#include <algorithm>
#include <regex>
#include <iostream>

namespace CRP {

typedef uint64_t Id;

struct VectorHasher {
	std::size_t operator()(std::vector<Graph::TURN_TYPE> const& vec) const {
		std::size_t seed = 0;
		for(auto& i : vec) {
			seed ^= i + 0x9e3779b9 + (seed << 6) + (seed >> 2);
		}
		return seed;
	}
};

class OSMParser : public SaxHandler {
public:
	OSMParser();
	~OSMParser() = default;

	bool parseGraph(const std::string &graphFile, Graph &graph);

	void startElement(const std::string &uri, const std::string &localName, const std::string &qName, const std::vector<Attribute> &attributes);

	void endElement(const std::string &uri, const std::string &localName, const std::string &qName);

	template<typename T>
	static std::vector<T> flatten(const std::vector<std::vector<T>> &container);

private:
	enum TURN_RESTRICTION {NO_LEFT_TURN, NO_RIGHT_TURN, NO_STRAIGHT_ON, NO_U_TURN, ONLY_RIGHT_TURN, ONLY_LEFT_TURN, ONLY_STRAIGHT_ON, NO_ENTRY, INVALID, NONE};

	const Id maxId = std::numeric_limits<Id>::max();

	struct Node {
		float lat;
		float lon;
	};

	struct Way {
		std::vector<Id> nodes;
		Speed maxSpeed;
		STREET_TYPE type;
		float maxHeight;
		bool oneway;
	};

	struct Restriction {
		Id via;
		Id to;
		TURN_RESTRICTION turnRestriction;
	};



	std::unordered_map<Id, std::vector<Restriction>> restrictions;
	std::unordered_map<Id, Node> nodes;
	std::unordered_map<Id, Way> ways;
	Id currentWay;
	Id currentNode;
	bool validNode;
	Restriction currentRestriction;
	bool inRelation;


	void buildGraph(Graph &graph);

	/**
	 * Compute the distance between two nodes @a u and @a v using the haversine formula
	 * @param u
	 * @param v
	 * @return The distance between @a u and @a v.
	 */
	inline float getDistance(Node u, Node v) {
		const int R = 6371000; // earth mean radius in meters
		double phi1 = toRadian(u.lat);
		double phi2 = toRadian(v.lat);
		double dPhi = toRadian(v.lat - u.lat);
		double dLambda = toRadian(v.lon - u.lon);

		double a = sin(dPhi/2.0) * sin(dPhi/2.0) + cos(phi1)*cos(phi2)*sin(dLambda/2.0)*sin(dLambda/2.0);
		double c = 2 * atan2(sqrt(a), sqrt(1.0-a));

		return (float) R * c;
	}

	inline double toRadian(double degreeVal) {
		return degreeVal * M_PI / 180.0;
	}

	inline void extractNode(const std::vector<Attribute> &attributes) {
		Id id = 0;
		Node node = {0,0};
		uint8_t numParsed = 0;
		for (Attribute a : attributes) {
			if (a.qName == "id") {
				id = std::stoll(a.value);
				numParsed++;
			} else if (a.qName == "lat") {
				node.lat = std::stof(a.value);
				numParsed++;
			} else if (a.qName == "lon") {
				node.lon = std::stof(a.value);
				numParsed++;
			}
		}

		if (numParsed == 3) {
			nodes.insert(std::make_pair(id, node));
			currentNode = id;
			validNode = true;
		}
	}

	inline void parseNodeTag(const std::vector<Attribute> &attributes) {
		std::string tagKey = attributes[0].value;
		if (tagKey == "amenity" || tagKey == "power" || SaxParser::stringStartsWith(tagKey, "addr") || tagKey == "natural" || tagKey == "shop" || tagKey == "tourism") {
			validNode = false;
		}
	}

	inline void parseWayTag(const std::vector<Attribute> &attributes) {
		std::string tagKey = attributes[0].value;
		std::string tagVal = attributes[1].value;

		if (tagKey == "maxspeed") {
			//std::cout << tagVal << std::endl;
			//if (!std::regex_match(tagVal, std::regex("^[A-Za-z]+$"))) {
			if (isInteger(tagVal)) {
				ways[currentWay].maxSpeed = std::stoi(tagVal);
			}
		} else if (tagKey == "maxheight") {
			//std::cout << tagVal << std::endl;
			if (isFloat(tagVal)) {
				ways[currentWay].maxHeight = std::stof(tagVal);
			}
		} else if (tagKey == "junction") {
			if (tagVal == "roundabout" || tagVal == "mini_roundabout" || tagVal == "turning_loop") {
				if (ways[currentWay].nodes.front() != ways[currentWay].nodes.back()) {
					ways[currentWay].nodes.push_back(ways[currentWay].nodes.front());
					ways[currentWay].oneway = true;
				}
			}
		} else if (tagKey == "oneway") {
			if (tagVal == "yes" || tagVal == "1" || tagVal == "true") {
				ways[currentWay].oneway = true;
			} else if (tagVal == "-1" || tagVal == "reverse") {
				std::reverse(ways[currentWay].nodes.begin(), ways[currentWay].nodes.end());
				ways[currentWay].oneway = true;
			}
		} else if (tagKey == "highway") {
			if (tagVal == "motorway") {
				ways[currentWay].type = MOTORWAY;
			} else if (tagVal == "trunk") {
				ways[currentWay].type = TRUNK;
			} else if (tagVal == "primary") {
				ways[currentWay].type = PRIMARY;
			} else if (tagVal == "secondary") {
				ways[currentWay].type = SECONDARY;
			} else if (tagVal == "tertiary") {
				ways[currentWay].type = TERTIARY;
			} else if (tagVal == "unclassified") {
				ways[currentWay].type = UNCLASSIFIED;
			} else if (tagVal == "residential") {
				ways[currentWay].type = RESIDENTIAL;
			} else if (tagVal == "service") {
				ways[currentWay].type = SERVICE;
			} else if (tagVal == "motorway_link") {
				ways[currentWay].type = MOTORWAY_LINK;
			} else if (tagVal == "trunk_link") {
				ways[currentWay].type = TRUNK_LINK;
			} else if (tagVal == "primary_link") {
				ways[currentWay].type = PRIMARY_LINK;
			} else if (tagVal == "secondary_link") {
				ways[currentWay].type = SECONDARY_LINK;
			} else if (tagVal == "tertiary_link") {
				ways[currentWay].type = TERTIARY_LINK;
			} else if (tagVal == "living_street") {
				ways[currentWay].type = LIVING_STREET;
			} else if (tagVal == "road") {
				ways[currentWay].type = ROAD;
			}
		}
	}

	inline void parseRelationTag(const std::vector<Attribute> &attributes) {
		std::string tagKey = attributes[0].value;
		std::string tagVal = attributes[1].value;
		if (tagKey == "restriction") {
			if (tagVal == "no_left_turn") {
				currentRestriction.turnRestriction = NO_LEFT_TURN;
			} else if (tagVal == "no_right_turn") {
				currentRestriction.turnRestriction = NO_RIGHT_TURN;
			} else if (tagVal == "no_straight_on") {
				currentRestriction.turnRestriction = NO_STRAIGHT_ON;
			} else if (tagVal == "no_u_turn") {
				currentRestriction.turnRestriction = NO_U_TURN;
			} else if (tagVal == "only_right_turn") {
				currentRestriction.turnRestriction = ONLY_RIGHT_TURN;
			} else if (tagVal == "only_left_turn") {
				currentRestriction.turnRestriction = ONLY_LEFT_TURN;
			} else if (tagVal == "only_straight_on") {
				currentRestriction.turnRestriction = ONLY_STRAIGHT_ON;
			} else if (tagVal == "no_entry") {
				currentRestriction.turnRestriction = NO_ENTRY;
			}
		}
	}

	inline void parseMember(const std::vector<Attribute> &attributes) {
		Id refId = maxId;
		enum Type {FROM, VIA, TO, INVALID};
		Type type = INVALID;
		for (const Attribute &a : attributes) {
			if (a.qName == "ref") {
				refId = std::stoll(a.value);
			} else if (a.qName == "role") {
				if (a.value == "from") {
					type = FROM;
				} else if (a.value == "to") {
					type = TO;
				} else if (a.value == "via") {
					type = VIA;
				}
			}
		}

		if (refId != maxId && type != INVALID) {
			if (type == FROM) {
				currentWay = refId;
			} else if (type == TO) {
				currentRestriction.to = refId;
			} else {
				currentRestriction.via = refId;
			}
		}
	}

	inline bool isInteger(const std::string &str) {
		for (unsigned i = 0; i < str.size(); ++i) {
			if (!std::isdigit(str[i])) return false;
		}
		return true;
	}

	inline bool isFloat(const std::string &str) {
		for (unsigned i = 0; i < str.size(); ++i) {
			if (!std::isdigit(str[i]) && str[i] != '.' && str[i] != ',') return false;
		}
		return true;
	}
};

template<typename T>
std::vector<T> OSMParser::flatten(const std::vector<std::vector<T>> &container) {
	std::size_t finalSize = 0;
	for (const auto &part : container) {
		finalSize += part.size();
	}

	std::vector<T> result(finalSize);
	std::size_t idx = 0;
	for (const auto &part : container) {
		for (const T &element : part) {
			result[idx++] = element;
		}
	}

	return result;
}

} /* namespace CRP */



#endif /* IO_OSMPARSER_H_ */
