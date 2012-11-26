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

#include <iostream>

#include "railrouting.hh"

int main(int argc, char** argv) {
	if (argc != 2) {
		std::cerr << "Usage: " << argv[0] << " file.osm" << std::endl;
		return 1;
	}

	RailRouting routing;
	routing.Parse(argv[1]);

	RailRouting::FindRouteResult result;

	if (!routing.FindRoute("Лосиноостровская", "Лось", result)) {
		std::cerr << "Unable to find route: " << result.StatusString() << std::endl;
		return 1;
	}

	std::cout << "Route found, distance = " << result.distance/1000.0 << " km" << std::endl;
	std::cout << "Start node id: " << result.start_node->GetId() << ", name: " << result.start_node->GetTag("name") << std::endl;
	std::cout << "End node id: " << result.end_node->GetId() << ", name: " << result.end_node->GetTag("name") << std::endl;

	std::cout << "Route:" << std::endl;

	std::cout << std::setiosflags(std::ios::fixed) << std::setprecision(7);

	for (std::vector<const Node*>::const_iterator node = result.route_nodes.begin(); node != result.route_nodes.end(); ++node)
		std::cout << "  " << (*node)->GetLonD() << ", " << (*node)->GetLatD() << std::endl;

	return 0;
}
