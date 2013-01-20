#include "asfs.h"

string getAbsolutePath(string relativePath);
void respond(session* ses, string respond);
void respond(session* ses, string resp);
void openDataConnection(session* ses);
void closeDataConnection(session* ses);
string runLs(session* ses, list<string> args);
string cutPathPrefix(string path);

/** execute proper command */
void execCmd(session* ses, list<string> line) {
    // assume, that head of list is command name
    string command = line.front();
    line.pop_front(); // args

    // check if command exists
    commandPtr cmd = NULL;
    cmd = cmds[command];
    if ((cmd != NULL) && ((ses->isAuthenticated == true) || (command == "USER") || (command == "PASS"))) {
        // print command
        stringstream lineStr;
        lineStr << command << " ";
        if (line.size() != 0) {
            list<string> argsCopy = line;
            for (unsigned i = 0; i < line.size(); ++i) {
                lineStr << argsCopy.front() << " ";
                argsCopy.pop_front();
            }
        }
        //lineStr << " :: " << ses->currentDir;
        printEvent(ses, lineStr.str());

        // execute command
        cmd(ses, line);
    } else {
        respond(ses, "502 Command not implemented or user is not authenticated");
    }
}

void execUSER(session* ses, list<string> args) {
    try {
        // load users
        ifstream ifs("users.txt", ifstream::in);

        list<string> users;
        string user;
        while (ifs.good()) {
            ifs >> user;
            users.push_back(user);

            ifs >> user; // ignore password
        }

        if (users.size() == 0) {
            throw string("Error with users.txt file");
        }

        // find given user name in list
        bool isFound = false;
        list<string>::iterator it;
        for (it = users.begin(); it != users.end(); it++) {
            if (*it == args.front()) {
                isFound = true;
                ses->login = *it;
                break;
            }
        }

        // give proper respond
        string resp("");
        if (isFound) {
            resp += "331 User name okay, need password.";
        } else {
            resp += "532 Need account for storing files.";
        }
        respond(ses, resp);
    } catch (string err) {
        cerr << err;
    }
}

void execPASS(session* ses, list<string> args) {
    try {
        // load passwords
        ifstream ifs("users.txt", ifstream::in);

        string user;
        string password;
        while (ifs.good()) {
            ifs >> user;
            ifs >> password;
            if (ses->login == user) {
                ses->password = password;
            }
        }

        bool isPasswordMatch = false;
        if (ses->password == args.front()) {
            isPasswordMatch = true;
            ses->isAuthenticated = true;
        }

        // give proper respond
        string resp("");
        if (isPasswordMatch) {
            resp += "230 User logged in, proceed.";
        } else {
            resp += "230 User logged in, proceed.";
        }
        respond(ses, resp);
    } catch (string err) {
        cerr << err;
    }
}

void execPWD(session* ses, list<string> args) {
    string path = ses->currentDir;

    string substring = cutPathPrefix(path);

    string resp = "257 \"";
    resp += substring;
    resp += "\" created.";
    
    respond(ses, resp);
}

void execLIST(session* ses, list<string> args) {
    openDataConnection(ses);
    if (ses->dsck <= 0) {
        cerr << "Data connection closed.\n";
    }

    string listing = runLs(ses, args);
    string resp("125 Data connection already open; transfer starting.");
    respond(ses, resp);

    // send listing through data connection
    write(ses->dsck, listing.c_str(), listing.length());

    string resp2("250 Requested file action okay, completed.");
    respond(ses, resp2);

    closeDataConnection(ses);
}

void execTYPE(session* ses, list<string> args) {
    string newType = args.front();
    string ascii("A");
    string image("I");

    bool isParamLegal = true; // assume true
    if (newType == ascii) {
        ses->t = ASCII;
    } else if (newType == image) {
        ses->t = IMAGE;
    } else {
        isParamLegal = false;
    }

    string resp("");
    if (isParamLegal) {
        resp += "200 Command okay.";
    } else {
        resp += "504 Command not implemented for that parameter.";
    }
    respond(ses, resp);
}

void execPASV(session* ses, list<string> args) {
    int port = dataConnectionPort;
    string ip;
    if (ses->m != PASSIVE) {
        int sck = socket(AF_INET, SOCK_STREAM, defaultProtocol);
        ERROR(sck);

        sockaddr_in addr;

        memset(&addr, 0, sizeof(sockaddr_in)); // clear structure

        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_ANY);
        addr.sin_port = htons(port);
        
        // find free port
        int result;
        while ((result = bind(sck, (sockaddr*) &addr, sizeof(sockaddr_in))) == -1 && errno == EADDRINUSE) {
            addr.sin_port = htons(++port);
        }
        ERROR(result);

        result = listen(sck, connectionsQueueLength);
        ERROR(result);

        ses->dsck = sck;
        ses->m = PASSIVE;

        // get server ip
        socklen_t addr_len = sizeof(sockaddr_in);
        if (getsockname(ses->csck, (sockaddr*)&addr, &addr_len) != -1) {
            ip.assign(inet_ntoa(addr.sin_addr));
        }
    }

    short upperByte = port / 256;
    short downByte = port % 256;
    
    // replace dots to commas
    int pos = 0;
    while ((pos = ip.find('.')) != string::npos) {
        ip.replace(pos, 1, ",");
    }    

    stringstream resp;
    resp << "227 Entering Passive Mode (" << ip << "," << upperByte << "," << downByte << ")";
    // string resp("227 Entering Passive Mode (127,0,0,1,4,127)");
    respond(ses, resp.str());
}

