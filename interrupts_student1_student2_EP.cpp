/**
 * @file interrupts.cpp
 * @author Sasisekhar Govind
 * @brief template main.cpp file for Assignment 3 Part 1 of SYSC4001
 * 
 * 
 * @author Student 1: <Bhagya Patel, 101324150>
 * @author Student 2: <Name, ID>
 * @brief External Priority Scheduler (No Preemption)
 * 
 * This scheduler uses FCFS (First Come First Serve) as the priority mechanism.
 * Once a process starts running, it continues until:
 * 1. It needs I/O (then goes to WAITING)
 * 2. It completes (then goes to TERMINATED)
 */

#include<interrupts_student1_student2.hpp>

void FCFS(std::vector<PCB> &ready_queue) {
    // MODIFICATION: Changed to priority sort - Lower PID = Higher Priority
    std::sort( 
                ready_queue.begin(),
                ready_queue.end(),
                []( const PCB &first, const PCB &second ){
                    // Sort so lowest PID is at the back (will be popped first)
                    return (first.PID > second.PID); 
                } 
            );
}

std::tuple<std::string /* add std::string for bonus mark */ > run_simulation(std::vector<PCB> list_processes) {

    std::vector<PCB> ready_queue;   //The ready queue of processes
    std::vector<PCB> wait_queue;    //The wait queue of processes
    std::vector<PCB> job_list;      //A list to keep track of all the processes. This is similar
                                    //to the "Process, Arrival time, Burst time" table that you
                                    //see in questions. You don't need to use it, I put it here
                                    //to make the code easier :).

    unsigned int current_time = 0;
    PCB running;

    // Track when process return from I/O
    std::vector<std::pair<int, unsigned int>> io_completion_times; //{PID, completion_time}

    // ADDITION: Track time since last I/O for running process
    unsigned int time_since_last_io = 0;

    //Initialize an empty running process
    idle_CPU(running);

    std::string execution_status;

    //make the output table (the header row)
    execution_status = print_exec_header();

    //Loop while till there are no ready or waiting processes.
    //This is the main reason I have job_list, you don't have to use it.
    while(!all_process_terminated(job_list) || job_list.empty()) {

        //Inside this loop, there are three things you must do:
        // 1) Populate the ready queue with processes as they arrive
        // 2) Manage the wait queue
        // 3) Schedule processes from the ready queue

        // ADDITION: Step 1 - Handle I/O completions first (before new arrivals)
        for(auto it = io_completion_times.begin(); it != io_completion_times.end(); ) {
            if(it->second == current_time) {
                // Find the process in wait_queue
                for(auto wait_it = wait_queue.begin(); wait_it != wait_queue.end(); wait_it++) {
                    if(wait_it->PID == it->first) {
                        // I/O completed - move to ready queue
                        wait_it->state = READY;
                        execution_status += print_exec_status(current_time, wait_it->PID, WAITING, READY);
                        ready_queue.push_back(*wait_it);
                        sync_queue(job_list, *wait_it);
                        
                        wait_queue.erase(wait_it);
                        break;
                    }
                }
                it = io_completion_times.erase(it);
            } else {
                ++it;
            }
        }

        //Population of ready queue is given to you as an example.
        //Go through the list of proceeses
        for(auto &process : list_processes) {
            if(process.arrival_time == current_time) {//check if the AT = current time
                //if so, assign memory and put the process into the ready queue
                // MODIFICATION: Only process if not already assigned
                if(process.state == NOT_ASSIGNED) {
                    assign_memory(process);

                    process.state = READY;  //Set the process state to READY
                    ready_queue.push_back(process); //Add the process to the ready queue
                    job_list.push_back(process); //Add it to the list of processes

                    execution_status += print_exec_status(current_time, process.PID, NEW, READY);
                }
            }
        }

        ///////////////////////MANAGE WAIT QUEUE/////////////////////////
        //This mainly involves keeping track of how long a process must remain in the ready queue
        // NOTE: This is now handled above in Step 1 (I/O completions)
        /////////////////////////////////////////////////////////////////

        // ADDITION: Step 2 - Check if running process has completed (check BEFORE I/O)
        if(running.state == RUNNING && running.remaining_time == 0) {
            execution_status += print_exec_status(current_time, running.PID, RUNNING, TERMINATED);
            terminate_process(running, job_list);
            idle_CPU(running);
            time_since_last_io = 0;
        }

        // ADDITION: Step 3 - Check if running process needs I/O
        if(running.state == RUNNING && running.io_freq > 0) {
            if(time_since_last_io >= running.io_freq) {
                // Process needs I/O
                execution_status += print_exec_status(current_time, running.PID, RUNNING, WAITING);
                running.state = WAITING;
                wait_queue.push_back(running);
                sync_queue(job_list, running);
                
                // Schedule I/O completion
                io_completion_times.push_back({running.PID, current_time + running.io_duration});
                
                idle_CPU(running);
                time_since_last_io = 0;
            }
        }

        //////////////////////////SCHEDULER//////////////////////////////
        FCFS(ready_queue); //example of FCFS is shown here
        
        // ADDITION: Step 4 - Schedule next process if CPU is idle
        if(running.state == NOT_ASSIGNED && !ready_queue.empty()) {
            running = ready_queue.back();
            ready_queue.pop_back();
            
            execution_status += print_exec_status(current_time, running.PID, READY, RUNNING);
            running.state = RUNNING;
            sync_queue(job_list, running);
            time_since_last_io = 0;
        }
        /////////////////////////////////////////////////////////////////

        // ADDITION: Step 5 - Execute one time unit (AFTER all checks)
        if(running.state == RUNNING && running.remaining_time > 0) {
            running.remaining_time--;
            time_since_last_io++;
            sync_queue(job_list, running);
        }

        // ADDITION: Step 6 - Advance time
        current_time++;

        // ADDITION: Safety check to prevent infinite loops
        if(current_time > 10000) {
            std::cerr << "Simulation timeout at time " << current_time << std::endl;
            break;
        }
    }
    
    //Close the output table
    execution_status += print_exec_footer();

    return std::make_tuple(execution_status);
}


