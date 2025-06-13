// Apache 2.0 License
// Author: Christopher Diggins of Ara 3D Inc for Speckle Systems Ltd.
// This is a C++ wrapper around the Web-IFC component library by Tom van Diggelen and That Open Company 
// that is appropriate for use via PInvoke
// It is built based on the specific needs of a Speckle IFC import service:
// https://github.com/specklesystems/speckle-server/blob/main/packages/fileimport-service/ifc/parser_v2.js#L26
//
// And was inspired by:
// - https://github.com/ThatOpen/engine_web-ifc/blob/main/src/ts/web-ifc-api.ts
// - https://github.com/ThatOpen/engine_web-ifc/blob/main/src/cpp/web-ifc-wasm.cpp
// - https://github.com/ThatOpen/engine_web-ifc/blob/main/src/cpp/web-ifc-test.cpp

#include <string>
#include <algorithm>
#include <vector>
#include <cstdint>
#include <memory>
#include <fstream>

#include "../engine_web-ifc/src/cpp/web-ifc/modelmanager/ModelManager.h"

using namespace webifc::manager;
using namespace webifc::parsing;
using namespace webifc::geometry;
using namespace webifc::schema;

// Forward declarations of classes
class Model;
class Api;
class Mesh;
class Geometry;
class Vertex;

// Exposed C functions 
extern "C"
{
    __declspec(dllexport) Api* InitializeApi();
    __declspec(dllexport) void FinalizeApi(Api* api);
    __declspec(dllexport) Model* LoadModel(Api* api, const char* fileName);
    __declspec(dllexport) ::Geometry* GetGeometryFromId(Api* api, const Model* model, uint32_t id);
    __declspec(dllexport) int GetNumGeometries(Api* api, const Model* model);
    __declspec(dllexport) ::Geometry* GetGeometryFromIndex(Api* api, const Model* model, int32_t index);
    __declspec(dllexport) int GetNumMeshes(Api* api, const ::Geometry* geom);
    __declspec(dllexport) uint32_t GetGeometryId(Api* api, const ::Geometry* geom);
    __declspec(dllexport) Mesh* GetMesh(Api* api, const ::Geometry* geom, int index);
    __declspec(dllexport) uint32_t GetMeshId(Api* api, const ::Mesh* mesh);
    __declspec(dllexport) double* GetTransform(Api* api, Mesh* mesh);
    __declspec(dllexport) double* GetColor(Api* api, Mesh* mesh);
    __declspec(dllexport) int GetNumVertices(Api* api, const Mesh* mesh);
    __declspec(dllexport) Vertex* GetVertices(Api* api, const Mesh* mesh);
    __declspec(dllexport) int GetNumIndices(Api* api, const Mesh* mesh);
    __declspec(dllexport) uint32_t* GetIndices(Api* api, const Mesh* mesh);
	__declspec(dllexport) const char* GetGuid(const Model* model, const ::Geometry* geom);
	__declspec(dllexport) const char* GetEntityType(const Model* model, const ::Geometry* geom);
	__declspec(dllexport) uint32_t GetEntityTypeId(const Model* model, const ::Geometry* geom);
	__declspec(dllexport) void FreeString(const char* str);
}

// Vertex data structure as used by the web-IFC engine
struct Vertex 
{
    double Vx, Vy, Vz;
    double Nx, Ny, Nz;
};

// Color data
struct Color 
{
    double R, G, B, A;
    Color() : R(0), G(0), B(0), A(0) {}
    Color(double r, double g, double b, double a)
        : R(r), G(g), B(b), A(a) {}
};

struct Mesh 
{
    IfcGeometry* geometry = nullptr;
    Color color;
    uint32_t id;
	std::array<double, 16> transform = {};
    explicit Mesh(uint32_t meshId) 
        : id(meshId)
    { }
};

struct Geometry 
{
    uint32_t id;
    IfcFlatMesh* flatMesh = nullptr;
    std::vector<Mesh*> meshes;    
    explicit Geometry(uint32_t geoId)
        : id(geoId)
    {}
};

// Model class, abstraction over the web-IFC engine concept of Model ID
struct Model
{
    uint32_t id;
    IfcLoader* loader;
    IfcGeometryProcessor* geometryProcessor;
	IfcSchemaManager* schemaManager;
    std::vector<::Geometry*> geometryList;
    std::unordered_map<uint32_t, ::Geometry*> geometries;

    Model(IfcSchemaManager* schemas, IfcLoader* loader, IfcGeometryProcessor* processor, uint32_t modelId)
        : id(modelId), loader(loader), geometryProcessor(processor), schemaManager(schemas)
    {
		for (auto type : schemas->GetIfcElementList())
		{
			//// TODO: maybe some of these elments are desired. In fact, I think there may be explicit requests for IFCSPACE?
			//if (type == IFCOPENINGELEMENT
			//	|| type == IFCSPACE
			//	|| type == IFCOPENINGSTANDARDCASE)
			//{
			//	continue;
			//}

			for (auto eId : loader->GetExpressIDsWithType(type))
			{
				const auto& flatMesh = geometryProcessor->GetFlatMesh(eId);
				const auto& g = new ::Geometry(eId);
				for (const auto& placedGeom : flatMesh.geometries)
				{
					auto mesh = ToMesh(placedGeom);
					g->meshes.push_back(mesh);
				}
				geometries[eId] = g;
				geometryList.push_back(g);
			}
		}
    }

