#pragma once

#include <tinyxml2.h>
#include "Labs/Common/ImageRGB.h"

using VCX::Labs::Common::ImageRGB;

namespace VCX::Labs::Project {
    glm::vec3 parseHex(const char *s);
    glm::vec3 parseColor(const char *s);
    glm::vec4 opaticalColor(const tinyxml2::XMLElement *ele, const char *attr);
    std::vector<float> getNum(const char *&s, int n = -INT_MAX);
    std::vector<glm::vec2> getPoint(const char *s);

    void renderImage(ImageRGB &image, const tinyxml2::XMLElement *root, int &width, int &height, bool skipDrawing);
    void conditions(tinyxml2::XMLElement const *ele);

    void drawRect(const tinyxml2::XMLElement *ele);
    void drawPolygon(const tinyxml2::XMLElement *ele);
    void drawCircle(const tinyxml2::XMLElement *ele);
    void drawPath(const tinyxml2::XMLElement *ele);

    void drawFilled(glm::vec4, std::vector<std::vector<glm::vec2>> const &, int rule = 0);
    void drawOutline(glm::vec4, std::vector<glm::vec2>, float);
    void drawRing(glm::vec4, glm::vec2, float, float = 0);

    void cubic(std::vector<glm::vec2> &points, glm::vec2 p0, glm::vec2 p1, glm::vec2 p2, glm::vec2 p3);
    void quadratic(std::vector<glm::vec2> &points, glm::vec2 p0, glm::vec2 p1, glm::vec2 p2);

    void arc(std::vector<glm::vec2> &points, glm::vec2 p0, glm::vec2 p1, float rx, float ry, float rotation, int largeArc, int sweep);
}