﻿#include "Terrain.h"
#include "external_files/stb/stb_image.h"
#include "Colours.h"
#include "LoggingManager.h"
#include "AssetManager.h"

#include <algorithm>

#define UNPASSABLE_NAV_COST_LIMIT		1.0f

namespace piolot {

	glm::vec3 Terrain::ComputeGridNormal(const int _x, const int _z)
	{

		if (_x > nodeCountX || _z > nodeCountZ || _x < 0 || _z < 0)
		{
			LOGGER.AddToLog("Out of Bounds error when generating normals for Terrain", PE_LOG_ERROR);
		}

		// If X and Z are the co ordinates. Get the Four surrounding Vertices it would be connected to.

		/*		B
		*		|
		*C --- O ---- A
		*		|
		*		D
		*
		*		O is the current Vertex. A - D are the surrounding vertices. We compute the normals by computing the cross product for,
		*		OA * OB
		*		OB * OC
		*		OC * OD
		*		OD * OA
		*
		*		And averaging it.
		*/

		glm::vec3 o_position = vertices[_x * nodeCountZ + _z].position;

		glm::vec3 a_position = vertices[((_x + 1 < nodeCountX) ? _x + 1 : _x) * nodeCountZ + _z].position;
		auto tester_temp = _x * nodeCountZ + (_z - 1 > nodeCountZ ? _z : _z - 1);
		glm::vec3 b_position = vertices[_x * nodeCountZ + ((_z - 1 < 0) ? _z : _z - 1)].position;
		glm::vec3 c_position = vertices[((_x - 1 < 0) ? _x : _x - 1) * nodeCountZ + _z].position;
		glm::vec3 d_position = vertices[_x * nodeCountZ + ((_z + 1 < nodeCountZ) ? _z + 1 : _z)].position;

		glm::vec3 oa = a_position - o_position;
		glm::vec3 ob = b_position - o_position;
		glm::vec3 oc = c_position - o_position;
		glm::vec3 od = d_position - o_position;

		glm::vec3 normal = glm::normalize(
			glm::normalize(glm::cross(oa, ob)) +
			glm::normalize(glm::cross(ob, oc)) +
			glm::normalize(glm::cross(oc, od)) +
			glm::normalize(glm::cross(od, oa))
		);

		return normal;

	}

	Terrain::Terrain(int _mapLength, int _mapBreadth, float _gridLength, float _gridBreadth, std::string _heightMapFile)
		: length(_mapLength), breadth(_mapBreadth), gridLength(_gridLength), gridBreadth(_gridBreadth), heightMapFile(_heightMapFile)
	{

		nodeCountX = (length / gridLength) + 1;
		nodeCountZ = (breadth / gridBreadth) + 1;

		tiles = new MapTile*[nodeCountX];

		for (auto i = 0; i < nodeCountX; i++) {
			tiles[i] = new MapTile[nodeCountZ];
		}

		// Load the Image.
		stbi_set_flip_vertically_on_load(true);
		int image_width, image_height, nr_channels;

		auto data = stbi_load(_heightMapFile.c_str(), &image_width, &image_height, &nr_channels, 0);

		if (NULL == data) {
			LOGGER.AddToLog("Unable to load " + _heightMapFile, PE_LOG_ERROR);
		}

		for (auto i = 0; i < nodeCountX; i++)
		{
			for (auto j = 0; j < nodeCountZ; j++)
			{
				auto image_x = int(i * image_width / nodeCountX);
				auto image_z = int(j * image_height / nodeCountZ);

				float total = 0;

				auto channels_without_alpha = (nr_channels == 2 || nr_channels == 4) ? (nr_channels - 1) : nr_channels;

				// Do not count the Alpha
				for (auto it = 0; it < channels_without_alpha; it++)
				{
					total += int(data[((image_x + (image_z * image_width)) * nr_channels)] + it);
				}

				// To make it so that, the blacks are higher and the whites are lower, we subtract the values from pure white.
				total = ((255.0f * channels_without_alpha) - total) / (channels_without_alpha * 255.0f);

				tiles[i][j].tileIndexX = i;
				tiles[i][j].tileIndexZ = j;

				tiles[i][j].tilePosX = i * gridLength + gridLength / 2;
				tiles[i][j].tilePosZ = j * gridBreadth + gridBreadth / 2;

				tiles[i][j].tilePosY = total;
			}
		}

		this->vertices.resize(nodeCountX * nodeCountZ);

		for (auto i = 0; i < nodeCountX; i++) {
			for (auto j = 0; j < nodeCountZ; j++) {
				vertices[i * nodeCountZ + j] = TerrainVertexData();
				vertices[i * nodeCountZ + j].position = glm::vec3(tiles[i][j].tilePosX, tiles[i][j].tilePosY, tiles[i][j].tilePosZ);
				vertices[i * nodeCountZ + j].normal = glm::vec3();
				vertices[i * nodeCountZ + j].colour = green;
				vertices[i * nodeCountZ + j].texCoord = glm::vec3(i * 0.4, j * 0.4, 0);
			}
		}

		for (auto i = 0; i < nodeCountX - 1; i++)
		{
			for (auto j = 0; j < nodeCountZ - 1; j++)
			{
				indices.push_back(i * (nodeCountZ)+j);
				indices.push_back(i * (nodeCountZ)+j + 1);
				indices.push_back((i + 1) * (nodeCountZ)+j);

				indices.push_back((i + 1)* (nodeCountZ)+j);
				indices.push_back(i * (nodeCountZ)+j + 1);
				indices.push_back((i + 1) * (nodeCountZ)+j + 1);
			}
		}

		for (auto i = 0; i < nodeCountX; i++)
		{
			for (auto j = 0; j < nodeCountZ; j++) {

				vertices[i * nodeCountZ + j].normal = ComputeGridNormal(i, j);

			}
		}

		auto mesh = std::make_shared<Mesh>(&vertices[0], sizeof(TerrainVertexData), vertices.size(), indices);
		mesh->SetTextureNames(std::vector<std::string>{"grass"});

		objectPtr = std::make_shared<Object>("terrain", std::vector<std::shared_ptr<Mesh>>{mesh});

		if (!ASMGR.AddToObjects("terrain", objectPtr)) {
			LOGGER.AddToLog("Cannot load terrain into AssetManager", PE_LOG_ERROR);
		}

		this->objectName = "terrain";
		this->shaderName = "terrain";

		// Load Stuff for PathFinding
		InitPathFinding();

	}

