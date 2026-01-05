#define _POSIX_C_SOURCE  200112L  // Для использования setenv, tzset

#include <stdio.h>      // printf
#include <stdlib.h>     // getenv, setenv, tzset, EXIT_SUCCESS
#include <time.h>       // time, localtime, gmtime, mktime

int main() {
    time_t now;
    time(&now);  // Получаем текущее системное время (в секундах с 1970 г.)

    struct tm *tm_info;

    // Пытаемся получить локальное время на основе текущей переменной окружения TZ
    tm_info = localtime(&now);

    if (tm_info == NULL) {
        // Если системная база часовых поясов отсутствует или повреждена,
        // fallback: используем UTC + ручной сдвиг — "грубая" PST-реализация (UTC−8)
        printf("Environment doesn't support localtime(). Used UTC difference.\n");

        // Получаем время в UTC
        time(&now);
        tm_info = gmtime(&now);

        // Смещаем на 8 часов назад (в сторону PST, без учёта перехода на летнее время)
        tm_info->tm_hour -= 8;

        // Применяем нормализацию (например, если час стал отрицательным — переход на предыдущий день)
        mktime(tm_info);
    } else {
        // Локальное время успешно получено → система поддерживает переменную TZ
        // Задаём переменную окружения TZ = America/Los_Angeles (Калифорния)
        setenv("TZ", "America/Los_Angeles", 1);

        // Применяем изменения: обновляем таблицы времени в текущем процессе
        tzset();

        // Получаем локальное время уже с учётом установленной переменной TZ
        tm_info = localtime(&now);
    }

    // Выводим дату и время в формате Калифорнии
    printf("Current date and time in Pacific Standard Time: %02d-%02d-%04d %02d:%02d:%02d\n",
           tm_info->tm_mday, tm_info->tm_mon + 1, tm_info->tm_year + 1900,
           tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec);

    return EXIT_SUCCESS;
}

