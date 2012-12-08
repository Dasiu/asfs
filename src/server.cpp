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

map<string, commandPtr> cmds;

list<string> splitCommand(char cmd[]);
void handleClient(int socket);
void initServer();

void runServer() {

    initServer();

    int sck = socket(AF_INET, SOCK_STREAM, defaultProtocol);
    ERROR(sck);

    sockaddr_in addr;

    memset(&addr, 0, sizeof(sockaddr_in)); // clear structure

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    int result = bind(sck, (sockaddr*) &addr, sizeof(sockaddr_in));
    ERROR(result);

    result = listen(sck, connectionsQueueLength);
    ERROR(result);

    int clientSck;
    unsigned int addrSize = sizeof(addr);
    while ((clientSck = accept(sck, (sockaddr*) &addr, &addrSize)) != -1) {
        handleClient(clientSck);

        addrSize = sizeof(addr);
    }
}

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

void handleClient(int socket) {
    //write(socket, buf, BUF_SIZE);
    string response("220 Service ready for new user.\r\n");
    write(socket, response.c_str(), response.length());

    // create session
    session ses;
    ses.csck = socket;
    // ses.isLoginProvided = false;
    ses.currentDir.assign("./filesystem/");
    ses.t = ASCII;
    ses.f = NON_PRINT;
    ses.m = NONE;

    // command loop
    while (true) {
        char cmd[commandSize] = {0};
        read(socket, cmd, commandSize-1); // last character for null termination

        list<string> args = splitCommand(cmd);

        execCmd(&ses, args);
    }
}

void initServer() {
    cmds.insert(pair<string, commandPtr>("USER", execUSER));
    cmds.insert(pair<string, commandPtr>("PASS", execPASS));
    cmds.insert(pair<string, commandPtr>("PWD", execPWD));
    cmds.insert(pair<string, commandPtr>("LIST", execLIST));
    cmds.insert(pair<string, commandPtr>("TYPE", execTYPE));
    cmds.insert(pair<string, commandPtr>("PASV", execPASV));
    cmds.insert(pair<string, commandPtr>("MKD", execMKD));
    cmds.insert(pair<string, commandPtr>("CWD", execCWD));
}

void runServerDTP(session* ses) {
    pthread_t serverDTPThread;
    int result;

    result = pthread_create(&serverDTPThread, NULL, serverDTPMainLoop, (void*) ses);
    if (result){
        cerr << "ERROR; return code from pthread_create() is " << result << "\n";
        exit(EXIT_FAILURE);
    }
}

void* serverDTPMainLoop(void* s) {
    session* ses = (session*) s;

    cout << "poczatek serverDTPmainloo\n";

    int sck = socket(AF_INET, SOCK_STREAM, defaultProtocol);
    ERROR(sck);

    sockaddr_in addr;

    memset(&addr, 0, sizeof(sockaddr_in)); // clear structure

    addr.sin_family = AF_INET;
    addr.sin_port = htons(dataConnectionPort);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    int result = bind(sck, (sockaddr*) &addr, sizeof(sockaddr_in));
    ERROR(result);

    cout << "w dtp main loop, przed listen\n";
    result = listen(sck, connectionsQueueLength);
    ERROR(result);
    cout << "w dtp main loop, po listen\n";
    int clientSck;
    unsigned int addrSize = sizeof(addr);
    while ((clientSck = accept(sck, (sockaddr*) &addr, &addrSize)) != -1) {
        ses->dsck = clientSck;
        cout << clientSck << " Data connection created.\n";



        close(ses->dsck);
        addrSize = sizeof(addr);
        cerr << "po resopndzie\n";
        pthread_exit(NULL);

    }

    pthread_exit(NULL);
}