#include "philo.h"

long get_time_ms(void)
{
    struct timeval tv;

    gettimeofday(&tv, NULL);
    return ((long)(tv.tv_sec * 1000) + (long)(tv.tv_usec / 1000));
}

void ft_usleep(long ms)
{
    long start;

    start = get_time_ms();
    while ((get_time_ms() - start) < ms)
        usleep(50);
}

int is_simulation_end(t_data *data)
{
    int ret;

    pthread_mutex_lock(&data->end_mutex);
    ret = data->simulation_end;
    pthread_mutex_unlock(&data->end_mutex);
    return ret;
}

void print_status(t_philo *philo, char *msg)
{
    long ts;

    pthread_mutex_lock(&philo->data->print_mutex);
    if (!philo->data->simulation_end)
    {
        ts = get_time_ms() - philo->data->start_time;
        printf("%ld %d %s \n", ts, philo->id, msg);
    }
    pthread_mutex_unlock(&philo->data->print_mutex);
}