int main(int argc, char** argv) {

    //Get the input file from the user
    if(argc != 2) {
        std::cout << "ERROR!\nExpected 1 argument, received " << argc - 1 << std::endl;
        std::cout << "To run the program, do: ./interrutps <your_input_file.txt>" << std::endl;
        return -1;
    }

    //Open the input file
    auto file_name = argv[1];
    std::ifstream input_file;
    input_file.open(file_name);

    //Ensure that the file actually opens
    if (!input_file.is_open()) {
        std::cerr << "Error: Unable to open file: " << file_name << std::endl;
        return -1;
    }

    //Parse the entire input file and populate a vector of PCBs.
    //To do so, the add_process() helper function is used (see include file).
    std::string line;
    std::vector<PCB> list_process;
    while(std::getline(input_file, line)) {
        auto input_tokens = split_delim(line, ", ");
        auto new_process = add_process(input_tokens);
        list_process.push_back(new_process);
    }
    input_file.close();

    //With the list of processes, run the simulation
    auto [exec] = run_simulation(list_process);

    // ADDITION: Extract test case number from input filename
    std::string output_filename = "execution.txt";  // default
    std::string input_filename = file_name;
    
    // Find "test" in the filename and extract the number
    size_t test_pos = input_filename.find("test");
    if(test_pos != std::string::npos) {
        // Extract number after "test"
        size_t num_start = test_pos + 4;  // 4 = length of "test"
        size_t num_end = input_filename.find_first_not_of("0123456789", num_start);
        
        if(num_end == std::string::npos) {
            num_end = input_filename.find(".txt");
        }
        
        if(num_start < input_filename.length()) {
            std::string test_number = input_filename.substr(num_start, num_end - num_start);
            if(!test_number.empty()) {
                output_filename = "execution" + test_number + ".txt";
            }
        }
    }

    write_output(exec, output_filename.c_str());

    return 0;
}