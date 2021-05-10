#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>

#define BUF_SIZE 0x1000 // 4096 

int run(pid_t pid, FILE* fresult);

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("Use ./main.out <pid>\n");
        return -1;
    }
    
    FILE* fresult = fopen("result.txt", "wt");
    run(atoi(argv[1]), fresult);

    return 0;
}

int fread_file(FILE* fresult, const char* filename, void (*print_func)(FILE*, const char*))
{
    char buf[BUF_SIZE];
    int len;

    fprintf(fresult, "File: %s\n", filename);
    FILE *f = fopen(filename, "r");
    while ((len = fread(buf, 1, BUF_SIZE, f)) > 0)
    {
        for (int i = 0; i < len; i++)
            if (buf[i] == 0)
                buf[i] = 10; // \n - символ конца строки 0x0A (переход на новую строку)

        buf[len - 1] = 0;
        print_func(fresult, buf);
    }
    fclose(f);
    return 0;
}

void fread_link(FILE* fresult, const char* filename, const char* showname)
{
    char str[BUF_SIZE];
    readlink(filename, str, BUF_SIZE); // Считывает значение символьной ссылки
    fprintf(fresult, "%s -> %s\n", showname, str);
}

// Вывод содержимого директории FD
void fread_fd(FILE* fresult, const char* fd_name)
{
    fprintf(fresult, "File: %s\n", fd_name);

    DIR *dp;
    if ((dp = opendir(fd_name)) == NULL)
    {
        printf("%s\n", strerror(errno));
        exit(errno);
    }

    struct dirent *dirp;
    char path[BUF_SIZE];
    while ((dirp = readdir(dp)) != NULL)
    {
        // Пропуск каталогов . и ..
        if ((strcmp(dirp->d_name, ".") != 0) && (strcmp(dirp->d_name, "..") != 0))
        {
            sprintf(path, "%s/%s", fd_name, dirp->d_name);
            fread_link(fresult, path, dirp->d_name);
        }
    }

    if (closedir(dp) < 0)
    {
        printf("%s", strerror(errno));
        exit(errno);
    }
}

void simple_output(FILE* file, const char *buf)
{
    fprintf(file, "%s\n", buf);
}

void associative_output(FILE* file, const char *buf, const char *outputNames[], int len)
{
    const char* format = "%20s:\t %s\n";
    char *cur = strtok(buf, " ");   // поиск разделителей в файле
    for (int i = 0; cur != NULL && i < len; i++)
    {
        fprintf(file, format, outputNames[i], cur);
        cur = strtok(NULL, " ");
    }
}

