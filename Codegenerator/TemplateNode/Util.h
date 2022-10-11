#pragma once

bool isConnected();

namespace Loglevel
{
enum Loglevel
{
    status, debug, error
};
}

void log(const char* message, Loglevel::Loglevel loglevel);