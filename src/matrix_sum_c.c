

// mutex counter variable or similar that states which next row to take for the thread.
// a thread completes one row and then reads the mutex var and then computes that row. 
// thread incrememnts the variable.
// when the variable has reached "size" we stop. 
// all the results for every single row needs to be either stored into an array
// OR 
// have another mutex variable for min, max, total that they can update after completing a row.

// if we have a sort of while loop in every thread they can check for a condition and terminate
// when every row or task has been processed. 

// remember that it is a extension of task B) so the value needs to be returned and each thread
// keeps track of its total sum and min and max values. easy