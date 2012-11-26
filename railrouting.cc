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

#include <unordered_map>
#include <limits>
#include <cassert>

#include "railrouting.hh"

#include "lazyinit_array.hh"

#include "geomath.hh"

RailRouting::RailRouting() {
	AddPass(NULL, &RailRouting::ProcessWay, NULL, NULL, false, "loading ways");
	AddPass(&RailRouting::ProcessNode, NULL, NULL, NULL, false, "loading nodes");
	AddPass(NULL, NULL, NULL, &RailRouting::Prepare, false, "preparing");
}

void RailRouting::ProcessNode(Node& node) {
	if (needed_nodes_.find(node.GetId()) != needed_nodes_.end())
		nodes_.insert(std::make_pair(node.GetId(), node));
}

void RailRouting::ProcessWay(Way& way) {
	std::string railway;
	if (way.GetTag("railway", railway) && (railway == "rail" || railway == "abandoned" || railway == "disused" || railway == "narrow_gauge")) {
		for (int i = 0; i < way.GetNodesCount(); ++i)
			needed_nodes_.insert(way.NodeAt(i));
		ways_.insert(std::make_pair(way.GetId(), way));
	}
}

void RailRouting::Prepare() {
	/* drop unused */
	needed_nodes_.clear();

	std::cerr << nodes_.size() << " nodes" << std::endl;
	std::cerr << ways_.size() << " ways" << std::endl;

	typedef std::unordered_map<osmid_t, ConnectivityInfo> NodeConnectivityMap;
	NodeConnectivityMap node_connectivity;

	typedef std::unordered_map<osmid_t, int> IdToRouteNodeMap;
	IdToRouteNodeMap id_to_routenode;

	typedef std::multimap<std::string, osmid_t> TempStopMap;
	TempStopMap temp_stops_;

	/* find stops */
	for (NodeMap::const_iterator node = nodes_.begin(); node != nodes_.end(); node++) {
		if (node->second.IsTag("railway", "station") || node->second.IsTag("railway", "halt") ||
					(node->second.IsTag("public_transport", "stop_position") && node->second.IsTag("train", "yes"))
				) {
			std::string name;
			if (node->second.GetTag("name", name)) {
				temp_stops_.insert(std::make_pair(name, node->second.GetId()));
				node_connectivity[node->second.GetId()].isstop = true;
			}
			if (node->second.GetTag("alt_name", name)) {
				temp_stops_.insert(std::make_pair(name, node->second.GetId()));
				node_connectivity[node->second.GetId()].isstop = true;
			}
			if (node->second.GetTag("official_name", name)) {
				temp_stops_.insert(std::make_pair(name, node->second.GetId()));
				node_connectivity[node->second.GetId()].isstop = true;
			}
		}
	}

	/* process all ways and build connectivity map */
	for (WayMap::const_iterator way = ways_.begin(); way != ways_.end(); way++) {
		if (way->second.GetNodesCount() <= 1)
			continue;

		for (int i = 0; i < way->second.GetNodesCount(); i++) {
			node_connectivity[way->second.NodeAt(i)].nedges += ((i == 0 || i == way->second.GetNodesCount() - 1) ? 1 : 2);
			node_connectivity[way->second.NodeAt(i)].nways++;
		}
	}

	/* create route nodes */
	for (NodeMap::const_iterator node = nodes_.begin(); node != nodes_.end(); node++) {
		NodeConnectivityMap::const_iterator connectivity = node_connectivity.find(node->first);
		assert(connectivity != node_connectivity.end());

		if (connectivity->second.nways > 1 || connectivity->second.nedges != 2 || connectivity->second.isstop) {
			route_nodes_.push_back(RouteNode(node->first, connectivity->second.nedges, route_edge_pool_.alloc(connectivity->second.nedges)));
			id_to_routenode.insert(std::make_pair(node->first, route_nodes_.size() - 1));
		}
	}

	std::cerr << route_nodes_.size() << " routing nodes" << std::endl;

	/* split real edges to route edges */
	int nedges = 0;
	for (WayMap::const_iterator way = ways_.begin(); way != ways_.end(); way++) {
		if (way->second.GetNodesCount() < 2) {
			std::cerr << "way #" << way->first << ": has only " << way->second.GetNodesCount() << " nodes, skipping" << std::endl;
			continue;
		}

		/* find first node */
		NodeMap::const_iterator start_node = nodes_.find(way->second.NodeAt(0));
		if (start_node == nodes_.end()) {
			std::cerr << "way #" << way->first << ": missing node[0] #" << way->second.NodeAt(0) << ", skipping" << std::endl;
			continue;
		}

		NodeMap::const_iterator prev_node = start_node;
		int start_route_node = -1;
		int start_node_pos = 0;

		NodeMap::const_iterator second_node = nodes_.end();

		/* find route node index for first node */
		{
			IdToRouteNodeMap::const_iterator routeidx = id_to_routenode.find(prev_node->first);
			assert(routeidx != id_to_routenode.end());

			start_route_node = routeidx->second;
		}

		double dist = 0.0;
		for (int node_pos = 1; node_pos < way->second.GetNodesCount(); ++node_pos) {
			/* find current node */
			NodeMap::const_iterator this_node = nodes_.find(way->second.NodeAt(node_pos));
			if (this_node == nodes_.end()) {
				std::cerr << "way #" << way->first << ": missing node[" << node_pos << "] #" << way->second.NodeAt(node_pos) << ", skipping rest" << std::endl;
				node_pos = way->second.GetNodesCount();
				continue;
			}

			if (node_pos == 1)
				second_node = this_node;

			/* add distance of last segment */
			dist += Distance(prev_node->second, this_node->second);

			/* check whether this is a routing node */
			int this_route_node = -1;
			{
				IdToRouteNodeMap::const_iterator routeidx = id_to_routenode.find(this_node->first);
				if (routeidx != id_to_routenode.end())
					this_route_node = routeidx->second;

				/* last node must be route node; just a check */
				if (node_pos == way->second.GetNodesCount() - 1)
					assert(this_route_node != -1);
			}

			/* (node positions should fit into ushorts) */
			assert(start_node_pos < 65535 && node_pos < 65535);

			/* if we're at routing node, add routing edge */
			if (this_route_node != -1) {
				int edge_pos;

				/* add forward edge, taking oneway into account */
				if (!way->second.IsTag("oneway", "-1") && !way->second.IsTag("designated_direction", "backward")) {
					for (edge_pos = 0; edge_pos < route_nodes_[start_route_node].nedges; edge_pos++) {
						if (route_nodes_[start_route_node].edges[edge_pos].osmid == 0) {
							route_nodes_[start_route_node].edges[edge_pos].osmid = way->first;
							route_nodes_[start_route_node].edges[edge_pos].start_pos = start_node_pos;
							route_nodes_[start_route_node].edges[edge_pos].end_pos = node_pos;
							route_nodes_[start_route_node].edges[edge_pos].node = this_route_node;
							route_nodes_[start_route_node].edges[edge_pos].direction = Bearing(start_node->second, second_node->second);;
							route_nodes_[start_route_node].edges[edge_pos].length = dist;
							nedges++;
							break;
						}
					}
					assert(edge_pos != route_nodes_[start_route_node].nedges);
				}

				/* add backward edge, taking oneway into account */
				if (!way->second.IsTag("oneway", "yes") && !way->second.IsTag("designated_direction", "forward")) {
					for (edge_pos = 0; edge_pos < route_nodes_[this_route_node].nedges; edge_pos++) {
						if (route_nodes_[this_route_node].edges[edge_pos].osmid == 0) {
							route_nodes_[this_route_node].edges[edge_pos].osmid = way->first;
							route_nodes_[this_route_node].edges[edge_pos].start_pos = node_pos;
							route_nodes_[this_route_node].edges[edge_pos].end_pos = start_node_pos;
							route_nodes_[this_route_node].edges[edge_pos].node = start_route_node;
							route_nodes_[this_route_node].edges[edge_pos].direction = Bearing(this_node->second, prev_node->second);
							route_nodes_[this_route_node].edges[edge_pos].length = dist;
							nedges++;
							break;
						}
					}
					assert(edge_pos != route_nodes_[this_route_node].nedges);
				}

				dist = 0.0;
				start_node = this_node;
				start_route_node = this_route_node;
				start_node_pos = node_pos;
			}

			prev_node = this_node;
		}
	}

	std::cerr << nedges << " routing edges" << std::endl;

	/* postprocess stops */
	for (TempStopMap::const_iterator stop = temp_stops_.begin(); stop != temp_stops_.end(); ++stop) {
		IdToRouteNodeMap::const_iterator routeidx = id_to_routenode.find(stop->second);
		assert(routeidx != id_to_routenode.end());

		stops_.insert(std::make_pair(stop->first, routeidx->second));
	}
}

