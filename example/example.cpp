#include "osrm/match_parameters.hpp"
#include "osrm/nearest_parameters.hpp"
#include "osrm/route_parameters.hpp"
#include "osrm/table_parameters.hpp"
#include "osrm/trip_parameters.hpp"

#include "osrm/coordinate.hpp"
#include "osrm/engine_config.hpp"
#include "osrm/json_container.hpp"

#include "osrm/osrm.hpp"
#include "osrm/status.hpp"

#include <exception>
#include <iostream>
#include <string>
#include <utility>

#include <cstdlib>

int main(int argc, const char *argv[])
{
    if (argc < 2)
    {
        std::cerr << "Usage: " << argv[0] << " data.osrm\n";
        return EXIT_FAILURE;
    }

    using namespace osrm;

    // Configure based on a .osrm base path, and no datasets in shared mem from osrm-datastore
    EngineConfig config;

    config.storage_config = {argv[1]};
    config.use_shared_memory = false;

    // We support two routing speed up techniques:
    // - Contraction Hierarchies (CH): requires extract+contract pre-processing
    // - Multi-Level Dijkstra (MLD): requires extract+partition+customize pre-processing
    //
    config.algorithm = EngineConfig::Algorithm::CH;
    // config.algorithm = EngineConfig::Algorithm::MLD;

    // Routing machine with several services (such as Route, Table, Nearest, Trip, Match)
    const OSRM osrm{config};

    // The following shows how to use the Route service; configure this service
    RouteParameters params(false,
                           false,
                           osrm::engine::api::RouteParameters::AnnotationsType::Nodes,
                           osrm::engine::api::RouteParameters::GeometriesType::GeoJSON,
                           osrm::engine::api::RouteParameters::OverviewType::Full,
                           false
                           );

    // Route
    params.coordinates.push_back({util::FloatLongitude{103.9880}, util::FloatLatitude{1.3556}});
    params.coordinates.push_back({util::FloatLongitude{103.9554}, util::FloatLatitude{1.3562}});

    // Response is in JSON format
    engine::api::ResultT result = json::Object();

    // Execute routing request, this does the heavy lifting
    const auto status = osrm.Route(params, result);

    auto &json_result = result.get<json::Object>();
    if (status == Status::Ok)
    {
        auto &routes = json_result.values["routes"].get<json::Array>();

        // Let's just use the first route
        auto &route = routes.values.at(0).get<json::Object>();
        const auto distance = route.values["distance"].get<json::Number>().value;
        const auto duration = route.values["duration"].get<json::Number>().value;

        json::Object &geometry = route.values["geometry"].get<json::Object>();

        std::string type = geometry.values["type"].get<json::String>().value;
        std::cout << type << std::endl;

        json::Array &coordinates = geometry.values["coordinates"].get<json::Array>();

        std::cout << std::setprecision(10);
        for (int index = 0; index < coordinates.values.size(); index++) {
            std::vector<json::Value> pair = coordinates.values.at(index).get<json::Array>().values;
            double x = pair[0].get<json::Number>().value;
            double y = pair[1].get<json::Number>().value;
            std::cout << x << " , " << y << std::endl;
        }


        // Warn users if extract does not contain the default coordinates from above
        if (distance == 0 || duration == 0)
        {
            std::cout << "Note: distance or duration is zero. ";
            std::cout << "You are probably doing a query outside of the OSM extract.\n\n";
        }

        std::cout << "Distance: " << distance << " meter\n";
        std::cout << "Duration: " << duration << " seconds\n";
        return EXIT_SUCCESS;
    }
    else if (status == Status::Error)
    {
        const auto code = json_result.values["code"].get<json::String>().value;
        const auto message = json_result.values["message"].get<json::String>().value;

        std::cout << "Code: " << code << "\n";
        std::cout << "Message: " << code << "\n";
        return EXIT_FAILURE;
    }
}