void stat_output(FILE* file, const char *buf)
{
    const char *outputNames[] = 
    {
        "pid",          // ID процесса
        "filename",     // Имя файла
        "state",        // Состояние процесса
        "ppid",         // ID родительского процесса
        "gid",          // ID группы процесса
        "session",      // ID сессии процесса
        "tty_nr",       // Контролирующий терминал процесса
        "tp_gid",       // ID внешней группы процессов контролирующего терминала
        "flags",        // Флаги ядра процесса
        "minflt",       // Количество минорных ошибок процесса (Минорные ошибки не включают ошибки загрузки страниц памяти с диска)
        "cminflt",      // Количество минорных ошибок дочерних процессов (Минорные ошибки не включают ошибки загрузки страниц памяти с диска)
        "majflt",       // Количество Мажоных ошибок процесса
        "cmajflt",      // Количество Мажоных ошибок дочерних процессов процесса
        "utime",        // Количество времени, в течение которого этот процесс был запланирован в пользовательском режиме
        "stime",        // Количество времени, в течение которого этот процесс был запланирован в режиме ядра
        "cutime",       // Количество времени, в течение которого ожидаемые дети этого процесса были запланированы в пользовательском режиме
        "cstime",       // Количество времени, в течение которого ожидаемые дети этого процесса были запланированы в режиме ядра
        "priority",     // Приоритет процесса
        "nice",         // nice
        "num_threads",  // Количество потоков
        "itrealvalue",  // Время в тиках до следующего SIGALRM отправленного в процесс из-за интервального таймера.
        "start_time",  // Время запуска процесса 
        "vsize",        // Объем виртуальной памяти в байтах
        "rss",          // Resident Set Size: Количество страниц процесса в физической памяти.
        "rsslim",       // Текущий лимит в байтах на RSS процесса
        "startcode",    // Адрес, над которым может работать текст программы
        "endcode",      // Адрес, над которым может работать текст программы
        "startstack",   // Адрес начала (т. е. дна) стека
        "kstkesp",      // Текущее значение ESP (Stack pointer), найденное на странице стека ядра для данного процесса.
        "kstkeip",      // Текущее значение EIP (instruction pointer)
        "signal",       // Растровое изображение отложенных сигналов, отображаемое в виде десятичного числа
        "blocked",      // Растровое изображение заблокированных сигналов, отображаемое в виде десятичного числа
        "sigignore",    // Растровое изображение игнорированных сигналов, отображаемое в виде десятичного числа
        "sigcatch",     // Растровое изображение пойманных сигналов, отображаемое в виде десятичного числа.
        "wchan",        // Канал, в котором происходит ожидание процесса.
        "nswap",        // Количество страниц, поменявшихся местами
        "cnswap",       // Накопительный своп для дочерних процессов
        "exit_signal",  // Сигнал, который будет послан родителю, когда процесс будет завершен.
        "processor",    // Номер процессора, на котором было последнее выполнение.
        "rt_priority",  // Приоритет планирования в реальном времени- число в диапазоне от 1 до 99 для процессов, запланированных в соответствии с политикой реального времени
        "policy",       // Политика планирования
        "delayacct_blkio_tics", // Общие блочные задержки ввода/вывода
        "quest_time",   // Гостевое время процесса
        "cquest_time",  // Гостевое время  дочерних процессов
        "start_data",   // Адрес, над которым размещаются инициализированные и неинициализированные данные программы (BSS).
        "end_data",     // Адрес, под которым размещаются инициализированные и неинициализированные данные программы (BSS).
        "start_brk",    // Адрес, выше которого куча программ может быть расширена с помощью brk.
        "arg_start",    // Адрес, над которым размещаются аргументы командной строки программы (argv).
        "arg_end",      // Адрес, под которым размещаются аргументы командной строки программы (argv).
        "env_start",    // Адрес, над которым размещена программная среда
        "env_end",      // Адрес, под которым размещена программная среда
        "exit_code"     // Состояние выхода потока в форме, сообщаемой waitpid.
    };
    associative_output(file, buf, outputNames, 51);
}


void statm_output(FILE* file, const char *buf)
{
    const char *outputNames[] = 
    {
        "size",       // общий размер программы
        "resident",   // размер резидентной части
        "share",      // разделяемые страницы
        "trs",        // текст (код)
        "drs",        // данные/стек
        "lrs",        // библиотека
        "dt"         // "дикие" (dirty) страницы

    };
    associative_output(file, buf, outputNames, 7);
}

int run(pid_t pid, FILE* fresult)
{
    char filename[50];
    sprintf(filename, "/proc/%d/", pid);
    int len = strlen(filename);

    strcpy(filename + len, "cmdline");
    fread_file(fresult, filename, simple_output);
    fprintf(fresult, "\n");

    strcpy(filename + len, "cwd");
    fread_link(fresult, filename, filename);
    fprintf(fresult, "\n");

    strcpy(filename + len, "exe");
    fread_link(fresult, filename, filename);
    fprintf(fresult, "\n");

    strcpy(filename + len, "fd");
    fread_fd(fresult, filename);
    fprintf(fresult, "\n");

    strcpy(filename + len, "root");
    fread_link(fresult, filename, filename);
    fprintf(fresult, "\n");

    strcpy(filename + len, "environ");
    fread_file(fresult, filename, simple_output);
    fprintf(fresult, "\n");

    strcpy(filename + len, "maps");
    /* обозначение полей
     1 - начальный адрес выделенного участка памяти
     2 - конечный адрес выделенного участка памяти
     3 - права доступа (r - чтение, w - запись, x - исполняемый, s - разделяемы, p - приватный)
     4 - смещение от которого выполняется распределение
     5 - старший и младший номера устр-в
     6 - номер inode
     */
    fread_file(fresult, filename, simple_output);
    fprintf(fresult, "\n");

    // strcpy(filename + len, "mem");
    // fread_file(fresult, filename, simple_output);
    // fprintf(fresult, "\n");

    strcpy(filename + len, "stat");
    fread_file(fresult, filename, stat_output);
    fprintf(fresult, "\n");

    strcpy(filename + len, "statm");
    fread_file(fresult, filename, statm_output);
    fprintf(fresult, "\n");

    return 0;
}
