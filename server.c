#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <utmp.h>
#include <time.h>
#include <pwd.h>
#include <sys/socket.h>
#include <sys/uio.h>

int main() {

    if (access("fifoToServer", F_OK) == -1) // daca canalul fifo nu exista atunci il creeaza prin mknod
    { 
        if (mknod("fifoToServer", S_IFIFO | 0666, 0) == -1) // verificam situatia in care apare vreo eroare la crearea fisierului
        { 
            perror("Nu se poate crea fisierul fifoToServer.");
            exit(EXIT_FAILURE);
        }
    }

    int fdToServer;

    if ((fdToServer = open("fifoToServer", O_RDONLY)) == -1) 
    {
        perror("Nu se poate deschide pentru citire fisierul fifoToServer.");
        exit(EXIT_FAILURE);
    }

    if (access("fifoToClient", F_OK) == -1) // daca canalul fifo nu exista atunci il creeaza prin mknod
    { 
        if (mknod("fifoToClient", S_IFIFO | 0666, 0) == -1) // verificam situatia in care apare vreo eroare la crearea fisierului
        { 
            perror("Nu se poate crea fisierul fifoToClient.");
            exit(EXIT_FAILURE);
        }
    }

    int fdToClient;

    if ((fdToClient = open("fifoToClient", O_WRONLY)) == -1) 
    {
        perror("Nu se poate deschide pentru scriere fisierul fifoToServer.");
        exit(EXIT_FAILURE);
    }

    char buffer[256];    
    int readLength = 0, logat = 0;

    do{
        readLength = read(fdToServer, buffer, 256); 
        buffer[readLength] = '\0'; 

        if(strstr(buffer, "login") != NULL)
        {   
            // printf("%s\n", buffer);            
    
            pid_t pid = fork();

            if(pid == 0)
            {
                char usernamePrimit[100];
                strcpy(usernamePrimit, buffer + 5); 
                // printf("%s\n", usernamePrimit); 
                strcpy(usernamePrimit, usernamePrimit + 3); 

                // printf("Utilizatorul %s s-a autentificat.\n", usernamePrimit);

                int fdUsers;
                fdUsers = open("users.txt", O_RDONLY);

                char memorie[1000]; 
                memorie[0] = '\0'; 
                
                int memorieEfectiva;
                memorieEfectiva = read(fdUsers, memorie, 1000);
                memorie[memorieEfectiva] = '\0';

                char *utilizator = strtok(memorie,",");
                while(utilizator)
                {
                    if(strcmp(utilizator, usernamePrimit) == 0)
                    {
                        exit(1);
                    }   

                    utilizator = strtok(NULL,",");
                }
                
                exit(2);
                close(fdUsers);
            }
            else
            {  
                int semnal; 
                waitpid(pid, &semnal, 0);

                if(WEXITSTATUS(semnal) != 2)
                {
                    if(logat == 0)
                    {
                        logat = 1;
                        write(fdToClient, "Autentificarea s-a realizat cu succes!", strlen("Autentificarea s-a realizat cu succes!"));
                    }
                    else 
                        write(fdToClient, "Autentificare nereusita! Exista deja un utilizator conectat.", strlen("Autentificare nereusita! Exista deja un utilizator conectat."));
                    
                }
                else if(WEXITSTATUS(semnal) == 2)
                        write(fdToClient, "Nu se poate realiza autentificarea cu userul dat.", strlen("Nu se poate realiza autentificarea cu userul dat."));
            }
        }
        else if(strcmp(buffer, "get-logged-users") == 0)
        {
            struct utmp *n;
            setutent();
            n = getutent();

            while(n) 
            {
                if(n->ut_type == USER_PROCESS) 
                {
                    write(fdToClient, n->ut_user, strlen(n->ut_user));
                    write(fdToClient, n->ut_host, strlen(n->ut_host));
                    write(fdToClient, n->ut_line, strlen(n->ut_line));
                    // write(fdToClient, n->ut_tv.tv_sec, strlen(n->ut_tv.tv_sec)); segmentatio fault de la comanda asta 
                }
                n = getutent();
            }
        }
        else if(strstr(buffer, "get-proc-info : ") != 0)
        {
            int sockets[2], child;

            socketpair(AF_UNIX, SOCK_STREAM, 0, sockets);
            child = fork();

            if(child == 0)
            {
                close(sockets[1]);

                strcpy(buffer, buffer + 16);
                char c[256] = "/proc/";
                strcat(c, buffer);
                strcat(c, "/status");

                int fdInfo = open(c, O_RDONLY); 
                int amDeCitit; 
                char linie[1000];

                do{
                    amDeCitit = read(fdInfo, linie, 1000);
                    linie[amDeCitit] = '\0';

                    char *info = strtok(linie, "\n");
                    while(info)
                    {   
                        if(strstr(info,"Name") != 0 || strstr(info,"State") != 0 || strstr(info," Ppid") != 0 || strstr(info,"VmSize") != 0 || strstr(info,"Uid") != 0)
                        {
                            write(sockets[0], info, strlen(info));
                            write(sockets[0],"\n",1);
                        }

                        info = strtok(NULL,"\n");
                    }                   
                }while(amDeCitit);

                close(sockets[0]); 
            }
            else if(child != 0) 
            { 
                close(sockets[0]);

                char deCitit[1024];
                deCitit[0]='\0';

                int l =read(sockets[1], deCitit, 1024);
                deCitit[l] = '\0';
                write(fdToClient, deCitit, strlen(deCitit));

                close(sockets[1]); 
            }            
        }
        else if(strcmp(buffer, "logout") == 0)
        {
            if(logat == 1)
                write(fdToClient, "Delogarea s-a realizat cu succes!", strlen("Delogarea s-a realizat cu succes!"));
            else 
                write(fdToClient, "Comanda nu se poate realiza! Nu exista niciun utilizator conectat.", strlen("Comanda nu se poate realiza! Nu exista niciun utilizator conectat."));

            logat = 0;
        }
        else if(strcmp(buffer, "quit") == 0)
        {
            if(logat == 1)
                logat = 0;
        }
        else 
            write(fdToClient, "Comanda nu exista sau nu poate fi executata deoarece nu sunteti autentificat!", strlen("Comanda nu exista sau nu poate fi executata deoarece nu sunteti autentificat!"));
   
    } while(readLength);    

    close(fdToServer);
    close(fdToClient);
}