#include <cstdint>
#include <iostream>
#include <filesystem>
#include <string>
#include <fstream>
#include <stack>

#if (defined(_MSC_VER) && _MSC_VER == 1900)
namespace fs = std::tr2::sys;
#else
namespace fs = std::filesystem;
#endif

#include "external/json/src/json.hpp"

using json = nlohmann::json;

std::string stringFromFile(fs::path path)
{
#ifdef _WIN32
	std::ifstream in{ path.wstring(), std::ios::binary };
#else
	std::ifstream in{ path.string(), std::ios::binary };
#endif
	in.seekg(0, SEEK_END);
	
	int64_t size = in.tellg();

	if (size < 0 || static_cast<uint64_t>(size) > std::numeric_limits<size_t>::max())
	{
		throw std::runtime_error("File too large to read into memory");
	}

	std::string ret(static_cast<size_t>(size), '\0');

	in.seekg(0, SEEK_SET);

	if (!in.good())
	{
		throw std::runtime_error("Seeks failed");
	}

	in.read(&ret[0], size);

	if (!in.good())
	{
		throw std::runtime_error("Read failed");
	}

	// strip comments in style //
	std::string::size_type commentBegin, commentEnd{ 0 };
	
	while ((commentBegin = ret.find("//", commentEnd)) != std::string::npos)
	{
		commentEnd = ret.find_first_of("\r\n", commentBegin);
		if (commentEnd == std::string::npos)
		{
			commentEnd = ret.size();
		}
		ret.replace(commentBegin, commentEnd - commentBegin, std::string());
		commentEnd = commentBegin;
	}

	// strip comments in style /* */
	commentEnd = 0;

	while ((commentBegin = ret.find("/*", commentEnd)) != std::string::npos)
	{
		commentEnd = ret.find("*/");
		if (commentEnd == std::string::npos)
		{
			commentEnd = ret.size();
		}
		ret.replace(commentBegin, commentEnd + 2 - commentBegin, std::string());
		commentEnd = commentBegin;
	}

	return ret;
}

void stringToFile(fs::path path, std::string const & input)
{
#ifdef _WIN32
	std::ofstream out{ path.wstring(), std::ios::binary };
#else
	std::ofstream out{ path.string(), std::ios::binary };
#endif

	out.write(&input[0], input.size());
	out.flush();

	if (!out.good())
	{
		throw std::runtime_error("Failed to write");
	}
}

int utf8_main(int argc, char* argv[])
{
	try
	{
		if (argc < 3)
		{
			std::cerr << "Syntax:\nsb-massive-patch <unpacked-root> <patch-root>" << std::endl;
			return 1;
		}

		auto unpackedRoot = fs::u8path(std::string{ argv[1] } );
		auto patchRoot = fs::u8path(std::string{ argv[2] });

		std::cout << "Enumerating: " << unpackedRoot << std::endl;

		uint32_t enumerated{ 0 }, patched{ 0 }, failed{ 0 };
		for (fs::recursive_directory_iterator next{ unpackedRoot }, end; next != end; ++next)
		{
			if (!fs::is_regular_file(next->status()))
			{
				continue;
			}

			auto file{ next->path() };
			
			if (file.has_extension() && file.extension() == ".object")
			{
				try
				{
					++enumerated;
					std::cout << "Checking \"" << file << "\"" << std::endl;

					// Parse as json (throws on failure)
					auto asJson = json::parse(stringFromFile(file));

					auto printable = asJson.find("printable");

					if (printable == asJson.end())
					{
						// No /printable, maybe not an object?
						continue;
					}

					// Generate patch file path
					auto following = std::mismatch(file.begin(), file.end(), unpackedRoot.begin(), unpackedRoot.end()).first;
					
					auto patchPath = patchRoot;

					for (;following != file.end(); ++following)
					{
						patchPath /= *following;
					}

					patchPath += ".patch";

					auto patchPathParent = patchPath.parent_path();

					if (!fs::exists(patchPathParent) && !fs::create_directories(patchPathParent))
					{
						std::cerr << "Error: Failed to create directory structure \"" << patchPathParent << "\"" << std::endl;
					}

					std::cout << "Writing patch: \"" << patchPath << "\"" << std::endl;

					// Make a copy
					auto originalJson = asJson;

					// Change the value in 'asJson'
					*printable = true;

					// Diff and save
					stringToFile(patchPath, json::diff(originalJson, asJson).dump());
					++patched;
				}
				catch (std::exception const & e)
				{
					++failed;
					std::cerr << "Exception file \"" << file << "\" : " << e.what() << std::endl;
				}
			}
		}

		std::cout << "Complete\nenumerated: " << enumerated << " patched: " << patched << " failed: " << failed << std::endl;
		return 0;
	}
	catch (std::exception const & e)
	{
		std::cerr << e.what() << std::endl;
		return 1;
	}
	catch (...)
	{
		std::cerr << "Unknown exception" << std::endl;
		return 1;
	}
}
