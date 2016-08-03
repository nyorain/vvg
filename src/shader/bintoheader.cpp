#include <fstream>
#include <string>
#include <iostream>
#include <vector>
#include <cstdint>
#include <stdexcept>
#include <cstring>

#include "cxxopts.hpp"

//beginning of the file to output
constexpr auto fileTemplate = 1 + R"SRC(
#ifndef BINARY_%i_HEADER_INCLUDE
#define BINARY_%i_HEADER_INCLUDE

#include <stdint.h>

//just check for c++11
#if defined(__cplusplus) && __cplusplus >= 201103L
	constexpr	
#else
	const
#endif

%t %n[] = {
	%d
};

#endif //header guard)SRC";

void replace(std::string& str, const std::string& from, const std::string& to) 
{
    if(from.empty()) return;
    size_t start_pos = 0;
    while((start_pos = str.find(from, start_pos)) != std::string::npos) 
	{
        str.replace(start_pos, from.length(), to);
        start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
    }
}

//how many chars should be in one line (excluding beginning tab).
constexpr auto lineLength = 80;

std::string parseFileName(const std::string& name)
{
	auto pos1 = name.find_last_of('\\');
	auto pos2 = name.find_last_of('/');

	if(pos1 == std::string::npos) pos1 = 0;
	else ++pos1;
	if(pos2 == std::string::npos) pos2 = 0;
	else ++pos2;

	auto ret = name.substr(std::max(pos1, pos2)); 
	return ret;
}

std::vector<std::uint8_t> loadFile(const std::string& file)
{
	auto openmode = std::ios::ate | std::ios::binary;

	std::ifstream ifs(file, openmode);
	if(!ifs.is_open()) 
	{
		std::cerr << "loadFile: couldnt open input file " << file << "\n";
		std::cerr << "error: " << std::strerror(errno) << "\n";
		exit(EXIT_FAILURE);
	}

	auto size = ifs.tellg();
	ifs.seekg(0, std::ios::beg);

	std::vector<std::uint8_t> buffer(size);
	auto data = reinterpret_cast<char*>(buffer.data());
	if(!ifs.read(data, size)) 
	{
		std::cerr << "loadFile: failed to read input file " << file << "\n";
		std::cerr << "error: " << std::strerror(errno) << "\n";
		exit(EXIT_FAILURE);
	}

	return buffer;
}

void outputData(const std::vector<std::uint8_t>& data, const std::string& file, 
	const std::string& name, int size)
{
	auto fname = parseFileName(file);
	std::ofstream ofs(fname);
	if(!ofs.is_open()) 
	{
		std::cerr << "outputData: couldnt open output file " << file << "\n";
		std::cerr << "error: " << std::strerror(errno) << "\n";
		exit(EXIT_FAILURE);
	}

	auto byteSize = size / 8;
	if(data.size() % byteSize) 
	{
		std::cerr << "outputData: file size is not a multiple of specified integer size\n";
		std::cerr << "\tfile size: " << data.size() << " bytes\n";
		std::cerr << "\tspecified size: " << size / 8 << " bytes\n";
		exit(EXIT_FAILURE);
	}

	std::string dataString;
	dataString.reserve(data.size() * 5); //approx.

	auto currColSize = 0u;
	for(auto i = 0u; i < data.size() / byteSize; ++i)
	{
		std::uint64_t value = 0;
		auto ptr = &data[i * byteSize];

		if(size == 8) value = *reinterpret_cast<const std::uint8_t*>(ptr);
		else if(size == 16) value = *reinterpret_cast<const std::uint16_t*>(ptr);
		else if(size == 32) value = *reinterpret_cast<const std::uint32_t*>(ptr);
		else if(size == 64) value = *reinterpret_cast<const std::uint64_t*>(ptr);

		auto str = std::to_string(value);
		if(i < data.size() / byteSize - 1) str += ", "; //check if last value or not
		if(currColSize + str.size() > lineLength) 
		{
			dataString.append("\n\t");
			currColSize = 0;
		}
		currColSize += str.size();
		dataString.append(str);
	}

	std::string outputTypeName = "uint" + std::to_string(size) + "_t";
	std::string includeGuard = name;
	for(auto& ch : includeGuard) ch = std::toupper(ch);

	std::string txt = fileTemplate;
	replace(txt, "%t", outputTypeName);
	replace(txt, "%d", dataString);
	replace(txt, "%n", name);
	replace(txt, "%i", includeGuard);

	ofs << txt;
}

int main(int argc, char** argv)
{
	//parse args
	std::string input;
	std::string output;
	std::string name = "binaryData";
	unsigned int size = 8;

	try
	{
		cxxopts::Options options("bintoheader", "Program to convert files into C headers");

		options.add_options()
			("i,input", "the input file. Can be any type of file, binary as well as text",
				cxxopts::value<std::string>(input))
			("o,output", "the output file. By default the input flie name with a '.h' suffix", 
				cxxopts::value<std::string>(output))
			("s,size", "the byte size of the array {8|16|32|64}. Defaulted to 8. Will fail if "
				"file size is not multiple.", cxxopts::value<unsigned int>(size))
			("n,name", "the name of the array in the c header. Defaulted to 'binaryData'", 
				cxxopts::value<std::string>(name))
			("h,help", "Print this help string");

		options.parse(argc, argv);
		if(options.count("help"))
		{
			std::cerr << options.help({""}) << "\n";
			return EXIT_SUCCESS;
		}
		
		if(options.count("input") != 1)
		{
			std::cerr << "Invalid usage.\n" << options.help({""}) << "\n";
			return EXIT_FAILURE;
		}

		if(!(size == 8 || size == 16 || size == 32 || size == 64))
		{
			std::cerr << "Invalid size paramter.\n";
			return EXIT_FAILURE;
		}

		if(!options.count("output")) output = input + ".h";
		replace(name, ".", "_");
	} 
	catch(const cxxopts::OptionException& e)
	{
		std::cerr << "Could not parse given arguments\n";
			return EXIT_FAILURE;
	}

	auto data = loadFile(input);
	outputData(data, output, name, size);
	std::cout << "Succesfully parsed " << name << ". " << data.size () << " bytes\n";
}
