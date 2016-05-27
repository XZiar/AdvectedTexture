#pragma once

#include "3deRely.h"

namespace b3d
{
using std::string;


class Coord2D
{
public:
	float u, v;
	Coord2D() { u = v = 0.0; };
	Coord2D(const float &iu, const float &iv) :u(iu), v(iv) { };

	Coord2D operator+(const Coord2D &c) const;
	Coord2D operator*(const float &n) const;
	operator float*() { return &u; };
};

_MM_ALIGN16 class Vertex
{
public:
	union
	{
		__m128 dat;
		struct
		{
			float x, y, z, w;
		};
		struct
		{
			float r, g, b, alpha;
		};
	};
	Vertex();
	Vertex(const __m128 &idat);
	Vertex(const float ix, const float iy, const float iz, const float ia = 0) :x(ix), y(iy), z(iz), w(ia) { };
	operator float*() const { return (float *)&x; };
	operator __m128() const { return dat; };
	operator glm::tvec3<float, glm::highp>() const { return glm::vec3(x, y, z); };

	float length() const;
	float length_sqr() const;
	Vertex muladd(const float &n, const Vertex &v) const;
	Vertex mixmul(const Vertex &v) const;

	Vertex operator+(const Vertex &v) const;
	Vertex &operator+=(const Vertex &right);
	Vertex operator-(const Vertex &v) const;
	Vertex &operator-=(const Vertex &right);
	Vertex operator/(const float &n) const;
	Vertex &operator/=(const float &right);
	Vertex operator*(const float &n) const;
	Vertex &operator*=(const float &right);
	Vertex operator*(const Vertex &v) const;
	float operator&(const Vertex &v) const;//点积
};

class Normal : public Vertex
{
public:
	Normal() : Vertex() { };
	Normal(const float &ix, const float &iy, const float &iz, const float &iw = 0.0f) :Vertex(ix, iy, iz, iw) { };
	Normal(const Vertex &v);//归一化
};

class Point
{
public:
	Vertex pos;
	Normal norm;
	Coord2D tcoord;

	Point() { };
	Point(const Vertex &v, const Normal &n, const Coord2D &t) : pos(v), norm(n), tcoord(t) { };
};

class Triangle
{
public:
	Vertex points[3];
	Normal norms[3];
	Coord2D tcoords[3];

	Triangle();
	Triangle(const Vertex &va, const Vertex &vb, const Vertex &vc);
	Triangle(const Vertex &va, const Normal &na, const Vertex &vb, const Normal &nb, const Vertex &vc, const Normal &nc);
	Triangle(const Vertex &va, const Normal &na, const Coord2D &ta, const Vertex &vb, const Normal &nb, const Coord2D &tb, const Vertex &vc, const Normal &nc, const Coord2D &tc);
};

class Light
{
public:
	enum class Type : uint8_t
	{
		Parallel = 0x1, Point = 0x2, Spot = 0x3
	};
	enum class Property : uint8_t
	{
		Position = 0x1,
		Ambient = 0x2,
		Diffuse = 0x4,
		Specular = 0x8,
		Atten = 0x10
	};
public:
	Vertex position,
		ambient,
		diffuse,
		specular,
		attenuation;
	float coang, exponent;//for spot light
	int type;
	bool bLight;
	float rangy, rangz, rdis,
		angy, angz, dis;
public:
	Light(const Type type);
	bool turn();
	void move(const float dangy, const float dangz, const float ddis);
	void SetProperty(const uint8_t prop, const float r, const float g, const float b, const float a = 1.0f);
	void SetProperty(const Property prop, const float r, const float g, const float b, const float a = 1.0f)
	{
		SetProperty(uint8_t(prop), r, g, b, a);
	}
	void SetLumi(const float lum);
};

class Material
{
public:
	enum class Property : uint8_t
	{
		Ambient = 0x1,
		Diffuse = 0x2,
		Specular = 0x4,
		Emission = 0x8,
		Shiness = 0x10,
		Reflect = 0x20,
		Refract = 0x40,
		RefractRate = 0x80
	};
private:
	Vertex ambient,
		diffuse,
		specular,
		emission;
	float /*高光权重*/shiness,
		/*反射比率*/reflect,
		/*折射比率*/refract,
		/*折射率*/rfr;
public:
	string name;
	Material();
	~Material();
	void SetMtl(const uint8_t prop, const Vertex &);
	void SetMtl(const Property prop, const float r, const float g, const float b)
	{
		SetMtl(uint8_t(prop), Vertex(r, g, b));
	}
	void SetMtl(const uint8_t prop, const float val);
	void SetMtl(const Property prop, const float val)
	{
		SetMtl(uint8_t(prop), val);
	}
};

class Camera
{
public:
	Normal u, v, n;//to right,up,toward
	Vertex position;
	int width, height;
	float fovy, aspect, zNear, zFar;
	Camera(int w = 1120, int h = 630);
	void resize(int w, int h);
};


}