	void Terrain::Render()
	{
		Entity::Render();
	}

	void Terrain::Update(float _delatTime, float _totalTime)
	{
		Entity::Update(_delatTime);

		if (this->areVerticesDirty) {
			this->objectPtr->GetMeshes()[0]->UpdateVertices(&vertices[0], sizeof(TerrainVertexData), vertices.size());
			areVerticesDirty = false;
		}

	}

	float Terrain::GetHeightAtPos(const float& _x, const float& _z)
	{
		auto temp = GetNodeIndicesFromPos(_x, _z);
		return GetHeightForNode(temp.x, temp.y);
	}

	float Terrain::GetHeightForNode(const int& _x, const int& _z)
	{
		return tiles[_x][_z].tilePosY;
	}

	glm::vec2 Terrain::GetNodeIndicesFromPos(const float& _x, const float& _z) const
	{
		return glm::vec2(glm::min(int(_x / gridLength), int(nodeCountX - 1)), glm::min(int(_z / gridBreadth), int(nodeCountZ - 1)));
	}

	void Terrain::HighlightNode(const unsigned _x, const unsigned _z)
	{

		this->vertices[_x * nodeCountZ + _z].colour = yellow;
		this->vertices[_x * nodeCountZ + _z].texCoord.z = 1.0f;
		areVerticesDirty = true;

	}

	void Terrain::ClearColours()
	{

		auto all_tile_sets = GetAllTileSets();
		auto number_tile_sets = all_tile_sets.size();

		for (auto i = 0; i < nodeCountX; i++) {
			for (auto j = 0; j < nodeCountZ; j++) {

				int index = std::distance( all_tile_sets.begin(), std::find(all_tile_sets.begin(), all_tile_sets.end(), tiles[i][j].navTileSet));

				this->vertices[i * nodeCountZ + j].colour = red * (float(index) / number_tile_sets);
				this->vertices[i * nodeCountZ + j].texCoord.z = 0.0f;
			}
		}
		areVerticesDirty = true;

	}

	float HCost(MapTile * _pointA, MapTile * _pointB)
	{
		return abs(_pointA->tilePosX - _pointB->tilePosX) + abs(_pointA->tilePosZ - _pointB->tilePosZ);
	}

