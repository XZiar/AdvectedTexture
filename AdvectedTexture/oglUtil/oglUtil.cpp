#include "oglUtil.h"

namespace oglu
{
using std::forward;
using glm::vec3;

oglShader::oglShader(const Type type, const char * fpath) : shaderType(type)
{
	FILE * fp;
	if (fopen_s(&fp, fpath, "rb") != 0)
		return;

	fseek(fp, 0, SEEK_END);
	long fsize = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	char * dat = new char[fsize + 1];
	fread(dat, fsize, 1, fp);
	dat[fsize] = '\0';
	src.assign(dat);

	fclose(fp);

	shaderID = glCreateShader(GLenum(type));
	glShaderSource(shaderID, 1, &dat, NULL);
	delete[] dat;
}

oglShader::~oglShader()
{
	if (shaderID)
		glDeleteShader(shaderID);
}

bool oglShader::compile(string & msg)
{
	glCompileShader(shaderID);

	GLint result;
	char logstr[4096] = { 0 };

	glGetShaderiv(shaderID, GL_COMPILE_STATUS, &result);
	if (!result)
	{
		glGetShaderInfoLog(shaderID, sizeof(logstr), NULL, logstr);
		msg.assign(logstr);
		return false;
	}
	return true;
}

GLint oglProgram::getUniLoc(const string & name)
{
	auto it = uniLocs.find(name);
	if (it != uniLocs.end())
		return it->second;
	//not existed yet
	GLint loc = glGetUniformLocation(programID, name.c_str());
	uniLocs.insert(make_pair(name, loc));
	return loc;
}

oglProgram::oglProgram()
{
	programID = glCreateProgram();
	glGenBuffers(1, &ID_lgtVBO);
	glGenBuffers(1, &ID_mtVBO);
}

oglProgram::~oglProgram()
{
	if (programID)
		glDeleteProgram(programID);
}

void oglProgram::addShader(oglShader && shader)
{
	glAttachShader(programID, shader.shaderID);
	shaders.push_back(forward<oglShader>(shader));
}

bool oglProgram::link(string & msg)
{
	glLinkProgram(programID);

	int result;
	char logstr[4096] = { 0 };

	glGetProgramiv(programID, GL_LINK_STATUS, &result);
	if (!result)
	{
		glGetProgramInfoLog(programID, sizeof(logstr), NULL, logstr);
		glDeleteProgram(programID);
		msg.assign(logstr);
		return false;
	}
	return true;
}

void oglProgram::use()
{
	glUseProgram(programID);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	GLuint idx;
}

void oglProgram::setProject(const Camera & cam, const int wdWidth, const int wdHeight)
{
	glViewport((wdWidth & 0x3f) / 2, (wdHeight & 0x3f) / 2, cam.width, cam.height);

	//mat4 projMat = glm::frustum(-RProjZ, +RProjZ, -Aspect*RProjZ, +Aspect*RProjZ, 1.0, 32768.0);
	matrix_Proj = glm::perspective(cam.fovy, cam.aspect, cam.zNear, cam.zFar);
	glUniformMatrix4fv(IDX_projMat, 1, GL_FALSE, glm::value_ptr(matrix_Proj));
}

void oglProgram::setCamera(const Camera & cam)
{
	matrix_View = glm::lookAt(vec3(cam.position), vec3(cam.position + cam.n), vec3(cam.v));
	glUniformMatrix4fv(IDX_viewMat, 1, GL_FALSE, glm::value_ptr(matrix_View));
	glUniform3fv(IDX_camPos, 1, cam.position);
}

void oglProgram::setLight(const uint8_t id, const Light & light)
{
	glBindBuffer(GL_UNIFORM_BUFFER, ID_lgtVBO);
	glBufferSubData(GL_UNIFORM_BUFFER, 96 * id, 96, &light);
}

void oglProgram::setMaterial(const Material & mt)
{
	glBindBuffer(GL_UNIFORM_BUFFER, ID_mtVBO);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, 80, &mt);
}

