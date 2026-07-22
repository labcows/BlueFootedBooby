#pragma once
#include "pch.h"

class Object;

class Hit
{
public:
	float distance = -1.0f; // ray -> hit
	math::vec3 point = math::vec3(0.0f);
	math::vec3 normal = math::vec3(0.0f);
	math::vec2 uv = math::vec2(0.0f);
	
	const Object* obj = nullptr;
};