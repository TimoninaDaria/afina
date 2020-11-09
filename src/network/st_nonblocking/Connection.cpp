#include "Connection.h"

#include <cassert>
#include <unistd.h>

#include <iostream>

namespace Afina {
namespace Network {
namespace STnonblock {


// See Connection.h
void Connection::DoRead() {
    try {
        if ((bytes = read(_socket, read_buf + read_end, buf_size - read_end)) > 0) {
            read_end += bytes;
            while (read_end - read_begin > 0) {
                if (!command_to_execute) {
                    std::size_t parsed = 0;
                    if (parser.Parse(read_buf + read_begin, read_end - read_begin, parsed)) {
                        command_to_execute = parser.Build(arg_remains);
                        if (arg_remains > 0) {
                            arg_remains += 2;
                        }
                    }

                    if (parsed == 0) {
                        break;
                    } else {
                        read_begin += parsed;
                    }
                }
                if (command_to_execute && arg_remains > 0) {
                    std::size_t to_read = std::min(arg_remains, read_end - read_begin);
                    command_param.append(read_buf + read_begin, to_read);

                    arg_remains -= to_read;
                    read_begin += to_read;
                }

                if (command_to_execute && arg_remains == 0) {
                    std::string result;
                    if (command_param.size()) {
                        command_param.resize(command_param.size() - 2);
                    }
                    command_to_execute->Execute(*pStorage, command_param, result);

                    result += "\r\n";
                    answers.push_back(std::move(result));
                    if (!(_event.events & EPOLLOUT)) {
                        _event.events |= EPOLLOUT;
                    }

                    command_to_execute.reset();
                    command_param.resize(0);
                    parser.Reset();
                }
            }
            if (read_begin == read_end) {
                read_begin = read_end = 0;
            } else if (read_end == buf_size) {
                std::memmove(read_buf, read_buf + read_begin, read_end - read_begin);
            }
        } else {
            check_alive = false;
        }
    } catch (std::runtime_error &ex) {
        answers.push_back("ERROR\r\n");
        if (!(_event.events & EPOLLOUT)) {
            _event.events |= EPOLLOUT;
        }
    }
}


// See Connection.h
void Connection::Start() {
    check_alive = true;
    read_begin = read_end = 0;
    offset = 0;
    _event.events = EPOLLIN;
}

// See Connection.h
void Connection::OnError() {
    check_alive = false;
    _event.events = 0;
}

// See Connection.h
void Connection::OnClose() {
    check_alive = false;
    _event.events = 0;
}

// See Connection.h
void Connection::DoWrite() {
    static constexpr size_t write_size = 64;
    iovec write[write_size];
    size_t write_v = 0;
    {
        auto it = answers.begin();
        write[write_v].iov_base = &((*it)[0]) + offset;
        write[write_v].iov_len = it->size() - offset;
        it++;
        write_v++;
        for (; it != answers.end(); it++) {
            write[write_v].iov_base = &((*it)[0]);
            write[write_v].iov_len = it->size();
            if (++write_v >= write_size) {
                break;
            }
        }
    }

    int writed;
    if ((writed = writev(_socket, write, write_v)) > 0) {
        size_t i = 0;
        while (i < write_v && writed >= write[i].iov_len) {
            answers.pop_front();
            writed -= write[i].iov_len;
            i++;
        }
        offset = writed;
    } else {
        check_alive = false;
    }

    if (answers.empty()) {
        _event.events &= ~EPOLLOUT;
    }
}

} // namespace STnonblock
} // namespace Network
} // namespace Afina