void oglProgram::drawObject(const function<void(void)>& draw, const Vertex & vTrans, const Vertex & vRotate, const float fScale)
{
	mat4 rMat = glm::rotate(mat4(), vRotate.x * float(M_PI), vec3(1.0f, 0.0f, 0.0f));
	rMat = glm::rotate(rMat, vRotate.y * float(M_PI), vec3(0.0f, 1.0f, 0.0f));
	rMat = glm::rotate(rMat, vRotate.z * float(M_PI), vec3(0.0f, 0.0f, 1.0f));
	glUniformMatrix4fv(IDX_normMat, 1, GL_FALSE, glm::value_ptr(matrix_Norm * rMat));

	mat4 mMat = glm::translate(matrix_Model, vec3(vTrans));
	mMat = glm::scale(mMat, vec3(fScale));
	glUniformMatrix4fv(IDX_modelMat, 1, GL_FALSE, glm::value_ptr(mMat * rMat));

	draw();

	glUniformMatrix4fv(IDX_modelMat, 1, GL_FALSE, glm::value_ptr(matrix_Model));
	glUniformMatrix4fv(IDX_normMat, 1, GL_FALSE, glm::value_ptr(matrix_Norm));
}

_oglBuffer::_oglBuffer(const Type _type) :bufferType(_type)
{
	glGenBuffers(1, &bID);
}

_oglBuffer::~_oglBuffer()
{
	glDeleteBuffers(1, &bID);
}

void _oglBuffer::write(const void * dat, const size_t size, const DrawMode mode)
{
	glBindBuffer((GLenum)bufferType, bID);
	glBufferData((GLenum)bufferType, size, dat, (GLenum)mode);
	glBindBuffer((GLenum)bufferType, 0);
}



oglVAO::oglVAO(const Mode _mode) :vaoMode(_mode)
{
	glGenVertexArrays(1, &vaoID);
}

oglVAO::~oglVAO()
{
	glDeleteVertexArrays(1, &vaoID);
}

void oglVAO::draw(const GLsizei size, const GLint offset)
{
	glBindVertexArray(vaoID);
	glDrawArrays((GLenum)vaoMode, offset, size);
	glBindVertexArray(0);
}

void _oglTexture::parseFormat(const Format format, GLint & intertype, GLenum & datatype, GLenum & comptype)
{
	switch (format)
	{
	case Format::RGB:
		intertype = GL_RGB;
		datatype = GL_UNSIGNED_BYTE;
		comptype = GL_RGB;
		break;
	case Format::RGBA:
		intertype = GL_RGBA;
		datatype = GL_UNSIGNED_BYTE;
		comptype = GL_RGBA;
		break;
	case Format::RGBf:
		intertype = GL_RGB32F;
		datatype = GL_FLOAT;
		comptype = GL_RGB;
		break;
	case Format::RGBAf:
		intertype = GL_RGBA32F;
		datatype = GL_FLOAT;
		comptype = GL_RGBA;
		break;
	}
}

_oglTexture::_oglTexture(const Type _type) : type(_type)
{
	glGenTextures(1, &tID);
}

_oglTexture::~_oglTexture()
{
	glDeleteTextures(1, &tID);
}

void _oglTexture::setData(const Format format, const GLsizei w, const GLsizei h, const void * data)
{
	glBindTexture((GLenum)type, tID);

	GLint intertype;
	GLenum datatype, comptype;
	parseFormat(format, intertype, datatype, comptype);
	glTexImage2D((GLenum)type, 0, intertype, w, h, 0, comptype, datatype, data);
	//glBindTexture((GLenum)type, 0);
}

void _oglTexture::setData(const Format format, const GLsizei w, const GLsizei h, const oglBuffer buf)
{
	glBindTexture((GLenum)type, tID);
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, buf->bID);

	GLint intertype;
	GLenum datatype, comptype;
	parseFormat(format, intertype, datatype, comptype);
	glTexImage2D((GLenum)type, 0, intertype, w, h, 0, comptype, datatype, NULL);

	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
	//glBindTexture((GLenum)type, 0);
}


}