	std::vector<MapTile *> Terrain::GetPathFromTiles(MapTile * _startTile, MapTile * _endTile)
	{
		std::vector<MapTile*> return_vector;

		if ( _startTile->navTileSet != _endTile->navTileSet)
		{
			// They are not in the same tile set. Return the empty stuff.
			return return_vector;
		}

		for ( auto i = 0 ; i < nodeCountX ; i++)
		{
			for ( auto j = 0 ; j < nodeCountZ; j++)
			{
				tiles[i][j].navFCost = tiles[i][j].navGCost = INT_MAX;
				tiles[i][j].navOpen = tiles[i][j].navClosed = false;
			}
		}

		std::vector<MapTile *> open_set;

		_startTile->navGCost = 0.0f;
		_startTile->navFCost = HCost(_startTile, _endTile);
		_startTile->navOpen = true;

		open_set.push_back(_startTile);

		auto active_node = open_set[0];

		bool path_found = false;

		while ( !path_found && !open_set.empty())
		{
			int best_node_index = 0;
			active_node = open_set[best_node_index];
			for ( auto index = 0; index < open_set.size(); index++ )
			{
				if ( open_set[index]->navFCost < active_node->navFCost)
				{
					active_node = open_set[index];
					best_node_index = index;
				}
			}

			if (nullptr == active_node)
			{
				// This means there is no path.
				break;
			}

			active_node->navOpen = false;
			open_set.erase(open_set.begin() + best_node_index);

			// TODO: Can we just check if they point to the same tile, since they are pointers..
			if (_endTile->tileIndexX == active_node->tileIndexX && _endTile->tileIndexZ == active_node->tileIndexZ)
			{
				// We have reached the target. Retrace our Path.
				std::vector<MapTile *> temp_path;

				auto pathing_current_node = active_node;

				while ( _startTile != pathing_current_node )
				{
					temp_path.push_back(pathing_current_node);
					pathing_current_node = pathing_current_node->navParent;
				}

				for (auto i : temp_path)
				{
					return_vector.push_back(i);
				}

				return return_vector;

			}else
			{
				for (auto i = 0 ; i < active_node->navNeighbourCount ; i++)
				{
					if ( nullptr != active_node->navNeighbours[i])
					{
						const auto new_g = active_node->navGCost + active_node->navCost;
						const auto new_f = new_g + HCost(active_node->navNeighbours[i], _endTile);
						
						// Check if B is in the open list or closed list.
						if ( active_node->navNeighbours[i]->navOpen || active_node->navNeighbours[i]->navClosed)
						{
							if (new_f < active_node->navNeighbours[i]->navFCost)
							{
								active_node->navNeighbours[i]->navGCost = new_g;
								active_node->navNeighbours[i]->navFCost = new_f;
								active_node->navNeighbours[i]->navParent = active_node;
							}
						}else
						{
							// If it is not in Open or Closed Sets, Add it to the open list.
							active_node->navNeighbours[i]->navGCost = new_g;
							active_node->navNeighbours[i]->navFCost = new_f;
							active_node->navNeighbours[i]->navParent = active_node;
							active_node->navNeighbours[i]->navOpen = true;
							open_set.push_back(active_node->navNeighbours[i]);
						}

					}
				}
				active_node->navClosed = true;
			}
			
		}

		// TODO: Further reading is required.

		// Set the active node as the tile with least F value, in the Openset.
		// Remove the Active node from the Open set  ( and add it to the closed set.?? )
		// Calculate the new G, value of the Neighbours as  Neighbour's G = A's G value + A's Cost.

		// For each neighbour, B of A,
		//		Check if B is in the Open List or Closed List.
		//		1. If it is not in Open or Closed List, set B's G value to the Computed value. and the F Value = new G + H(B, EndNode), Add it to the OpenList, Set its parent pointer to A.
		//		2. If it is in one of the Open or Closed Lists, Check if the current G value of B node is higher than the newly computed one., if it is update the Value and set the Parent to A.
		// Select a new Active node A as the tile with least F Valueu in the Openset and repeat unitl A is the goal node.
		// If the open list turns up empty, then there is no path.
		// Build the path by traversing the parent pointer of the Goal Node to Start and then reverse it.

		return return_vector;
	}

	std::vector<MapTile *> Terrain::GetPathFromPositions(glm::vec3 _startPosition, glm::vec3 _endPosition)
	{

		// Get the Nodes from the Position.
		auto start_node_indices = GetNodeIndicesFromPos(_startPosition.x, _startPosition.z);
		auto end_node_indices = GetNodeIndicesFromPos(_endPosition.x, _endPosition.z);

		return GetPathFromTiles(
			GetTileFromIndices(start_node_indices.x, start_node_indices.y),
			GetTileFromIndices(end_node_indices.x, end_node_indices.y)
		);



	}

	MapTile* Terrain::GetTileFromIndices(int _x, int _y)
	{
		return &tiles[_x][_y];
	}