bool RailRouting::FindRoute(const std::string& name_a, const std::string& name_b, FindRouteResult& result) const {
	typedef std::multimap<double, int> NodeQueue;
	NodeQueue queue;

	typedef std::set<int> NodeSet;
	NodeSet start_nodes;
	NodeSet fin_nodes;

	result.start_count = result.end_count = 0;

	lazyinit_array<int> starts(route_nodes_.size(), -1);
	lazyinit_array<int> prevs(route_nodes_.size(), -1);
	lazyinit_array<double> lengths(route_nodes_.size(), std::numeric_limits<double>::infinity());

	/* find stops for station A */
	std::pair<StopMap::const_iterator, StopMap::const_iterator> stops = stops_.equal_range(name_a);
	for (StopMap::const_iterator stop = stops.first; stop != stops.second; stop++) {
		start_nodes.insert(stop->second);
		prevs[stop->second] = -1;
		starts[stop->second] = stop->second;
		lengths[stop->second] = 0.0;
		queue.insert(std::make_pair(0.0, stop->second));
		result.start_count++;
	}

	/* fin stop for station B */
	stops = stops_.equal_range(name_b);
	for (StopMap::const_iterator stop = stops.first; stop != stops.second; stop++) {
		fin_nodes.insert(stop->second);
		result.end_count++;
	}

	if (start_nodes.empty() && fin_nodes.empty()) {
		result.status = FindRouteResult::BOTH_STATIONS_NOT_FOUND;
		return false;
	} else if (start_nodes.empty()) {
		result.status = FindRouteResult::START_STATION_NOT_FOUND;
		return false;
	} else if (fin_nodes.empty()) {
		result.status = FindRouteResult::END_STATION_NOT_FOUND;
		return false;
	}

	/* Dijkstra */
	double shortest_length = std::numeric_limits<double>::infinity();
	while (!queue.empty()) {
		/* take first node from queue */
		const int current_node = queue.begin()->second;
		const double current_length = queue.begin()->first;

		queue.erase(queue.begin());

		/* if it's length has changed, it was visited earlier, so we
		 * don't need to revisit it (1) */
		if (lengths[current_node] < current_length)
			continue;

		/* if it's fin node, remember route length */
		if (fin_nodes.find(current_node) != fin_nodes.end())
			shortest_length = std::min(shortest_length, lengths[current_node]);

		for (int nedge = 0; nedge < route_nodes_[current_node].nedges; nedge++) {
			const double new_length = lengths[current_node] + route_nodes_[current_node].edges[nedge].length;

			/* we may just ignore longer routes that already found ones */
			if (new_length > shortest_length)
				break;

			const int other_node = route_nodes_[current_node].edges[nedge].node;
			if (other_node == -1) {
				/* may happen if we've dropped some incomplete ways in Prepare() */
				continue;
			}

			if (new_length < lengths[other_node]) {
				starts[other_node] = starts[current_node];
				prevs[other_node] = current_node;
				lengths[other_node] = new_length;

				queue.insert(std::make_pair(new_length, other_node));
			}
		}
	}

	/* find closest final node */
	int best_fin = *fin_nodes.begin();
	double best_length = lengths[best_fin];
	for (NodeSet::const_iterator fin = ++fin_nodes.begin(); fin != fin_nodes.end(); ++fin) {
		if (lengths[*fin] < best_length) {
			best_length = lengths[*fin];
			best_fin = *fin;
		}
	}

	if (best_length == std::numeric_limits<double>::infinity()) {
		result.status = FindRouteResult::NO_ROUTE_FOUND;
		return false;
	}

	/* fill rest of RouteResult */
	NodeMap::const_iterator start_node = nodes_.find(route_nodes_[starts[best_fin]].osmid);
	NodeMap::const_iterator end_node = nodes_.find(route_nodes_[best_fin].osmid);

	assert(start_node != nodes_.end());
	assert(end_node != nodes_.end());

	result.start_node = &start_node->second;
	result.end_node = &end_node->second;
	result.distance = best_length;
	result.status = FindRouteResult::OK;

	/* recover route */
	std::vector<const Node*> temp_nodes;

	int current_route_node = best_fin;
	int prev_route_node = prevs[current_route_node];
	while (prev_route_node != -1) {
		const RouteEdge* edge;
		/* find edge from prev to current node*/
		for (edge = route_nodes_[prev_route_node].edges; edge != route_nodes_[prev_route_node].edges + route_nodes_[prev_route_node].nedges; edge++)
			if (edge->node == current_route_node)
				break;

		assert(edge != route_nodes_[prev_route_node].edges + route_nodes_[prev_route_node].nedges);

		/* find way which forms the edge*/
		WayMap::const_iterator way = ways_.find(edge->osmid);
		assert(way != ways_.end());

		/* traverse way in correct direction, and get all nodes that belong to this route edge*/
		if (edge->start_pos < edge->end_pos) {
			for (int node_pos = edge->end_pos; node_pos > edge->start_pos; --node_pos) {
				NodeMap::const_iterator node = nodes_.find(way->second.NodeAt(node_pos));
				assert(node != nodes_.end());

				temp_nodes.push_back(&node->second);
			}
		} else {
			for (int node_pos = edge->end_pos; node_pos < edge->start_pos; ++node_pos) {
				NodeMap::const_iterator node = nodes_.find(way->second.NodeAt(node_pos));
				assert(node != nodes_.end());

				temp_nodes.push_back(&node->second);
			}
		}

		/* one step back */
		current_route_node = prev_route_node;
		prev_route_node = prevs[current_route_node];
	}

	/* add first node (not added in the above loop) */
	temp_nodes.push_back(&start_node->second);

	/* reverse route in forward direction */
	std::reverse(temp_nodes.begin(), temp_nodes.end());

	/* find sharp turns */
	std::vector<const Node*> temp_sharp_turns;
	if (temp_nodes.size() > 2) {
		double prev_bearing = Bearing(**temp_nodes.begin(), **(temp_nodes.begin()+1));
		for (std::vector<const Node*>::const_iterator node = ++temp_nodes.begin(); node != --temp_nodes.end(); ++node) {
			double bearing = Bearing(**(node), **(node+1));

			double delta = (bearing - prev_bearing)/M_PI*180.0;
			if (delta > 180.0) delta -= 360.0;
			if (delta < -180.0) delta += 360.0;
			if (delta < 0.0) delta = -delta;

			if (delta > 90.0)
				temp_sharp_turns.push_back(*node);

			prev_bearing = bearing;
		}
	}

	result.route_nodes.swap(temp_nodes);
	result.sharp_turns.swap(temp_sharp_turns);

	return true;
}
