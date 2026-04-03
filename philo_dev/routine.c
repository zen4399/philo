#include "philo.h"

static void take_forks(t_philo *philo)
{
    if (philo->id % 2 == 0)
    {
        pthread_mutex_lock(philo->right_fork);
        print_status(philo, "has taken a fork");
        pthread_mutex_lock(philo->left_fork);
        print_status(philo, "has taken a fork");
    }
    else
    {
        pthread_mutex_lock(philo->left_fork);
        print_status(philo, "has taken a fork");
        pthread_mutex_lock(philo->right_fork);
        print_status(philo, "has taken a fork");
    }
}

static void put_forks(t_philo *philo)
{
    pthread_mutex_unlock(philo->left_fork);
    pthread_mutex_unlock(philo->right_fork);
}

static void eat(t_philo *philo)
{
    t_data *data;

    data = philo->data;
    take_forks(philo);
    if (is_simulation_end(data))
    {
        put_forks(philo);
        return;
    }
    pthread_mutex_lock(&philo->eat_mutex);
    philo->last_eat_time = get_time_ms();
    pthread_mutex_unlock(&philo->eat_mutex);
    print_status(philo, "is eating");
    ft_usleep(data->time_to_eat);
    philo->eat_count++;
    put_forks(philo);
    if (data->must_eat_count != -1 && philo->eat_count == data->must_eat_count)
    {
        pthread_mutex_lock(&data->full_mutex);
        data->full_count++;
        pthread_mutex_unlock(&data->full_mutex);
    }
}

static void one_philo_routine(t_philo *philo)
{
    pthread_mutex_lock(philo->left_fork);
    print_status(philo, "has taken a fork");
    ft_usleep(philo->data->time_to_die);
    pthread_mutex_unlock(philo->left_fork);
}

void *philo_routine(void *arg)
{
    t_philo *philo;
    t_data *data;

    philo = (t_philo *)arg;
    data = philo->data;
    if (data->num_philo == 1)
    {
        one_philo_routine(philo);
        return (NULL);
    }
    if (philo->id % 2 == 0)
        ft_usleep(1);
    while (!is_simulation_end(data))
    {
        eat(philo);
        if (is_simulation_end(data))
            break;
        print_status(philo, "is sleeping");
        ft_usleep(data->time_to_sleep);
        print_status(philo, "is thinking");
        if (data->num_philo % 2 != 0)
            ft_usleep(data->time_to_eat);
    }
    return NULL;
}
