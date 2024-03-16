# Assignment 1
## Team Members
|std_id|Name|
|--------|-|
|k21-4703|Ali Raza|
|k21-4736|Syed Saadullah Hussaini|
|k21-3100|Muhammad Sameed|
## Output Screenshots
//-- Add output screenshots here. --//
## Results and Analysis
//-- Show graphs results and charts where necessary and discuss the results and what they signify. --// 
## Major Problems Encountered
1. Issue 1: Due to memory constraints, I couldn't run the MPI program over a huge number of matrix sizes.
   ![image](https://github.com/NUCES-Khi/matrixtimesvector-sas/assets/88710028/3ee16ef8-5e06-4e3b-bf49-bb882f5c1674)

    - Solution1: Reduced the number of matrixes
    - Solution2: Tried to use MPI_Scatterv and MPI_Gatherv, but I was unable to implement these fucntions.
    - **Resolved**
