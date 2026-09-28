#pragma once

#include <vector>

#include "imgui/imgui.h"




namespace features::chams
{
    using pieces = std::vector<std::vector<ImVec2>>;


    std::vector<ImVec2> convex_hull(std::vector<ImVec2> pts);


    void subtract_poly(std::vector<ImVec2> piece, const std::vector<ImVec2>& b, pieces& out);


    void draw_segment_outside_union(ImDrawList* dl, const ImVec2& a, const ImVec2& b,
        const pieces& polys, int skip, ImU32 color);


    void draw_flat(ImDrawList* dl, const pieces& clipped, const pieces& hulls,
        ImU32 fill, ImU32 outline);
}

