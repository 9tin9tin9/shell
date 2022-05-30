#include <stdio.h>
#include <unistd.h>
#include <string>
#include <vector>
#include <unordered_map>
#include "config.h"

using Args = std::vector<const char*>;
void cd(const Args& args);
void exit(const Args& args);
void help(const Args& args);

using builtin_func_type = void(*)(const Args&);
std::unordered_map<std::string, builtin_func_type> builtin_funcs = {
    {"cd", cd},
    {"help", help},
    {"exit", exit},
};

void cd(const Args& args){
    const char* path = args.size() > 1 ? args[1] : "";
    if (chdir(path) == -1){
        fprintf(stderr, "%s\n", strerror(errno));
    }
}

void exit(const Args& args){
    exit(0);
}

void help(const Args& args){
    puts("An extremely simple shell that should not be used daily");
    puts("builtin commands:");
    for (auto f : builtin_funcs){
        puts(f.first.c_str());
    }
}

std::string readline(FILE* stream){
    std::string line;
    bool escaped = false;
    bool end = false;
    int c;
    while((c = fgetc(stream)) != EOF){
        line.push_back(c);
        if (c == '\n' && !escaped){
            break;
        }
        escaped = c == '\\' && !escaped;
    }
    if (feof(stream)){
        exit(0);
    }
    if (ferror(stream)){
        perror("fgetc");
        exit(1);
    }
    return line;
}

std::vector<const char*> parse(const std::string& line){
    std::vector<const char*> args;
    std::string word, spaces = SPACES;
    for (auto c : line){
        if (spaces.find(c) != std::string::npos){
            args.push_back(strdup(word.c_str()));
            word.clear();
        }else{
            word.push_back(c);
        }
    }
    args.push_back(NULL);
    return args;
}

int builtin(const Args& args){
    auto it = builtin_funcs.find(args[0]);
    if (it != builtin_funcs.end()){
        it->second(args);
        return 1;
    }else{
        return 0;
    }
}

void spawn(const Args& args){
    auto pid = fork();
    int stat_loc;
    if (pid == -1){
        // error
        perror("fork");
        exit(1);
    }else if (pid == 0){
        // child
        if (execvp(args[0], (char**)args.data()) == -1){
            perror(args[0]);
            exit(1);
        }
    }else{
        // parent
        if (wait(&stat_loc) == -1){
            perror("wait");
            exit(1);
        }
    }
}

int main(int argc, char** argv){

    while(1){
        printf("%s", PS1);
        std::string line = readline(stdin);
        std::vector<const char*> args = parse(line);

        if (!builtin(args)){
            spawn(args);
        }

        std::for_each(args.begin(), args.end(),
            [](auto s){
                free((void*)s);
            });
    }
    return 0;
}
