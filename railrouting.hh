/*
 * Copyright (C) 2012 Dmitry Marakasov
 *
 * This file is part of rail routing demo.
 *
 * rail routing demo is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * rail routing demo is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with rail routing demo.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef RAILROUTING_HH
#define RAILROUTING_HH

#include <map>
#include <unordered_map>
#include <set>

#include "pool.hh"

#include "ParserBase.hh"

class RailRouting : public ParserBase<RailRouting> {
private:
	struct RouteNode;

	struct RouteEdge {
		osmid_t osmid;
		unsigned short start_pos;
		unsigned short end_pos;
		int node;
		float direction;
		double length;

		RouteEdge() : osmid(0), start_pos(-1), end_pos(-1), node(-1), direction(0), length(0.0) {
		}
	};

	struct RouteNode {
		osmid_t osmid;
		int nedges;
		RouteEdge* edges;

		RouteNode(osmid_t id, int n, RouteEdge* e) : osmid(id), nedges(n), edges(e) {
		}
	};

	struct ConnectivityInfo {
		int nedges;
		int nways;
		bool isstop;

		ConnectivityInfo() : nedges(0), nways(0), isstop(false) {
		}
	};

public:
	struct FindRouteResult {
		enum RouteStatus {
			OK,
			START_STATION_NOT_FOUND,
			END_STATION_NOT_FOUND,
			BOTH_STATIONS_NOT_FOUND,
			NO_ROUTE_FOUND,
		} status;

		int start_count;
		int end_count;

		const Node* start_node;
		const Node* end_node;

		double distance;

		std::vector<const Node*> route_nodes;
		std::vector<const Node*> sharp_turns;

		const char* StatusString() const {
			switch (status) {
			case OK:
				return "OK";
			case START_STATION_NOT_FOUND:
				return "Start station not found";
			case END_STATION_NOT_FOUND:
				return "End station not found";
			case BOTH_STATIONS_NOT_FOUND:
				return "Both stations not found";
			case NO_ROUTE_FOUND:
				return "No route found";
			}
			return "Unknown";
		}
	};

private:
	/* OSM nodes and ways */
	typedef std::map<osmid_t, Node> NodeMap;
	NodeMap nodes_;

	typedef std::map<osmid_t, Way> WayMap;
	WayMap ways_;

	/* set of node ids referenced by ways */
	typedef std::set<osmid_t> IdSet;
	IdSet needed_nodes_;

	/* map of node ids for stops by name */
	typedef std::multimap<std::string, int> StopMap;
	StopMap stops_;

	/* route nodes */
	typedef std::vector<RouteNode> RouteNodeVector;
	RouteNodeVector route_nodes_;

	/* mapping of ism node ids to routing node indexes */

	/* pool for route edges */
	typedef pool<RouteEdge> RouteEdgePool;
	RouteEdgePool route_edge_pool_;

private:
	void ProcessNode(Node& node);
	void ProcessWay(Way& way);
	void Prepare();

public:
	RailRouting();

	bool FindRoute(const std::string& name_a, const std::string& name_b, FindRouteResult& result) const;
};

#endif
