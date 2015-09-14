/******************************************************************************
 * Copyright AllSeen Alliance. All rights reserved.
 *
 *    Permission to use, copy, modify, and/or distribute this software for any
 *    purpose with or without fee is hereby granted, provided that the above
 *    copyright notice and this permission notice appear in all copies.
 *
 *    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 *    WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 *    MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 *    ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 *    WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 *    ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 *    OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 ******************************************************************************/
#include <unistd.h>
#include <vector>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

#include <qcc/Environ.h>
#include <qcc/Thread.h>

#include <alljoyn/Init.h>

#include "MPApplication.h"
#include "MPSecurityMngr.h"

using namespace secmgr_tests;
using namespace ajn::securitymgr;
using namespace ajn;
using namespace std;

static int be_peer()
{
    srand(time(nullptr) + (int)getpid());

    MPApplication app(getpid());

    QStatus status = app.Start();

    if (ER_OK != status) {
        cerr << "Failed to start Peer " << getpid()  << "." << endl;
        return EXIT_FAILURE;
    }

    status = app.WaitUntilFinished();
    cerr << "Peer " << getpid()  << " finished " << status << endl;

    return ER_OK == status ? EXIT_SUCCESS : EXIT_FAILURE;
}

static int be_secmgr(int peers)
{
    string storage_path;

    Environ::GetAppEnviron()->Add("STORAGE_PATH", "/tmp/mpsecmgr.db");
    storage_path = Environ::GetAppEnviron()->Find("STORAGE_PATH", "/tmp/mpsecmgr.db").c_str();
    remove(storage_path.c_str());

    MPSecurityMngr mgr;
    cout << "Starting secmgr" << endl;
    QStatus status = mgr.Start(peers);
    if (ER_OK != status) {
        cerr << "Secmgr: Failed to start the security manager " << status << endl;
        return EXIT_FAILURE;
    }
    cout << "waiting until finished secmgr" << endl;
    status = mgr.WaitUntilFinished();

    cout << "Secmgr " << getpid()  << " finished " << status << endl;
    if (ER_OK != status) {
        cerr << "Secmgr: WaitUntilFinished failed exiting: "  << status << endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

int CDECL_CALL main(int argc, char** argv)
{
    int status = true;
    int peers;
    if (argc == 1) {
        peers = 4;
    } else if (argc == 2) {
        peers = atoi(argv[1]);
    } else if (argc == 3) {
        if (AllJoynInit() != ER_OK) {
            return EXIT_FAILURE;
        }

    #ifdef ROUTER
        if (AllJoynRouterInit() != ER_OK) {
            AllJoynShutdown();
            return EXIT_FAILURE;
        }
    #endif
        int rc = EXIT_FAILURE;
        if (0 == strcmp(argv[1], "p")) {
            rc =  be_peer();
        } else {
            peers = atoi(argv[2]);
            rc = be_secmgr(peers);
        }
#ifdef ROUTER
        AllJoynRouterShutdown();
#endif

        AllJoynShutdown();

        return rc;
    }

    vector<pid_t> children(peers + 1);
    for (size_t i = 0; i < children.size(); ++i) {
        if ((children[i] = fork()) == 0) {
            cout << "pid = " << getpid() << endl;
            char* newArgs[4];
            newArgs[0] = argv[0];
            char buf[20];
            if (i < 1) {
                cout << "[MAIN] SecMgr needs " << peers << " peers." << endl;
                newArgs[1] = (char*)"mgr";
                snprintf(buf, 20, "%d", peers);
                newArgs[2] = buf;
            } else {
                newArgs[1] = (char*)"p";
                newArgs[2] = (char*)"10";
            }
            newArgs[3] = nullptr;
            execv(argv[0], newArgs);
            cerr << "[MAIN] Exec fails." <<  endl;
            return EXIT_FAILURE;
        } else if (children[i] == -1) {
            perror("fork");
            return EXIT_FAILURE;
        }
        //add some delay before starting the next process
        //to avoid too much concurrency during start-up.
        qcc::Sleep(100);
    }
    if (AllJoynInit() != ER_OK) {
        return EXIT_FAILURE;
    }

    pid_t mngr_pid = children[0];

    cout << "Main test: waiting for " << children.size() << " children" << endl;
    while (children.size()) {
        cout << "Main: waiting for children to stop " << endl;
        int rv;
        pid_t pid = waitpid(0, &rv, 0);
        if (pid < 0) {
            cerr << "could not wait for PID" << endl;
            perror("waitpid");
            status = false;
            break;
        }
        vector<pid_t>::iterator it = children.begin();
        for (; it != children.end(); it++) {
            if (*it == rv) {
                children.erase(it);
                break;
            }
        }

        if (!WIFEXITED(rv) || WEXITSTATUS(rv) != EXIT_SUCCESS) {
            cerr << "Main: waiting for " << rv << " WIFEXITED(rv)= " << WIFEXITED(rv) << " " <<
                WEXITSTATUS(rv) << endl;
            cerr << "Main: waiting for " << rv << " WISIGNALED(rv)= " << WIFSIGNALED(rv) << " " <<
                WTERMSIG(rv) << endl;
            status = false;
            break;
        }
        cout << "Main: process " << pid << " fininshed successful. " << rv << endl;
        if (mngr_pid == pid) {
            break;
        }
    }

    if (children.size() != 0) {
        for (size_t t = 0; t < children.size(); t++) {
            kill(children[t], SIGTERM);
        }
        qcc::Sleep(2000);
        for (size_t t = 0; t < children.size(); t++) {
            kill(children[t], SIGKILL);
        }
    }
    if (status == false) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
