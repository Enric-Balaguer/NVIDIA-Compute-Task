#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <filesystem>
#include <omp.h>
#include <atomic>

struct InputData {
    int homes;
    int max;
    std::vector<int> pieces;
};

InputData get_data(const std::string& filename);

/*
OpenMP Solution. Main idea:
    1. Acquire data: homes, max, pieces. I choose not to attempt to parallelize this process using OpenMP because:
        - The potential speedup (parallelizing char to int conversion), might be limited by I/O operations
        - The input is small (maximum lines: 10,000+2)
        - Could use memory mapped files if amount of lines were much bigger, but I think for small files like these,
        I would be overcomplicating it.

    2. I tried parallelizing this with simple #pragma omp for loops but couldn't quite nail the exact same solution
    as in serial solution due to the need to break the loop if candy acquired for a starting house == max_candy

    3. So instead, I create a team of threads, with each one having their own private solution.

    4. Then use an atomic index, that can only be increased atomically, so that each thread computes the same
    iteration as in serial solution.

    5. The scheduling of the threads is dynamic, which is good! Some iterations might take a lot more time than others.

    6. Once finished for loop, save thread specific solutions to pre-initialized shared memory array.
    However, if max_candy is encountered at any index before that, break the loop! With this setup, I allow all other 
    threads to finish their current iteration instead of just stop computation of its current iteration.
    This is important! As breaking the other thread's current loop iteration might lead to non-accurate answers. 

    7. Iterate through the pre-initialized shared memory array and acquire max solution.
*/

int main(int argc, char* argv[]) {
    std::string filename = "input.txt";
    if (argc > 1) {
        filename = argv[1];
    }

    InputData data = get_data(filename);
    int homes = data.homes;
    int max = data.max;
    std::vector<int> pieces = data.pieces;

    int num_threads = omp_get_max_threads(); //Even if we are allocated less than max_threads, solution still works.
    std::vector<std::vector<int>> thread_solutions(num_threads, std::vector<int>(3, -1));
    std::atomic<bool> stop(false);
    int idx = 0;

    #pragma omp parallel 
    {
        int thread_id = omp_get_thread_num();
        std::vector<int>& solution_final = thread_solutions[thread_id];

        while (true) {
            if (stop.load(std::memory_order_acquire)) break;

            int i;
            #pragma omp atomic capture
            i = idx++; 
            if (i >= homes) break;

            std::vector<int> solution_i;
            if (pieces[i] > max) {
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
                stop.store(true, std::memory_order_release); // Trigger cancellation
            }
            solution_final = solution_i[0] > solution_final[0] ? solution_i : solution_final;
        }
        thread_solutions[thread_id] = solution_final; // We encounter False Sharing here, but can be ignored due to my limited amount of threads
                                                      // If amount of available threads was big, we should improve this bit
    }

    // Find the best solution among all threads
    std::vector<int> best_solution = {-1, -1, -1};
    for (std::vector<int>& solution : thread_solutions) {
        if (solution[0] >= best_solution[0]) {
            if (solution[0] == best_solution[0] && solution[1] < best_solution[1]) {
                best_solution = solution;
            }
            else if (solution[0] > best_solution[0]){
                best_solution = solution;
            }
        }
    }

    if (best_solution[0] == -1) {
        std::cout << "There are no solutions." << std::endl;
    }
    else {
        std::cout << "Start at home " << best_solution[1]+1 << " and go to home " << best_solution[2]+1 
                  << " getting "<< best_solution[0] << " pieces of candy." << std::endl;
    }
    
    return 0;
}

InputData get_data(const std::string& filename) {
    std::vector<int> data;
    InputData result{0, 0, {}};

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