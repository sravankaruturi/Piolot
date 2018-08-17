﻿#pragma once
#include "Entity.h"
#include <memory>
#include "Object.h"

namespace piolot {

	struct TerrainVertexData{
		glm::vec3 position;
		glm::vec3 normal;
		glm::vec3 colour;
		glm::vec3 texCoord;
	};

	class MapTile {

	public:

		int tileIndexX;
		int tileIndexZ;

		float tilePosX;
		float tilePosZ;
		float tilePosY;


		/* Calculated Navigation Stuff */
		float navCost = 0.0f;
		bool navWalkable = true;
		MapTile * navNeighbours[8];
		int navNeighbourCount = 0;
		// Open Set?
		bool navOpen;
		// Closed Set?
		bool navClosed;
		MapTile * navParent = nullptr;
		float navFCost = 0;
		float navGCost = 0;
		int navTileSet;

	public:
		MapTile() = default;

	};

	class Terrain : public Entity
	{

		unsigned int length, breadth;

	public:
		unsigned GetLength() const
		{
			return length;
		}

		unsigned GetBreadth() const
		{
			return breadth;
		}

		float GetGridLength() const
		{
			return gridLength;
		}

		float GetGridBreadth() const
		{
			return gridBreadth;
		}

		unsigned GetNodeCountX() const
		{
			return nodeCountX;
		}

		unsigned GetNodeCountZ() const
		{
			return nodeCountZ;
		}

		MapTile** GetTiles() const
		{
			return tiles;
		}

		const std::string& GetHeightMapFile() const
		{
			return heightMapFile;
		}

		const std::vector<TerrainVertexData>& GetVertices() const
		{
			return vertices;
		}

		const std::vector<unsigned>& GetIndices() const
		{
			return indices;
		}

		const std::shared_ptr<Object>& GetObjectPtr() const
		{
			return objectPtr;
		}

	private:
		float gridLength, gridBreadth;

		unsigned int nodeCountX, nodeCountZ;

		MapTile ** tiles;

		std::string heightMapFile;

		std::vector<TerrainVertexData> vertices;

		bool areVerticesDirty = false;

		std::vector<unsigned int> indices;

		std::shared_ptr<Object> objectPtr;

		glm::vec3 ComputeGridNormal(int _x,int _z);

	public:

		Terrain(int _mapLength, int _mapBreadth, float _gridLength, float _gridBreadth, std::string _heightMapFile);

		void Render();

		void Update(float _delatTime, float _totalTime);

		float GetHeightAtPos(const float& _x, const float& _z);
		float GetHeightForNode(const int& _x, const int& _z);

		glm::vec2 GetNodeIndicesFromPos(const float& _x, const float& _z) const;

		void HighlightNode(const unsigned int _x, const unsigned int _z);

		void ClearColours();

		std::vector<MapTile *> GetPathFromTiles(MapTile * _startTile, MapTile * _endTile);
		std::vector<MapTile *> GetPathFromPositions(glm::vec3, glm::vec3);

		MapTile * GetTileFromIndices(int _x, int _y);

		void InitPathFinding();

		void FillNeighbours(MapTile& _tile);

		~Terrain();

	};


}
