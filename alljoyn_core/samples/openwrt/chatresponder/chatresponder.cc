/*
 * Copyright (c) 2010-2012, AllSeen Alliance. All rights reserved.
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
 */

#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <queue>
#include <string>
#include <vector>


#include <alljoyn/BusAttachment.h>
#include <alljoyn/BusObject.h>
#include <alljoyn/InterfaceDescription.h>
#include <alljoyn/SessionPortListener.h>
#include <alljoyn/Status.h>
#include <alljoyn/version.h>

using namespace ajn;
using namespace std;

#define SignalHander(_a) static_cast<MessageReceiver::SignalHandler>(_a)


static const char* CHAT_SERVICE_INTERFACE_NAME = "org.alljoyn.bus.samples.chat";
static const char* NAME_PREFIX = "org.alljoyn.bus.samples.chat.";
static const char* CHAT_SERVICE_OBJECT_PATH = "/chatService";
static const SessionPort CHAT_PORT = 27;

static const char* CHAT_XML_INTERFACE_DESCRIPTION =
    "<node name=\"/chatService\">"
    "  <interface name=\"org.alljoyn.bus.samples.chat\">"

    "    <signal name=\"Chat\">"
    "      <arg name=\"str\" type=\"s\"/>"
    "    </signal>"

    "  </interface>"
    "</node>";



class ChatResponder : private BusObject {
  public:
    ChatResponder(BusAttachment& bus);
    ~ChatResponder();


  private:
    BusAttachment& bus;
    const InterfaceDescription::Member* chat;

    pthread_t responderThread;
    queue<Message> messages;
    pthread_mutex_t lock;
    pthread_cond_t queueChanged;

    bool stopping;

    void HandleChat(const InterfaceDescription::Member* member, const char* srcPath, Message& message);

    static void* ResponderThreadWrapper(void* arg);
    void ResponderThread();

    static string RunCommand(const char* cmd, ...);
};


ChatResponder::ChatResponder(BusAttachment& bus) :
    BusObject(CHAT_SERVICE_OBJECT_PATH),
    bus(bus),
    stopping(false)
{
    pthread_mutex_init(&lock, NULL);
    pthread_cond_init(&queueChanged, NULL);

    const InterfaceDescription* ifc = bus.GetInterface(CHAT_SERVICE_INTERFACE_NAME);
    if (!ifc) {
        bus.CreateInterfacesFromXml(CHAT_XML_INTERFACE_DESCRIPTION);
        ifc = bus.GetInterface(CHAT_SERVICE_INTERFACE_NAME);
    }

    assert(ifc);

    chat = ifc->GetMember("Chat");

    AddInterface(*ifc);
    bus.RegisterBusObject(*this);
    bus.RegisterSignalHandler(this, SignalHandler(&ChatResponder::HandleChat), chat, NULL);

    pthread_create(&responderThread, NULL, ResponderThreadWrapper, this);
}


ChatResponder::~ChatResponder()
{
    pthread_mutex_lock(&lock);
    while (!messages.empty()) {
        messages.pop();
    }
    stopping = true;
    pthread_cond_signal(&queueChanged);
    pthread_mutex_unlock(&lock);
    pthread_join(responderThread, NULL);

    bus.UnregisterSignalHandler(this, SignalHandler(&ChatResponder::HandleChat), chat, NULL);
    bus.UnregisterBusObject(*this);

    pthread_cond_destroy(&queueChanged);
    pthread_mutex_destroy(&lock);
}


void ChatResponder::HandleChat(const InterfaceDescription::Member* member, const char* srcPath, Message& message)
{
    pthread_mutex_lock(&lock);
    messages.push(message);
    pthread_cond_signal(&queueChanged);
    pthread_mutex_unlock(&lock);
}


void* ChatResponder::ResponderThreadWrapper(void* arg)
{
    ChatResponder* cr = reinterpret_cast<ChatResponder*>(arg);
    assert(cr);
    cr->ResponderThread();
    return NULL;
}


void ChatResponder::ResponderThread()
{
    pthread_mutex_lock(&lock);
    while (!stopping) {
        while (!messages.empty()) {
            Message message = messages.front();
            messages.pop();
            pthread_mutex_unlock(&lock);

            const char* msg;
            SessionId sessionId = message->GetSessionId();
            string resp;

            message->GetArgs("s", &msg);

            printf("Received from %s: %s\n", message->GetSender(), msg);

            if (strcmp(msg, "query name") == 0) {
                resp = RunCommand("uname", "-n", NULL);
            } else if (strcmp(msg, "query ip") == 0) {
                resp = RunCommand("ifconfig", NULL);
            } else if (strcmp(msg, "query uptime") == 0) {
                resp = RunCommand("uptime", NULL);
            } else if ((strcmp(msg, "query alljoyn") == 0) || (strcmp(msg, "query aj") == 0)) {
                resp = strdup(GetBuildInfo());
            } else if (strcmp(msg, "query openwrt") == 0) {
                resp = RunCommand("uname", "-a", NULL);
            } else {
                resp = "Ignoring Message: \"";
                resp += msg;
                resp += "\"";
            }

            MsgArg arg("s", resp.c_str());
            Signal(NULL, sessionId, *chat, &arg, 1);
            pthread_mutex_lock(&lock);
        }
        pthread_cond_wait(&queueChanged, &lock);
    }
    pthread_mutex_unlock(&lock);
}


