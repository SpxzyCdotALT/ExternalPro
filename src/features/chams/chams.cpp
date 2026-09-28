#include "chams.hpp"

#include <algorithm>
#include <cmath>

namespace features::chams
{
    namespace
    {
        bool seg_inside_poly(const ImVec2& a, const ImVec2& b,
            const std::vector<ImVec2>& poly, float& out_t0, float& out_t1)
        {
            const int n = (int)poly.size();
            if (n < 3)
                return false;

            ImVec2 c(0, 0);
            for (const auto& p : poly)
            {
                c.x += p.x;
                c.y += p.y;
            }
            c.x /= n;
            c.y /= n;

            float t0 = 0.0f, t1 = 1.0f;
            const float eps = 0.25f;

            for (int i = 0; i < n; ++i)
            {
                const ImVec2& p = poly[i];
                const ImVec2& q = poly[(i + 1) % n];
                const float ex = q.x - p.x, ey = q.y - p.y;

                auto side = [&](const ImVec2& v) { return ex * (v.y - p.y) - ey * (v.x - p.x); };
                float s = -1.0f;
                if (side(c) >= 0.0f)
                    s = 1.0f;

                const float f0 = s * side(a);
                const float f1 = s * side(b);
                const float df = f1 - f0;

                if (fabsf(df) < 1e-6f)
                {
                    if (f0 < -eps)
                        return false;
                    continue;
                }

                const float tc = (-eps - f0) / df;
                if (df > 0.0f)
                {
                    if (tc > t0)
                        t0 = tc;
                }
                else
                {
                    if (tc < t1)
                        t1 = tc;
                }

                if (t0 >= t1)
                    return false;
            }

            if (t0 < 0.0f) t0 = 0.0f;
            if (t1 > 1.0f) t1 = 1.0f;
            out_t0 = t0;
            out_t1 = t1;
            return out_t1 > out_t0;
        }

        std::vector<ImVec2> clip_half_plane(const std::vector<ImVec2>& poly,
            const ImVec2& p, const ImVec2& q, float s)
        {
            std::vector<ImVec2> out;
            const int n = (int)poly.size();
            if (n < 3)
                return out;
            out.reserve(n + 2);

            auto f = [&](const ImVec2& v) {
                return s * ((q.x - p.x) * (v.y - p.y) - (q.y - p.y) * (v.x - p.x));
            };

            for (int i = 0; i < n; ++i)
            {
                const ImVec2& a = poly[i];
                const ImVec2& b = poly[(i + 1) % n];
                const float fa = f(a), fb = f(b);
                if (fa >= 0.0f)
                    out.push_back(a);

                if ((fa < 0.0f) != (fb < 0.0f))
                {
                    const float t = fa / (fa - fb);
                    out.push_back(ImVec2(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t));
                }
            }

            if (out.size() < 3)
                out.clear();

            return out;
        }
    }

    std::vector<ImVec2> convex_hull(std::vector<ImVec2> pts)
    {
        if (pts.size() < 3)
            return pts;

        std::sort(pts.begin(), pts.end(), [](const ImVec2& a, const ImVec2& b) {
            if (a.x < b.x)
                return true;
            if (a.x > b.x)
                return false;
            return a.y < b.y;
        });

        auto cross = [](const ImVec2& o, const ImVec2& a, const ImVec2& b) {
            return (a.x - o.x) * (b.y - o.y) - (a.y - o.y) * (b.x - o.x);
        };

        std::vector<ImVec2> hull(pts.size() * 2);
        int k = 0;
        for (size_t i = 0; i < pts.size(); ++i)
        {
            while (k >= 2 && cross(hull[k - 2], hull[k - 1], pts[i]) <= 0.0f)
                k--;
            hull[k++] = pts[i];
        }

        for (int i = (int)pts.size() - 2, t = k + 1; i >= 0; --i)
        {
            while (k >= t && cross(hull[k - 2], hull[k - 1], pts[i]) <= 0.0f)
                k--;
            hull[k++] = pts[i];
        }

        if (k > 0)
            hull.resize(k - 1);
        else
            hull.resize(0);

        return hull;
    }

    void subtract_poly(std::vector<ImVec2> piece, const std::vector<ImVec2>& b, pieces& out)
    {
        const int n = (int)b.size();
        if (n < 3)
        {
            if (piece.size() >= 3)
                out.push_back(std::move(piece));
            return;
        }

        ImVec2 c(0, 0);
        for (const auto& v : b)
        {
            c.x += v.x;
            c.y += v.y;
        }
        c.x /= n;
        c.y /= n;

        for (int i = 0; i < n && piece.size() >= 3; ++i)
        {
            const ImVec2& p = b[i];
            const ImVec2& q = b[(i + 1) % n];
            const float cs = (q.x - p.x) * (c.y - p.y) - (q.y - p.y) * (c.x - p.x);
            float s = -1.0f;
            if (cs >= 0.0f)
                s = 1.0f;

            auto outside = clip_half_plane(piece, p, q, -s);
            if (!outside.empty())
                out.push_back(std::move(outside));
            piece = clip_half_plane(piece, p, q, s);
        }
    }

    void draw_segment_outside_union(ImDrawList* dl, const ImVec2& a, const ImVec2& b,
        const pieces& polys, int skip, ImU32 color)
    {
        std::vector<std::pair<float, float>> covered;
        for (int i = 0; i < (int)polys.size(); ++i)
        {
            if (i == skip)
                continue;
            float t0 = 0.0f, t1 = 0.0f;
            if (seg_inside_poly(a, b, polys[i], t0, t1))
                covered.emplace_back(t0, t1);
        }

        auto lerp_pt = [&](float t) { return ImVec2(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t); };

        if (covered.empty())
        {
            dl->AddLine(a, b, color, 1.0f);
            return;
        }

        std::sort(covered.begin(), covered.end());

        const float min_piece = 0.002f;
        float cursor = 0.0f;
        for (const auto& iv : covered)
        {
            if (iv.first > cursor + min_piece)
                dl->AddLine(lerp_pt(cursor), lerp_pt(iv.first), color, 1.0f);
            if (iv.second > cursor)
                cursor = iv.second;
            if (cursor >= 1.0f)
                break;
        }

        if (cursor < 1.0f - min_piece)
            dl->AddLine(lerp_pt(cursor), lerp_pt(1.0f), color, 1.0f);
    }

    void draw_flat(ImDrawList* dl, const pieces& clipped, const pieces& hulls,
        ImU32 fill, ImU32 outline)
    {
        if (!dl)
            return;

        const ImDrawListFlags backup = dl->Flags;
        dl->Flags &= ~ImDrawListFlags_AntiAliasedFill;

        for (const auto& piece : clipped)
            if (piece.size() >= 3)
                dl->AddConvexPolyFilled(piece.data(), (int)piece.size(), fill);

        dl->Flags = backup;

        for (int i = 0; i < (int)hulls.size(); ++i)
        {
            const auto& hull = hulls[i];
            const int n = (int)hull.size();
            if (n < 2)
                continue;
            for (int e = 0; e < n; ++e)
                draw_segment_outside_union(dl, hull[e], hull[(e + 1) % n], hulls, i, outline);
        }
    }
}
