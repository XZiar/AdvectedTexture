
#include "3dBasic/3dElement.h"
#include "oclUtil/oclUtil.h"
#include "oglUtil/oglUtil.h"

#include "rely.h"


using std::chrono::milliseconds;
using std::chrono::system_clock;
using std::chrono::duration_cast;
using namespace oglu;
using namespace oclu;

static unique_ptr<oglProgram> glProg;
static oclCommandQue clComQue;
static oclPlatfrom clPlat;
static oclProgram clProg;
static oclKernel clkGenNoise, clkGenColorful, clkGenStepNoise, clkGenMultiNoise, clkGenMultiNoiseCos;

static oglBuffer glVBOVert, glVBOtex, glVBONtex;
static oclMem clMemTex, clMemPbo, clMemNPbo;
static shared_ptr<oglVAO> VAO;
static oglTexture glTex;

uint64_t t_begin, t_end;
static int dim;
static bool bMovPOI = false;
static int sx, sy, mx, my;
static Camera cam;
static int clMode = 0;

void setTitle()
{
	char str[64];
	sprintf_s(str, "openGL+openCL");
	glutSetWindowTitle(str);
}

void genScreenBox()
{
	glVBOVert.reset(new _oglBuffer(_oglBuffer::Type::Array));
	VAO.reset(new oglVAO(oglVAO::Mode::Triangles));

	GLfloat DatVert[] =
	{
		-1.0f, -1.0f, 0.0f,
		1.0f, -1.0f, 0.0f,
		-1.0f, 1.0f, 0.0f,

		1.0f, 1.0f, 0.0f,
		-1.0f, 1.0f, 0.0f,
		1.0f, -1.0f, 0.0f,
	};
	glVBOVert->write(DatVert, sizeof(DatVert));
	VAO->prepare(glVBOVert, glProg->IDX_Vert_Pos, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), 0,
		glTex, glProg->getUniLoc("tex"), 0);
}

void initGL()
{
	glewInit();
	const GLubyte *version = glGetString(GL_VERSION);
	printf("GL Version:%s\n", version);
	glProg.reset(new oglProgram());

	string msg;
	{
		oglShader vert(oglShader::Type::Vertex, "basic.vert");
		if (vert.compile(msg))
			glProg->addShader(move(vert));
		else
			printf("ERROR on Vertex Shader Compiler:\n%s\n", msg.c_str());
	}
	{
		oglShader frag(oglShader::Type::Fragment, "basic.frag");
		if (frag.compile(msg))
			glProg->addShader(move(frag));
		else
			printf("ERROR on Fragment Shader Compiler:\n%s\n", msg.c_str());
	}
	{
		if (!glProg->link(msg))
			printf("ERROR on Program Linker:\n%s\n", msg.c_str());
	}
	glProg->use();

	glVBOtex.reset(new _oglBuffer(_oglBuffer::Type::Pixel));
	glVBONtex.reset(new _oglBuffer(_oglBuffer::Type::Pixel));
	glTex.reset(new _oglTexture(_oglTexture::Type::Tex2D));
	float *empty = new float[dim * dim * 8];
	for (int a = 0; a < dim*dim * 8; a += 4)
		empty[a] = 1.0f;

	glTex->setProperty(_oglTexture::PropType::Wrap, _oglTexture::PropVal::Clamp,
		_oglTexture::PropType::Filter, _oglTexture::PropVal::Nearest);
	glTex->setData(_oglTexture::Format::RGBAf, dim, dim, empty);
	for (int a = 1; a < dim*dim * 8; a += 4)
		empty[a] = 1.0f;
	glTex->setData(_oglTexture::Format::RGBAf, dim, dim, empty);

	glVBOtex->write(empty, dim * dim * 8 * 4, _oglBuffer::DrawMode::DynamicDraw);
	glVBONtex->write(nullptr, 1920 * 1920 * 4 * 4, _oglBuffer::DrawMode::DynamicDraw);

	genScreenBox();

	delete[] empty;
}

void runCL(const int mode);
void initCL()
{
	string msg;
	auto plats = oclUtil::getGLinterOPPlatforms();
	for (auto & p : plats)
	{
		printf("\n%s\n%s\n", p->name.c_str(), p->ver.c_str());
	}
	clPlat = plats[0];
	clComQue = oclUtil::getCommandQueue(clPlat);
	clProg.reset(new _oclProgram(clPlat));

	if (!clProg->load("test.cl", msg))
	{
		printf("Error:\n%s\n", msg.c_str());
	}
	clkGenNoise = oclUtil::getKernel(clProg, "genNoise");
	clkGenColorful = oclUtil::getKernel(clProg, "genColorful");
	clkGenStepNoise = oclUtil::getKernel(clProg, "genStepNoise");
	clkGenMultiNoise = oclUtil::getKernel(clProg, "genMultiNoise");
	clkGenMultiNoiseCos = oclUtil::getKernel(clProg, "genMultiNoiseCos");
	printf("Load CL kernel success!\n");
	float *empty = new float[dim * dim * 8];
	for (int a = 2; a < dim*dim * 8; a += 4)
		empty[a] = 1.0f;
	glTex->setData(_oglTexture::Format::RGBAf, dim, dim, empty);

	//clMemTex = clPlat->createMem(glTex);

	for (int a = 1; a < dim*dim * 8; a += 4)
		empty[a] = 1.0f;
	glTex->setData(_oglTexture::Format::RGBAf, dim, dim, empty);
	delete[] empty;
	clMemPbo = clPlat->createMem(glVBOtex);
	clMemNPbo = clPlat->createMem(glVBONtex);

	runCL(clMode);
}

