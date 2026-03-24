#include "philo.h"

#define USAGE_MSG "Usage: ./philo N ttd tte tts [eat_count]\n"
#define PARSE_ERR_MSG "Error: invalid argument\n"

size_t ft_strlen(const char *s)
{
    size_t len = 0;
    while (s[len])
        len++;
    return len;
}

static int ft_atoi(const char *str)
{
    int i;
    int result;

    i = 0;
    result = 0;
    if (str[i] == '\0')
        return -1;
    while (str[i])
    {
        if (str[i] < '0' || str[i] > '9')
            return -1;
        result = result * 10 + (str[i] - '0');
        if (result < 0)
            return -1;
        i++;
    }
    if (result == 0)
        return -1;
    return result;
}

int parse_args(int argc, char **argv, t_data *data)
{
    if (argc != 5 && argc != 6)
    {
        write(STDERR_FILENO, USAGE_MSG, sizeof(USAGE_MSG) - 1);
        return 1;
    }
    data->num_philo = ft_atoi(argv[1]);
    data->time_to_die = ft_atoi(argv[2]);
    data->time_to_eat = ft_atoi(argv[3]);
    data->time_to_sleep = ft_atoi(argv[4]);
    data->must_eat_count = -1;
    if (argc == 6)
        data->must_eat_count = ft_atoi(argv[5]);
    if (data->num_philo < 1 || data->time_to_die < 1 || data->time_to_eat < 1 ||
        data->time_to_sleep < 1)
    {
        write(STDERR_FILENO, PARSE_ERR_MSG, sizeof(PARSE_ERR_MSG) - 1);
        return 1;
    }
    return 0;
}

int main(int argc, char **argv)
{
    t_data data;
    t_philo *philos;
    pthred_t monitor;

    if (parse_args(argc, argv, &data))
        return 1;

    philos = init_data(&data);
    if (!philos)
        return 1;
    if (start_simulation(&data, &monitor))
    {
        cleanup(&data, philos);
        return 1;
    }
    join_threads(&data, monitor);
    cleanup(&data, philos);

    return 0;
}
