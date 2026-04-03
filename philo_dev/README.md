*This project has been created as part of the 42 curriculum by izen*

# Philosophers

## Description

The Dining Philosophers problem is a classic concurrency exercise. One or more philosophers sit at a round table with a bowl of spaghetti. Each philosopher alternates between eating, thinking, and sleeping. There is one fork between each pair of adjacent philosophers — a philosopher needs both their left and right fork to eat.

The simulation ends when a philosopher dies of starvation, or when all philosophers have eaten a specified number of times (optional). The goal is to prevent any philosopher from dying while avoiding data races.

**Implementation:** threads (one per philosopher) + mutexes (one per fork).

## Instructions

### Compilation

```bash
make        # build
make re     # full rebuild
make clean  # remove object files
make fclean # remove all generated files
```

### Execution

```bash
./philo number_of_philosophers time_to_die time_to_eat time_to_sleep [number_of_times_each_philosopher_must_eat]
```

| Argument | Description |
|---|---|
| `number_of_philosophers` | Number of philosophers (and forks) |
| `time_to_die` (ms) | Time since last meal before a philosopher dies |
| `time_to_eat` (ms) | Time it takes to eat (holds 2 forks) |
| `time_to_sleep` (ms) | Time spent sleeping |
| `number_of_times_each_philosopher_must_eat` | (optional) Stop when all have eaten this many times |

### Examples

```bash
./philo 5 800 200 200
./philo 5 800 200 200 7
./philo 1 800 200 200    # single philosopher — dies (only one fork)
```

### Log format

```
timestamp_in_ms X has taken a fork
timestamp_in_ms X is eating
timestamp_in_ms X is sleeping
timestamp_in_ms X is thinking
timestamp_in_ms X died
```

## Resources

- [Dining philosophers problem — Wikipedia](https://en.wikipedia.org/wiki/Dining_philosophers_problem)
- [POSIX Threads Programming — LLNL](https://hpc-tutorials.llnl.gov/posix/)
- `man pthread_create`, `man pthread_mutex_init`, `man gettimeofday`, `man usleep`

### AI usage

AI (Claude) was used to assist with:
- Drafting this README structure based on the subject requirements

All generated content was reviewed, tested, and understood before being included in the project.
