// map_renderer.h
#pragma once
#include <string>

#include "transport_catalogue.h"

std::string RenderBusSvg(const Bus& bus,
                         double width = 800.0,
                         double height = 600.0,
                         double padding = 50.0);
