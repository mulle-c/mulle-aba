//
//  main.c
//  mulle-aba-test
//
//  Created by Nat! on 19.03.15.
//  Copyright (c) 2015 Mulle kybernetiK. All rights reserved.
//
//  Redistributions of source code must retain the above copyright notice, this
//  list of conditions and the following disclaimer.
//
//  Redistributions in binary form must reproduce the above copyright notice,
//  this list of conditions and the following disclaimer in the documentation
//  and/or other materials provided with the distribution.
//
//  Neither the name of Mulle kybernetiK nor the names of its contributors
//  may be used to endorse or promote products derived from this software
//  without specific prior written permission.
//
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
//  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
//  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
//  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
//  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
//  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
//  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
//  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
//  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
//  POSSIBILITY OF SUCH DAMAGE.
//
#include "mulle_aba_storage.h"
#include "mulle_aba.h"

#include <mulle_thread/mulle_atomic.h>
#include <mulle_thread/mulle_thread.h>
#include <stdio.h>
#include <errno.h>


#define PROGRESS     0
#define FOREVER      1
#define LOOPS        (1 + (rand() % 100000))
#define ITERATIONS   400
#define MAX_THREADS  4


#if DEBUG_PRINT
extern void   mulle_aba_print( void);
#else
# define mulle_aba_print()
#endif

static mulle_thread_key_t   timestamp_thread_key;
char  *mulle_aba_thread_name( void);

#pragma mark -
#pragma mark track allocations

#include "pointer_array.h"


struct _pointer_array    allocations;
mulle_thread_mutex_t          alloc_lock;

//void  *test_realloc( void *q, size_t size)
//{
//   void           *p;
//   unsigned int   i;
//   
//   p = realloc( q, size);
//   if( ! p)
//      return( p);
//
//   if( mulle_thread_mutex_lock( &alloc_lock))
//      abort();
//   
//   if( q)
//   {
//      if( p != q)
//      {
//         i = _pointer_array_index( &allocations, q);  // analayzer it's ok, just a pointer comparison
//         assert( i != -1);
//         _pointer_array_set( &allocations, i, p);
//      }
//   }
//   else
//      _pointer_array_add( &allocations, p, realloc);
//   mulle_thread_mutex_unlock( &alloc_lock);
//
//   return( p);
//}


void  *test_calloc( size_t n, size_t size)
{
   void   *p;
   
   p = calloc( n, size);
   if( ! p)
   {
#if MULLE_ABA_TRACE
      abort();
#endif
      return( p);
   }
   
   if( mulle_thread_mutex_lock( &alloc_lock))
      abort();
   _pointer_array_add( &allocations, p, realloc);
   mulle_thread_mutex_unlock( &alloc_lock);
#if MULLE_ABA_TRACE
   fprintf( stderr,  "%s: *** alloc( %p) ***\n", mulle_aba_thread_name(), p);
#endif
   return( p);
}


void  test_free( void *p)
{
   unsigned int   i;
   
   if( ! p)
      return;
   
#if MULLE_ABA_TRACE || MULLE_ABA_TRACE_FREE
   fprintf( stderr,  "%s: *** free( %p) ***\n", mulle_aba_thread_name(), p);
#endif

   if( mulle_thread_mutex_lock( &alloc_lock))
      abort();
   
   i = _pointer_array_index( &allocations, p);
   assert( i != -1);  // if assert, this is a double free or not a malloc block
   _pointer_array_set( &allocations, i, NULL);
   
   mulle_thread_mutex_unlock( &alloc_lock);
   
   free( p);
}


#pragma mark -
#pragma mark global variables

static struct _mulle_aba_linked_list   list;     // common
static mulle_atomic_ptr_t              alloced;  // common


#pragma mark -
#pragma mark reset allocator between tests

static void   reset_memory()
{
   struct  _pointer_array_enumerator   rover;
   void                                *p;

#if MULLE_ABA_TRACE
   fprintf( stderr, "\n================================================\n");
#endif

   rover = _pointer_array_enumerate( &allocations);
   while( (p = _pointer_array_enumerator_next( &rover)) != (void *) -1)
   {
      if( p)
      {
         fprintf( stdout, "*");
#if MULLE_ABA_TRACE
         fprintf( stderr, "%s: leak %p\n", mulle_aba_thread_name(), p);
#endif
#if DEBUG
         abort();
#endif
         free( p);
      }
   }
   _pointer_array_enumerator_done( &rover);

   _pointer_array_done( &allocations, free);
   memset( &allocations, 0, sizeof( allocations));
   
   memset( &alloced, 0, sizeof( alloced));
   memset( &list, 0, sizeof( list));
}



#pragma mark -
#pragma mark run test

