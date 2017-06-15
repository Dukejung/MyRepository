#include "fpga_api.h"

#include <iostream>
#include <cstring>
#define DATA_SIZE SIZE*(SIZE+1) // fpga bram data size

#define min(x,y) (((x)<(y))?(x):(y))

FPGA::FPGA(off_t data_addr, off_t api_addr)
{
    api_ = new unsigned int[SIZE];    // use api_ as tempolar output 
    data_ = new float[DATA_SIZE];	
}

FPGA::~FPGA()
{
    delete[] api_;
    delete[] data_;
}

float* FPGA::matrix(void)
{
	return data_ + SIZE;
}

float* FPGA::vector(void)
{
	return data_;
}

const float* FPGA::run()
{
    float* vec = this->vector();
    float* mat = this->matrix();
    float* out  = reinterpret_cast<float*>(api_);  

    for(int i = 0 ; i < SIZE; ++i)
    {
        out[i] = 0;

        for(int j = 0 ; j < SIZE; ++j){
           out[i] += vec[j] * mat[SIZE*i + j];
	}
    }

	for(int i = 0 ; i < SIZE; ++i)
	{
		data_[i] = out[i];
	}

    return data_;    
}

void FPGA::largeMV(const float* large_mat, const float* input,
		float* output, int M, int N)
{
    float* vec = this->vector(); //64
    float* mat = this->matrix(); //64x64
 
    int N2 = ((N-1)/SIZE +1)*SIZE;
    int M2 = ((M-1)/SIZE +1)*SIZE;
    int vecindex = 0;
    for(vecindex = 0; vecindex < M2/SIZE; vecindex++){
     int matindex = 0;
     for(matindex = 0; matindex < N2/SIZE; matindex++){
       int largeindex = vecindex*SIZE + matindex*M*SIZE;
       int smallindex = 0;
       int counter = 0;
       while(counter<SIZE*SIZE){
         if(largeindex + smallindex>=M*N){
           mat[counter] = 0;
         }else if((vecindex==M2/SIZE-1)&&(counter%SIZE>=(SIZE-(M2-M)))&&(M2!=M)){
            mat[counter] = 0;           
         }else{
           mat[counter]= large_mat[largeindex + smallindex]; 
         }

         smallindex++;
 	 if((counter%SIZE == SIZE-1)){
	    smallindex += M - SIZE;
	  }
	 counter++;
       }
   for(int k=0;k<SIZE;k++){
       int inputindex = vecindex*SIZE+k;
       if(inputindex < M){
         vec[k] = input[inputindex];
       }else{
         vec[k] = 0;
       }
     }
   FPGA::run();         
   for(int k=0;k<SIZE;k++){
     int outputindex = matindex*SIZE+k;
     if(outputindex<N){
       output[outputindex] += vec[k];
     }
     else break;
   } 
  }  
} 

}
