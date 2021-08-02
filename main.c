#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <math.h>
#include <sys/time.h>
int TotalRow,TotalColumn,aRow,aColumn,bRow,bColumn;
int** Mat; // matrix for calculations
int** aMat; //first matrix
int** bMat; //second matrix
struct arg_struct  //struct tor multithreading per element
    {  int thread_row;
        int thread_column;};
struct argrow_struct //struct tor multithreading per row
    {int thread_row;};

void free_mat(int rows, int **mat){  //function to free allocation of a matrix
    int i=0;
    for(i=0;i<rows;i++)
        free(mat[i]);
    free(mat);}


int** allocate_mat(int r, int c){  // function to allocate a matrix given number of rows and number of columns
    int **arr = malloc(r * sizeof(*arr));
	for(int i = 0; i < r; i++)
	{   arr[i] = malloc(c * sizeof(**arr));
	    if(!arr[i])
	    {   for(int j = 0; j < i; j++)
	        {free(arr[j]);}
	        free(arr);
	        exit(-1);}}
    return arr;
}


void writetofile(FILE* fp){   // function to write into the output file
    fprintf(fp, "row=%d col=%d\n",aRow,bColumn );  // first line in the output file shows the dimensions
    for (int Row = 0; Row < aRow; Row++)  // output matrix into file
    {       for (int Column = 0; Column < bColumn; Column++)
                    {fprintf(fp,"%d\t", Mat[Row][Column]);}
                fprintf(fp, "\n" );}
    fprintf(fp, "\n" );
    fclose(fp);
}


void openfile(FILE* file){  // function to open a file and read into a matrix
    fscanf(file, "row=%d col=%d",&TotalRow,&TotalColumn);  // get dimensions
    Mat=allocate_mat(TotalRow,TotalColumn);
    int n,m;
    for(n=0;n<TotalRow;n++){  // read the matrix
        for(m=0;m<TotalColumn;m++)
            {fscanf(file, "%d",&Mat[n][m]);}}  //scan element
    fclose(file);}


void read_matrices(FILE* file1, FILE* file2){ // function to read matrices
    openfile(file1); //open first input file
    aMat=Mat;  //store first matrix information
    aRow=TotalRow;
    aColumn=TotalColumn;
    openfile(file2);  //open second input file
    bMat=Mat;      //store second matrix information
    bRow=TotalRow;
    bColumn=TotalColumn;}


void noMultithread(FILE* fp){   // function for single thread multipication
    int Row,Column;
    Mat=allocate_mat(aRow,bColumn);  //allocate matrix for result
    for(Row=0; Row < aRow; Row++){  //calculate result
        for(Column=0; Column < bColumn; Column++){
            Mat[Row][Column] = 0;
            for(int k=0; k < aColumn; k++){
                Mat[Row][Column] += aMat[Row][k] * bMat[k][Column];}
        }    }
    writetofile(fp);  // write result into the output file
    }


void* multiply_element(void *arguments)  //runner method for threads created by multithread_rowcol()
{   struct arg_struct *args = arguments;
    int sum=0;  //sum of the element
    for (int i = 0; i < bRow; i++) //multiply
        {sum = sum + (aMat[args -> thread_row][i]*bMat[i][args -> thread_column]);}
    Mat[args -> thread_row][args -> thread_column] = sum;
    pthread_exit(NULL); //exit from thread
}


void multithread_rowcol( FILE* fp ){ //function for multithreaded multiplication per element
    pthread_t  *threads;
    struct arg_struct *args; // arguments for pthread_create
    Mat=allocate_mat(aRow,bColumn);  //allocate matrix
    threads = (pthread_t *) malloc((aRow*bColumn)* sizeof(pthread_t)); //array of threads with result matrix dimensions
    args=( struct arg_struct *) malloc((aRow*bColumn)* sizeof( struct arg_struct)); //array of argumens with result matrix dimensions
    int num=0; //counter for threads
    int Row,Column;
    for (Row = 0; Row < aRow; Row++) //multiply
    {   for (Column = 0; Column < bColumn; Column++)
        {   args[num].thread_row = Row; //set arguments with the current data to pass it to thread
            args[num].thread_column = Column;
            if(pthread_create(&threads[num], NULL, multiply_element,(void *)&args[num] ) <0)  //create a thread and return error if failed upon creation
                {   perror("error creating thread!");
                    exit(-1);}
            num++;} //increment counter
    }
    int i;
    for (i = 0; i < (aRow*bColumn); i++)   //block main thread until all the treads end
        {pthread_join(threads[i], NULL);}
    free(args);  //free data
    free(threads);
    writetofile(fp); // write result into the output file
}


void * multiply_row(void *arguments){ //runner method for threads created by multithread_row()
    struct arg_struct *args = arguments;
    for (int i = 0; i < bColumn; i++) //calculate row
    {   Mat[args -> thread_row][i]=0; //initialize element to zero
        for (int j = 0; j < aColumn; j++)
        {   Mat[args -> thread_row][i]= Mat[args -> thread_row][i] + (aMat[args -> thread_row][j]*bMat[j][i]);}
        }
    pthread_exit(NULL); //exit from thread
}


