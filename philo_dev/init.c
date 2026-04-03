/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   init.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: izen <izen@student.42tokyo.jp>             +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/03 21:49:51 by izen              #+#    #+#             */
/*   Updated: 2026/04/03 21:49:52 by izen             ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "philo.h"

static int init_philos(t_data *data, t_philo *philos)
{
    int i;

    i = 0;
    while (i < data->num_philo)
    {
        philos[i].id = i + 1;
        philos[i].eat_count = 0;
        philos[i].last_eat_time = data->start_time;
        philos[i].left_fork = &data->forks[i];
        philos[i].right_fork = &data->forks[(i + 1) % data->num_philo];
        philos[i].data = data;
        if (pthread_mutex_init(&philos[i].eat_mutex, NULL))
            return 1;
        i++;
    }
    data->philos = philos;
    return 0;
}

static int init_mutexes(t_data *data)
{
    int i;

    data->forks = malloc(sizeof(pthread_mutex_t) * data->num_philo);
    if (!data->forks)
        return 1;
    i = 0;
    while (i < data->num_philo)
    {
        if (pthread_mutex_init(&data->forks[i], NULL))
            return 1;
        i++;
    }
    if (pthread_mutex_init(&data->print_mutex, NULL))
        return 1;
    if (pthread_mutex_init(&data->end_mutex, NULL))
        return 1;
    if (pthread_mutex_init(&data->full_mutex, NULL))
        return 1;
    return 0;
}

t_philo *init_data(t_data *data)
{
    t_philo *philos;

    data->simulation_end = 0;
    data->full_count = 0;
    data->start_time = get_time_ms();
    philos = malloc(sizeof(t_philo) * data->num_philo);
    if (!philos)
        return NULL;
    if (init_mutexes(data))
    {
        free(philos);
        return NULL;
    }
    if (init_philos(data, philos))
    {
        free(philos);
        return NULL;
    }
    return philos;
}

static void destroy_mutexes(t_data *data, t_philo *philos)
{
    int i;

    i = 0;
    while (i < data->num_philo)
    {
        pthread_mutex_destroy(&philos[i].eat_mutex);
        pthread_mutex_destroy(&data->forks[i]);
        i++;
    }
    pthread_mutex_destroy(&data->print_mutex);
    pthread_mutex_destroy(&data->end_mutex);
    pthread_mutex_destroy(&data->full_mutex);
}

void cleanup(t_data *data, t_philo *philos)
{
    destroy_mutexes(data, philos);
    free(data->forks);
    free(philos);
}
