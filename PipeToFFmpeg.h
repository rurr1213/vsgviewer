#include <fstream>
#include <iostream>
#include <unistd.h>  // For pipe and fork
#include <sys/wait.h> // For waitpid
#include <vector>
#include <cstdint>

class PipeToFFmpeg {
private:
    FILE *pipe_in;

public:
    PipeToFFmpeg() : pipe_in(nullptr) {}

    ~PipeToFFmpeg() {
        deinit();
    }

    bool init(int width, int height) {
        // Create a pipe
        int pipefd[2];
        if (pipe(pipefd) == -1) {
            perror("pipe");
            std::cout << "pipe create failed in parent" << std::endl;
            return false;
        }


        pid_t pid = fork();
        if (pid == -1) {
            perror("fork");
            return false;
        }

        if (pid == 0) { // Child process
            // Redirect stdin to the read end of the pipe
            close(pipefd[1]);  // Close the write end in the child
            dup2(pipefd[0], STDIN_FILENO);
            close(pipefd[0]);

            // Construct and execute the ffmpeg command.  Adjust as needed!
            // Important: -c copy to avoid ffmpeg re-encoding
//           execlp("ffmpeg", "ffmpeg", "-re", "-i", "-", "-c", "copy", "-f", "rtp", "rtp://172.18.0.3:5018",  NULL);
           execlp("ffmpeg", "ffmpeg",
                "-re",
                "-i",
                "-",
                "-analyzeduration", "0",
                "-probesize", "32",
                "-flush_packets", "1",
                "-g", "1",
                "-c", "copy",
                "-f",
                "rtp", "rtp://172.18.0.3:5018",
                "-loglevel", "error",  // Suppress frame rate and fps reporting
            NULL);


            perror("execlp"); // This should only be reached on error
            exit(1);
        } else { // Parent process
            close(pipefd[0]);  // Close the read end in the parent
            pipe_in = fdopen(pipefd[1], "wb");
            if (!pipe_in) {
                perror("fdopen");
                std::cout << "pipe open failed in parent" << std::endl;
                return false;
            }
        }
        return true;
    }



    void encodeAndStream(std::vector<uint8_t>& packet) {
        if (!pipe_in) return;

        // Write the encoded packet to the pipe
        if (fwrite(packet.data(), 1, packet.size(), pipe_in) != packet.size()) {
            perror("fwrite");
        }
        fflush(pipe_in); // Ensure data is sent immediately
    }

    bool deinit() {
        if (pipe_in) {
            fclose(pipe_in);
            pipe_in = nullptr;
            wait(NULL);  //IMPORTANT!
        }
        return true;
    }
};