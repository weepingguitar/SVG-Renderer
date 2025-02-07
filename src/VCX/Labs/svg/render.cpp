# include <iostream>
# include <set>
# include <unordered_map>
# include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

# include "render.h"

namespace VCX::Labs::Project {

    static glm::ivec2 offset;
    static float width, height, ratio;


    glm::vec3 parseHex(const char *s) {
        bool isValid = true;
        for (int i = 0; s[i]; ++i) {
            if (!isxdigit(static_cast<unsigned char>(s[i]))) {
                isValid = false;
                break;
            }
        }

        size_t length = strlen(s);
        if (!isValid || (length != 3 && length != 6)) {
            std::cerr << "parseHex error: Invalid hex string" << std::endl;
            return glm::vec3(0.0f);
        }

        int charsPerComponent = (length == 3) ? 1 : 2;

        int r = std::stoi(std::string(s, s + charsPerComponent), nullptr, 16);
        int g = std::stoi(std::string(s + charsPerComponent, s + 2 * charsPerComponent), nullptr, 16);
        int b = std::stoi(std::string(s + 2 * charsPerComponent, s + 3 * charsPerComponent), nullptr, 16);

        float scale = (charsPerComponent == 1) ? 15.0f : 255.0f;
        return glm::vec3(r / scale, g / scale, b / scale);
    }

    glm::vec3 parseColor(const char *s) {
        if (s == nullptr) {
            s = "black";
        }

        if (s[0] == '#') {
            return parseHex(s + 1);
        }

        if (strncmp(s, "rgb", 3) == 0) {
            auto components = getNum(s);
            if (components.size() < 3) {
                std::cerr << "parseColor: invalid RGB color string" << std::endl;
                return glm::vec3(0.0f);
            }
            return glm::vec3(components[0] / 255.0f, components[1] / 255.0f, components[2] / 255.0f);
        }

        static const std::unordered_map<std::string, glm::vec3> namedColors = {
            {"black",   parseHex("000000")},
            {"gray",    parseHex("808080")},
            {"white",   parseHex("ffffff")},
            {"red",     parseHex("ff0000")},
            {"green",   parseHex("008000")},
            {"yellow",  parseHex("ffff00")},
            {"blue",    parseHex("0000ff")},
            {"orange",  parseHex("ffa500")}
        };

        auto it = namedColors.find(s);
        if (it != namedColors.end()) {
            return it->second;
        }

        std::cerr << "parseColor: invalid color string " << s << std::endl;
        return glm::vec3(0.0f);
    }

    glm::vec4 opaticalColor(const tinyxml2::XMLElement *ele, const char *attr) {
        const char *color = ele->Attribute(attr);

        if (!color) {
            if (strcmp(attr, "stroke") == 0) {
                color = "none";
            } else if (strcmp(attr, "fill") == 0) {
                color = "black";
            } else {
                return glm::vec4(0.0f);
            }
        }

        if (strcmp(color, "transparent") == 0 || strcmp(color, "none") == 0) {
            return glm::vec4(0.0f);
        }

        glm::vec3 rgb = parseColor(color);
        float opacity = ele->FloatAttribute((std::string(attr) + "-opacity").c_str(), 1.0f);

        return glm::vec4(rgb, opacity);
    }

    std::vector<float> getNum(const char *&s, int n){
        std::vector<float> nums;
        int pos = 0;
        bool hadDot = 0;

        for (int i = 0;; ++i) {

            if((pos == i) && s[i] == '.')
                hadDot = 1;

            if (!isdigit(s[i]) && s[i] != '.' && s[i] != '-' && s[i] != '+' || ((s[i] == '-' || s[i]=='.') && pos < i)) {
                bool out = hadDot;
                if (s[i]!='.' && i != pos || hadDot){
                    nums.push_back(std::stof(std::string(s + pos, s + i)));
                    hadDot = 0;
                }
                if (s[i]=='.'){
                    hadDot = 1;
                    if (out){
                        pos = i;
                    }
                }
                else
                    pos = i + (s[i]!='-');
            }

            if (!s[i] || nums.size() == n){
                s += i - 1;
                break;
            }
        }
        return nums;
    }


