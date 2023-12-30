#include <fstream>
#include <filesystem>
#include <iostream>
#include <string>

import wmapsym;


void LaunchProcess() {
	std::string filepath_s;
	std::cout << "\n" << "Enter Wesnoth map file path: ";
	std::getline(std::cin, filepath_s);
	std::filesystem::path filepath = filepath_s;
	std::ifstream input_file{filepath};

	if (!input_file.is_open())
		throw std::exception{"failed to open input file"};

	int rotation_deg;
	while (true) {
		std::cout << "Sample quarter rotation in degrees: ";
		std::string temp;
		std::getline(std::cin, temp);
		rotation_deg = temp.empty() ? 0 : std::stoi(temp);

		if (rotation_deg % 90 == 0)
			break;

		std::cout << "Rotation must be divisible by 90\n";
	}

	wmapsym::WesnothMap map{input_file};
	wmapsym::Simple4PlayersSymmetrizer symmetrizer{map, rotation_deg};
	auto& symmetrized_map = symmetrizer.GetSymmetrizedMap();

	std::filesystem::path out_filepath = filepath;
	out_filepath.replace_filename("sym_" + filepath.filename().string());

	std::ofstream output_file{out_filepath};
	if (!output_file.is_open())
		throw std::exception{"failed to open output file"};

	symmetrized_map.WriteToFile(output_file);

	std::cout << "Successfully completed" << std::endl;
	std::cout << "Output file path: " << out_filepath.string() << std::endl;
}


void SafeLaunch() {
	while (true) {
		try {
			LaunchProcess();
		} catch (std::exception& e) {
			std::cout << "ERROR: " << e.what() << std::endl;
		}
	}
}


int main() {
	SafeLaunch();
}
