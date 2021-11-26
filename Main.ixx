#define CL_HPP_ENABLE_EXCEPTIONS
#include <CL/opencl.hpp>
#include <cstdio>
#include <cerrno>
import Window;

using namespace cl;

#define EXIT() { Exit(); return; }
inline const char *clGetErrorString(cl_int ec);

enum
{
	xBatches = 10,
	yBatches = 10,
	nBatches = 10,

	batchW = width / xBatches,
	batchH = height / yBatches,
};

Platform platform;
Device device;
Context ctx;
CommandQueue q;
Program prog;
Buffer pixelMem;
Kernel background;
Kernel waves;

string src;

string GetSrc(const char *file)
{
	FILE *fs;
	if (errno_t err = fopen_s(&fs, "Shaders.cl", "rb"))
	{
		char msg[69];
		if (strerror_s(msg, err))
			printf("can't open 'Shaders.cl': %s\n", msg);
		else
			printf("can't open 'Shaders.cl'\n");
		return string {};
	}

	fseek(fs, 0, SEEK_END);
	string src(ftell(fs), '\x69');
	rewind(fs);
	fread(src.data(), 1, src.length(), fs);
	fclose(fs);

	return std::move(src);
}

void Init()
{
	try
	{
		Platform::get(&platform);
		vector<Device> devices;
		platform.getDevices(CL_DEVICE_TYPE_GPU, &devices);

		device = devices[0];
		string deviceName = device.getInfo<CL_DEVICE_NAME>();
		printf("Using OpenCL Device '%s'\n", deviceName.c_str());

		ctx = Context(device, nullptr, nullptr, nullptr);

		q = CommandQueue(ctx, device, QueueProperties::None);

		src = GetSrc("Shaders.cl");

		prog = Program(ctx, src);
		try
		{
			prog.build(device);
		}
		catch (const BuildError &e)
		{
			BuildLogType log = prog.getBuildInfo<CL_PROGRAM_BUILD_LOG>();
			for (const auto &msg : log)
				puts(msg.second.c_str());
			EXIT();
		}

		pixelMem = Buffer(ctx, CL_MEM_READ_WRITE | CL_MEM_USE_HOST_PTR, (size_type) width * height * sizeof(int), pixels);

		background = Kernel(prog, "background");
		background.setArg(0, pixelMem);

		waves = Kernel(prog, "waves");
		waves.setArg(0, pixelMem);
		waves.setArg(4, 200.0f);
		waves.setArg(5, 30.0f);
		waves.setArg(6, 7.5f);
		waves.setArg(7, cl_float4 { .x = 0, .y = 0.78, .z = 1.0, .w = 0.4 });
		waves.setArg(8, cl_float4 { .x = 0.2, .y = 0.12, .z = 0.3, .w = 1.6 });
	}
	catch (const Error &e)
	{
		printf("opencl err %s(): %s\n", e.what(), clGetErrorString(e.err()));
		EXIT();
	}
}

void Shader(Kernel kernel)
{
	for (int yOff = 0; yOff < height; yOff += batchH)
	{
		kernel.setArg(2, yOff);
		for (int xOff = 0; xOff < width; xOff += batchH)
		{
			kernel.setArg(1, xOff);
			q.enqueueNDRangeKernel(kernel, NullRange, NDRange(batchW, batchH));
		}
	}
	
	q.flush();
	q.finish();
}

void Draw()
{
	try
	{
		Shader(background);
		waves.setArg(3, (float) clock() * -0.0002f);
		Shader(waves);
	}
	catch (const Error &e)
	{
		printf("opencl err %s(): %s\n", e.what(), clGetErrorString(e.err()));
		EXIT();
	}
}

inline const char* clGetErrorString(cl_int ec)
{
	switch (ec)
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