    std::vector<glm::vec2> getPoint(const char *s){
        auto numbers = getNum(s);
        std::vector<glm::vec2> points;
        for (int i = 0; i < numbers.size(); i += 2)
            points.push_back({numbers[i], numbers[i + 1]});
        return points;
    }


    static ImageRGB *canvas;
    static bool skip;

    void renderImage(ImageRGB &image, const tinyxml2::XMLElement *root, int &outWidth, int &outHeight, bool skipDrawing) {
        canvas = &image;
        width = outWidth;
        height = outHeight;
        skip = skipDrawing;

        glm::vec2 imageDimensions(0.0f);
        root->QueryFloatAttribute("width", &imageDimensions.x);
        root->QueryFloatAttribute("height", &imageDimensions.y);

        if (const char *viewBoxAttr = root->Attribute("viewBox")) {
            std::vector<glm::vec2> viewBox = getPoint(viewBoxAttr);
            glm::vec2 viewBoxSize = viewBox[1];

            if (imageDimensions.y == 0.0f) {
                imageDimensions = viewBoxSize;
            }

            float aspectRatio = imageDimensions.x / imageDimensions.y;
            float viewBoxAspectRatio = viewBoxSize.x / viewBoxSize.y;

            if (viewBoxAspectRatio < aspectRatio) {
                viewBoxSize.x = viewBoxSize.y * aspectRatio;
            } else {
                viewBoxSize.y = viewBoxSize.x / aspectRatio;
            }

            imageDimensions = viewBoxSize;
        }

        if ((width / imageDimensions.x) < (height / imageDimensions.y)) {
            height = static_cast<int>(std::ceil(width * imageDimensions.y / imageDimensions.x));
            ratio = imageDimensions.x / width;
        } else {
            width = static_cast<int>(std::ceil(height * imageDimensions.x / imageDimensions.y));
            ratio = imageDimensions.y / height;
        }

        if (const char *viewBoxAttr = root->Attribute("viewBox")) {
            std::vector<glm::vec2> viewBox = getPoint(viewBoxAttr);
            glm::vec2 viewBoxOrigin = viewBox[0];
            glm::vec2 viewBoxSize = viewBox[1];

            offset = (viewBoxOrigin + viewBoxSize - imageDimensions) / (2.0f * ratio);
        } else {
            offset = glm::vec2(0.0f);
        }

        outWidth = width;
        outHeight = height;

        image = Common::CreatePureImageRGB(height, width, {1.0f, 1.0f, 1.0f});

        conditions(root);
    }

    void conditions(tinyxml2::XMLElement const *ele){
        for (auto child = ele->FirstChildElement(); child != NULL; child = child->NextSiblingElement()) {
            if (child->Name() == std::string("g"))
                conditions(child);
            else if (child->Name() == std::string("rect"))
                drawRect(child);
            else if (child->Name() == std::string("polygon"))
                drawPolygon(child);
            else if (child->Name() == std::string("circle"))
                drawCircle(child);
            else if (child->Name() == std::string("path"))
                drawPath(child);
        }
        return;
    }

    void cubic(std::vector<glm::vec2> &points, glm::vec2 p0, glm::vec2 p1, glm::vec2 p2, glm::vec2 p3){
        float t = 0;
        for (; t < 1;t += 0.01){
            glm::vec2 p = (1 - t) * (1 - t) * (1 - t) * p0 + 3 * (1 - t) * (1 - t) * t * p1 + 3 * (1 - t) * t * t * p2 + t * t * t * p3;
            points.push_back(p);
        }
    }

    void quadratic(std::vector<glm::vec2> &points, glm::vec2 p0, glm::vec2 p1, glm::vec2 p2){
        float t = 0;
        for (; t < 1;t += 0.02){
            glm::vec2 p = (1 - t) * (1 - t) * p0 + 2 * (1 - t) * t * p1 + t * t * p2;
            points.push_back(p);
        }
    }

