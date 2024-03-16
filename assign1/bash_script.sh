#!/bin/bash

# Define the sizes to test
sizes=(64 128 256 512 1024 2048 4096 8192)

# Define the tile size for tiled programs
tile_size=1

# Initialize a variable to store the cumulative duration
cumulative_duration=0
test_count=0

# Function to run a program and record its execution time
run_and_time() {
    local program=$1
    local size=$2
    local output_file=$3
    local extra_arg=$4
    local is_mpi=$5

    # Run the program and time it
    local start_time=$(date +%s.%N)
    if [[ "$is_mpi" == "yes" ]]; then
        mpirun -np 4 ./$program $size $size $extra_arg # Adjust -np (number of processes) as needed
    else
        if [[ -n "$extra_arg" ]]; then
            ./$program $size $size $extra_arg
        else
            ./$program $size $size
        fi
    fi
    local end_time=$(date +%s.%N)
    
    # Calculate the duration
    local duration=$(echo "$end_time - $start_time" | bc)
    
    # Update cumulative duration and test count
    cumulative_duration=$(echo "$cumulative_duration + $duration" | bc)
    ((test_count++))
    
    # Calculate the average duration so far
    local average_duration=$(echo "scale=2; $cumulative_duration / $test_count" | bc)
    
    # Append the result to the output file
    echo "$test_count, $program, $size, $duration, $average_duration" >> $output_file
}

# Main loop to run each program with each size
for size in "${sizes[@]}"; do
    # Run non-tiled programs
    for program in "mXv_task02" "mXv_omp_naiv_task_03"; do
        for i in {1..10}; do
            run_and_time $program $size "results.csv" "" "no"
        done
    done
    # Run MPI non-tiled program
    for program in "mXv_mpi_task_4"; do
        for i in {1..10}; do
            run_and_time $program $size "results.csv" "" "yes"
        done
    done
    # Run tiled programs with a constant tile size
    for program in "mXv_omp_tiled_Task05" "mXv_tiled_mpi_task_6"; do
        for i in {1..10}; do
            run_and_time $program $size "results.csv" $tile_size "no"
        done
    done
done

# Inform the user that the script has finished running
echo "Benchmarking completed. Results saved to results.csv."
