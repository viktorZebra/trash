#include <syslog.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/resource.h>
#include <sys/stat.h> //umask
#include <unistd.h> //setsid
#include <stdio.h> //perror
#include <signal.h> //sidaction
#include <string.h> 
#include <errno.h> 
#include <pthread.h>
#include <math.h>
#include <time.h>


#define LOCKFILE "/home/ilya/Desktop/operating-systems/Semester #6/lab4/part1/daemon.pid"
#define LOCKMODE (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH) 


void daemonize(const char *cmd)
{
    int fd0, fd1, fd2;
    pid_t pid;
    struct rlimit rl;
    struct sigaction sa;

    // 1. Сбрасывание маски режима создания файла, чтобы демон мог создавать файлы с 
    // любыми правами доступа.

    umask(0);

    // 2. Потомок утрачивает группу предка.

    if ((pid = fork()) < 0)
        perror("Ошибка fork!\n");
    else if (pid != 0) //родительский процесс
        exit(0);


    // 3. Стать лидером новой сессии, лидером группы процессов, утратить управляющий терминал.

    setsid();

    sa.sa_handler = SIG_IGN;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if (sigaction(SIGHUP, &sa, NULL) < 0)
        perror("Невозможно игнорировать сигнал SIGHUP!\n");

    // 4. Назначить корневой каталог текущим рабочим каталогом, 
    // чтобы впоследствии можно было отмонтировать файловую систему.

    if (chdir("/") < 0)
        perror("Невозможно назначить корневой каталог текущим рабочим каталогом!\n");
    
    // 5. Закрыть все файловые дескрипторы.

    if (getrlimit(RLIMIT_NOFILE, &rl) < 0)
        perror("Невозможно получить максимальный номер дискриптора!\n");

    if (rl.rlim_max == RLIM_INFINITY)
        rl.rlim_max = 1024;
    for (int i = 0; i < rl.rlim_max; i++)
        close(i);

    // 6. Присоеденить файловые дескрипторы 0, 1, 2 к /dev/null

    // fd0 = open("/dev/null", O_RDWR);
    fd1 = dup(fd0); //копируем файловый дискриптор
    fd2 = dup(fd0); // 0

    // 7. инициализировать файл журнала.

    openlog(cmd, LOG_CONS, LOG_DAEMON);
    // if (fd0 != 0 || fd1 != 1 || fd2 != 2)
    // {
    //     syslog(LOG_ERR, "ошибочные файловые дескрипторы %d %d %d\n", fd0, fd1, fd2);
    //     exit(1);
    // }

    syslog(LOG_WARNING, "Демон запущен!");
    
}

int lockfile(int fd)
{
    struct flock fl;

    fl.l_type = F_WRLCK;                // Режим блокирования разделения записи.

    fl.l_start = 0;                     // Старт со смещения 0.

    fl.l_whence = SEEK_SET;             // С начала файла.

    fl.l_len = 0;                       // Файл блокируется до его конца.

    return(fcntl(fd, F_SETLK, &fl));    // Установить блокировку
                                        // Если конфликтующая блокировка удерживается другим процессом, 
                                        // то данный вызов вернёт -1 и установит значение errno в EACCES или EAGAIN. 
}

int already_running(void)
{
    syslog(LOG_ERR, "Проверка на многократный запуск");

    int fd;
    char buf[16];

    fd = open(LOCKFILE, O_RDWR | O_CREAT, LOCKMODE);

    if (fd < 0)
    {
        syslog(LOG_ERR, "Невозможно открыть %s: %s!", LOCKFILE, strerror(errno));
        exit(1);
    }

    syslog(LOG_WARNING, "Lock-файл открыт!");

    if (lockfile(fd) < 0)
    {
        if (errno == EACCES || errno == EAGAIN)
        {
            close(fd);
            return(1);
        }

        syslog(LOG_ERR, "Невозможно установить блокировку на %s: %s!\n", LOCKFILE, strerror(errno));
        exit(1);
    }

    syslog(LOG_WARNING, "Запись PID");

    ftruncate(fd, 0);
    sprintf(buf, "%ld", (long)getpid());
    write(fd, buf, strlen(buf) + 1);

    syslog(LOG_WARNING, "PID записан");
     
    return 0;
}

int thr_fn(sigset_t *mask)
{
    int err, signo;
    time_t my_time;

    for (;;)
    {
        err = sigwait(mask, &signo);

        if (err != 0)
        {
            syslog(LOG_ERR, "Ошибка вызова функции sigwait");
            exit(1);
        }

        syslog(LOG_INFO, "Пользователь %s с иденнтификатором %d отправил сигнал", getlogin(), getuid());

        switch (signo)
        {
        case SIGHUP:
            my_time = time(NULL);
            syslog(LOG_INFO, "Получен сигнал SIGHUP; время: %s", ctime(&my_time));
            for (int i = 0; i < 30000; i++)
            {
                printf("%f", sin(i) * cos(i) * sin(i) * cos(i) * sin(i) * cos(i));
            }


            break;

        case SIGTERM:
            syslog(LOG_INFO, "Получен сигнал SIGTERM; выход");
            exit(0);
            break;

        default:
            syslog(LOG_INFO, "Получен иной сигнал %d\n", signo);
            break;
        }
    }

    return 0;
}

int main(int argc, char *argv[]) 
{
    int err;
    pthread_t tid;
    char *cmd;
    struct sigaction sa;

    if ((cmd = strrchr(argv[0], '/')) == NULL) // Поиск последнего вхождения символа в строку 
                                               // (cmd - команда, запустившая данный процесс)
        cmd = argv[0];
    else
        cmd++;

    // Перейти в режим демона.

    daemonize(cmd);

    // Убедиться в том, что ранее не была запущена другая копия демона.

    if (already_running() != 0)
    {
        syslog(LOG_ERR, "Демон уже запущен!\n");
        exit(1);
    }

    syslog(LOG_WARNING, "Проверка существования одного экземпляра пройдена!");

    // Восстановить действие по умолчанию для сигнала SIGHUB 
    // и заблокировать все сигналы.

    sa.sa_handler = SIG_DFL;
    sigemptyset(&sa.sa_mask);

    sa.sa_flags = 0;

    if (sigaction(SIGHUP, &sa, NULL) < 0)
        syslog(LOG_ERR, "Невозможно восстановить действие SIG_DFL для SIGHUP");

    sigset_t mask;
    sigfillset(&mask);

    if ((err = pthread_sigmask(SIG_BLOCK, &mask, NULL)) != 0)
        syslog(LOG_ERR, "Ошибка выполнения операции SIG_BLOCK");

    // Создать поток, который будет заниматься обработкой SIGHUP и SIGTERM

    err = pthread_create(&tid, NULL, (void* (*)(void*))thr_fn, &mask);

    if (err != 0)
        syslog(LOG_ERR, "Невозможно создать поток");

    while(1) 
    {
        for (int i = 0; i < 30000; i++)
        {
            sin(i) * cos(i) * sin(i) * cos(i) * sin(i) * cos(i);
        }
        sleep(20);
    }

    return 0;
}