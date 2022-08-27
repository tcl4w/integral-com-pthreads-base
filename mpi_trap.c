#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>

const long thread_count = 8;
double GLOBAL_MUTEX_SUM = 0;
double GLOBAL_SEM_SUM = 0;
double GLOBAL_BUSY_SUM = 0;

pthread_mutex_t mutex;
sem_t semaphore;

int wait_flag = 0;

double a = 1.0, b = 5.0, h;      
int n = 10000000;      

double f(double x);
double serial_trapezoid();
void* mutex_trapezoid(void* rank);
void* sem_trapezoid(void* rank);
void mutex_main();
void sem_main();


// MACROS provided by Dr. Karlsson for distributing work to threads
#define BLOCK_LOW(id,p,n) ((id)*(n)/(p))
#define BLOCK_HIGH(id,p,n)  (BLOCK_LOW((id)+1,p,n)-1)



int main( int argc, char **argv )

{
  double integral;  

  h = (b-a)/n;
  integral = serial_trapezoid();

  printf("With n = %d trapezoids, our estimate\n", n);
  printf("of the integral from %f to %f = %.15e\n\n",
    a, b, integral);

  mutex_main();
  sem_main();

  return 0;
}

double serial_trapezoid()
{
  double integral;
  int i;

  integral = (f(a)+f(b))/2.0;

  for( i=1; i <= n-1; i++)
  {
    integral += f(a+(i*h));
  }
  integral = integral*h;

  return integral;
}


void* mutex_trapezoid(void* rank)
{ 
  long thread_rank = (long)rank;
  double local_sum = 0.0;
  long long i;
  int special_case = (int)thread_rank;


  long long my_first_i = BLOCK_LOW(thread_rank, thread_count, n); //AAAAAA
  long long my_last_i = BLOCK_HIGH(thread_rank, thread_count, n);

  if( special_case == 1)
  {
    local_sum += (f(a)+f(b))/2.0;
  }

  for( i= my_first_i; i <= my_last_i; i++)
  {
    local_sum += f(a+(i*h));
  }
  local_sum = local_sum * h;

  pthread_mutex_lock(&mutex);
  GLOBAL_MUTEX_SUM += local_sum;
  pthread_mutex_unlock(&mutex);

  return NULL;
 
}


void* sem_trapezoid(void *rank)
{
  long thread_rank = (long)rank;
  double local_sum = 0.0;
  long long i;
  int special_case = (int)thread_rank;

  // allocate a chunk of work to the thread
  long long my_first_i = BLOCK_LOW(thread_rank, thread_count, n);
  long long my_last_i = BLOCK_HIGH(thread_rank, thread_count, n);

  if( special_case == 1)
  {
    local_sum += (f(a)+f(b))/2.0;
  }

  // Karlsson's trapezoid code
  for( i= my_first_i; i <= my_last_i; i++)
  {
    local_sum += f(a+(i*h));
  }
  local_sum = local_sum * h;

  // update the global sum
  sem_wait(&semaphore);
  GLOBAL_SEM_SUM += local_sum;
  sem_post(&semaphore);

  return NULL;
}


void mutex_main()
{
  pthread_t* thread_handles;


  pthread_mutex_init(&mutex, NULL);
  thread_handles = malloc( thread_count * sizeof(pthread_t));

  long thread;
  for( thread=0; thread < thread_count; thread++)
    pthread_create( &thread_handles[thread], NULL, mutex_trapezoid, 
      (void*) thread );

  for( thread=0; thread < thread_count; thread++)
    pthread_join( thread_handles[thread], NULL);

  free(thread_handles);
  pthread_mutex_destroy(&mutex);

  printf("MUTEX\n");
  printf("With n = %d trapezoids, our estimate\n", n);
  printf("of the integral from %f to %f = %.15e\n\n",
    a, b, GLOBAL_MUTEX_SUM);  
}


void sem_main()
{
    pthread_t* thread_handles;

  sem_init(&semaphore, 0, 1);
  thread_handles = malloc( thread_count * sizeof(pthread_t));


  long thread;
  for( thread=0; thread < thread_count; thread++)
    pthread_create( &thread_handles[thread], NULL, sem_trapezoid, 
      (void*) thread);


  for( thread=0; thread < thread_count; thread++)
    pthread_join( thread_handles[thread], NULL);

  free(thread_handles);
  sem_destroy(&semaphore);

  printf("SEMAPHORE\n");
  printf("With n = %d trapezoids, our estimate\n", n);
  printf("of the integral from %f to %f = %.15e\n\n",
    a, b, GLOBAL_SEM_SUM);  
}




double f(double x)
{
  return x*x;
}
