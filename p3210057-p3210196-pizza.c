#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include "p3210057-p3210196-pizza.h"
#include <time.h>
#include <unistd.h>

pthread_mutex_t mutexTel, mutexPrep, mutexOven, mutexDelivery, mutexRev, mutexMess, mutexService, mutexCooling;
pthread_cond_t condTel, condOven, condDelivery, condPrep, condRev, condMess, condService, condCooling;
int av_tel = n_tel, av_c = n_cook , av_o = n_oven, av_d = n_deliverer, rev = 0, ord_m, ord_p, ord_s, num_of_unsucc_ord = 0, num_of_succ_ord = 0, max_time_for_succ_ord, max_time_of_cooling_ord, revenue_while_variable = 1, cooling_time = 1, serving_time = 1, screen = 1;
long  max_cool = 0, max_serv = 0;
float avg_cool, avg_serv;

typedef struct{
	int seed;
	int id;
}Thread_args;

void* routine(void* args) {
    //order
    struct timespec start;
    Thread_args* arg = (Thread_args*)args;
    clock_gettime(CLOCK_REALTIME, &start);
    //tel
    if (av_tel == 1){
   		pthread_mutex_lock(&mutexTel);
    }
    while (av_tel == 0) {
       	pthread_cond_wait(&condTel, &mutexTel);
    }
    av_tel--;
    int id = arg->id;
    int seed = arg->seed;
    int pizzas = rand_r(&seed) % n_orderhigh + n_orderlow;
    int pepperoni = 0;
    int margarita = 0; 
    int special = 0;
    int prob;
    for (int i = 0; i <= pizzas; i++) {
        prob = rand_r(&seed) % 100 + 1;
        if (prob <= Pp) {
            pepperoni++;
        } else if (prob <= Pp + Pm) {
            margarita++;
        } else {
            special++;
        }
    }
    int pay = rand_r(&seed) % t_paymenthigh + t_paymentlow;
    sleep(pay);
    prob = rand_r(&seed) % 100 + 1;
    if (prob <= 5) {
        pthread_mutex_lock(&mutexMess);
        while (screen != 1){
            pthread_cond_wait(&condMess, &mutexMess);
        }
        screen--;
        printf("Order no. %d failed\n", id);
        screen++;
        pthread_cond_signal(&condMess);
        pthread_mutex_unlock(&mutexMess);
        num_of_unsucc_ord++;
        pthread_exit(NULL);
    }
    else{
        pthread_mutex_lock(&mutexMess);
        while (screen != 1) {
            pthread_cond_wait(&condMess, &mutexMess);
        }
        screen--;
        printf("Order no. %d succeeded\n", id);
        screen++;
        pthread_cond_signal(&condMess);
        pthread_mutex_unlock(&mutexMess);
        pthread_mutex_lock(&mutexRev);
        while (revenue_while_variable != 1) {
            pthread_cond_wait(&condRev, &mutexRev);
        }
        revenue_while_variable--;
        rev = rev + margarita * Cm + pepperoni * Cp + special * Cs;
        ord_m += margarita;
        ord_p += pepperoni;
        ord_s += special;
        num_of_succ_ord++;
        revenue_while_variable++;
        pthread_cond_signal(&condRev);
        pthread_mutex_unlock(&mutexRev);
    }
    av_tel++;
    pthread_cond_signal(&condTel);
    pthread_mutex_unlock(&mutexTel);

    //prep
    if (av_c == 1){
   		pthread_mutex_lock(&mutexPrep);
    }
    while (av_c == 0) {
       	pthread_cond_wait(&condPrep, &mutexPrep);
    }
    av_c--;
    
    sleep(t_prep * pizzas);
    if (av_o == pizzas){
    	pthread_mutex_lock(&mutexOven);
    }
    while (av_o < pizzas) {
       	pthread_cond_wait(&condOven, &mutexOven);
    }
    av_o -= pizzas;
    av_c++;
    pthread_cond_signal(&condPrep);
    pthread_mutex_unlock(&mutexPrep);

	
    //oven
    sleep(t_bake);
   
    av_o += pizzas;
    pthread_cond_broadcast(&condOven);
    pthread_mutex_unlock(&mutexOven);

    //deliver
    if (av_d == 1){
    
        pthread_mutex_lock(&mutexDelivery);

    }
    while (av_d == 0) {
       	pthread_cond_wait(&condDelivery, &mutexDelivery);
    }
    sleep(t_pack * pizzas);
    struct timespec finish1;
    clock_gettime(CLOCK_REALTIME, &finish1);
    long min_prep = (finish1.tv_sec - start.tv_sec);
    pthread_mutex_lock(&mutexMess);
    while (screen != 1) {
        pthread_cond_wait(&condMess, &mutexMess);
    }
    screen--;
    printf("Order with order id %d was prepared in %ld minutes.\n", id, min_prep);
    screen++;
    pthread_cond_signal(&condMess);
    pthread_mutex_unlock(&mutexMess);
  
    av_d--;
    int del = rand_r(&seed) % t_delhigh + t_dellow;
    sleep(del);
    struct timespec finish2;
    clock_gettime(CLOCK_REALTIME, &finish2);
    long min = (finish2.tv_sec - start.tv_sec);
    pthread_mutex_lock(&mutexMess);
    while (screen != 1) {
       	pthread_cond_wait(&condMess, &mutexMess);
    }
    screen--;
    printf("Order with order id %d was delivered in %ld minutes.\n", id, min);
    screen++;
    pthread_cond_signal(&condMess);
    pthread_mutex_unlock(&mutexMess);
    sleep(del);
    av_d ++;
    pthread_cond_signal(&condDelivery);
   	pthread_mutex_unlock(&mutexDelivery);
    
    long min1 = min - min_prep;
    pthread_mutex_lock(&mutexService);
    while (serving_time != 1) {
       	pthread_cond_wait(&condService, &mutexService);
    }
    serving_time--;
    if (max_serv < min) {
       	max_serv = min;
    }
    serving_time++;
    pthread_cond_signal(&condService);
    pthread_mutex_unlock(&mutexService);
    pthread_mutex_lock(&mutexCooling);
    while (cooling_time != 1) {
       	pthread_cond_wait(&condCooling, &mutexCooling);
    }
    cooling_time--;
    if (max_cool < min_prep) {
       	max_cool = min_prep;
    }
    cooling_time++;
    pthread_cond_signal(&condCooling);
    pthread_mutex_unlock(&mutexCooling);
    avg_cool += min1;
    avg_serv += min;
}


