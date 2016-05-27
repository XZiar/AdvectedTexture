#pragma once

#include "oclRely.h"
#include "../oglUtil/oglUtil.h"

namespace oclu
{
using std::string;
using std::vector;
using std::shared_ptr;
using oglu::oglBuffer;
using oglu::oglTexture;


class _oclPlatfrom;
using oclPlatfrom = shared_ptr<_oclPlatfrom>;
class _oclDevice;
using oclDevice = shared_ptr<_oclDevice>;
class _oclCommandQue;
using oclCommandQue = shared_ptr<_oclCommandQue>;
class _oclMem;
using oclMem = shared_ptr<_oclMem>;
class _oclProgram;
using oclProgram = shared_ptr<_oclProgram>;
class _oclKernel;
using oclKernel = shared_ptr<_oclKernel>;

class oclUtil
{
private:
	friend class _oclProgram;
	static vector<oclPlatfrom> plfs;
	static void init();
public:
	static vector<oclPlatfrom> getPlatforms();
	static vector<oclPlatfrom> getGLinterOPPlatforms();
	static oclCommandQue getCommandQueue(const oclPlatfrom);
	static oclCommandQue getCommandQueue(const oclPlatfrom, const oclDevice);
	static oclKernel getKernel(const oclProgram, const char *);
	static const char * getErrorString(const cl_int);
};

class _oclMem
{
public:
	enum class Type : cl_mem_flags
	{
		ReadOnly = CL_MEM_READ_ONLY, WriteOnly = CL_MEM_WRITE_ONLY, ReadWrite = CL_MEM_READ_WRITE,
		HostUse = CL_MEM_USE_HOST_PTR, HostAlloc = CL_MEM_ALLOC_HOST_PTR, HostCopy = CL_MEM_COPY_HOST_PTR
	};
private:
	friend class _oclKernel;
	friend class _oclPlatfrom;
	Type type;
	bool isGL;
	cl_mem memID;
	size_t size;
	oglBuffer glBuf;
	oglTexture glTex;
	_oclMem(const cl_context &, const Type, const size_t);
	_oclMem(const cl_context &, const Type, const oglBuffer);
	_oclMem(const cl_context &, const Type, const oglTexture);
public:
	bool lock(const oclCommandQue);
	bool unlock(const oclCommandQue);
	bool write(const oclCommandQue, const void *, const size_t, const bool isBlock = true);
	bool read(const oclCommandQue, void *, const size_t, const bool isBlock = true);
	~_oclMem();
};

class _oclPlatfrom
{
private:
	friend class _oclProgram;
	friend class oclUtil;
	bool isFirst = true;
	cl_platform_id pID;
	cl_device_id defDevID;
	cl_context context;
	vector<oclDevice> devs;
	oclDevice defDev;
	_oclPlatfrom(const cl_platform_id _pID);
	void init();
	void glInit(const cl_context_properties[]);
public:
	string name, ver;
	~_oclPlatfrom();
	oclMem createMem(const oglBuffer);
	oclMem createMem(const oglTexture);
	oclMem createMem(const _oclMem::Type, const size_t);
};

class _oclDevice
{
private:
	friend class _oclPlatfrom;
	friend class _oclProgram;
	friend class oclUtil;
	cl_device_id dID;
	_oclDevice(const _oclPlatfrom & _plat, const cl_device_id _dID);
public:
	string name, vendor, profile;
};

class _oclCommandQue
{
private:
	friend class _oclMem;
	friend class _oclKernel;
	friend class oclUtil;
	cl_command_queue cmdQue;
	_oclCommandQue(const cl_context &, const cl_device_id dID);
public:
	~_oclCommandQue();
};

class _oclProgram
{
private:
	friend class _oclMem;
	friend class _oclKernel;
	oclPlatfrom plat;
	cl_program program;
	string src;
public:
	_oclProgram(const oclPlatfrom _plat);
	~_oclProgram();
	bool load(const char * fname, string & msg);
};

class _oclKernel
{
private:
	friend class oclUtil;
	cl_kernel kernel;
	oclProgram clProg;
	_oclKernel(const oclProgram, const char *);
public:
	~_oclKernel();
	bool setArg(const cl_uint, const oclMem);
	template<typename T>
	bool setArg(const cl_uint idx, const T & dat)
	{
		cl_int ret = clSetKernelArg(kernel, idx, sizeof(T), &dat);
		return ret == CL_SUCCESS;
	};
	template<typename T, size_t N>
	bool setArg(const cl_uint idx, const T(&dat)[N])
	{
		cl_int ret = clSetKernelArg(kernel, idx, sizeof(T) * N, &dat[0]);
		return ret == CL_SUCCESS;
	};
	template<cl_uint N>
	bool run(const oclCommandQue cmdQue, const size_t(&worksize)[N], bool isBlock = true, const size_t(&workoffset)[N] = { 0 }, const size_t * localsize = nullptr)
	{
		/* Execute OpenCL Kernel */
		cl_int ret;
		if (isBlock)
		{
			cl_event enentPoint;
			ret = clEnqueueNDRangeKernel(cmdQue->cmdQue, kernel, N, workoffset, worksize, localsize, 0, NULL, &enentPoint);
			clWaitForEvents(1, &enentPoint); //wait
			clReleaseEvent(enentPoint);
		}
		else
			ret = clEnqueueNDRangeKernel(cmdQue->cmdQue, kernel, N, workoffset, worksize, localsize, 0, NULL, NULL);
		return ret == CL_SUCCESS;
	}
};


}