static void    run_thread_gc_free_list_test( void)
{
   struct _mulle_aba_free_entry   *entry;
   unsigned long                  i;
   void                           *thread;
   
   thread = mulle_aba_thread_name();
   
   for( i = 0; i < LOOPS; i++)
   {
      entry = (void *) _mulle_aba_linked_list_remove_one( &list);
      if( entry)
      {
#if MULLE_ABA_TRACE
         fprintf( stderr, "%s: reused %p (%p) from %p\n", mulle_aba_thread_name(), entry, entry->_next, &list);
#endif
         assert( ! entry->_link._next);
      }
      else
      {
         entry = test_calloc( 1, sizeof( *entry));
         _mulle_atomic_increment_pointer( &alloced);
#if MULLE_ABA_TRACE            
         fprintf( stderr, "%s: allocated %p (%p)\n", mulle_aba_thread_name(), entry, entry->_next);
#endif
      }
      
      assert( entry);
      entry->_pointer = (void *) i;
      entry->_owner   = thread;

      _mulle_aba_linked_list_add( &list, &entry->_link);
   }
}


void  multi_threaded_test_each_thread()
{
#if PROGRESS
   fprintf( stdout,  "."); fflush( stdout);
#endif
   run_thread_gc_free_list_test();
}


static void   _wait_around( mulle_atomic_ptr_t *n_threads)
{
   // wait for all threads to materialize
   _mulle_atomic_decrement_pointer( n_threads);
   while( _mulle_atomic_read_pointer( n_threads) != 0)
      sched_yield();
}


struct thread_info
{
   char                  name[ 64];
   mulle_atomic_ptr_t    *n_threads;
};


static mulle_thread_rval_t   run_test( struct thread_info *info)
{
   mulle_thread_setspecific( timestamp_thread_key, strdup( info->name));

   _wait_around( info->n_threads);
   multi_threaded_test_each_thread();

   return( 0);
}


static void   finish_test( void)
{
   struct _mulle_aba_free_entry   *entry;
   // remove all from list, so any leak means trouble
   
   while( _mulle_atomic_decrement_pointer( &alloced))
   {
      entry = (void *) _mulle_aba_linked_list_remove_one( &list);
      assert( entry);
      test_free( entry);
   }
}


void  multi_threaded_test( intptr_t n)
{
   int                  i;
   mulle_thread_t       *threads;
   struct thread_info   *info;
   mulle_atomic_ptr_t   n_threads;
   
#if MULLE_ABA_TRACE
   fprintf( stderr, "////////////////////////////////\n");
   fprintf( stderr, "multi_threaded_test( %ld) starts\n", n); 
#endif
   threads = alloca( n * sizeof( mulle_thread_t));
   assert( threads);
   
   n_threads._nonatomic = (void *) n;
   info = alloca( sizeof(struct thread_info) * n);

   for( i = 1; i < n; i++)
   {
      info[ i].n_threads = &n_threads;
      sprintf( info[ i].name, "thread #%d", i);
      
      if( mulle_thread_create( (void *) run_test, &info[ i], &threads[ i]))
         abort();
   }
   
   info[ 0].n_threads = &n_threads;
   sprintf( info[ 0].name, "thread #%d", 0);
   run_test( &info[ 0]);

   for( i = 1; i < n; i++)
      if( mulle_thread_join( threads[ i]))
      {
         perror( "mulle_thread_join");
         abort();
      }
   
   finish_test();
   
#if MULLE_ABA_TRACE
   fprintf( stderr, "%s: multi_threaded_test( %ld) ends\n", mulle_aba_thread_name(), n);
#endif
}


char  *mulle_aba_thread_name( void)
{
   return( mulle_thread_getspecific( timestamp_thread_key));
}


#ifdef __APPLE__
#include <sys/resource.h>

__attribute__((constructor))
static void  __enable_core_dumps(void)
{
    struct rlimit   limit;

    limit.rlim_cur = RLIM_INFINITY;
    limit.rlim_max = RLIM_INFINITY;
    setrlimit(RLIMIT_CORE, &limit);
}
#endif

int   _main(int argc, const char * argv[])
{
   unsigned int   i;
   unsigned int   j;
   int            rval;
   
   srand( (unsigned int) time( NULL));
   
   rval = mulle_thread_mutex_init( &alloc_lock);
   assert( ! rval);
   
   rval = mulle_thread_key_create( &timestamp_thread_key, free);
   assert( ! rval);
   
   rval = mulle_thread_setspecific( timestamp_thread_key, strdup( "main"));
   assert( ! rval);
   
#if MULLE_ABA_TRACE
   fprintf( stderr, "%s\n", mulle_aba_thread_name());
#endif
   

forever:
   reset_memory();

   //
   // if there are leaks anywhere, it will assert in
   // _mulle_aba_storage_done which is called by reset_memory
   // eventually
   //
   
   for( i = 0; i < ITERATIONS; i++)
   {
#if MULLE_ABA_TRACE || PROGRESS
# if MULLE_ABA_TRACE
      fprintf( stderr, "iteration %d\n", i);
# else
      fprintf( stdout, "iteration %d\n", i);
# endif
#endif
      for( j = 1; j <= MAX_THREADS; j += j)
      {
         multi_threaded_test( j);
         reset_memory();
      }
   }

#if FOREVER
   goto forever;
#endif

   mulle_thread_mutex_destroy( &alloc_lock);

   return( 0);
}


#ifndef NO_MAIN
int   main(int argc, const char * argv[])
{
   return( _main( argc, argv));
}
#endif