    void drawRect(const tinyxml2::XMLElement *ele) {
        float x, y, w, h, rx, ry;
        x = ele->FloatAttribute("x", 0);
        y = ele->FloatAttribute("y", 0);
        rx = ele->FloatAttribute("rx", -1);
        ry = ele->FloatAttribute("ry", -1);
        if (rx < 0) rx = ry;
        if (ry < 0) ry = rx;
        if (rx < 0) rx = ry = 0;
        if (ele->QueryFloatAttribute("width", &w)) return;
        if (ele->QueryFloatAttribute("height", &h)) return;

        tinyxml2::XMLDocument doc;
        auto path = doc.NewElement("path");
        for (auto s: {"stroke", "stroke-width", "fill", "fill-opacity", "stroke-opacity"})
            if (ele->Attribute(s)) path->SetAttribute(s, ele->Attribute(s));
        std::string d = "M " + std::to_string(x + w/2) + " " + std::to_string(y);
        d += " h " + std::to_string(w/2 - rx);
        d += " a " + std::to_string(rx) + " " + std::to_string(ry) + " 0 0 1 " + std::to_string(rx) + " " + std::to_string(ry);
        d += " v " + std::to_string(h - ry*2);
        d += " a " + std::to_string(rx) + " " + std::to_string(ry) + " 0 0 1 " + std::to_string(-rx) + " " + std::to_string(ry);
        d += " h " + std::to_string(-w + rx*2);
        d += " a " + std::to_string(rx) + " " + std::to_string(ry) + " 0 0 1 " + std::to_string(-rx) + " " + std::to_string(-ry);
        d += " v " + std::to_string(-h + ry*2);
        d += " a " + std::to_string(rx) + " " + std::to_string(ry) + " 0 0 1 " + std::to_string(rx) + " " + std::to_string(-ry);
        d += " z";
        path->SetAttribute("d", d.c_str());
        drawPath(path);
    }

    void drawPolygon(const tinyxml2::XMLElement *ele) {
        auto points = getPoint(ele->Attribute("points"));
        if (points.empty()){
            std::cout << "Empty polygon" << std::endl;
            return;
        }
        for (auto &p : points) p /= ratio;
        points.push_back(points[0]);
        int n = points.size() - 1;

        const char *fillRule = ele->Attribute("fill-rule");
        int rule = 0;
        if (fillRule && strcmp(fillRule, "evenodd") == 0) rule = 1;
        glm::vec4 color = opaticalColor(ele, "fill");
        if (color.a > 0)
            drawFilled(color, {points}, rule);

        color = opaticalColor(ele, "stroke");
        float width = ele->FloatAttribute("stroke-width", 1) / ratio / 2;
        if (color.a > 0){
            drawOutline(color, points, width);
        }
    }

    void drawCircle(const tinyxml2::XMLElement *ele) {
        float cx, cy, r;
        if (ele->QueryFloatAttribute("cx", &cx)) return;
        if (ele->QueryFloatAttribute("cy", &cy)) return;
        if (ele->QueryFloatAttribute("r", &r)) return;
        cx /= ratio, cy /= ratio, r /= ratio;

        glm::vec4 color = opaticalColor(ele, "fill");
        if (color.a > 0)
            drawRing(color, { cx, cy }, r);

        color = opaticalColor(ele, "stroke");
        float width = ele->FloatAttribute("stroke-width", 1) / ratio / 2;
        if (color.a > 0)
            drawRing(color, { cx, cy }, r + width, r - width);
    }
    
