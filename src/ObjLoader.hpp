#pragma once

#include <tiny_obj_loader.h>
#include "Render/Model.hpp"
#include "Render/Shape.hpp"
#include "Render/Materials/M_BlinnPhongMaterial.hpp" // TODO: 这里可以根据需要添加其他材质类型

using namespace aries::model;

class ObjLoader {
public:
    static sptr<Model> LoadModel(const string& filename);
private:
    static vector<Triangle> ParseMesh(const tinyobj::mesh_t& mesh, const tinyobj::attrib_t& attrib);
};