string ChatResponder::RunCommand(const char* cmd, ...)
{
    char buf[4096];
    string result;
    ssize_t rd;
    int ret;
    int pfd[2];
    pid_t child;
    int status;

    ret = pipe(pfd);
    if (ret < 0) {
        ret = snprintf(buf, sizeof(buf) - 1, "Failed to create pipe for \"%s\": %s", cmd, strerror(errno));
        result = string(buf, ret);
        goto exit;
    }

    child = fork();

    if (child == 0) {
        va_list argp;
        vector<char*> argv;
        argv.push_back(strdup(cmd));
        va_start(argp, cmd);
        const char* arg = NULL;
        ssize_t wt;

        while ((arg = va_arg(argp, char*)) != NULL) {
            argv.push_back(strdup(arg));
        }
        va_end(argp);
        argv.push_back(NULL);

        ret = dup2(pfd[1], STDOUT_FILENO);
        if (ret < 0) {
            ret = snprintf(buf, sizeof(buf) - 1, "Failed redirect stdout for \"%s\": %s", cmd, strerror(errno));
            wt = 0;
            while (wt < ret) {
                wt += write(pfd[1], buf + wt, ret - wt);
            }
            close(pfd[0]);
            close(pfd[1]);
            for (size_t i = 0; i < argv.size() - 1; ++i) {
                free(argv[i]);
            }
            exit(1);
        }

        close(pfd[0]);
        close(pfd[1]);

        execvp(cmd, &argv.front());

        // If we get here, then executing cmd failed.
        ret = snprintf(buf, sizeof(buf) - 1, "Failed to execute \"%s\": %s", cmd, strerror(errno));
        wt = 0;
        while (wt < ret) {
            wt += write(pfd[1], buf + wt, ret - wt);
        }

        for (size_t i = 0; i < argv.size() - 1; ++i) {
            free(argv[i]);
        }
        exit(1);
    } else if (child > 0) {
        close(pfd[1]);

        while ((rd = read(pfd[0], buf, sizeof(buf))) > 0) {
            result += string(buf, rd);
        }
        result.erase(result.find_last_not_of(" \b\v\t\r\n") + 1);

        close(pfd[0]);

        waitpid(child, &status, 0);
    } else {
        ret = snprintf(buf, sizeof(buf) - 1, "Failed to spawn process for \"%s\": %s", cmd, strerror(errno));
        result = string(buf, ret);
    }

exit:
    return result;
}





class SessionManager : private SessionPortListener {
  public:
    SessionManager(BusAttachment& bus);
    ~SessionManager();

  private:
    BusAttachment& bus;
    SessionPort sessionPort;
    string name;

    bool AcceptSessionJoiner(SessionPort sessionPort, const char* joiner, const SessionOpts& opts);
};



SessionManager::SessionManager(BusAttachment& bus) : bus(bus), sessionPort(CHAT_PORT), name(NAME_PREFIX)
{
    SessionOpts opts(SessionOpts::TRAFFIC_MESSAGES, true, SessionOpts::PROXIMITY_ANY, TRANSPORT_ANY);

    name += "OpenWRT_";
    name += bus.GetGlobalGUIDString().substr(0, 8).c_str();

    bus.RequestName(name.c_str(), DBUS_NAME_FLAG_DO_NOT_QUEUE);
    bus.BindSessionPort(sessionPort, opts, *this);
    bus.AdvertiseName(name.c_str(), TRANSPORT_ANY);
}


SessionManager::~SessionManager()
{
    bus.CancelAdvertiseName(name.c_str(), TRANSPORT_ANY);
    bus.UnbindSessionPort(sessionPort);
}



bool SessionManager::AcceptSessionJoiner(SessionPort sessionPort, const char* joiner, const SessionOpts& opts)
{
    return true;
}




static volatile sig_atomic_t quit;

static void SignalHandler(int sig)
{
    switch (sig) {
    case SIGINT:
    case SIGTERM:
        quit = 1;
        break;
    }
}

int main(int argc, char** argv)
{
    struct sigaction act, oldact;
    sigset_t sigmask, waitmask;

    // Block all signals by default for all threads.
    sigfillset(&sigmask);
    sigdelset(&sigmask, SIGSEGV);
    pthread_sigmask(SIG_BLOCK, &sigmask, NULL);

    // Setup a handler for SIGINT and SIGTERM
    act.sa_handler = SignalHandler;
    sigemptyset(&act.sa_mask);
    act.sa_flags = SA_SIGINFO | SA_RESTART;
    sigaction(SIGINT, &act, &oldact);
    sigaction(SIGTERM, &act, &oldact);


    // Setup and start the real application.
    BusAttachment bus("Chat Responder", true);

    bus.Start();
    bus.Connect();

    ChatResponder* cr = new ChatResponder(bus);
    SessionManager* sm = new SessionManager(bus);

    // Setup signals to wait for.
    sigfillset(&waitmask);
    sigdelset(&waitmask, SIGINT);
    sigdelset(&waitmask, SIGTERM);

    quit = 0;

    while (!quit) {
        // Wait for a signal.
        sigsuspend(&waitmask);
    }

    delete sm;
    delete cr;

    // Shutdown and clean up the real application.
    bus.Stop();
    bus.Join();

    return 0;
}
