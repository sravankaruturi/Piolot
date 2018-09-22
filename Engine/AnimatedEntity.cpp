﻿#include "AnimatedEntity.h"
#include "AssetManager.h"
#include "Object.h"

namespace piolot {



	AnimatedEntity::AnimatedEntity(const std::string & _entityName, const std::string & _objectPath, const std::string & _shaderName)
		: Entity(_entityName, _objectPath, _shaderName)
	{

		//matrixDirty = true;
		for (int i = 0; i < 32; i++) {
			boneMatrices.push_back(glm::mat4(1.0f));
		}

	}

	void AnimatedEntity::PlayAnimation(float _deltaTime)
	{

		// Play the Animation.

		// Update the Bone Matrices.

		// Update the TotalTime.

		animationTotalTime += _deltaTime;

		ASMGR.objects.at(objectName)->BoneTransform(animationTotalTime, this->boneMatrices);


	}

	void AnimatedEntity::Update(float _deltaTime)
	{
		Entity::Update(_deltaTime);
	}

	void AnimatedEntity::Render()
	{

		ASMGR.shaders.at(shaderName)->use();

		auto loc = ASMGR.shaders.at(shaderName)->GetUniformLocation("u_BoneMatrices");

		PE_GL(glUniformMatrix4fv(loc, boneMatrices.size(), GL_FALSE, &boneMatrices[0][0][0]));

		Entity::Render();

	}
}
