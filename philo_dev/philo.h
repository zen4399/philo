#ifndef PHILO_H
#define PHILO_H

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

typedef struct s_philo t_philo;

typedef struct s_data
{
    int num_philo;
    long time_to_die;
    long time_to_eat;
    long time_to_sleep;
    int must_eat_count;
    long start_time;
    int simulation_end;
    int full_count;
    pthread_mutex_t print_mutex;
    pthread_mutex_t end_mutex;
    pthread_mutex_t full_mutex;
    pthread_mutex_t *forks;
    t_philo *philos;
} t_data;

struct s_phile
{
    int id;
    int eat_count;
    long last_eat_time;
    pthread_t thread;
    pthread_mutex_t *left_fork;
    pthread_mutex_t *right_fork;
    pthread_mutex_t eat_mutex;
    t_data *data;
};

int parse_args(int argc, char **argv, t_data *data);
t_philo *init_data(t_data *data);
void cleanup(t_data *data, t_philo *philos);

void *philo_routine(void *arg);

void *monitor_routine(void *arg);

#endif