    void drawPath(const tinyxml2::XMLElement *ele) {
        float x = 0, y = 0;                         
        std::vector<std::vector<glm::vec2>> paths;  
        std::vector<glm::vec2> path;                
        char prevCommand = 0;                       
        float bx = 0, by = 0;                      

        const char *s = ele->Attribute("d");
        if (!s) return;
        int len = strlen(s);

        for (const char *i = s; i < s + len; i++) {
            char command = 0;
            if (!isspace(*i)) {
                if (isalpha(*i)){
                    command = *i++;
                    prevCommand = command;
                    if (command == 'M') prevCommand = 'L';
                    if (command == 'm') prevCommand = 'l';
                }
                else command = prevCommand;
            }
            else continue;
            
            if (toupper(command) == 'M') {
                auto p = getNum(i, 2);
                if (p.size() < 2) return;
                if (command == 'm') {
                    x += p[0];
                    y += p[1];
                } else {
                    x = p[0];
                    y = p[1];
                }
                if (!path.empty()){
                    paths.push_back(path);
                    path.clear();
                }
                path.push_back({ x, y });
            }
            else if (toupper(command) == 'L') {
                auto p = getNum(i, 2);
                if (p.size() < 2) return;
                if (command == 'l') {
                    x += p[0];
                    y += p[1];
                } else {
                    x = p[0];
                    y = p[1];
                }
                path.push_back({ x, y });
            }
            else if (toupper(command) == 'H') {
                auto p = getNum(i, 1);
                if (p.size() < 1) return;
                if (command == 'h') x += p[0];
                else x = p[0];
                path.push_back({ x, y });
            }
            else if (toupper(command) == 'V') {
                auto p = getNum(i, 1);
                if (p.size() < 1) return;
                if (command == 'v') y += p[0];
                else y = p[0];
                path.push_back({ x, y });
            }
            else if (toupper(command) == 'Z') {
                --i;
                if (path.empty()) return;
                path.push_back(path[0]);
                x = path[0].x;
                y = path[0].y;
            }
            else if (toupper(command) == 'C') {
                auto p = getNum(i, 6);
                if (p.size() < 6) return;
                if (command == 'c') {
                    p[0] += x;
                    p[1] += y;
                    p[2] += x;
                    p[3] += y;
                    p[4] += x;
                    p[5] += y;
                }
                cubic(path, { x, y }, { p[0], p[1] }, { p[2], p[3] }, { p[4], p[5] });

                bx = p[2];
                by = p[3];
                x = p[4];
                y = p[5];
                path.push_back({ x, y });
            }
            else if (toupper(command) == 'S') {
                auto p = getNum(i, 4);
                if (p.size() < 4) return;
                if (command == 's') {
                    p[0] += x;
                    p[1] += y;
                    p[2] += x;
                    p[3] += y;
                }
                if (toupper(prevCommand) == 'C' || toupper(prevCommand) == 'S')
                    bx = 2 * x - bx, by = 2 * y - by;
                else
                    bx = x, by = y;

                cubic(path, { x, y }, { bx, by }, { p[0], p[1] }, { p[2], p[3] });
                bx = p[0];
                by = p[1];
                x = p[2];
                y = p[3];
                path.push_back({ x, y });
            }
            else if (toupper(command) == 'Q') {
                auto p = getNum(i, 4);
                if (p.size() < 4) return;
                if (command == 'q') {
                    p[0] += x;
                    p[1] += y;
                    p[2] += x;
                    p[3] += y;
                }
                quadratic(path, { x, y }, { p[0], p[1] }, { p[2], p[3] });
                bx = p[0];
                by = p[1];
                x = p[2];
                y = p[3];
                path.push_back({ x, y });
            }
            else if (toupper(command) == 'T') {
                auto p = getNum(i, 2);
                if (p.size() < 2) return;
                if (command == 't') {
                    p[0] += x;
                    p[1] += y;
                }
                if (toupper(prevCommand) == 'Q' || toupper(prevCommand) == 'T')
                    bx = 2 * x - bx, by = 2 * y - by;
                else
                    bx = x, by = y;
                quadratic(path, { x, y }, { bx, by }, { p[0], p[1] });
                bx = x;
                by = y;
                x = p[0];
                y = p[1];
                path.push_back({ x, y });
            }
            else if (toupper(command) == 'A') {
                auto p = getNum(i, 7);
                if (p.size() < 7) return;
                if (command == 'a') {
                    p[5] += x;
                    p[6] += y;
                }

                arc(path, { x, y }, { p[5], p[6] }, p[0], p[1], p[2], p[3], p[4]);

                x = p[5];
                y = p[6];
                path.push_back({ x, y });
            }
            else ++i;
        }
        if (!path.empty())
            paths.push_back(path);
        
        for (auto &path : paths){
            if (path.empty()) return;
            for (auto &p : path) p /= ratio;
            path.push_back(path[0]);
        }

        const char *fillRule = ele->Attribute("fill-rule");
        int rule = 0;
        if (fillRule && strcmp(fillRule, "evenodd") == 0) rule = 1;
        glm::vec4 color = opaticalColor(ele, "fill");
        if (color.a > 0)
            drawFilled(color, paths, rule);
        
        for (auto &path : paths){
            color = opaticalColor(ele, "stroke");
            float width = ele->FloatAttribute("stroke-width", 1) / ratio / 2;
            if (color.a > 0){
                path.pop_back();
                drawOutline(color, path, width);
            }
        }
    }

