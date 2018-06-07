/*
 * wrr1.c
 *
 *  Created on: Jun 7, 2018
 *      Author: admin
 */


#include <stdio.h>
#include <pthread.h>
#include <assert.h>

//pthread_mutex_t m1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t m, m1;

void *main1_thd (void *arg)
{
   (void) arg;
   int ret;
   ret = pthread_mutex_lock(&m);
   assert(ret == 0);

   ret = pthread_mutex_unlock(&m);
   assert(ret == 0);

   ret = pthread_mutex_lock(&m1);
   assert(ret == 0);

   ret = pthread_mutex_unlock(&m1);
   assert(ret == 0);


   return arg;
}

void *main2_thd (void *arg)
{
   (void) arg;
   int ret;


   printf ("threadddd!\n");
   ret = pthread_mutex_lock(&m);
   assert(ret == 0);

   ret = pthread_mutex_unlock(&m);
   assert(ret == 0);

   return arg;
}

void *main3_thd (void *arg)
{
   (void) arg;
   int ret;


   printf ("threadddd!\n");
   ret = pthread_mutex_lock(&m1);
   assert(ret == 0);

   ret = pthread_mutex_unlock(&m1);
   assert(ret == 0);

   return arg;
}

int main (int argc, char ** argv)
{
   int ret;
   pthread_t th[3];
   pthread_mutex_init(&m1, NULL);
   pthread_mutex_init(&m, NULL);

   (void) argc;
   (void) argv;

   ret = pthread_create (th + 0, 0, main2_thd, 0);
   assert (ret == 0);
   ret = pthread_create (th + 1, 0, main2_thd, 0);
   assert (ret == 0);

   ret = pthread_mutex_lock(&m);
   assert(ret == 0);

   ret = pthread_mutex_unlock(&m);
   assert(ret == 0);

//   ret = pthread_mutex_lock(&m1);
//   assert(ret == 0);
//
//   ret = pthread_mutex_unlock(&m1);
//   assert(ret == 0);

   ret = pthread_join (th[0], 0);
   assert (ret == 0);

   ret = pthread_join (th[1], 0);
   assert (ret == 0);
   return 0;
}


