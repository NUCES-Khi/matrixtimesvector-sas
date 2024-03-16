# Assignment 1
## Team Members
|std_id|Name|
|--------|-|
|k21-4703|Ali Raza|
|k21-4736|Syed Saadullah Hussaini|
|k21-3100|Muhammad Sameed|
## Output Screenshots


![image](https://github.com/NUCES-Khi/matrixtimesvector-sas/assets/88710028/cdee955a-c105-4510-8434-7e7d878407ea)

Information about the Results data:
![image](https://github.com/NUCES-Khi/matrixtimesvector-sas/assets/88710028/f827e0af-4656-48da-9883-8e15b01e39da)

Descriptive statistics for the 'Time taken' column:
![image](https://github.com/NUCES-Khi/matrixtimesvector-sas/assets/88710028/a0107772-a0e6-4222-8698-2c3a574f57b8)

Average Time Taken by each Matrix Size:
![image](https://github.com/NUCES-Khi/matrixtimesvector-sas/assets/88710028/a7d6eea8-a138-4e23-a882-6372bb67422c)

Time Taken vs. Matrix Size:
![image](https://github.com/NUCES-Khi/matrixtimesvector-sas/assets/88710028/113fb729-0b3f-4d9c-9028-05cb1e02523f)

Avg. Time Taken by Task file and Matrix Size:
![image](https://github.com/NUCES-Khi/matrixtimesvector-sas/assets/88710028/d3a0918b-6a6f-4bf5-b9fc-fff3f725175c)


## Results and Analysis

## Analysis of Matrix-Vector Multiplication Performance

This analysis examines various implementations for matrix-vector multiplication using two graphs.

**Graph 1: Time vs. Matrix Size**

* Execution time increases with matrix size for all implementations due to growing computations.
* Sequential implementation is slowest (limited to one core).
* OpenMP implementations generally outperform sequential, with tiling impact unclear for these matrix sizes.
* MPI implementations show varying performance. Interestingly, the naive MPI version might be faster than the tiled version for the studied sizes, suggesting potential tiling overhead.

**Optimal implementation depends on matrix size and resources.** For smaller sizes, sequential might suffice. For larger sizes, parallel implementations (OpenMP or MPI) are necessary. While tiled MPI often excels, here the naive MPI version might be fastest.

**Graph 2: Average Time by Task File and Matrix Size**

* Similar to Graph 1, execution time increases with matrix size for all task files.
* Variation in execution time exists between task files for the same size due to factors like algorithms or task distribution.
* Based on the graph, `mXv_naive_mpi_task_4` appears to be the fastest overall, suggesting the naive MPI implementation with task number 4 offers the most efficient approach for these matrix sizes.

**Overall, the naive MPI implementation, particularly `mXv_naive_mpi_task_4`, might be most efficient for the studied sizes. Further investigation is needed to understand the performance variation and why naive MPI outperforms tiled MPI here.**


## Major Problems Encountered
1. Issue 1: Due to memory constraints, I couldn't run the MPI program over a huge number of matrix sizes.
   ![image](https://github.com/NUCES-Khi/matrixtimesvector-sas/assets/88710028/3ee16ef8-5e06-4e3b-bf49-bb882f5c1674)

    - Solution1: Reduced the number of matrixes
    - Solution2: Tried to use MPI_Scatterv and MPI_Gatherv, but I was unable to implement these fucntions.
    - Solution3: Check possible memory leaks. ![image](https://github.com/NUCES-Khi/matrixtimesvector-sas/assets/88710028/4325b3f1-281b-4c3d-a342-00fe5003bb70)

    - **Couldn't Resolved**