    void drawPoint(glm::ivec2 p, glm::vec4 color) {
        p -= offset;

        if (p.x < 0 || p.y < 0 || p.x >= width || p.y >= height) return;
        std::array<size_t, 2> P = {(std::size_t)p.y, (std::size_t)p.x};
        if (!skip){
            auto && proxy = canvas->At(P[0], P[1]);
            proxy = glm::vec3(color) * color.a + static_cast<glm::vec3>(proxy) * (1.0f - color.a);
        }
    }

    void drawFilled(
        glm::vec4 const                             color,
        std::vector<std::vector<glm::vec2>> const & polygons,
        int                                         rule) {

        struct segment{
            glm::vec2 a, b;
            int k;
        };
        std::vector<segment> segments;
        for (auto const &polygon : polygons) {
            int n = polygon.size() - 1;
            for (int i = 0; i < n; ++i) {
                segment s;
                s.a = polygon[i];
                s.b = polygon[i+1];
                if (s.a.x > s.b.x) std::swap(s.a, s.b), s.k = -1;
                else s.k = 1;
                segments.push_back(s);
            }
        }
        int n = segments.size();
        std::vector<int> sortedL(n), sortedR(n);
        for (int i = 0; i < n; ++i) sortedL[i] = sortedR[i] = i;
        std::sort(sortedL.begin(), sortedL.end(), [&](int a, int b){
            return segments[a].a.x < segments[b].a.x;
        });
        std::sort(sortedR.begin(), sortedR.end(), [&](int a, int b){
            return segments[a].b.x < segments[b].b.x;
        });

        std::set<int> cur;
        int iL = 0, iR = 0;

        for (int x = offset.x; x < offset.x + width; ++x) {
            if (rule == 0){
                std::vector<std::pair<float, int>> ys;
                while (iL < n && segments[sortedL[iL]].a.x <= x)
                    cur.insert(sortedL[iL++]);
                while (iR < n && segments[sortedR[iR]].b.x <= x)
                    cur.erase(sortedR[iR++]);
                for (auto const &i : cur) {
                    segment const &s = segments[i];
                    float y = s.a.y + (s.b.y-s.a.y) * (x-s.a.x) / (s.b.x-s.a.x);
                    y = std::max(std::min(s.a.y, s.b.y), std::min(std::max(s.a.y, s.b.y), y));
                    ys.push_back({ y, s.k });
                }
                std::sort(ys.begin(), ys.end());
                for (int i = 0, t = 0; i < ys.size(); ++i){
                    t += ys[i].second;
                    if (t != 0)
                        for (int y = ceil(ys[i].first); y <= ys[i + 1].first; ++y)
                            drawPoint({ x, y }, color);
                }
            }
            else{
                std::vector<float> ys;
                while (iL < n && segments[sortedL[iL]].a.x <= x)
                    cur.insert(sortedL[iL++]);
                while (iR < n && segments[sortedR[iR]].b.x <= x)
                    cur.erase(sortedR[iR++]);
                for (auto const &i : cur) {
                    segment const &s = segments[i];
                    float y = s.a.y + (s.b.y-s.a.y) * (x-s.a.x) / (s.b.x-s.a.x);
                    y = std::max(std::min(s.a.y, s.b.y), std::min(std::max(s.a.y, s.b.y), y));
                    ys.push_back(y);
                }
                std::sort(ys.begin(), ys.end());
                for (int i = 0; i < ys.size(); i += 2)
                    for (int y = ceil(ys[i]); y <= ys[i + 1]; ++y)
                        drawPoint({ x, y }, color);
            }
        }
    }

