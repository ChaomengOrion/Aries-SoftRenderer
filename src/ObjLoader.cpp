#define TINYOBJLOADER_IMPLEMENTATION

#include "ObjLoader.hpp"
#include <iostream>
#include <filesystem>
#include "Render/TextureManager.hpp"

using namespace tinyobj;
using namespace aries::material;

sptr<Model> ObjLoader::LoadModel(const string& filename) {
    attrib_t attrib;
    vector<shape_t> shapes;
    vector<material_t> materials;
    vector<sptr<IMaterial>> materialPtrs; // 用于存储材质指针

    string err;

    // 计算 obj 文件所在目录，用作 mtl 搜索路径
    string base_dir = std::filesystem::path(filename).parent_path().string();
    if (!base_dir.empty() && base_dir.back()!='/' && base_dir.back()!='\\')
        base_dir += "/";

    bool ret = tinyobj::LoadObj(
        &attrib, &shapes, &materials, &err,
        filename.c_str(),
        base_dir.c_str()   // 这里传入 base_dir，让 tinyobj 去这个目录下找 .mtl
    );

    if (!err.empty()) {
        std::cerr << "[ObjLoader]: 加载中发生异常：\n" << err << std::endl;
    }

    if (!ret) {
        throw std::runtime_error("Failed to load OBJ file: " + filename);
    }

    // 访问并打印 map_Kd (即 diffuse_texname)
    for (const auto& mat : materials) {
        if (!mat.diffuse_texname.empty()) {
            // 替换字符串中所有空格为下划线
            std::string diffuse_name = mat.diffuse_texname;
            std::replace(diffuse_name.begin(), diffuse_name.end(), ' ', '_'); 
            string texPath = base_dir + diffuse_name; // 这里把空格替换为下划线
            std::cout << "[ObjLoader] 材质 “" << mat.name << "” 的 map_Kd = " << texPath << std::endl;

            // 尝试加载纹理
            sptr<Texture> texture = nullptr;

            try {
                texture = TextureManager::GetInstance().LoadTexture(texPath);
            } catch (const std::exception& e) {
                std::cerr << "[ObjLoader] 加载纹理失败: " << e.what() << std::endl;
            }

            if (texture) {
                materialPtrs.push_back(std::make_shared<BlinnPhongMaterial>(mat.name, texture)); // 创建材质球并存储
                std::cout << "[ObjLoader] 材质 “" << mat.name << "” 的纹理加载成功: " << texPath << std::endl;
            } else {
                materialPtrs.push_back(std::make_shared<BlinnPhongMaterial>(mat.name, nullptr)); // 如果加载失败，使用空纹理
            }

            //!materialPtrs.back()->shader = std::make_shared<BlinnPhongShader>(); // 设置材质的着色器
        }
    }

    vector<sptr<Shape>> outShapes;

    for (const shape_t& shape : shapes) {
        sptr<Shape> out = std::make_shared<Shape>(); // 创建一个新的Object实例
        out->name = shape.name; // 设置名称
        out->mesh = ParseMesh(shape.mesh, attrib); // 解析 mesh 并转换为 Triangle 数组

        if (shape.mesh.material_ids.size() > 0) {
            int matId = shape.mesh.material_ids[0]; //! 获取第一个材质ID
            if (matId >= 0 && matId < materials.size()) {
                std::cout << "[ObjLoader] 使用材质: " << materials[matId].name << std::endl;
                out->material = materialPtrs[matId]; // 设置材质球
            } else {
                std::cerr << "[ObjLoader] 无效的材质ID: " << matId << "，使用默认材质球" << std::endl;
                // TODO: out->material = std::make_shared<Material>("DefaultMaterial"); // 使用默认材质
                //!out->material->shader = std::make_shared<PreviewShader>(); // 设置默认材质的着色器
            }
        } else {
            throw std::runtime_error("Shape " + shape.name + " has no material assigned."); // TODO: 这里可以考虑使用默认材质
        }
        
        std::cout << "[ObjLoader] 已加载形状: " << out->name << ", 顶点数: " << shape.mesh.indices.size() << std::endl;
        outShapes.push_back(std::move(out)); // 将对象添加到列表中
    }

    return std::make_shared<Model>(std::move(filename), std::move(outShapes));
}

// 解析tinyobj::mesh_t并转换为我们的Triangle数组对象
vector<Triangle> ObjLoader::ParseMesh(const mesh_t& mesh, const attrib_t& attrib) {
    vector<Triangle> triangles;

    for (size_t f = 0; f < mesh.num_face_vertices.size(); f++) {
        uint8_t fv = mesh.num_face_vertices[f];

        if (fv != 3) {
            std::cerr << "[ObjLoader] 仅支持三角面组成的模型，不支持顶点数超过3的面" << std::endl;
            continue;
        }

        // 获取顶点、法线和UV坐标
        std::array<Vector3f, 3> vertices, colors, normals;
        std::array<Vector2f, 3> uvs;

        for (int v = 0; v < fv; v++) {
            int idx = mesh.indices[f * 3 + v].vertex_index;
            vertices[v] = Vector3f(attrib.vertices[3 * idx], attrib.vertices[3 * idx + 1], attrib.vertices[3 * idx + 2]);

            if (!attrib.normals.empty()) {
                int n_idx = mesh.indices[f * 3 + v].normal_index;
                normals[v] = Vector3f(attrib.normals[3 * n_idx], attrib.normals[3 * n_idx + 1], attrib.normals[3 * n_idx + 2]);
            } else {
                normals[v] = Vector3f(0, 0, 1); // 默认法线
            }

            if (!attrib.texcoords.empty()) {
                int t_idx = mesh.indices[f * 3 + v].texcoord_index;
                uvs[v] = Vector2f(attrib.texcoords[2 * t_idx], attrib.texcoords[2 * t_idx + 1]);
            } else {
                uvs[v] = Vector2f(0, 0); // 默认UV坐标
            }
        }

        // 创建三角形对象
        triangles.emplace_back(vertices, normals, uvs);
    }

    return triangles;
}