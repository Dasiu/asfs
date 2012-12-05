#ifndef ASFS_H
#define ASFS_H

#include <iostream>
#include <fstream>
#include <map>
#include <string>
#include <list>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <limits.h>

using namespace std;

enum type {
	ASCII,
	IMAGE
};

enum format {
	NON_PRINT
};

struct session {
    int csck; // command connection socket
    int dsck; // data socket
    string login;
    string password;
    string currentDir;
	type t;
	format f;
};

typedef void (*commandPtr)(session*, list<string>);

extern map<string, commandPtr> cmds;
extern const unsigned int dataConnectionPort;
extern const unsigned int defaultProtocol;
extern const unsigned int connectionsQueueLength;

void warnIfError(int errCode);

void runServer();
void runServerDTP(session* ses);
void* serverDTPMainLoop(void* s);

void execCmd(session* ses, list<string> line);
void execUSER(session* ses, list<string> args);
void execPASS(session* ses, list<string> args);
void execPWD(session* ses, list<string> args);
void execLIST(session* ses, list<string> args);
void execTYPE(session* ses, list<string> args);
void execPASV(session* ses, list<string> args);

#endif