    ::Geometry* GetGeometry(uint32_t geoId) const
    {
        auto it = geometries.find(geoId);
        if (it == geometries.end())
            return nullptr;
        return it->second;
    }

	Mesh* ToMesh(const IfcPlacedGeometry& pg) const
	{
		auto r = new Mesh(pg.geometryExpressID);
		r->color = Color(pg.color.r, pg.color.g, pg.color.b, pg.color.a);
		r->geometry = &(geometryProcessor->GetGeometry(pg.geometryExpressID));
		r->transform = pg.flatTransformation;
		return r;
	}

	const char* GetGuid(const ::Geometry* geom) const
	{
		const uint32_t& geoExpressId = geom->id;

		if (!loader->IsValidExpressID(geoExpressId))
		{
			return nullptr;
		}

		// The GUID is normally the first argument of entities
		loader->MoveToLineArgument(geoExpressId, 0);
		const std::string& guid = std::string(loader->GetDecodedStringArgument());

		auto result = (char*)malloc(guid.size() + 1);

		if (result)
		{
			std::memcpy(result, guid.c_str(), guid.size() + 1);
		}

		return result;
	}

	const char* GetEntityType(const ::Geometry* geom) const
	{
		if (schemaManager == nullptr)
		{
			return nullptr;
		}

		const uint32_t& geoExpressId = geom->id;
		const uint32_t& lineType = loader->GetLineType(geoExpressId);

		if (lineType == 0)
		{
			return nullptr;
		}

		const std::string& ifcProduct = schemaManager->IfcTypeCodeToType(lineType);
		auto result = (char*)malloc(ifcProduct.size() + 1);

		if (result)
		{
			std::memcpy(result, ifcProduct.c_str(), ifcProduct.size() + 1);
		}

		return result;
	}

	uint32_t GetEntityTypeId(const ::Geometry* geom) const
	{
		if (schemaManager == nullptr)
		{
			return 0;
		}

		const uint32_t& geoExpressId = geom->id;

		if (!loader->IsValidExpressID(geoExpressId))
		{
			return 0;
		}

		return loader->GetLineType(geoExpressId);
	}
};

struct Api 
{
    ModelManager* manager = new ModelManager(false);
    IfcSchemaManager* schemaManager = new IfcSchemaManager();
    LoaderSettings* settings = new webifc::manager::LoaderSettings();

    Api() 
    {
        manager->SetLogLevel(6); // Turns off logging
    }   

    Model* LoadModel(const char* fileName)
    {
        auto modelId = manager->CreateModel(*settings);
        auto loader = manager->GetIfcLoader(modelId);
        std::ifstream ifs;
        // NOTE: may fail if the file has unicode characters. This needs to be tested  
        ifs.open(fileName, std::ifstream::in);
        loader->LoadFile(ifs);
        return new ::Model(schemaManager, loader, manager->GetGeometryProcessor(modelId), modelId);
    }
};

//==

Api* InitializeApi() {
    return new Api();
}

void FinalizeApi(Api* api) {
    delete api->manager;
    delete api->schemaManager;
    delete api->settings;
    delete api;
}

Model* LoadModel(Api* api, const char* fileName) {
    return api->LoadModel(fileName);
}

double* GetTransform(Api* api, Mesh* mesh) {
    return mesh->transform.data();
}

double* GetColor(Api* api, Mesh* mesh) {
    return &mesh->color.R;
}

::Geometry* GetGeometryFromId(Api* api, const Model* model, uint32_t id) {
    return model->GetGeometry(id);
}

int GetNumGeometries(Api* api, const Model* model) {
    return model->geometries.size();
}

::Geometry* GetGeometryFromIndex(Api* api, const Model* model, int32_t index) {
    return model->geometryList[index];
}

uint32_t GetGeometryId(Api* api, const ::Geometry* geom) {
    return geom->id;
}

int GetNumMeshes(Api* api, const ::Geometry* geom) {
    return geom->meshes.size();
}

Mesh* GetMesh(Api* api, const ::Geometry* geom, int index) {
    return geom->meshes[index];
}

uint32_t GetMeshId(Api* api, const ::Mesh* mesh) {
    return mesh->id;
}

int GetNumVertices(Api* api, const Mesh* mesh) {
    return mesh->geometry->vertexData.size() / 6;
}

Vertex* GetVertices(Api* api, const Mesh* mesh) {
    return reinterpret_cast<Vertex*>(mesh->geometry->vertexData.data());
}

int GetNumIndices(Api* api, const Mesh* mesh) {
    return mesh->geometry->indexData.size();
}

uint32_t* GetIndices(Api* api, const Mesh* mesh) {
    return mesh->geometry->indexData.data();
}

const char* GetGuid(const Model* model, const ::Geometry* geom)
{
	return model->GetGuid(geom);
}

const char* GetEntityType(const Model* model, const ::Geometry* geom)
{
	return model->GetEntityType(geom);
}

uint32_t GetEntityTypeId(const Model* model, const ::Geometry* geom)
{
	return model->GetEntityTypeId(geom);
}

void FreeString(const char* str)
{
	free((void*)str);
}