int main(int argc, char* argv[])
{
    if (argc != 3) {
        printf("Wrong number of arguments\n");
        exit(-1);
    }

    int n_cust = atoi(argv[1]);
    if (n_cust <= 0) {
        printf("The number of threads I have to create should be positive. You gave me %d.\n", n_cust);
        exit(-1);
    }

	unsigned int Seed = atoi(argv[2]);
    int paramArray[n_cust];
    int rc;
    pthread_t threads[n_cust];
    pthread_mutex_init(&mutexTel, NULL);
    pthread_mutex_init(&mutexMess, NULL);
    pthread_mutex_init(&mutexService, NULL);
    pthread_mutex_init(&mutexCooling, NULL);
    pthread_mutex_init(&mutexRev, NULL);
    pthread_mutex_init(&mutexPrep, NULL);
    pthread_mutex_init(&mutexOven, NULL);
    pthread_mutex_init(&mutexDelivery, NULL);

    pthread_cond_init(&condTel, NULL);
    pthread_cond_init(&condCooling, NULL);
    pthread_cond_init(&condService, NULL);
    pthread_cond_init(&condRev, NULL);
    pthread_cond_init(&condOven, NULL);
    pthread_cond_init(&condMess, NULL);
    pthread_cond_init(&condDelivery, NULL);
    pthread_cond_init(&condPrep, NULL);
    for (int t = 1; t <= n_cust; t++) {
        paramArray[t] = t;
        Thread_args* args = (Thread_args*)malloc(sizeof(Thread_args));
        args->id = t;
        args->seed = Seed + t;
        rc = pthread_create(&threads[t], NULL, routine, args);

        if (rc != 0) {
            printf("The return value from pthread_create() was %d!\n", rc);
            exit(-1);
        }
        
        sleep(rand_r(&Seed) % t_orderhigh + t_orderlow); 

    }
    for (int i = 1; i <= n_cust; i++){
        if (pthread_join(threads[i], NULL)!=0){
            printf("An error has occurred.");
        }
    }
    printf("The total revenue of the pizzeria was %d, the unsuccessful orders were %d, the successful were %d, %d margheritas, %d pepperonis and %d specials were sold.\n",
           rev, num_of_unsucc_ord, num_of_succ_ord, ord_m, ord_p, ord_s);
    printf("The average time of servicing an order was %f and the max time for servicing one was %ld.\n", (avg_serv / num_of_succ_ord), max_serv);
    printf("The average time of delivering an order was %f and the max time for delivering one was %ld.\n", (avg_cool / num_of_succ_ord), max_cool);
    
    pthread_mutex_destroy(&mutexTel);
    pthread_mutex_destroy(&mutexCooling);
    pthread_mutex_destroy(&mutexService);
    pthread_mutex_destroy(&mutexRev);
    pthread_mutex_destroy(&mutexMess);
    pthread_mutex_destroy(&mutexOven);
    pthread_mutex_destroy(&mutexPrep);
    pthread_mutex_destroy(&mutexDelivery);

    pthread_cond_destroy(&condTel);
    pthread_cond_destroy(&condCooling);
    pthread_cond_destroy(&condService);
    pthread_cond_destroy(&condRev);
    pthread_cond_destroy(&condMess);
    pthread_cond_destroy(&condOven);
    pthread_cond_destroy(&condDelivery);
    pthread_cond_destroy(&condPrep);
    return 0;
}