void execMKD(session* ses, list<string> args) {
    string dir("");
    dir += ses->currentDir;
    dir += args.front();

    string resp("");
    if (mkdir(dir.c_str(), 0755) != -1) {
        resp += "257 ";
        resp += cutPathPrefix(dir);
        resp += " Directory created.";
    } else {
        resp += "550 Requested action not taken.";
    }
    respond(ses, resp);
}

// directory name cannot contain space
void execCWD(session* ses, list<string> args) {
    string subDir = args.front();
    if (subDir == "..") {
        if (ses->currentDir != "./filesystem/") {
            string newPath = ses->currentDir;

            // cut last /
            newPath = newPath.erase(newPath.length()-1, 1);

            // cut current directory
            int pos = newPath.find_last_of("/");
            newPath = newPath.substr(0, pos+1);
            
            ses->currentDir = newPath;
        }
    } else {
        stringstream newPath;
        if (subDir[0] == '/') { // if absolute path
            newPath << "./filesystem" << subDir;

            ses->currentDir = newPath.str();
            // subDir = subDir.substr(1, subDir.length());
        } else {
            subDir = subDir + "/";
            ses->currentDir += subDir;
        }
    }

    string resp("200 directory changed to \"");
    resp += cutPathPrefix(ses->currentDir);
    resp += "\".";
    respond(ses, resp);
}

void execSTOR(session* ses, list<string> args) {
    int fd = 0;
    char buf[BUF_SIZE] = {0};
    int bytesRead = 0;
    string path(ses->currentDir);
    if (path[path.size()] != '/') {
        path += "/";
    }
    path += args.front();

    string dataChunk;
    int pos = 0;

    // open data connection
    openDataConnection(ses);
    if (ses->dsck <= 0) {
        cerr << "Data connection closed.\n";
    }
    
    string resp("125 Data connection opened.");
    respond(ses, resp);

    // read data
    switch (ses->t) {
        case ASCII:
            fd = open(path.c_str(), O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

            while ((bytesRead = read(ses->dsck, buf, BUF_SIZE)) != 0 &&
                    bytesRead != -1) {
                // convert new line characters
                dataChunk.assign(buf);
                while ((pos = dataChunk.find("\r\n")) != string::npos) {
                    dataChunk.replace(pos, 2, "\n");
                }  

                write(fd, dataChunk.c_str(), dataChunk.size());
            }

            close(fd);
            break;
        case IMAGE:
            fd = open(path.c_str(), O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
            cerr << "desc: " << fd << "\n";

            while ((bytesRead = read(ses->dsck, buf, BUF_SIZE)) != 0 &&
                    bytesRead != -1) {
                write(fd, buf, bytesRead);
            }
            close(fd);
            break;
        default:
            printEvent(ses, "Undefined session type");
            break;
    }
    
    closeDataConnection(ses);
    resp.assign("226 Closing data connection. Requested file action successful.");
    respond(ses, resp);
}

void execRETR(session* ses, list<string> args) {
    int fd = 0;
    string path(ses->currentDir);

    if (path[path.size()] != '/') {
        path += "/";
    }
    
    path += args.front();

    fd = open(path.c_str(), O_RDONLY);

    if (fd == -1) {
        cerr << "cant open file\n";
    }

    char buf[BUF_SIZE] = {0};
    int readBytes = 0;

    openDataConnection(ses);
    if (ses->dsck <= 0) {
        cerr << "Data connection closed.\n";
    }
    
    string resp("125 Data connection opened.");
    respond(ses, resp);

    while ((readBytes = read(fd, buf, BUF_SIZE)) != 0) {
        write(ses->dsck, buf, readBytes);
    }

    resp.assign("226 Closing data connection. Requested file action successful.");
    respond(ses, resp);

    closeDataConnection(ses);
    close(fd);
}

/** works on empty directories */
void execRMD(session* ses, list<string> args) {
    string dir(ses->currentDir);
    dir += args.front();
    string resp;
    if (rmdir(dir.c_str()) != -1) {
        resp.assign("250 Operation successful.");
        respond(ses, resp);
    } else {
        resp.assign("550 Operation failed.");
        respond(ses, resp);
    }
}

string getAbsolutePath(string relativePath) {
    char buf[1000] = {0};
 
    char* canonicalPath = realpath(relativePath.c_str(), buf);
    string absolutePath;
    if (canonicalPath != NULL) {
        absolutePath.assign(canonicalPath);
    } else {
        absolutePath.assign("");
    }

    return absolutePath;
}

void openDataConnection(session* ses) {
    int dataSck = accept(ses->dsck, NULL, NULL);
    ERROR(dataSck);
    close(ses->dsck);
    ses->dsck = dataSck;
}

void closeDataConnection(session* ses) {
    close(ses->dsck);
    ses->m = NONE;
}

string runLs(session* ses, list<string> args) {
    FILE *fp;

    char path[1035];

    string cmd("ls ");
    cmd += getAbsolutePath(ses->currentDir);
    cmd +=  " -A -n | tail -n+2";

    fp = popen(cmd.c_str(), "r");
    if (fp == NULL) {
        cerr << "Failed to run command\n";
    }

    string result("");
    /* Read the output a line at a time - output it. */
    while (fgets(path, sizeof(path)-1, fp) != NULL) {
        result += path;
    }

    pclose(fp);

    return result;
}

void respond(session* ses, const char* msg) {
    string resp(msg);
    respond(ses, resp);
}

void respond(session* ses, string resp) {
    resp += "\r\n";
    int length = resp.length();
    int status = write(ses->csck, resp.c_str(), length);
    if (status != length) {
        cerr << "Respond: write error.\n";
    }
}

/**
 * cutting default prefix, which is "./filesystem"
 */
string cutPathPrefix(string path) {
    int pos = path.find("/", 2);
    string substring = path.substr(pos, 1000);
    return substring;
}