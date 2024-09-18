#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <filesystem>
#include <unordered_map>

struct InputData {
    int homes;
    int max;
    std::vector<int> pieces;
};

InputData get_data(const std::string& filename);

/*
Provided that it later needed to be parallelised using OpenMP, I didn't try to find any "smart" dynamic programming solutions.
Serial Solution. Main idea:
    1. Acquire data: homes, max, pieces.
    2. Loop through pieces, considering the indexed house the starting house at each iteration. 
    3. Acquire maximum candy possible without surpassing max candy by iterating to the "right" of the starting house.
    Iterating through the left is redundant work, since we start from the first house.
    3. Keep track of max candy throughout the loop (keep in mind if there is a tie, first encountered house gets chosen)
    4. If at any point, maximum candy possible given a starting home is equal to max candy, break the loop, as that is the solution.
*/

int main(int argc, char* argv[]) {
    std::string filename = "input.txt"; // input.txt is default filename value
    if (argc > 1) {
        filename = argv[1];
    }

    InputData data = get_data(filename);
    int homes = data.homes;
    int max = data.max;
    std::vector<int> pieces = data.pieces;
    std::vector<int> solution_final = {-1, -1, -1}; //{max_possible_candy, starting_index, finishing_index}

    for (int i=0; i<pieces.size(); i++) {
        std::vector<int> solution_i;
        if (pieces[i]>max) {
            solution_i = {-1, -1, -1};
        }
        else {
            // Assuming first and last house are not connected
            int curr_candy = pieces[i];
            int curr_idx = i;

            while (curr_candy <= max && curr_idx < homes-1) {
                curr_idx++;
                curr_candy += pieces[curr_idx];
            }
            if (curr_candy > max) {
                curr_candy -= pieces[curr_idx];
                curr_idx--;
            }
            
            solution_i = {curr_candy, i, curr_idx};
        }
        if (solution_i[0] == max) {
            solution_final = solution_i;
            break;
        }
        solution_final = solution_i[0] > solution_final[0] ? solution_i : solution_final;
    }

    if (solution_final[0] == -1) {
        std::cout << "There are no solutions." << std::endl;
    }
    else {
        std::cout << "Start at home " << solution_final[1]+1 << " and go to home " << solution_final[2]+1
                  << " getting "<< solution_final[0] << " pieces of candy." << std::endl;
    }

    return 0;
}

InputData get_data(const std::string& filename) {
    std::vector<int> data;
    InputData result{0, 0, {}};  // Initialize with default values

    if (std::filesystem::exists(filename)) {
        std::ifstream file(filename);
        
        if (file.is_open()) {
            std::string line;
            while (std::getline(file, line)) {
                data.push_back(std::stoi(line));
            }
            file.close();

            if (data.size() >= 2) {
                result.homes = data[0];
                result.max = data[1];
                result.pieces = std::vector<int>(data.begin() + 2, data.end());
            } else {
                std::cerr << "Not enough data in the file." << std::endl;
            }
        } else {
            std::cerr << "Unable to open file" << std::endl;
        }
    } else {
        std::cerr << "Error: File '" << filename << "' not found in the current directory." << std::endl;
    }

    return result;
}