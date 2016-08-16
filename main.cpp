
int utf8_main(int, char*[]);

#ifdef _WIN32
#include <Windows.h>

#include <vector>
#include <string>
#include <iostream>

int wmain(int argc, wchar_t* argv[])
{
	std::vector<std::string> args(argc);
	std::vector<char*> arg_p(argc);

	for (int i = 0; i < argc; ++i)
	{
		args[i].resize(::WideCharToMultiByte(CP_UTF8, 0, argv[i], -1, nullptr, 0, nullptr, nullptr));
		if (!args[i].size())
		{
			std::cerr << "Failed to get length of argument" << std::endl;
			return -1;
		}
		args[i].resize(::WideCharToMultiByte(CP_UTF8, 0, argv[i], -1, &args[i][0], static_cast<int>(args[i].size()), nullptr, nullptr));
		arg_p[i] = &args[i][0];
	}
	
	return utf8_main(argc, &arg_p[0]);
}
#else 
int main(int argc, char* argv[])
{
	return utf8_main(argc, argv);
}
#endif

