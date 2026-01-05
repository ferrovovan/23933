// Стандарт: C99

#define _XOPEN_SOURCE // для putenv и других X/Open и POSIX расширений

#include <stdio.h>              // Стандартный ввод-вывод (printf, perror)
#include <stdlib.h>             // Стандартные функции (atoi, getenv, putenv)
#include <unistd.h>             // POSIX-функции (getpid, getuid, getopt)
#include <sys/resource.h>       // Определяет структуру rlimit и функции getrlimit/setrlimit
#include <limits.h>             // Определения для размеров системных ресурсов
#include <linux/limits.h>       // Для PATH_MAX (макс. длина пути)
#include <errno.h>              // Для защиты от переполнения.

extern char **environ;  // Переменная среды (массив строк вида "NAME=VALUE")
// находится в <unistd.h> в glibc: extern char **__environ;\ #define environ __environ
// Фактически environ — это макрос, ссылающийся на __environ.
// Но в программе лучше писать "extern char **environ;",
//  потому что это общеупотребимый способ взаимодействия с окружением в POSIX-среде.

// Вывод справки по опциям
void print_help(char *program_name) {
    printf("Usage: %s [options]\n", program_name);
    printf("Available options:\n");
    printf("  -i        Show user and group IDs (real and effective)\n");
    printf("  -s        Make the process leader of its process group\n");
    printf("  -p        Show PID, PPID, and GID\n");
    printf("  -u        Show current ulimit (number of open files)\n");
    printf("  -U<val>   Set ulimit value\n");
    printf("  -c        Show core file size limits\n");
    printf("  -C<val>   Set core file size limit\n");
    printf("  -d        Show current working directory\n");
    printf("  -v        Print all environment variables\n");
    printf("  -V<name=value>  Set/modify environment variable\n");
    printf("\nOptions are processed in order from left to right.\n");
}

// Проверки на ошибки опущены.
long parse_limit(const char *arg);

int main(int argc, char *argv[]) {
    // Список допустимых опций: ':' после символа означает, что опция требует аргумент
    const char *options = "ispucdvU:C:V:";

    if (argc == 1) {
        // При отсутствии аргументов — выводим справку
        print_help(argv[0]);
        return EXIT_FAILURE;
    }


    // Создаём обратный массив указателей на аргументы, исключая имя программы
    // для чтения справа налево с помощью getopt().
    char **reversed_args = malloc(argc * sizeof(char*));
    for (int i = 1; i < argc; ++i) {
        reversed_args[i] = argv[argc - i];
    }

    // Заменяем аргументы
    optind = 0; // сброс индекса
    
    int opt;              // Текущая опция
    long new_limit;       // Хранение нового значения лимита
    struct rlimit limit;  // Структура для хранения лимитов ресурсов
    char cwd[PATH_MAX];   // Буфер для хранения пути

    // Обработка опций, определённых в переменной options
    while ((opt = getopt(argc, reversed_args, options)) != -1) {
        switch (opt) {
            case 'i':
                printf("Effective user ID: %u\nEffective group ID: %u\n", geteuid(), getegid());
                printf("Real user ID: %u\nReal group ID: %u\n", getuid(), getgid());
                break;

            case 's':
                setpgid(0, 0); // Сделать текущий процесс лидером группы, назначив его ID группы равным его PID.
                break;

            case 'p':
                printf("PID: %u\n", getpid());
                printf("Parent PID: %u\n", getppid());
                printf("Group ID: %u\n", getpgrp());
                break;

            case 'u':
                // Получение лимита на количество открытых файлов
                getrlimit(RLIMIT_NOFILE, &limit);
                printf("uLimit: soft = %llu, hard = %llu\n",
                   (unsigned long long)limit.rlim_cur,
                   (unsigned long long)limit.rlim_max);
                break;

            case 'U':
                // Установка нового лимита на количество открытых файлов
                new_limit = parse_limit(optarg);
                    if (new_limit >= 0) {
                        getrlimit(RLIMIT_NOFILE, &limit);
                        limit.rlim_cur = (rlim_t) new_limit;
                        setrlimit(RLIMIT_NOFILE, &limit);
                    } // Если parse_limit() возвращает отрицательное значение, установка лимита пропускается.
                break;

            case 'c':
                // Печать лимита на размер core-файла
                getrlimit(RLIMIT_CORE, &limit);
                printf("Core file size: soft = %llu, hard = %llu\n",
                   (unsigned long long)limit.rlim_cur,
                   (unsigned long long)limit.rlim_max);
                break;

            case 'C':
                // Изменение лимита на размер core-файла
                new_limit = parse_limit(optarg);
                if (new_limit >= 0) {
                    getrlimit(RLIMIT_CORE, &limit);
                    limit.rlim_cur = (rlim_t) new_limit;
                    setrlimit(RLIMIT_CORE, &limit);
                }  // Если parse_limit() возвращает отрицательное значение, установка лимита пропускается.
                break;

            case 'd':
                // Получение текущей директории
                getcwd(cwd, sizeof(cwd));
                printf("Current directory: %s\n", cwd);
                break;

            case 'v':
                // Перебор всех переменных окружения
                for (char **env = environ; *env != NULL; ++env) {
                    printf("%s\n", *env);
                }
                break;

            case 'V':
                putenv(optarg);
                break;

            default:
                fprintf(stderr, "Unknown option or error. Run without arguments for help.\n");
                return EXIT_FAILURE;
        }
    }
    free(reversed_args);
    return EXIT_SUCCESS;
}

long parse_limit(const char *arg) {
    char *endptr;
    errno = 0;

    long val = strtol(arg, &endptr, 10);

    // Проверка: была ли вообще цифра
    if (endptr == arg) {
        fprintf(stderr, "Ошибка: не удалось распознать число\n");
        return -1;
    }

    // Проверка: не вышло ли за пределы типа long
    if (errno == ERANGE && (val == LONG_MAX || val == LONG_MIN)) {
        fprintf(stderr, "Ошибка: переполнение при чтении числа\n");
        return -1;
    }

    // Проверка: остались ли мусорные символы
    if (*endptr != '\0') {
        fprintf(stderr, "Ошибка: некорректные символы после числа\n");
        return -1;
    }

    return val;
}