void runCL(const int mode)
{
	t_begin = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
	const size_t ws[]{ dim, dim };
	const size_t wws[]{ cam.width, cam.height };


	if (mode < 2)
	{
		if (!clMemPbo->lock(clComQue))
			getchar();

		if (mode == 0)
		{
			clkGenColorful->setArg(0, clMemPbo);
			clkGenColorful->run<2>(clComQue, ws);
		}
		else if (mode == 1)
		{
			clkGenNoise->setArg(0, clMemPbo);
			clkGenNoise->run<2>(clComQue, ws);
		}

		if (!clMemPbo->unlock(clComQue))
			getchar();

		glTex->setData(_oglTexture::Format::RGBAf, dim, dim, glVBOtex);
	}
	else
	{
		if (!clMemNPbo->lock(clComQue))
			getchar();

		if (mode == 2)
		{
			clkGenStepNoise->setArg(0, 6);
			clkGenStepNoise->setArg(1, clMemNPbo);
			clkGenStepNoise->run<2>(clComQue, wws);
		}
		else if (mode == 3)
		{
			clkGenMultiNoise->setArg(0, 6);
			clkGenMultiNoise->setArg(1, clMemNPbo);
			clkGenMultiNoise->run<2>(clComQue, wws);
		}
		else if (mode == 4)
		{
			clkGenMultiNoiseCos->setArg(0, 6);
			clkGenMultiNoiseCos->setArg(1, clMemNPbo);
			clkGenMultiNoiseCos->run<2>(clComQue, wws);
		}

		if (!clMemNPbo->unlock(clComQue))
			getchar();

		glTex->setData(_oglTexture::Format::RGBAf, wws[0], wws[1], glVBONtex);
	}

	t_end = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
	printf("mode %d : running time:%lld\n", mode, t_end - t_begin);
}

void display(void)
{
	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	VAO->draw(6);

	glutSwapBuffers();
}

void reshape(int w, int h)
{
	cam.resize(w & 0x8fc0, h & 0x8fc0);

	glProg->setProject(cam, w, h);
}

void onSpecialKey(int key, int x, int y)
{
	switch (key)
	{
	case GLUT_KEY_LEFT:
		break;
	case GLUT_KEY_RIGHT:
		break;
	case GLUT_KEY_UP:
		break;
	case GLUT_KEY_DOWN:
		break;
	default:
		break;
	}
	setTitle();
	glutPostRedisplay();
}

void onKeyboard(unsigned char key, int x, int y)
{
	switch (key)
	{
	case 13:
		clMode = (clMode + 1) % 5;
		runCL(clMode);
		break;
	default:
		break;
	}
	glutPostRedisplay();
}

void onMouse(int button, int state, int x, int y)
{
	switch (button)
	{
	case GLUT_LEFT_BUTTON:
		if (state == GLUT_DOWN)
		{
			bMovPOI = true;
			sx = x, sy = y;
		}
		else
			bMovPOI = false;
		return;
	}

}

void onMotion(int x, int y)
{
	if (bMovPOI)
	{
		int dx = x - sx, dy = y - sy;
		sx = x, sy = y;
		float pdx = 10.0*dx / cam.width, pdy = 10.0*dy / cam.height;
		//cam.move(-pdx, pdy, 0);
		glutPostRedisplay();
	}
}

void onWheel(int button, int dir, int x, int y)
{
	if (dir == 1)//forward
		;
	else if (dir == -1)//backward
		;
	glutPostRedisplay();
}

int main(int argc, char** argv)
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
	glutInitWindowSize(cam.width, cam.height);
	glutInitWindowPosition(100, 100);
	glutCreateWindow(argv[0]);
	setTitle();

	if (argc > 1)
		dim = atoi(argv[1]);
	else
	{
		printf("input dim:");
		scanf_s("%d", &dim);
		getchar();
	}
	initGL();
	initCL();

	glutDisplayFunc(display);
	glutReshapeFunc(reshape);

	glutSpecialFunc(onSpecialKey);
	glutKeyboardFunc(onKeyboard);
	glutMouseFunc(onMouse);
	glutMotionFunc(onMotion);
	glutMouseWheelFunc(onWheel);

	glutMainLoop();

	return 0;
}
