#include "oclUtil.h"


namespace oclu
{
using std::min;
using std::max;


template<class T>
static shared_ptr<T> ErrorConstruct(const cl_int code)
{
	printf("\nERROR:%s\n", oclUtil::getErrorString(code));
	return shared_ptr<T>(nullptr);
}


vector<oclPlatfrom> oclUtil::plfs;
void oclUtil::init()
{
	static bool isFirst = true;
	if (!isFirst)
		return;

	cl_uint numPlatforms = 0;
	clGetPlatformIDs(0, NULL, &numPlatforms);
	//Get all Platforms
	vector<cl_platform_id> platformIDs(numPlatforms);
	clGetPlatformIDs(numPlatforms, platformIDs.data(), NULL);

	for (const auto & pID : platformIDs)
	{
		plfs.push_back
		(oclPlatfrom(
			new _oclPlatfrom(pID)
		));
	}
	isFirst = false;
}

vector<oclPlatfrom> oclUtil::getPlatforms()
{
	init();
	return plfs;
}

vector<oclPlatfrom> oclUtil::getGLinterOPPlatforms()
{
	init();
	vector<oclPlatfrom> glps;
	for (auto & plat : plfs)
	{
		//Additional attributes to OpenCL context creation
		//which associate an OpenGL context with the OpenCL context 
		cl_context_properties props[] =
		{
			//OpenCL platform
			CL_CONTEXT_PLATFORM, (cl_context_properties)plat->pID,
			//OpenGL context
			CL_GL_CONTEXT_KHR,   (cl_context_properties)wglGetCurrentContext(),
			//HDC used to create the OpenGL context
			CL_WGL_HDC_KHR,     (cl_context_properties)wglGetCurrentDC(),
			0
		};
		clGetGLContextInfoKHR_fn clGetGLContext = (clGetGLContextInfoKHR_fn)clGetExtensionFunctionAddressForPlatform(plat->pID, "clGetGLContextInfoKHR");
		cl_device_id dID;
		cl_int ret = clGetGLContext(props, CL_CURRENT_DEVICE_FOR_GL_CONTEXT_KHR, sizeof(cl_device_id), &dID, NULL);
		if (ret == CL_SUCCESS)
		{
			oclPlatfrom p(new _oclPlatfrom(plat->pID));
			p->defDevID = dID;
			p->glInit(props);
			glps.push_back(p);
		}
	}
	return glps;
}

oclCommandQue oclUtil::getCommandQueue(const oclPlatfrom plat)
{
	return getCommandQueue(plat, plat->defDev);
}

oclCommandQue oclUtil::getCommandQueue(const oclPlatfrom plat, const oclDevice dev)
{
	plat->init();
	oclCommandQue cq(new _oclCommandQue(plat->context, dev->dID));
	return cq;
}

oclKernel oclUtil::getKernel(const oclProgram prog, const char * kname)
{
	try
	{
		_oclKernel* ker = new _oclKernel(prog, kname);
		return oclKernel(ker);
	}
	catch (const cl_int e)
	{
		return ErrorConstruct<_oclKernel>(e);
	}
}



_oclMem::_oclMem(const cl_context & context, const Type _type, const size_t _size) : type(_type), size(_size)
{
	isGL = false;
	cl_int ret;
	memID = clCreateBuffer(context, (cl_mem_flags)type, size, NULL, &ret);
	if (ret != CL_SUCCESS)
		throw ret;
}

_oclMem::_oclMem(const cl_context & context, const Type _type, const oglBuffer buf) : type(_type), size(0x7fffffff), glBuf(buf)
{
	isGL = true;
	cl_int ret;
	memID = clCreateFromGLBuffer(context, (cl_mem_flags)type, glBuf->bID, &ret);
	if (ret != CL_SUCCESS)
		throw ret;
}

_oclMem::_oclMem(const cl_context & context, const Type _type, const oglTexture tex) : type(_type), size(0x7fffffff), glTex(tex)
{
	isGL = true;
	cl_int ret;
	memID = clCreateFromGLTexture(context, (cl_mem_flags)type, (cl_GLenum)glTex->type, 0, glTex->tID, &ret);
	if (ret != CL_SUCCESS)
		throw ret;
}

bool _oclMem::lock(const oclCommandQue cmdQue)
{
	if (!isGL)
		return false;
	glFlush();
	cl_int ret = clEnqueueAcquireGLObjects(cmdQue->cmdQue, 1, &memID, 0, NULL, NULL);
	return ret == CL_SUCCESS;
}

bool _oclMem::unlock(const oclCommandQue cmdQue)
{
	if (!isGL)
		return false;
	clFlush(cmdQue->cmdQue);
	cl_int ret = clEnqueueReleaseGLObjects(cmdQue->cmdQue, 1, &memID, 0, NULL, NULL);
	return ret == CL_SUCCESS;
}

bool _oclMem::write(const oclCommandQue cmdQue, const void * buf, const size_t _size, const bool isBlock)
{
	cl_int ret = clEnqueueWriteBuffer(cmdQue->cmdQue, memID, isBlock ? CL_TRUE : CL_FALSE, 0, min(_size, size), buf, 0, NULL, NULL);
	return ret == CL_SUCCESS;
}

bool _oclMem::read(const oclCommandQue cmdQue, void * buf, const size_t _size, const bool isBlock)
{
	cl_int ret = clEnqueueReadBuffer(cmdQue->cmdQue, memID, isBlock ? CL_TRUE : CL_FALSE, 0, min(_size, size), buf, 0, NULL, NULL);
	return ret == CL_SUCCESS;
}

_oclMem::~_oclMem()
{
	clReleaseMemObject(memID);
}



_oclPlatfrom::_oclPlatfrom(const cl_platform_id _pID) :pID(_pID)
{
	{
		char str[128] = { 0 };
		clGetPlatformInfo(pID, CL_PLATFORM_NAME, 127, str, NULL);
		name.assign(str);
		clGetPlatformInfo(pID, CL_PLATFORM_VERSION, 127, str, NULL);
		ver.assign(str);
	}
	clGetDeviceIDs(pID, CL_DEVICE_TYPE_DEFAULT, 1, &defDevID, NULL);
	cl_uint numDevices;
	clGetDeviceIDs(pID, CL_DEVICE_TYPE_ALL, 0, 0, &numDevices);
	// Get all Device Info
	vector<cl_device_id> deviceIDs(numDevices);
	clGetDeviceIDs(pID, CL_DEVICE_TYPE_ALL, numDevices, deviceIDs.data(), NULL);

	for (const auto & dID : deviceIDs)
	{
		devs.push_back
		(oclDevice(
			new _oclDevice(*this, dID)
		));
	}
}

void _oclPlatfrom::init()
{
	if (!isFirst)
		return;
	cl_int ret;

	//create OpenCL context
	cl_context_properties props[] = { CL_CONTEXT_PLATFORM, reinterpret_cast<cl_context_properties>(pID), 0 };
	//context = clCreateContextFromType(props, CL_DEVICE_TYPE_DEFAULT, NULL, NULL, &ret);
	context = clCreateContext(props, 1, &defDevID, NULL, NULL, &ret);
	isFirst = false;
	for (auto & d : devs)
	{
		if (d->dID == defDevID)
			defDev = d;
	}
}

void _oclPlatfrom::glInit(const cl_context_properties props[])
{
	cl_int ret;
	//create OpenCL context
	context = clCreateContext(props, 1, &defDevID, NULL, NULL, &ret);
	isFirst = false;
	for (auto & d : devs)
	{
		if (d->dID == defDevID)
			defDev = d;
	}
}

_oclPlatfrom::~_oclPlatfrom()
{
	cl_int ret;
	ret = clReleaseContext(context);
}

oclMem _oclPlatfrom::createMem(const oglBuffer buf)
{
	try
	{
		_oclMem* _mem = new _oclMem(context, _oclMem::Type::ReadWrite, buf);
		return oclMem(_mem);
	}
	catch (const cl_int e)
	{
		return ErrorConstruct<_oclMem>(e);
	}
}

oclMem _oclPlatfrom::createMem(const oglTexture tex)
{
	try
	{
		_oclMem* _mem = new _oclMem(context, _oclMem::Type::ReadWrite, tex);
		return oclMem(_mem);
	}
	catch (const cl_int e)
	{
		return ErrorConstruct<_oclMem>(e);
	}
}

oclMem _oclPlatfrom::createMem(const _oclMem::Type _type, const size_t _size)
{
	try
	{
		_oclMem* _mem = new _oclMem(context, _type, _size);
		return oclMem(_mem);
	}
	catch (const cl_int e)
	{
		return ErrorConstruct<_oclMem>(e);
	}
}



_oclDevice::_oclDevice(const _oclPlatfrom & _plat, const cl_device_id _dID) :dID(_dID)
{
	char str[128] = { 0 };
	clGetDeviceInfo(dID, CL_DEVICE_NAME, 127, str, NULL);
	name.assign(str);
	clGetDeviceInfo(dID, CL_DEVICE_VENDOR, 127, str, NULL);
	vendor.assign(str);
	clGetDeviceInfo(dID, CL_DEVICE_PROFILE, 127, str, NULL);
	profile.assign(str);
}



_oclCommandQue::_oclCommandQue(const cl_context & context, const cl_device_id dID)
{
	cl_int ret;
	cmdQue = clCreateCommandQueue(context, dID, 0, &ret);
}

_oclCommandQue::~_oclCommandQue()
{
	cl_int ret;
	ret = clFlush(cmdQue);
	ret = clFinish(cmdQue);
	ret = clReleaseCommandQueue(cmdQue);
}



_oclProgram::_oclProgram(const oclPlatfrom _plat) : plat(_plat)
{
	plat->init();
}

_oclProgram::~_oclProgram()
{
	/* Finalization */
	cl_int ret;
	ret = clReleaseProgram(program);
}

bool _oclProgram::load(const char * fname, string & msg)
{
	FILE *fp;
	cl_int ret;
	char logstr[20480];

	if (fopen_s(&fp, fname, "rb") != 0)
	{
		msg.assign("cannot open file\n");
		return false;
	}

	fseek(fp, 0, SEEK_END);
	size_t fsize = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	char * _src = new char[fsize + 1];
	fread(_src, fsize, 1, fp);
	_src[fsize] = '\0';
	src.assign(_src);
	fclose(fp);
	delete[] _src;

	const char *_src_tmp = src.c_str();
	/* Create Kernel Program from the source */
	program = clCreateProgramWithSource(plat->context, 1, &_src_tmp, &fsize, &ret);
	if (ret != CL_SUCCESS)
	{
		sprintf_s(logstr, "Fail when create program, error code: %d", ret);
		msg.assign(logstr);
		return false;
	}

	/* Build Kernel Program */
	ret = clBuildProgram(program, 1, &plat->defDevID, NULL, NULL, NULL);
	if (ret != CL_SUCCESS)
	{
		clGetProgramBuildInfo(program, plat->defDevID, CL_PROGRAM_BUILD_LOG, sizeof(logstr), logstr, NULL);
		msg.assign(logstr);
		return false;
	}
	return true;
}



_oclKernel::_oclKernel(const oclProgram _prog, const char * kname) :clProg(_prog)
{
	cl_int ret;
	kernel = clCreateKernel(clProg->program, kname, &ret);
	if (ret != CL_SUCCESS)
		throw ret;
}

_oclKernel::~_oclKernel()
{
	clReleaseKernel(kernel);
}

bool _oclKernel::setArg(const cl_uint idx, const oclMem mem)
{
	cl_int ret = clSetKernelArg(kernel, idx, sizeof(cl_mem), &(mem.get()->memID));
	return ret == CL_SUCCESS;
}



const char * oclUtil::getErrorString(const cl_int code)
{
	switch (code)
	{
		// run-time and JIT compiler errors
	case 0: return "CL_SUCCESS";
	case -1: return "CL_DEVICE_NOT_FOUND";
	case -2: return "CL_DEVICE_NOT_AVAILABLE";
	case -3: return "CL_COMPILER_NOT_AVAILABLE";
	case -4: return "CL_MEM_OBJECT_ALLOCATION_FAILURE";
	case -5: return "CL_OUT_OF_RESOURCES";
	case -6: return "CL_OUT_OF_HOST_MEMORY";
	case -7: return "CL_PROFILING_INFO_NOT_AVAILABLE";
	case -8: return "CL_MEM_COPY_OVERLAP";
	case -9: return "CL_IMAGE_FORMAT_MISMATCH";
	case -10: return "CL_IMAGE_FORMAT_NOT_SUPPORTED";
	case -11: return "CL_BUILD_PROGRAM_FAILURE";
	case -12: return "CL_MAP_FAILURE";
	case -13: return "CL_MISALIGNED_SUB_BUFFER_OFFSET";
	case -14: return "CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST";
	case -15: return "CL_COMPILE_PROGRAM_FAILURE";
	case -16: return "CL_LINKER_NOT_AVAILABLE";
	case -17: return "CL_LINK_PROGRAM_FAILURE";
	case -18: return "CL_DEVICE_PARTITION_FAILED";
	case -19: return "CL_KERNEL_ARG_INFO_NOT_AVAILABLE";

		// compile-time errors
	case -30: return "CL_INVALID_VALUE";
	case -31: return "CL_INVALID_DEVICE_TYPE";
	case -32: return "CL_INVALID_PLATFORM";
	case -33: return "CL_INVALID_DEVICE";
	case -34: return "CL_INVALID_CONTEXT";
	case -35: return "CL_INVALID_QUEUE_PROPERTIES";
	case -36: return "CL_INVALID_COMMAND_QUEUE";
	case -37: return "CL_INVALID_HOST_PTR";
	case -38: return "CL_INVALID_MEM_OBJECT";
	case -39: return "CL_INVALID_IMAGE_FORMAT_DESCRIPTOR";
	case -40: return "CL_INVALID_IMAGE_SIZE";
	case -41: return "CL_INVALID_SAMPLER";
	case -42: return "CL_INVALID_BINARY";
	case -43: return "CL_INVALID_BUILD_OPTIONS";
	case -44: return "CL_INVALID_PROGRAM";
	case -45: return "CL_INVALID_PROGRAM_EXECUTABLE";
	case -46: return "CL_INVALID_KERNEL_NAME";
	case -47: return "CL_INVALID_KERNEL_DEFINITION";
	case -48: return "CL_INVALID_KERNEL";
	case -49: return "CL_INVALID_ARG_INDEX";
	case -50: return "CL_INVALID_ARG_VALUE";
	case -51: return "CL_INVALID_ARG_SIZE";
	case -52: return "CL_INVALID_KERNEL_ARGS";
	case -53: return "CL_INVALID_WORK_DIMENSION";
	case -54: return "CL_INVALID_WORK_GROUP_SIZE";
	case -55: return "CL_INVALID_WORK_ITEM_SIZE";
	case -56: return "CL_INVALID_GLOBAL_OFFSET";
	case -57: return "CL_INVALID_EVENT_WAIT_LIST";
	case -58: return "CL_INVALID_EVENT";
	case -59: return "CL_INVALID_OPERATION";
	case -60: return "CL_INVALID_GL_OBJECT";
	case -61: return "CL_INVALID_BUFFER_SIZE";
	case -62: return "CL_INVALID_MIP_LEVEL";
	case -63: return "CL_INVALID_GLOBAL_WORK_SIZE";
	case -64: return "CL_INVALID_PROPERTY";
	case -65: return "CL_INVALID_IMAGE_DESCRIPTOR";
	case -66: return "CL_INVALID_COMPILER_OPTIONS";
	case -67: return "CL_INVALID_LINKER_OPTIONS";
	case -68: return "CL_INVALID_DEVICE_PARTITION_COUNT";

		// extension errors
	case -1000: return "CL_INVALID_GL_SHAREGROUP_REFERENCE_KHR";
	case -1001: return "CL_PLATFORM_NOT_FOUND_KHR";
	case -1002: return "CL_INVALID_D3D10_DEVICE_KHR";
	case -1003: return "CL_INVALID_D3D10_RESOURCE_KHR";
	case -1004: return "CL_D3D10_RESOURCE_ALREADY_ACQUIRED_KHR";
	case -1005: return "CL_D3D10_RESOURCE_NOT_ACQUIRED_KHR";
	default: return "Unknown OpenCL error";
	}
}


}

