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
                break;
            }
        }

        // give proper respond
        string success("331 User name okay, need password.\r\n");
        string fail("532 Need account for storing files.\r\n");
        if (isFound) {
            write(ses->csck, success.c_str(), success.length());
        } else {
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

        list<string> passwords;
        string user;
        while (ifs.good()) {
            ifs >> user; // ignore user name

            ifs >> user;
            passwords.push_back(user);
        }

        if (passwords.size() == 0) {
            throw string("Error with users.txt file");
        }

        // find given password in list
        bool isFound = false;
        list<string>::iterator it;
        for (it = passwords.begin(); it != passwords.end(); it++) {
            if (*it == args.front()) {
                isFound = true;
                break;
            }
        }

        // give proper respond
        string success("230 User logged in, proceed.\r\n");
        string fail("530 Not logged in.\r\n");
        if (isFound) {
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

