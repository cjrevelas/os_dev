#include <pthread.h>
#include <stdio.h>
#include <stdatomic.h>

#define ITERATIONS 1000000

atomic_int counter = 0;

void *worker( void *arg )
{
    for ( int i=0; i<ITERATIONS; i++ )
    {
        atomic_fetch_add( &counter, 1 );
    }

    return NULL;
}

int main( void )
{
    pthread_t t1, t2;

    pthread_create( &t1, NULL, worker, NULL );
    pthread_create( &t2, NULL, worker, NULL );

    pthread_join( t1, NULL );
    pthread_join( t2, NULL );

    printf( "counter = %d\n", counter );

    return 0;
}