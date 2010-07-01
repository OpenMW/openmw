#ifndef COMMANDSERVER_COMMAND_HPP
#define COMMANDSERVER_COMMAND_HPP

namespace OMW
{
    ///
    /// A Command is currently defined as a string input that, when processed,
    /// will generate a string output.  The string output is passed to the
    /// mReplyFunction as soon as the command has been processed.
    ///
    class Command
    {
    public:
        std::string                         mCommand;
        boost::function1<void, std::string> mReplyFunction;
    };
}

#endif COMMANDSERVER_COMMAND_HPP
