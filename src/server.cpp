#include "asfs.h"

void warnIfError(const char* file, int line, int code) {
    if (code == -1) {
        cerr << file << ":" << line << ": ";
        perror("");
    }
}

const unsigned int defaultProtocol = 0;
const unsigned int port = 1150;
const unsigned int dataConnectionPort = 1151;
const unsigned int connectionsQueueLength = 1;
const unsigned int commandSize = 1000;
const unsigned int bufSize = 1024;
const unsigned int connectionLimit = 128;

map<string, commandPtr> cmds;

list<string> splitCommand(char cmd[]);
void handleClient(session* ses);
void initCommandMap();

#include "asfs.h"

pthread_t client_threads [connectionLimit];
pthread_t main_thread;
pthread_mutex_t sock_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t ses_mutex  = PTHREAD_MUTEX_INITIALIZER;

int cln_sockets [connectionLimit];
session ses;

void* client_loop (void* arg) 
{
    printf("poczatek client loop\n");
    session sess = *((session*) arg);
    printf("po sess\n");
    pthread_mutex_unlock (&ses_mutex);

    printf("przed printf\n");
    sess.ip.c_str();
    printf("przed printf po sess test\n");
    printf ("New client (%s) thread started for socket %d\n", sess.ip.c_str(), sess.csck);

    handleClient(&sess);

    close (sess.csck);
    pthread_mutex_lock (&sock_mutex);
    for (unsigned i = 0; i < connectionLimit; i++)
        if (cln_sockets [i] == sess.csck) {
            cln_sockets [i] = 0;
            break;
        } 
    pthread_mutex_unlock (&sock_mutex); 

    printf ("Client thread ending for socket %d\n", sess.csck);
    pthread_exit (NULL);
}

