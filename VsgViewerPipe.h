#pragma once

#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <iostream>
#include <string>
#include <cstring>
#include <errno.h>
#include <chrono>
#include <thread>
#include <signal.h>

class VsgViewerPipe
{
public:
    enum Mode { SERVER, CLIENT };

    VsgViewerPipe(Mode mode)
    {
        signal(SIGPIPE, SIG_IGN);

        if (mode == SERVER)
        {
            inputPipe = "/tmp/vsgviewerout";
            outputPipe = "/tmp/vsgviewerin";
        }
        else
        {
            inputPipe = "/tmp/vsgviewerin";
            outputPipe = "/tmp/vsgviewerout";
        }

        // Create named pipes if they do not exist
        mkfifo(inputPipe.c_str(), 0666);
        mkfifo(outputPipe.c_str(), 0666);

        // Open pipes
        inputFd = open(inputPipe.c_str(), O_RDONLY | O_NONBLOCK);
        if (inputFd == -1)
        {
            std::cerr << "Failed to open input pipe: " << strerror(errno) << std::endl;
        }

        openOutputPipe();
    }

    ~VsgViewerPipe()
    {
        close(inputFd);
        close(outputFd);
        unlink(inputPipe.c_str());
        unlink(outputPipe.c_str());
    }

    ssize_t write(const std::string& message)
    {
        ssize_t bytesWritten = -1;
        if (outputFd == -1)
        {
            openOutputPipe();
        }
        try{

            // Check if the other end of the pipe is open
            int flags = fcntl(outputFd, F_GETFL);
            if (flags == -1)
            {
                // Failed to get flags, assume pipe is closed
                close(outputFd);
                outputFd = -1;
                return 0;
            }
            
            ssize_t bytesWritten = ::write(outputFd, message.c_str(), message.size());
            if (bytesWritten == -1)  {
                if (errno == EAGAIN || errno == EWOULDBLOCK)
                {
                    // Pipe is full, cannot write now
                    return -1;
                }
                else if (errno == EPIPE)
                {
                    // Pipe is closed, ignore and return 0 bytes written
                    return 0;
                }
                else
                {
                    std::cerr << "Failed to write to output pipe: " << strerror(errno) << std::endl;
                    return -1;
                }
            }
        }catch(...){
            bytesWritten = -1;
        }
        return bytesWritten;
    }

    ssize_t read(std::string& message)
    {
        char buffer[1024];
        ssize_t bytesRead = ::read(inputFd, buffer, sizeof(buffer) - 1);
        if (bytesRead > 0)
        {
            buffer[bytesRead] = '\0';
            message = buffer;
        }
        return bytesRead;
    }

private:
    bool openOutputPipe()
    {
        outputFd = open(outputPipe.c_str(), O_WRONLY | O_NONBLOCK);
        if (outputFd == -1)
        {
            return false;
        }
        return true;
    }

    std::string inputPipe;
    std::string outputPipe;
    int inputFd;
    int outputFd;
};