    void drawOutline(
        glm::vec4 const  color,
        std::vector<glm::vec2> p,
        float            width) {
        
        if (p.size() < 2) return;

        if (p[0] == p.back()) p.push_back(p[1]);

        std::vector<glm::vec2> points;

        for (int _ = 0; _ < 2; ++_){
            glm::vec2 prev(0);
            bool first = true;
            for (int i = 0; i < p.size() - 1; ++i) if (glm::length(p[i+1]-p[i]) > 0.001f) {
                glm::vec2 normal = glm::normalize(glm::vec2(p[i+1].y - p[i].y, p[i].x - p[i+1].x));
                if (!first && ((p[i].x - prev.x) * (p[i+1].y - prev.y) - (p[i].y - prev.y) * (p[i+1].x - prev.x) > 0)) {
                    float alpha = acos(glm::dot(glm::normalize(p[i+1] - p[i]), glm::normalize(prev - p[i])));
                    if (alpha > asin(0.25) * 2)
                        points.push_back(p[i] + normal * width - glm::normalize(p[i+1] - p[i]) * width / tan(alpha / 2));
                }
                points.push_back(p[i] + normal * width);
                points.push_back(p[i+1] + normal * width);
                first = false;
                prev = p[i];
            }
            std::reverse(p.begin(), p.end());
        }
        points.push_back(points[0]);
        drawFilled(color, {points});
    }

    void drawRing(
        glm::vec4 const  color,
        glm::vec2 const  center,
        float            r1,
        float            r2) {
        
        for (int x = offset.x; x < offset.x + width; ++x) if (fabs(x - center.x) <= r1) {
            float y1 = center.y - sqrt(r1*r1 - (x-center.x)*(x-center.x));
            float y2 = center.y + sqrt(r1*r1 - (x-center.x)*(x-center.x));
            float y3 = y1, y4 = y1;
            if (fabs(x - center.x) <= r2) {
                y3 = center.y - sqrt(r2*r2 - (x-center.x)*(x-center.x));
                y4 = center.y + sqrt(r2*r2 - (x-center.x)*(x-center.x));
            }
            for (int y = y1; y < y3; ++y)
                drawPoint({ x, y }, color);
            for (int y = y4; y < y2; ++y)
                drawPoint({ x, y }, color);
        }
    }


    glm::vec2 Rotate(glm::vec2 p, float angle) {
        float c = cos(angle);
        float s = sin(angle);
        return {p.x * c - p.y * s, p.x * s + p.y * c};
    }

    void subdivide(glm::vec2 center, float l, float r, std::vector<glm::vec2> &p, float th){
        glm::vec2 p0 = {cos(l), sin(l)};
        glm::vec2 p1 = {cos(r), sin(r)};
        if (glm::length(p1 - p0) > th || fabs(r - l) > 1){
            float m = (l + r) * 0.5f;
            subdivide(center, l, m, p, th);
            subdivide(center, m, r, p, th);
        } else {
            p.push_back(center + p1);
        }
    }

    void arc(std::vector<glm::vec2> &points, glm::vec2 p0, glm::vec2 p1, float rx, float ry, float rotation, int largeArc, int sweep) {
        if (rx == 0 || ry == 0) return;
        rotation = glm::radians(rotation);
        p1 = Rotate(p1 - p0, -rotation);
        p1.x /= rx;
        p1.y /= ry;
        float xxyy = p1.x * p1.x + p1.y * p1.y;
        if (xxyy > 4) {
            float k = sqrt(xxyy) / 2;
            p1 /= k;
            rx *= k;
            ry *= k;
            xxyy = 4;
        }
        glm::vec2 center;
        center.x = (p1.x - p1.y * sqrt((4 - xxyy) * xxyy) / xxyy) / 2;
        center.y = (p1.y + p1.x * sqrt((4 - xxyy) * xxyy) / xxyy) / 2;
        if ((largeArc == 1) ^ (sweep == 1) ^ (center.x * p1.y - center.y * p1.x < 0))
            center = p1 - center;
        std::vector<glm::vec2> p;
        float l = atan2(-center.y, -center.x);
        float r = atan2(p1.y - center.y, p1.x - center.x);
        if (sweep == 0) std::swap(l, r);
        if (l > r) r += 2 * acos(-1);
        if (sweep == 0) std::swap(l, r);
        subdivide(center, l, r, p, ratio / (rx + ry));
        for (auto &v : p){
            v.x *= rx;
            v.y *= ry;
            v = Rotate(v, rotation) + p0;
            points.push_back(v);
        }
    }
}