void* main_loop (void* arg)
{
    int srv_socket, tmp_socket;
    socklen_t cln_addr_size;
    struct sockaddr_in srv_addr, cln_addr;

    srv_addr.sin_family = AF_INET;
    srv_addr.sin_addr.s_addr = INADDR_ANY;
    srv_addr.sin_port = htons (port);

    bzero (cln_sockets, connectionLimit * sizeof (int));

    if ((srv_socket = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
        perror ("Main socket creation error:");
        exit (EXIT_FAILURE);
    }

    if (bind (srv_socket, (struct sockaddr*) &srv_addr, sizeof srv_addr) == -1) {
        perror ("Main socket bind error:");
        exit (EXIT_FAILURE);
    }

    if (listen (srv_socket, 16) == -1) {
        perror ("Main socket listen error:");
        exit (EXIT_FAILURE);
    } 

    printf ("Main socket started\n");

    initCommandMap();
    while (1) {
        cln_addr_size = sizeof cln_addr;
        if ((tmp_socket = accept (srv_socket, (struct sockaddr*) &cln_addr, &cln_addr_size)) == -1) {
            perror ("Main socket accept error:");
            continue;
        }

        printf("[%d:%s]\n", ntohs(cln_addr.sin_port), inet_ntoa(cln_addr.sin_addr));

        pthread_mutex_lock (&sock_mutex);
        unsigned i;
        for (i = 0; i < connectionLimit; i++)
            if (cln_sockets [i] == 0) {
                cln_sockets [i] = tmp_socket;
                break;
            } 
        pthread_mutex_unlock (&sock_mutex);

        if (i == connectionLimit) {
            printf ("Too many connections\n");
            close (tmp_socket);
            continue;
        }

        printf("przed ip\n");
        string ip(inet_ntoa(cln_addr.sin_addr));
        printf("po ip\n");

        pthread_mutex_lock (&ses_mutex);
        // create session
        ses.csck = cln_sockets[i];
        // ses.isLoginProvided = false;
        ses.currentDir.assign("./filesystem/");
        ses.t = ASCII;
        ses.f = NON_PRINT;
        ses.m = NONE;
        ses.ip = ip;
        printf("po init\n");

        if (pthread_create (&client_threads [i], NULL, client_loop, &ses) != 0) {
            pthread_mutex_unlock (&ses_mutex);
            printf ("Error creating new client thread\n");
            continue;
        } 
    }
}

int main (int argc, char** argv)
{
    if (pthread_create (&main_thread, NULL, main_loop, NULL) != 0) {
        printf ("Thread create error\n");
        exit (EXIT_FAILURE);
    }

    printf ("Main thread started\n");
    printf ("Press <ENTER> to shutdown server\n");
    getc (stdin);

    return EXIT_SUCCESS;
}

// void runServer() {

//     initCommandMap();

//     int sck = socket(AF_INET, SOCK_STREAM, defaultProtocol);
//     ERROR(sck);

//     sockaddr_in addr;

//     memset(&addr, 0, sizeof(sockaddr_in)); // clear structure

//     addr.sin_family = AF_INET;
//     addr.sin_port = htons(port);

//     int result = bind(sck, (sockaddr*) &addr, sizeof(sockaddr_in));
//     ERROR(result);

//     result = listen(sck, connectionsQueueLength);
//     ERROR(result);

//     int clientSck;
//     unsigned int addrSize = sizeof(addr);
//     while ((clientSck = accept(sck, (sockaddr*) &addr, &addrSize)) != -1) {
//         handleClient(clientSck);

//         addrSize = sizeof(addr);
//     }
// }

// elements of list has to be deleted later
list<string> splitCommand(char cmd[]) {
    list<string> args;
    
    // printf("adam 1\n");
    char* token = strtok(cmd, " \r\n");
    while (token != NULL) {
        //printf("%s\n", token);

        //string arg = new string(token);
        string arg(token);
        args.push_back(token);

        token = strtok(NULL, "\r\n");
    }
    return args;
}

void handleClient(session* ses) {
    //write(socket, buf, BUF_SIZE);
    string response("220 Service ready for new user.\r\n");
    write(ses->csck, response.c_str(), response.length());

    // command loop
    while (true) {
        char cmd[commandSize] = {0};
        read(ses->csck, cmd, commandSize-1); // last character for null termination

        list<string> args = splitCommand(cmd);

        execCmd(ses, args);
    }
}

void initCommandMap() {
    cmds.insert(pair<string, commandPtr>("USER", execUSER));
    cmds.insert(pair<string, commandPtr>("PASS", execPASS));
    cmds.insert(pair<string, commandPtr>("PWD", execPWD));
    cmds.insert(pair<string, commandPtr>("LIST", execLIST));
    cmds.insert(pair<string, commandPtr>("TYPE", execTYPE));
    cmds.insert(pair<string, commandPtr>("PASV", execPASV));
    cmds.insert(pair<string, commandPtr>("MKD", execMKD));
    cmds.insert(pair<string, commandPtr>("CWD", execCWD));
}

// void runServerDTP(session* ses) {
//     pthread_t serverDTPThread;
//     int result;

//     result = pthread_create(&serverDTPThread, NULL, serverDTPMainLoop, (void*) ses);
//     if (result){
//         cerr << "ERROR; return code from pthread_create() is " << result << "\n";
//         exit(EXIT_FAILURE);
//     }
// }

// void* serverDTPMainLoop(void* s) {
//     session* ses = (session*) s;

//     cout << "poczatek serverDTPmainloo\n";

//     int sck = socket(AF_INET, SOCK_STREAM, defaultProtocol);
//     ERROR(sck);

//     sockaddr_in addr;

//     memset(&addr, 0, sizeof(sockaddr_in)); // clear structure

//     addr.sin_family = AF_INET;
//     addr.sin_port = htons(dataConnectionPort);
//     addr.sin_addr.s_addr = htonl(INADDR_ANY);

//     int result = bind(sck, (sockaddr*) &addr, sizeof(sockaddr_in));
//     ERROR(result);

//     cout << "w dtp main loop, przed listen\n";
//     result = listen(sck, connectionsQueueLength);
//     ERROR(result);
//     cout << "w dtp main loop, po listen\n";
//     int clientSck;
//     unsigned int addrSize = sizeof(addr);
//     while ((clientSck = accept(sck, (sockaddr*) &addr, &addrSize)) != -1) {
//         ses->dsck = clientSck;
//         cout << clientSck << " Data connection created.\n";



//         close(ses->dsck);
//         addrSize = sizeof(addr);
//         cerr << "po resopndzie\n";
//         pthread_exit(NULL);

//     }

//     pthread_exit(NULL);
// }