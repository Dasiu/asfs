#ifndef ASFS_H
#define ASFS_H

#include <iostream>
#include <fstream>
#include <map>
#include <string>
#include <list>
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

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
    // bool isLoginProvided; // true after successful USER command
    string currentDir;
	type t;
	format f;
};

typedef void (*commandPtr)(session*, list<string>);

extern map<string, commandPtr> cmds;

void runServer();

void execCmd(session* ses, list<string> line);
void execUSER(session* ses, list<string> args);
void execPASS(session* ses, list<string> args);
void execPWD(session* ses, list<string> args);
void execLIST(session* ses, list<string> args);
void execTYPE(session* ses, list<string> args);

#endif