	void Terrain::InitPathFinding()
	{

		for (auto i = 0; i < nodeCountX; i++) {
			for (auto j = 0; j < nodeCountZ; j++) {

				auto& tile = tiles[i][j];

				FillNeighbours(tile);

				// For each neighbour, add to the cost.
				auto variance = 0.0f;

				for (auto k = 0; k < tile.navNeighbourCount; k++){

					const auto temp = tile.navNeighbours[k]->tilePosY;
					variance += temp * temp;

				}

				variance /= float(tile.navNeighbourCount);

				tile.navCost = (( variance > 0.9f ) ? (0.9f) : variance) + 0.1f;
				tile.navWalkable = (tile.navCost < UNPASSABLE_NAV_COST_LIMIT );
				tile.navTileSet = i * nodeCountZ + j;

			}
		}

		// Update the Tilsets based on whether you can walk or not.
		bool changed = false;
		do {
			changed = false;

			for (auto i = 0; i < nodeCountX; i++) {
				for (auto j = 0; j < nodeCountZ; j++) {

					auto&tile = tiles[i][j];
					if (tile.navWalkable) {
						for (auto k = 0; k < tile.navNeighbourCount; k++) {
							const auto neighbour = tile.navNeighbours[k];
							if (neighbour->navWalkable && neighbour->navTileSet < tile.navTileSet) {
								changed = true;
								tile.navTileSet = neighbour->navTileSet;
							}
						}
					}

				}
			}
		} while (changed);

		// Log all the tilesets.
		std::vector<int> tile_sets = GetAllTileSets();

		std::string log_temp = "Available tilesets left: ";
		for ( auto it: tile_sets)
		{
			log_temp += " " + std::to_string(it) + ",";
		}

		LOGGER.AddToLog(log_temp, PE_LOG_INFO);

	}

	void Terrain::FillNeighbours(MapTile& _tile)
	{

		// We Update the Tile Itself.
		const auto& temp_x = _tile.tileIndexX;
		const auto& temp_z = _tile.tileIndexZ;

		/* TODO: Too many branches... Do something later on.*/
		if (temp_z + 1 < nodeCountZ)
		{
			_tile.navNeighbours[_tile.navNeighbourCount] = &tiles[temp_x][temp_z + 1];
			_tile.navNeighbourCount++;
		}

		if (temp_z - 1 > 0)
		{
			_tile.navNeighbours[_tile.navNeighbourCount] = &tiles[temp_x][temp_z - 1];
			_tile.navNeighbourCount++;
		}

		if (temp_x + 1 < nodeCountX)
		{

			_tile.navNeighbours[_tile.navNeighbourCount] = &tiles[temp_x + 1][temp_z];
			_tile.navNeighbourCount++;

			if (temp_z + 1 < nodeCountZ)
			{
				_tile.navNeighbours[_tile.navNeighbourCount] = &tiles[temp_x + 1][temp_z + 1];
				_tile.navNeighbourCount++;
			}

			if (temp_z - 1 > 0)
			{
				_tile.navNeighbours[_tile.navNeighbourCount] = &tiles[temp_x + 1][temp_z - 1];
				_tile.navNeighbourCount++;
			}
		}

		if (temp_x - 1 > 0)
		{
			_tile.navNeighbours[_tile.navNeighbourCount] = &tiles[temp_x - 1][temp_z];
			_tile.navNeighbourCount++;

			if (temp_z + 1 < nodeCountZ)
			{
				_tile.navNeighbours[_tile.navNeighbourCount] = &tiles[temp_x - 1][temp_z + 1];
				_tile.navNeighbourCount++;
			}

			if (temp_z - 1 > 0)
			{
				_tile.navNeighbours[_tile.navNeighbourCount] = &tiles[temp_x - 1][temp_z - 1];
				_tile.navNeighbourCount++;
			}
		}
	}

	void Terrain::OnImguiRender()
	{



	}

	int Terrain::GetNodeSetFromPos(float _x, float _z)
	{

		auto start_node_indices = GetNodeIndicesFromPos(_x, _z);
		auto tile = GetTileFromIndices(start_node_indices.x, start_node_indices.y);
		return tile->navTileSet;

	}

	std::vector<int> Terrain::GetAllTileSets()
	{
		// Log all the tilesets.
		std::vector<int> tile_sets;
		for (auto i = 0; i < nodeCountX; i++) {
			for (auto j = 0; j < nodeCountZ; j++) {

				const auto& tile = tiles[i][j];

				// If you cannot find it, add.
				if (std::find(tile_sets.begin(), tile_sets.end(), tile.navTileSet) == tile_sets.end()) {
					tile_sets.push_back(tile.navTileSet);
				}

			}
		}
		return tile_sets;
	}

	Terrain::~Terrain()
	{
		for (auto i = 0; i < nodeCountX; i++) {
			delete[] tiles[i];
		}
		
		delete[] tiles;
	}


}