void multithread_row(FILE* fp ){ //function for multithreaded multiplication per row
    pthread_t  *threads;
    struct argrow_struct *args; // arguments for pthread_create
    Mat=allocate_mat(aRow,bColumn); //allocate matrix
    threads = (pthread_t* ) malloc(aRow* sizeof(pthread_t)); //array of threads with dimenesion of aRow
    args=( struct argrow_struct *) malloc(aRow* sizeof( struct argrow_struct)); //array of arguments with dimenesion of aRow
    int num=0; //counter for threads
    int Row;
    for (Row = 0; Row < aRow; Row++) //multiply
    {   args[num].thread_row = Row; //set arguments with the current data to pass it to thread
        if(pthread_create(&threads[num], NULL, multiply_row,(void *)&args[num] ) <0) //create a thread and return error if failed upon creation
        {   perror("error creating thread!");
            exit(-1);}
        num++;} //increment counter
    int i;
    for (i = 0; i < (aRow); i++) //block main thread until all the treads end
    {pthread_join(threads[i], NULL);}
    free(args); //free data
    free(threads);
    writetofile(fp); // write result into the output file
}


int main(int argc,char* argv[])
{   FILE* file1;
    FILE* file2;
    FILE* file3;
    aColumn=0;
    bRow=0;
    switch (argc)  //check inputs
   {
       case 1: file1 = fopen("a.txt", "r"); //if no inputs found use defaults a.txt, b.txt and c.txt
        file2 = fopen("b.txt", "r");
        file3 = fopen("c.txt", "w");
               break;
       case 2: if (!(file1 = fopen(argv[1], "r"))) { //if one input found use it as the first matrix and use the rest b.txt and c.txt
            perror(argv[1]);
            exit(1);}
            file2 = fopen("b.txt", "r");
            file3 = fopen("c.txt", "w");
                break;
       case 3: if (!(file1 = fopen(argv[1], "r"))) {  //if two inputs found use them as the first and second matrices and use c.txt for output
            perror(argv[1]);
            exit(1);}
        if (!(file2 = fopen(argv[2], "r"))) {
            perror(argv[1]);
            exit(1);}
            file3 = fopen("c.txt", "w");
               break;
       case 4: if (!(file1 = fopen(argv[1], "r"))) { //if three inputs found use them as the first and second matrices and output
            perror(argv[1]);
            exit(1);}
        if (!(file2 = fopen(argv[2], "r"))) {
            perror(argv[1]);
            exit(1);}
        if (!(file3 = fopen(argv[3], "w"))) {
            perror(argv[1]);
            exit(1);}
               break;
       default:
                break;
   }

   read_matrices(file1,file2); //read files
   if(aColumn==0 || bRow==0){ //check if the matrices are empty
        printf("ENCOUNTERED EMPTY MATRICES\n");
        return 0;}
   if(aColumn != bRow){ //check for dimensions
        printf("invalid dimensions\n");
        return 0;}



   struct timeval stop, start; //to calculate time
//------------------------------------first method-----------------------------------------------------------------------
    gettimeofday(&start, NULL);
    noMultithread(file3);
    gettimeofday(&stop, NULL); //end checking time
    printf("Seconds taken by single thread %lu\n", stop.tv_sec - start.tv_sec);
    printf("Microseconds taken by single thread: %lu\n", stop.tv_usec - start.tv_usec);
    printf("Number of threads by single thread: 1\n");
    printf("\n");
//-------------------------------------second method-----------------------------------------------------------------------
    if(argc< 4){file3=fopen("c.txt", "a");}
    else{file3 = fopen(argv[3], "a");}
    gettimeofday(&start, NULL);
    multithread_row(file3);
    gettimeofday(&stop, NULL); //end checking time
    printf("Seconds taken by multithreaded per row %lu\n", stop.tv_sec - start.tv_sec);
    printf("Microseconds taken by multithreaded per row: %lu\n", stop.tv_usec - start.tv_usec);
    printf("Number of threads by multithreaded per row: %i\n",aRow+1);
    printf("\n");
//------------------------------------third method----------------------------------------------------------------------
    if(argc < 4){file3=fopen("c.txt", "a");}
    else{file3 = fopen(argv[3], "a");}
    gettimeofday(&start, NULL);
    multithread_rowcol(file3);
    gettimeofday(&stop, NULL); //end checking time
    printf("Seconds taken by multithreaded per element %lu\n", stop.tv_sec - start.tv_sec);
    printf("Microseconds taken by multithreaded per element: %lu\n", stop.tv_usec - start.tv_usec);
    printf("Number of threads by multithreaded per element: %i\n",aRow* bColumn+1);
    printf("\n");
//----------------------------------------------------------------------------------------------------------------------------
    free_mat(aRow,aMat); //free matrices
    free_mat(bRow,bMat);
    free_mat(aRow,Mat);
    return 0;
}
