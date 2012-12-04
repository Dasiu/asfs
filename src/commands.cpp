#include "asfs.h"

/** execute proper command */
void execCmd(session* ses, list<string> line) {
    // assume, that head of list is command name
    string command = line.front();
    line.pop_front(); // args

    // check if command exists
    commandPtr cmd = NULL;
    cmd = cmds[command];
    if (cmd != NULL) {
        cmd(ses, line);
    } else {
        string respond = "502 Command not implemented.\r\n";
        write(ses->csck, respond.c_str(), respond.length());
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
        if (isFound) {
            string success("331 User name okay, need password.\r\n");
            write(ses->csck, success.c_str(), success.length());
        } else {
            string fail("532 Need account for storing files.\r\n");
            write(ses->csck, fail.c_str(), fail.length());
        }
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
        }

        // give proper respond
        string success("230 User logged in, proceed.\r\n");
        string fail("530 Not logged in.\r\n");
        if (isPasswordMatch) {
            write(ses->csck, success.c_str(), success.length());
        } else {
            write(ses->csck, fail.c_str(), fail.length());
        }
    } catch (string err) {
        cerr << err;
    }
}

// string getAbsolutePath(string relativePath) {
//     return relativePath;
// }

void execPWD(session* ses, list<string> args) {
    string respond = "257 ";
    // respond += getAbsolutePath(ses->currentDir);
    respond += ses->currentDir;
    respond += " created.\r\n";
    
    write(ses->csck, respond.c_str(), respond.length());
}

void openDataConnection(session* ses);
void execLIST(session* ses, list<string> args) {
    // string respond("550 Operation not supported.\r\n");
    // write(ses->csck, respond.c_str(), respond.length());

    openDataConnection(ses);
    if (ses->dsck <= 0) cerr << "Data connection closed.\n";

    string msg("drwxr-xr-x 2 1000 1000 4096 Dec  4 16:51 bin\n-rw-r--r-- 1 1000 1000  275 Nov 18  2011 config\n-rw-r--r-- 1 1000 1000 1623 Dec  4 16:59 log\n-rw-r--r-- 1 1000 1000  690 Nov 18  2011 Makefile\ndrwxr-xr-x 2 1000 1000 4096 Dec  4 16:51 obj\n-rw-r--r-- 1 1000 1000  584 Nov 18  2011 README.markdown\ndrwxr-xr-x 2 1000 1000 4096 Nov 18  2011 src\n-rw-r--r-- 1 1000 1000    0 Dec  4 17:01 test\n-rw-r--r-- 1 1000 1000   70 Nov 18  2011 users\n");
    string respond("125 Data connection already open; transfer starting.\r\n");
    write(ses->csck, respond.c_str(), respond.length());

    write(ses->dsck, msg.c_str(), msg.length());

    string respond2("250 Requested file action okay, completed.\r\n");
    write(ses->csck, respond2.c_str(), respond2.length());

    close(ses->dsck);
    // string respond("200 Command okay.\r\n");
    // write(ses->csck, respond.c_str(), respond.length());
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

    if (isParamLegal) {
        string success("200 Command okay.\r\n");
        write(ses->csck, success.c_str(), success.length());
    } else {
        string fail("504 Command not implemented for that parameter.\r\n");
        write(ses->csck, fail.c_str(), fail.length());
    }    
}

void execPASV(session* ses, list<string> args) {
    cout << "poczatek serverDTPmainloo\n";

    int sck = socket(AF_INET, SOCK_STREAM, defaultProtocol);
    warnIfError(sck);

    sockaddr_in addr;

    memset(&addr, 0, sizeof(sockaddr_in)); // clear structure

    addr.sin_family = AF_INET;
    addr.sin_port = htons(dataConnectionPort);

    int result = bind(sck, (sockaddr*) &addr, sizeof(sockaddr_in));
    warnIfError(result);

    cout << "w dtp main loop, przed listen\n";
    result = listen(sck, connectionsQueueLength);
    warnIfError(result);
    cout << "w dtp main loop, po listen\n";

    ses->dsck = sck;
    // this doesn't use connection data port constant!
    string respond("227 Entering Passive Mode (127,0,0,1,4,127)\r\n");
    write(ses->csck, respond.c_str(), respond.length());
}

void openDataConnection(session* ses) {
    int dataSck = accept(ses->dsck, NULL, NULL);
    close(ses->dsck);
    ses->dsck = dataSck;
}