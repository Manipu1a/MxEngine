//***************************************************************************************
// MathHelper.h by Frank Luna (C) 2011 All Rights Reserved.
//
// Helper math class.
//***************************************************************************************

#pragma once

#include <Windows.h>
#include <DirectXMath.h>
#include <cstdint>

class MathHelper
{
public:
	// Returns random float in [0, 1).
	static float RandF()
	{
		return (float)(rand()) / (float)RAND_MAX;
	}

	// Returns random float in [a, b).
	static float RandF(float a, float b)
	{
		return a + RandF()*(b-a);
	}

    static int Rand(int a, int b)
    {
        return a + rand() % ((b - a) + 1);
    }

	template<typename T>
	static T Min(const T& a, const T& b)
	{
		return a < b ? a : b;
	}

	template<typename T>
	static T Max(const T& a, const T& b)
	{
		return a > b ? a : b;
	}
	 
	template<typename T>
	static T Lerp(const T& a, const T& b, float t)
	{
		return a + (b-a)*t;
	}

	template<typename T>
	static T Clamp(const T& x, const T& low, const T& high)
	{
		return x < low ? low : (x > high ? high : x); 
	}

	// Returns the polar angle of the point (x,y) in [0, 2*PI).
	static float AngleFromXY(float x, float y);

	static DirectX::XMVECTOR SphericalToCartesian(float radius, float theta, float phi)
	{
		return DirectX::XMVectorSet(
			radius*sinf(phi)*cosf(theta),
			radius*cosf(phi),
			radius*sinf(phi)*sinf(theta),
			1.0f);
	}

    static DirectX::XMMATRIX InverseTranspose(DirectX::CXMMATRIX M)
	{
		// Inverse-transpose is just applied to normals.  So zero out 
		// translation row so that it doesn't get into our inverse-transpose
		// calculation--we don't want the inverse-transpose of the translation.
        DirectX::XMMATRIX A = M;
        A.r[3] = DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);

        DirectX::XMVECTOR det = DirectX::XMMatrixDeterminant(A);
        return DirectX::XMMatrixTranspose(DirectX::XMMatrixInverse(&det, A));
	}

    static DirectX::XMFLOAT4X4 Identity4x4()
    {
        static DirectX::XMFLOAT4X4 I(
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f);

        return I;
    }

    static DirectX::XMVECTOR RandUnitVec3();
    static DirectX::XMVECTOR RandHemisphereUnitVec3(DirectX::XMVECTOR n);

	static const float Infinity;
	static const float Pi;

	static DirectX::XMMATRIX Lookat(const DirectX::XMFLOAT3& pos, const DirectX::XMFLOAT3& target, const DirectX::XMFLOAT3& up)
	{
		DirectX::XMVECTOR P = DirectX::XMLoadFloat3(&pos);
		DirectX::XMVECTOR T = DirectX::XMLoadFloat3(&target);
		DirectX::XMVECTOR U = DirectX::XMLoadFloat3(&up);
		
		DirectX::FXMVECTOR Forward = DirectX::XMVector3Normalize(DirectX::XMVectorSubtract(T, P));
		DirectX::FXMVECTOR Right = DirectX::XMVector3Normalize(DirectX::XMVector3Cross(U, Forward));
		DirectX::FXMVECTOR Up = DirectX::XMVector3Cross(Forward, Right);

		DirectX::XMVECTOR Z = DirectX::XMVector3Normalize(DirectX::XMVectorSubtract(T, P));
		DirectX::XMVECTOR Y = U;
		DirectX::XMVECTOR X = DirectX::XMVector3Normalize(DirectX::XMVector3Cross(Y, Z));
		Y = DirectX::XMVector3Normalize(DirectX::XMVector3Cross(Z, X));

		DirectX::XMFLOAT4 W(-DirectX::XMVectorGetX(DirectX::XMVector3Dot(X, P)), -DirectX::XMVectorGetX(DirectX::XMVector3Dot(Y, P)), -DirectX::XMVectorGetX(DirectX::XMVector3Dot(Z, P)), 1.0);
		DirectX::XMVECTOR vW = DirectX::XMLoadFloat4(&W);
	
		DirectX::XMMATRIX lookatMatrix(X, Y, Z, vW);
	
		DirectX::XMMATRIX m(X.m128_f32[0], Y.m128_f32[0], Z.m128_f32[0], -DirectX::XMVectorGetX(DirectX::XMVector3Dot(X, P)),
			X.m128_f32[1], Y.m128_f32[1], Z.m128_f32[1], -DirectX::XMVectorGetX(DirectX::XMVector3Dot(Y, P)),
			X.m128_f32[2], Y.m128_f32[2], Z.m128_f32[2], -DirectX::XMVectorGetX(DirectX::XMVector3Dot(Z, P)),
			X.m128_f32[3], Y.m128_f32[3], Z.m128_f32[3], 1.0);

		return lookatMatrix;
	}

};

