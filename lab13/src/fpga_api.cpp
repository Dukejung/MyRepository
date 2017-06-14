#include "fpga_api.h"
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

#define DATA_SIZE SIZE*(SIZE+1)*sizeof(float) // fpga bram data size

#define min(x,y) (((x)<(y))?(x):(y))

FPGA::FPGA(off_t data_addr, off_t api_addr)
{
    fd_ = open("/dev/mem", O_RDWR);
    data_ = static_cast<float*>(mmap(NULL, DATA_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, fd_, data_addr));
    api_ = static_cast<unsigned int*>(mmap(NULL, sizeof(unsigned int), PROT_READ|PROT_WRITE, MAP_SHARED,fd_, api_addr));
}

FPGA::~FPGA()
{
    munmap(data_, DATA_SIZE );
    munmap(api_, sizeof(unsigned int));
    close(fd_);
}

float* FPGA::matrix(void)
{
	return data_ + SIZE;
}

float* FPGA::vector(void)
{
	return data_;
}

const float* __attribute__((optimize("O0"))) FPGA::run()
{
    *api_ = 0x5555;
    while(*api_ == 0x5555);

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
         //do something
         //matrix 0 case
  //       std::cout<<"l+s "<<largeindex + smallindex << std::endl;
//         std::cout<<"largeindex: " << largeindex << std::endl;
         if(largeindex + smallindex>=M*N){
           mat[counter] = 0;
         }else if((vecindex==M2/SIZE-1)&&(counter%SIZE>=(SIZE-(M2-M)))&&(M2!=M)){
            mat[counter] = 0;           
         }else{
           mat[counter] = large_mat[largeindex + smallindex]; 
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
