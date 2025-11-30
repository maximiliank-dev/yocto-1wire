#include <fstream>
#include <iostream>
#include <poll.h>
#include <sstream>
#include <string>

#include <cerrno>
#include <csignal>
#include <sys/stat.h>
#include <syslog.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <memory>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "constants.h"
#include "logger.h"
#include <cstring>
#include <vector>

#define LOG_FILE "onewire_log.txt"
#define DRIVER_PATH "/dev/onewire_dev"

#define PORT 1033
#define BUFFER_SIZE 512

volatile sig_atomic_t stop = 0;

int server_fd, new_socket;
struct sockaddr_in server_addr, client_addr;

/**
 * Signal handling
 * Has a C style interface
 */
extern "C" void
handle_signal (int sig)
{
    if (sig == SIGTERM || sig == SIGINT)
    {
        stop = 1;
    }
}

std::vector<char>
convert_to_bvec (std::string in)
{
    std::vector<char> r;
    r.reserve (in.length ());

    for (char c : in)
    {
        r.push_back (c);
    }
    return r;
}

/**
 * Creates a tcp server
 */
void
create_server (logger::Logger &log)
{
    int opt = 1;

    // Creating socket file descriptor
    if ((server_fd = socket (AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror ("socket failed");
        exit (EXIT_FAILURE);
    }
    // Forcefully attaching socket to the port 8080
    if (setsockopt (server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof (opt)))
    {
        perror ("setsockopt");
        exit (EXIT_FAILURE);
    }
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons (PORT);
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons (PORT);

    // Bind the socket to the network address and port
    if (bind (server_fd, (struct sockaddr *)&server_addr, sizeof (server_addr)) < 0)
    {
        perror ("bind failed");
        exit (EXIT_FAILURE);
    }
    // Start listening for incoming connections
    if (listen (server_fd, 1) < 0)
    {
        perror ("listen");
        exit (EXIT_FAILURE);
    }

    log.log ("Created TCP server");
    log.log ("Server listening on port ", PORT);
}

/**
 * Accepts a new connection
 */
int
wait_for_conenction (logger::Logger &log)
{

    struct pollfd pfd{ .fd = server_fd, .events = POLLIN };
    socklen_t client_addr_len = sizeof (client_addr);

    while (!stop)
    {
        int rcv = poll (&pfd, 1, 300); // 300ms

        if (rcv < 0)
        {
            return -1;
        }

        // if rcv == 0 => no connection was detected
        if (rcv > 0)
        {

            // Accept incoming connection
            new_socket = accept (server_fd, (struct sockaddr *)&client_addr, &client_addr_len);

            if (new_socket < 0)
            {
                perror ("accept");
                int err = errno;
                log.log ("Error ", std::strerror (err));
                return -1;
            }
            return 0;
        }
    }
    return 0;
}

/**
 * opens the driver and writes all commands
 * then read all commands and returns the result
 */
std::string
write_commands (const std::vector<char> &command, std::string device_name)
{
    std::string ret{ "" };
    std::fstream fs;
    fs.open (device_name, std::fstream::out);

    if (!fs.is_open ())
    {
        throw std::runtime_error ("Error: onewire_driver is not open");
    }

    // write the command
    for (char c : command)
    {
        fs.put (c);
    }
    fs.close ();

    // wait some time
    sleep (1);

    fs.open (device_name, std::fstream::in);

    // read the result
    char c;
    while (fs.get (c))
    {
        ret.push_back (c);
    }
    fs.close ();

    return ret;
}

/**
 * Default way of creating a demon
 */
void
demonize ()
{
    pid_t pid;

    pid = fork ();
    if (pid < 0)
    {
        perror ("fork");
        exit (EXIT_FAILURE);
    }

    if (pid < 0)
    {
        perror ("fork");
        exit (EXIT_FAILURE);
    }
    else if (pid > 0)
    {
        exit (EXIT_SUCCESS);
    } // return ;

    pid_t sid = setsid ();
    if (sid < 0)
    {
        perror ("Error in setsid");
        exit (EXIT_FAILURE);
    }

    if ((chdir ("/")) < 0)
    {
        perror ("Error in chdir");
        exit (EXIT_FAILURE);
    }

    stop = 0;

    close (STDIN_FILENO);
    close (STDOUT_FILENO);
    close (STDERR_FILENO);
}

/**
 * Arguments:
 * -m: send the measure temperature command
 * -r: send a read scratchpad command
 * [arg1 ]: Sends the command string to the 1-Wire driver
 * else: starts the TCP server
 */
int
main (int argc, char *argv[])
{

    // register the signals
    signal (SIGTERM, handle_signal);
    signal (SIGINT, handle_signal);

    std::unique_ptr<logger::LogCout> sink = std::make_unique<logger::LogCout> ();
    logger::Logger log (std::move (sink));

    // parse the arguments
    if (argc > 1)
    {
        if (std::string (argv[1]).compare ("-m") == 0)
        { // measure temperature
            log.log ("measure temp");
            std::string s = write_commands ({ 'C', 'T' }, DRIVER_PATH);
            log.log ("Got ", s);
        }
        else if (std::string (argv[1]).compare ("-r") == 0)
        { // read scratchpad
            std::string s = write_commands ({ 'R', 'S' }, DRIVER_PATH);
            log.log ("Got ", s);
            std::ostringstream os;
            for (size_t i = 0; i < s.size (); ++i)
            {
                os << std::hex << (int)s[i] << " ";
            }
            log.log ("Got 0x", os.str ());
        }
        else
        { // write a chain of commands
            std::string argument = "";
            std::stringstream stream;
            std::vector<char> char_arr;

            if (argc > 2)
            {
                log.log ("Warning only argv[1] is used all others are ignored");
            }

            int k = 0;
            char c = argv[1][k];
            while (c != '\0')
            {
                char_arr.push_back (c);
                stream << std::hex << c;

                c = argv[1][++k];
            }
            log.log ("converted ", stream.str ());
            write_commands (char_arr, DRIVER_PATH);
        }
    }
    else
    {
        log.log ("TCP server");

        create_server (log);

        log.log ("Accept server");
        while (!stop)
        {
            int ret = wait_for_conenction (log);

            if (stop || ret < 0)
                break;

            char client_ip[INET_ADDRSTRLEN];
            inet_ntop (AF_INET, &(client_addr.sin_addr), client_ip, INET_ADDRSTRLEN);

            int client_port = ntohs (client_addr.sin_port);
            log.log ("connection accepted at port ", client_port);

            char bufa[256];
            const int buf_size = 256;
            ssize_t bytes_read;

            while (!stop && (bytes_read = read (new_socket, bufa, buf_size - 1)))
            {

                log.log ("Reading...");
                if (bytes_read > 0)
                {
                    bufa[bytes_read] = '\0';
                }
                else if (bytes_read == 0)
                {
                    log.log ("Closing connection");
                    break;
                }
                else
                {
                    throw std::runtime_error ("Error reading buffer got "
                                              + std::to_string (bytes_read));
                }
                std::string buf;

                for (int i = 0; i < bytes_read; i++)
                {
                    buf.push_back (bufa[i]);
                }

                log.log ("Got ", bytes_read, " ", buf);
                std::vector<char> char_arr = convert_to_bvec (buf);

                std::string s = write_commands (char_arr, "/dev/onewire_dev");

                log.log ("send string", s);
                write (new_socket, s.c_str (), s.length ());
            }
            close (new_socket);
            log.log ("socket closed");
        }
        close (server_fd);
    }

    return 0;
}
