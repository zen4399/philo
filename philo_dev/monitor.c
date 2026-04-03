#include "philo.h"

static int check_death(t_data *data, int i)
{
    long elapsed;

    pthread_mutex_lock(&data->philos[i].eat_mutex);
    elapsed = get_time_ms() - data->philos[i].last_eat_time;
    pthread_mutex_unlock(&data->philos[i].eat_mutex);
    if (elapsed < data->time_to_die)
        return 0;
    pthread_mutex_lock(&data->end_mutex);
    data->simulation_end = 1;
    pthread_mutex_unlock(&data->end_mutex);
    pthread_mutex_lock(&data->print_mutex);
    printf("%ld %d died\n", get_time_ms() - data->start_time,
           data->philos[i].id);
    pthread_mutex_unlock(&data->print_mutex);
    return 1;
}

static int check_all_full(t_data *data)
{
    int full;

    if (data->must_eat_count == -1)
        return 0;
    pthread_mutex_lock(&data->full_mutex);
    full = data->full_count;
    pthread_mutex_unlock(&data->full_mutex);
    if (full < data->num_philo)
        return 0;
    pthread_mutex_lock(&data->end_mutex);
    data->simulation_end = 1;
    pthread_mutex_unlock(&data->end_mutex);
    return 1;
}

void *monitor_routine(void *arg)
{
    t_data *data;
    int i;

    data = (t_data *)arg;
    while (1)
    {
        i = 0;
        while (i < data->num_philo)
        {
            if (check_death(data, i))
                return NULL;
            i++;
        }
        if (check_all_full(data))
            return NULL;
        usleep(500);
    }
    return NULL;
}
