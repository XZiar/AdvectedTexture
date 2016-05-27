#pragma once

#include "oglRely.h"
#include "../3dBasic/3dElement.h"

namespace oclu
{
class _oclMem;
}

namespace oglu
{
using std::string;
using std::vector;
using std::map;
using std::function;
using std::shared_ptr;
using glm::mat4;
using b3d::Vertex;
using b3d::Camera;
using b3d::Material;
using b3d::Light;

class oglShader
{
public:
	enum class Type : GLenum
	{
		Vertex = GL_VERTEX_SHADER, Geometry = GL_GEOMETRY_SHADER, Fragment = GL_FRAGMENT_SHADER,
		TessCtrl = GL_TESS_CONTROL_SHADER, TessEval = GL_TESS_EVALUATION_SHADER,
		Compute = GL_COMPUTE_SHADER
	};
private:
	friend class oglProgram;
	Type shaderType;
	GLuint shaderID = 0;
	string src;
public:
	oglShader(const Type, const char *);
	oglShader(const oglShader &) = delete;
	oglShader & operator = (const oglShader &) = delete;
	oglShader(oglShader &&) = default;
	oglShader & operator = (oglShader &&) = default;
	~oglShader();

	bool compile(string & msg);
};

class oglProgram
{
private:
	GLuint programID = 0;
	vector<oglShader> shaders;
	map<string, GLint> uniLocs;
	mat4 matrix_Proj, matrix_View, matrix_Model, matrix_Norm;
	GLuint ID_lgtVBO, ID_mtVBO;
public:
	const static GLuint
		IDX_projMat = GL_INVALID_INDEX,
		IDX_viewMat = GL_INVALID_INDEX,
		IDX_modelMat = GL_INVALID_INDEX,
		IDX_normMat = GL_INVALID_INDEX,
		IDX_camPos = GL_INVALID_INDEX;
	const static GLuint
		IDX_Vert_Pos = 0,
		IDX_Vert_Norm = GL_INVALID_INDEX,
		IDX_Vert_Color = GL_INVALID_INDEX,
		IDX_Vert_Texc = GL_INVALID_INDEX;
	const static GLuint
		IDX_Uni_Light = 0,
		IDX_Uni_Material = 1;
	oglProgram();
	~oglProgram();

	void addShader(oglShader && shader);
	bool link(string & msg);
	void use();
	GLint getUniLoc(const string &);
	void setProject(const Camera &, const int wdWidth, const int wdHeight);
	void setCamera(const Camera &);
	void setLight(const uint8_t id, const Light &);
	void setMaterial(const Material &);
	void drawObject(const function<void(void)> &, const Vertex & vTrans, const Vertex & vRotate, const float fScale);
};

class _oglBuffer
{
public:
	enum class Type : GLenum
	{
		Array = GL_ARRAY_BUFFER, Element = GL_ELEMENT_ARRAY_BUFFER, Uniform = GL_UNIFORM_BUFFER, Pixel = GL_PIXEL_UNPACK_BUFFER
	};
	enum class DrawMode : GLenum
	{
		StreamDraw = GL_STREAM_DRAW, StreamRead = GL_STREAM_READ, StreamCopy = GL_STREAM_COPY,
		StaticDraw = GL_STATIC_DRAW, StaticRead = GL_STATIC_READ, StaticCopy = GL_STATIC_COPY,
		DynamicDraw = GL_DYNAMIC_DRAW, DynamicRead = GL_DYNAMIC_READ, DynamicCopy = GL_DYNAMIC_COPY,
	};
private:
	friend class oglVAO;
	friend class oclu::_oclMem;
	friend class _oglTexture;
	Type bufferType;
	GLuint bID;
public:
	_oglBuffer(const Type);
	~_oglBuffer();

	void write(const void *, const size_t, const DrawMode = DrawMode::StaticDraw);
};
using oglBuffer = shared_ptr<_oglBuffer>;

class _oglTexture
{
public:
	enum class Type : GLenum { Tex2D = GL_TEXTURE_2D, };
	enum class Format : GLenum
	{
		RGB = GL_RGB, RGBA = GL_RGBA, RGBf = GL_RGB32F, RGBAf = GL_RGBA32F
	};
	enum class PropType { Wrap, Filter };
	enum class PropVal : GLint
	{
		Linear = GL_LINEAR, Nearest = GL_NEAREST,
		Repeat = GL_REPEAT, Clamp = GL_CLAMP,
	};
private:
	friend class oglVAO;
	friend class oclu::_oclMem;
	Type type;
	void parseFormat(const Format format, GLint & intertype, GLenum & datatype, GLenum & comptype);
	void _setProperty() { };
	template <class... T>
	void _setProperty(const PropType _type, const PropVal params, T... args);
public:
	GLuint tID;
	_oglTexture(const Type _type = Type::Tex2D);
	~_oglTexture();
	template <class... T>
	void setProperty(T... args)
	{
		glBindTexture((GLenum)type, tID);
		_setProperty(args...);
		glBindTexture((GLenum)type, 0);
	}
	void setData(const Format format, const GLsizei w, const GLsizei h, const void *);
	void setData(const Format format, const GLsizei w, const GLsizei h, const oglBuffer);
};
using oglTexture = shared_ptr<_oglTexture>;

class oglVAO
{
public:
	enum class Mode : GLenum
	{
		Triangles = GL_TRIANGLES
	};
private:
	Mode vaoMode;
	GLuint vaoID;
	void _prepare() { };
	template <class... T>
	void _prepare(
		const oglBuffer buf,
		const GLuint index,
		const GLint size,
		const GLenum type,
		const GLboolean normalized,
		const GLsizei stride,
		const GLint offset,
		T... args);
	template <class... T>
	void _prepare(const oglTexture tex, const GLint index, const uint8_t tidx, T... args)
	{
		glActiveTexture(GL_TEXTURE0 + tidx);
		glBindTexture((GLenum)tex->type, tex->tID);
		glUniform1i(index, tidx);
	}
public:
	oglVAO(const Mode);
	~oglVAO();

	template <class... T>
	void prepare(T... args)
	{
		glBindVertexArray(vaoID);
		_prepare(args...);
		glBindVertexArray(0);
	}
	void draw(const GLsizei size, const GLint offset = 0);
};



template <class... T>
void _oglTexture::_setProperty(const PropType _type, const PropVal params, T... args)
{

	switch (_type)
	{
	case PropType::Wrap:
		glTexParameteri((GLenum)type, GL_TEXTURE_WRAP_S, (GLint)params);
		glTexParameteri((GLenum)type, GL_TEXTURE_WRAP_T, (GLint)params);
		break;
	case PropType::Filter:
		glTexParameteri((GLenum)type, GL_TEXTURE_MAG_FILTER, (GLint)params);
		glTexParameteri((GLenum)type, GL_TEXTURE_MIN_FILTER, (GLint)params);
		break;
	}
	_setProperty(args...);
}



template <class... T>
void oglVAO::_prepare(
	const oglBuffer buf,
	const GLuint index,
	const GLint size,
	const GLenum type,
	const GLboolean normalized,
	const GLsizei stride,
	const GLint offset,
	T... args)
{
	glBindBuffer((GLenum)buf->bufferType, buf->bID);
	glEnableVertexAttribArray(index);//vertex
	glVertexAttribPointer(index, size, type, normalized, stride, (void*)offset);
	_prepare(args...);
}


}