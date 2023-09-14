// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2023 Darby Johnston
// All rights reserved.

#include <tlApp/IApp.h>

#include <tlCore/String.h>
#include <tlCore/StringFormat.h>

#include <iostream>

namespace tl
{
    namespace app
    {
        std::vector<std::string> convert(int argc, char* argv[])
        {
            std::vector<std::string> out;
            for (int i = 0; i < argc; ++i)
            {
                out.push_back(argv[i]);
            }
            return out;
        }

        std::vector<std::string> convert(int argc, wchar_t* argv[])
        {
            std::vector<std::string> out;
            for (int i = 0; i < argc; ++i)
            {
                out.push_back(string::fromWide(argv[i]));
            }
            return out;
        }

        struct IApp::Private
        {
            struct CmdLineData
            {
                std::vector<std::string> argv;
                std::string name;
                std::string summary;
                std::vector<std::shared_ptr<ICmdLineArg> > args;
                std::vector<std::shared_ptr<ICmdLineOption> > options;
            };
            CmdLineData cmdLine;

            std::shared_ptr<observer::ListObserver<log::Item> > logObserver;
        };

        void IApp::_init(
            const std::vector<std::string>& argv,
            const std::shared_ptr<system::Context>& context,
            const std::string& cmdLineName,
            const std::string& cmdLineSummary,
            const std::vector<std::shared_ptr<ICmdLineArg> >& cmdLineArgs,
            const std::vector<std::shared_ptr<ICmdLineOption> >& cmdLineOptions)
        {
            TLRENDER_P();

            _context = context;

            // Parse the command line.
            for (size_t i = 1; i < argv.size(); ++i)
            {
                p.cmdLine.argv.push_back(argv[i]);
            }
            p.cmdLine.name = cmdLineName;
            p.cmdLine.summary = cmdLineSummary;
            p.cmdLine.args = cmdLineArgs;
            p.cmdLine.options = cmdLineOptions;
            p.cmdLine.options.push_back(CmdLineFlagOption::create(
                _options.log,
                { "-log" },
                "Print the log to the console."));
            p.cmdLine.options.push_back(CmdLineFlagOption::create(
                _options.help,
                { "-help", "-h", "--help", "--h" },
                "Show this message."));
            _exit = _parseCmdLine();

            // Setup the log.
            if (_options.log)
            {
                p.logObserver = observer::ListObserver<log::Item>::create(
                    context->getSystem<log::System>()->observeLog(),
                    [this](const std::vector<log::Item>& value)
                    {
                        const size_t options =
                            static_cast<size_t>(log::StringConvert::Time) |
                            static_cast<size_t>(log::StringConvert::Prefix);
                        for (const auto& i : value)
                        {
                            _print("[LOG] " + toString(i, options));
                        }
                    },
                    observer::CallbackAction::Suppress);
            }
        }
        
        IApp::IApp() :
            _p(new Private)
        {}

        IApp::~IApp()
        {}

        const std::shared_ptr<system::Context>& IApp::getContext() const
        {
            return _context;
        }

        int IApp::getExit() const
        {
            return _exit;
        }

        void IApp::_log(const std::string& value, log::Type type)
        {
            _context->log(_p->cmdLine.name, value, type);
        }

        void IApp::_print(const std::string& value)
        {
            std::cout << value << std::endl;
        }

        void IApp::_printNewline()
        {
            std::cout << std::endl;
        }

        void IApp::_printError(const std::string& value)
        {
            std::cerr << "ERROR: " << value << std::endl;
        }

        int IApp::_parseCmdLine()
        {
            TLRENDER_P();
            for (const auto& i : p.cmdLine.options)
            {
                try
                {
                    i->parse(p.cmdLine.argv);
                }
                catch (const std::exception& e)
                {
                    throw std::runtime_error(string::Format("Cannot parse option \"{0}\": {1}").
                        arg(i->getMatchedName()).
                        arg(e.what()));
                }
            }
            size_t requiredArgs = 0;
            size_t optionalArgs = 0;
            for (const auto& i : p.cmdLine.args)
            {
                if (!i->isOptional())
                {
                    ++requiredArgs;
                }
                else
                {
                    ++optionalArgs;
                }
            }
            if (p.cmdLine.argv.size() < requiredArgs ||
                p.cmdLine.argv.size() > requiredArgs + optionalArgs ||
                _options.help)
            {
                _printCmdLineHelp();
                return 1;
            }
            for (const auto& i : p.cmdLine.args)
            {
                try
                {
                    if (!(p.cmdLine.argv.empty() && i->isOptional()))
                    {
                        i->parse(p.cmdLine.argv);
                    }
                }
                catch (const std::exception& e)
                {
                    throw std::runtime_error(string::Format("Cannot parse argument \"{0}\": {1}").
                        arg(i->getName()).
                        arg(e.what()));
                }
            }
            return 0;
        }

        void IApp::_printCmdLineHelp()
        {
            TLRENDER_P();
            _print("\n" + p.cmdLine.name + "\n");
            _print("    " + p.cmdLine.summary + "\n");
            _print("Usage:\n");
            {
                std::stringstream ss;
                ss << "    " + p.cmdLine.name;
                if (p.cmdLine.args.size())
                {
                    std::vector<std::string> args;
                    for (const auto& i : p.cmdLine.args)
                    {
                        const bool optional = i->isOptional();
                        args.push_back(
                            (optional ? "[" : "(") +
                            string::toLower(i->getName()) +
                            (optional ? "]" : ")"));
                    }
                    ss << " " << string::join(args, " ");
                }
                if (p.cmdLine.options.size())
                {
                    ss << " [option],...";
                }
                _print(ss.str());
                _printNewline();
            }
            _print("Arguments:\n");
            for (const auto& i : p.cmdLine.args)
            {
                _print("    " + i->getName());
                _print("        " + i->getHelp());
                _printNewline();
            }
            _print("Options:\n");
            for (const auto& i : p.cmdLine.options)
            {
                bool first = true;
                for (const auto& j : i->getHelpText())
                {
                    if (first)
                    {
                        first = false;
                        _print("    " + j);
                    }
                    else
                    {
                        _print("        " + j);
                    }
                }
                _printNewline();
            }
        }
    }
}
