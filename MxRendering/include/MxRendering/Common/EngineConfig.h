#pragma once

enum class RenderLayer : int
{
	Opaque = 0,
	Alpha = 1,
	Mirrors = 2,
	Transparent = 3,
	Sky = 4,
	Debug = 5,
	Irradiance = 6,
	DeferredGeo = 7,
	DeferredLight = 8,
	Count
};
